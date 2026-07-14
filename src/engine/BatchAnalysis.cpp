#include "BatchAnalysis.h"

#include "GtpProtocol.h"
#include "KataGoRules.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

#include <algorithm>
#include <array>
#include <charconv>
#include <cmath>
#include <cctype>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>

namespace lizzie::engine {

namespace {

void collectDepthFirst(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    std::vector<lizzie::core::NodeId>* result)
{
    const lizzie::core::GameNode* node = model.findNode(nodeId);
    if (node == nullptr) {
        return;
    }
    result->push_back(nodeId);
    for (lizzie::core::NodeId child : node->children) {
        collectDepthFirst(model, child, result);
    }
}

void appendInitialStone(
    std::vector<InitialStone>* stones,
    lizzie::core::Color color,
    lizzie::core::Point point)
{
    const auto existing = std::ranges::find_if(*stones, [point](const InitialStone& stone) {
        return stone.point == point;
    });
    if (existing != stones->end()) {
        if (existing->color == color) {
            return;
        }
        stones->erase(existing);
    }
    stones->push_back(InitialStone{color, point});
}

void appendInitialStones(
    std::vector<InitialStone>* stones,
    const lizzie::core::SetupStones& setup)
{
    for (lizzie::core::Point point : setup.black) {
        appendInitialStone(stones, lizzie::core::Color::Black, point);
    }
    for (lizzie::core::Point point : setup.white) {
        appendInitialStone(stones, lizzie::core::Color::White, point);
    }
}

std::string nodeLabel(lizzie::core::NodeId nodeId)
{
    std::ostringstream stream;
    stream << "node " << nodeId;
    return stream.str();
}

std::optional<lizzie::core::NodeId> firstResignNodeInPath(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId)
{
    for (lizzie::core::NodeId pathNodeId : model.pathTo(nodeId)) {
        const lizzie::core::GameNode& node = model.node(pathNodeId);
        if (node.move.has_value() && node.move->type == lizzie::core::MoveType::Resign) {
            return pathNodeId;
        }
    }
    return std::nullopt;
}

std::optional<lizzie::core::NodeId> lastNonRootSetupNodeInPath(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId)
{
    std::optional<lizzie::core::NodeId> result;
    const std::vector<lizzie::core::NodeId> path = model.pathTo(nodeId);
    for (std::size_t index = 0; index < path.size(); ++index) {
        const lizzie::core::GameNode& node = model.node(path[index]);
        if (index == 0) {
            continue;
        }
        if (!lizzie::core::normalizedSetupStones(node.setupStones).isEmpty()) {
            result = path[index];
        }
    }
    return result;
}

lizzie::core::Color sideToMoveForRequest(const AnalysisRequest& request)
{
    if (request.moves.empty()) {
        return request.initialPlayer;
    }
    return lizzie::core::opponent(request.moves.back().color);
}

bool asciiCaseEquals(std::string_view left, std::string_view right)
{
    if (left.size() != right.size()) {
        return false;
    }
    return std::equal(left.begin(), left.end(), right.begin(), right.end(), [](char leftChar, char rightChar) {
        return std::tolower(static_cast<unsigned char>(leftChar)) ==
            std::tolower(static_cast<unsigned char>(rightChar));
    });
}

std::optional<std::size_t> findAsciiCaseInsensitive(
    std::string_view text,
    std::string_view needle,
    std::size_t start = 0)
{
    if (needle.empty() || needle.size() > text.size() || start > text.size() - needle.size()) {
        return std::nullopt;
    }
    for (std::size_t position = start; position + needle.size() <= text.size(); ++position) {
        if (asciiCaseEquals(text.substr(position, needle.size()), needle)) {
            return position;
        }
    }
    return std::nullopt;
}

QString moveText(const lizzie::core::Move& move)
{
    switch (move.type) {
    case lizzie::core::MoveType::Play:
        return QString::fromStdString(lizzie::core::pointToSgf(move.point));
    case lizzie::core::MoveType::Pass:
        return QStringLiteral("pass");
    case lizzie::core::MoveType::Resign:
        return QStringLiteral("resign");
    }
    return QStringLiteral("pass");
}

QString moveSortKey(const lizzie::core::Move& move)
{
    return moveText(move);
}

void sortAnalysisCandidates(lizzie::core::AnalysisSnapshot* snapshot)
{
    std::ranges::sort(snapshot->candidates, [](const auto& left, const auto& right) {
        if (left.visits != right.visits) {
            return left.visits > right.visits;
        }
        return moveSortKey(left.move) < moveSortKey(right.move);
    });
}

void fillRootFromBestCandidate(
    lizzie::core::AnalysisSnapshot* snapshot,
    bool hasRootWinrate,
    bool hasRootScoreMean)
{
    if (snapshot->candidates.empty()) {
        return;
    }
    const lizzie::core::MoveCandidate& best = snapshot->candidates.front();
    if (snapshot->rootVisits == 0) {
        snapshot->rootVisits = best.visits;
    }
    if (!hasRootWinrate) {
        snapshot->rootWinrate = best.winrate;
    }
    if (!hasRootScoreMean) {
        snapshot->rootScoreMean = best.scoreMean;
    }
}

double effectiveKomi(std::optional<double> overrideKomi, std::optional<double> gameKomi)
{
    if (overrideKomi.has_value() && std::isfinite(*overrideKomi)) {
        return *overrideKomi;
    }
    if (gameKomi.has_value() && std::isfinite(*gameKomi)) {
        return *gameKomi;
    }
    return 7.5;
}

void discardIncompleteOwnership(lizzie::core::AnalysisSnapshot* snapshot, lizzie::core::BoardSize boardSize)
{
    const std::size_t expectedOwnershipSize = static_cast<std::size_t>(boardSize.pointCount());
    auto hasCompleteFiniteOwnership = [expectedOwnershipSize](const std::vector<double>& ownership) {
        return ownership.size() == expectedOwnershipSize &&
            std::ranges::all_of(ownership, [](double value) { return std::isfinite(value); });
    };
    if (!snapshot->ownership.empty() && !hasCompleteFiniteOwnership(snapshot->ownership)) {
        snapshot->ownership.clear();
    }
    for (lizzie::core::MoveCandidate& candidate : snapshot->candidates) {
        if (!candidate.ownership.empty() && !hasCompleteFiniteOwnership(candidate.ownership)) {
            candidate.ownership.clear();
        }
    }
}

void discardMismatchedPvVisits(lizzie::core::AnalysisSnapshot* snapshot)
{
    for (lizzie::core::MoveCandidate& candidate : snapshot->candidates) {
        if (!candidate.pvVisits.empty() && candidate.pvVisits.size() != candidate.pv.size()) {
            candidate.pvVisits.clear();
        }
    }
}

std::optional<double> finiteJsonDouble(const QJsonValue& value)
{
    if (value.isString()) {
        bool ok = false;
        const double number = value.toString().trimmed().toDouble(&ok);
        if (!ok || !std::isfinite(number)) {
            return std::nullopt;
        }
        return number;
    }
    if (!value.isDouble()) {
        return std::nullopt;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number)) {
        return std::nullopt;
    }
    return number;
}

std::optional<double> jsonDoubleFieldOptional(
    const QJsonObject& object,
    const char* primary,
    const char* fallback = nullptr)
{
    const QJsonValue primaryValue = object.value(primary);
    if (const std::optional<double> primaryNumber = finiteJsonDouble(primaryValue)) {
        return primaryNumber;
    }
    if (fallback == nullptr) {
        return std::nullopt;
    }
    const QJsonValue fallbackValue = object.value(fallback);
    return finiteJsonDouble(fallbackValue);
}

double jsonDoubleField(const QJsonObject& object, const char* primary, const char* fallback = nullptr)
{
    return jsonDoubleFieldOptional(object, primary, fallback).value_or(0.0);
}

std::optional<lizzie::core::Move> moveFromSidecarText(
    QString text,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color color)
{
    if (text.compare("pass", Qt::CaseInsensitive) == 0) {
        return lizzie::core::Move::pass(color);
    }
    if (text.compare("resign", Qt::CaseInsensitive) == 0) {
        return lizzie::core::Move::resign(color);
    }
    if (const auto point = lizzie::core::pointFromSgf(text.toStdString(), boardSize)) {
        return lizzie::core::Move::play(color, *point);
    }
    return std::nullopt;
}

QJsonArray ownershipToJson(const std::vector<double>& ownership, lizzie::core::BoardSize boardSize)
{
    QJsonArray result;
    if (ownership.size() != static_cast<std::size_t>(boardSize.pointCount()) ||
        !std::ranges::all_of(ownership, [](double value) { return std::isfinite(value); })) {
        return result;
    }
    for (double value : ownership) {
        result.push_back(value);
    }
    return result;
}

std::vector<double> ownershipFromJson(const QJsonValue& value, lizzie::core::BoardSize boardSize)
{
    if (!value.isArray()) {
        return {};
    }
    const QJsonArray array = value.toArray();
    if (array.size() != boardSize.pointCount()) {
        return {};
    }
    std::vector<double> result;
    result.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& item : array) {
        const std::optional<double> number = finiteJsonDouble(item);
        if (!number.has_value()) {
            return {};
        }
        result.push_back(*number);
    }
    return result;
}

std::optional<int> intFromJson(const QJsonValue& value)
{
    if (value.isDouble()) {
        const double number = value.toDouble();
        if (!std::isfinite(number) || std::floor(number) != number || number < 0.0 ||
            number > static_cast<double>(std::numeric_limits<int>::max())) {
            return std::nullopt;
        }
        return static_cast<int>(number);
    }
    if (!value.isString()) {
        return std::nullopt;
    }

    const std::string text = value.toString().trimmed().toStdString();
    if (text.empty()) {
        return std::nullopt;
    }
    int result = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const std::from_chars_result parsed = std::from_chars(begin, end, result);
    if (parsed.ec != std::errc{} || parsed.ptr != end || result < 0) {
        return std::nullopt;
    }
    return result;
}

QJsonArray intVectorToJson(const std::vector<int>& values)
{
    QJsonArray result;
    for (int value : values) {
        result.push_back(std::max(0, value));
    }
    return result;
}

std::vector<int> intVectorFromJson(const QJsonValue& value)
{
    std::vector<int> result;
    if (!value.isArray()) {
        return result;
    }
    const QJsonArray array = value.toArray();
    result.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& item : array) {
        if (const std::optional<int> parsed = intFromJson(item)) {
            result.push_back(*parsed);
        } else {
            return {};
        }
    }
    return result;
}

QString colorToJson(lizzie::core::Color color)
{
    return color == lizzie::core::Color::Black ? QStringLiteral("B") : QStringLiteral("W");
}

std::optional<lizzie::core::Color> colorFromJson(const QJsonValue& value)
{
    const QString text = value.toString().trimmed().toUpper();
    if (text == "B" || text == "BLACK") {
        return lizzie::core::Color::Black;
    }
    if (text == "W" || text == "WHITE") {
        return lizzie::core::Color::White;
    }
    return std::nullopt;
}

double normalizedRate(double value)
{
    if (!std::isfinite(value)) {
        return 0.0;
    }
    const double magnitude = std::abs(value);
    if (magnitude > 100.0) {
        value /= 10000.0;
    } else if (magnitude > 1.0) {
        value /= 100.0;
    }
    return std::clamp(value, 0.0, 1.0);
}

std::optional<double> jsonRateField(const QJsonObject& object, const char* primary, const char* fallback = nullptr)
{
    if (const std::optional<double> value = jsonDoubleFieldOptional(object, primary, fallback)) {
        return normalizedRate(*value);
    }
    return std::nullopt;
}

double finiteOrZero(double value)
{
    return std::isfinite(value) ? value : 0.0;
}

QJsonArray pvToJson(const std::vector<lizzie::core::Move>& pv)
{
    QJsonArray result;
    for (const lizzie::core::Move& move : pv) {
        result.push_back(moveText(move));
    }
    return result;
}

std::vector<lizzie::core::Move> pvFromJson(
    const QJsonValue& value,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color firstColor)
{
    std::vector<lizzie::core::Move> result;
    if (!value.isArray()) {
        return result;
    }
    lizzie::core::Color color = firstColor;
    for (const QJsonValue& item : value.toArray()) {
        if (const std::optional<lizzie::core::Move> move =
                moveFromSidecarText(item.toString(), boardSize, color)) {
            result.push_back(*move);
            color = lizzie::core::opponent(color);
        } else {
            return {};
        }
    }
    return result;
}

QJsonObject candidateToJson(const lizzie::core::MoveCandidate& candidate, lizzie::core::BoardSize boardSize)
{
    QJsonObject object;
    object.insert("move", moveText(candidate.move));
    object.insert("visits", std::max(0, candidate.visits));
    object.insert("winrate", normalizedRate(candidate.winrate));
    object.insert("scoreMean", finiteOrZero(candidate.scoreMean));
    object.insert("scoreStdev", finiteOrZero(candidate.scoreStdev));
    object.insert("policy", normalizedRate(candidate.policy));
    object.insert("pv", pvToJson(candidate.pv));
    object.insert("pvVisits", intVectorToJson(candidate.pvVisits));
    object.insert("ownership", ownershipToJson(candidate.ownership, boardSize));
    return object;
}

std::optional<lizzie::core::MoveCandidate> candidateFromJson(
    const QJsonObject& object,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove)
{
    const std::optional<lizzie::core::Move> move =
        moveFromSidecarText(object.value("move").toString(), boardSize, toMove);
    if (!move.has_value()) {
        return std::nullopt;
    }

    lizzie::core::MoveCandidate candidate{*move};
    const std::optional<int> visits = intFromJson(object.value("visits"));
    if (!visits.has_value()) {
        return std::nullopt;
    }
    candidate.visits = *visits;
    candidate.winrate = jsonRateField(object, "winrate").value_or(0.0);
    candidate.scoreMean = jsonDoubleField(object, "scoreMean", "scoreLead");
    candidate.scoreStdev = jsonDoubleField(object, "scoreStdev");
    candidate.policy = normalizedRate(jsonDoubleField(object, "policy", "prior"));
    candidate.pv = pvFromJson(object.value("pv"), boardSize, toMove);
    candidate.pvVisits = intVectorFromJson(object.value("pvVisits"));
    candidate.ownership = ownershipFromJson(object.value("ownership"), boardSize);
    return candidate;
}

QJsonObject snapshotToJson(const lizzie::core::AnalysisSnapshot& analysis, lizzie::core::BoardSize boardSize)
{
    QJsonObject object;
    object.insert("rootVisits", std::max(0, analysis.rootVisits));
    object.insert("rootWinrate", normalizedRate(analysis.rootWinrate));
    object.insert("rootScoreMean", finiteOrZero(analysis.rootScoreMean));
    object.insert("ownership", ownershipToJson(analysis.ownership, boardSize));
    if (analysis.winratePerspective.has_value()) {
        object.insert("winratePerspective", colorToJson(*analysis.winratePerspective));
    }

    QJsonArray candidates;
    std::vector<lizzie::core::MoveCandidate> sortedCandidates = analysis.candidates;
    std::ranges::sort(sortedCandidates, [](const auto& left, const auto& right) {
        const int leftVisits = std::max(0, left.visits);
        const int rightVisits = std::max(0, right.visits);
        if (leftVisits != rightVisits) {
            return leftVisits > rightVisits;
        }
        return moveSortKey(left.move) < moveSortKey(right.move);
    });
    for (const lizzie::core::MoveCandidate& candidate : sortedCandidates) {
        candidates.push_back(candidateToJson(candidate, boardSize));
    }
    object.insert("candidates", candidates);
    return object;
}

std::optional<lizzie::core::AnalysisSnapshot> snapshotFromJson(
    const QJsonObject& object,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove)
{
    lizzie::core::AnalysisSnapshot snapshot;
    snapshot.rootVisits = intFromJson(object.value("rootVisits")).value_or(0);
    const std::optional<double> rootWinrate = jsonRateField(object, "rootWinrate");
    if (rootWinrate.has_value()) {
        snapshot.rootWinrate = *rootWinrate;
    }
    const std::optional<double> rootScoreMean =
        jsonDoubleFieldOptional(object, "rootScoreMean", "rootScoreLead");
    if (rootScoreMean.has_value()) {
        snapshot.rootScoreMean = *rootScoreMean;
    }
    snapshot.ownership = ownershipFromJson(object.value("ownership"), boardSize);
    snapshot.winratePerspective =
        colorFromJson(object.value("winratePerspective")).value_or(lizzie::core::Color::Black);

    const QJsonArray candidates = object.value("candidates").toArray();
    snapshot.candidates.reserve(static_cast<std::size_t>(candidates.size()));
    for (const QJsonValue& value : candidates) {
        if (!value.isObject()) {
            continue;
        }
        if (std::optional<lizzie::core::MoveCandidate> candidate =
                candidateFromJson(value.toObject(), boardSize, toMove)) {
            snapshot.candidates.push_back(std::move(*candidate));
        }
    }

    sortAnalysisCandidates(&snapshot);
    fillRootFromBestCandidate(&snapshot, rootWinrate.has_value(), rootScoreMean.has_value());
    discardIncompleteOwnership(&snapshot, boardSize);
    discardMismatchedPvVisits(&snapshot);

    if (snapshot.rootVisits == 0 && snapshot.candidates.empty() && snapshot.ownership.empty()) {
        return std::nullopt;
    }
    return snapshot;
}

std::optional<lizzie::core::NodeId> nodeIdFromJson(const QJsonValue& value)
{
    std::string text;
    if (value.isString()) {
        text = value.toString().trimmed().toStdString();
        if (text.empty()) {
            return std::nullopt;
        }
    } else if (value.isDouble()) {
        const double number = value.toDouble();
        if (!std::isfinite(number) || std::floor(number) != number || number < 0.0 ||
            number > static_cast<double>(std::numeric_limits<qint64>::max())) {
            return std::nullopt;
        }
        text = std::to_string(value.toInteger());
    } else {
        return std::nullopt;
    }

    lizzie::core::NodeId nodeId = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const std::from_chars_result parsed = std::from_chars(begin, end, nodeId);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return std::nullopt;
    }
    return nodeId;
}

std::string pointSignature(lizzie::core::Point point)
{
    return lizzie::core::pointToSgf(point);
}

std::string moveSignature(const lizzie::core::Move& move)
{
    std::string result = move.color == lizzie::core::Color::Black ? "B:" : "W:";
    switch (move.type) {
    case lizzie::core::MoveType::Play:
        result += pointSignature(move.point);
        break;
    case lizzie::core::MoveType::Pass:
        result += "pass";
        break;
    case lizzie::core::MoveType::Resign:
        result += "resign";
        break;
    }
    return result;
}

void appendPointListSignature(std::ostringstream* stream, std::string_view label, const std::vector<lizzie::core::Point>& points)
{
    if (points.empty()) {
        return;
    }
    std::vector<lizzie::core::Point> canonical = points;
    std::ranges::sort(canonical, [](lizzie::core::Point left, lizzie::core::Point right) {
        if (left.x != right.x) {
            return left.x < right.x;
        }
        return left.y < right.y;
    });
    canonical.erase(std::unique(canonical.begin(), canonical.end()), canonical.end());

    *stream << label;
    for (lizzie::core::Point point : canonical) {
        *stream << pointSignature(point) << ',';
    }
}

char colorSignature(lizzie::core::Color color)
{
    return color == lizzie::core::Color::Black ? 'B' : 'W';
}

std::string buildAnalysisPositionKey(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    bool includePlayerToMove,
    bool canonicalizeRules)
{
    std::ostringstream stream;
    const lizzie::core::BoardSize boardSize = model.boardSize();
    stream << "v1|SZ:" << boardSize.width << 'x' << boardSize.height;
    const lizzie::core::GameInfo& info = model.gameInfo();
    const std::string rules = canonicalizeRules ? canonicalRulesForKataGo(info.rules) : info.rules;
    stream << "|RU:" << rules << "|KM:";
    if (info.komi.has_value() && std::isfinite(*info.komi)) {
        stream << *info.komi;
    }

    const std::vector<lizzie::core::NodeId> path = model.pathTo(nodeId);
    for (std::size_t index = 0; index < path.size(); ++index) {
        const lizzie::core::NodeId pathNodeId = path[index];
        const lizzie::core::GameNode& node = model.node(pathNodeId);
        const lizzie::core::SetupStones setup = lizzie::core::normalizedSetupStones(node.setupStones);
        stream << "|N:";
        appendPointListSignature(&stream, "AB:", setup.black);
        appendPointListSignature(&stream, "AW:", setup.white);
        if (index == 0) {
            stream << "AE:";
        } else {
            appendPointListSignature(&stream, "AE:", setup.empty);
        }
        if (node.move.has_value()) {
            stream << "M:" << moveSignature(*node.move);
        }
    }
    std::string positionError;
    const lizzie::core::BoardPosition position = model.positionAt(nodeId, &positionError);
    if (includePlayerToMove && positionError.empty()) {
        stream << "|P:" << colorSignature(position.sideToMove());
    }
    return stream.str();
}

std::string canonicalizeRulesInPositionKey(std::string_view positionKey)
{
    std::string normalized(positionKey);
    const std::size_t ruleBegin = normalized.find("|RU:");
    if (ruleBegin == std::string::npos) {
        return normalized;
    }
    const std::size_t valueBegin = ruleBegin + 4;
    const std::size_t valueEnd = normalized.find("|KM:", valueBegin);
    if (valueEnd == std::string::npos) {
        return normalized;
    }
    const std::string rules = std::string(std::string_view(normalized).substr(valueBegin, valueEnd - valueBegin));
    normalized.replace(valueBegin, valueEnd - valueBegin, canonicalRulesForKataGo(rules));
    return normalized;
}

lizzie::core::Color moveDerivedSideToMove(const lizzie::core::GameModel& model, lizzie::core::NodeId nodeId)
{
    lizzie::core::Color sideToMove = lizzie::core::Color::Black;
    for (lizzie::core::NodeId pathNodeId : model.pathTo(nodeId)) {
        const lizzie::core::GameNode& node = model.node(pathNodeId);
        if (node.move.has_value()) {
            sideToMove = lizzie::core::opponent(node.move->color);
        }
    }
    return sideToMove;
}

bool analysisPositionKeyMatchesImpl(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    std::string_view positionKey)
{
    const std::string canonicalizedPositionKey = canonicalizeRulesInPositionKey(positionKey);
    for (bool canonicalizeRules : {true, false}) {
        const std::string expected = buildAnalysisPositionKey(model, nodeId, true, canonicalizeRules);
        if (positionKey == expected || canonicalizedPositionKey == expected) {
            return true;
        }
    }

    bool matchesLegacyKey = false;
    for (bool canonicalizeRules : {true, false}) {
        const std::string expected = buildAnalysisPositionKey(model, nodeId, false, canonicalizeRules);
        matchesLegacyKey = matchesLegacyKey || positionKey == expected || canonicalizedPositionKey == expected;
    }
    if (!matchesLegacyKey) {
        return false;
    }

    std::string positionError;
    const lizzie::core::BoardPosition position = model.positionAt(nodeId, &positionError);
    return positionError.empty() && position.sideToMove() == moveDerivedSideToMove(model, nodeId);
}

std::string trimCopy(std::string_view text)
{
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return std::string(text.substr(begin, end - begin));
}

std::optional<lizzie::core::Color> colorFromSgfText(std::string_view text)
{
    std::string normalized = trimCopy(text);
    std::ranges::transform(normalized, normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    if (normalized == "B" || normalized == "BLACK") {
        return lizzie::core::Color::Black;
    }
    if (normalized == "W" || normalized == "WHITE") {
        return lizzie::core::Color::White;
    }
    return std::nullopt;
}

std::vector<std::string> splitWhitespace(std::string_view text)
{
    std::vector<std::string> tokens;
    std::size_t position = 0;
    while (position < text.size()) {
        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position])) != 0) {
            ++position;
        }
        const std::size_t begin = position;
        while (position < text.size() && std::isspace(static_cast<unsigned char>(text[position])) == 0) {
            ++position;
        }
        if (begin < position) {
            tokens.emplace_back(text.substr(begin, position - begin));
        }
    }
    return tokens;
}

std::vector<std::string> splitLegacyLines(std::string_view text)
{
    std::vector<std::string> lines;
    std::size_t position = 0;
    while (position <= text.size()) {
        const std::size_t next = text.find('\n', position);
        const std::size_t end = next == std::string_view::npos ? text.size() : next;
        std::string line = trimCopy(text.substr(position, end - position));
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!line.empty()) {
            lines.push_back(std::move(line));
        }
        if (next == std::string_view::npos) {
            break;
        }
        position = next + 1;
    }
    return lines;
}

std::optional<double> parseDoubleStrict(std::string_view text)
{
    const std::string trimmed = trimCopy(text);
    double value = 0.0;
    const char* begin = trimmed.data();
    const char* end = trimmed.data() + trimmed.size();
    const std::from_chars_result parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end || !std::isfinite(value)) {
        return std::nullopt;
    }
    return value;
}

std::optional<int> parseIntStrict(std::string_view text)
{
    const std::string trimmed = trimCopy(text);
    int value = 0;
    const char* begin = trimmed.data();
    const char* end = trimmed.data() + trimmed.size();
    const std::from_chars_result parsed = std::from_chars(begin, end, value);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return std::nullopt;
    }
    return value;
}

std::optional<int> parseLegacyVisits(std::string_view text)
{
    std::string normalized = trimCopy(text);
    normalized.erase(
        std::remove(normalized.begin(), normalized.end(), ','),
        normalized.end());
    if (normalized.empty()) {
        return std::nullopt;
    }

    double multiplier = 1.0;
    const char suffix = static_cast<char>(std::tolower(static_cast<unsigned char>(normalized.back())));
    if (suffix == 'k' || suffix == 'm') {
        multiplier = suffix == 'k' ? 1000.0 : 1000000.0;
        normalized.pop_back();
    }

    const std::optional<double> value = parseDoubleStrict(normalized);
    if (!value.has_value() || *value < 0.0) {
        return std::nullopt;
    }
    const double scaled = *value * multiplier;
    if (!std::isfinite(scaled) || scaled < 0.0 ||
        scaled > static_cast<double>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    const long long rounded = std::llround(scaled);
    if (rounded < 0 || rounded > static_cast<long long>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    return static_cast<int>(rounded);
}

double clamp01(double value)
{
    return std::clamp(value, 0.0, 1.0);
}

std::optional<double> parseLegacyRootWinrate(std::string_view text)
{
    const std::optional<double> value = parseDoubleStrict(text);
    if (!value.has_value()) {
        return std::nullopt;
    }
    const double fraction = std::abs(*value) > 1.0 ? *value / 100.0 : *value;
    return clamp01(1.0 - fraction);
}

std::optional<double> parseLegacyFraction(std::string_view text)
{
    const std::optional<double> value = parseDoubleStrict(text);
    if (!value.has_value()) {
        return std::nullopt;
    }
    if (std::abs(*value) > 100.0) {
        return *value / 10000.0;
    }
    if (std::abs(*value) > 1.0) {
        return *value / 100.0;
    }
    return *value;
}

std::optional<double> parseCommentWinrate(std::string_view text)
{
    std::string normalized = trimCopy(text);
    if (!normalized.empty() && normalized.back() == '%') {
        normalized.pop_back();
    }
    const std::optional<double> value = parseDoubleStrict(normalized);
    if (!value.has_value()) {
        return std::nullopt;
    }
    return clamp01(std::abs(*value) > 1.0 ? *value / 100.0 : *value);
}

std::optional<lizzie::core::Move> moveFromLegacyText(
    std::string_view text,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color color)
{
    std::string normalized = trimCopy(text);
    std::ranges::transform(normalized, normalized.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    if (normalized == "pass") {
        return lizzie::core::Move::pass(color);
    }
    if (normalized == "resign") {
        return lizzie::core::Move::resign(color);
    }
    if (const std::optional<lizzie::core::Point> point = parseGtpPoint(text, boardSize)) {
        return lizzie::core::Move::play(color, *point);
    }
    if (const std::optional<lizzie::core::Point> point = lizzie::core::pointFromSgf(trimCopy(text), boardSize)) {
        return lizzie::core::Move::play(color, *point);
    }
    return std::nullopt;
}

bool isLegacyPvBoundary(const std::string& token)
{
    static constexpr std::string_view boundaries[] = {
        "info",
        "move",
        "edgeVisits",
        "visits",
        "winrate",
        "score",
        "scoreMean",
        "scoreLead",
        "scoreStdev",
        "stdev",
        "scoreSelfplay",
        "policy",
        "prior",
        "noResultValue",
        "pv",
        "pvVisits",
        "pvEdgeVisits",
        "ownership",
        "ownershipStdev",
        "movesOwnership",
        "movesOwnershipStdev",
        "rootInfo",
        "order",
        "lcb",
        "utility",
        "utilityLcb",
        "weight",
        "isSymmetryOf",
        "rawStWrError",
        "rawStScoreError",
        "rawVarTimeLeft",
    };
    return std::ranges::any_of(boundaries, [&](std::string_view boundary) {
        return asciiCaseEquals(token, boundary);
    });
}

std::optional<std::vector<lizzie::core::Move>> parseStrictLegacyPv(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t end,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color firstColor)
{
    std::vector<lizzie::core::Move> result;
    lizzie::core::Color color = firstColor;
    for (std::size_t index = begin; index < end; ++index) {
        if (const std::optional<lizzie::core::Move> move =
                moveFromLegacyText(tokens[index], boardSize, color)) {
            result.push_back(*move);
            color = lizzie::core::opponent(color);
        } else {
            return std::nullopt;
        }
    }
    return result;
}

std::optional<std::string> generatedAnalysisComment(std::string_view comment)
{
    constexpr std::string_view marker = "LizzieYzy analysis";
    std::optional<std::size_t> position = findAsciiCaseInsensitive(comment, marker);
    while (position.has_value()) {
        const std::size_t after = *position + marker.size();
        if (after == comment.size() || comment[after] == '\n' || comment[after] == '\r') {
            return std::string(comment.substr(*position));
        }
        position = findAsciiCaseInsensitive(comment, marker, after);
    }
    return std::nullopt;
}

std::optional<std::string> fieldValue(std::string_view line, std::string_view name)
{
    if (line.size() < name.size() || !asciiCaseEquals(line.substr(0, name.size()), name)) {
        return std::nullopt;
    }
    return trimCopy(line.substr(name.size()));
}

std::vector<double> parseGeneratedOwnershipLine(std::string_view text)
{
    std::vector<double> result;
    const std::vector<std::string> tokens = splitWhitespace(text);
    result.reserve(tokens.size());
    for (const std::string& token : tokens) {
        if (const std::optional<double> value = parseDoubleStrict(token)) {
            result.push_back(*value);
        } else {
            return {};
        }
    }
    return result;
}

std::optional<std::vector<double>> parseStrictOwnershipTokens(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t end)
{
    std::vector<double> result;
    result.reserve(end - begin);
    for (std::size_t index = begin; index < end; ++index) {
        if (const std::optional<double> value = parseDoubleStrict(tokens[index])) {
            result.push_back(*value);
        } else {
            return std::nullopt;
        }
    }
    return result;
}

std::optional<std::vector<int>> parseStrictVisitTokens(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t end)
{
    std::vector<int> result;
    result.reserve(end - begin);
    for (std::size_t index = begin; index < end; ++index) {
        if (const std::optional<int> visits = parseLegacyVisits(tokens[index])) {
            result.push_back(*visits);
        } else {
            return std::nullopt;
        }
    }
    return result;
}

std::optional<lizzie::core::MoveCandidate> parseGeneratedCommentCandidate(
    std::string_view line,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove)
{
    const std::vector<std::string> tokens = splitWhitespace(line);
    if (tokens.empty()) {
        return std::nullopt;
    }
    const std::optional<lizzie::core::Move> move = moveFromLegacyText(tokens.front(), boardSize, toMove);
    if (!move.has_value()) {
        return std::nullopt;
    }

    lizzie::core::MoveCandidate candidate{*move};
    bool hasMovesOwnership = false;
    for (std::size_t index = 1; index < tokens.size(); ++index) {
        const std::string& key = tokens[index];
        if (asciiCaseEquals(key, "visits") && index + 1 < tokens.size()) {
            candidate.visits = parseLegacyVisits(tokens[++index]).value_or(0);
        } else if (asciiCaseEquals(key, "winrate") && index + 1 < tokens.size()) {
            candidate.winrate = parseCommentWinrate(tokens[++index]).value_or(0.0);
        } else if (
            (asciiCaseEquals(key, "score") || asciiCaseEquals(key, "scoreMean") ||
                asciiCaseEquals(key, "scoreLead")) &&
            index + 1 < tokens.size()) {
            candidate.scoreMean = parseDoubleStrict(tokens[++index]).value_or(0.0);
        } else if (
            (asciiCaseEquals(key, "stdev") || asciiCaseEquals(key, "scoreStdev")) &&
            index + 1 < tokens.size()) {
            candidate.scoreStdev = parseDoubleStrict(tokens[++index]).value_or(0.0);
        } else if (
            (asciiCaseEquals(key, "policy") || asciiCaseEquals(key, "prior")) &&
            index + 1 < tokens.size()) {
            candidate.policy = parseCommentWinrate(tokens[++index]).value_or(0.0);
        } else if (asciiCaseEquals(key, "pv")) {
            const std::size_t pvBegin = index + 1;
            std::size_t pvEnd = pvBegin;
            while (pvEnd < tokens.size() && !isLegacyPvBoundary(tokens[pvEnd])) {
                ++pvEnd;
            }
            if (std::optional<std::vector<lizzie::core::Move>> pv =
                    parseStrictLegacyPv(tokens, pvBegin, pvEnd, boardSize, toMove)) {
                candidate.pv = std::move(*pv);
            }
            index = pvEnd == 0 ? pvEnd : pvEnd - 1;
        } else if (asciiCaseEquals(key, "pvVisits")) {
            const std::size_t visitsBegin = index + 1;
            std::size_t visitsEnd = visitsBegin;
            while (visitsEnd < tokens.size() && !isLegacyPvBoundary(tokens[visitsEnd])) {
                ++visitsEnd;
            }
            std::optional<std::vector<int>> visits = parseStrictVisitTokens(tokens, visitsBegin, visitsEnd);
            if (visits.has_value() && !visits->empty()) {
                candidate.pvVisits.insert(candidate.pvVisits.end(), visits->begin(), visits->end());
            }
            index = visitsEnd == 0 ? visitsEnd : visitsEnd - 1;
        } else if (asciiCaseEquals(key, "ownership") || asciiCaseEquals(key, "movesOwnership")) {
            const bool isMovesOwnership = asciiCaseEquals(key, "movesOwnership");
            const std::size_t ownershipBegin = index + 1;
            std::size_t ownershipEnd = ownershipBegin;
            while (ownershipEnd < tokens.size() && !isLegacyPvBoundary(tokens[ownershipEnd])) {
                ++ownershipEnd;
            }
            std::optional<std::vector<double>> ownership =
                parseStrictOwnershipTokens(tokens, ownershipBegin, ownershipEnd);
            if (ownership.has_value() && !ownership->empty() && (isMovesOwnership || !hasMovesOwnership)) {
                candidate.ownership = std::move(*ownership);
                hasMovesOwnership = isMovesOwnership;
            }
            index = ownershipEnd == 0 ? ownershipEnd : ownershipEnd - 1;
        }
    }
    return candidate;
}

std::optional<lizzie::core::AnalysisSnapshot> parseGeneratedAnalysisComment(
    std::string_view comment,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove)
{
    const std::optional<std::string> section = generatedAnalysisComment(comment);
    if (!section.has_value()) {
        return std::nullopt;
    }

    const std::vector<std::string> lines = splitLegacyLines(*section);
    if (lines.empty() || !asciiCaseEquals(lines.front(), "LizzieYzy analysis")) {
        return std::nullopt;
    }

    lizzie::core::AnalysisSnapshot snapshot;
    snapshot.winratePerspective = lizzie::core::Color::Black;
    bool hasRootWinrate = false;
    bool hasRootScoreMean = false;
    bool inCandidates = false;
    for (std::size_t index = 1; index < lines.size(); ++index) {
        const std::string line = trimCopy(lines[index]);
        if (asciiCaseEquals(line, "Candidates:")) {
            inCandidates = true;
            continue;
        }
        if (inCandidates) {
            if (std::optional<lizzie::core::MoveCandidate> candidate =
                    parseGeneratedCommentCandidate(line, boardSize, toMove)) {
                snapshot.candidates.push_back(std::move(*candidate));
            }
            continue;
        }
        if (const std::optional<std::string> value = fieldValue(line, "WinratePerspective:")) {
            snapshot.winratePerspective = colorFromSgfText(*value).value_or(lizzie::core::Color::Black);
        } else if (const std::optional<std::string> value = fieldValue(line, "Winrate:")) {
            if (const std::optional<double> winrate = parseCommentWinrate(*value)) {
                snapshot.rootWinrate = *winrate;
                hasRootWinrate = true;
            }
        } else if (const std::optional<std::string> value = fieldValue(line, "ScoreMean:")) {
            if (const std::optional<double> score = parseDoubleStrict(*value)) {
                snapshot.rootScoreMean = *score;
                hasRootScoreMean = true;
            }
        } else if (const std::optional<std::string> value = fieldValue(line, "ScoreLead:")) {
            if (const std::optional<double> score = parseDoubleStrict(*value)) {
                snapshot.rootScoreMean = *score;
                hasRootScoreMean = true;
            }
        } else if (const std::optional<std::string> value = fieldValue(line, "Visits:")) {
            snapshot.rootVisits = parseLegacyVisits(*value).value_or(0);
        } else if (const std::optional<std::string> value = fieldValue(line, "Ownership:")) {
            snapshot.ownership = parseGeneratedOwnershipLine(*value);
        }
    }

    sortAnalysisCandidates(&snapshot);
    fillRootFromBestCandidate(&snapshot, hasRootWinrate, hasRootScoreMean);
    discardIncompleteOwnership(&snapshot, boardSize);
    discardMismatchedPvVisits(&snapshot);

    if (snapshot.rootVisits == 0 && snapshot.candidates.empty() && snapshot.ownership.empty()) {
        return std::nullopt;
    }
    return snapshot;
}

std::optional<lizzie::core::MoveCandidate> parseLegacyCandidate(
    const std::vector<std::string>& tokens,
    std::size_t begin,
    std::size_t end,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove)
{
    lizzie::core::MoveCandidate candidate;
    bool hasMove = false;
    bool hasMovesOwnership = false;

    for (std::size_t index = begin; index < end; ++index) {
        const std::string& key = tokens[index];
        if (asciiCaseEquals(key, "move") && index + 1 < end) {
            if (const std::optional<lizzie::core::Move> move =
                    moveFromLegacyText(tokens[++index], boardSize, toMove)) {
                candidate.move = *move;
                hasMove = true;
            }
        } else if (asciiCaseEquals(key, "visits") && index + 1 < end) {
            candidate.visits = parseLegacyVisits(tokens[++index]).value_or(0);
        } else if (asciiCaseEquals(key, "winrate") && index + 1 < end) {
            candidate.winrate = clamp01(parseLegacyFraction(tokens[++index]).value_or(0.0));
        } else if ((asciiCaseEquals(key, "prior") || asciiCaseEquals(key, "policy")) && index + 1 < end) {
            candidate.policy = clamp01(parseLegacyFraction(tokens[++index]).value_or(0.0));
        } else if (
            (asciiCaseEquals(key, "scoreMean") || asciiCaseEquals(key, "scoreLead")) && index + 1 < end) {
            candidate.scoreMean = parseDoubleStrict(tokens[++index]).value_or(0.0);
        } else if (asciiCaseEquals(key, "scoreStdev") && index + 1 < end) {
            candidate.scoreStdev = parseDoubleStrict(tokens[++index]).value_or(0.0);
        } else if (asciiCaseEquals(key, "pv")) {
            const std::size_t pvBegin = index + 1;
            std::size_t pvEnd = pvBegin;
            while (pvEnd < end && !isLegacyPvBoundary(tokens[pvEnd])) {
                ++pvEnd;
            }
            if (std::optional<std::vector<lizzie::core::Move>> pv =
                    parseStrictLegacyPv(tokens, pvBegin, pvEnd, boardSize, toMove)) {
                candidate.pv = std::move(*pv);
            }
            index = pvEnd == 0 ? pvEnd : pvEnd - 1;
        } else if (asciiCaseEquals(key, "pvVisits")) {
            const std::size_t visitsBegin = index + 1;
            std::size_t visitsEnd = visitsBegin;
            while (visitsEnd < end && !isLegacyPvBoundary(tokens[visitsEnd])) {
                ++visitsEnd;
            }
            std::optional<std::vector<int>> visits = parseStrictVisitTokens(tokens, visitsBegin, visitsEnd);
            if (visits.has_value() && !visits->empty()) {
                candidate.pvVisits.insert(candidate.pvVisits.end(), visits->begin(), visits->end());
            }
            index = visitsEnd == 0 ? visitsEnd : visitsEnd - 1;
        } else if (asciiCaseEquals(key, "ownership") || asciiCaseEquals(key, "movesOwnership")) {
            const bool isMovesOwnership = asciiCaseEquals(key, "movesOwnership");
            const std::size_t ownershipBegin = index + 1;
            std::size_t ownershipEnd = ownershipBegin;
            while (ownershipEnd < end && !isLegacyPvBoundary(tokens[ownershipEnd])) {
                ++ownershipEnd;
            }
            std::optional<std::vector<double>> ownership =
                parseStrictOwnershipTokens(tokens, ownershipBegin, ownershipEnd);
            if (ownership.has_value() && !ownership->empty() && (isMovesOwnership || !hasMovesOwnership)) {
                candidate.ownership = std::move(*ownership);
                hasMovesOwnership = isMovesOwnership;
            }
            index = ownershipEnd == 0 ? ownershipEnd : ownershipEnd - 1;
        }
    }

    if (!hasMove) {
        return std::nullopt;
    }
    return candidate;
}

bool isLegacyRootOwnershipTail(const std::vector<std::string>& tokens, std::size_t index)
{
    if (!asciiCaseEquals(tokens[index], "ownership") || index + 1 >= tokens.size()) {
        return false;
    }
    for (std::size_t valueIndex = index + 1; valueIndex < tokens.size(); ++valueIndex) {
        if (!parseDoubleStrict(tokens[valueIndex]).has_value()) {
            return false;
        }
    }
    return true;
}

std::vector<double> parseLegacyOwnershipTail(const std::vector<std::string>& tokens, std::size_t begin)
{
    std::vector<double> result;
    for (std::size_t index = begin; index < tokens.size(); ++index) {
        if (const std::optional<double> value = parseDoubleStrict(tokens[index])) {
            result.push_back(*value);
        } else {
            break;
        }
    }
    return result;
}

std::optional<lizzie::core::AnalysisSnapshot> parseLegacyLizziePayload(
    std::string_view payload,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove)
{
    const std::vector<std::string> lines = splitLegacyLines(payload);
    if (lines.empty()) {
        return std::nullopt;
    }

    const std::vector<std::string> header = splitWhitespace(lines.front());
    if (header.size() < 3) {
        return std::nullopt;
    }

    lizzie::core::AnalysisSnapshot snapshot;
    snapshot.winratePerspective = lizzie::core::Color::Black;
    const std::optional<double> rootWinrate = parseLegacyRootWinrate(header[1]);
    if (rootWinrate.has_value()) {
        snapshot.rootWinrate = *rootWinrate;
    }
    snapshot.rootVisits = parseLegacyVisits(header[2]).value_or(0);
    std::optional<double> rootScoreMean;
    if (header.size() >= 4) {
        rootScoreMean = parseDoubleStrict(header[3]);
        if (rootScoreMean.has_value()) {
            snapshot.rootScoreMean = *rootScoreMean;
        }
    }
    const double headerScoreStdev = header.size() >= 5 ? parseDoubleStrict(header[4]).value_or(0.0) : 0.0;

    std::string candidateLine;
    for (std::size_t index = 1; index < lines.size(); ++index) {
        if (!candidateLine.empty()) {
            candidateLine.push_back(' ');
        }
        candidateLine += lines[index];
    }

    const std::vector<std::string> tokens = splitWhitespace(candidateLine);
    std::size_t tailOwnership = tokens.size();
    for (std::size_t index = 0; index < tokens.size(); ++index) {
        if (isLegacyRootOwnershipTail(tokens, index)) {
            tailOwnership = index;
            snapshot.ownership = parseLegacyOwnershipTail(tokens, index + 1);
            break;
        }
    }

    std::size_t begin = 0;
    while (begin < tailOwnership) {
        while (begin < tailOwnership && asciiCaseEquals(tokens[begin], "info")) {
            ++begin;
        }
        if (begin >= tailOwnership) {
            break;
        }
        std::size_t end = begin + 1;
        while (end < tailOwnership && !asciiCaseEquals(tokens[end], "info")) {
            ++end;
        }
        if (std::optional<lizzie::core::MoveCandidate> candidate =
                parseLegacyCandidate(tokens, begin, end, boardSize, toMove)) {
            snapshot.candidates.push_back(std::move(*candidate));
        }
        begin = end;
    }

    sortAnalysisCandidates(&snapshot);
    if (!snapshot.candidates.empty() && snapshot.candidates.front().scoreStdev == 0.0 && headerScoreStdev > 0.0) {
        snapshot.candidates.front().scoreStdev = headerScoreStdev;
    }
    fillRootFromBestCandidate(&snapshot, rootWinrate.has_value(), rootScoreMean.has_value());
    discardIncompleteOwnership(&snapshot, boardSize);
    discardMismatchedPvVisits(&snapshot);

    if (snapshot.rootVisits == 0 && snapshot.candidates.empty() && snapshot.ownership.empty()) {
        return std::nullopt;
    }
    return snapshot;
}

const std::vector<std::string>* legacyAnalysisValuesForNode(const lizzie::core::GameNode& node)
{
    const std::array<std::string_view, 2> keys =
        node.parent.has_value() ? std::array<std::string_view, 2>{"LZ", "LZOP"}
                                : std::array<std::string_view, 2>{"LZOP", "LZ"};
    for (std::string_view key : keys) {
        const auto found = node.properties.find(std::string(key));
        if (found != node.properties.end() && !found->second.empty()) {
            return &found->second;
        }
    }
    for (std::string_view key : keys) {
        for (const auto& [name, values] : node.properties) {
            if (asciiCaseEquals(name, key) && !values.empty()) {
                return &values;
            }
        }
    }
    return nullptr;
}

std::optional<lizzie::core::Color> legacyWinratePerspectiveForNode(const lizzie::core::GameNode& node)
{
    const auto found = node.properties.find("LZYPERSP");
    if (found == node.properties.end()) {
        for (const auto& [name, values] : node.properties) {
            if (asciiCaseEquals(name, "LZYPERSP")) {
                for (const std::string& value : values) {
                    if (std::optional<lizzie::core::Color> color = colorFromSgfText(value)) {
                        return color;
                    }
                }
                return std::nullopt;
            }
        }
        return std::nullopt;
    }
    for (const std::string& value : found->second) {
        if (std::optional<lizzie::core::Color> color = colorFromSgfText(value)) {
            return color;
        }
    }
    return std::nullopt;
}

std::vector<InitialStone> initialStonesForPosition(const lizzie::core::BoardPosition& position)
{
    std::vector<InitialStone> result;
    const lizzie::core::BoardSize boardSize = position.size();
    result.reserve(static_cast<std::size_t>(boardSize.pointCount()));
    for (int y = 0; y < boardSize.height; ++y) {
        for (int x = 0; x < boardSize.width; ++x) {
            const lizzie::core::Point point{x, y};
            if (std::optional<lizzie::core::Color> color = lizzie::core::colorFor(position.at(point))) {
                result.push_back(InitialStone{*color, point});
            }
        }
    }
    return result;
}

AnalysisRequest baseRequestForNode(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    const BatchAnalysisOptions& options)
{
    AnalysisRequest request;
    request.id = analysisRequestId(nodeId);
    request.boardSize = model.boardSize();
    request.includeOwnership = options.includeOwnership;
    request.maxVisits = options.maxVisits;
    request.maxPlayouts = options.maxPlayouts;
    request.maxTime = options.maxTime;
    request.playoutDoublingAdvantage = options.playoutDoublingAdvantage;
    request.wideRootNoise = options.wideRootNoise;

    const lizzie::core::GameInfo& info = model.gameInfo();
    request.rules =
        canonicalRulesForKataGo(options.rulesOverride.value_or(info.rules.empty() ? std::string("Chinese") : info.rules));
    request.komi = effectiveKomi(options.komiOverride, info.komi);
    return request;
}

AnalysisRequest buildReplayRequestForNode(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    const BatchAnalysisOptions& options)
{
    AnalysisRequest request = baseRequestForNode(model, nodeId, options);

    const std::vector<lizzie::core::NodeId> path = model.pathTo(nodeId);
    for (std::size_t index = 0; index < path.size(); ++index) {
        const lizzie::core::GameNode& node = model.node(path[index]);
        const lizzie::core::SetupStones setup = lizzie::core::normalizedSetupStones(node.setupStones);
        if (index == 0) {
            appendInitialStones(&request.initialStones, setup);
            std::string rootPositionError;
            const lizzie::core::BoardPosition rootPosition = model.positionAt(path[index], &rootPositionError);
            if (rootPositionError.empty()) {
                request.initialPlayer = rootPosition.sideToMove();
            }
        }

        if (node.move.has_value()) {
            if (node.move->type != lizzie::core::MoveType::Resign) {
                request.moves.push_back(*node.move);
            }
        }
    }

    return request;
}

AnalysisRequest buildSnapshotRequestForNode(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId snapshotNodeId,
    lizzie::core::NodeId targetNodeId,
    const BatchAnalysisOptions& options)
{
    AnalysisRequest request = baseRequestForNode(model, targetNodeId, options);

    std::string snapshotError;
    const lizzie::core::BoardPosition snapshot = model.positionAt(snapshotNodeId, &snapshotError);
    if (snapshotError.empty()) {
        request.initialStones = initialStonesForPosition(snapshot);
        request.initialPlayer = snapshot.sideToMove();
    }

    const std::vector<lizzie::core::NodeId> path = model.pathTo(targetNodeId);
    bool appendMoves = false;
    for (lizzie::core::NodeId pathNodeId : path) {
        if (appendMoves) {
            const lizzie::core::GameNode& node = model.node(pathNodeId);
            if (node.move.has_value() && node.move->type != lizzie::core::MoveType::Resign) {
                request.moves.push_back(*node.move);
            }
        }
        if (pathNodeId == snapshotNodeId) {
            appendMoves = true;
        }
    }

    return request;
}

}  // namespace

std::vector<lizzie::core::NodeId> collectAnalysisNodeIds(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId root)
{
    std::vector<lizzie::core::NodeId> result;
    collectDepthFirst(model, root, &result);
    return result;
}

BatchAnalysisPlan buildBatchAnalysisPlan(
    const lizzie::core::GameModel& model,
    const std::vector<lizzie::core::NodeId>& nodes,
    const BatchAnalysisOptions& options)
{
    BatchAnalysisPlan plan;

    const std::vector<lizzie::core::NodeId> selected =
        nodes.empty() ? collectAnalysisNodeIds(model, model.rootId()) : nodes;

    std::unordered_set<lizzie::core::NodeId> seenNodes;
    for (lizzie::core::NodeId nodeId : selected) {
        if (!seenNodes.insert(nodeId).second) {
            continue;
        }
        if (model.findNode(nodeId) == nullptr) {
            plan.warnings.push_back(nodeLabel(nodeId) + " does not exist and was skipped");
            continue;
        }
        if (const std::optional<lizzie::core::NodeId> resignNode = firstResignNodeInPath(model, nodeId)) {
            plan.warnings.push_back(
                nodeLabel(nodeId) + " follows resign node " + std::to_string(*resignNode) +
                " and was skipped");
            continue;
        }
        std::string positionError;
        const lizzie::core::BoardPosition position = model.positionAt(nodeId, &positionError);
        if (!positionError.empty()) {
            plan.warnings.push_back(nodeLabel(nodeId) + ": " + positionError + "; analysis was skipped");
            continue;
        }

        AnalysisRequest request;
        if (const std::optional<lizzie::core::NodeId> setupNode = lastNonRootSetupNodeInPath(model, nodeId)) {
            request = buildSnapshotRequestForNode(model, *setupNode, nodeId, options);
        } else {
            request = buildReplayRequestForNode(model, nodeId, options);
        }
        if (sideToMoveForRequest(request) != position.sideToMove()) {
            request = buildSnapshotRequestForNode(model, nodeId, nodeId, options);
        }
        plan.items.push_back(BatchAnalysisItem{nodeId, std::move(request)});
    }
    return plan;
}

std::string analysisRequestId(lizzie::core::NodeId nodeId)
{
    return "node:" + std::to_string(nodeId);
}

std::optional<lizzie::core::NodeId> nodeIdFromAnalysisRequestId(const std::string& id)
{
    constexpr std::string_view prefix = "node:";
    if (!id.starts_with(prefix)) {
        return std::nullopt;
    }

    lizzie::core::NodeId nodeId = 0;
    const char* begin = id.data() + prefix.size();
    const char* end = id.data() + id.size();
    const std::from_chars_result parsed = std::from_chars(begin, end, nodeId);
    if (parsed.ec != std::errc{} || parsed.ptr != end) {
        return std::nullopt;
    }
    return nodeId;
}

std::string analysisPositionKey(const lizzie::core::GameModel& model, lizzie::core::NodeId nodeId)
{
    return buildAnalysisPositionKey(model, nodeId, true, true);
}

bool analysisPositionKeyMatches(
    const lizzie::core::GameModel& model,
    lizzie::core::NodeId nodeId,
    std::string_view positionKey)
{
    return analysisPositionKeyMatchesImpl(model, nodeId, positionKey);
}

lizzie::core::AnalysisSnapshot analysisSnapshotFromResponse(
    const lizzie::core::GameModel& model,
    const AnalysisResponse& response)
{
    const bool hasRootWinrate = std::isfinite(response.rootWinrate);
    const bool hasRootScoreMean = std::isfinite(response.rootScoreMean);

    lizzie::core::AnalysisSnapshot snapshot;
    snapshot.candidates = response.moveInfos;
    snapshot.ownership = response.ownership;
    snapshot.rootWinrate = normalizedRate(response.rootWinrate);
    snapshot.rootScoreMean = finiteOrZero(response.rootScoreMean);
    snapshot.rootVisits = std::max(0, response.rootVisits);
    snapshot.winratePerspective = response.winratePerspective;

    for (lizzie::core::MoveCandidate& candidate : snapshot.candidates) {
        candidate.visits = std::max(0, candidate.visits);
        candidate.winrate = normalizedRate(candidate.winrate);
        candidate.scoreMean = finiteOrZero(candidate.scoreMean);
        candidate.scoreStdev = finiteOrZero(candidate.scoreStdev);
        candidate.policy = normalizedRate(candidate.policy);
        for (int& visits : candidate.pvVisits) {
            visits = std::max(0, visits);
        }
    }
    sortAnalysisCandidates(&snapshot);
    fillRootFromBestCandidate(&snapshot, hasRootWinrate, hasRootScoreMean);
    discardIncompleteOwnership(&snapshot, model.boardSize());
    discardMismatchedPvVisits(&snapshot);
    return snapshot;
}

bool applyAnalysisResponse(
    lizzie::core::GameModel* model,
    const AnalysisResponse& response,
    std::string_view expectedPositionKey)
{
    if (model == nullptr) {
        return false;
    }
    if (!response.errorMessage.empty()) {
        return false;
    }
    const std::optional<lizzie::core::NodeId> nodeId = nodeIdFromAnalysisRequestId(response.id);
    if (!nodeId.has_value()) {
        return false;
    }
    lizzie::core::GameNode* node = model->findNode(*nodeId);
    if (node == nullptr) {
        return false;
    }
    if (!expectedPositionKey.empty() && !analysisPositionKeyMatches(*model, *nodeId, expectedPositionKey)) {
        return false;
    }

    node->analysis = analysisSnapshotFromResponse(*model, response);
    node->analysisError.clear();
    return true;
}

QJsonObject analysisSidecarToJson(
    const lizzie::core::GameModel& model,
    const std::string& sgfPath,
    std::optional<int> collectionGameIndex)
{
    QJsonObject root;
    root.insert("format", "lizzieyzy-analysis-sidecar-v1");
    root.insert("sgf", QString::fromStdString(sgfPath));
    if (collectionGameIndex.has_value()) {
        root.insert("collectionGameIndex", *collectionGameIndex);
    }

    QJsonArray nodes;
    for (lizzie::core::NodeId nodeId : collectAnalysisNodeIds(model, model.rootId())) {
        const lizzie::core::GameNode& node = model.node(nodeId);
        if (!node.analysis.has_value() && node.analysisError.empty()) {
            continue;
        }
        QJsonObject nodeObject;
        nodeObject.insert("nodeId", QString::number(nodeId));
        nodeObject.insert("positionKey", QString::fromStdString(analysisPositionKey(model, nodeId)));
        if (!node.analysisError.empty()) {
            nodeObject.insert("analysisError", QString::fromStdString(node.analysisError));
        } else if (node.analysis.has_value()) {
            nodeObject.insert("analysis", snapshotToJson(*node.analysis, model.boardSize()));
        }
        nodes.push_back(nodeObject);
    }
    root.insert("nodes", nodes);
    return root;
}

int applyAnalysisSidecarJson(
    lizzie::core::GameModel* model,
    const QJsonObject& root,
    std::vector<std::string>* warnings,
    std::optional<int> expectedCollectionGameIndex)
{
    if (model == nullptr) {
        return 0;
    }

    const QString format = root.value("format").toString();
    if (!format.isEmpty() && format != "lizzieyzy-analysis-sidecar-v1") {
        if (warnings != nullptr) {
            warnings->push_back("unsupported analysis sidecar format: " + format.toStdString());
        }
        return 0;
    }

    if (expectedCollectionGameIndex.has_value()) {
        const std::optional<int> collectionGameIndex = intFromJson(root.value("collectionGameIndex"));
        if (!collectionGameIndex.has_value()) {
            if (warnings != nullptr) {
                warnings->push_back("analysis sidecar is missing collectionGameIndex for SGF collection");
            }
            return 0;
        }
        if (*collectionGameIndex != *expectedCollectionGameIndex) {
            if (warnings != nullptr) {
                warnings->push_back(
                    "analysis sidecar collectionGameIndex " + std::to_string(*collectionGameIndex) +
                    " does not match selected SGF game index " + std::to_string(*expectedCollectionGameIndex));
            }
            return 0;
        }
    }

    const QJsonValue nodesValue = root.value("nodes");
    if (!nodesValue.isArray()) {
        if (warnings != nullptr) {
            warnings->push_back("analysis sidecar is missing a valid nodes array");
        }
        return 0;
    }

    int applied = 0;
    const QJsonArray nodes = nodesValue.toArray();
    for (qsizetype index = 0; index < nodes.size(); ++index) {
        const QJsonValue value = nodes.at(index);
        if (!value.isObject()) {
            if (warnings != nullptr) {
                warnings->push_back(
                    "analysis sidecar node entry " + std::to_string(index) + " is not an object");
            }
            continue;
        }
        const QJsonObject nodeObject = value.toObject();
        const std::optional<lizzie::core::NodeId> nodeId = nodeIdFromJson(nodeObject.value("nodeId"));
        if (!nodeId.has_value()) {
            if (warnings != nullptr) {
                warnings->push_back("analysis sidecar node is missing a valid nodeId");
            }
            continue;
        }

        lizzie::core::GameNode* node = model->findNode(*nodeId);
        if (node == nullptr) {
            if (warnings != nullptr) {
                warnings->push_back("analysis sidecar node " + std::to_string(*nodeId) + " does not exist");
            }
            continue;
        }
        const QString positionKey = nodeObject.value("positionKey").toString();
        if (!positionKey.isEmpty() && !analysisPositionKeyMatches(*model, *nodeId, positionKey.toStdString())) {
            if (warnings != nullptr) {
                warnings->push_back("analysis sidecar node " + std::to_string(*nodeId) + " does not match current position");
            }
            continue;
        }

        std::string error;
        const lizzie::core::BoardPosition position = model->positionAt(*nodeId, &error);
        if (!error.empty()) {
            if (warnings != nullptr) {
                warnings->push_back("analysis sidecar node " + std::to_string(*nodeId) + ": " + error);
            }
            continue;
        }

        const QString analysisError = nodeObject.value("analysisError").toString();
        const QJsonValue analysisValue = nodeObject.value("analysis");
        if (analysisValue.isObject()) {
            const std::optional<lizzie::core::AnalysisSnapshot> snapshot =
                snapshotFromJson(analysisValue.toObject(), model->boardSize(), position.sideToMove());
            if (snapshot.has_value()) {
                node->analysis = *snapshot;
                node->analysisError.clear();
                ++applied;
                continue;
            }
        }

        if (!analysisError.isEmpty()) {
            node->analysis.reset();
            node->analysisError = analysisError.toStdString();
            ++applied;
            continue;
        }

        if (warnings != nullptr) {
            warnings->push_back("analysis sidecar node " + std::to_string(*nodeId) + " has no usable analysis");
        }
    }
    return applied;
}

int importLegacyLizzieAnalysis(lizzie::core::GameModel* model, std::vector<std::string>* warnings)
{
    if (model == nullptr) {
        return 0;
    }

    int imported = 0;
    for (lizzie::core::NodeId nodeId : collectAnalysisNodeIds(*model, model->rootId())) {
        lizzie::core::GameNode* node = model->findNode(nodeId);
        if (node == nullptr || node->analysis.has_value()) {
            continue;
        }

        const std::vector<std::string>* values = legacyAnalysisValuesForNode(*node);

        std::string error;
        const lizzie::core::BoardPosition position = model->positionAt(nodeId, &error);
        if (!error.empty()) {
            if (warnings != nullptr) {
                warnings->push_back("legacy analysis node " + std::to_string(nodeId) + ": " + error);
            }
            continue;
        }

        bool applied = false;
        const auto applySnapshot = [&](const lizzie::core::AnalysisSnapshot& snapshot) {
            lizzie::core::AnalysisSnapshot restored = snapshot;
            if (std::optional<lizzie::core::Color> perspective = legacyWinratePerspectiveForNode(*node)) {
                restored.winratePerspective = *perspective;
            }
            node->analysis = std::move(restored);
            node->analysisError.clear();
            ++imported;
            applied = true;
        };

        if (values != nullptr) {
            for (const std::string& payload : *values) {
                const std::optional<lizzie::core::AnalysisSnapshot> snapshot =
                    parseLegacyLizziePayload(payload, model->boardSize(), position.sideToMove());
                if (snapshot.has_value()) {
                    applySnapshot(*snapshot);
                    break;
                }
            }
        }

        if (!applied) {
            const std::optional<lizzie::core::AnalysisSnapshot> snapshot =
                parseGeneratedAnalysisComment(node->comment, model->boardSize(), position.sideToMove());
            if (snapshot.has_value()) {
                applySnapshot(*snapshot);
            }
        }

        if (!applied && values != nullptr && warnings != nullptr) {
            warnings->push_back("legacy analysis node " + std::to_string(nodeId) + " has no usable LZ or comment data");
        }
    }
    return imported;
}

}  // namespace lizzie::engine

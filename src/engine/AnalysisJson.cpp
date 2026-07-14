#include "AnalysisJson.h"

#include "GtpProtocol.h"
#include "KataGoRules.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>

namespace lizzie::engine {

namespace {

constexpr std::string_view kAnalysisGtpColumns = "ABCDEFGHJKLMNOPQRSTUVWXYZ";

[[nodiscard]] double normalizedRate(double value)
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

QString colorToKataGo(lizzie::core::Color color)
{
    return color == lizzie::core::Color::Black ? "B" : "W";
}

bool isPointOnBoard(lizzie::core::Point point, lizzie::core::BoardSize boardSize)
{
    return boardSize.isValid() && point.x >= 0 && point.y >= 0 && point.x < boardSize.width &&
        point.y < boardSize.height;
}

lizzie::core::BoardSize effectiveRequestBoardSize(lizzie::core::BoardSize boardSize)
{
    return boardSize.isValid() ? boardSize : lizzie::core::BoardSize::square(19);
}

QString analysisPointToJsonText(lizzie::core::Point point, lizzie::core::BoardSize boardSize)
{
    if (!isPointOnBoard(point, boardSize)) {
        return {};
    }
    if (isGtpBoardSizeSupported(boardSize)) {
        return QString::fromStdString(formatPointForGtp(point, boardSize));
    }
    return QStringLiteral("(%1,%2)").arg(point.x).arg(point.y);
}

std::optional<QJsonValue> movePointToJson(const lizzie::core::Move& move, lizzie::core::BoardSize boardSize)
{
    switch (move.type) {
    case lizzie::core::MoveType::Play:
        if (!isPointOnBoard(move.point, boardSize)) {
            return std::nullopt;
        }
        return analysisPointToJsonText(move.point, boardSize);
    case lizzie::core::MoveType::Pass:
        return QStringLiteral("pass");
    case lizzie::core::MoveType::Resign:
        return QStringLiteral("resign");
    }
    return std::nullopt;
}

QString moveSortKey(const lizzie::core::Move& move, lizzie::core::BoardSize boardSize)
{
    switch (move.type) {
    case lizzie::core::MoveType::Play:
        return analysisPointToJsonText(move.point, boardSize);
    case lizzie::core::MoveType::Pass:
        return QStringLiteral("pass");
    case lizzie::core::MoveType::Resign:
        return QStringLiteral("resign");
    }
    return {};
}

std::optional<QJsonArray> moveToJsonArray(const lizzie::core::Move& move, lizzie::core::BoardSize boardSize)
{
    const std::optional<QJsonValue> point = movePointToJson(move, boardSize);
    if (!point.has_value()) {
        return std::nullopt;
    }
    QJsonArray result;
    result.push_back(colorToKataGo(move.color));
    result.push_back(*point);
    return result;
}

std::optional<QJsonArray> initialStoneToJsonArray(const InitialStone& stone, lizzie::core::BoardSize boardSize)
{
    if (!isPointOnBoard(stone.point, boardSize)) {
        return std::nullopt;
    }
    QJsonArray result;
    result.push_back(colorToKataGo(stone.color));
    result.push_back(analysisPointToJsonText(stone.point, boardSize));
    return result;
}

std::string_view trimView(std::string_view text)
{
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front())) != 0) {
        text.remove_prefix(1);
    }
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.back())) != 0) {
        text.remove_suffix(1);
    }
    return text;
}

std::optional<int> parseIntView(std::string_view text)
{
    text = trimView(text);
    int value = 0;
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const std::from_chars_result result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end) {
        return std::nullopt;
    }
    return value;
}

std::optional<lizzie::core::Point> parseKataGoMachinePoint(
    std::string_view text,
    lizzie::core::BoardSize boardSize)
{
    text = trimView(text);
    if (text.size() < 5 || text.front() != '(' || text.back() != ')') {
        return std::nullopt;
    }
    text.remove_prefix(1);
    text.remove_suffix(1);
    const std::size_t comma = text.find(',');
    if (comma == std::string_view::npos || text.find(',', comma + 1) != std::string_view::npos) {
        return std::nullopt;
    }

    const std::optional<int> x = parseIntView(text.substr(0, comma));
    const std::optional<int> y = parseIntView(text.substr(comma + 1));
    if (!x.has_value() || !y.has_value()) {
        return std::nullopt;
    }

    const lizzie::core::Point point{*x, *y};
    return isPointOnBoard(point, boardSize) ? std::optional<lizzie::core::Point>(point) : std::nullopt;
}

std::optional<lizzie::core::Point> parseKataGoExtendedGtpPoint(
    std::string_view text,
    lizzie::core::BoardSize boardSize)
{
    text = trimView(text);
    if (text.size() < 3) {
        return std::nullopt;
    }

    std::size_t rowBegin = 0;
    while (rowBegin < text.size() && std::isalpha(static_cast<unsigned char>(text[rowBegin])) != 0) {
        ++rowBegin;
    }
    if (rowBegin != 2 || rowBegin == text.size()) {
        return std::nullopt;
    }

    const char prefix = static_cast<char>(std::toupper(static_cast<unsigned char>(text[0])));
    const char suffix = static_cast<char>(std::toupper(static_cast<unsigned char>(text[1])));
    if (prefix < 'A' || prefix > 'Z') {
        return std::nullopt;
    }
    const std::size_t suffixColumn = kAnalysisGtpColumns.find(suffix);
    if (suffixColumn == std::string_view::npos) {
        return std::nullopt;
    }
    const std::optional<int> row = parseIntView(text.substr(rowBegin));
    if (!row.has_value()) {
        return std::nullopt;
    }

    const int x = (prefix - 'A' + 1) * static_cast<int>(kAnalysisGtpColumns.size()) +
        static_cast<int>(suffixColumn);
    const int y = boardSize.height - *row;
    const lizzie::core::Point point{x, y};
    return isPointOnBoard(point, boardSize) ? std::optional<lizzie::core::Point>(point) : std::nullopt;
}

std::optional<lizzie::core::Point> parseAnalysisPoint(
    std::string_view text,
    lizzie::core::BoardSize boardSize)
{
    text = trimView(text);
    if (const auto point = parseGtpPoint(text, boardSize)) {
        return point;
    }
    if (const auto point = parseKataGoExtendedGtpPoint(text, boardSize)) {
        return point;
    }
    return parseKataGoMachinePoint(text, boardSize);
}

std::optional<double> finiteDouble(const QJsonValue& value)
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

std::vector<double> ownershipArrayFromJson(const QJsonValue& value, lizzie::core::BoardSize boardSize)
{
    if (!value.isArray()) {
        return {};
    }
    const QJsonArray array = value.toArray();
    if (array.size() != boardSize.pointCount()) {
        return {};
    }
    std::vector<double> ownership;
    ownership.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& item : array) {
        const std::optional<double> number = finiteDouble(item);
        if (!number.has_value()) {
            return {};
        }
        ownership.push_back(*number);
    }
    return ownership;
}

std::optional<int> intFromJson(const QJsonValue& value)
{
    if (value.isString()) {
        const QString text = value.toString().trimmed();
        if (text.isEmpty()) {
            return std::nullopt;
        }
        int result = 0;
        const std::string bytes = text.toStdString();
        const char* begin = bytes.data();
        const char* end = bytes.data() + bytes.size();
        const std::from_chars_result parsed = std::from_chars(begin, end, result);
        if (parsed.ec != std::errc{} || parsed.ptr != end || result < 0) {
            return std::nullopt;
        }
        return result;
    }
    if (!value.isDouble()) {
        return std::nullopt;
    }
    const double number = value.toDouble();
    if (!std::isfinite(number) || std::floor(number) != number || number < 0.0 ||
        number > static_cast<double>(std::numeric_limits<int>::max())) {
        return std::nullopt;
    }
    return static_cast<int>(number);
}

std::vector<int> intArrayFromJson(const QJsonValue& value)
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
    const QJsonArray array = value.toArray();
    result.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& item : array) {
        const QString moveText = item.toString();
        if (moveText.compare("pass", Qt::CaseInsensitive) == 0) {
            result.push_back(lizzie::core::Move::pass(color));
            color = lizzie::core::opponent(color);
        } else if (moveText.compare("resign", Qt::CaseInsensitive) == 0) {
            result.push_back(lizzie::core::Move::resign(color));
            color = lizzie::core::opponent(color);
        } else if (const auto point = parseAnalysisPoint(moveText.toStdString(), boardSize)) {
            result.push_back(lizzie::core::Move::play(color, *point));
            color = lizzie::core::opponent(color);
        } else {
            return {};
        }
    }
    return result;
}

std::optional<lizzie::core::Move> moveFromJsonText(
    QString moveText,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color color)
{
    if (moveText.compare("pass", Qt::CaseInsensitive) == 0) {
        return lizzie::core::Move::pass(color);
    }
    if (moveText.compare("resign", Qt::CaseInsensitive) == 0) {
        return lizzie::core::Move::resign(color);
    }
    if (const auto point = parseAnalysisPoint(moveText.toStdString(), boardSize)) {
        return lizzie::core::Move::play(color, *point);
    }
    return std::nullopt;
}

std::optional<double> optionalDoubleField(
    const QJsonObject& object,
    const char* primary,
    const char* fallback = nullptr)
{
    if (const std::optional<double> primaryValue = finiteDouble(object.value(primary))) {
        return primaryValue;
    }
    if (fallback == nullptr) {
        return std::nullopt;
    }
    return finiteDouble(object.value(fallback));
}

double doubleField(const QJsonObject& object, const char* primary, const char* fallback = nullptr)
{
    return optionalDoubleField(object, primary, fallback).value_or(0.0);
}

std::optional<double> rateField(const QJsonObject& object, const char* primary, const char* fallback = nullptr)
{
    if (const std::optional<double> value = optionalDoubleField(object, primary, fallback)) {
        return normalizedRate(*value);
    }
    return std::nullopt;
}

bool hasFiniteValue(std::optional<double> value)
{
    return value.has_value() && std::isfinite(*value);
}

bool hasPositiveInt(std::optional<int> value)
{
    return value.has_value() && *value > 0;
}

bool hasPositiveFiniteValue(std::optional<double> value)
{
    return hasFiniteValue(value) && *value > 0.0;
}

bool hasNonZeroFiniteValue(std::optional<double> value)
{
    return hasFiniteValue(value) && *value != 0.0;
}

}  // namespace

QJsonObject AnalysisRequest::toJson() const
{
    constexpr double kDefaultKomi = 7.5;
    const lizzie::core::BoardSize requestBoardSize = effectiveRequestBoardSize(boardSize);
    const bool sourceBoardSizeValid = boardSize.isValid();

    QJsonObject object;
    object.insert("id", QString::fromStdString(id));
    object.insert("boardXSize", requestBoardSize.width);
    object.insert("boardYSize", requestBoardSize.height);
    object.insert("rules", QString::fromStdString(canonicalRulesForKataGo(rules)));
    object.insert("komi", std::isfinite(komi) ? komi : kDefaultKomi);
    object.insert("includeOwnership", includeOwnership);
    object.insert("includePVVisits", includePVVisits);
    object.insert("initialPlayer", colorToKataGo(initialPlayer));

    QJsonArray movesArray;
    for (const lizzie::core::Move& move : moves) {
        if (move.type == lizzie::core::MoveType::Play && !sourceBoardSizeValid) {
            continue;
        }
        if (const std::optional<QJsonArray> moveJson = moveToJsonArray(move, requestBoardSize)) {
            movesArray.push_back(*moveJson);
        }
    }
    object.insert("moves", movesArray);

    QJsonArray initialStonesArray;
    if (sourceBoardSizeValid) {
        for (const InitialStone& stone : initialStones) {
            if (const std::optional<QJsonArray> stoneJson = initialStoneToJsonArray(stone, requestBoardSize)) {
                initialStonesArray.push_back(*stoneJson);
            }
        }
    }
    object.insert("initialStones", initialStonesArray);

    if (hasPositiveInt(maxVisits)) {
        object.insert("maxVisits", *maxVisits);
    }
    if (hasPositiveInt(maxPlayouts)) {
        object.insert("maxPlayouts", *maxPlayouts);
    }
    if (hasPositiveFiniteValue(maxTime)) {
        object.insert("maxTime", *maxTime);
    }
    QJsonObject overrideSettings;
    if (hasNonZeroFiniteValue(playoutDoublingAdvantage)) {
        overrideSettings.insert("playoutDoublingAdvantage", *playoutDoublingAdvantage);
    }
    if (hasPositiveFiniteValue(wideRootNoise)) {
        overrideSettings.insert("wideRootNoise", *wideRootNoise);
    }
    overrideSettings.insert("reportAnalysisWinratesAs", "BLACK");
    if (!overrideSettings.isEmpty()) {
        object.insert("overrideSettings", overrideSettings);
    }
    return object;
}

std::string AnalysisRequest::toJsonLine() const
{
    QJsonDocument document(toJson());
    QByteArray json = document.toJson(QJsonDocument::Compact);
    json.push_back('\n');
    return json.toStdString();
}

std::optional<AnalysisResponse> parseAnalysisResponse(
    std::string_view jsonLine,
    lizzie::core::BoardSize boardSize,
    lizzie::core::Color toMove)
{
    QJsonParseError error;
    const QByteArray bytes(jsonLine.data(), static_cast<qsizetype>(jsonLine.size()));
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return std::nullopt;
    }

    const QJsonObject object = document.object();
    AnalysisResponse response;
    response.rawJson = std::string(jsonLine);
    response.id = object.value("id").toString().toStdString();
    if (response.id.empty()) {
        return std::nullopt;
    }
    response.errorMessage = object.value("error").toString().toStdString();
    if (!response.errorMessage.empty()) {
        return response;
    }
    if (!boardSize.isValid()) {
        return std::nullopt;
    }
    response.ownership = ownershipArrayFromJson(object.value("ownership"), boardSize);

    const bool hasRootInfoObject = object.value("rootInfo").isObject();
    const QJsonObject rootInfo = object.value("rootInfo").toObject();
    const std::vector<double> rootInfoOwnership = ownershipArrayFromJson(rootInfo.value("ownership"), boardSize);
    if (response.ownership.empty() && hasRootInfoObject) {
        response.ownership = rootInfoOwnership;
    }
    const std::optional<double> rootWinrate = rateField(rootInfo, "winrate");
    const std::optional<double> rootScoreMean = optionalDoubleField(rootInfo, "scoreMean", "scoreLead");
    const std::optional<int> rootVisits = intFromJson(rootInfo.value("visits"));
    const bool hasPositiveRootVisits = rootVisits.has_value() && *rootVisits > 0;
    const bool hasRootInfo =
        hasRootInfoObject &&
        (rootWinrate.has_value() || rootScoreMean.has_value() || hasPositiveRootVisits || !rootInfoOwnership.empty());
    if (rootWinrate.has_value()) {
        response.rootWinrate = *rootWinrate;
    }
    if (rootScoreMean.has_value()) {
        response.rootScoreMean = *rootScoreMean;
    }
    response.rootVisits = rootVisits.value_or(0);
    response.winratePerspective = lizzie::core::Color::Black;

    const QJsonArray moveInfos = object.value("moveInfos").toArray();
    response.moveInfos.reserve(static_cast<std::size_t>(moveInfos.size()));
    for (const QJsonValue& value : moveInfos) {
        const QJsonObject moveObject = value.toObject();
        const std::optional<lizzie::core::Move> move =
            moveFromJsonText(moveObject.value("move").toString(), boardSize, toMove);
        if (!move.has_value()) {
            continue;
        }

        lizzie::core::MoveCandidate candidate{*move};
        const std::optional<int> visits = intFromJson(moveObject.value("visits"));
        if (!visits.has_value()) {
            continue;
        }
        candidate.visits = *visits;
        candidate.winrate = rateField(moveObject, "winrate").value_or(0.0);
        candidate.scoreMean = doubleField(moveObject, "scoreMean", "scoreLead");
        candidate.scoreStdev = doubleField(moveObject, "scoreStdev");
        candidate.policy = rateField(moveObject, "policy", "prior").value_or(0.0);
        candidate.pv = pvFromJson(moveObject.value("pv"), boardSize, toMove);
        candidate.pvVisits = intArrayFromJson(moveObject.value("pvVisits"));
        if (!candidate.pvVisits.empty() && candidate.pvVisits.size() != candidate.pv.size()) {
            candidate.pvVisits.clear();
        }
        candidate.ownership = ownershipArrayFromJson(moveObject.value("ownership"), boardSize);
        response.moveInfos.push_back(std::move(candidate));
    }
    std::ranges::sort(response.moveInfos, [&](const auto& left, const auto& right) {
        if (left.visits != right.visits) {
            return left.visits > right.visits;
        }
        return moveSortKey(left.move, boardSize) < moveSortKey(right.move, boardSize);
    });
    if (!response.moveInfos.empty()) {
        const lizzie::core::MoveCandidate& best = response.moveInfos.front();
        if (!hasPositiveRootVisits) {
            response.rootVisits = best.visits;
        }
        if (!rootWinrate.has_value()) {
            response.rootWinrate = best.winrate;
        }
        if (!rootScoreMean.has_value()) {
            response.rootScoreMean = best.scoreMean;
        }
    }
    if (response.errorMessage.empty() && !hasRootInfo && response.moveInfos.empty() && response.ownership.empty()) {
        return std::nullopt;
    }

    return response;
}

}  // namespace lizzie::engine

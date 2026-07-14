#include "AnalysisProcess.h"
#include "BatchAnalysis.h"
#include "GtpProtocol.h"
#include "KataGoProcess.h"
#include "PositionSync.h"
#include "RealtimeAnalysis.h"
#include "Sgf.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QTimer>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace lizzie::core;
using namespace lizzie::engine;

namespace {

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

std::optional<std::string> envValue(const char* name)
{
    const char* value = std::getenv(name);
    if (value == nullptr) {
        return std::nullopt;
    }
    return std::string(value);
}

std::string boolText(bool value)
{
    return value ? "true" : "false";
}

bool hasNonWhitespaceText(std::string_view value)
{
    return std::any_of(value.begin(), value.end(), [](unsigned char character) {
        return !std::isspace(character);
    });
}

void requireUsableEnvPath(std::string_view environmentName, const std::string& value, bool requireExecutable)
{
    const bool hasText = hasNonWhitespaceText(value);
    const QFileInfo info(QString::fromStdString(value));
    const bool exists = hasText && info.exists();
    const bool isFile = exists && info.isFile();
    const bool readable = isFile && info.isReadable();
    const bool executable = isFile && info.isExecutable();
    const bool usable = hasText && readable && (!requireExecutable || executable);
    if (usable) {
        return;
    }

    std::ostringstream message;
    message << environmentName << " is invalid: value=" << value
            << "; hasText=" << boolText(hasText)
            << "; absolutePath=" << (hasText ? info.absoluteFilePath().toStdString() : std::string("(blank)"))
            << "; canonicalPath=" << (exists ? info.canonicalFilePath().toStdString() : std::string("(missing)"))
            << "; exists=" << boolText(exists)
            << "; file=" << boolText(isFile)
            << "; readable=" << boolText(readable);
    if (requireExecutable) {
        message << "; executable=" << boolText(executable);
    }
    throw std::runtime_error(message.str());
}

void printIntegrationEnvPathStatus(
    std::string_view environmentName,
    const std::optional<std::string>& value,
    bool requireExecutable)
{
    const bool isSet = value.has_value();
    const std::string text = isSet ? *value : std::string();
    const bool hasText = isSet && hasNonWhitespaceText(text);
    const QFileInfo info(QString::fromStdString(text));
    const bool exists = hasText && info.exists();
    const bool isFile = exists && info.isFile();
    const bool readable = isFile && info.isReadable();
    const bool executable = isFile && info.isExecutable();
    const bool usable = hasText && readable && (!requireExecutable || executable);
    const std::string prefix = "katago.integration.env." + std::string(environmentName);

    std::cout << prefix << ".set: " << boolText(isSet) << '\n';
    std::cout << prefix << ".value: " << (isSet ? (text.empty() ? std::string("(empty)") : text) : std::string("(unset)"))
              << '\n';
    std::cout << prefix << ".hasText: " << boolText(hasText) << '\n';
    std::cout << prefix << ".absolutePath: "
              << (hasText ? info.absoluteFilePath().toStdString() : std::string("(blank)")) << '\n';
    std::cout << prefix << ".canonicalPath: "
              << (exists ? info.canonicalFilePath().toStdString() : std::string("(missing)")) << '\n';
    std::cout << prefix << ".exists: " << boolText(exists) << '\n';
    std::cout << prefix << ".file: " << boolText(isFile) << '\n';
    std::cout << prefix << ".readable: " << boolText(readable) << '\n';
    if (requireExecutable) {
        std::cout << prefix << ".executable: " << boolText(executable) << '\n';
    }
    std::cout << prefix << ".usable: " << boolText(usable) << '\n';
}

void printIntegrationEnvStatus(
    const std::optional<std::string>& executable,
    const std::optional<std::string>& model,
    const std::optional<std::string>& analysisConfig,
    const std::optional<std::string>& gtpConfig)
{
    printIntegrationEnvPathStatus("LIZZIE_KATAGO_EXECUTABLE", executable, true);
    printIntegrationEnvPathStatus("LIZZIE_KATAGO_MODEL", model, false);
    printIntegrationEnvPathStatus("LIZZIE_KATAGO_ANALYSIS_CONFIG", analysisConfig, false);
    printIntegrationEnvPathStatus("LIZZIE_KATAGO_GTP_CONFIG", gtpConfig, false);
}

int timeoutMs()
{
    const std::optional<std::string> value = envValue("LIZZIE_KATAGO_TIMEOUT_MS");
    if (!value.has_value()) {
        return 30000;
    }
    try {
        return std::max(1000, std::stoi(*value));
    } catch (...) {
        return 30000;
    }
}

constexpr std::string_view kHandicapNineByNineSgf = "(;GM[1]FF[4]SZ[9]RU[Chinese]KM[7.5]HA[2]AB[dd][fd])";
constexpr std::string_view kRectangularNineByThirteenSgf = "(;GM[1]FF[4]SZ[9:13]RU[Japanese]KM[6.5])";
constexpr std::string_view kBranchNineByNineSgf = "(;GM[1]FF[4]SZ[9]RU[Chinese]KM[7.5];B[dd](;W[ee])(;W[ff]))";
constexpr std::string_view kEmptyNineByNineSgf = "(;GM[1]FF[4]SZ[9]RU[Chinese]KM[7.5])";

AnalysisRequest makeHandicapNineByNineRequest()
{
    const GameModel model = SgfParser().parseGame(kHandicapNineByNineSgf);

    BatchAnalysisOptions options;
    options.maxVisits = 8;
    options.includeOwnership = true;
    const BatchAnalysisPlan plan = buildBatchAnalysisPlan(model, {model.rootId()}, options);
    require(
        plan.warnings.empty(),
        plan.warnings.empty() ? "handicap analysis request should not warn" : plan.warnings.front());
    require(plan.items.size() == 1, "handicap analysis request should include the root node");

    AnalysisRequest request = plan.items.front().request;
    require(request.initialStones.size() == 2, "handicap analysis request should serialize setup stones");
    require(request.initialPlayer == Color::White, "handicap analysis request should analyze white to move");
    require(request.moves.empty(), "handicap root analysis request should not replay moves");
    request.id = "integration:handicap-9x9";
    return request;
}

GtpCommand genmoveCommand(Color color)
{
    return GtpCommand{std::nullopt, "genmove", {color == Color::Black ? "B" : "W"}};
}

std::string moveKey(const Move& move, BoardSize boardSize)
{
    switch (move.type) {
    case MoveType::Play:
        return formatPointForGtp(move.point, boardSize);
    case MoveType::Pass:
        return "pass";
    case MoveType::Resign:
        return "resign";
    }
    return {};
}

bool sameMove(const Move& left, const Move& right)
{
    return left.color == right.color && left.type == right.type && (!left.isPlay() || left.point == right.point);
}

double normalizedRateForIntegration(double value)
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

bool nearlyEqual(double left, double right)
{
    return std::abs(left - right) < 0.000001;
}

bool sameDoubleVector(const std::vector<double>& left, const std::vector<double>& right)
{
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t index = 0; index < left.size(); ++index) {
        if (!nearlyEqual(left[index], right[index])) {
            return false;
        }
    }
    return true;
}

double jsonDoubleField(const QJsonObject& object, const char* primary, const char* fallback = nullptr)
{
    const QJsonValue primaryValue = object.value(primary);
    if (primaryValue.isDouble()) {
        return primaryValue.toDouble();
    }
    if (fallback == nullptr) {
        return 0.0;
    }
    const QJsonValue fallbackValue = object.value(fallback);
    return fallbackValue.isDouble() ? fallbackValue.toDouble() : 0.0;
}

std::vector<double> jsonDoubleArray(const QJsonValue& value)
{
    std::vector<double> result;
    if (!value.isArray()) {
        return result;
    }
    const QJsonArray array = value.toArray();
    result.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& item : array) {
        if (item.isDouble()) {
            result.push_back(item.toDouble());
        }
    }
    return result;
}

std::vector<int> jsonIntArray(const QJsonValue& value)
{
    std::vector<int> result;
    if (!value.isArray()) {
        return result;
    }
    const QJsonArray array = value.toArray();
    result.reserve(static_cast<std::size_t>(array.size()));
    for (const QJsonValue& item : array) {
        if (item.isDouble()) {
            result.push_back(item.toInt());
        }
    }
    return result;
}

std::string normalizedMoveText(std::string text)
{
    for (char& ch : text) {
        ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    return text;
}

bool moveTextMatches(const Move& move, const QString& rawText, BoardSize boardSize)
{
    return normalizedMoveText(moveKey(move, boardSize)) == normalizedMoveText(rawText.toStdString());
}

std::optional<std::string> analysisResponseMismatchAgainstRawJson(
    const AnalysisResponse& response,
    BoardSize boardSize)
{
    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(QByteArray::fromStdString(response.rawJson), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        return "analysis response raw JSON should parse";
    }

    const QJsonObject object = document.object();
    if (response.id != object.value("id").toString().toStdString()) {
        return "analysis response id should match raw JSON";
    }
    if (response.errorMessage != object.value("error").toString().toStdString()) {
        return "analysis response error message should match raw JSON";
    }
    if (!response.errorMessage.empty()) {
        return std::nullopt;
    }
    if (response.winratePerspective != Color::Black) {
        return "analysis JSON winrate perspective should be forced to black";
    }

    const QJsonObject rootInfo = object.value("rootInfo").toObject();
    const bool hasRootInfo = object.value("rootInfo").isObject();
    if (hasRootInfo) {
        if (response.rootVisits != rootInfo.value("visits").toInt()) {
            return "analysis response root visits should match raw rootInfo visits";
        }
        if (!nearlyEqual(response.rootWinrate, normalizedRateForIntegration(rootInfo.value("winrate").toDouble()))) {
            return "analysis response root winrate should match raw rootInfo winrate";
        }
        if (!nearlyEqual(response.rootScoreMean, jsonDoubleField(rootInfo, "scoreMean", "scoreLead"))) {
            return "analysis response root score should match raw rootInfo score";
        }
    }

    std::vector<double> rawOwnership = jsonDoubleArray(object.value("ownership"));
    if (rawOwnership.empty() && hasRootInfo) {
        rawOwnership = jsonDoubleArray(rootInfo.value("ownership"));
    }
    if (!sameDoubleVector(response.ownership, rawOwnership)) {
        return "analysis response ownership should match raw JSON ownership";
    }

    const QJsonArray moveInfos = object.value("moveInfos").toArray();
    if (response.moveInfos.size() != static_cast<std::size_t>(moveInfos.size())) {
        return "analysis response candidate count should match raw moveInfos count";
    }

    for (qsizetype index = 0; index < moveInfos.size(); ++index) {
        const QJsonObject moveInfo = moveInfos.at(index).toObject();
        const MoveCandidate& candidate = response.moveInfos[static_cast<std::size_t>(index)];
        if (!moveTextMatches(candidate.move, moveInfo.value("move").toString(), boardSize)) {
            return "analysis response candidate move should match raw moveInfo move";
        }
        if (candidate.visits != moveInfo.value("visits").toInt()) {
            return "analysis response candidate visits should match raw moveInfo visits";
        }
        if (!nearlyEqual(candidate.winrate, normalizedRateForIntegration(moveInfo.value("winrate").toDouble()))) {
            return "analysis response candidate winrate should match raw moveInfo winrate";
        }
        if (!nearlyEqual(candidate.scoreMean, jsonDoubleField(moveInfo, "scoreMean", "scoreLead"))) {
            return "analysis response candidate score should match raw moveInfo score";
        }
        if (!nearlyEqual(candidate.scoreStdev, moveInfo.value("scoreStdev").toDouble())) {
            return "analysis response candidate score stdev should match raw moveInfo scoreStdev";
        }
        if (!nearlyEqual(candidate.policy, normalizedRateForIntegration(jsonDoubleField(moveInfo, "policy", "prior")))) {
            return "analysis response candidate policy should match raw moveInfo policy";
        }
        const QJsonArray rawPv = moveInfo.value("pv").toArray();
        if (candidate.pv.size() != static_cast<std::size_t>(rawPv.size())) {
            return "analysis response candidate PV length should match raw moveInfo PV";
        }
        for (qsizetype pvIndex = 0; pvIndex < rawPv.size(); ++pvIndex) {
            if (!moveTextMatches(
                    candidate.pv[static_cast<std::size_t>(pvIndex)],
                    rawPv.at(pvIndex).toString(),
                    boardSize)) {
                return "analysis response candidate PV should match raw moveInfo PV";
            }
        }
        if (candidate.pvVisits != jsonIntArray(moveInfo.value("pvVisits"))) {
            return "analysis response candidate PV visits should match raw moveInfo pvVisits";
        }
        if (!sameDoubleVector(candidate.ownership, jsonDoubleArray(moveInfo.value("ownership")))) {
            return "analysis response candidate ownership should match raw moveInfo ownership";
        }
    }

    if (!hasRootInfo && !response.moveInfos.empty()) {
        const auto best = std::ranges::max_element(
            response.moveInfos,
            [](const MoveCandidate& left, const MoveCandidate& right) {
                return left.visits < right.visits;
            });
        if (response.rootVisits != best->visits ||
            !nearlyEqual(response.rootWinrate, best->winrate) ||
            !nearlyEqual(response.rootScoreMean, best->scoreMean)) {
            return "analysis response root fallback should match the best raw moveInfo";
        }
    }

    return std::nullopt;
}

std::optional<std::string> snapshotMismatchAgainstRawLine(
    const AnalysisSnapshot& snapshot,
    const KataAnalyzeLine& rawLine,
    BoardSize boardSize,
    Color toMove)
{
    if (!snapshot.winratePerspective.has_value() || *snapshot.winratePerspective != toMove) {
        return "snapshot winrate perspective should match the analyzed side to move";
    }

    std::vector<MoveCandidate> expectedCandidates;
    for (const KataAnalyzeInfo& info : rawLine.infos) {
        if (std::optional<MoveCandidate> candidate = candidateFromKataAnalyzeInfo(info, boardSize, toMove)) {
            expectedCandidates.push_back(std::move(*candidate));
        }
    }
    std::ranges::sort(expectedCandidates, [&](const MoveCandidate& left, const MoveCandidate& right) {
        if (left.visits != right.visits) {
            return left.visits > right.visits;
        }
        return moveKey(left.move, boardSize) < moveKey(right.move, boardSize);
    });

    if (expectedCandidates.empty()) {
        return "raw kata-analyze line should contain at least one parseable candidate";
    }
    if (snapshot.candidates.size() != expectedCandidates.size()) {
        return "snapshot candidate count should match raw kata-analyze candidate count";
    }

    for (std::size_t index = 0; index < expectedCandidates.size(); ++index) {
        const MoveCandidate& actual = snapshot.candidates[index];
        const MoveCandidate& expected = expectedCandidates[index];
        if (!sameMove(actual.move, expected.move)) {
            return "snapshot candidate order should match raw visits-sorted KataGo output";
        }
        if (actual.visits != expected.visits) {
            return "snapshot candidate visits should match raw KataGo output";
        }
        if (!nearlyEqual(actual.winrate, expected.winrate)) {
            return "snapshot candidate winrate should match raw KataGo output";
        }
        if (!nearlyEqual(actual.scoreMean, expected.scoreMean)) {
            return "snapshot candidate score mean should match raw KataGo output";
        }
        if (!nearlyEqual(actual.scoreStdev, expected.scoreStdev)) {
            return "snapshot candidate score stdev should match raw KataGo output";
        }
        if (!nearlyEqual(actual.policy, expected.policy)) {
            return "snapshot candidate policy should match raw KataGo output";
        }
        if (actual.pv.size() != expected.pv.size()) {
            return "snapshot candidate PV length should match raw KataGo output";
        }
        for (std::size_t pvIndex = 0; pvIndex < expected.pv.size(); ++pvIndex) {
            if (!sameMove(actual.pv[pvIndex], expected.pv[pvIndex])) {
                return "snapshot candidate PV should match raw KataGo output";
            }
        }
        if (actual.pvVisits != expected.pvVisits) {
            return "snapshot candidate PV visits should match raw KataGo output";
        }
        if (!sameDoubleVector(actual.ownership, expected.ownership)) {
            return "snapshot candidate ownership should match raw KataGo output";
        }
    }

    if (rawLine.rootInfo.has_value()) {
        const KataAnalyzeRootInfo& rootInfo = *rawLine.rootInfo;
        if (snapshot.rootVisits != rootInfo.visits) {
            return "snapshot root visits should match raw rootInfo visits";
        }
        if (!nearlyEqual(snapshot.rootWinrate, normalizedRateForIntegration(rootInfo.winrate))) {
            return "snapshot root winrate should match raw rootInfo winrate";
        }
        if (!nearlyEqual(snapshot.rootScoreMean, rootInfo.scoreMean)) {
            return "snapshot root score mean should match raw rootInfo score";
        }
    } else {
        const MoveCandidate& best = expectedCandidates.front();
        if (snapshot.rootVisits != best.visits || !nearlyEqual(snapshot.rootWinrate, best.winrate) ||
            !nearlyEqual(snapshot.rootScoreMean, best.scoreMean)) {
            return "snapshot root fallback should match best raw candidate";
        }
    }
    if (!rawLine.ownership.empty() && rawLine.ownership.size() == static_cast<std::size_t>(boardSize.pointCount())) {
        if (!sameDoubleVector(snapshot.ownership, rawLine.ownership)) {
            return "snapshot ownership should match raw top-level ownership";
        }
    } else if (rawLine.rootInfo.has_value()) {
        const KataAnalyzeRootInfo& rootInfo = *rawLine.rootInfo;
        if (!rootInfo.ownership.empty() && rootInfo.ownership.size() == static_cast<std::size_t>(boardSize.pointCount()) &&
            !sameDoubleVector(snapshot.ownership, rootInfo.ownership)) {
            return "snapshot root ownership should match raw rootInfo ownership";
        }
    }

    return std::nullopt;
}

void runAnalysisIntegration(const KataGoConfig& config, int timeout)
{
    AnalysisProcess process;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool failed = false;
    bool finished = false;
    bool timedOut = false;
    std::string failureMessage;
    std::vector<AnalysisResponse> responses;

    QObject::connect(&timer, &QTimer::timeout, [&] {
        timedOut = true;
        loop.quit();
    });
    QObject::connect(&process, &AnalysisProcess::failed, [&](const QString& message) {
        failed = true;
        failureMessage = message.toStdString();
        loop.quit();
    });
    QObject::connect(&process, &AnalysisProcess::parseFailed, [&](const QString& line) {
        failed = true;
        failureMessage = "could not parse analysis stdout line: " + line.toStdString();
        loop.quit();
    });
    QObject::connect(&process, &AnalysisProcess::responseReceived, [&](const AnalysisResponse& response) {
        responses.push_back(response);
    });
    QObject::connect(&process, &AnalysisProcess::finished, [&](bool cancelled) {
        finished = !cancelled;
        loop.quit();
    });

    timer.start(timeout);
    process.startAnalysis(config, {makeHandicapNineByNineRequest()});
    loop.exec();
    timer.stop();
    process.cancel();

    require(!timedOut, "KataGo analysis integration timed out");
    require(!failed, failureMessage.empty() ? "KataGo analysis integration failed" : failureMessage);
    require(finished, "KataGo analysis process did not finish cleanly");
    require(responses.size() == 1, "KataGo analysis should return exactly one response");
    require(responses.front().id == "integration:handicap-9x9", "KataGo analysis response should preserve request id");
    require(!responses.front().moveInfos.empty(), "KataGo analysis should include candidate moves");
    require(responses.front().rootVisits > 0, "KataGo analysis should include root visits");
    if (const std::optional<std::string> mismatch =
            analysisResponseMismatchAgainstRawJson(responses.front(), BoardSize::square(9))) {
        require(false, "KataGo analysis raw-output mismatch: " + *mismatch);
    }
    require(
        responses.front().ownership.empty() || responses.front().ownership.size() == 81,
        "KataGo analysis ownership should match 9x9 board size");
}

void runRealtimeGtpIntegration(const KataGoConfig& config, int timeout)
{
    KataGoProcess process;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool failed = false;
    bool timedOut = false;
    bool listCommandsSeen = false;
    bool analyzeStarted = false;
    bool setupReplaySent = false;
    std::string failureMessage;
    std::optional<AnalysisSnapshot> snapshot;
    const GameModel model = SgfParser().parseGame(kHandicapNineByNineSgf);
    const BoardPosition position = model.currentPosition();
    RealtimeAnalysisAccumulator accumulator(model.boardSize(), position.sideToMove());

    QObject::connect(&timer, &QTimer::timeout, [&] {
        timedOut = true;
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::startupFailed, [&](const QString& message) {
        failed = true;
        failureMessage = message.toStdString();
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& queued) {
        if (!queued.command.has_value()) {
            return;
        }

        const QueuedCommand& command = *queued.command;
        const GtpResponse& response = queued.response;
        if (!response.success) {
            failed = true;
            failureMessage = "GTP command failed: " + command.name + " " + response.payload;
            loop.quit();
            return;
        }

        if (command.name != "list_commands") {
            return;
        }

        listCommandsSeen = true;
        const EngineCapabilities capabilities = EngineCapabilities::fromListCommandsPayload(response.payload);
        if (!capabilities.kataAnalyze) {
            failed = true;
            failureMessage = "KataGo GTP integration requires kata-analyze support";
            loop.quit();
            return;
        }

        PositionSyncOptions syncOptions;
        syncOptions.supportsKataSetRules = capabilities.kataSetRules;
        const PositionSyncPlan syncPlan = buildPositionSyncPlan(model, model.rootId(), syncOptions);
        if (syncPlan.fatal) {
            failed = true;
            failureMessage = syncPlan.warnings.empty()
                ? "could not build handicap GTP sync plan"
                : syncPlan.warnings.front();
            loop.quit();
            return;
        }
        for (const GtpCommand& command : syncPlan.commands) {
            process.sendCommand(command);
        }
        setupReplaySent = true;

        RealtimeAnalysisOptions options;
        options.intervalCentiseconds = 25;
        options.includeOwnership = true;
        options.player = position.sideToMove();
        process.sendCommand(buildKataAnalyzeCommand(options));
        analyzeStarted = true;
    });
    QObject::connect(&process, &KataGoProcess::kataAnalyzeLineReceived, [&](const KataAnalyzeLine& line) {
        const std::optional<AnalysisSnapshot> next = accumulator.processUpdate(line);
        if (!next.has_value() || next->candidates.empty() || next->rootVisits <= 0) {
            return;
        }
        if (const std::optional<std::string> mismatch =
                snapshotMismatchAgainstRawLine(*next, line, model.boardSize(), position.sideToMove())) {
            failed = true;
            failureMessage = "KataGo GTP realtime raw-output mismatch: " + *mismatch;
            loop.quit();
            return;
        }
        snapshot = next;
        process.sendCommand(buildStopAnalyzeCommand());
        loop.quit();
    });

    timer.start(timeout);
    process.startGtp(config);
    loop.exec();
    timer.stop();
    process.stop();

    require(!timedOut, "KataGo GTP realtime analysis timed out");
    require(!failed, failureMessage.empty() ? "KataGo GTP realtime analysis failed" : failureMessage);
    require(listCommandsSeen, "KataGo GTP realtime startup probe did not complete");
    require(setupReplaySent, "KataGo GTP realtime analysis should replay handicap setup stones");
    require(analyzeStarted, "KataGo GTP realtime analysis did not start");
    require(snapshot.has_value(), "KataGo GTP realtime analysis should return an analysis snapshot");
    require(!snapshot->candidates.empty(), "KataGo GTP realtime analysis should include candidate moves");
    require(snapshot->rootVisits > 0, "KataGo GTP realtime analysis should include root visits");
    require(
        snapshot->ownership.empty() || snapshot->ownership.size() == 81,
        "KataGo GTP realtime ownership should match 9x9 board size when present");
}

void runRectangularRealtimeGtpIntegration(const KataGoConfig& config, int timeout)
{
    KataGoProcess process;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool failed = false;
    bool timedOut = false;
    bool listCommandsSeen = false;
    bool rectangularBoardSizeAccepted = false;
    bool analyzeStarted = false;
    std::string failureMessage;
    std::optional<AnalysisSnapshot> snapshot;
    const GameModel model = SgfParser().parseGame(kRectangularNineByThirteenSgf);
    const BoardPosition position = model.currentPosition();
    RealtimeAnalysisAccumulator accumulator(model.boardSize(), position.sideToMove());

    QObject::connect(&timer, &QTimer::timeout, [&] {
        timedOut = true;
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::startupFailed, [&](const QString& message) {
        failed = true;
        failureMessage = message.toStdString();
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& queued) {
        if (!queued.command.has_value()) {
            return;
        }

        const QueuedCommand& command = *queued.command;
        const GtpResponse& response = queued.response;
        if (!response.success) {
            failed = true;
            failureMessage = "GTP command failed: " + command.name + " " + response.payload;
            loop.quit();
            return;
        }

        if (command.name == "rectangular_boardsize") {
            rectangularBoardSizeAccepted = true;
            return;
        }
        if (command.name != "list_commands") {
            return;
        }

        listCommandsSeen = true;
        const EngineCapabilities capabilities = EngineCapabilities::fromListCommandsPayload(response.payload);
        if (!capabilities.kataAnalyze) {
            failed = true;
            failureMessage = "KataGo rectangular GTP integration requires kata-analyze support";
            loop.quit();
            return;
        }

        PositionSyncOptions syncOptions;
        syncOptions.supportsKataSetRules = capabilities.kataSetRules;
        const PositionSyncPlan syncPlan = buildPositionSyncPlan(model, model.rootId(), syncOptions);
        if (syncPlan.fatal) {
            failed = true;
            failureMessage = syncPlan.warnings.empty()
                ? "could not build rectangular GTP sync plan"
                : syncPlan.warnings.front();
            loop.quit();
            return;
        }
        for (const GtpCommand& command : syncPlan.commands) {
            process.sendCommand(command);
        }

        RealtimeAnalysisOptions options;
        options.intervalCentiseconds = 25;
        options.includeOwnership = true;
        options.player = position.sideToMove();
        process.sendCommand(buildKataAnalyzeCommand(options));
        analyzeStarted = true;
    });
    QObject::connect(&process, &KataGoProcess::kataAnalyzeLineReceived, [&](const KataAnalyzeLine& line) {
        const std::optional<AnalysisSnapshot> next = accumulator.processUpdate(line);
        if (!next.has_value() || next->candidates.empty() || next->rootVisits <= 0) {
            return;
        }
        if (const std::optional<std::string> mismatch =
                snapshotMismatchAgainstRawLine(*next, line, model.boardSize(), position.sideToMove())) {
            failed = true;
            failureMessage = "KataGo rectangular realtime raw-output mismatch: " + *mismatch;
            loop.quit();
            return;
        }
        snapshot = next;
        process.sendCommand(buildStopAnalyzeCommand());
        loop.quit();
    });

    timer.start(timeout);
    process.startGtp(config);
    loop.exec();
    timer.stop();
    process.stop();

    require(!timedOut, "KataGo rectangular GTP realtime analysis timed out");
    require(!failed, failureMessage.empty() ? "KataGo rectangular GTP realtime analysis failed" : failureMessage);
    require(listCommandsSeen, "KataGo rectangular GTP startup probe did not complete");
    require(rectangularBoardSizeAccepted, "KataGo rectangular GTP sync should accept rectangular_boardsize 9 13");
    require(analyzeStarted, "KataGo rectangular GTP realtime analysis did not start");
    require(snapshot.has_value(), "KataGo rectangular GTP realtime analysis should return an analysis snapshot");
    require(!snapshot->candidates.empty(), "KataGo rectangular GTP realtime analysis should include candidate moves");
    require(snapshot->rootVisits > 0, "KataGo rectangular GTP realtime analysis should include root visits");
    require(
        snapshot->ownership.empty() || snapshot->ownership.size() == 117,
        "KataGo rectangular GTP ownership should match 9x13 board size when present");
}

void runBranchReplayRealtimeGtpIntegration(const KataGoConfig& config, int timeout)
{
    KataGoProcess process;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool failed = false;
    bool timedOut = false;
    bool listCommandsSeen = false;
    bool branchReplaySent = false;
    bool analyzeStarted = false;
    std::string failureMessage;
    const GameModel model = SgfParser().parseGame(kBranchNineByNineSgf);
    const NodeId mainMoveNode = model.root().children.front();
    const GameNode& firstMoveNode = model.node(mainMoveNode);
    require(firstMoveNode.children.size() == 2, "branch integration fixture should have two white variations");
    const NodeId siblingBranchNode = firstMoveNode.children.front();
    const NodeId selectedBranchNode = firstMoveNode.children.back();
    const BoardPosition position = model.positionAt(selectedBranchNode);
    RealtimeAnalysisAccumulator accumulator(model.boardSize(), position.sideToMove());
    std::optional<AnalysisSnapshot> snapshot;

    const auto buildAndCheckSyncPlan = [&](bool supportsKataSetRules) {
        PositionSyncOptions syncOptions;
        syncOptions.supportsKataSetRules = supportsKataSetRules;
        const PositionSyncPlan syncPlan = buildPositionSyncPlan(model, selectedBranchNode, syncOptions);
        if (syncPlan.fatal) {
            failed = true;
            failureMessage = syncPlan.warnings.empty()
                ? "could not build branch replay GTP sync plan"
                : syncPlan.warnings.front();
            return syncPlan;
        }

        std::vector<std::string> playMoves;
        for (const GtpCommand& command : syncPlan.commands) {
            if (command.name == "play" && command.arguments.size() == 2) {
                playMoves.push_back(command.arguments[0] + " " + command.arguments[1]);
            }
        }
        const Move& mainMove = *model.node(mainMoveNode).move;
        const Move& selectedMove = *model.node(selectedBranchNode).move;
        const Move& siblingMove = *model.node(siblingBranchNode).move;
        const std::string expectedMain = "B " + moveKey(mainMove, model.boardSize());
        const std::string expectedSelected = "W " + moveKey(selectedMove, model.boardSize());
        const std::string forbiddenSibling = "W " + moveKey(siblingMove, model.boardSize());

        if (playMoves.size() != 2 ||
            std::ranges::find(playMoves, expectedMain) == playMoves.end() ||
            std::ranges::find(playMoves, expectedSelected) == playMoves.end() ||
            std::ranges::find(playMoves, forbiddenSibling) != playMoves.end()) {
            failed = true;
            failureMessage = "branch replay sync should include only the selected variation path";
        }
        return syncPlan;
    };

    QObject::connect(&timer, &QTimer::timeout, [&] {
        timedOut = true;
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::startupFailed, [&](const QString& message) {
        failed = true;
        failureMessage = message.toStdString();
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& queued) {
        if (!queued.command.has_value()) {
            return;
        }

        const QueuedCommand& command = *queued.command;
        const GtpResponse& response = queued.response;
        if (!response.success) {
            failed = true;
            failureMessage = "GTP command failed: " + command.name + " " + response.payload;
            loop.quit();
            return;
        }

        if (command.name != "list_commands") {
            return;
        }

        listCommandsSeen = true;
        const EngineCapabilities capabilities = EngineCapabilities::fromListCommandsPayload(response.payload);
        if (!capabilities.kataAnalyze) {
            failed = true;
            failureMessage = "KataGo branch replay integration requires kata-analyze support";
            loop.quit();
            return;
        }

        const PositionSyncPlan syncPlan = buildAndCheckSyncPlan(capabilities.kataSetRules);
        if (failed) {
            loop.quit();
            return;
        }
        for (const GtpCommand& syncCommand : syncPlan.commands) {
            process.sendCommand(syncCommand);
        }
        branchReplaySent = true;

        RealtimeAnalysisOptions options;
        options.intervalCentiseconds = 25;
        options.includeOwnership = false;
        options.player = position.sideToMove();
        process.sendCommand(buildKataAnalyzeCommand(options));
        analyzeStarted = true;
    });
    QObject::connect(&process, &KataGoProcess::kataAnalyzeLineReceived, [&](const KataAnalyzeLine& line) {
        const std::optional<AnalysisSnapshot> next = accumulator.processUpdate(line);
        if (!next.has_value() || next->candidates.empty() || next->rootVisits <= 0) {
            return;
        }
        if (const std::optional<std::string> mismatch =
                snapshotMismatchAgainstRawLine(*next, line, model.boardSize(), position.sideToMove())) {
            failed = true;
            failureMessage = "KataGo branch replay raw-output mismatch: " + *mismatch;
            loop.quit();
            return;
        }
        snapshot = next;
        process.sendCommand(buildStopAnalyzeCommand());
        loop.quit();
    });

    timer.start(timeout);
    process.startGtp(config);
    loop.exec();
    timer.stop();
    process.stop();

    require(!timedOut, "KataGo branch replay realtime analysis timed out");
    require(!failed, failureMessage.empty() ? "KataGo branch replay realtime analysis failed" : failureMessage);
    require(listCommandsSeen, "KataGo branch replay startup probe did not complete");
    require(branchReplaySent, "KataGo branch replay should send the selected variation path");
    require(analyzeStarted, "KataGo branch replay realtime analysis did not start");
    require(snapshot.has_value(), "KataGo branch replay should return an analysis snapshot");
    require(!snapshot->candidates.empty(), "KataGo branch replay should include candidate moves");
}

void runGtpSelfPlayIntegration(const KataGoConfig& config, int timeout)
{
    KataGoProcess process;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool failed = false;
    bool timedOut = false;
    bool listCommandsSeen = false;
    std::string failureMessage;
    int generatedMoves = 0;
    Color nextColor = Color::Black;
    BoardPosition selfPlayPosition(BoardSize::square(9));

    QObject::connect(&timer, &QTimer::timeout, [&] {
        timedOut = true;
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::startupFailed, [&](const QString& message) {
        failed = true;
        failureMessage = message.toStdString();
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& queued) {
        if (!queued.command.has_value()) {
            return;
        }

        const QueuedCommand& command = *queued.command;
        const GtpResponse& response = queued.response;
        if (!response.success) {
            failed = true;
            failureMessage = "GTP command failed: " + command.name + " " + response.payload;
            loop.quit();
            return;
        }

        if (command.name == "list_commands") {
            listCommandsSeen = true;
            const EngineCapabilities capabilities = EngineCapabilities::fromListCommandsPayload(response.payload);
            if (!capabilities.genmove) {
                failed = true;
                failureMessage = "KataGo GTP integration requires genmove support";
                loop.quit();
                return;
            }
            process.sendCommand(GtpCommand{std::nullopt, "boardsize", {"9"}});
            process.sendCommand(GtpCommand{std::nullopt, "clear_board", {}});
            process.sendCommand(GtpCommand{std::nullopt, "komi", {"7.5"}});
            process.sendCommand(genmoveCommand(nextColor));
            return;
        }

        if (command.name != "genmove") {
            return;
        }

        const std::optional<Move> generatedMove =
            moveFromGtpToken(response.payload, selfPlayPosition.size(), nextColor);
        if (!generatedMove.has_value() || generatedMove->type == MoveType::Resign) {
            failed = true;
            failureMessage = "KataGo GTP genmove returned an unplayable move";
            loop.quit();
            return;
        }

        const PlayResult localPlay = selfPlayPosition.playMove(*generatedMove);
        if (!localPlay.ok) {
            failed = true;
            failureMessage = "KataGo GTP genmove returned a locally illegal move: " + localPlay.error;
            loop.quit();
            return;
        }

        ++generatedMoves;
        nextColor = selfPlayPosition.sideToMove();
        if (generatedMoves >= 2) {
            loop.quit();
            return;
        }
        process.sendCommand(genmoveCommand(nextColor));
    });

    timer.start(timeout);
    process.startGtp(config);
    loop.exec();
    timer.stop();
    process.stop();

    require(!timedOut, "KataGo GTP self-play integration timed out");
    require(!failed, failureMessage.empty() ? "KataGo GTP self-play integration failed" : failureMessage);
    require(listCommandsSeen, "KataGo GTP startup probe did not complete");
    require(generatedMoves == 2, "KataGo GTP self-play should generate two moves");
}

void runGtpHumanReplyIntegration(const KataGoConfig& config, int timeout)
{
    KataGoProcess process;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool failed = false;
    bool timedOut = false;
    bool listCommandsSeen = false;
    bool userMoveAccepted = false;
    bool legalReplyAccepted = false;
    std::string failureMessage;
    BoardPosition position(BoardSize::square(9));
    const Move userMove = Move::play(Color::Black, Point{3, 3});
    const PlayResult userPlay = position.playMove(userMove);
    require(userPlay.ok, "Human-vs-AI integration fixture should play the user move locally");

    QObject::connect(&timer, &QTimer::timeout, [&] {
        timedOut = true;
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::startupFailed, [&](const QString& message) {
        failed = true;
        failureMessage = message.toStdString();
        loop.quit();
    });
    QObject::connect(&process, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& queued) {
        if (!queued.command.has_value()) {
            return;
        }

        const QueuedCommand& command = *queued.command;
        const GtpResponse& response = queued.response;
        if (!response.success) {
            failed = true;
            failureMessage = "GTP command failed: " + command.name + " " + response.payload;
            loop.quit();
            return;
        }

        if (command.name == "list_commands") {
            listCommandsSeen = true;
            const EngineCapabilities capabilities = EngineCapabilities::fromListCommandsPayload(response.payload);
            if (!capabilities.genmove) {
                failed = true;
                failureMessage = "KataGo GTP human reply integration requires genmove support";
                loop.quit();
                return;
            }
            process.sendCommand(GtpCommand{std::nullopt, "boardsize", {"9"}});
            process.sendCommand(GtpCommand{std::nullopt, "clear_board", {}});
            process.sendCommand(GtpCommand{std::nullopt, "komi", {"7.5"}});
            process.sendCommand(GtpCommand{std::nullopt, "play", {"B", "D4"}});
            return;
        }

        if (command.name == "play") {
            userMoveAccepted = true;
            process.sendCommand(genmoveCommand(Color::White));
            return;
        }

        if (command.name != "genmove") {
            return;
        }

        const std::optional<Move> replyMove =
            moveFromGtpToken(response.payload, position.size(), Color::White);
        if (!replyMove.has_value() || replyMove->type == MoveType::Resign) {
            failed = true;
            failureMessage = "KataGo GTP human reply returned an unplayable move";
            loop.quit();
            return;
        }

        const PlayResult replyPlay = position.playMove(*replyMove);
        if (!replyPlay.ok) {
            failed = true;
            failureMessage = "KataGo GTP human reply returned a locally illegal move: " + replyPlay.error;
            loop.quit();
            return;
        }

        legalReplyAccepted = true;
        loop.quit();
    });

    timer.start(timeout);
    process.startGtp(config);
    loop.exec();
    timer.stop();
    process.stop();

    require(!timedOut, "KataGo GTP human reply integration timed out");
    require(!failed, failureMessage.empty() ? "KataGo GTP human reply integration failed" : failureMessage);
    require(listCommandsSeen, "KataGo GTP human reply startup probe did not complete");
    require(userMoveAccepted, "KataGo GTP human reply should accept the user's move before genmove");
    require(legalReplyAccepted, "KataGo GTP human reply should add one legal engine reply");
    require(position.sideToMove() == Color::Black, "KataGo GTP human reply should leave black to move after the reply");
}

void runGtpEngineGameIntegration(const KataGoConfig& config, int timeout)
{
    KataGoProcess mainProcess;
    KataGoProcess compareProcess;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool failed = false;
    bool timedOut = false;
    bool mainReady = false;
    bool compareReady = false;
    bool mainGenmoveRequested = false;
    bool mainMoveGenerated = false;
    bool compareAcceptedMainMove = false;
    bool compareMoveGenerated = false;
    bool mainAcceptedCompareMove = false;
    std::string failureMessage;
    BoardPosition position(BoardSize::square(9));

    const auto fail = [&](std::string message) {
        if (!failed) {
            failed = true;
            failureMessage = std::move(message);
            loop.quit();
        }
    };

    const auto sendInitialSync = [](KataGoProcess& process) {
        process.sendCommand(GtpCommand{std::nullopt, "boardsize", {"9"}});
        process.sendCommand(GtpCommand{std::nullopt, "clear_board", {}});
        process.sendCommand(GtpCommand{std::nullopt, "komi", {"7.5"}});
    };

    const auto maybeStartMainMove = [&] {
        if (!mainGenmoveRequested && mainReady && compareReady) {
            mainGenmoveRequested = true;
            mainProcess.sendCommand(genmoveCommand(Color::Black));
        }
    };

    const auto handleListCommands = [&](const QueuedGtpResponse& queued, const char* label, bool* ready) {
        const EngineCapabilities capabilities = EngineCapabilities::fromListCommandsPayload(queued.response.payload);
        if (!capabilities.genmove) {
            fail(std::string(label) + " engine-game integration requires genmove support");
            return;
        }
        *ready = true;
        maybeStartMainMove();
    };

    QObject::connect(&timer, &QTimer::timeout, [&] {
        timedOut = true;
        loop.quit();
    });
    QObject::connect(&mainProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
        fail("main engine-game KataGo startup failed: " + message.toStdString());
    });
    QObject::connect(&compareProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
        fail("compare engine-game KataGo startup failed: " + message.toStdString());
    });
    QObject::connect(&mainProcess, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& queued) {
        if (!queued.command.has_value()) {
            return;
        }

        const QueuedCommand& command = *queued.command;
        const GtpResponse& response = queued.response;
        if (!response.success) {
            fail("main engine-game GTP command failed: " + command.name + " " + response.payload);
            return;
        }

        if (command.name == "list_commands") {
            sendInitialSync(mainProcess);
            handleListCommands(queued, "main", &mainReady);
            return;
        }

        if (command.name == "genmove") {
            const std::optional<Move> generatedMove =
                moveFromGtpToken(response.payload, position.size(), Color::Black);
            if (!generatedMove.has_value() || generatedMove->type == MoveType::Resign) {
                fail("main engine-game genmove returned an unplayable move");
                return;
            }
            const PlayResult localPlay = position.playMove(*generatedMove);
            if (!localPlay.ok) {
                fail("main engine-game genmove returned a locally illegal move: " + localPlay.error);
                return;
            }

            mainMoveGenerated = true;
            compareProcess.sendCommand(
                GtpCommand{std::nullopt, "play", {"B", moveKey(*generatedMove, position.size())}});
            return;
        }

        if (command.name == "play") {
            mainAcceptedCompareMove = true;
            loop.quit();
        }
    });
    QObject::connect(&compareProcess, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& queued) {
        if (!queued.command.has_value()) {
            return;
        }

        const QueuedCommand& command = *queued.command;
        const GtpResponse& response = queued.response;
        if (!response.success) {
            fail("compare engine-game GTP command failed: " + command.name + " " + response.payload);
            return;
        }

        if (command.name == "list_commands") {
            sendInitialSync(compareProcess);
            handleListCommands(queued, "compare", &compareReady);
            return;
        }

        if (command.name == "play") {
            compareAcceptedMainMove = true;
            compareProcess.sendCommand(genmoveCommand(Color::White));
            return;
        }

        if (command.name == "genmove") {
            const std::optional<Move> generatedMove =
                moveFromGtpToken(response.payload, position.size(), Color::White);
            if (!generatedMove.has_value() || generatedMove->type == MoveType::Resign) {
                fail("compare engine-game genmove returned an unplayable move");
                return;
            }
            const PlayResult localPlay = position.playMove(*generatedMove);
            if (!localPlay.ok) {
                fail("compare engine-game genmove returned a locally illegal move: " + localPlay.error);
                return;
            }

            compareMoveGenerated = true;
            mainProcess.sendCommand(
                GtpCommand{std::nullopt, "play", {"W", moveKey(*generatedMove, position.size())}});
        }
    });

    timer.start(timeout);
    mainProcess.startGtp(config);
    compareProcess.startGtp(config);
    loop.exec();
    timer.stop();
    mainProcess.stop();
    compareProcess.stop();

    require(!timedOut, "KataGo GTP engine-game integration timed out");
    require(!failed, failureMessage.empty() ? "KataGo GTP engine-game integration failed" : failureMessage);
    require(mainReady, "main engine-game KataGo startup probe did not complete");
    require(compareReady, "compare engine-game KataGo startup probe did not complete");
    require(mainMoveGenerated, "main engine-game KataGo should generate the black move");
    require(compareAcceptedMainMove, "compare engine-game KataGo should accept the main engine move");
    require(compareMoveGenerated, "compare engine-game KataGo should generate the white move");
    require(mainAcceptedCompareMove, "main engine-game KataGo should accept the compare engine move");
    require(position.sideToMove() == Color::Black, "KataGo GTP engine-game should leave black to move after two moves");
}

void runDualRealtimeCompareIntegration(const KataGoConfig& config, int timeout)
{
    KataGoProcess mainProcess;
    KataGoProcess compareProcess;
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);

    bool failed = false;
    bool timedOut = false;
    bool mainListCommandsSeen = false;
    bool compareListCommandsSeen = false;
    bool mainAnalyzeStarted = false;
    bool compareAnalyzeStarted = false;
    std::string failureMessage;
    const GameModel model = SgfParser().parseGame(kEmptyNineByNineSgf);
    const BoardPosition position = model.currentPosition();
    RealtimeAnalysisAccumulator mainAccumulator(model.boardSize(), position.sideToMove());
    RealtimeAnalysisAccumulator compareAccumulator(model.boardSize(), position.sideToMove());
    std::optional<AnalysisSnapshot> mainSnapshot;
    std::optional<AnalysisSnapshot> compareSnapshot;

    const auto fail = [&](std::string message) {
        if (!failed) {
            failed = true;
            failureMessage = std::move(message);
            loop.quit();
        }
    };

    const auto startAnalyze =
        [&](KataGoProcess& process, const char* label, bool* listSeen, bool* analyzeStarted, bool supportsKataSetRules) {
        *listSeen = true;
        PositionSyncOptions syncOptions;
        syncOptions.supportsKataSetRules = supportsKataSetRules;
        const PositionSyncPlan syncPlan = buildPositionSyncPlan(model, model.rootId(), syncOptions);
        if (syncPlan.fatal) {
            fail(std::string(label) + " compare sync plan failed: " +
                (syncPlan.warnings.empty() ? "unknown error" : syncPlan.warnings.front()));
            return;
        }
        for (const GtpCommand& command : syncPlan.commands) {
            process.sendCommand(command);
        }

        RealtimeAnalysisOptions options;
        options.intervalCentiseconds = 25;
        options.includeOwnership = false;
        options.player = position.sideToMove();
        process.sendCommand(buildKataAnalyzeCommand(options));
        *analyzeStarted = true;
    };

    const auto handleCommandResponse =
        [&](KataGoProcess& process, const char* label, bool* listSeen, bool* analyzeStarted, const QueuedGtpResponse& queued) {
            if (!queued.command.has_value()) {
                return;
            }

            const QueuedCommand& command = *queued.command;
            const GtpResponse& response = queued.response;
            if (!response.success && command.name != "kata-stop") {
                fail(std::string(label) + " GTP command failed: " + command.name + " " + response.payload);
                return;
            }
            if (command.name != "list_commands") {
                return;
            }

            const EngineCapabilities capabilities = EngineCapabilities::fromListCommandsPayload(response.payload);
            if (!capabilities.kataAnalyze) {
                fail(std::string(label) + " compare integration requires kata-analyze support");
                return;
            }
            startAnalyze(process, label, listSeen, analyzeStarted, capabilities.kataSetRules);
        };

    const auto handleAnalyzeLine =
        [&](KataGoProcess& process,
            RealtimeAnalysisAccumulator& accumulator,
            std::optional<AnalysisSnapshot>* snapshot,
            const char* label,
            const KataAnalyzeLine& line) {
            if (snapshot->has_value()) {
                return;
            }
            const std::optional<AnalysisSnapshot> next = accumulator.processUpdate(line);
            if (!next.has_value() || next->candidates.empty() || next->rootVisits <= 0) {
                return;
            }
            if (const std::optional<std::string> mismatch =
                    snapshotMismatchAgainstRawLine(*next, line, model.boardSize(), position.sideToMove())) {
                fail(std::string(label) + " compare raw-output mismatch: " + *mismatch);
                return;
            }
            *snapshot = next;
            process.sendCommand(buildStopAnalyzeCommand());
            if (mainSnapshot.has_value() && compareSnapshot.has_value()) {
                loop.quit();
            }
        };

    QObject::connect(&timer, &QTimer::timeout, [&] {
        timedOut = true;
        loop.quit();
    });
    QObject::connect(&mainProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
        fail("main compare KataGo startup failed: " + message.toStdString());
    });
    QObject::connect(&compareProcess, &KataGoProcess::startupFailed, [&](const QString& message) {
        fail("compare KataGo startup failed: " + message.toStdString());
    });
    QObject::connect(&mainProcess, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& queued) {
        handleCommandResponse(mainProcess, "main", &mainListCommandsSeen, &mainAnalyzeStarted, queued);
    });
    QObject::connect(&compareProcess, &KataGoProcess::commandResponseReceived, [&](const QueuedGtpResponse& queued) {
        handleCommandResponse(compareProcess, "compare", &compareListCommandsSeen, &compareAnalyzeStarted, queued);
    });
    QObject::connect(&mainProcess, &KataGoProcess::kataAnalyzeLineReceived, [&](const KataAnalyzeLine& line) {
        handleAnalyzeLine(mainProcess, mainAccumulator, &mainSnapshot, "main", line);
    });
    QObject::connect(&compareProcess, &KataGoProcess::kataAnalyzeLineReceived, [&](const KataAnalyzeLine& line) {
        handleAnalyzeLine(compareProcess, compareAccumulator, &compareSnapshot, "compare", line);
    });

    timer.start(timeout);
    mainProcess.startGtp(config);
    compareProcess.startGtp(config);
    loop.exec();
    timer.stop();
    mainProcess.stop();
    compareProcess.stop();

    require(!timedOut, "KataGo dual realtime compare integration timed out");
    require(!failed, failureMessage.empty() ? "KataGo dual realtime compare integration failed" : failureMessage);
    require(mainListCommandsSeen, "main compare KataGo startup probe did not complete");
    require(compareListCommandsSeen, "compare KataGo startup probe did not complete");
    require(mainAnalyzeStarted, "main compare kata-analyze did not start");
    require(compareAnalyzeStarted, "compare kata-analyze did not start");
    require(mainSnapshot.has_value(), "main compare KataGo should return an analysis snapshot");
    require(compareSnapshot.has_value(), "compare KataGo should return an analysis snapshot");
    require(!mainSnapshot->candidates.empty(), "main compare KataGo should include candidates");
    require(!compareSnapshot->candidates.empty(), "compare KataGo should include candidates");
}

}  // namespace

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);

    const std::optional<std::string> executable = envValue("LIZZIE_KATAGO_EXECUTABLE");
    const std::optional<std::string> model = envValue("LIZZIE_KATAGO_MODEL");
    const std::optional<std::string> analysisConfig = envValue("LIZZIE_KATAGO_ANALYSIS_CONFIG");
    const std::optional<std::string> gtpConfig = envValue("LIZZIE_KATAGO_GTP_CONFIG");

    if (!executable.has_value() && !model.has_value() && !analysisConfig.has_value() && !gtpConfig.has_value()) {
        std::cout << "katago_integration_tests skipped; set LIZZIE_KATAGO_* env vars to enable\n";
        printIntegrationEnvStatus(executable, model, analysisConfig, gtpConfig);
        return EXIT_SUCCESS;
    }

    try {
        require(executable.has_value(), "LIZZIE_KATAGO_EXECUTABLE is required");
        require(model.has_value(), "LIZZIE_KATAGO_MODEL is required");
        require(
            analysisConfig.has_value() || gtpConfig.has_value(),
            "set LIZZIE_KATAGO_ANALYSIS_CONFIG or LIZZIE_KATAGO_GTP_CONFIG");
        requireUsableEnvPath("LIZZIE_KATAGO_EXECUTABLE", *executable, true);
        requireUsableEnvPath("LIZZIE_KATAGO_MODEL", *model, false);
        if (analysisConfig.has_value()) {
            requireUsableEnvPath("LIZZIE_KATAGO_ANALYSIS_CONFIG", *analysisConfig, false);
        }
        if (gtpConfig.has_value()) {
            requireUsableEnvPath("LIZZIE_KATAGO_GTP_CONFIG", *gtpConfig, false);
        }

        KataGoConfig config;
        config.executable = std::filesystem::path(*executable);
        config.model = std::filesystem::path(*model);

        const int timeout = timeoutMs();
        if (analysisConfig.has_value()) {
            config.analysisConfig = std::filesystem::path(*analysisConfig);
            runAnalysisIntegration(config, timeout);
        }
        if (gtpConfig.has_value()) {
            config.gtpConfig = std::filesystem::path(*gtpConfig);
            runRealtimeGtpIntegration(config, timeout);
            runRectangularRealtimeGtpIntegration(config, timeout);
            runBranchReplayRealtimeGtpIntegration(config, timeout);
            runDualRealtimeCompareIntegration(config, timeout);
            runGtpHumanReplyIntegration(config, timeout);
            runGtpEngineGameIntegration(config, timeout);
            runGtpSelfPlayIntegration(config, timeout);
        }
    } catch (const std::exception& error) {
        std::cerr << "katago_integration_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "katago_integration_tests passed\n";
    return EXIT_SUCCESS;
}

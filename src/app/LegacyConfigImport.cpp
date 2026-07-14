#include "LegacyConfigImport.h"

#include <QFileInfo>
#include <QJsonArray>
#include <QJsonValue>

#include <algorithm>
#include <cmath>
#include <optional>
#include <vector>

namespace lizzie::app {

namespace {

constexpr int kMaxAnalysisIntervalCentiseconds = 10000;
constexpr int kMaxSearchLimit = 100000000;
constexpr double kMaxAnalysisTimeSeconds = 86400.0;
constexpr double kMinPlayoutDoublingAdvantage = -10.0;
constexpr double kMaxPlayoutDoublingAdvantage = 10.0;
constexpr double kMaxWideRootNoise = 10.0;

struct LegacyKataGoCommand {
    lizzie::engine::KataGoConfig config;
    bool isGtp = false;
    bool isAnalysis = false;
};

struct LegacyEngineEntry {
    QString command;
    bool isDefault = false;
};

QString resolveLegacyExecutablePath(const QString& value, const QDir& baseDir)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    const QFileInfo info(trimmed);
    if (info.isAbsolute()) {
        return info.filePath();
    }
    if (trimmed.contains('/') || trimmed.contains('\\')) {
        return QDir::cleanPath(baseDir.absoluteFilePath(trimmed));
    }
    return trimmed;
}

QString resolveLegacyFilePath(const QString& value, const QDir& baseDir)
{
    const QString trimmed = value.trimmed();
    if (trimmed.isEmpty()) {
        return {};
    }
    const QFileInfo info(trimmed);
    if (info.isAbsolute()) {
        return info.filePath();
    }
    return QDir::cleanPath(baseDir.absoluteFilePath(trimmed));
}

bool isLikelyKataGoExecutable(const QString& value)
{
    return QFileInfo(value.trimmed()).fileName().toLower().contains("katago");
}

std::optional<double> legacyDoubleValue(const QJsonValue& value)
{
    if (value.isDouble()) {
        const double number = value.toDouble();
        return std::isfinite(number) ? std::optional<double>(number) : std::nullopt;
    }
    if (!value.isString()) {
        return std::nullopt;
    }

    bool ok = false;
    const double number = value.toString().trimmed().toDouble(&ok);
    if (!ok || !std::isfinite(number)) {
        return std::nullopt;
    }
    return number;
}

std::optional<bool> legacyBoolValue(const QJsonValue& value)
{
    if (value.isBool()) {
        return value.toBool();
    }
    if (!value.isString()) {
        return std::nullopt;
    }

    const QString text = value.toString().trimmed().toLower();
    if (text == "true" || text == "1") {
        return true;
    }
    if (text == "false" || text == "0") {
        return false;
    }
    return std::nullopt;
}

bool legacyBoolValue(const QJsonValue& value, bool fallback)
{
    return legacyBoolValue(value).value_or(fallback);
}

std::optional<int> positiveBoundedLegacyInt(const QJsonValue& value, int maximum)
{
    if (value.isDouble()) {
        const double number = value.toDouble();
        if (!std::isfinite(number) || std::floor(number) != number || number <= 0.0) {
            return std::nullopt;
        }
        return static_cast<int>(std::min(number, static_cast<double>(maximum)));
    }
    if (!value.isString()) {
        return std::nullopt;
    }

    bool ok = false;
    const qlonglong number = value.toString().trimmed().toLongLong(&ok);
    if (!ok || number <= 0) {
        return std::nullopt;
    }
    return static_cast<int>(std::min<qlonglong>(number, maximum));
}

std::optional<double> positiveBoundedLegacyDouble(const QJsonValue& value, double maximum)
{
    const std::optional<double> number = legacyDoubleValue(value);
    if (!number.has_value() || *number <= 0.0) {
        return std::nullopt;
    }
    return std::min(*number, maximum);
}

std::optional<double> nonZeroBoundedLegacyDouble(const QJsonValue& value, double minimum, double maximum)
{
    const std::optional<double> number = legacyDoubleValue(value);
    if (!number.has_value() || *number == 0.0) {
        return std::nullopt;
    }
    return std::clamp(*number, minimum, maximum);
}

QJsonValue legacyValueWithFallback(const QJsonObject& object, const char* primary, const char* fallback)
{
    const QJsonValue primaryValue = object.value(QLatin1String(primary));
    return primaryValue.isUndefined() ? object.value(QLatin1String(fallback)) : primaryValue;
}

std::optional<LegacyKataGoCommand> parseLegacyKataGoCommand(
    const QString& command,
    const QDir& baseDir,
    const QString& workingDirectory = {})
{
    QStringList tokens;
    for (const std::string& token : splitCommandArguments(command)) {
        tokens.push_back(QString::fromStdString(token));
    }
    if (tokens.isEmpty()) {
        return std::nullopt;
    }
    if (!isLikelyKataGoExecutable(tokens.front())) {
        return std::nullopt;
    }

    int subcommandIndex = -1;
    bool isGtp = false;
    bool isAnalysis = false;
    for (int index = 1; index < tokens.size(); ++index) {
        const QString token = tokens[index].toLower();
        if (token == "gtp" || token == "analysis") {
            subcommandIndex = index;
            isGtp = token == "gtp";
            isAnalysis = token == "analysis";
            break;
        }
    }
    if (subcommandIndex < 0) {
        return std::nullopt;
    }

    LegacyKataGoCommand parsed;
    parsed.isGtp = isGtp;
    parsed.isAnalysis = isAnalysis;
    parsed.config.executable = resolveLegacyExecutablePath(tokens.front(), baseDir).toStdString();
    if (!workingDirectory.isEmpty()) {
        parsed.config.workingDirectory = workingDirectory.toStdString();
    }

    for (int index = 1; index < tokens.size(); ++index) {
        if (index == subcommandIndex) {
            continue;
        }
        const QString token = tokens[index];
        const QString lower = token.toLower();
        if ((lower == "-model" || lower == "--model") && index + 1 < tokens.size()) {
            parsed.config.model = resolveLegacyFilePath(tokens[++index], baseDir).toStdString();
        } else if ((lower == "-config" || lower == "--config") && index + 1 < tokens.size()) {
            const QString configPath = resolveLegacyFilePath(tokens[++index], baseDir);
            if (isAnalysis) {
                parsed.config.analysisConfig = configPath.toStdString();
            } else {
                parsed.config.gtpConfig = configPath.toStdString();
            }
        } else if (lower == "-quit-without-waiting") {
            continue;
        } else {
            parsed.config.extraArgs.push_back(token.toStdString());
        }
    }

    return parsed;
}

void mergeLegacyCommand(lizzie::engine::KataGoConfig* target, const LegacyKataGoCommand& parsed)
{
    if (target->executable.empty()) {
        target->executable = parsed.config.executable;
    }
    if (target->model.empty()) {
        target->model = parsed.config.model;
    }
    if (parsed.isGtp && target->gtpConfig.empty()) {
        target->gtpConfig = parsed.config.gtpConfig;
    }
    if (parsed.isAnalysis && target->analysisConfig.empty()) {
        target->analysisConfig = parsed.config.analysisConfig;
    }
    if (target->workingDirectory.empty()) {
        target->workingDirectory = parsed.config.workingDirectory;
    }
    if (target->extraArgs.empty()) {
        target->extraArgs = parsed.config.extraArgs;
    }
}

QJsonObject jsonObjectValue(const QJsonObject& object, const char* key)
{
    const QJsonValue value = object.value(QLatin1String(key));
    return value.isObject() ? value.toObject() : QJsonObject{};
}

std::vector<LegacyEngineEntry> legacyEngineEntries(const QJsonObject& leelaz, QStringList* notes)
{
    std::vector<LegacyEngineEntry> entries;
    const QJsonArray settingsList = leelaz.value("engine-settings-list").toArray();
    for (const QJsonValue& value : settingsList) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject object = value.toObject();
        if (legacyBoolValue(object.value("useJavaSSH"), false)) {
            if (notes != nullptr) {
                notes->push_back("Skipped Java SSH engine entry");
            }
            continue;
        }
        const QString command = object.value("command").toString().trimmed();
        if (!command.isEmpty()) {
            entries.push_back(LegacyEngineEntry{command, legacyBoolValue(object.value("isDefault"), false)});
        }
    }

    if (entries.empty()) {
        const QString primary = leelaz.value("engine-command").toString().trimmed();
        if (!primary.isEmpty()) {
            entries.push_back(LegacyEngineEntry{primary, true});
        }
        for (const QJsonValue& value : leelaz.value("engine-command-list").toArray()) {
            const QString command = value.toString().trimmed();
            if (!command.isEmpty()) {
                entries.push_back(LegacyEngineEntry{command, false});
            }
        }
    }

    auto defaultEntry = std::ranges::find_if(entries, [](const LegacyEngineEntry& entry) {
        return entry.isDefault;
    });
    if (defaultEntry != entries.end() && defaultEntry != entries.begin()) {
        std::rotate(entries.begin(), defaultEntry, defaultEntry + 1);
    }
    return entries;
}

}  // namespace

LegacyConfigImportResult importLegacyConfigObject(
    const QJsonObject& root,
    const QDir& baseDir,
    const EngineUiSettings& currentSettings)
{
    LegacyConfigImportResult result;
    result.settings = currentSettings;
    result.settings.config = {};
    result.settings.comparisonConfig = {};

    const QJsonObject leelaz = jsonObjectValue(root, "leelaz");
    const QJsonObject ui = jsonObjectValue(root, "ui");
    const QString workingDirectory =
        resolveLegacyFilePath(leelaz.value("engine-start-location").toString(), baseDir);

    const std::vector<LegacyEngineEntry> engines = legacyEngineEntries(leelaz, &result.notes);
    for (const LegacyEngineEntry& entry : engines) {
        const std::optional<LegacyKataGoCommand> parsed =
            parseLegacyKataGoCommand(entry.command, baseDir, workingDirectory);
        if (!parsed.has_value()) {
            result.notes.push_back("Skipped engine command that is not a KataGo gtp/analysis command");
            continue;
        }
        lizzie::engine::KataGoConfig* target =
            result.importedEngines == 0 ? &result.settings.config : &result.settings.comparisonConfig;
        mergeLegacyCommand(target, *parsed);
        ++result.importedEngines;
        if (result.importedEngines >= 2) {
            break;
        }
    }

    if (const std::optional<LegacyKataGoCommand> analysis =
            parseLegacyKataGoCommand(ui.value("analysis-engine-command").toString(), baseDir, workingDirectory)) {
        if (analysis->isAnalysis) {
            mergeLegacyCommand(&result.settings.config, *analysis);
        }
    }
    if (result.settings.config.gtpConfig.empty()) {
        if (const std::optional<LegacyKataGoCommand> estimate =
                parseLegacyKataGoCommand(
                    ui.value("katago-estimate-engine-command").toString(),
                    baseDir,
                    workingDirectory)) {
            if (estimate->isGtp) {
                mergeLegacyCommand(&result.settings.config, *estimate);
            }
        }
    }
    if (result.importedEngines == 0 &&
        (result.settings.config.hasGtpMinimum() || result.settings.config.hasAnalysisMinimum())) {
        result.importedEngines = 1;
    }

    const QJsonValue intervalValue = leelaz.value("analyze-update-interval-centisec");
    if (const std::optional<int> interval = positiveBoundedLegacyInt(intervalValue, kMaxAnalysisIntervalCentiseconds)) {
        result.settings.realtimeOptions.intervalCentiseconds = *interval;
    }
    if (legacyBoolValue(ui.value("limit-time"), false)) {
        if (const std::optional<double> maxAnalyzeSeconds =
                positiveBoundedLegacyDouble(leelaz.value("max-analyze-time-seconds"), kMaxAnalysisTimeSeconds)) {
            result.settings.realtimeOptions.maxTimeSeconds = *maxAnalyzeSeconds;
        }
    }
    if (legacyBoolValue(ui.value("limit-playout"), false)) {
        if (const std::optional<int> maxPlayouts =
                positiveBoundedLegacyInt(ui.value("limit-playouts"), kMaxSearchLimit)) {
            result.settings.realtimeOptions.maxPlayouts = *maxPlayouts;
        }
    }
    if (legacyBoolValue(ui.value("kata-visits-playouts-settings"), false)) {
        if (const std::optional<int> visits = positiveBoundedLegacyInt(ui.value("kata-visits-txt"), kMaxSearchLimit)) {
            result.settings.realtimeOptions.maxVisits = *visits;
        }
        if (const std::optional<int> playouts =
                positiveBoundedLegacyInt(ui.value("kata-playouts-txt"), kMaxSearchLimit)) {
            result.settings.realtimeOptions.maxPlayouts = *playouts;
        }
    }

    const bool usePda =
        legacyBoolValue(ui.value("autoload-kata-engine-pda"), false) ||
        legacyBoolValue(ui.value("chk-kata-engine-pda"), false);
    if (usePda) {
        if (const std::optional<double> pda = nonZeroBoundedLegacyDouble(
                legacyValueWithFallback(ui, "auto-load-txt-kata-engine-pda", "txt-kata-engine-pda"),
                kMinPlayoutDoublingAdvantage,
                kMaxPlayoutDoublingAdvantage)) {
            result.settings.realtimeOptions.playoutDoublingAdvantage = *pda;
        }
    }
    const bool useWrn =
        legacyBoolValue(ui.value("autoload-kata-engine-wrn"), false) ||
        legacyBoolValue(ui.value("chk-kata-engine-wrn"), false);
    if (useWrn) {
        if (const std::optional<double> wrn = positiveBoundedLegacyDouble(
                legacyValueWithFallback(ui, "auto-load-txt-kata-engine-wrn", "txt-kata-engine-wrn"),
                kMaxWideRootNoise)) {
            result.settings.realtimeOptions.analysisWideRootNoise = *wrn;
        }
    }

    const int language = ui.value("use-language").toInt(0);
    if (language == 1) {
        result.settings.appearance.language = LanguageMode::Chinese;
    } else if (language == 2) {
        result.settings.appearance.language = LanguageMode::English;
    }
    const QString theme = ui.value("theme").toString().toLower();
    if (theme.contains("dark")) {
        result.settings.appearance.theme = ThemeMode::Dark;
    } else if (theme.contains("light")) {
        result.settings.appearance.theme = ThemeMode::Light;
    }
    if (legacyBoolValue(ui.value("show-katago-estimate"), false) ||
        legacyBoolValue(ui.value("show-katago-estimate-onmainboard"), false)) {
        result.settings.boardDisplay.showOwnership = true;
    }

    result.hasUsableKataGo = result.settings.config.hasGtpMinimum() || result.settings.config.hasAnalysisMinimum();
    return result;
}

}  // namespace lizzie::app

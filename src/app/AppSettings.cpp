#include "AppSettings.h"

#include <algorithm>
#include <cmath>
#include <optional>

#include <QMetaType>
#include <QDir>
#include <QSettings>
#include <QStringList>
#include <QVariant>

namespace lizzie::app {

namespace {

constexpr int kDefaultAnalysisIntervalCentiseconds = 50;
constexpr int kMinAnalysisIntervalCentiseconds = 1;
constexpr int kMaxAnalysisIntervalCentiseconds = 10000;
constexpr int kMaxSearchLimit = 100000000;
constexpr double kMaxAnalysisTimeSeconds = 86400.0;
constexpr double kMinPlayoutDoublingAdvantage = -10.0;
constexpr double kMaxPlayoutDoublingAdvantage = 10.0;
constexpr double kMaxWideRootNoise = 10.0;
constexpr double kMinFontScale = 0.75;
constexpr double kMaxFontScale = 2.0;
constexpr double kDefaultOwnershipOpacity = 0.45;
constexpr double kMinOwnershipOpacity = 0.05;
constexpr double kMaxOwnershipOpacity = 1.0;

QString quoteCommandArgument(const QString& value)
{
    bool needsQuoting = value.isEmpty();
    for (const QChar ch : value) {
        if (ch.isSpace() || ch == '"') {
            needsQuoting = true;
            break;
        }
    }
    if (!needsQuoting) {
        return value;
    }

    QString escaped;
    escaped.reserve(value.size() + 2);
    for (const QChar ch : value) {
        if (ch == '"') {
            escaped += "\"\"\"";
        } else {
            escaped.push_back(ch);
        }
    }
    return '"' + escaped + '"';
}

QString shortcutSettingKey(const char* name)
{
    return QString("shortcuts/") + QLatin1String(name);
}

QKeySequence loadShortcutSetting(QSettings& settings, const char* name, const QKeySequence& fallback)
{
    const QString key = shortcutSettingKey(name);
    if (!settings.contains(key)) {
        return fallback;
    }
    return QKeySequence(settings.value(key).toString(), QKeySequence::PortableText);
}

void saveShortcutSetting(QSettings& settings, const char* name, const QKeySequence& sequence)
{
    settings.setValue(shortcutSettingKey(name), sequence.toString(QKeySequence::PortableText));
}

std::optional<bool> boolFromStoredValue(const QVariant& value)
{
    if (value.metaType().id() == QMetaType::Bool) {
        return value.toBool();
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

bool boolSetting(QSettings& settings, const char* key, bool fallback)
{
    if (!settings.contains(QLatin1String(key))) {
        return fallback;
    }
    return boolFromStoredValue(settings.value(QLatin1String(key))).value_or(fallback);
}

int boundedIntSetting(QSettings& settings, const char* key, int fallback, int minimum, int maximum)
{
    bool ok = false;
    const int value = settings.value(QLatin1String(key), fallback).toInt(&ok);
    if (!ok) {
        return fallback;
    }
    return std::clamp(value, minimum, maximum);
}

int boundedIntForSave(int value, int minimum, int maximum)
{
    return std::clamp(value, minimum, maximum);
}

std::optional<int> positiveBoundedIntSetting(QSettings& settings, const char* key, int maximum)
{
    bool ok = false;
    const int value = settings.value(QLatin1String(key), 0).toInt(&ok);
    if (!ok || value <= 0) {
        return std::nullopt;
    }
    return std::min(value, maximum);
}

int positiveBoundedIntForSave(const std::optional<int>& value, int maximum)
{
    if (!value.has_value() || *value <= 0) {
        return 0;
    }
    return std::min(*value, maximum);
}

double boundedDoubleSetting(QSettings& settings, const char* key, double fallback, double minimum, double maximum)
{
    bool ok = false;
    const double value = settings.value(QLatin1String(key), fallback).toDouble(&ok);
    if (!ok || !std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minimum, maximum);
}

double boundedDoubleForSave(double value, double fallback, double minimum, double maximum)
{
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, minimum, maximum);
}

std::optional<double> positiveBoundedDoubleSetting(QSettings& settings, const char* key, double maximum)
{
    bool ok = false;
    const double value = settings.value(QLatin1String(key), 0.0).toDouble(&ok);
    if (!ok || !std::isfinite(value) || value <= 0.0) {
        return std::nullopt;
    }
    return std::min(value, maximum);
}

double positiveBoundedDoubleForSave(const std::optional<double>& value, double maximum)
{
    if (!value.has_value() || !std::isfinite(*value) || *value <= 0.0) {
        return 0.0;
    }
    return std::min(*value, maximum);
}

std::optional<double> nonZeroBoundedDoubleSetting(QSettings& settings, const char* key, double minimum, double maximum)
{
    bool ok = false;
    const double value = settings.value(QLatin1String(key), 0.0).toDouble(&ok);
    if (!ok || !std::isfinite(value) || value == 0.0) {
        return std::nullopt;
    }
    return std::clamp(value, minimum, maximum);
}

double nonZeroBoundedDoubleForSave(const std::optional<double>& value, double minimum, double maximum)
{
    if (!value.has_value() || !std::isfinite(*value) || *value == 0.0) {
        return 0.0;
    }
    return std::clamp(*value, minimum, maximum);
}

}  // namespace

std::vector<std::string> splitCommandArguments(const QString& text)
{
    std::vector<std::string> result;
    QString current;
    bool inQuotes = false;
    bool tokenStarted = false;
    for (qsizetype index = 0; index < text.size();) {
        const QChar ch = text.at(index);
        if (ch == '"') {
            if (index + 2 < text.size() && text.at(index + 1) == '"' && text.at(index + 2) == '"') {
                current.push_back('"');
                tokenStarted = true;
                index += 3;
                continue;
            }
            inQuotes = !inQuotes;
            tokenStarted = true;
            ++index;
            continue;
        }
        if (!inQuotes && ch.isSpace()) {
            if (tokenStarted) {
                result.push_back(current.toStdString());
                current.clear();
                tokenStarted = false;
            }
            ++index;
            continue;
        }
        current.push_back(ch);
        tokenStarted = true;
        ++index;
    }
    if (tokenStarted) {
        result.push_back(current.toStdString());
    }
    return result;
}

QString joinCommandArguments(const std::vector<std::string>& values)
{
    QStringList result;
    for (const std::string& value : values) {
        result.push_back(quoteCommandArgument(QString::fromStdString(value)));
    }
    return result.join(' ');
}

std::vector<std::filesystem::path> splitPathList(const QString& text)
{
    std::vector<std::filesystem::path> result;
    for (const QString& path : text.split(QDir::listSeparator(), Qt::SkipEmptyParts)) {
        const QString trimmed = path.trimmed();
        if (!trimmed.isEmpty()) {
            result.push_back(trimmed.toStdString());
        }
    }
    return result;
}

QString joinPathList(const std::vector<std::filesystem::path>& values)
{
    QStringList result;
    for (const std::filesystem::path& value : values) {
        const QString path = QString::fromStdString(value.string()).trimmed();
        if (!path.isEmpty()) {
            result.push_back(path);
        }
    }
    return result.join(QDir::listSeparator());
}

OwnershipDisplayMode ownershipDisplayModeFromSetting(const QString& value)
{
    if (value == "mini") {
        return OwnershipDisplayMode::MiniBoard;
    }
    if (value == "both") {
        return OwnershipDisplayMode::BothBoards;
    }
    return OwnershipDisplayMode::MainBoard;
}

QString ownershipDisplayModeSetting(OwnershipDisplayMode mode)
{
    switch (mode) {
    case OwnershipDisplayMode::MiniBoard:
        return "mini";
    case OwnershipDisplayMode::BothBoards:
        return "both";
    case OwnershipDisplayMode::MainBoard:
        return "main";
    }
    return "main";
}

EngineUiSettings loadEngineUiSettings(QSettings& settings)
{
    EngineUiSettings result;
    result.config.executable = settings.value("engine/executable").toString().toStdString();
    result.config.model = settings.value("engine/model").toString().toStdString();
    result.config.gtpConfig = settings.value("engine/gtpConfig").toString().toStdString();
    result.config.analysisConfig = settings.value("engine/analysisConfig").toString().toStdString();
    result.config.workingDirectory = settings.value("engine/workingDirectory").toString().toStdString();
    result.config.libraryPaths = splitPathList(settings.value("engine/libraryPaths").toString());
    result.config.extraArgs = splitCommandArguments(settings.value("engine/extraArgs").toString());
    result.comparisonConfig.executable = settings.value("compare/executable").toString().toStdString();
    result.comparisonConfig.model = settings.value("compare/model").toString().toStdString();
    result.comparisonConfig.gtpConfig = settings.value("compare/gtpConfig").toString().toStdString();
    result.comparisonConfig.analysisConfig = settings.value("compare/analysisConfig").toString().toStdString();
    result.comparisonConfig.workingDirectory = settings.value("compare/workingDirectory").toString().toStdString();
    result.comparisonConfig.libraryPaths = splitPathList(settings.value("compare/libraryPaths").toString());
    result.comparisonConfig.extraArgs = splitCommandArguments(settings.value("compare/extraArgs").toString());
    result.realtimeOptions.intervalCentiseconds = boundedIntSetting(
        settings,
        "analysis/intervalCentiseconds",
        kDefaultAnalysisIntervalCentiseconds,
        kMinAnalysisIntervalCentiseconds,
        kMaxAnalysisIntervalCentiseconds);
    result.realtimeOptions.includeOwnership = boolSetting(settings, "analysis/includeOwnership", true);
    if (const std::optional<int> maxVisits = positiveBoundedIntSetting(settings, "analysis/maxVisits", kMaxSearchLimit)) {
        result.realtimeOptions.maxVisits = *maxVisits;
    }
    if (const std::optional<int> maxPlayouts = positiveBoundedIntSetting(settings, "analysis/maxPlayouts", kMaxSearchLimit)) {
        result.realtimeOptions.maxPlayouts = *maxPlayouts;
    }
    if (const std::optional<double> maxTime =
            positiveBoundedDoubleSetting(settings, "analysis/maxTimeSeconds", kMaxAnalysisTimeSeconds)) {
        result.realtimeOptions.maxTimeSeconds = *maxTime;
    }
    if (const std::optional<double> pda = nonZeroBoundedDoubleSetting(
            settings,
            "analysis/playoutDoublingAdvantage",
            kMinPlayoutDoublingAdvantage,
            kMaxPlayoutDoublingAdvantage)) {
        result.realtimeOptions.playoutDoublingAdvantage = *pda;
    }
    if (const std::optional<double> wrn =
            positiveBoundedDoubleSetting(settings, "analysis/analysisWideRootNoise", kMaxWideRootNoise)) {
        result.realtimeOptions.analysisWideRootNoise = *wrn;
    }

    const QString theme = settings.value("appearance/theme", "system").toString();
    if (theme == "light") {
        result.appearance.theme = ThemeMode::Light;
    } else if (theme == "dark") {
        result.appearance.theme = ThemeMode::Dark;
    } else {
        result.appearance.theme = ThemeMode::System;
    }
    result.appearance.language =
        settings.value("appearance/language", "en").toString() == "zh" ? LanguageMode::Chinese : LanguageMode::English;
    result.appearance.fontScale = boundedDoubleSetting(settings, "appearance/fontScale", 1.0, kMinFontScale, kMaxFontScale);
    result.boardDisplay.showOwnership = boolSetting(settings, "board/showOwnership", true);
    result.boardDisplay.ownershipDisplayMode =
        ownershipDisplayModeFromSetting(settings.value("board/ownershipDisplayMode", "main").toString());
    result.boardDisplay.ownershipOpacity = boundedDoubleSetting(
        settings,
        "board/ownershipOpacity",
        kDefaultOwnershipOpacity,
        kMinOwnershipOpacity,
        kMaxOwnershipOpacity);
    result.boardDisplay.blackStoneTexture = settings.value("board/blackStoneTexture").toString().toStdString();
    result.boardDisplay.whiteStoneTexture = settings.value("board/whiteStoneTexture").toString().toStdString();
    result.fileBehavior.importLegacySgfAnalysis = boolSetting(settings, "files/importLegacySgfAnalysis", true);
    result.fileBehavior.loadAnalysisSidecar = boolSetting(settings, "files/loadAnalysisSidecar", true);
    result.fileBehavior.writeAnalysisSidecarAfterBatch =
        boolSetting(settings, "files/writeAnalysisSidecarAfterBatch", true);

    const ShortcutSettings defaultShortcuts;
    result.shortcuts.newGame = loadShortcutSetting(settings, "new", defaultShortcuts.newGame);
    result.shortcuts.openSgf = loadShortcutSetting(settings, "open", defaultShortcuts.openSgf);
    result.shortcuts.saveSgf = loadShortcutSetting(settings, "save", defaultShortcuts.saveSgf);
    result.shortcuts.saveSgfAs = loadShortcutSetting(settings, "saveAs", defaultShortcuts.saveSgfAs);
    result.shortcuts.analyze = loadShortcutSetting(settings, "analyze", defaultShortcuts.analyze);
    result.shortcuts.estimate = loadShortcutSetting(settings, "estimate", defaultShortcuts.estimate);
    result.shortcuts.batchAnalyze = loadShortcutSetting(settings, "batch", defaultShortcuts.batchAnalyze);
    result.shortcuts.restartEngine = loadShortcutSetting(settings, "restartEngine", defaultShortcuts.restartEngine);
    result.shortcuts.compare = loadShortcutSetting(settings, "compare", defaultShortcuts.compare);
    result.shortcuts.aiMove = loadShortcutSetting(settings, "aiMove", defaultShortcuts.aiMove);
    result.shortcuts.humanVsAi = loadShortcutSetting(settings, "humanVsAi", defaultShortcuts.humanVsAi);
    result.shortcuts.selfPlay = loadShortcutSetting(settings, "selfPlay", defaultShortcuts.selfPlay);
    result.shortcuts.engineGame = loadShortcutSetting(settings, "engineGame", defaultShortcuts.engineGame);
    result.shortcuts.previousMove = loadShortcutSetting(settings, "previous", defaultShortcuts.previousMove);
    result.shortcuts.nextMove = loadShortcutSetting(settings, "next", defaultShortcuts.nextMove);
    result.shortcuts.undoMove = loadShortcutSetting(settings, "undo", defaultShortcuts.undoMove);
    result.shortcuts.passMove = loadShortcutSetting(settings, "pass", defaultShortcuts.passMove);
    result.shortcuts.resignMove = loadShortcutSetting(settings, "resign", defaultShortcuts.resignMove);
    result.shortcuts.settings = loadShortcutSetting(settings, "settings", defaultShortcuts.settings);
    return result;
}

void saveEngineUiSettings(QSettings& settings, const EngineUiSettings& appSettings)
{
    settings.setValue("engine/executable", QString::fromStdString(appSettings.config.executable.string()));
    settings.setValue("engine/model", QString::fromStdString(appSettings.config.model.string()));
    settings.setValue("engine/gtpConfig", QString::fromStdString(appSettings.config.gtpConfig.string()));
    settings.setValue("engine/analysisConfig", QString::fromStdString(appSettings.config.analysisConfig.string()));
    settings.setValue(
        "engine/workingDirectory",
        QString::fromStdString(appSettings.config.workingDirectory.string()));
    settings.setValue("engine/libraryPaths", joinPathList(appSettings.config.libraryPaths));
    settings.setValue("engine/extraArgs", joinCommandArguments(appSettings.config.extraArgs));
    settings.setValue(
        "compare/executable",
        QString::fromStdString(appSettings.comparisonConfig.executable.string()));
    settings.setValue("compare/model", QString::fromStdString(appSettings.comparisonConfig.model.string()));
    settings.setValue("compare/gtpConfig", QString::fromStdString(appSettings.comparisonConfig.gtpConfig.string()));
    settings.setValue(
        "compare/analysisConfig",
        QString::fromStdString(appSettings.comparisonConfig.analysisConfig.string()));
    settings.setValue(
        "compare/workingDirectory",
        QString::fromStdString(appSettings.comparisonConfig.workingDirectory.string()));
    settings.setValue("compare/libraryPaths", joinPathList(appSettings.comparisonConfig.libraryPaths));
    settings.setValue("compare/extraArgs", joinCommandArguments(appSettings.comparisonConfig.extraArgs));
    settings.setValue(
        "analysis/intervalCentiseconds",
        boundedIntForSave(
            appSettings.realtimeOptions.intervalCentiseconds,
            kMinAnalysisIntervalCentiseconds,
            kMaxAnalysisIntervalCentiseconds));
    settings.setValue("analysis/includeOwnership", appSettings.realtimeOptions.includeOwnership);
    settings.setValue(
        "analysis/maxVisits",
        positiveBoundedIntForSave(appSettings.realtimeOptions.maxVisits, kMaxSearchLimit));
    settings.setValue(
        "analysis/maxPlayouts",
        positiveBoundedIntForSave(appSettings.realtimeOptions.maxPlayouts, kMaxSearchLimit));
    settings.setValue(
        "analysis/maxTimeSeconds",
        positiveBoundedDoubleForSave(appSettings.realtimeOptions.maxTimeSeconds, kMaxAnalysisTimeSeconds));
    settings.setValue(
        "analysis/playoutDoublingAdvantage",
        nonZeroBoundedDoubleForSave(
            appSettings.realtimeOptions.playoutDoublingAdvantage,
            kMinPlayoutDoublingAdvantage,
            kMaxPlayoutDoublingAdvantage));
    settings.setValue(
        "analysis/analysisWideRootNoise",
        positiveBoundedDoubleForSave(appSettings.realtimeOptions.analysisWideRootNoise, kMaxWideRootNoise));

    QString theme = "system";
    if (appSettings.appearance.theme == ThemeMode::Light) {
        theme = "light";
    } else if (appSettings.appearance.theme == ThemeMode::Dark) {
        theme = "dark";
    }
    settings.setValue("appearance/theme", theme);
    settings.setValue(
        "appearance/language",
        appSettings.appearance.language == LanguageMode::Chinese ? "zh" : "en");
    settings.setValue(
        "appearance/fontScale",
        boundedDoubleForSave(appSettings.appearance.fontScale, 1.0, kMinFontScale, kMaxFontScale));
    settings.setValue("board/showOwnership", appSettings.boardDisplay.showOwnership);
    settings.setValue(
        "board/ownershipDisplayMode",
        ownershipDisplayModeSetting(appSettings.boardDisplay.ownershipDisplayMode));
    settings.setValue(
        "board/ownershipOpacity",
        boundedDoubleForSave(
            appSettings.boardDisplay.ownershipOpacity,
            kDefaultOwnershipOpacity,
            kMinOwnershipOpacity,
            kMaxOwnershipOpacity));
    settings.setValue(
        "board/blackStoneTexture",
        QString::fromStdString(appSettings.boardDisplay.blackStoneTexture.string()));
    settings.setValue(
        "board/whiteStoneTexture",
        QString::fromStdString(appSettings.boardDisplay.whiteStoneTexture.string()));
    settings.setValue("files/importLegacySgfAnalysis", appSettings.fileBehavior.importLegacySgfAnalysis);
    settings.setValue("files/loadAnalysisSidecar", appSettings.fileBehavior.loadAnalysisSidecar);
    settings.setValue(
        "files/writeAnalysisSidecarAfterBatch",
        appSettings.fileBehavior.writeAnalysisSidecarAfterBatch);

    saveShortcutSetting(settings, "new", appSettings.shortcuts.newGame);
    saveShortcutSetting(settings, "open", appSettings.shortcuts.openSgf);
    saveShortcutSetting(settings, "save", appSettings.shortcuts.saveSgf);
    saveShortcutSetting(settings, "saveAs", appSettings.shortcuts.saveSgfAs);
    saveShortcutSetting(settings, "analyze", appSettings.shortcuts.analyze);
    saveShortcutSetting(settings, "estimate", appSettings.shortcuts.estimate);
    saveShortcutSetting(settings, "batch", appSettings.shortcuts.batchAnalyze);
    saveShortcutSetting(settings, "restartEngine", appSettings.shortcuts.restartEngine);
    saveShortcutSetting(settings, "compare", appSettings.shortcuts.compare);
    saveShortcutSetting(settings, "aiMove", appSettings.shortcuts.aiMove);
    saveShortcutSetting(settings, "humanVsAi", appSettings.shortcuts.humanVsAi);
    saveShortcutSetting(settings, "selfPlay", appSettings.shortcuts.selfPlay);
    saveShortcutSetting(settings, "engineGame", appSettings.shortcuts.engineGame);
    saveShortcutSetting(settings, "previous", appSettings.shortcuts.previousMove);
    saveShortcutSetting(settings, "next", appSettings.shortcuts.nextMove);
    saveShortcutSetting(settings, "undo", appSettings.shortcuts.undoMove);
    saveShortcutSetting(settings, "pass", appSettings.shortcuts.passMove);
    saveShortcutSetting(settings, "resign", appSettings.shortcuts.resignMove);
    saveShortcutSetting(settings, "settings", appSettings.shortcuts.settings);
}

}  // namespace lizzie::app

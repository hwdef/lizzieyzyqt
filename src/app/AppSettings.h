#pragma once

#include "KataGoConfig.h"
#include "PositionSync.h"

#include <QKeySequence>
#include <QString>

#include <filesystem>
#include <string>
#include <vector>

class QSettings;

namespace lizzie::app {

enum class ThemeMode {
    System,
    Light,
    Dark,
};

enum class LanguageMode {
    English,
    Chinese,
};

enum class OwnershipDisplayMode {
    MainBoard,
    MiniBoard,
    BothBoards,
};

struct AppearanceSettings {
    ThemeMode theme = ThemeMode::System;
    LanguageMode language = LanguageMode::English;
    double fontScale = 1.0;
};

struct BoardDisplaySettings {
    bool showOwnership = true;
    OwnershipDisplayMode ownershipDisplayMode = OwnershipDisplayMode::MainBoard;
    double ownershipOpacity = 0.45;
    std::filesystem::path blackStoneTexture;
    std::filesystem::path whiteStoneTexture;
};

struct FileBehaviorSettings {
    bool importLegacySgfAnalysis = true;
    bool loadAnalysisSidecar = true;
    bool writeAnalysisSidecarAfterBatch = true;
};

struct ShortcutSettings {
    QKeySequence newGame = QKeySequence(QKeySequence::New);
    QKeySequence openSgf = QKeySequence(QKeySequence::Open);
    QKeySequence saveSgf = QKeySequence(QKeySequence::Save);
    QKeySequence saveSgfAs = QKeySequence(QKeySequence::SaveAs);
    QKeySequence analyze = QKeySequence(Qt::Key_F5);
    QKeySequence estimate = QKeySequence("Ctrl+E");
    QKeySequence batchAnalyze = QKeySequence("Ctrl+B");
    QKeySequence restartEngine = QKeySequence("Ctrl+R");
    QKeySequence compare = QKeySequence("Ctrl+D");
    QKeySequence aiMove = QKeySequence("Ctrl+G");
    QKeySequence humanVsAi = QKeySequence("Ctrl+H");
    QKeySequence selfPlay = QKeySequence("Ctrl+Shift+G");
    QKeySequence engineGame = QKeySequence("Ctrl+Shift+E");
    QKeySequence previousMove = QKeySequence(Qt::Key_Left);
    QKeySequence nextMove = QKeySequence(Qt::Key_Right);
    QKeySequence undoMove = QKeySequence(QKeySequence::Undo);
    QKeySequence passMove = QKeySequence("Ctrl+P");
    QKeySequence resignMove = QKeySequence("Ctrl+Shift+P");
    QKeySequence settings = QKeySequence(QKeySequence::Preferences);
};

struct EngineUiSettings {
    lizzie::engine::KataGoConfig config;
    lizzie::engine::KataGoConfig comparisonConfig;
    lizzie::engine::RealtimeAnalysisOptions realtimeOptions;
    AppearanceSettings appearance;
    BoardDisplaySettings boardDisplay;
    FileBehaviorSettings fileBehavior;
    ShortcutSettings shortcuts;
};

[[nodiscard]] std::vector<std::string> splitCommandArguments(const QString& text);
[[nodiscard]] QString joinCommandArguments(const std::vector<std::string>& values);
[[nodiscard]] std::vector<std::filesystem::path> splitPathList(const QString& text);
[[nodiscard]] QString joinPathList(const std::vector<std::filesystem::path>& values);
[[nodiscard]] OwnershipDisplayMode ownershipDisplayModeFromSetting(const QString& value);
[[nodiscard]] QString ownershipDisplayModeSetting(OwnershipDisplayMode mode);
[[nodiscard]] EngineUiSettings loadEngineUiSettings(QSettings& settings);
void saveEngineUiSettings(QSettings& settings, const EngineUiSettings& appSettings);

}  // namespace lizzie::app

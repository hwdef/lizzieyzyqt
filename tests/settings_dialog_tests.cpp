#include "EngineSettingsDialog.h"

#include <QApplication>
#include <QByteArray>
#include <QComboBox>
#include <QKeySequence>
#include <QLabel>
#include <QPushButton>
#include <QStringList>
#include <QVariant>
#include <QWidget>

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

QPushButton* findPathBrowseButton(lizzie::ui::EngineSettingsDialog& dialog, const char* key)
{
    for (QPushButton* button : dialog.findChildren<QPushButton*>("lizziePathBrowseButton")) {
        if (button->property("lizzieSettingsPathRowKey").toByteArray() == key) {
            return button;
        }
    }
    return nullptr;
}

}  // namespace

int main(int argc, char* argv[])
{
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }

    QApplication app(argc, argv);
    lizzie::ui::EngineSettingsDialog dialog;

    lizzie::ui::EngineUiSettings settings;
    settings.config.executable = "/opt/katago";
    settings.config.model = "/models/main.bin.gz";
    settings.config.gtpConfig = "/configs/main-gtp.cfg";
    settings.config.analysisConfig = "/configs/main-analysis.cfg";
    settings.config.workingDirectory = "/work/main";
    settings.config.libraryPaths = {"/opt/cuda/lib", "/opt/cudnn/lib"};
    settings.config.extraArgs = {"--override-config", "logDir=/tmp/main logs", R"(quoted"value\path)"};
    settings.comparisonConfig.executable = "/opt/katago-compare";
    settings.comparisonConfig.model = "/models/compare.bin.gz";
    settings.comparisonConfig.gtpConfig = "/configs/compare-gtp.cfg";
    settings.comparisonConfig.analysisConfig = "/configs/compare-analysis.cfg";
    settings.comparisonConfig.workingDirectory = "/work/compare";
    settings.comparisonConfig.libraryPaths = {"/opt/cuda-compare/lib", "/opt/cudnn-compare/lib"};
    settings.comparisonConfig.extraArgs = {"--override-config", "logDir=/tmp/compare logs", R"(compare"quoted)"};
    settings.realtimeOptions.intervalCentiseconds = 123;
    settings.realtimeOptions.includeOwnership = false;
    settings.realtimeOptions.maxVisits = 777;
    settings.realtimeOptions.maxPlayouts = 888;
    settings.realtimeOptions.maxTimeSeconds = 9.5;
    settings.realtimeOptions.playoutDoublingAdvantage = -1.25;
    settings.realtimeOptions.analysisWideRootNoise = 0.35;
    settings.appearance.theme = lizzie::ui::ThemeMode::Dark;
    settings.appearance.fontScale = 1.35;
    settings.boardDisplay.showOwnership = false;
    settings.boardDisplay.ownershipDisplayMode = lizzie::ui::OwnershipDisplayMode::BothBoards;
    settings.boardDisplay.ownershipOpacity = 0.7;
    settings.boardDisplay.blackStoneTexture = "/tmp/black-stone.png";
    settings.boardDisplay.whiteStoneTexture = "/tmp/white-stone.png";
    settings.fileBehavior.importLegacySgfAnalysis = false;
    settings.fileBehavior.loadAnalysisSidecar = false;
    settings.fileBehavior.writeAnalysisSidecarAfterBatch = false;
    settings.appearance.language = lizzie::ui::LanguageMode::Chinese;
    settings.shortcuts.saveSgfAs = QKeySequence("Ctrl+Shift+S");
    settings.shortcuts.analyze = QKeySequence("Ctrl+Alt+A");
    settings.shortcuts.humanVsAi = QKeySequence("Ctrl+Alt+H");
    settings.shortcuts.undoMove = QKeySequence("Ctrl+Alt+Z");
    settings.shortcuts.passMove = QKeySequence();
    settings.shortcuts.resignMove = QKeySequence("Ctrl+Alt+R");
    settings.shortcuts.settings = QKeySequence("Ctrl+,");

    dialog.setSettings(settings);
    const lizzie::ui::EngineUiSettings roundTrip = dialog.settings();

    require(roundTrip.config.executable == "/opt/katago", "main executable path should round-trip");
    require(roundTrip.config.model == "/models/main.bin.gz", "main model path should round-trip");
    require(roundTrip.config.gtpConfig == "/configs/main-gtp.cfg", "main GTP config path should round-trip");
    require(
        roundTrip.config.analysisConfig == "/configs/main-analysis.cfg",
        "main analysis config path should round-trip");
    require(roundTrip.config.workingDirectory == "/work/main", "main working directory should round-trip");
    require(roundTrip.config.libraryPaths == settings.config.libraryPaths, "main library paths should round-trip");
    require(
        roundTrip.config.extraArgs.size() == 3 && roundTrip.config.extraArgs[0] == "--override-config" &&
            roundTrip.config.extraArgs[1] == "logDir=/tmp/main logs" &&
            roundTrip.config.extraArgs[2] == R"(quoted"value\path)",
        "main extra args should round-trip");
    require(
        roundTrip.comparisonConfig.executable == "/opt/katago-compare",
        "compare executable path should round-trip");
    require(
        roundTrip.comparisonConfig.model == "/models/compare.bin.gz",
        "compare model path should round-trip");
    require(
        roundTrip.comparisonConfig.gtpConfig == "/configs/compare-gtp.cfg",
        "compare GTP config path should round-trip");
    require(
        roundTrip.comparisonConfig.analysisConfig == "/configs/compare-analysis.cfg",
        "compare analysis config path should round-trip");
    require(
        roundTrip.comparisonConfig.workingDirectory == "/work/compare",
        "compare working directory should round-trip");
    require(
        roundTrip.comparisonConfig.libraryPaths == settings.comparisonConfig.libraryPaths,
        "compare library paths should round-trip");
    require(
        roundTrip.comparisonConfig.extraArgs.size() == 3 &&
            roundTrip.comparisonConfig.extraArgs[0] == "--override-config" &&
            roundTrip.comparisonConfig.extraArgs[1] == "logDir=/tmp/compare logs" &&
            roundTrip.comparisonConfig.extraArgs[2] == R"(compare"quoted)",
        "compare extra args should round-trip");
    require(roundTrip.realtimeOptions.intervalCentiseconds == 123, "interval should round-trip");
    require(!roundTrip.realtimeOptions.includeOwnership, "analysis ownership setting should round-trip");
    require(roundTrip.realtimeOptions.maxVisits == 777, "max visits should round-trip");
    require(roundTrip.realtimeOptions.maxPlayouts == 888, "max playouts should round-trip");
    require(
        roundTrip.realtimeOptions.maxTimeSeconds.has_value() &&
            *roundTrip.realtimeOptions.maxTimeSeconds > 9.49 &&
            *roundTrip.realtimeOptions.maxTimeSeconds < 9.51,
        "max time should round-trip");
    require(
        roundTrip.realtimeOptions.playoutDoublingAdvantage.has_value() &&
            *roundTrip.realtimeOptions.playoutDoublingAdvantage > -1.26 &&
            *roundTrip.realtimeOptions.playoutDoublingAdvantage < -1.24,
        "PDA should round-trip");
    require(
        roundTrip.realtimeOptions.analysisWideRootNoise.has_value() &&
            *roundTrip.realtimeOptions.analysisWideRootNoise > 0.34 &&
            *roundTrip.realtimeOptions.analysisWideRootNoise < 0.36,
        "WRN should round-trip");
    require(roundTrip.appearance.theme == lizzie::ui::ThemeMode::Dark, "theme should round-trip");
    require(roundTrip.appearance.fontScale > 1.34 && roundTrip.appearance.fontScale < 1.36, "font scale should round-trip");
    require(
        roundTrip.boardDisplay.ownershipOpacity > 0.69 && roundTrip.boardDisplay.ownershipOpacity < 0.71,
        "ownership opacity should round-trip");
    require(!roundTrip.boardDisplay.showOwnership, "ownership visibility should round-trip");
    require(
        roundTrip.boardDisplay.ownershipDisplayMode == lizzie::ui::OwnershipDisplayMode::BothBoards,
        "ownership display mode should round-trip");
    require(
        roundTrip.boardDisplay.blackStoneTexture == "/tmp/black-stone.png",
        "black stone texture path should round-trip");
    require(
        roundTrip.boardDisplay.whiteStoneTexture == "/tmp/white-stone.png",
        "white stone texture path should round-trip");
    require(!roundTrip.fileBehavior.importLegacySgfAnalysis, "legacy SGF analysis setting should round-trip");
    require(!roundTrip.fileBehavior.loadAnalysisSidecar, "sidecar load setting should round-trip");
    require(!roundTrip.fileBehavior.writeAnalysisSidecarAfterBatch, "sidecar write setting should round-trip");
    require(roundTrip.appearance.language == lizzie::ui::LanguageMode::Chinese, "language should round-trip");
    require(roundTrip.shortcuts.saveSgfAs == QKeySequence("Ctrl+Shift+S"), "save-as shortcut should round-trip");
    require(roundTrip.shortcuts.analyze == QKeySequence("Ctrl+Alt+A"), "custom analyze shortcut should round-trip");
    require(roundTrip.shortcuts.humanVsAi == QKeySequence("Ctrl+Alt+H"), "custom Human vs AI shortcut should round-trip");
    require(roundTrip.shortcuts.undoMove == QKeySequence("Ctrl+Alt+Z"), "custom undo shortcut should round-trip");
    require(roundTrip.shortcuts.passMove.isEmpty(), "empty pass shortcut should stay disabled");
    require(roundTrip.shortcuts.resignMove == QKeySequence("Ctrl+Alt+R"), "resign shortcut should round-trip");
    require(roundTrip.shortcuts.settings == QKeySequence("Ctrl+,"), "settings shortcut should round-trip");
    require(dialog.windowTitle() == "设置", "settings dialog title should translate to Chinese");
    int zhUnlimitedSpinBoxes = 0;
    for (QSpinBox* spinBox : dialog.findChildren<QSpinBox*>()) {
        if (spinBox->specialValueText() == QString::fromUtf8("不限制")) {
            ++zhUnlimitedSpinBoxes;
        }
    }
    int zhUnlimitedDoubleSpinBoxes = 0;
    int zhPdaTooltips = 0;
    for (QDoubleSpinBox* spinBox : dialog.findChildren<QDoubleSpinBox*>()) {
        if (spinBox->specialValueText() == QString::fromUtf8("不限制")) {
            ++zhUnlimitedDoubleSpinBoxes;
        }
        if (spinBox->toolTip() == QString::fromUtf8("0 表示关闭 PDA") &&
            spinBox->accessibleDescription() == QString::fromUtf8("0 表示关闭 PDA")) {
            ++zhPdaTooltips;
        }
    }
    require(zhUnlimitedSpinBoxes == 2, "Chinese settings should mark visit/playout zero limits as unlimited");
    require(zhUnlimitedDoubleSpinBoxes == 2, "Chinese settings should mark time/WRN zero limits as unlimited");
    require(zhPdaTooltips == 1, "Chinese settings should explain zero PDA without changing its negative range");

    lizzie::ui::EngineSettingsDialog zeroDialog;
    lizzie::ui::EngineUiSettings zeroSettings;
    zeroSettings.appearance.language = lizzie::ui::LanguageMode::English;
    zeroDialog.setSettings(zeroSettings);
    const lizzie::ui::EngineUiSettings zeroRoundTrip = zeroDialog.settings();
    require(!zeroRoundTrip.realtimeOptions.maxVisits.has_value(), "zero Max Visits should clear the limit");
    require(!zeroRoundTrip.realtimeOptions.maxPlayouts.has_value(), "zero Max Playouts should clear the limit");
    require(!zeroRoundTrip.realtimeOptions.maxTimeSeconds.has_value(), "zero Max Time should clear the limit");
    require(!zeroRoundTrip.realtimeOptions.playoutDoublingAdvantage.has_value(), "zero PDA should disable PDA");
    require(!zeroRoundTrip.realtimeOptions.analysisWideRootNoise.has_value(), "zero WRN should clear the limit");
    int enUnlimitedSpinBoxes = 0;
    for (QSpinBox* spinBox : zeroDialog.findChildren<QSpinBox*>()) {
        if (spinBox->specialValueText() == "Unlimited") {
            ++enUnlimitedSpinBoxes;
        }
    }
    int enUnlimitedDoubleSpinBoxes = 0;
    int enPdaTooltips = 0;
    for (QDoubleSpinBox* spinBox : zeroDialog.findChildren<QDoubleSpinBox*>()) {
        if (spinBox->specialValueText() == "Unlimited") {
            ++enUnlimitedDoubleSpinBoxes;
        }
        if (spinBox->toolTip() == "0 disables PDA" && spinBox->accessibleDescription() == "0 disables PDA") {
            ++enPdaTooltips;
        }
    }
    require(enUnlimitedSpinBoxes == 2, "English settings should mark visit/playout zero limits as unlimited");
    require(enUnlimitedDoubleSpinBoxes == 2, "English settings should mark time/WRN zero limits as unlimited");
    require(enPdaTooltips == 1, "English settings should explain zero PDA without changing its negative range");

    QPushButton* resetShortcutsButton = nullptr;
    for (QPushButton* button : dialog.findChildren<QPushButton*>()) {
        if (button->text() == "重置快捷键" || button->text() == "Reset Shortcuts") {
            resetShortcutsButton = button;
            break;
        }
    }
    require(resetShortcutsButton != nullptr, "settings dialog should expose reset shortcuts button");

    resetShortcutsButton->click();
    const lizzie::ui::EngineUiSettings resetRoundTrip = dialog.settings();
    const lizzie::ui::ShortcutSettings defaultShortcuts;
    require(resetRoundTrip.shortcuts.saveSgfAs == defaultShortcuts.saveSgfAs, "reset should restore save-as shortcut");
    require(resetRoundTrip.shortcuts.analyze == defaultShortcuts.analyze, "reset should restore analyze shortcut");
    require(resetRoundTrip.shortcuts.humanVsAi == defaultShortcuts.humanVsAi, "reset should restore Human vs AI shortcut");
    require(resetRoundTrip.shortcuts.undoMove == defaultShortcuts.undoMove, "reset should restore undo shortcut");
    require(resetRoundTrip.shortcuts.passMove == defaultShortcuts.passMove, "reset should restore pass shortcut");
    require(resetRoundTrip.shortcuts.resignMove == defaultShortcuts.resignMove, "reset should restore resign shortcut");
    require(resetRoundTrip.shortcuts.settings == defaultShortcuts.settings, "reset should restore settings shortcut");
    require(resetRoundTrip.config.executable == "/opt/katago", "reset shortcuts should not alter engine settings");
    require(resetRoundTrip.appearance.theme == lizzie::ui::ThemeMode::Dark, "reset shortcuts should not alter theme");

    lizzie::ui::EngineSettingsDialog pathDialog;
    pathDialog.setSettings(settings);
    int fileSelectorCalls = 0;
    int directorySelectorCalls = 0;
    QStringList fileTitles;
    QStringList directoryTitles;
    QStringList fileInitialPaths;
    QStringList directoryInitialPaths;
    pathDialog.setFileSelector(
        [&](QWidget* parent, const QString& title, const QString& initialPath) -> QString {
            require(parent == &pathDialog, "file selector should receive settings dialog parent");
            ++fileSelectorCalls;
            fileTitles.push_back(title);
            fileInitialPaths.push_back(initialPath);
            if (initialPath == "/models/main.bin.gz") {
                return {};
            }
            return initialPath + ".selected";
        });
    pathDialog.setDirectorySelector(
        [&](QWidget* parent, const QString& title, const QString& initialPath) -> QString {
            require(parent == &pathDialog, "directory selector should receive settings dialog parent");
            ++directorySelectorCalls;
            directoryTitles.push_back(title);
            directoryInitialPaths.push_back(initialPath);
            return initialPath + ".selected";
        });

    const char* filePathKeys[] = {
        "katago",
        "model",
        "gtpConfig",
        "analysisConfig",
        "compareKatago",
        "compareModel",
        "compareGtpConfig",
        "compareAnalysisConfig",
        "blackStoneTexture",
        "whiteStoneTexture",
    };
    const char* directoryPathKeys[] = {
        "workDir",
        "compareWorkDir",
    };
    for (const char* key : filePathKeys) {
        QPushButton* button = findPathBrowseButton(pathDialog, key);
        require(button != nullptr, "settings dialog should expose a file path browse button by row key");
        button->click();
    }
    for (const char* key : directoryPathKeys) {
        QPushButton* button = findPathBrowseButton(pathDialog, key);
        require(button != nullptr, "settings dialog should expose a directory browse button by row key");
        button->click();
    }

    require(fileSelectorCalls == 10, "settings dialog should route every file path button through selector");
    require(directorySelectorCalls == 2, "settings dialog should route every directory path button through selector");
    for (const QString& title : fileTitles) {
        require(title == "选择文件", "file selector should receive localized title");
    }
    for (const QString& title : directoryTitles) {
        require(title == "选择目录", "directory selector should receive localized title");
    }
    require(fileInitialPaths.contains("/opt/katago"), "file selector should receive current executable path");
    require(fileInitialPaths.contains("/models/main.bin.gz"), "file selector should receive current model path");
    require(fileInitialPaths.contains("/configs/main-gtp.cfg"), "file selector should receive current GTP config path");
    require(
        fileInitialPaths.contains("/configs/main-analysis.cfg"),
        "file selector should receive current analysis config path");
    require(
        fileInitialPaths.contains("/configs/compare-analysis.cfg"),
        "file selector should receive current compare analysis config path");
    require(directoryInitialPaths.contains("/work/main"), "directory selector should receive current working directory");

    const lizzie::ui::EngineUiSettings pathRoundTrip = pathDialog.settings();
    require(pathRoundTrip.config.executable == "/opt/katago.selected", "selected executable path should apply");
    require(
        pathRoundTrip.config.model == "/models/main.bin.gz",
        "cancelled file selection should keep previous model path");
    require(pathRoundTrip.config.gtpConfig == "/configs/main-gtp.cfg.selected", "selected GTP config should apply");
    require(
        pathRoundTrip.config.analysisConfig == "/configs/main-analysis.cfg.selected",
        "selected analysis config should apply");
    require(pathRoundTrip.config.workingDirectory == "/work/main.selected", "selected working directory should apply");
    require(
        pathRoundTrip.comparisonConfig.executable == "/opt/katago-compare.selected",
        "selected compare executable should apply");
    require(
        pathRoundTrip.comparisonConfig.model == "/models/compare.bin.gz.selected",
        "selected compare model should apply");
    require(
        pathRoundTrip.comparisonConfig.gtpConfig == "/configs/compare-gtp.cfg.selected",
        "selected compare GTP config should apply");
    require(
        pathRoundTrip.comparisonConfig.analysisConfig == "/configs/compare-analysis.cfg.selected",
        "selected compare analysis config should apply");
    require(
        pathRoundTrip.comparisonConfig.workingDirectory == "/work/compare.selected",
        "selected compare working directory should apply");
    require(
        pathRoundTrip.boardDisplay.blackStoneTexture == "/tmp/black-stone.png.selected",
        "selected black stone texture should apply");
    require(
        pathRoundTrip.boardDisplay.whiteStoneTexture == "/tmp/white-stone.png.selected",
        "selected white stone texture should apply");

    bool foundModelLabel = false;
    bool foundMainEngineSection = false;
    bool foundCompareEngineSection = false;
    bool foundAnalysisSection = false;
    bool foundBoardDisplaySection = false;
    bool foundAppearanceSection = false;
    bool foundFileBehaviorSection = false;
    bool foundCompareAnalysisConfigLabel = false;
    bool foundAnalysisOwnershipLabel = false;
    bool foundShowOwnershipLabel = false;
    bool foundOwnershipDisplayLabel = false;
    bool foundOwnershipOpacityLabel = false;
    bool foundImportLegacySgfAnalysisLabel = false;
    bool foundLoadSidecarLabel = false;
    bool foundBatchSidecarLabel = false;
    bool foundHumanVsAiShortcutLabel = false;
    bool foundShortcutLabel = false;
    bool foundTranslatedThemeChoices = false;
    bool foundTranslatedLanguageChoices = false;
    bool foundTranslatedOwnershipDisplayChoices = false;
    int fileBrowseButtons = 0;
    int directoryBrowseButtons = 0;
    for (QLabel* label : dialog.findChildren<QLabel*>()) {
        foundModelLabel = foundModelLabel || label->text() == "模型";
        foundMainEngineSection = foundMainEngineSection || label->text() == "主引擎";
        foundCompareEngineSection = foundCompareEngineSection || label->text() == "对比引擎";
        foundAnalysisSection = foundAnalysisSection || label->text() == "分析";
        foundBoardDisplaySection = foundBoardDisplaySection || label->text() == "棋盘显示";
        foundAppearanceSection = foundAppearanceSection || label->text() == "外观";
        foundFileBehaviorSection = foundFileBehaviorSection || label->text() == "文件行为";
        foundCompareAnalysisConfigLabel = foundCompareAnalysisConfigLabel || label->text() == "对比分析配置";
        foundAnalysisOwnershipLabel = foundAnalysisOwnershipLabel || label->text() == "分析归属";
        foundShowOwnershipLabel = foundShowOwnershipLabel || label->text() == "显示归属";
        foundOwnershipDisplayLabel = foundOwnershipDisplayLabel || label->text() == "归属显示位置";
        foundOwnershipOpacityLabel = foundOwnershipOpacityLabel || label->text() == "归属透明度";
        foundImportLegacySgfAnalysisLabel = foundImportLegacySgfAnalysisLabel || label->text() == "导入旧 SGF 分析";
        foundLoadSidecarLabel = foundLoadSidecarLabel || label->text() == "加载分析附加文件";
        foundBatchSidecarLabel = foundBatchSidecarLabel || label->text() == "批量输出使用附加文件";
        foundHumanVsAiShortcutLabel = foundHumanVsAiShortcutLabel || label->text() == "人机对局";
        foundShortcutLabel = foundShortcutLabel || label->text() == "快捷键";
    }
    for (QComboBox* combo : dialog.findChildren<QComboBox*>()) {
        QStringList items;
        for (int index = 0; index < combo->count(); ++index) {
            items.push_back(combo->itemText(index));
        }
        foundTranslatedThemeChoices = foundTranslatedThemeChoices ||
            (items.contains("跟随系统") && items.contains("浅色") && items.contains("深色"));
        foundTranslatedLanguageChoices = foundTranslatedLanguageChoices ||
            (items.contains("English") && items.contains("中文"));
        foundTranslatedOwnershipDisplayChoices = foundTranslatedOwnershipDisplayChoices ||
            (items.contains("主棋盘") && items.contains("小棋盘") && items.contains("主棋盘和小棋盘"));
    }
    for (QPushButton* button : dialog.findChildren<QPushButton*>("lizziePathBrowseButton")) {
        if (button->toolTip() == "选择文件" && button->accessibleName() == "选择文件") {
            ++fileBrowseButtons;
        } else if (button->toolTip() == "选择目录" && button->accessibleName() == "选择目录") {
            ++directoryBrowseButtons;
        }
    }
    require(foundModelLabel, "settings dialog should translate model label");
    require(foundMainEngineSection, "settings dialog should group main engine settings");
    require(foundCompareEngineSection, "settings dialog should group compare engine settings");
    require(foundAnalysisSection, "settings dialog should group analysis settings");
    require(foundBoardDisplaySection, "settings dialog should group board display settings");
    require(foundAppearanceSection, "settings dialog should group appearance settings");
    require(foundFileBehaviorSection, "settings dialog should group file behavior settings");
    require(foundCompareAnalysisConfigLabel, "settings dialog should translate compare analysis config label");
    require(foundAnalysisOwnershipLabel, "settings dialog should translate analysis ownership label");
    require(foundShowOwnershipLabel, "settings dialog should translate ownership visibility label");
    require(foundOwnershipDisplayLabel, "settings dialog should translate ownership display label");
    require(foundOwnershipOpacityLabel, "settings dialog should translate ownership opacity label");
    require(foundImportLegacySgfAnalysisLabel, "settings dialog should translate legacy SGF import label");
    require(foundLoadSidecarLabel, "settings dialog should translate sidecar load label");
    require(foundBatchSidecarLabel, "settings dialog should explain batch sidecar output mode");
    require(foundHumanVsAiShortcutLabel, "settings dialog should translate Human vs AI shortcut");
    require(foundShortcutLabel, "settings dialog should translate shortcut section");
    require(foundTranslatedThemeChoices, "settings dialog should translate theme choices");
    require(foundTranslatedLanguageChoices, "settings dialog should translate language choices");
    require(foundTranslatedOwnershipDisplayChoices, "settings dialog should translate ownership display choices");
    require(fileBrowseButtons == 10, "settings dialog should expose translated file browse buttons");
    require(directoryBrowseButtons == 2, "settings dialog should expose translated directory browse buttons");

    std::cout << "settings_dialog_tests passed\n";
    return 0;
}

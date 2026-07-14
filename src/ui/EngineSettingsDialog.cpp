#include "EngineSettingsDialog.h"

#include <QDialogButtonBox>
#include <QByteArray>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <functional>
#include <utility>

namespace lizzie::ui {

namespace {

constexpr const char* kI18nKey = "lizzieSettingsI18nKey";
constexpr const char* kBrowseButtonI18nKey = "lizzieSettingsBrowseButtonI18nKey";
constexpr const char* kPathRowKey = "lizzieSettingsPathRowKey";

int themeIndex(ThemeMode theme)
{
    switch (theme) {
    case ThemeMode::System:
        return 0;
    case ThemeMode::Light:
        return 1;
    case ThemeMode::Dark:
        return 2;
    }
    return 0;
}

ThemeMode themeFromIndex(int index)
{
    switch (index) {
    case 1:
        return ThemeMode::Light;
    case 2:
        return ThemeMode::Dark;
    default:
        return ThemeMode::System;
    }
}

int languageIndex(LanguageMode language)
{
    return language == LanguageMode::Chinese ? 1 : 0;
}

LanguageMode languageFromIndex(int index)
{
    return index == 1 ? LanguageMode::Chinese : LanguageMode::English;
}

int ownershipDisplayModeIndex(OwnershipDisplayMode mode)
{
    switch (mode) {
    case OwnershipDisplayMode::MainBoard:
        return 0;
    case OwnershipDisplayMode::MiniBoard:
        return 1;
    case OwnershipDisplayMode::BothBoards:
        return 2;
    }
    return 0;
}

OwnershipDisplayMode ownershipDisplayModeFromIndex(int index)
{
    switch (index) {
    case 1:
        return OwnershipDisplayMode::MiniBoard;
    case 2:
        return OwnershipDisplayMode::BothBoards;
    default:
        return OwnershipDisplayMode::MainBoard;
    }
}

QString settingsText(LanguageMode language, const char* key)
{
    const bool zh = language == LanguageMode::Chinese;
    const QString name = QString::fromLatin1(key);
    if (name == "settings") {
        return zh ? "设置" : "Settings";
    }
    if (name == "katago") {
        return zh ? "KataGo" : "KataGo";
    }
    if (name == "mainEngine") {
        return zh ? "主引擎" : "Main Engine";
    }
    if (name == "model") {
        return zh ? "模型" : "Model";
    }
    if (name == "gtpConfig") {
        return zh ? "GTP 配置" : "GTP Config";
    }
    if (name == "analysisConfig") {
        return zh ? "分析配置" : "Analysis Config";
    }
    if (name == "workDir") {
        return zh ? "工作目录" : "Work Dir";
    }
    if (name == "libraryPaths") {
        return zh ? "库路径" : "Library Paths";
    }
    if (name == "extraArgs") {
        return zh ? "额外参数" : "Extra Args";
    }
    if (name == "compareKatago") {
        return zh ? "对比 KataGo" : "Compare KataGo";
    }
    if (name == "compareEngine") {
        return zh ? "对比引擎" : "Compare Engine";
    }
    if (name == "compareModel") {
        return zh ? "对比模型" : "Compare Model";
    }
    if (name == "compareGtpConfig") {
        return zh ? "对比 GTP 配置" : "Compare GTP Config";
    }
    if (name == "compareAnalysisConfig") {
        return zh ? "对比分析配置" : "Compare Analysis Config";
    }
    if (name == "compareWorkDir") {
        return zh ? "对比工作目录" : "Compare Work Dir";
    }
    if (name == "compareLibraryPaths") {
        return zh ? "对比库路径" : "Compare Library Paths";
    }
    if (name == "compareExtraArgs") {
        return zh ? "对比额外参数" : "Compare Extra Args";
    }
    if (name == "interval") {
        return zh ? "间隔" : "Interval";
    }
    if (name == "analysis") {
        return zh ? "分析" : "Analysis";
    }
    if (name == "maxVisits") {
        return zh ? "最大访问数" : "Max Visits";
    }
    if (name == "maxPlayouts") {
        return zh ? "最大模拟数" : "Max Playouts";
    }
    if (name == "maxTime") {
        return zh ? "最大时间" : "Max Time";
    }
    if (name == "pda") {
        return "PDA";
    }
    if (name == "wrn") {
        return "WRN";
    }
    if (name == "unlimited") {
        return zh ? "不限制" : "Unlimited";
    }
    if (name == "pdaTip") {
        return zh ? "0 表示关闭 PDA" : "0 disables PDA";
    }
    if (name == "ownership") {
        return zh ? "分析归属" : "Ownership";
    }
    if (name == "showOwnership") {
        return zh ? "显示归属" : "Show Ownership";
    }
    if (name == "ownershipDisplayMode") {
        return zh ? "归属显示位置" : "Ownership Display";
    }
    if (name == "boardDisplay") {
        return zh ? "棋盘显示" : "Board Display";
    }
    if (name == "ownershipOpacity") {
        return zh ? "归属透明度" : "Ownership Opacity";
    }
    if (name == "blackStoneTexture") {
        return zh ? "黑棋贴图" : "Black Stone Texture";
    }
    if (name == "whiteStoneTexture") {
        return zh ? "白棋贴图" : "White Stone Texture";
    }
    if (name == "theme") {
        return zh ? "主题" : "Theme";
    }
    if (name == "appearance") {
        return zh ? "外观" : "Appearance";
    }
    if (name == "language") {
        return zh ? "语言" : "Language";
    }
    if (name == "fontScale") {
        return zh ? "字体缩放" : "Font Scale";
    }
    if (name == "fileBehavior") {
        return zh ? "文件行为" : "File Behavior";
    }
    if (name == "importLegacySgfAnalysis") {
        return zh ? "导入旧 SGF 分析" : "Import Legacy SGF Analysis";
    }
    if (name == "loadAnalysisSidecar") {
        return zh ? "加载分析附加文件" : "Load Analysis Sidecar";
    }
    if (name == "writeSidecarAfterBatch") {
        return zh ? "批量输出使用附加文件" : "Use Sidecar For Batch Output";
    }
    if (name == "shortcuts") {
        return zh ? "快捷键" : "Shortcuts";
    }
    if (name == "newGame") {
        return zh ? "新建棋局" : "New Game";
    }
    if (name == "openSgf") {
        return zh ? "打开 SGF" : "Open SGF";
    }
    if (name == "saveSgf") {
        return zh ? "保存 SGF" : "Save SGF";
    }
    if (name == "saveSgfAs") {
        return zh ? "另存 SGF" : "Save SGF As";
    }
    if (name == "analyze") {
        return zh ? "分析" : "Analyze";
    }
    if (name == "estimate") {
        return zh ? "形势" : "Estimate";
    }
    if (name == "batchAnalyze") {
        return zh ? "批量分析" : "Batch Analyze";
    }
    if (name == "restartEngine") {
        return zh ? "重启引擎" : "Restart Engine";
    }
    if (name == "compareEngines") {
        return zh ? "对比引擎" : "Compare Engines";
    }
    if (name == "aiMove") {
        return zh ? "AI 落子" : "AI Move";
    }
    if (name == "humanVsAi") {
        return zh ? "人机对局" : "Human vs AI";
    }
    if (name == "selfPlay") {
        return zh ? "自对局" : "Self Play";
    }
    if (name == "engineGame") {
        return zh ? "引擎对局" : "Engine Game";
    }
    if (name == "previousMove") {
        return zh ? "后退一手" : "Previous Move";
    }
    if (name == "nextMove") {
        return zh ? "前进一手" : "Next Move";
    }
    if (name == "undoMove") {
        return zh ? "撤销" : "Undo";
    }
    if (name == "pass") {
        return zh ? "停一手" : "Pass";
    }
    if (name == "resign") {
        return zh ? "认输" : "Resign";
    }
    if (name == "resetShortcuts") {
        return zh ? "重置快捷键" : "Reset Shortcuts";
    }
    if (name == "systemTheme") {
        return zh ? "跟随系统" : "System";
    }
    if (name == "lightTheme") {
        return zh ? "浅色" : "Light";
    }
    if (name == "darkTheme") {
        return zh ? "深色" : "Dark";
    }
    if (name == "english") {
        return zh ? "English" : "English";
    }
    if (name == "chinese") {
        return zh ? "中文" : "中文";
    }
    if (name == "ownershipMainBoard") {
        return zh ? "主棋盘" : "Main Board";
    }
    if (name == "ownershipMiniBoard") {
        return zh ? "小棋盘" : "Mini Board";
    }
    if (name == "ownershipBothBoards") {
        return zh ? "主棋盘和小棋盘" : "Main and Mini Boards";
    }
    if (name == "selectFile") {
        return zh ? "选择文件" : "Select File";
    }
    if (name == "selectDirectory") {
        return zh ? "选择目录" : "Select Directory";
    }
    if (name == "ok") {
        return zh ? "确定" : "OK";
    }
    if (name == "cancel") {
        return zh ? "取消" : "Cancel";
    }
    return name;
}

QString defaultFileSelector(QWidget* parent, const QString& title, const QString& initialPath)
{
    return QFileDialog::getOpenFileName(parent, title, initialPath);
}

QString defaultDirectorySelector(QWidget* parent, const QString& title, const QString& initialPath)
{
    return QFileDialog::getExistingDirectory(parent, title, initialPath);
}

void addTranslatedRow(QFormLayout* form, const char* key, QWidget* field)
{
    form->addRow(settingsText(LanguageMode::English, key), field);
    if (auto* label = qobject_cast<QLabel*>(form->labelForField(field))) {
        label->setProperty(kI18nKey, key);
    }
}

QLabel* addSectionRow(QFormLayout* form, const char* key, QWidget* parent)
{
    auto* label = new QLabel(settingsText(LanguageMode::English, key), parent);
    label->setProperty(kI18nKey, key);
    QFont font = label->font();
    font.setBold(true);
    label->setFont(font);
    form->addRow(label);
    return label;
}

QLineEdit* addPathRow(
    QFormLayout* form,
    const char* key,
    QWidget* parent,
    const char* browseKey,
    const std::function<void(QLineEdit*)>& browse)
{
    auto* edit = new QLineEdit(parent);
    auto* browseButton = new QPushButton("...", parent);
    browseButton->setObjectName("lizziePathBrowseButton");
    browseButton->setProperty(kBrowseButtonI18nKey, browseKey);
    browseButton->setProperty(kPathRowKey, key);
    browseButton->setToolTip(settingsText(LanguageMode::English, browseKey));
    browseButton->setAccessibleName(settingsText(LanguageMode::English, browseKey));
    browseButton->setFixedWidth(34);
    auto* row = new QWidget(parent);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(edit);
    layout->addWidget(browseButton);
    addTranslatedRow(form, key, row);
    QObject::connect(browseButton, &QPushButton::clicked, edit, [edit, browse]() {
        browse(edit);
    });
    return edit;
}

QKeySequenceEdit* addShortcutRow(QFormLayout* form, const char* key, QWidget* parent)
{
    auto* edit = new QKeySequenceEdit(parent);
    addTranslatedRow(form, key, edit);
    return edit;
}

}  // namespace

EngineSettingsDialog::EngineSettingsDialog(QWidget* parent)
    : QDialog(parent)
    , fileSelector_(defaultFileSelector)
    , directorySelector_(defaultDirectorySelector)
{
    buildForm();
    retranslate(LanguageMode::English);
}

void EngineSettingsDialog::setFileSelector(PathSelector selector)
{
    fileSelector_ = std::move(selector);
    if (!fileSelector_) {
        fileSelector_ = defaultFileSelector;
    }
}

void EngineSettingsDialog::setDirectorySelector(PathSelector selector)
{
    directorySelector_ = std::move(selector);
    if (!directorySelector_) {
        directorySelector_ = defaultDirectorySelector;
    }
}

void EngineSettingsDialog::setSettings(const EngineUiSettings& settings)
{
    executableEdit_->setText(QString::fromStdString(settings.config.executable.string()));
    modelEdit_->setText(QString::fromStdString(settings.config.model.string()));
    gtpConfigEdit_->setText(QString::fromStdString(settings.config.gtpConfig.string()));
    analysisConfigEdit_->setText(QString::fromStdString(settings.config.analysisConfig.string()));
    workingDirectoryEdit_->setText(QString::fromStdString(settings.config.workingDirectory.string()));
    libraryPathsEdit_->setText(lizzie::app::joinPathList(settings.config.libraryPaths));
    extraArgsEdit_->setText(lizzie::app::joinCommandArguments(settings.config.extraArgs));
    comparisonExecutableEdit_->setText(QString::fromStdString(settings.comparisonConfig.executable.string()));
    comparisonModelEdit_->setText(QString::fromStdString(settings.comparisonConfig.model.string()));
    comparisonGtpConfigEdit_->setText(QString::fromStdString(settings.comparisonConfig.gtpConfig.string()));
    comparisonAnalysisConfigEdit_->setText(QString::fromStdString(settings.comparisonConfig.analysisConfig.string()));
    comparisonWorkingDirectoryEdit_->setText(QString::fromStdString(settings.comparisonConfig.workingDirectory.string()));
    comparisonLibraryPathsEdit_->setText(lizzie::app::joinPathList(settings.comparisonConfig.libraryPaths));
    comparisonExtraArgsEdit_->setText(lizzie::app::joinCommandArguments(settings.comparisonConfig.extraArgs));
    intervalEdit_->setValue(settings.realtimeOptions.intervalCentiseconds);
    ownershipEdit_->setChecked(settings.realtimeOptions.includeOwnership);
    maxVisitsEdit_->setValue(settings.realtimeOptions.maxVisits.value_or(0));
    maxPlayoutsEdit_->setValue(settings.realtimeOptions.maxPlayouts.value_or(0));
    maxTimeEdit_->setValue(settings.realtimeOptions.maxTimeSeconds.value_or(0.0));
    pdaEdit_->setValue(settings.realtimeOptions.playoutDoublingAdvantage.value_or(0.0));
    wrnEdit_->setValue(settings.realtimeOptions.analysisWideRootNoise.value_or(0.0));
    themeEdit_->setCurrentIndex(themeIndex(settings.appearance.theme));
    languageEdit_->setCurrentIndex(languageIndex(settings.appearance.language));
    retranslate(settings.appearance.language);
    fontScaleEdit_->setValue(settings.appearance.fontScale);
    showOwnershipEdit_->setChecked(settings.boardDisplay.showOwnership);
    ownershipDisplayModeEdit_->setCurrentIndex(ownershipDisplayModeIndex(settings.boardDisplay.ownershipDisplayMode));
    ownershipOpacityEdit_->setValue(settings.boardDisplay.ownershipOpacity);
    blackStoneTextureEdit_->setText(QString::fromStdString(settings.boardDisplay.blackStoneTexture.string()));
    whiteStoneTextureEdit_->setText(QString::fromStdString(settings.boardDisplay.whiteStoneTexture.string()));
    importLegacySgfAnalysisEdit_->setChecked(settings.fileBehavior.importLegacySgfAnalysis);
    loadAnalysisSidecarEdit_->setChecked(settings.fileBehavior.loadAnalysisSidecar);
    writeAnalysisSidecarEdit_->setChecked(settings.fileBehavior.writeAnalysisSidecarAfterBatch);
    newShortcutEdit_->setKeySequence(settings.shortcuts.newGame);
    openShortcutEdit_->setKeySequence(settings.shortcuts.openSgf);
    saveShortcutEdit_->setKeySequence(settings.shortcuts.saveSgf);
    saveAsShortcutEdit_->setKeySequence(settings.shortcuts.saveSgfAs);
    analyzeShortcutEdit_->setKeySequence(settings.shortcuts.analyze);
    estimateShortcutEdit_->setKeySequence(settings.shortcuts.estimate);
    batchShortcutEdit_->setKeySequence(settings.shortcuts.batchAnalyze);
    restartShortcutEdit_->setKeySequence(settings.shortcuts.restartEngine);
    compareShortcutEdit_->setKeySequence(settings.shortcuts.compare);
    aiMoveShortcutEdit_->setKeySequence(settings.shortcuts.aiMove);
    humanVsAiShortcutEdit_->setKeySequence(settings.shortcuts.humanVsAi);
    selfPlayShortcutEdit_->setKeySequence(settings.shortcuts.selfPlay);
    engineGameShortcutEdit_->setKeySequence(settings.shortcuts.engineGame);
    previousShortcutEdit_->setKeySequence(settings.shortcuts.previousMove);
    nextShortcutEdit_->setKeySequence(settings.shortcuts.nextMove);
    undoShortcutEdit_->setKeySequence(settings.shortcuts.undoMove);
    passShortcutEdit_->setKeySequence(settings.shortcuts.passMove);
    resignShortcutEdit_->setKeySequence(settings.shortcuts.resignMove);
    settingsShortcutEdit_->setKeySequence(settings.shortcuts.settings);
}

EngineUiSettings EngineSettingsDialog::settings() const
{
    EngineUiSettings result;
    result.config.executable = executableEdit_->text().toStdString();
    result.config.model = modelEdit_->text().toStdString();
    result.config.gtpConfig = gtpConfigEdit_->text().toStdString();
    result.config.analysisConfig = analysisConfigEdit_->text().toStdString();
    result.config.workingDirectory = workingDirectoryEdit_->text().toStdString();
    result.config.libraryPaths = lizzie::app::splitPathList(libraryPathsEdit_->text());
    result.config.extraArgs = lizzie::app::splitCommandArguments(extraArgsEdit_->text());
    result.comparisonConfig.executable = comparisonExecutableEdit_->text().toStdString();
    result.comparisonConfig.model = comparisonModelEdit_->text().toStdString();
    result.comparisonConfig.gtpConfig = comparisonGtpConfigEdit_->text().toStdString();
    result.comparisonConfig.analysisConfig = comparisonAnalysisConfigEdit_->text().toStdString();
    result.comparisonConfig.workingDirectory = comparisonWorkingDirectoryEdit_->text().toStdString();
    result.comparisonConfig.libraryPaths = lizzie::app::splitPathList(comparisonLibraryPathsEdit_->text());
    result.comparisonConfig.extraArgs = lizzie::app::splitCommandArguments(comparisonExtraArgsEdit_->text());
    result.realtimeOptions.intervalCentiseconds = intervalEdit_->value();
    result.realtimeOptions.includeOwnership = ownershipEdit_->isChecked();
    if (maxVisitsEdit_->value() > 0) {
        result.realtimeOptions.maxVisits = maxVisitsEdit_->value();
    }
    if (maxPlayoutsEdit_->value() > 0) {
        result.realtimeOptions.maxPlayouts = maxPlayoutsEdit_->value();
    }
    if (maxTimeEdit_->value() > 0.0) {
        result.realtimeOptions.maxTimeSeconds = maxTimeEdit_->value();
    }
    if (pdaEdit_->value() != 0.0) {
        result.realtimeOptions.playoutDoublingAdvantage = pdaEdit_->value();
    }
    if (wrnEdit_->value() > 0.0) {
        result.realtimeOptions.analysisWideRootNoise = wrnEdit_->value();
    }
    result.appearance.theme = themeFromIndex(themeEdit_->currentIndex());
    result.appearance.language = languageFromIndex(languageEdit_->currentIndex());
    result.appearance.fontScale = fontScaleEdit_->value();
    result.boardDisplay.showOwnership = showOwnershipEdit_->isChecked();
    result.boardDisplay.ownershipDisplayMode = ownershipDisplayModeFromIndex(ownershipDisplayModeEdit_->currentIndex());
    result.boardDisplay.ownershipOpacity = ownershipOpacityEdit_->value();
    result.boardDisplay.blackStoneTexture = blackStoneTextureEdit_->text().toStdString();
    result.boardDisplay.whiteStoneTexture = whiteStoneTextureEdit_->text().toStdString();
    result.fileBehavior.importLegacySgfAnalysis = importLegacySgfAnalysisEdit_->isChecked();
    result.fileBehavior.loadAnalysisSidecar = loadAnalysisSidecarEdit_->isChecked();
    result.fileBehavior.writeAnalysisSidecarAfterBatch = writeAnalysisSidecarEdit_->isChecked();
    result.shortcuts.newGame = newShortcutEdit_->keySequence();
    result.shortcuts.openSgf = openShortcutEdit_->keySequence();
    result.shortcuts.saveSgf = saveShortcutEdit_->keySequence();
    result.shortcuts.saveSgfAs = saveAsShortcutEdit_->keySequence();
    result.shortcuts.analyze = analyzeShortcutEdit_->keySequence();
    result.shortcuts.estimate = estimateShortcutEdit_->keySequence();
    result.shortcuts.batchAnalyze = batchShortcutEdit_->keySequence();
    result.shortcuts.restartEngine = restartShortcutEdit_->keySequence();
    result.shortcuts.compare = compareShortcutEdit_->keySequence();
    result.shortcuts.aiMove = aiMoveShortcutEdit_->keySequence();
    result.shortcuts.humanVsAi = humanVsAiShortcutEdit_->keySequence();
    result.shortcuts.selfPlay = selfPlayShortcutEdit_->keySequence();
    result.shortcuts.engineGame = engineGameShortcutEdit_->keySequence();
    result.shortcuts.previousMove = previousShortcutEdit_->keySequence();
    result.shortcuts.nextMove = nextShortcutEdit_->keySequence();
    result.shortcuts.undoMove = undoShortcutEdit_->keySequence();
    result.shortcuts.passMove = passShortcutEdit_->keySequence();
    result.shortcuts.resignMove = resignShortcutEdit_->keySequence();
    result.shortcuts.settings = settingsShortcutEdit_->keySequence();
    return result;
}

void EngineSettingsDialog::browseFile(QLineEdit* edit)
{
    const QString path = fileSelector_(this, settingsText(activeLanguage_, "selectFile"), edit->text());
    if (!path.isEmpty()) {
        edit->setText(path);
    }
}

void EngineSettingsDialog::browseDirectory(QLineEdit* edit)
{
    const QString path = directorySelector_(this, settingsText(activeLanguage_, "selectDirectory"), edit->text());
    if (!path.isEmpty()) {
        edit->setText(path);
    }
}

void EngineSettingsDialog::retranslate(LanguageMode language)
{
    activeLanguage_ = language;
    setWindowTitle(settingsText(language, "settings"));

    for (QLabel* label : findChildren<QLabel*>()) {
        const QByteArray key = label->property(kI18nKey).toByteArray();
        if (!key.isEmpty()) {
            label->setText(settingsText(language, key.constData()));
        }
    }
    for (QPushButton* button : findChildren<QPushButton*>()) {
        const QByteArray key = button->property(kBrowseButtonI18nKey).toByteArray();
        if (!key.isEmpty()) {
            const QString text = settingsText(language, key.constData());
            button->setToolTip(text);
            button->setAccessibleName(text);
        }
    }

    if (themeEdit_ != nullptr && themeEdit_->count() >= 3) {
        themeEdit_->setItemText(0, settingsText(language, "systemTheme"));
        themeEdit_->setItemText(1, settingsText(language, "lightTheme"));
        themeEdit_->setItemText(2, settingsText(language, "darkTheme"));
    }
    if (languageEdit_ != nullptr && languageEdit_->count() >= 2) {
        languageEdit_->setItemText(0, settingsText(language, "english"));
        languageEdit_->setItemText(1, settingsText(language, "chinese"));
    }
    if (ownershipDisplayModeEdit_ != nullptr && ownershipDisplayModeEdit_->count() >= 3) {
        ownershipDisplayModeEdit_->setItemText(0, settingsText(language, "ownershipMainBoard"));
        ownershipDisplayModeEdit_->setItemText(1, settingsText(language, "ownershipMiniBoard"));
        ownershipDisplayModeEdit_->setItemText(2, settingsText(language, "ownershipBothBoards"));
    }
    const QString unlimited = settingsText(language, "unlimited");
    if (maxVisitsEdit_ != nullptr) {
        maxVisitsEdit_->setSpecialValueText(unlimited);
    }
    if (maxPlayoutsEdit_ != nullptr) {
        maxPlayoutsEdit_->setSpecialValueText(unlimited);
    }
    if (maxTimeEdit_ != nullptr) {
        maxTimeEdit_->setSpecialValueText(unlimited);
    }
    if (pdaEdit_ != nullptr) {
        pdaEdit_->setToolTip(settingsText(language, "pdaTip"));
        pdaEdit_->setAccessibleDescription(settingsText(language, "pdaTip"));
    }
    if (wrnEdit_ != nullptr) {
        wrnEdit_->setSpecialValueText(unlimited);
    }
    if (resetShortcutsButton_ != nullptr) {
        resetShortcutsButton_->setText(settingsText(language, "resetShortcuts"));
    }
    if (buttons_ != nullptr) {
        if (QPushButton* ok = buttons_->button(QDialogButtonBox::Ok)) {
            ok->setText(settingsText(language, "ok"));
        }
        if (QPushButton* cancel = buttons_->button(QDialogButtonBox::Cancel)) {
            cancel->setText(settingsText(language, "cancel"));
        }
    }
}

void EngineSettingsDialog::resetShortcutEdits()
{
    const ShortcutSettings defaults;
    newShortcutEdit_->setKeySequence(defaults.newGame);
    openShortcutEdit_->setKeySequence(defaults.openSgf);
    saveShortcutEdit_->setKeySequence(defaults.saveSgf);
    saveAsShortcutEdit_->setKeySequence(defaults.saveSgfAs);
    analyzeShortcutEdit_->setKeySequence(defaults.analyze);
    estimateShortcutEdit_->setKeySequence(defaults.estimate);
    batchShortcutEdit_->setKeySequence(defaults.batchAnalyze);
    restartShortcutEdit_->setKeySequence(defaults.restartEngine);
    compareShortcutEdit_->setKeySequence(defaults.compare);
    aiMoveShortcutEdit_->setKeySequence(defaults.aiMove);
    humanVsAiShortcutEdit_->setKeySequence(defaults.humanVsAi);
    selfPlayShortcutEdit_->setKeySequence(defaults.selfPlay);
    engineGameShortcutEdit_->setKeySequence(defaults.engineGame);
    previousShortcutEdit_->setKeySequence(defaults.previousMove);
    nextShortcutEdit_->setKeySequence(defaults.nextMove);
    undoShortcutEdit_->setKeySequence(defaults.undoMove);
    passShortcutEdit_->setKeySequence(defaults.passMove);
    resignShortcutEdit_->setKeySequence(defaults.resignMove);
    settingsShortcutEdit_->setKeySequence(defaults.settings);
}

void EngineSettingsDialog::buildForm()
{
    auto* root = new QVBoxLayout(this);
    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto* page = new QWidget(this);
    form_ = new QFormLayout(page);
    scrollArea->setWidget(page);
    root->addWidget(scrollArea, 1);

    addSectionRow(form_, "mainEngine", this);
    executableEdit_ = addPathRow(form_, "katago", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });
    modelEdit_ = addPathRow(form_, "model", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });
    gtpConfigEdit_ = addPathRow(form_, "gtpConfig", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });
    analysisConfigEdit_ = addPathRow(form_, "analysisConfig", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });
    workingDirectoryEdit_ = addPathRow(form_, "workDir", this, "selectDirectory", [this](QLineEdit* edit) {
        browseDirectory(edit);
    });
    libraryPathsEdit_ = new QLineEdit(this);
    addTranslatedRow(form_, "libraryPaths", libraryPathsEdit_);
    extraArgsEdit_ = new QLineEdit(this);
    addTranslatedRow(form_, "extraArgs", extraArgsEdit_);

    addSectionRow(form_, "compareEngine", this);
    comparisonExecutableEdit_ = addPathRow(form_, "compareKatago", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });
    comparisonModelEdit_ = addPathRow(form_, "compareModel", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });
    comparisonGtpConfigEdit_ = addPathRow(form_, "compareGtpConfig", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });
    comparisonAnalysisConfigEdit_ = addPathRow(form_, "compareAnalysisConfig", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });
    comparisonWorkingDirectoryEdit_ = addPathRow(form_, "compareWorkDir", this, "selectDirectory", [this](QLineEdit* edit) {
        browseDirectory(edit);
    });
    comparisonLibraryPathsEdit_ = new QLineEdit(this);
    addTranslatedRow(form_, "compareLibraryPaths", comparisonLibraryPathsEdit_);
    comparisonExtraArgsEdit_ = new QLineEdit(this);
    addTranslatedRow(form_, "compareExtraArgs", comparisonExtraArgsEdit_);

    addSectionRow(form_, "analysis", this);
    intervalEdit_ = new QSpinBox(this);
    intervalEdit_->setRange(1, 10000);
    intervalEdit_->setSuffix(" cs");
    addTranslatedRow(form_, "interval", intervalEdit_);

    maxVisitsEdit_ = new QSpinBox(this);
    maxVisitsEdit_->setRange(0, 100000000);
    addTranslatedRow(form_, "maxVisits", maxVisitsEdit_);

    maxPlayoutsEdit_ = new QSpinBox(this);
    maxPlayoutsEdit_->setRange(0, 100000000);
    addTranslatedRow(form_, "maxPlayouts", maxPlayoutsEdit_);

    maxTimeEdit_ = new QDoubleSpinBox(this);
    maxTimeEdit_->setRange(0.0, 86400.0);
    maxTimeEdit_->setDecimals(2);
    maxTimeEdit_->setSuffix(" s");
    addTranslatedRow(form_, "maxTime", maxTimeEdit_);

    pdaEdit_ = new QDoubleSpinBox(this);
    pdaEdit_->setRange(-10.0, 10.0);
    pdaEdit_->setDecimals(3);
    pdaEdit_->setSingleStep(0.05);
    addTranslatedRow(form_, "pda", pdaEdit_);

    wrnEdit_ = new QDoubleSpinBox(this);
    wrnEdit_->setRange(0.0, 10.0);
    wrnEdit_->setDecimals(3);
    wrnEdit_->setSingleStep(0.01);
    addTranslatedRow(form_, "wrn", wrnEdit_);

    ownershipEdit_ = new QCheckBox(this);
    addTranslatedRow(form_, "ownership", ownershipEdit_);

    addSectionRow(form_, "boardDisplay", this);
    showOwnershipEdit_ = new QCheckBox(this);
    addTranslatedRow(form_, "showOwnership", showOwnershipEdit_);

    ownershipDisplayModeEdit_ = new QComboBox(this);
    ownershipDisplayModeEdit_->addItems({"Main Board", "Mini Board", "Main and Mini Boards"});
    addTranslatedRow(form_, "ownershipDisplayMode", ownershipDisplayModeEdit_);

    ownershipOpacityEdit_ = new QDoubleSpinBox(this);
    ownershipOpacityEdit_->setRange(0.05, 1.0);
    ownershipOpacityEdit_->setSingleStep(0.05);
    ownershipOpacityEdit_->setDecimals(2);
    addTranslatedRow(form_, "ownershipOpacity", ownershipOpacityEdit_);

    blackStoneTextureEdit_ = addPathRow(form_, "blackStoneTexture", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });
    whiteStoneTextureEdit_ = addPathRow(form_, "whiteStoneTexture", this, "selectFile", [this](QLineEdit* edit) {
        browseFile(edit);
    });

    addSectionRow(form_, "appearance", this);
    themeEdit_ = new QComboBox(this);
    themeEdit_->addItems({"System", "Light", "Dark"});
    addTranslatedRow(form_, "theme", themeEdit_);

    languageEdit_ = new QComboBox(this);
    languageEdit_->addItems({"English", "中文"});
    addTranslatedRow(form_, "language", languageEdit_);
    connect(languageEdit_, &QComboBox::currentIndexChanged, this, [this](int index) {
        retranslate(languageFromIndex(index));
    });

    fontScaleEdit_ = new QDoubleSpinBox(this);
    fontScaleEdit_->setRange(0.75, 2.0);
    fontScaleEdit_->setSingleStep(0.05);
    fontScaleEdit_->setDecimals(2);
    addTranslatedRow(form_, "fontScale", fontScaleEdit_);

    addSectionRow(form_, "fileBehavior", this);
    importLegacySgfAnalysisEdit_ = new QCheckBox(this);
    addTranslatedRow(form_, "importLegacySgfAnalysis", importLegacySgfAnalysisEdit_);
    loadAnalysisSidecarEdit_ = new QCheckBox(this);
    addTranslatedRow(form_, "loadAnalysisSidecar", loadAnalysisSidecarEdit_);
    writeAnalysisSidecarEdit_ = new QCheckBox(this);
    addTranslatedRow(form_, "writeSidecarAfterBatch", writeAnalysisSidecarEdit_);

    addSectionRow(form_, "shortcuts", this);
    newShortcutEdit_ = addShortcutRow(form_, "newGame", this);
    openShortcutEdit_ = addShortcutRow(form_, "openSgf", this);
    saveShortcutEdit_ = addShortcutRow(form_, "saveSgf", this);
    saveAsShortcutEdit_ = addShortcutRow(form_, "saveSgfAs", this);
    analyzeShortcutEdit_ = addShortcutRow(form_, "analyze", this);
    estimateShortcutEdit_ = addShortcutRow(form_, "estimate", this);
    batchShortcutEdit_ = addShortcutRow(form_, "batchAnalyze", this);
    restartShortcutEdit_ = addShortcutRow(form_, "restartEngine", this);
    compareShortcutEdit_ = addShortcutRow(form_, "compareEngines", this);
    aiMoveShortcutEdit_ = addShortcutRow(form_, "aiMove", this);
    humanVsAiShortcutEdit_ = addShortcutRow(form_, "humanVsAi", this);
    selfPlayShortcutEdit_ = addShortcutRow(form_, "selfPlay", this);
    engineGameShortcutEdit_ = addShortcutRow(form_, "engineGame", this);
    previousShortcutEdit_ = addShortcutRow(form_, "previousMove", this);
    nextShortcutEdit_ = addShortcutRow(form_, "nextMove", this);
    undoShortcutEdit_ = addShortcutRow(form_, "undoMove", this);
    passShortcutEdit_ = addShortcutRow(form_, "pass", this);
    resignShortcutEdit_ = addShortcutRow(form_, "resign", this);
    settingsShortcutEdit_ = addShortcutRow(form_, "settings", this);
    resetShortcutsButton_ = new QPushButton(this);
    form_->addRow(resetShortcutsButton_);
    connect(resetShortcutsButton_, &QPushButton::clicked, this, &EngineSettingsDialog::resetShortcutEdits);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    root->addWidget(buttons_);
    connect(buttons_, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons_, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

}  // namespace lizzie::ui

#pragma once

#include "AppSettings.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDoubleSpinBox>
#include <QKeySequenceEdit>
#include <QLineEdit>
#include <QSpinBox>
#include <QString>

#include <functional>

class QDialogButtonBox;
class QFormLayout;
class QPushButton;

namespace lizzie::ui {

using lizzie::app::AppearanceSettings;
using lizzie::app::BoardDisplaySettings;
using lizzie::app::EngineUiSettings;
using lizzie::app::FileBehaviorSettings;
using lizzie::app::LanguageMode;
using lizzie::app::OwnershipDisplayMode;
using lizzie::app::ShortcutSettings;
using lizzie::app::ThemeMode;

class EngineSettingsDialog : public QDialog {
    Q_OBJECT

public:
    using PathSelector = std::function<QString(QWidget* parent, const QString& title, const QString& initialPath)>;

    explicit EngineSettingsDialog(QWidget* parent = nullptr);

    void setSettings(const EngineUiSettings& settings);
    [[nodiscard]] EngineUiSettings settings() const;
    void setFileSelector(PathSelector selector);
    void setDirectorySelector(PathSelector selector);

private:
    void browseFile(QLineEdit* edit);
    void browseDirectory(QLineEdit* edit);
    void buildForm();
    void retranslate(LanguageMode language);
    void resetShortcutEdits();

    QFormLayout* form_ = nullptr;
    QLineEdit* executableEdit_ = nullptr;
    QLineEdit* modelEdit_ = nullptr;
    QLineEdit* gtpConfigEdit_ = nullptr;
    QLineEdit* analysisConfigEdit_ = nullptr;
    QLineEdit* workingDirectoryEdit_ = nullptr;
    QLineEdit* libraryPathsEdit_ = nullptr;
    QLineEdit* extraArgsEdit_ = nullptr;
    QLineEdit* comparisonExecutableEdit_ = nullptr;
    QLineEdit* comparisonModelEdit_ = nullptr;
    QLineEdit* comparisonGtpConfigEdit_ = nullptr;
    QLineEdit* comparisonAnalysisConfigEdit_ = nullptr;
    QLineEdit* comparisonWorkingDirectoryEdit_ = nullptr;
    QLineEdit* comparisonLibraryPathsEdit_ = nullptr;
    QLineEdit* comparisonExtraArgsEdit_ = nullptr;
    QSpinBox* intervalEdit_ = nullptr;
    QSpinBox* maxVisitsEdit_ = nullptr;
    QSpinBox* maxPlayoutsEdit_ = nullptr;
    QDoubleSpinBox* maxTimeEdit_ = nullptr;
    QDoubleSpinBox* pdaEdit_ = nullptr;
    QDoubleSpinBox* wrnEdit_ = nullptr;
    QCheckBox* ownershipEdit_ = nullptr;
    QCheckBox* showOwnershipEdit_ = nullptr;
    QComboBox* ownershipDisplayModeEdit_ = nullptr;
    QLineEdit* blackStoneTextureEdit_ = nullptr;
    QLineEdit* whiteStoneTextureEdit_ = nullptr;
    QCheckBox* importLegacySgfAnalysisEdit_ = nullptr;
    QCheckBox* loadAnalysisSidecarEdit_ = nullptr;
    QCheckBox* writeAnalysisSidecarEdit_ = nullptr;
    QDoubleSpinBox* ownershipOpacityEdit_ = nullptr;
    QComboBox* themeEdit_ = nullptr;
    QComboBox* languageEdit_ = nullptr;
    QDoubleSpinBox* fontScaleEdit_ = nullptr;
    QKeySequenceEdit* newShortcutEdit_ = nullptr;
    QKeySequenceEdit* openShortcutEdit_ = nullptr;
    QKeySequenceEdit* saveShortcutEdit_ = nullptr;
    QKeySequenceEdit* saveAsShortcutEdit_ = nullptr;
    QKeySequenceEdit* analyzeShortcutEdit_ = nullptr;
    QKeySequenceEdit* estimateShortcutEdit_ = nullptr;
    QKeySequenceEdit* batchShortcutEdit_ = nullptr;
    QKeySequenceEdit* restartShortcutEdit_ = nullptr;
    QKeySequenceEdit* compareShortcutEdit_ = nullptr;
    QKeySequenceEdit* aiMoveShortcutEdit_ = nullptr;
    QKeySequenceEdit* humanVsAiShortcutEdit_ = nullptr;
    QKeySequenceEdit* selfPlayShortcutEdit_ = nullptr;
    QKeySequenceEdit* engineGameShortcutEdit_ = nullptr;
    QKeySequenceEdit* previousShortcutEdit_ = nullptr;
    QKeySequenceEdit* nextShortcutEdit_ = nullptr;
    QKeySequenceEdit* undoShortcutEdit_ = nullptr;
    QKeySequenceEdit* passShortcutEdit_ = nullptr;
    QKeySequenceEdit* resignShortcutEdit_ = nullptr;
    QKeySequenceEdit* settingsShortcutEdit_ = nullptr;
    QPushButton* resetShortcutsButton_ = nullptr;
    QDialogButtonBox* buttons_ = nullptr;
    LanguageMode activeLanguage_ = LanguageMode::English;
    PathSelector fileSelector_;
    PathSelector directorySelector_;
};

}  // namespace lizzie::ui

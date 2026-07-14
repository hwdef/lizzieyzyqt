#pragma once

#include "AnalysisGraphWidget.h"
#include "AnalysisStreamGate.h"
#include "AnalysisStore.h"
#include "AppSettings.h"
#include "BatchAnalysisRun.h"
#include "BatchAnalysisTracker.h"
#include "EngineManager.h"
#include "GameDocumentSession.h"
#include "GameEditor.h"
#include "GameModel.h"
#include "GtpConsoleWidget.h"
#include "PositionRequestGuard.h"
#include "RealtimeAnalysis.h"
#include "ThreadedAnalysisProcess.h"
#include "ThreadedKataGoProcess.h"

#include <QAction>
#include <QCloseEvent>
#include <QLabel>
#include <QMainWindow>
#include <QPointer>
#include <QProgressDialog>
#include <QTableWidget>
#include <QTreeWidget>
#include <QPlainTextEdit>

#include <functional>
#include <optional>
#include <string>
#include <vector>

class QQuickWidget;
class QToolBar;
class QDockWidget;
class QMenu;

namespace lizzie::ui {

class BoardQuickItem;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    using SgfPathSelector = std::function<QString(QWidget*, const QString&, const QString&)>;
    using ConfigPathSelector = std::function<QString(QWidget*, const QString&, const QString&, const QString&)>;
    using FirstRunPathSelector = std::function<QString(QWidget*, const QString&, const QString&)>;

    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    void setSgfPathSelectors(SgfPathSelector openSelector, SgfPathSelector saveSelector);
    void setJavaConfigPathSelector(ConfigPathSelector selector);
    void setFirstRunPathSelector(FirstRunPathSelector selector);

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void newGame();
    void openSgf();
    void saveSgf();
    void saveSgfAs();
    void previousMove();
    void nextMove();
    void undoMove();
    void passMove();
    void resignMove();
    void playMoveAt(int x, int y);
    void showSettings();
    void editGameInfo();
    void promoteCurrentVariation();
    void deleteCurrentBranch();
    void startOrStopAnalysis();
    void startEstimate();
    void startBatchAnalysis();
    void restartEngine();
    void startOrStopCompare();
    void requestAiMove();
    void toggleHumanVsAi();
    void toggleSelfPlay();
    void toggleEngineGame();
    void handleCommandResponse(const lizzie::engine::QueuedGtpResponse& response);
    void handleCompareCommandResponse(const lizzie::engine::QueuedGtpResponse& response);
    void handleAnalyzeLine(const lizzie::engine::KataAnalyzeLine& line);
    void handleCompareAnalyzeLine(const lizzie::engine::KataAnalyzeLine& line);
    void handleBatchResponse(const lizzie::engine::AnalysisResponse& response);
    void handleBatchRequestFailed(const QString& id, const QString& message, const QString& rawLine);
    void handleBatchProgress(int completed, int total);
    void handleBatchFinished(bool cancelled);
    void sendConsoleCommand(const QString& command);
    bool importLegacyJavaConfig(const QString& path);
    void selectTreeNode();
    void updateCurrentComment();

private:
    using AnalysisRunMode = lizzie::app::BatchAnalysisRunMode;

    enum class BoardEditMode {
        Play,
        Try,
        Label,
        Circle,
        Square,
        Triangle,
        Cross,
        Clear,
        SetupBlack,
        SetupWhite,
        SetupClear,
    };

    void buildToolbar();
    void buildDocks();
    void buildMenus();
    void connectEngine();
    void loadSettings();
    void saveSettings();
    bool loadSgfFromPath(
        const QString& path,
        std::optional<lizzie::core::NodeId> restoreNode = std::nullopt,
        bool interactiveErrors = true,
        std::optional<int> restoreCollectionGameIndex = std::nullopt);
    bool saveSgfAsInteractive();
    bool restoreLastSession();
    bool editSettings();
    void applyEngineSettingsUpdate(lizzie::app::EngineUiSettings nextSettings);
    bool editFirstRunEngineSettings();
    bool importLegacyJavaConfigFromDialog();
    void maybeShowFirstRunWizard();
    void applyAppearance();
    void applyBoardDisplaySettings();
    void applyShortcuts();
    void applyActionTooltips();
    void retranslateUi();
    [[nodiscard]] QString uiText(const char* key) const;
    void updateBoard();
    void updateCandidates();
    void showCandidatePreview(int row);
    void refreshCandidatePreview();
    void updateCompareTable();
    void updateGraph();
    void updateGameTree();
    void appendGameTreeNode(QTreeWidgetItem* parent, lizzie::core::NodeId nodeId);
    void updateCommentEditor();
    void appendEngineLogLine(const QString& line, const QString& source);
    void reportSystemDiagnostic(
        const QString& message,
        const QString& source,
        const QString& consolePrefix = QString(),
        int timeoutMs = 5000);
    void setBoardEditMode(BoardEditMode mode);
    void editMarkupAt(lizzie::core::Point point);
    void syncAndStartRealtimeAnalysis();
    void failRealtimeAnalysisStartup(const QString& message);
    void stopRealtimeAnalysis();
    void syncAndStartCompareAnalysis();
    void failCompareAnalysisStartup(const QString& message);
    void stopCompareAnalysis();
    void syncAndRequestGenMove();
    [[nodiscard]] QString mainGenMoveSkippedStatus(bool boardSync) const;
    void failMainGenMoveRequest(const QString& message);
    void syncAndRequestCompareGenMove();
    void failCompareGenMoveRequest(const QString& message);
    void requestEngineGameMove();
    void cancelBatchAnalysisForGameChange();
    void stopAutomatedPlayModes();
    void stopHumanVsAi();
    void stopSelfPlay();
    void stopEngineGame();
    bool applyEngineMove(const lizzie::core::Move& move, const QString& source, const QString& consolePrefix);
    void markAnalysisSidecarSyncPending();
    void markOutstandingBatchNodesFailed(const QString& message);
    [[nodiscard]] std::vector<AnalysisGraphPoint> graphPoints() const;
    int loadAnalysisSidecar(const QString& sgfPath);
    bool writeAnalysisSidecar(
        const QString& path,
        std::optional<QString> sgfPathOverride = std::nullopt,
        QString* errorMessage = nullptr) const;
    bool saveSgfTo(const QString& path, bool interactiveError = true, bool syncAppliedSidecar = true);
    [[nodiscard]] bool realtimeAnalysisRequested() const;
    [[nodiscard]] bool compareAnalysisRequested() const;

    lizzie::core::GameModel game_;
    lizzie::app::AnalysisStore analysisStore_;
    lizzie::app::GameEditor gameEditor_;
    lizzie::app::GameDocumentSession documentSession_;
    bool appliedAnalysisSidecarForDocument_ = false;
    bool analysisSidecarSyncPending_ = false;
    SgfPathSelector openSgfPathSelector_;
    SgfPathSelector saveSgfPathSelector_;
    ConfigPathSelector javaConfigPathSelector_;
    FirstRunPathSelector firstRunPathSelector_;
    lizzie::app::EngineUiSettings engineSettings_;
    lizzie::app::EngineManager engines_;
    bool mainCapabilitiesKnown_ = false;
    bool compareCapabilitiesKnown_ = false;
    lizzie::engine::RealtimeAnalysisAccumulator realtimeAccumulator_;
    lizzie::engine::RealtimeAnalysisAccumulator compareAccumulator_;
    bool realtimeAnalysisActive_ = false;
    bool pendingAnalysisStart_ = false;
    bool compareAnalysisActive_ = false;
    bool pendingCompareStart_ = false;
    std::optional<lizzie::engine::GtpCommand> pendingRealtimeAnalyzeCommand_;
    std::optional<lizzie::engine::GtpCommand> pendingCompareAnalyzeCommand_;
    bool pendingGenMoveStart_ = false;
    bool pendingCompareGenMoveStart_ = false;
    PositionRequestGuard genMoveGuard_;
    PositionRequestGuard compareGenMoveGuard_;
    bool humanVsAiActive_ = false;
    bool selfPlayActive_ = false;
    bool engineGameActive_ = false;
    bool batchRunFailed_ = false;
    bool updatingUi_ = false;
    BoardEditMode boardEditMode_ = BoardEditMode::Play;
    AnalysisRunMode analysisRunMode_ = AnalysisRunMode::None;
    AnalysisStreamGate realtimeStreamGate_;
    AnalysisStreamGate compareStreamGate_;
    std::optional<lizzie::core::AnalysisSnapshot> compareAnalysis_;
    lizzie::core::NodeId compareAnalysisNode_ = 0;
    std::optional<lizzie::core::Move> previewCandidateMove_;
    lizzie::app::BatchAnalysisTracker batchTracker_;

    BoardQuickItem* boardItem_ = nullptr;
    BoardQuickItem* ownershipBoardItem_ = nullptr;
    QToolBar* mainToolbar_ = nullptr;
    QDockWidget* candidatesDock_ = nullptr;
    QDockWidget* ownershipDock_ = nullptr;
    QDockWidget* treeDock_ = nullptr;
    QDockWidget* commentDock_ = nullptr;
    QDockWidget* graphDock_ = nullptr;
    QDockWidget* consoleDock_ = nullptr;
    QDockWidget* engineLogDock_ = nullptr;
    QDockWidget* compareDock_ = nullptr;
    QMenu* fileMenu_ = nullptr;
    QMenu* engineMenu_ = nullptr;
    QMenu* navigateMenu_ = nullptr;
    QMenu* markupMenu_ = nullptr;
    QMenu* viewMenu_ = nullptr;
    QAction* newAction_ = nullptr;
    QAction* openAction_ = nullptr;
    QAction* saveAction_ = nullptr;
    QAction* saveAsAction_ = nullptr;
    QAction* gameInfoAction_ = nullptr;
    QAction* importJavaConfigAction_ = nullptr;
    QAction* analyzeAction_ = nullptr;
    QAction* estimateAction_ = nullptr;
    QAction* batchAction_ = nullptr;
    QAction* restartEngineAction_ = nullptr;
    QAction* compareAction_ = nullptr;
    QAction* aiMoveAction_ = nullptr;
    QAction* humanVsAiAction_ = nullptr;
    QAction* selfPlayAction_ = nullptr;
    QAction* engineGameAction_ = nullptr;
    QAction* previousAction_ = nullptr;
    QAction* nextAction_ = nullptr;
    QAction* undoAction_ = nullptr;
    QAction* passAction_ = nullptr;
    QAction* resignAction_ = nullptr;
    QAction* promoteVariationAction_ = nullptr;
    QAction* deleteBranchAction_ = nullptr;
    QAction* playModeAction_ = nullptr;
    QAction* tryModeAction_ = nullptr;
    QAction* labelModeAction_ = nullptr;
    QAction* circleMarkAction_ = nullptr;
    QAction* squareMarkAction_ = nullptr;
    QAction* triangleMarkAction_ = nullptr;
    QAction* crossMarkAction_ = nullptr;
    QAction* clearMarkupAction_ = nullptr;
    QAction* setupBlackAction_ = nullptr;
    QAction* setupWhiteAction_ = nullptr;
    QAction* setupClearAction_ = nullptr;
    QAction* settingsAction_ = nullptr;
    QPointer<QProgressDialog> batchProgress_;
    QPointer<QTableWidget> candidatesTable_;
    QPointer<QTreeWidget> gameTreeWidget_;
    QPointer<QPlainTextEdit> commentEditor_;
    QPointer<AnalysisGraphWidget> graphWidget_;
    QPointer<GtpConsoleWidget> consoleWidget_;
    QPointer<QPlainTextEdit> engineLogOutput_;
    QPointer<QTableWidget> compareTable_;
};

}  // namespace lizzie::ui

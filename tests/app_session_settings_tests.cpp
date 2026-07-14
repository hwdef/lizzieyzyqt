#include "FirstRun.h"
#include "SessionSettings.h"
#include "WindowPlacement.h"

#include <QByteArray>
#include <QGuiApplication>
#include <QScreen>
#include <QSettings>
#include <QTemporaryDir>

#include <cstdlib>
#include <iostream>
#include <optional>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

QSettings makeSettings(const QTemporaryDir& dir)
{
    return QSettings(dir.filePath("settings.ini"), QSettings::IniFormat);
}

void testFirstRunStartupPlanning()
{
    lizzie::app::EngineUiSettings settings;

    require(
        lizzie::app::planFirstRunStartup(true, settings) == lizzie::app::FirstRunStartupAction::Skip,
        "completed first-run should skip the wizard");

    require(
        lizzie::app::planFirstRunStartup(false, settings) == lizzie::app::FirstRunStartupAction::ShowWizard,
        "missing engine paths should show the first-run wizard");

    settings.config.executable = "/opt/katago";
    settings.config.model = "/models/model.bin.gz";
    settings.config.gtpConfig = "/configs/gtp.cfg";
    require(
        lizzie::app::planFirstRunStartup(false, settings) == lizzie::app::FirstRunStartupAction::ShowWizard,
        "stored GTP-only engine settings should still show the first-run wizard");

    settings.config.analysisConfig = "/configs/analysis.cfg";
    require(
        lizzie::app::planFirstRunStartup(false, settings) == lizzie::app::FirstRunStartupAction::MarkComplete,
        "stored GTP and analysis engine settings should complete first-run without showing the wizard");

    settings.config.gtpConfig.clear();
    require(
        lizzie::app::planFirstRunStartup(false, settings) == lizzie::app::FirstRunStartupAction::ShowWizard,
        "analysis-only engine settings should not skip first-run realtime configuration");
}

void testWindowPlacementPlanning()
{
    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    require(primaryScreen != nullptr, "offscreen test should provide a primary screen");

    const QRect available = primaryScreen->availableGeometry();
    const QRect visibleFrame = available.adjusted(10, 10, -10, -10);
    require(
        lizzie::app::windowFrameIsSubstantiallyVisible(visibleFrame),
        "visible frame should be considered substantially visible");
    require(
        lizzie::app::requiredVisibleAreaForWindowFrame(QRect(0, 0, 1000, 900)) == 320 * 240,
        "large restored windows should cap the required visible area");
    require(
        lizzie::app::requiredVisibleAreaForWindowFrame(QRect(0, 0, 20, 30)) == 20 * 30,
        "small restored windows should use their own area as the visibility threshold");
    require(
        !lizzie::app::adjustedWindowGeometryForAvailableScreens(visibleFrame, visibleFrame.size()).has_value(),
        "visible frame should not need window placement adjustment");

    const QRect barelyVisibleFrame(available.right() - 8, available.bottom() - 8, 960, 720);
    require(
        !lizzie::app::windowFrameIsSubstantiallyVisible(barelyVisibleFrame),
        "barely visible frame should not satisfy the minimum visible area");
    require(
        lizzie::app::adjustedWindowGeometryForAvailableScreens(barelyVisibleFrame, barelyVisibleFrame.size())
            .has_value(),
        "barely visible frame should be adjusted onto the primary screen");

    const QRect offscreenFrame(available.right() + 10000, available.bottom() + 10000, 200, 120);
    require(
        !lizzie::app::windowFrameIsSubstantiallyVisible(offscreenFrame),
        "offscreen frame should not be substantially visible");
    const std::optional<QRect> adjusted =
        lizzie::app::adjustedWindowGeometryForAvailableScreens(offscreenFrame, offscreenFrame.size());
    require(adjusted.has_value(), "offscreen frame should be adjusted onto the primary screen");
    require(
        available.intersects(*adjusted),
        "adjusted window geometry should intersect the available primary screen");
    require(
        adjusted->size() == QSize(640, 480).boundedTo(available.size()),
        "adjusted window geometry should use the minimum usable size bounded by the primary screen");

    const QSize largeWindowSize(available.width() + 1000, available.height() + 1000);
    const std::optional<QRect> adjustedLarge =
        lizzie::app::adjustedWindowGeometryForAvailableScreens(offscreenFrame, largeWindowSize);
    require(adjustedLarge.has_value(), "large offscreen frame should still be adjusted");
    require(
        adjustedLarge->size() == largeWindowSize.boundedTo(available.size()),
        "large adjusted window geometry should be bounded by the primary screen");
}

}  // namespace

int main(int argc, char* argv[])
{
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }
    QGuiApplication app(argc, argv);

    try {
        QTemporaryDir dir;
        require(dir.isValid(), "temporary settings directory should be valid");

        QSettings settings = makeSettings(dir);
        lizzie::app::SessionSettings emptySession = lizzie::app::loadSessionSettings(settings);
        require(emptySession.lastFile.isEmpty(), "empty session should have no last file");
        require(!emptySession.currentNodeId.has_value(), "empty session should have no current node");
        require(!emptySession.collectionGameIndex.has_value(), "empty session should have no collection game index");
        require(!lizzie::app::firstRunComplete(settings), "first-run should default to incomplete");

        lizzie::app::saveSessionSettings(settings, std::optional<QString>{QStringLiteral("/tmp/game.sgf")}, 42, 1);
        lizzie::app::setFirstRunComplete(settings, true);
        settings.sync();

        QSettings reloaded = makeSettings(dir);
        const lizzie::app::SessionSettings savedSession = lizzie::app::loadSessionSettings(reloaded);
        require(savedSession.lastFile == "/tmp/game.sgf", "session should persist last file");
        require(
            savedSession.currentNodeId.has_value() && *savedSession.currentNodeId == 42,
            "session should persist current node");
        require(
            savedSession.collectionGameIndex.has_value() && *savedSession.collectionGameIndex == 1,
            "session should persist collection game index");
        require(lizzie::app::firstRunComplete(reloaded), "first-run completion should persist");

        reloaded.setValue("startup/firstRunComplete", "true");
        require(lizzie::app::firstRunComplete(reloaded), "string true first-run completion should load");
        reloaded.setValue("startup/firstRunComplete", "1");
        require(lizzie::app::firstRunComplete(reloaded), "string one first-run completion should load");
        reloaded.setValue("startup/firstRunComplete", "false");
        require(!lizzie::app::firstRunComplete(reloaded), "string false first-run completion should load");
        reloaded.setValue("startup/firstRunComplete", "not-a-bool");
        require(!lizzie::app::firstRunComplete(reloaded), "malformed first-run completion should fall back to false");

        reloaded.setValue("session/currentNodeId", "not-a-node-id");
        reloaded.setValue("session/collectionGameIndex", -1);
        const lizzie::app::SessionSettings malformedSession = lizzie::app::loadSessionSettings(reloaded);
        require(!malformedSession.currentNodeId.has_value(), "malformed current node should be ignored");
        require(!malformedSession.collectionGameIndex.has_value(), "malformed collection game index should be ignored");

        lizzie::app::saveSessionSettings(reloaded, std::optional<QString>{QStringLiteral("/tmp/game.sgf")}, 7);
        reloaded.sync();
        QSettings singleGameReload = makeSettings(dir);
        const lizzie::app::SessionSettings singleGameSession =
            lizzie::app::loadSessionSettings(singleGameReload);
        require(
            !singleGameSession.collectionGameIndex.has_value(),
            "single-game session save should remove stale collection game index");

        lizzie::app::saveSessionSettings(reloaded, std::nullopt, 99);
        lizzie::app::setFirstRunComplete(reloaded, false);
        reloaded.sync();

        QSettings cleared = makeSettings(dir);
        const lizzie::app::SessionSettings clearedSession = lizzie::app::loadSessionSettings(cleared);
        require(clearedSession.lastFile.isEmpty(), "cleared session should remove last file");
        require(!clearedSession.currentNodeId.has_value(), "cleared session should remove current node");
        require(!clearedSession.collectionGameIndex.has_value(), "cleared session should remove collection game index");
        require(!lizzie::app::firstRunComplete(cleared), "first-run completion should be clearable");

        testFirstRunStartupPlanning();
        testWindowPlacementPlanning();
    } catch (const std::exception& error) {
        std::cerr << "app_session_settings_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "app_session_settings_tests passed\n";
    return EXIT_SUCCESS;
}

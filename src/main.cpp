#include <QAction>
#include <QApplication>
#include <QByteArray>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFont>
#include <QFontMetricsF>
#include <QGuiApplication>
#include <QHeaderView>
#include <QImage>
#include <QImageReader>
#include <QKeySequence>
#include <QLibraryInfo>
#include <QLocale>
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QMenu>
#include <QMenuBar>
#include <QMetaType>
#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QPalette>
#include <QPushButton>
#include <QQmlEngine>
#include <QQuickWidget>
#include <QQuickWindow>
#include <QRect>
#include <QScreen>
#include <QSGRendererInterface>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QStyle>
#include <QStyleHints>
#include <QSurfaceFormat>
#include <QSysInfo>
#include <QTableWidget>
#include <QToolBar>
#include <QTreeWidget>
#include <QVariant>
#include <QWidget>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "AppSettings.h"
#include "EngineSettingsDialog.h"
#include "MainWindow.h"
#include "WindowPlacement.h"

#ifndef LIZZIEYZY_QT_VERSION
#define LIZZIEYZY_QT_VERSION "unknown"
#endif

namespace {

bool argumentEquals(char* argument, std::string_view expected)
{
    return argument != nullptr && std::string_view(argument) == expected;
}

bool argumentStartsWith(char* argument, std::string_view expected)
{
    return argument != nullptr && std::string_view(argument).starts_with(expected);
}

QByteArray environmentValue(const char* name)
{
    if (!qEnvironmentVariableIsSet(name)) {
        return QByteArray("(unset)");
    }
    const QByteArray value = qgetenv(name);
    return value.isEmpty() ? QByteArray("(empty)") : value;
}

void printText(const std::string& key, const QString& value)
{
    const QByteArray bytes = value.toUtf8();
    std::cout << key << ": " << bytes.constData() << '\n';
}

void printText(const std::string& key, const QByteArray& value)
{
    std::cout << key << ": " << value.constData() << '\n';
}

void printBool(const std::string& key, bool value)
{
    std::cout << key << ": " << (value ? "true" : "false") << '\n';
}

void printStringList(const std::string& prefix, const QStringList& values)
{
    std::cout << prefix << ".count: " << values.size() << '\n';
    for (int index = 0; index < values.size(); ++index) {
        printText(prefix + "." + std::to_string(index), values.at(index));
    }
}

bool hasNonWhitespaceText(const QString& text)
{
    return std::ranges::any_of(text, [](QChar ch) {
        return !ch.isSpace();
    });
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

void printFilesystemPathStatus(const std::string& prefix, const QString& path)
{
    const bool hasText = hasNonWhitespaceText(path);
    const QFileInfo info(path);
    const bool exists = hasText && info.exists();
    printBool(prefix + ".hasText", hasText);
    printText(prefix + ".absolutePath", hasText ? info.absoluteFilePath() : QStringLiteral("(blank)"));
    printText(prefix + ".canonicalPath", exists ? info.canonicalFilePath() : QStringLiteral("(missing)"));
    printBool(prefix + ".exists", exists);
    printBool(prefix + ".file", exists && info.isFile());
    printBool(prefix + ".dir", exists && info.isDir());
    printBool(prefix + ".readable", exists && info.isReadable());
    printBool(prefix + ".writable", exists && info.isWritable());
    printBool(prefix + ".executable", exists && info.isExecutable());
    std::cout << prefix << ".size: " << (exists ? info.size() : -1) << '\n';
    printText(
        prefix + ".lastModifiedUtc",
        exists ? info.lastModified().toUTC().toString(Qt::ISODateWithMs) : QStringLiteral("(missing)"));
}

QString platformPluginFileName(const QString& baseName)
{
#if defined(Q_OS_WIN)
    return baseName + ".dll";
#elif defined(Q_OS_MACOS)
    return "lib" + baseName + ".dylib";
#else
    return "lib" + baseName + ".so";
#endif
}

void appendUnique(QStringList& values, const QString& value)
{
    if (!value.isEmpty() && !values.contains(value)) {
        values.push_back(value);
    }
}

QStringList commonWaylandPlatformPluginBaseNames()
{
    QStringList baseNames;
    appendUnique(baseNames, "qwayland");
    appendUnique(baseNames, "qwayland-generic");
    appendUnique(baseNames, "qwayland-egl");
    return baseNames;
}

QStringList commonWindowsPlatformPluginBaseNames()
{
    QStringList baseNames;
    appendUnique(baseNames, "qwindows");
    return baseNames;
}

QStringList platformPluginBaseNames(const QString& platformName)
{
    QStringList baseNames;
    if (platformName == "offscreen") {
        appendUnique(baseNames, "qoffscreen");
    } else if (platformName == "minimal") {
        appendUnique(baseNames, "qminimal");
    } else if (platformName == "xcb") {
        appendUnique(baseNames, "qxcb");
    } else if (platformName.startsWith("wayland")) {
        baseNames = commonWaylandPlatformPluginBaseNames();
    } else if (platformName.startsWith("windows")) {
        baseNames = commonWindowsPlatformPluginBaseNames();
    } else if (!platformName.isEmpty()) {
        appendUnique(baseNames, "q" + platformName);
    }
    return baseNames;
}

QStringList platformPluginSearchRoots(const QString& pluginPath, const QStringList& libraryPaths)
{
    QStringList roots;
    appendUnique(roots, pluginPath);
    for (const QString& libraryPath : libraryPaths) {
        appendUnique(roots, libraryPath);
    }
    return roots;
}

QString platformPluginPath(const QString& root, const QString& fileName)
{
    return root + "/platforms/" + fileName;
}

void printAvailablePlatformPluginDiagnostics(const QStringList& roots)
{
    std::cout << "qt.platformPlugin.availableRoot.count: " << roots.size() << '\n';
    for (int rootIndex = 0; rootIndex < roots.size(); ++rootIndex) {
        const QString root = roots.at(rootIndex);
        const QString platformsPath = root + "/platforms";
        const std::string prefix = "qt.platformPlugin.availableRoot." + std::to_string(rootIndex);
        printText(prefix + ".path", root);
        printText(prefix + ".platformsDir", platformsPath);
        printFilesystemPathStatus(prefix + ".platformsDir", platformsPath);

        const QFileInfoList plugins =
            QDir(platformsPath).entryInfoList(QDir::Files | QDir::Readable, QDir::Name);
        std::cout << prefix << ".plugin.count: " << plugins.size() << '\n';
        for (int pluginIndex = 0; pluginIndex < plugins.size(); ++pluginIndex) {
            const QFileInfo& plugin = plugins.at(pluginIndex);
            const std::string pluginPrefix =
                prefix + ".plugin." + std::to_string(pluginIndex);
            printText(pluginPrefix + ".fileName", plugin.fileName());
            printText(pluginPrefix + ".path", plugin.absoluteFilePath());
            printFilesystemPathStatus(pluginPrefix, plugin.absoluteFilePath());
        }
    }
}

bool isQtResourcePath(const QString& path)
{
    return path.startsWith("qrc:/") || path.startsWith(":/");
}

void printQmlImportPathDiagnostics()
{
    QQmlEngine diagnosticQmlEngine;
    const QStringList importPaths = diagnosticQmlEngine.importPathList();
    printStringList("qt.qmlImportPath", importPaths);
    for (int index = 0; index < importPaths.size(); ++index) {
        const QString path = importPaths.at(index);
        const std::string prefix = "qt.qmlImportPath." + std::to_string(index);
        const bool resourcePath = isQtResourcePath(path);
        printBool(prefix + ".resource", resourcePath);
        if (!resourcePath) {
            printFilesystemPathStatus(prefix, path);
        }
    }
}

struct StandardPathDiagnostic {
    const char* name;
    QStandardPaths::StandardLocation location;
};

void printStandardPathDiagnostics()
{
    const StandardPathDiagnostic diagnostics[] = {
        {"home", QStandardPaths::HomeLocation},
        {"documents", QStandardPaths::DocumentsLocation},
        {"config", QStandardPaths::ConfigLocation},
        {"appConfig", QStandardPaths::AppConfigLocation},
        {"genericConfig", QStandardPaths::GenericConfigLocation},
        {"genericData", QStandardPaths::GenericDataLocation},
        {"appData", QStandardPaths::AppDataLocation},
        {"genericCache", QStandardPaths::GenericCacheLocation},
        {"cache", QStandardPaths::CacheLocation},
        {"runtime", QStandardPaths::RuntimeLocation},
        {"temp", QStandardPaths::TempLocation},
    };

    for (const StandardPathDiagnostic& diagnostic : diagnostics) {
        const std::string prefix = std::string("qt.standardPath.") + diagnostic.name;
        const QString path = QStandardPaths::writableLocation(diagnostic.location);
        printText(prefix, path.isEmpty() ? QStringLiteral("(empty)") : path);
        printFilesystemPathStatus(prefix, path);
    }
}

void printApplicationFontDiagnostics()
{
    const QFont font = qApp->font();
    const QFontMetricsF metrics(font);
    const QString asciiSample = QStringLiteral("Candidate 100% +123.4");
    const QString cjkSample =
        QString::fromUtf8("\xE5\x80\x99\xE9\x80\x89\xE7\x82\xB9 100% +123.4");

    printText("qt.font.application.family", font.family());
    printText("qt.font.application.styleName", font.styleName().isEmpty() ? QStringLiteral("(empty)") : font.styleName());
    std::cout << "qt.font.application.pointSizeF: " << font.pointSizeF() << '\n';
    std::cout << "qt.font.application.pixelSize: " << font.pixelSize() << '\n';
    std::cout << "qt.font.application.weight: " << font.weight() << '\n';
    printBool("qt.font.application.bold", font.bold());
    printBool("qt.font.application.italic", font.italic());
    printBool("qt.font.application.fixedPitch", font.fixedPitch());
    printBool("qt.font.application.kerning", font.kerning());
    std::cout << "qt.font.application.hintingPreference: " << static_cast<int>(font.hintingPreference()) << '\n';
    std::cout << "qt.font.metrics.height: " << metrics.height() << '\n';
    std::cout << "qt.font.metrics.lineSpacing: " << metrics.lineSpacing() << '\n';
    std::cout << "qt.font.metrics.ascent: " << metrics.ascent() << '\n';
    std::cout << "qt.font.metrics.descent: " << metrics.descent() << '\n';
    std::cout << "qt.font.metrics.averageCharWidth: " << metrics.averageCharWidth() << '\n';
    std::cout << "qt.font.metrics.sampleAsciiWidth: " << metrics.horizontalAdvance(asciiSample) << '\n';
    std::cout << "qt.font.metrics.sampleCjkWidth: " << metrics.horizontalAdvance(cjkSample) << '\n';
}

QString colorSchemeText(Qt::ColorScheme colorScheme)
{
    switch (colorScheme) {
    case Qt::ColorScheme::Unknown:
        return QStringLiteral("Unknown");
    case Qt::ColorScheme::Light:
        return QStringLiteral("Light");
    case Qt::ColorScheme::Dark:
        return QStringLiteral("Dark");
    }
    return QString("ColorScheme(%1)").arg(static_cast<int>(colorScheme));
}

QString paletteColorText(const QPalette& palette, QPalette::ColorRole role)
{
    return palette.color(role).name(QColor::HexArgb);
}

void printApplicationAppearanceDiagnostics()
{
    const QStyle* style = qApp->style();
    const QPalette palette = qApp->palette();
    printText("qt.appearance.style.objectName", style != nullptr ? style->objectName() : QStringLiteral("(none)"));
    printText(
        "qt.appearance.style.className",
        style != nullptr ? QString::fromLatin1(style->metaObject()->className()) : QStringLiteral("(none)"));
    printText("qt.appearance.colorScheme", colorSchemeText(qApp->styleHints()->colorScheme()));
    printText("qt.appearance.palette.window", paletteColorText(palette, QPalette::Window));
    printText("qt.appearance.palette.windowText", paletteColorText(palette, QPalette::WindowText));
    printText("qt.appearance.palette.base", paletteColorText(palette, QPalette::Base));
    printText("qt.appearance.palette.text", paletteColorText(palette, QPalette::Text));
    printText("qt.appearance.palette.button", paletteColorText(palette, QPalette::Button));
    printText("qt.appearance.palette.buttonText", paletteColorText(palette, QPalette::ButtonText));
    printText("qt.appearance.palette.highlight", paletteColorText(palette, QPalette::Highlight));
    printText("qt.appearance.palette.highlightedText", paletteColorText(palette, QPalette::HighlightedText));
    printBool("qt.appearance.palette.windowLooksDark", palette.color(QPalette::Window).lightness() < 128);
    const QString styleSheet = qApp->styleSheet();
    printBool("qt.appearance.styleSheet.present", !styleSheet.isEmpty());
    std::cout << "qt.appearance.styleSheet.length: " << styleSheet.size() << '\n';
}

void printDialogDiagnostics()
{
    QFileDialog fileDialog;
    printBool(
        "qt.dialog.fileDialog.defaultDontUseNativeDialog",
        fileDialog.testOption(QFileDialog::DontUseNativeDialog));
    printBool(
        "qt.dialog.fileDialog.defaultMayUseNativeDialog",
        !fileDialog.testOption(QFileDialog::DontUseNativeDialog));

    lizzie::ui::EngineSettingsDialog settingsDialog;
    const QList<QPushButton*> browseButtons = settingsDialog.findChildren<QPushButton*>("lizziePathBrowseButton");
    std::cout << "qt.dialog.settings.pathBrowseButton.count: " << browseButtons.size() << '\n';
    int fileButtonCount = 0;
    int directoryButtonCount = 0;
    for (int index = 0; index < browseButtons.size(); ++index) {
        const QPushButton* button = browseButtons.at(index);
        const QByteArray rowKey = button->property("lizzieSettingsPathRowKey").toByteArray();
        const QByteArray browseKey = button->property("lizzieSettingsBrowseButtonI18nKey").toByteArray();
        if (browseKey == "selectFile") {
            ++fileButtonCount;
        } else if (browseKey == "selectDirectory") {
            ++directoryButtonCount;
        }
        const std::string prefix = "qt.dialog.settings.pathBrowseButton." + std::to_string(index);
        printText(prefix + ".rowKey", rowKey.isEmpty() ? QByteArray("(unset)") : rowKey);
        printText(prefix + ".selectorKind", browseKey.isEmpty() ? QByteArray("(unset)") : browseKey);
        printText(prefix + ".toolTip", button->toolTip());
        printText(prefix + ".accessibleName", button->accessibleName());
    }
    std::cout << "qt.dialog.settings.pathBrowseButton.file.count: " << fileButtonCount << '\n';
    std::cout << "qt.dialog.settings.pathBrowseButton.directory.count: " << directoryButtonCount << '\n';
}

QString sizeText(const QSize& size)
{
    return QString("%1x%2").arg(size.width()).arg(size.height());
}

QString dockAreaText(Qt::DockWidgetArea area)
{
    switch (area) {
    case Qt::LeftDockWidgetArea:
        return QStringLiteral("Left");
    case Qt::RightDockWidgetArea:
        return QStringLiteral("Right");
    case Qt::TopDockWidgetArea:
        return QStringLiteral("Top");
    case Qt::BottomDockWidgetArea:
        return QStringLiteral("Bottom");
    case Qt::NoDockWidgetArea:
        return QStringLiteral("None");
    case Qt::AllDockWidgetAreas:
        return QStringLiteral("All");
    }
    return QString("DockWidgetArea(%1)").arg(static_cast<int>(area));
}

QString toolBarAreaText(Qt::ToolBarArea area)
{
    switch (area) {
    case Qt::LeftToolBarArea:
        return QStringLiteral("Left");
    case Qt::RightToolBarArea:
        return QStringLiteral("Right");
    case Qt::TopToolBarArea:
        return QStringLiteral("Top");
    case Qt::BottomToolBarArea:
        return QStringLiteral("Bottom");
    case Qt::NoToolBarArea:
        return QStringLiteral("None");
    case Qt::AllToolBarAreas:
        return QStringLiteral("All");
    }
    return QString("ToolBarArea(%1)").arg(static_cast<int>(area));
}

QString widgetClassName(const QObject* object)
{
    return object != nullptr ? QString::fromLatin1(object->metaObject()->className()) : QStringLiteral("(none)");
}

QString actionTextForDiagnostics(const QAction* action)
{
    if (action == nullptr) {
        return QStringLiteral("(none)");
    }
    QString text = action->text();
    text.remove('&');
    return text.isEmpty() ? QStringLiteral("(empty)") : text;
}

QString shortcutTextForDiagnostics(const QAction* action)
{
    if (action == nullptr || action->shortcut().isEmpty()) {
        return QStringLiteral("(empty)");
    }
    return action->shortcut().toString(QKeySequence::PortableText);
}

template <typename Widget>
QList<Widget*> sortedNamedChildren(const QWidget& parent)
{
    QList<Widget*> children = parent.findChildren<Widget*>();
    std::ranges::sort(children, [](const Widget* left, const Widget* right) {
        const QString leftName = left == nullptr ? QString() : left->objectName();
        const QString rightName = right == nullptr ? QString() : right->objectName();
        if (leftName == rightName) {
            return widgetClassName(left) < widgetClassName(right);
        }
        return leftName < rightName;
    });
    return children;
}

void printActionListDiagnostics(const std::string& prefix, const QList<QAction*>& actions)
{
    int commandActionCount = 0;
    std::cout << prefix << ".action.count: " << actions.size() << '\n';
    for (int index = 0; index < actions.size(); ++index) {
        const QAction* action = actions.at(index);
        const std::string actionPrefix = prefix + ".action." + std::to_string(index);
        const bool separator = action != nullptr && action->isSeparator();
        if (!separator) {
            ++commandActionCount;
        }
        printText(actionPrefix + ".text", actionTextForDiagnostics(action));
        printBool(actionPrefix + ".separator", separator);
        printBool(actionPrefix + ".enabled", action != nullptr && action->isEnabled());
        printBool(actionPrefix + ".visible", action != nullptr && action->isVisible());
        printBool(actionPrefix + ".checkable", action != nullptr && action->isCheckable());
        printBool(actionPrefix + ".checked", action != nullptr && action->isChecked());
        printText(actionPrefix + ".shortcut", shortcutTextForDiagnostics(action));
        printBool(actionPrefix + ".iconPresent", action != nullptr && !action->icon().isNull());
    }
    std::cout << prefix << ".commandAction.count: " << commandActionCount << '\n';
}

void printTableWidgetDiagnostics(const std::string& prefix, const QTableWidget* table)
{
    printText(prefix + ".objectName", table != nullptr ? table->objectName() : QStringLiteral("(none)"));
    printText(prefix + ".className", widgetClassName(table));
    std::cout << prefix << ".row.count: " << (table != nullptr ? table->rowCount() : -1) << '\n';
    std::cout << prefix << ".column.count: " << (table != nullptr ? table->columnCount() : -1) << '\n';
    if (table == nullptr) {
        std::cout << prefix << ".horizontalHeader.count: 0\n";
        return;
    }
    QStringList headers;
    for (int column = 0; column < table->columnCount(); ++column) {
        const QTableWidgetItem* item = table->horizontalHeaderItem(column);
        headers.push_back(item != nullptr ? item->text() : QStringLiteral("(empty)"));
    }
    printStringList(prefix + ".horizontalHeader", headers);
    printBool(prefix + ".verticalHeader.visible", table->verticalHeader()->isVisible());
    std::cout << prefix << ".selectionMode: " << static_cast<int>(table->selectionMode()) << '\n';
    std::cout << prefix << ".selectionBehavior: " << static_cast<int>(table->selectionBehavior()) << '\n';
    std::cout << prefix << ".editTriggers: " << static_cast<int>(table->editTriggers()) << '\n';
}

void printMainWindowUiDiagnostics()
{
    lizzie::ui::MainWindow window;
    window.ensurePolished();

    printText("qt.ui.mainWindow.objectName", window.objectName().isEmpty() ? QStringLiteral("(empty)") : window.objectName());
    printText("qt.ui.mainWindow.windowTitle", window.windowTitle());
    printText("qt.ui.mainWindow.size", sizeText(window.size()));
    printText("qt.ui.mainWindow.minimumSize", sizeText(window.minimumSize()));
    printText("qt.ui.mainWindow.centralWidget.className", widgetClassName(window.centralWidget()));
    printText(
        "qt.ui.mainWindow.centralWidget.objectName",
        window.centralWidget() != nullptr && !window.centralWidget()->objectName().isEmpty()
            ? window.centralWidget()->objectName()
            : QStringLiteral("(empty)"));

    const QList<QQuickWidget*> quickWidgets = sortedNamedChildren<QQuickWidget>(window);
    std::cout << "qt.ui.mainWindow.quickWidget.count: " << quickWidgets.size() << '\n';
    for (int index = 0; index < quickWidgets.size(); ++index) {
        const QQuickWidget* quickWidget = quickWidgets.at(index);
        const std::string prefix = "qt.ui.mainWindow.quickWidget." + std::to_string(index);
        printText(prefix + ".objectName", quickWidget->objectName());
        printText(prefix + ".size", sizeText(quickWidget->size()));
        printText(prefix + ".minimumSize", sizeText(quickWidget->minimumSize()));
        std::cout << prefix << ".resizeMode: " << static_cast<int>(quickWidget->resizeMode()) << '\n';
        printBool(prefix + ".rootObjectPresent", quickWidget->rootObject() != nullptr);
    }

    const QList<QToolBar*> toolBars = sortedNamedChildren<QToolBar>(window);
    std::cout << "qt.ui.mainWindow.toolBar.count: " << toolBars.size() << '\n';
    for (int index = 0; index < toolBars.size(); ++index) {
        const QToolBar* toolBar = toolBars.at(index);
        const std::string prefix = "qt.ui.mainWindow.toolBar." + std::to_string(index);
        printText(prefix + ".objectName", toolBar->objectName());
        printText(prefix + ".windowTitle", toolBar->windowTitle());
        printBool(prefix + ".movable", toolBar->isMovable());
        printBool(prefix + ".floatable", toolBar->isFloatable());
        printText(prefix + ".area", toolBarAreaText(window.toolBarArea(toolBar)));
        printActionListDiagnostics(prefix, toolBar->actions());
    }

    const QList<QDockWidget*> docks = sortedNamedChildren<QDockWidget>(window);
    std::cout << "qt.ui.mainWindow.dock.count: " << docks.size() << '\n';
    for (int index = 0; index < docks.size(); ++index) {
        QDockWidget* dock = docks.at(index);
        const QWidget* dockWidget = dock->widget();
        const std::string prefix = "qt.ui.mainWindow.dock." + std::to_string(index);
        printText(prefix + ".objectName", dock->objectName());
        printText(prefix + ".windowTitle", dock->windowTitle());
        printText(prefix + ".area", dockAreaText(window.dockWidgetArea(dock)));
        printBool(prefix + ".floating", dock->isFloating());
        printBool(prefix + ".visible", dock->isVisible());
        printText(prefix + ".widget.className", widgetClassName(dockWidget));
        printText(
            prefix + ".widget.objectName",
            dockWidget != nullptr && !dockWidget->objectName().isEmpty()
                ? dockWidget->objectName()
                : QStringLiteral("(empty)"));
    }

    const QList<QTableWidget*> tables = sortedNamedChildren<QTableWidget>(window);
    std::cout << "qt.ui.mainWindow.table.count: " << tables.size() << '\n';
    for (int index = 0; index < tables.size(); ++index) {
        printTableWidgetDiagnostics("qt.ui.mainWindow.table." + std::to_string(index), tables.at(index));
    }

    const QList<QTreeWidget*> trees = sortedNamedChildren<QTreeWidget>(window);
    std::cout << "qt.ui.mainWindow.tree.count: " << trees.size() << '\n';
    for (int index = 0; index < trees.size(); ++index) {
        const QTreeWidget* tree = trees.at(index);
        const std::string prefix = "qt.ui.mainWindow.tree." + std::to_string(index);
        printText(prefix + ".objectName", tree->objectName());
        std::cout << prefix << ".column.count: " << tree->columnCount() << '\n';
        printText(prefix + ".header.0", tree->headerItem() != nullptr ? tree->headerItem()->text(0) : QStringLiteral("(empty)"));
        printBool(prefix + ".column.1.hidden", tree->isColumnHidden(1));
    }

    QList<QMenu*> menus;
    for (QAction* action : window.menuBar()->actions()) {
        if (action != nullptr && action->menu() != nullptr) {
            menus.push_back(action->menu());
        }
    }
    std::cout << "qt.ui.mainWindow.menu.count: " << menus.size() << '\n';
    for (int index = 0; index < menus.size(); ++index) {
        const QMenu* menu = menus.at(index);
        const std::string prefix = "qt.ui.mainWindow.menu." + std::to_string(index);
        printText(prefix + ".title", actionTextForDiagnostics(menu->menuAction()));
        printActionListDiagnostics(prefix, menu->actions());
    }
}

QString layoutDirectionText(Qt::LayoutDirection direction)
{
    switch (direction) {
    case Qt::LeftToRight:
        return QStringLiteral("LeftToRight");
    case Qt::RightToLeft:
        return QStringLiteral("RightToLeft");
    case Qt::LayoutDirectionAuto:
        return QStringLiteral("LayoutDirectionAuto");
    default:
        return QString("LayoutDirection(%1)").arg(static_cast<int>(direction));
    }
}

void printLocaleCoreDiagnostics(const std::string& prefix, const QLocale& locale)
{
    printText(prefix + ".name", locale.name());
    printText(prefix + ".bcp47Name", locale.bcp47Name());
    printText(prefix + ".nativeLanguageName", locale.nativeLanguageName());
    printText(prefix + ".nativeTerritoryName", locale.nativeTerritoryName());
    printText(prefix + ".textDirection", layoutDirectionText(locale.textDirection()));
    std::cout << prefix << ".measurementSystem: " << static_cast<int>(locale.measurementSystem()) << '\n';
    printText(prefix + ".decimalPoint", QString(locale.decimalPoint()));
    printText(prefix + ".groupSeparator", QString(locale.groupSeparator()));
    std::cout << prefix << ".firstDayOfWeek: " << static_cast<int>(locale.firstDayOfWeek()) << '\n';
    printStringList(prefix + ".uiLanguage", locale.uiLanguages());
}

void printLocaleDiagnostics()
{
    printLocaleCoreDiagnostics("qt.locale.default", QLocale());
    printLocaleCoreDiagnostics("qt.locale.system", QLocale::system());
}

QString settingsFormatText(QSettings::Format format)
{
    switch (format) {
    case QSettings::NativeFormat:
        return QStringLiteral("NativeFormat");
    case QSettings::IniFormat:
        return QStringLiteral("IniFormat");
    case QSettings::InvalidFormat:
        return QStringLiteral("InvalidFormat");
    default:
        return QString("CustomFormat(%1)").arg(static_cast<int>(format));
    }
}

QString settingsScopeText(QSettings::Scope scope)
{
    switch (scope) {
    case QSettings::UserScope:
        return QStringLiteral("UserScope");
    case QSettings::SystemScope:
        return QStringLiteral("SystemScope");
    default:
        return QString("Scope(%1)").arg(static_cast<int>(scope));
    }
}

QString themeModeText(lizzie::app::ThemeMode theme)
{
    switch (theme) {
    case lizzie::app::ThemeMode::System:
        return QStringLiteral("system");
    case lizzie::app::ThemeMode::Light:
        return QStringLiteral("light");
    case lizzie::app::ThemeMode::Dark:
        return QStringLiteral("dark");
    }
    return QString("theme(%1)").arg(static_cast<int>(theme));
}

QString languageModeText(lizzie::app::LanguageMode language)
{
    switch (language) {
    case lizzie::app::LanguageMode::English:
        return QStringLiteral("en");
    case lizzie::app::LanguageMode::Chinese:
        return QStringLiteral("zh");
    }
    return QString("language(%1)").arg(static_cast<int>(language));
}

bool settingsLocationIsFilesystemBacked(const QString& fileName)
{
    if (fileName.isEmpty()) {
        return false;
    }
#if defined(Q_OS_WIN)
    if (fileName.startsWith("HKEY_", Qt::CaseInsensitive)) {
        return false;
    }
#endif
    return true;
}

void printNonFilesystemSettingsLocationStatus()
{
    printText("qt.settings.file.absolutePath", QStringLiteral("(not-filesystem)"));
    printText("qt.settings.file.canonicalPath", QStringLiteral("(not-filesystem)"));
    printBool("qt.settings.file.exists", false);
    printBool("qt.settings.file.file", false);
    printBool("qt.settings.file.dir", false);
    printBool("qt.settings.file.readable", false);
    printBool("qt.settings.file.writable", false);
    printBool("qt.settings.file.executable", false);
    std::cout << "qt.settings.file.size: -1\n";
    printText("qt.settings.file.lastModifiedUtc", QStringLiteral("(not-filesystem)"));
}

void printSettingsStringValue(QSettings& settings, const std::string& prefix, const QString& key)
{
    const bool present = settings.contains(key);
    const QString value = present ? settings.value(key).toString() : QStringLiteral("(unset)");
    printBool(prefix + ".present", present);
    printText(prefix + ".value", value);
}

void printSettingsCommandArgumentsValue(QSettings& settings, const std::string& prefix, const QString& key)
{
    const bool present = settings.contains(key);
    const QString value = present ? settings.value(key).toString() : QStringLiteral("(unset)");
    printBool(prefix + ".present", present);
    printText(prefix + ".value", value);
    const std::vector<std::string> parsed = present ? lizzie::app::splitCommandArguments(value) : std::vector<std::string>{};
    std::cout << prefix << ".parsed.count: " << parsed.size() << '\n';
    for (std::size_t index = 0; index < parsed.size(); ++index) {
        printText(prefix + ".parsed." + std::to_string(index), QString::fromStdString(parsed[index]));
    }
}

lizzie::engine::KataGoConfig engineConfigFromSettings(QSettings& settings, const QString& group)
{
    lizzie::engine::KataGoConfig config;
    config.executable = settings.value(group + QStringLiteral("/executable")).toString().toStdString();
    config.model = settings.value(group + QStringLiteral("/model")).toString().toStdString();
    config.gtpConfig = settings.value(group + QStringLiteral("/gtpConfig")).toString().toStdString();
    config.analysisConfig = settings.value(group + QStringLiteral("/analysisConfig")).toString().toStdString();
    config.workingDirectory = settings.value(group + QStringLiteral("/workingDirectory")).toString().toStdString();
    config.extraArgs =
        lizzie::app::splitCommandArguments(settings.value(group + QStringLiteral("/extraArgs")).toString());
    return config;
}

void printSettingsPathValue(QSettings& settings, const std::string& prefix, const QString& key)
{
    const bool present = settings.contains(key);
    const QString value = present ? settings.value(key).toString() : QStringLiteral("(unset)");
    printBool(prefix + ".present", present);
    printText(prefix + ".value", value);
    printFilesystemPathStatus(prefix + ".path", present ? value : QString());
}

struct SettingsPathReadiness {
    bool present = false;
    QString value;
    bool hasText = false;
    QFileInfo info;
    bool exists = false;
    bool isFile = false;
    bool readable = false;
    bool executable = false;
    bool usable = false;
};

SettingsPathReadiness inspectSettingsPathReadiness(QSettings& settings, const QString& key, bool requireExecutable)
{
    SettingsPathReadiness status;
    status.present = settings.contains(key);
    status.value = status.present ? settings.value(key).toString() : QString();
    status.hasText = hasNonWhitespaceText(status.value);
    status.info = QFileInfo(status.value);
    status.exists = status.hasText && status.info.exists();
    status.isFile = status.exists && status.info.isFile();
    status.readable = status.isFile && status.info.isReadable();
    status.executable = status.isFile && status.info.isExecutable();
    status.usable = status.present && status.hasText && status.readable && (!requireExecutable || status.executable);
    return status;
}

void appendSettingsPathReadinessName(
    QStringList& missing,
    QStringList& invalid,
    const QString& name,
    const SettingsPathReadiness& status)
{
    if (!status.present || !status.hasText) {
        missing.push_back(name);
    } else if (!status.usable) {
        invalid.push_back(name);
    }
}

struct SettingsEngineConfigReadiness {
    bool complete = false;
    bool ready = false;
    bool gtpReady = false;
    bool analysisReady = false;
    QString status;
    QStringList missing;
    QStringList invalid;
};

SettingsEngineConfigReadiness inspectSettingsEngineConfigReadiness(QSettings& settings, const QString& group)
{
    const SettingsPathReadiness executable =
        inspectSettingsPathReadiness(settings, group + QStringLiteral("/executable"), true);
    const SettingsPathReadiness model =
        inspectSettingsPathReadiness(settings, group + QStringLiteral("/model"), false);
    const SettingsPathReadiness gtpConfig =
        inspectSettingsPathReadiness(settings, group + QStringLiteral("/gtpConfig"), false);
    const SettingsPathReadiness analysisConfig =
        inspectSettingsPathReadiness(settings, group + QStringLiteral("/analysisConfig"), false);

    SettingsEngineConfigReadiness readiness;
    appendSettingsPathReadinessName(readiness.missing, readiness.invalid, QStringLiteral("executable"), executable);
    appendSettingsPathReadinessName(readiness.missing, readiness.invalid, QStringLiteral("model"), model);
    appendSettingsPathReadinessName(readiness.missing, readiness.invalid, QStringLiteral("gtpConfig"), gtpConfig);
    appendSettingsPathReadinessName(
        readiness.missing,
        readiness.invalid,
        QStringLiteral("analysisConfig"),
        analysisConfig);

    const bool executableUsable = executable.usable;
    const bool modelUsable = model.usable;
    readiness.gtpReady = executableUsable && modelUsable && gtpConfig.usable;
    readiness.analysisReady = executableUsable && modelUsable && analysisConfig.usable;
    readiness.complete = readiness.missing.empty();
    readiness.ready = readiness.complete && readiness.invalid.empty();
    readiness.status =
        readiness.ready ? QStringLiteral("ready")
                        : (!readiness.complete ? QStringLiteral("missing") : QStringLiteral("invalid"));
    return readiness;
}

void printSettingsEngineConfigReadinessDiagnostics(QSettings& settings, const std::string& prefix, const QString& group)
{
    const SettingsEngineConfigReadiness readiness = inspectSettingsEngineConfigReadiness(settings, group);
    printBool(prefix + ".config.complete", readiness.complete);
    printBool(prefix + ".config.ready", readiness.ready);
    printBool(prefix + ".config.gtpReady", readiness.gtpReady);
    printBool(prefix + ".config.analysisReady", readiness.analysisReady);
    printText(prefix + ".config.status", readiness.status);
    printStringList(prefix + ".config.missing", readiness.missing);
    printStringList(prefix + ".config.invalid", readiness.invalid);
}

void printSettingsByteArrayValue(QSettings& settings, const std::string& prefix, const QString& key)
{
    const bool present = settings.contains(key);
    const QByteArray value = present ? settings.value(key).toByteArray() : QByteArray();
    printBool(prefix + ".present", present);
    std::cout << prefix << ".size: " << value.size() << '\n';
}

void printSettingsEngineConfigDiagnostics(QSettings& settings, const std::string& prefix, const QString& group)
{
    printSettingsPathValue(settings, prefix + ".executable", group + QStringLiteral("/executable"));
    printSettingsPathValue(settings, prefix + ".model", group + QStringLiteral("/model"));
    printSettingsPathValue(settings, prefix + ".gtpConfig", group + QStringLiteral("/gtpConfig"));
    printSettingsPathValue(settings, prefix + ".analysisConfig", group + QStringLiteral("/analysisConfig"));
    printSettingsPathValue(settings, prefix + ".workingDirectory", group + QStringLiteral("/workingDirectory"));
    printSettingsCommandArgumentsValue(settings, prefix + ".extraArgs", group + QStringLiteral("/extraArgs"));
    const lizzie::engine::KataGoConfig config = engineConfigFromSettings(settings, group);
    printBool(prefix + ".hasGtpMinimum", config.hasGtpMinimum());
    printBool(prefix + ".hasAnalysisMinimum", config.hasAnalysisMinimum());
    printSettingsEngineConfigReadinessDiagnostics(settings, prefix, group);
}

void printSettingsAnalysisOptionDiagnostics(QSettings& settings)
{
    printSettingsStringValue(
        settings,
        "qt.settings.analysis.intervalCentiseconds",
        QStringLiteral("analysis/intervalCentiseconds"));
    printSettingsStringValue(
        settings,
        "qt.settings.analysis.includeOwnership",
        QStringLiteral("analysis/includeOwnership"));
    printSettingsStringValue(
        settings,
        "qt.settings.analysis.maxVisits",
        QStringLiteral("analysis/maxVisits"));
    printSettingsStringValue(
        settings,
        "qt.settings.analysis.maxPlayouts",
        QStringLiteral("analysis/maxPlayouts"));
    printSettingsStringValue(
        settings,
        "qt.settings.analysis.maxTimeSeconds",
        QStringLiteral("analysis/maxTimeSeconds"));
    printSettingsStringValue(
        settings,
        "qt.settings.analysis.playoutDoublingAdvantage",
        QStringLiteral("analysis/playoutDoublingAdvantage"));
    printSettingsStringValue(
        settings,
        "qt.settings.analysis.analysisWideRootNoise",
        QStringLiteral("analysis/analysisWideRootNoise"));
}

void printSettingsBoardDisplayDiagnostics(QSettings& settings)
{
    printSettingsStringValue(settings, "qt.settings.board.showOwnership", QStringLiteral("board/showOwnership"));
    printSettingsStringValue(
        settings,
        "qt.settings.board.ownershipDisplayMode",
        QStringLiteral("board/ownershipDisplayMode"));
    printSettingsStringValue(settings, "qt.settings.board.ownershipOpacity", QStringLiteral("board/ownershipOpacity"));
    printSettingsPathValue(
        settings,
        "qt.settings.board.blackStoneTexture",
        QStringLiteral("board/blackStoneTexture"));
    printSettingsPathValue(
        settings,
        "qt.settings.board.whiteStoneTexture",
        QStringLiteral("board/whiteStoneTexture"));
}

void printSettingsFileBehaviorDiagnostics(QSettings& settings)
{
    printSettingsStringValue(
        settings,
        "qt.settings.files.importLegacySgfAnalysis",
        QStringLiteral("files/importLegacySgfAnalysis"));
    printSettingsStringValue(
        settings,
        "qt.settings.files.loadAnalysisSidecar",
        QStringLiteral("files/loadAnalysisSidecar"));
    printSettingsStringValue(
        settings,
        "qt.settings.files.writeAnalysisSidecarAfterBatch",
        QStringLiteral("files/writeAnalysisSidecarAfterBatch"));
}

void printSettingsAppearanceDiagnostics(QSettings& settings)
{
    printSettingsStringValue(settings, "qt.settings.appearance.theme", QStringLiteral("appearance/theme"));
    printSettingsStringValue(settings, "qt.settings.appearance.language", QStringLiteral("appearance/language"));
    printSettingsStringValue(settings, "qt.settings.appearance.fontScale", QStringLiteral("appearance/fontScale"));

    const lizzie::app::EngineUiSettings normalized = lizzie::app::loadEngineUiSettings(settings);
    printText("qt.settings.appearance.normalized.theme", themeModeText(normalized.appearance.theme));
    printText("qt.settings.appearance.normalized.language", languageModeText(normalized.appearance.language));
    std::cout << "qt.settings.appearance.normalized.fontScale: " << normalized.appearance.fontScale << '\n';
}

void printSettingsShortcutDiagnostics(QSettings& settings)
{
    const std::pair<const char*, const char*> shortcuts[] = {
        {"new", "shortcuts/new"},
        {"open", "shortcuts/open"},
        {"save", "shortcuts/save"},
        {"saveAs", "shortcuts/saveAs"},
        {"analyze", "shortcuts/analyze"},
        {"estimate", "shortcuts/estimate"},
        {"batch", "shortcuts/batch"},
        {"restartEngine", "shortcuts/restartEngine"},
        {"compare", "shortcuts/compare"},
        {"aiMove", "shortcuts/aiMove"},
        {"humanVsAi", "shortcuts/humanVsAi"},
        {"selfPlay", "shortcuts/selfPlay"},
        {"engineGame", "shortcuts/engineGame"},
        {"previous", "shortcuts/previous"},
        {"next", "shortcuts/next"},
        {"undo", "shortcuts/undo"},
        {"pass", "shortcuts/pass"},
        {"resign", "shortcuts/resign"},
        {"settings", "shortcuts/settings"},
    };
    for (const auto& shortcut : shortcuts) {
        printSettingsStringValue(
            settings,
            std::string("qt.settings.shortcuts.") + shortcut.first,
            QString::fromLatin1(shortcut.second));
    }
}

QString diagnosticRectText(const QRect& rect)
{
    return QString("%1x%2+%3+%4").arg(rect.width()).arg(rect.height()).arg(rect.x()).arg(rect.y());
}

void printSavedWindowGeometryRestoreDiagnostics(QSettings& settings)
{
    constexpr const char* prefix = "qt.settings.window.geometry";
    const bool present = settings.contains("window/geometry");
    const QByteArray value = present ? settings.value("window/geometry").toByteArray() : QByteArray();
    QWidget diagnosticWindow;
    const bool restoreSucceeded = present && !value.isEmpty() && diagnosticWindow.restoreGeometry(value);
    const QRect restoredGeometry = restoreSucceeded ? diagnosticWindow.geometry() : QRect();
    const QRect restoredFrameGeometry = restoreSucceeded ? diagnosticWindow.frameGeometry() : QRect();
    const auto visibleArea =
        restoreSucceeded ? lizzie::app::windowVisibleAreaOnAvailableScreens(restoredFrameGeometry) : 0;
    const auto requiredVisibleArea =
        restoreSucceeded ? lizzie::app::requiredVisibleAreaForWindowFrame(restoredFrameGeometry) : 0;
    const auto adjustedGeometry = restoreSucceeded
        ? lizzie::app::adjustedWindowGeometryForAvailableScreens(restoredFrameGeometry, diagnosticWindow.size())
        : std::nullopt;

    printBool(std::string(prefix) + ".restoreSucceeded", restoreSucceeded);
    printText(
        std::string(prefix) + ".restoredGeometry",
        restoreSucceeded ? diagnosticRectText(restoredGeometry) : QStringLiteral("(unavailable)"));
    printText(
        std::string(prefix) + ".restoredFrameGeometry",
        restoreSucceeded ? diagnosticRectText(restoredFrameGeometry) : QStringLiteral("(unavailable)"));
    std::cout << prefix << ".visibleAreaOnAvailableScreens: " << visibleArea << '\n';
    std::cout << prefix << ".requiredVisibleArea: " << requiredVisibleArea << '\n';
    printBool(
        std::string(prefix) + ".substantiallyVisible",
        restoreSucceeded && lizzie::app::windowFrameIsSubstantiallyVisible(restoredFrameGeometry));
    printBool(std::string(prefix) + ".wouldRecenter", adjustedGeometry.has_value());
    printText(
        std::string(prefix) + ".adjustedGeometry",
        adjustedGeometry.has_value() ? diagnosticRectText(*adjustedGeometry) : QStringLiteral("(not-needed)"));
}

void printSavedWindowStateRestoreDiagnostics(QSettings& settings)
{
    constexpr const char* prefix = "qt.settings.window.state";
    const bool present = settings.contains("window/state");
    const QByteArray value = present ? settings.value("window/state").toByteArray() : QByteArray();
    QMainWindow diagnosticWindow;
    const bool restoreSucceeded = present && !value.isEmpty() && diagnosticWindow.restoreState(value);
    printBool(std::string(prefix) + ".restoreSucceeded", restoreSucceeded);
}

void printSettingsStoredValueDiagnostics(QSettings& settings)
{
    const bool firstRunPresent = settings.contains("startup/firstRunComplete");
    const std::optional<bool> firstRunValue =
        firstRunPresent ? boolFromStoredValue(settings.value("startup/firstRunComplete")) : std::nullopt;
    printBool("qt.settings.startup.firstRunComplete.present", firstRunPresent);
    printText(
        "qt.settings.startup.firstRunComplete.value",
        firstRunValue.has_value() ? (*firstRunValue ? QStringLiteral("true") : QStringLiteral("false"))
                                  : QStringLiteral("(unset)"));
    printSettingsEngineConfigDiagnostics(settings, "qt.settings.engine", QStringLiteral("engine"));
    printSettingsEngineConfigDiagnostics(settings, "qt.settings.compare", QStringLiteral("compare"));
    printSettingsAnalysisOptionDiagnostics(settings);
    printSettingsBoardDisplayDiagnostics(settings);
    printSettingsFileBehaviorDiagnostics(settings);
    printSettingsShortcutDiagnostics(settings);
    printSettingsAppearanceDiagnostics(settings);
    printSettingsPathValue(settings, "qt.settings.session.lastFile", QStringLiteral("session/lastFile"));
    printSettingsStringValue(settings, "qt.settings.session.currentNodeId", QStringLiteral("session/currentNodeId"));
    printSettingsStringValue(
        settings,
        "qt.settings.session.collectionGameIndex",
        QStringLiteral("session/collectionGameIndex"));
    printSettingsByteArrayValue(settings, "qt.settings.window.geometry", QStringLiteral("window/geometry"));
    printSavedWindowGeometryRestoreDiagnostics(settings);
    printSettingsByteArrayValue(settings, "qt.settings.window.state", QStringLiteral("window/state"));
    printSavedWindowStateRestoreDiagnostics(settings);
}

void printSettingsStorageDiagnosticsFor(QSettings& settings, const QString& diagnosticsOverrideFile)
{
    const QString fileName = settings.fileName();
    const bool filesystemBacked = settingsLocationIsFilesystemBacked(fileName);
    printText(
        "qt.settings.diagnosticsOverrideFile",
        diagnosticsOverrideFile.isEmpty() ? QStringLiteral("(unset)") : diagnosticsOverrideFile);
    printText("qt.settings.organizationName", QCoreApplication::organizationName());
    printText("qt.settings.applicationName", QCoreApplication::applicationName());
    printText("qt.settings.defaultFormat", settingsFormatText(QSettings::defaultFormat()));
    printText("qt.settings.format", settingsFormatText(settings.format()));
    printText("qt.settings.scope", settingsScopeText(QSettings::UserScope));
    printText("qt.settings.fileName", fileName.isEmpty() ? QStringLiteral("(empty)") : fileName);
    printBool("qt.settings.fileSystemBacked", filesystemBacked);
    if (filesystemBacked) {
        printFilesystemPathStatus("qt.settings.file", fileName);
    } else {
        printNonFilesystemSettingsLocationStatus();
    }
    printSettingsStoredValueDiagnostics(settings);
}

void printSettingsStorageDiagnostics()
{
    const QByteArray diagnosticsSettingsFile = qgetenv("LIZZIE_DIAGNOSTICS_SETTINGS_FILE");
    if (!diagnosticsSettingsFile.isEmpty()) {
        const QString diagnosticsSettingsPath = QString::fromLocal8Bit(diagnosticsSettingsFile);
        QSettings settings(diagnosticsSettingsPath, QSettings::IniFormat);
        printSettingsStorageDiagnosticsFor(settings, diagnosticsSettingsPath);
        return;
    }

    QSettings settings;
    printSettingsStorageDiagnosticsFor(settings, QString());
}

struct StoredEngineConfigReadiness {
    SettingsEngineConfigReadiness main;
    SettingsEngineConfigReadiness compare;
};

StoredEngineConfigReadiness inspectStoredEngineConfigReadiness()
{
    StoredEngineConfigReadiness readiness;
    const QByteArray diagnosticsSettingsFile = qgetenv("LIZZIE_DIAGNOSTICS_SETTINGS_FILE");
    if (!diagnosticsSettingsFile.isEmpty()) {
        QSettings settings(QString::fromLocal8Bit(diagnosticsSettingsFile), QSettings::IniFormat);
        readiness.main = inspectSettingsEngineConfigReadiness(settings, QStringLiteral("engine"));
        readiness.compare = inspectSettingsEngineConfigReadiness(settings, QStringLiteral("compare"));
        return readiness;
    }

    QSettings settings;
    readiness.main = inspectSettingsEngineConfigReadiness(settings, QStringLiteral("engine"));
    readiness.compare = inspectSettingsEngineConfigReadiness(settings, QStringLiteral("compare"));
    return readiness;
}

void printPlatformPluginCandidateDiagnostics(
    const std::string& prefix,
    const QStringList& baseNames,
    const QStringList& roots)
{
    printStringList(prefix + ".expectedFile", [&]() {
        QStringList fileNames;
        for (const QString& baseName : baseNames) {
            appendUnique(fileNames, platformPluginFileName(baseName));
        }
        return fileNames;
    }());
    std::cout << prefix << ".searchRoot.count: " << roots.size() << '\n';
    for (int index = 0; index < roots.size(); ++index) {
        printText(prefix + ".searchRoot." + std::to_string(index), roots.at(index));
    }

    bool found = false;
    int candidateIndex = 0;
    for (const QString& baseName : baseNames) {
        const QString fileName = platformPluginFileName(baseName);
        for (const QString& root : roots) {
            const QString path = platformPluginPath(root, fileName);
            const std::string candidatePrefix = prefix + ".candidate." + std::to_string(candidateIndex);
            printText(candidatePrefix + ".baseName", baseName);
            printText(candidatePrefix + ".path", path);
            printFilesystemPathStatus(candidatePrefix, path);
            const QFileInfo info(path);
            found = found || (info.exists() && info.isFile() && info.isReadable());
            ++candidateIndex;
        }
    }
    std::cout << prefix << ".candidate.count: " << candidateIndex << '\n';
    printBool(prefix + ".found", found);
}

void printPlatformPluginDiagnostics(const QString& platformName, const QString& pluginPath, const QStringList& libraryPaths)
{
    const QStringList roots = platformPluginSearchRoots(pluginPath, libraryPaths);
    printText("qt.platformPlugin.current.name", platformName);
    printPlatformPluginCandidateDiagnostics(
        "qt.platformPlugin.current",
        platformPluginBaseNames(platformName),
        roots);
    printPlatformPluginCandidateDiagnostics(
        "qt.platformPlugin.commonWayland",
        commonWaylandPlatformPluginBaseNames(),
        roots);
    printPlatformPluginCandidateDiagnostics(
        "qt.platformPlugin.commonWindows",
        commonWindowsPlatformPluginBaseNames(),
        roots);
    printAvailablePlatformPluginDiagnostics(roots);
}

QString qtRuntimeLibraryFileName(const QString& moduleName)
{
#if defined(Q_OS_WIN)
    return QStringLiteral("Qt6") + moduleName + QStringLiteral(".dll");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("Qt") + moduleName + QStringLiteral(".framework/Qt") + moduleName;
#else
    return QStringLiteral("libQt6") + moduleName + QStringLiteral(".so.6");
#endif
}

void printAppLocalQtRuntimeDiagnostics(const QString& appDir)
{
    printText("qt.runtime.appLocal.dir", appDir);
    printFilesystemPathStatus("qt.runtime.appLocal.dir", appDir);

    const QStringList modules = {
        QStringLiteral("Core"),
        QStringLiteral("Gui"),
        QStringLiteral("Widgets"),
        QStringLiteral("Quick"),
        QStringLiteral("QuickWidgets"),
        QStringLiteral("Qml"),
        QStringLiteral("Network"),
        QStringLiteral("OpenGL"),
    };
    std::cout << "qt.runtime.appLocal.expectedLibrary.count: " << modules.size() << '\n';
    for (int index = 0; index < modules.size(); ++index) {
        const QString module = modules.at(index);
        const QString fileName = qtRuntimeLibraryFileName(module);
        const QString path = appDir + '/' + fileName;
        const std::string prefix = "qt.runtime.appLocal.expectedLibrary." + std::to_string(index);
        printText(prefix + ".module", module);
        printText(prefix + ".fileName", fileName);
        printText(prefix + ".path", path);
        printFilesystemPathStatus(prefix, path);
    }

    const QStringList pluginBaseNames = {
        QStringLiteral("qwindows"),
        QStringLiteral("qwayland"),
        QStringLiteral("qwayland-generic"),
        QStringLiteral("qwayland-egl"),
        QStringLiteral("qxcb"),
        QStringLiteral("qoffscreen"),
    };
    std::cout << "qt.runtime.appLocal.platformPlugin.count: " << pluginBaseNames.size() << '\n';
    for (int index = 0; index < pluginBaseNames.size(); ++index) {
        const QString baseName = pluginBaseNames.at(index);
        const QString fileName = platformPluginFileName(baseName);
        const QString path = platformPluginPath(appDir, fileName);
        const std::string prefix = "qt.runtime.appLocal.platformPlugin." + std::to_string(index);
        printText(prefix + ".baseName", baseName);
        printText(prefix + ".fileName", fileName);
        printText(prefix + ".path", path);
        printFilesystemPathStatus(prefix, path);
    }
}

QString rectText(const QRect& rect)
{
    return QString("%1x%2+%3+%4").arg(rect.width()).arg(rect.height()).arg(rect.x()).arg(rect.y());
}

qint64 scaledCoordinate(int value, qreal scale)
{
    return static_cast<qint64>(std::llround(static_cast<qreal>(value) * scale));
}

QString rectDevicePixelText(const QRect& rect, qreal devicePixelRatio)
{
    return QString("%1x%2+%3+%4")
        .arg(scaledCoordinate(rect.width(), devicePixelRatio))
        .arg(scaledCoordinate(rect.height(), devicePixelRatio))
        .arg(scaledCoordinate(rect.x(), devicePixelRatio))
        .arg(scaledCoordinate(rect.y(), devicePixelRatio));
}

QString screenOrientationText(Qt::ScreenOrientation orientation);

void printScreenCoreDiagnostics(const std::string& prefix, const QScreen* screen)
{
    if (screen == nullptr) {
        printText(prefix + "name", QStringLiteral("(none)"));
        printText(prefix + "geometry", QStringLiteral("(none)"));
        printText(prefix + "availableGeometry", QStringLiteral("(none)"));
        printText(prefix + "virtualGeometry", QStringLiteral("(none)"));
        printText(prefix + "availableVirtualGeometry", QStringLiteral("(none)"));
        printText(prefix + "geometryDevicePixels", QStringLiteral("(none)"));
        printText(prefix + "availableGeometryDevicePixels", QStringLiteral("(none)"));
        printText(prefix + "virtualGeometryDevicePixels", QStringLiteral("(none)"));
        printText(prefix + "availableVirtualGeometryDevicePixels", QStringLiteral("(none)"));
        printText(prefix + "orientation", QStringLiteral("(none)"));
        printText(prefix + "primaryOrientation", QStringLiteral("(none)"));
        printText(prefix + "nativeOrientation", QStringLiteral("(none)"));
        printText(prefix + "devicePixelRatio", QStringLiteral("(none)"));
        printText(prefix + "logicalDpi", QStringLiteral("(none)"));
        printText(prefix + "logicalDpiX", QStringLiteral("(none)"));
        printText(prefix + "logicalDpiY", QStringLiteral("(none)"));
        printText(prefix + "physicalDpi", QStringLiteral("(none)"));
        printText(prefix + "physicalDpiX", QStringLiteral("(none)"));
        printText(prefix + "physicalDpiY", QStringLiteral("(none)"));
        printText(prefix + "physicalSizeMm", QStringLiteral("(none)"));
        printText(prefix + "refreshRate", QStringLiteral("(none)"));
        std::cout << prefix << "depth: -1\n";
        printText(prefix + "manufacturer", QStringLiteral("(none)"));
        printText(prefix + "model", QStringLiteral("(none)"));
        printText(prefix + "serialNumber", QStringLiteral("(none)"));
        return;
    }

    printText(prefix + "name", screen->name());
    printText(prefix + "geometry", rectText(screen->geometry()));
    printText(prefix + "availableGeometry", rectText(screen->availableGeometry()));
    printText(prefix + "virtualGeometry", rectText(screen->virtualGeometry()));
    printText(prefix + "availableVirtualGeometry", rectText(screen->availableVirtualGeometry()));
    printText(prefix + "geometryDevicePixels", rectDevicePixelText(screen->geometry(), screen->devicePixelRatio()));
    printText(
        prefix + "availableGeometryDevicePixels",
        rectDevicePixelText(screen->availableGeometry(), screen->devicePixelRatio()));
    printText(
        prefix + "virtualGeometryDevicePixels",
        rectDevicePixelText(screen->virtualGeometry(), screen->devicePixelRatio()));
    printText(
        prefix + "availableVirtualGeometryDevicePixels",
        rectDevicePixelText(screen->availableVirtualGeometry(), screen->devicePixelRatio()));
    printText(prefix + "orientation", screenOrientationText(screen->orientation()));
    printText(prefix + "primaryOrientation", screenOrientationText(screen->primaryOrientation()));
    printText(prefix + "nativeOrientation", screenOrientationText(screen->nativeOrientation()));
    printText(prefix + "devicePixelRatio", QString::number(screen->devicePixelRatio(), 'f', 3));
    printText(prefix + "logicalDpi", QString::number(screen->logicalDotsPerInch(), 'f', 2));
    printText(prefix + "logicalDpiX", QString::number(screen->logicalDotsPerInchX(), 'f', 2));
    printText(prefix + "logicalDpiY", QString::number(screen->logicalDotsPerInchY(), 'f', 2));
    printText(prefix + "physicalDpi", QString::number(screen->physicalDotsPerInch(), 'f', 2));
    printText(prefix + "physicalDpiX", QString::number(screen->physicalDotsPerInchX(), 'f', 2));
    printText(prefix + "physicalDpiY", QString::number(screen->physicalDotsPerInchY(), 'f', 2));
    printText(
        prefix + "physicalSizeMm",
        QString("%1x%2")
            .arg(screen->physicalSize().width(), 0, 'f', 1)
            .arg(screen->physicalSize().height(), 0, 'f', 1));
    printText(prefix + "refreshRate", QString::number(screen->refreshRate(), 'f', 2));
    std::cout << prefix << "depth: " << screen->depth() << '\n';
    printText(prefix + "manufacturer", screen->manufacturer());
    printText(prefix + "model", screen->model());
    printText(prefix + "serialNumber", screen->serialNumber());
}

QString highDpiPolicyText(Qt::HighDpiScaleFactorRoundingPolicy policy)
{
    switch (policy) {
    case Qt::HighDpiScaleFactorRoundingPolicy::Unset:
        return "Unset";
    case Qt::HighDpiScaleFactorRoundingPolicy::Round:
        return "Round";
    case Qt::HighDpiScaleFactorRoundingPolicy::Ceil:
        return "Ceil";
    case Qt::HighDpiScaleFactorRoundingPolicy::Floor:
        return "Floor";
    case Qt::HighDpiScaleFactorRoundingPolicy::RoundPreferFloor:
        return "RoundPreferFloor";
    case Qt::HighDpiScaleFactorRoundingPolicy::PassThrough:
        return "PassThrough";
    }
    return QString::number(static_cast<int>(policy));
}

QString screenOrientationText(Qt::ScreenOrientation orientation)
{
    switch (orientation) {
    case Qt::PrimaryOrientation:
        return "Primary";
    case Qt::PortraitOrientation:
        return "Portrait";
    case Qt::LandscapeOrientation:
        return "Landscape";
    case Qt::InvertedPortraitOrientation:
        return "InvertedPortrait";
    case Qt::InvertedLandscapeOrientation:
        return "InvertedLandscape";
    }
    return QString::number(static_cast<int>(orientation));
}

QString quickGraphicsApiText(QSGRendererInterface::GraphicsApi api)
{
    switch (api) {
    case QSGRendererInterface::Unknown:
        return "Unknown";
    case QSGRendererInterface::Software:
        return "Software";
    case QSGRendererInterface::OpenVG:
        return "OpenVG";
    case QSGRendererInterface::OpenGL:
        return "OpenGL";
    case QSGRendererInterface::Direct3D11:
        return "Direct3D11";
    case QSGRendererInterface::Vulkan:
        return "Vulkan";
    case QSGRendererInterface::Metal:
        return "Metal";
    case QSGRendererInterface::Null:
        return "Null";
    case QSGRendererInterface::Direct3D12:
        return "Direct3D12";
    }
    return QString::number(static_cast<int>(api));
}

QString quickShaderTypeText(QSGRendererInterface::ShaderType shaderType)
{
    switch (shaderType) {
    case QSGRendererInterface::UnknownShadingLanguage:
        return "Unknown";
    case QSGRendererInterface::GLSL:
        return "GLSL";
    case QSGRendererInterface::HLSL:
        return "HLSL";
    case QSGRendererInterface::RhiShader:
        return "RhiShader";
    }
    return QString::number(static_cast<int>(shaderType));
}

QString shaderCompilationTypesText(QSGRendererInterface::ShaderCompilationTypes types)
{
    QStringList values;
    if (types.testFlag(QSGRendererInterface::RuntimeCompilation)) {
        values.push_back("RuntimeCompilation");
    }
    if (types.testFlag(QSGRendererInterface::OfflineCompilation)) {
        values.push_back("OfflineCompilation");
    }
    return values.empty() ? QStringLiteral("(none)") : values.join('|');
}

QString shaderSourceTypesText(QSGRendererInterface::ShaderSourceTypes types)
{
    QStringList values;
    if (types.testFlag(QSGRendererInterface::ShaderSourceString)) {
        values.push_back("ShaderSourceString");
    }
    if (types.testFlag(QSGRendererInterface::ShaderSourceFile)) {
        values.push_back("ShaderSourceFile");
    }
    if (types.testFlag(QSGRendererInterface::ShaderByteCode)) {
        values.push_back("ShaderByteCode");
    }
    return values.empty() ? QStringLiteral("(none)") : values.join('|');
}

QString openGlModuleTypeText(QOpenGLContext::OpenGLModuleType type)
{
    switch (type) {
    case QOpenGLContext::LibGL:
        return QStringLiteral("LibGL");
    case QOpenGLContext::LibGLES:
        return QStringLiteral("LibGLES");
    }
    return QString("OpenGLModuleType(%1)").arg(static_cast<int>(type));
}

QString surfaceRenderableTypeText(QSurfaceFormat::RenderableType type)
{
    switch (type) {
    case QSurfaceFormat::DefaultRenderableType:
        return QStringLiteral("DefaultRenderableType");
    case QSurfaceFormat::OpenGL:
        return QStringLiteral("OpenGL");
    case QSurfaceFormat::OpenGLES:
        return QStringLiteral("OpenGLES");
    case QSurfaceFormat::OpenVG:
        return QStringLiteral("OpenVG");
    }
    return QString("RenderableType(%1)").arg(static_cast<int>(type));
}

QString surfaceProfileText(QSurfaceFormat::OpenGLContextProfile profile)
{
    switch (profile) {
    case QSurfaceFormat::NoProfile:
        return QStringLiteral("NoProfile");
    case QSurfaceFormat::CoreProfile:
        return QStringLiteral("CoreProfile");
    case QSurfaceFormat::CompatibilityProfile:
        return QStringLiteral("CompatibilityProfile");
    }
    return QString("OpenGLContextProfile(%1)").arg(static_cast<int>(profile));
}

QString openGlString(QOpenGLFunctions* functions, GLenum name)
{
    if (functions == nullptr) {
        return QStringLiteral("(unavailable)");
    }
    const GLubyte* value = functions->glGetString(name);
    if (value == nullptr) {
        return QStringLiteral("(unavailable)");
    }
    return QString::fromLatin1(reinterpret_cast<const char*>(value));
}

void printOpenGlDiagnostics()
{
    printText("qt.opengl.moduleType", openGlModuleTypeText(QOpenGLContext::openGLModuleType()));

    QOpenGLContext context;
    context.setFormat(QSurfaceFormat::defaultFormat());
    const bool contextCreated = context.create();
    printBool("qt.opengl.context.created", contextCreated);
    printBool("qt.opengl.context.valid", context.isValid());

    QOffscreenSurface surface;
    if (contextCreated) {
        surface.setFormat(context.format());
    }
    surface.create();
    printBool("qt.opengl.surface.valid", surface.isValid());

    const bool makeCurrentSucceeded = contextCreated && surface.isValid() && context.makeCurrent(&surface);
    printBool("qt.opengl.context.makeCurrentSucceeded", makeCurrentSucceeded);

    const QSurfaceFormat format = context.format();
    printText("qt.opengl.context.format.renderableType", surfaceRenderableTypeText(format.renderableType()));
    std::cout << "qt.opengl.context.format.majorVersion: " << format.majorVersion() << '\n';
    std::cout << "qt.opengl.context.format.minorVersion: " << format.minorVersion() << '\n';
    printText("qt.opengl.context.format.profile", surfaceProfileText(format.profile()));
    std::cout << "qt.opengl.context.format.depthBufferSize: " << format.depthBufferSize() << '\n';
    std::cout << "qt.opengl.context.format.stencilBufferSize: " << format.stencilBufferSize() << '\n';
    std::cout << "qt.opengl.context.format.samples: " << format.samples() << '\n';

    if (!makeCurrentSucceeded) {
        printBool("qt.opengl.functions.initialized", false);
        printText("qt.opengl.vendor", QStringLiteral("(unavailable)"));
        printText("qt.opengl.renderer", QStringLiteral("(unavailable)"));
        printText("qt.opengl.version", QStringLiteral("(unavailable)"));
        printText("qt.opengl.shadingLanguageVersion", QStringLiteral("(unavailable)"));
        return;
    }

    QOpenGLFunctions* functions = context.functions();
    const bool functionsInitialized = functions != nullptr;
    if (functions != nullptr) {
        functions->initializeOpenGLFunctions();
    }
    printBool("qt.opengl.functions.initialized", functionsInitialized);
    printText("qt.opengl.vendor", functionsInitialized ? openGlString(functions, GL_VENDOR) : QStringLiteral("(unavailable)"));
    printText(
        "qt.opengl.renderer",
        functionsInitialized ? openGlString(functions, GL_RENDERER) : QStringLiteral("(unavailable)"));
    printText("qt.opengl.version", functionsInitialized ? openGlString(functions, GL_VERSION) : QStringLiteral("(unavailable)"));
    printText(
        "qt.opengl.shadingLanguageVersion",
        functionsInitialized ? openGlString(functions, GL_SHADING_LANGUAGE_VERSION) : QStringLiteral("(unavailable)"));
    context.doneCurrent();
}

struct PathEnvironmentStatus {
    QByteArray value;
    QFileInfo info;
    bool isSet = false;
    bool hasText = false;
    bool exists = false;
    bool isFile = false;
    bool readable = false;
    bool writable = false;
    bool executable = false;
    bool usable = false;
};

PathEnvironmentStatus inspectPathEnvironment(const char* environmentName, bool requireExecutable)
{
    PathEnvironmentStatus status;
    status.value = qgetenv(environmentName);
    status.isSet = qEnvironmentVariableIsSet(environmentName);
    status.hasText = std::ranges::any_of(status.value, [](unsigned char ch) {
        return std::isspace(ch) == 0;
    });
    status.info = QFileInfo(QString::fromLocal8Bit(status.value));
    status.exists = status.hasText && status.info.exists();
    status.isFile = status.exists && status.info.isFile();
    status.readable = status.isFile && status.info.isReadable();
    status.writable = status.isFile && status.info.isWritable();
    status.executable = status.isFile && status.info.isExecutable();
    status.usable = status.hasText && status.readable && (!requireExecutable || status.executable);
    return status;
}

void printPathEnvironmentStatus(
    const std::string& prefix,
    const char* environmentName,
    bool requireExecutable,
    const PathEnvironmentStatus* knownStatus = nullptr)
{
    const PathEnvironmentStatus inspected = knownStatus == nullptr ? inspectPathEnvironment(environmentName, requireExecutable)
                                                                   : PathEnvironmentStatus{};
    const PathEnvironmentStatus& status = knownStatus == nullptr ? inspected : *knownStatus;
    printText(prefix + ".value", status.isSet ? status.value : QByteArray("(unset)"));

    printBool(prefix + ".hasText", status.hasText);
    printText(
        prefix + ".absolutePath",
        !status.isSet ? QStringLiteral("(unset)")
                      : (!status.hasText ? QStringLiteral("(blank)") : status.info.absoluteFilePath()));
    printText(prefix + ".canonicalPath", status.exists ? status.info.canonicalFilePath() : QStringLiteral("(missing)"));
    printBool(prefix + ".exists", status.exists);
    printBool(prefix + ".file", status.isFile);
    printBool(prefix + ".readable", status.readable);
    printBool(prefix + ".writable", status.writable);
    std::cout << prefix << ".size: " << (status.isFile ? status.info.size() : -1) << '\n';
    printText(
        prefix + ".lastModifiedUtc",
        status.isFile ? status.info.lastModified().toUTC().toString(Qt::ISODateWithMs) : QStringLiteral("(missing)"));
    if (requireExecutable) {
        printBool(prefix + ".executable", status.executable);
    }
}

void appendPathEnvironmentReadinessName(
    QStringList& missing,
    QStringList& invalid,
    const QString& name,
    const PathEnvironmentStatus& status)
{
    if (!status.isSet || !status.hasText) {
        missing.push_back(name);
    } else if (!status.usable) {
        invalid.push_back(name);
    }
}

struct KataGoEnvironmentReadiness {
    bool complete = false;
    bool ready = false;
    QString status;
    QStringList missing;
    QStringList invalid;
};

KataGoEnvironmentReadiness inspectKataGoEnvironmentReadiness()
{
    const PathEnvironmentStatus executable = inspectPathEnvironment("LIZZIE_KATAGO_EXECUTABLE", true);
    const PathEnvironmentStatus model = inspectPathEnvironment("LIZZIE_KATAGO_MODEL", false);
    const PathEnvironmentStatus analysisConfig = inspectPathEnvironment("LIZZIE_KATAGO_ANALYSIS_CONFIG", false);
    const PathEnvironmentStatus gtpConfig = inspectPathEnvironment("LIZZIE_KATAGO_GTP_CONFIG", false);

    KataGoEnvironmentReadiness readiness;
    appendPathEnvironmentReadinessName(readiness.missing, readiness.invalid, QStringLiteral("executable"), executable);
    appendPathEnvironmentReadinessName(readiness.missing, readiness.invalid, QStringLiteral("model"), model);
    appendPathEnvironmentReadinessName(
        readiness.missing,
        readiness.invalid,
        QStringLiteral("analysisConfig"),
        analysisConfig);
    appendPathEnvironmentReadinessName(readiness.missing, readiness.invalid, QStringLiteral("gtpConfig"), gtpConfig);

    readiness.complete = readiness.missing.empty();
    readiness.ready = readiness.complete && readiness.invalid.empty();
    readiness.status =
        readiness.ready ? QStringLiteral("ready")
                        : (!readiness.complete ? QStringLiteral("missing") : QStringLiteral("invalid"));
    return readiness;
}

void printKataGoEnvironmentReadiness()
{
    const KataGoEnvironmentReadiness readiness = inspectKataGoEnvironmentReadiness();
    printBool("katago.env.complete", readiness.complete);
    printBool("katago.env.ready", readiness.ready);
    printText("katago.env.status", readiness.status);
    printStringList("katago.env.missing", readiness.missing);
    printStringList("katago.env.invalid", readiness.invalid);
}

void printPathListEnvironmentStatus(const std::string& prefix, const char* environmentName)
{
    const QByteArray value = qgetenv(environmentName);
    const bool isSet = qEnvironmentVariableIsSet(environmentName);
    printText(prefix + ".value", isSet ? (value.isEmpty() ? QByteArray("(empty)") : value) : QByteArray("(unset)"));

    const QStringList paths =
        isSet ? QString::fromLocal8Bit(value).split(QDir::listSeparator(), Qt::KeepEmptyParts) : QStringList();
    std::cout << prefix << ".count: " << paths.size() << '\n';
    for (int index = 0; index < paths.size(); ++index) {
        const std::string entryPrefix = prefix + "." + std::to_string(index);
        printText(entryPrefix + ".value", paths.at(index));
        printFilesystemPathStatus(entryPrefix, paths.at(index));
    }
}

void printQtInstallPathStatus(const std::string& key, QLibraryInfo::LibraryPath path)
{
    const QString value = QLibraryInfo::path(path);
    printText(key, value);
    printFilesystemPathStatus(key, value);
}

void printQtBuildRuntimeDiagnostics()
{
    const QString buildVersion = QString::fromLatin1(QT_VERSION_STR);
    const QString runtimeVersion = QString::fromLatin1(qVersion());
    printText("qt.buildVersion", buildVersion);
    printText("qt.runtimeVersion", runtimeVersion);
    printBool("qt.version.matchesBuild", buildVersion == runtimeVersion);
    printQtInstallPathStatus("qt.install.prefixPath", QLibraryInfo::PrefixPath);
    printQtInstallPathStatus("qt.install.binariesPath", QLibraryInfo::BinariesPath);
    printQtInstallPathStatus("qt.install.librariesPath", QLibraryInfo::LibrariesPath);
    printQtInstallPathStatus("qt.install.libraryExecutablesPath", QLibraryInfo::LibraryExecutablesPath);
    printQtInstallPathStatus("qt.install.pluginsPath", QLibraryInfo::PluginsPath);
    printQtInstallPathStatus("qt.install.qmlImportsPath", QLibraryInfo::QmlImportsPath);
    printQtInstallPathStatus("qt.install.archDataPath", QLibraryInfo::ArchDataPath);
    printQtInstallPathStatus("qt.install.dataPath", QLibraryInfo::DataPath);
    printQtInstallPathStatus("qt.install.translationsPath", QLibraryInfo::TranslationsPath);
}

QString targetOsFamily()
{
#if defined(Q_OS_WIN)
    return QStringLiteral("windows");
#elif defined(Q_OS_LINUX)
    return QStringLiteral("linux");
#elif defined(Q_OS_MACOS)
    return QStringLiteral("macos");
#else
    const QString kernel = QSysInfo::kernelType().toLower();
    return kernel.isEmpty() ? QStringLiteral("other") : kernel;
#endif
}

bool environmentIsSet(const char* environmentName)
{
    return !qgetenv(environmentName).isEmpty();
}

bool environmentTruthy(const char* environmentName)
{
    const QString value = QString::fromLocal8Bit(qgetenv(environmentName)).trimmed().toLower();
    return !value.isEmpty() && value != "0" && value != "false" && value != "no";
}

bool environmentContains(const char* environmentName, const QString& needle)
{
    return QString::fromLocal8Bit(qgetenv(environmentName)).contains(needle, Qt::CaseInsensitive);
}

bool environmentContainsAny(const char* environmentName, const QStringList& needles)
{
    const QString value = QString::fromLocal8Bit(qgetenv(environmentName));
    for (const QString& needle : needles) {
        if (value.contains(needle, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

bool kdeSessionDetected()
{
    return environmentTruthy("KDE_FULL_SESSION") ||
        environmentIsSet("KDE_SESSION_VERSION") ||
        environmentContains("XDG_CURRENT_DESKTOP", QStringLiteral("kde")) ||
        environmentContains("XDG_SESSION_DESKTOP", QStringLiteral("kde")) ||
        environmentContains("DESKTOP_SESSION", QStringLiteral("kde"));
}

bool qtWaylandPlatformDetected(const QString& platformName)
{
    return platformName.startsWith(QStringLiteral("wayland"), Qt::CaseInsensitive);
}

bool waylandSessionDetected()
{
    return environmentContains("XDG_SESSION_TYPE", QStringLiteral("wayland")) ||
        environmentIsSet("WAYLAND_DISPLAY");
}

QStringList nvidiaEnvironmentHintSources()
{
    const char* explicitHintNames[] = {
        "__GLX_VENDOR_LIBRARY_NAME",
        "__NV_PRIME_RENDER_OFFLOAD",
        "__NV_PRIME_RENDER_OFFLOAD_PROVIDER",
        "__VK_LAYER_NV_optimus",
        "NVIDIA_VISIBLE_DEVICES",
        "NVIDIA_DRIVER_CAPABILITIES",
        "CUDA_VISIBLE_DEVICES",
        "CUDA_DEVICE_ORDER",
    };

    QStringList sources;
    for (const char* name : explicitHintNames) {
        if (environmentIsSet(name)) {
            sources.push_back(QString::fromLatin1(name));
        }
    }
    const QStringList gpuNeedles = {
        QStringLiteral("nvidia"),
        QStringLiteral("cuda"),
        QStringLiteral("tensorrt"),
    };
    const char* valueHintNames[] = {
        "GBM_BACKEND",
        "MESA_LOADER_DRIVER_OVERRIDE",
        "LIBVA_DRIVER_NAME",
        "__EGL_VENDOR_LIBRARY_FILENAMES",
        "VK_ICD_FILENAMES",
        "VK_DRIVER_FILES",
        "VK_INSTANCE_LAYERS",
    };
    for (const char* name : valueHintNames) {
        const QString source = QString::fromLatin1(name);
        if (!sources.contains(source) && environmentContainsAny(name, gpuNeedles)) {
            sources.push_back(source);
        }
    }
    return sources;
}

bool screenIsAtLeast4K(const QScreen* screen)
{
    if (screen == nullptr) {
        return false;
    }
    const QRect geometry = screen->geometry();
    const qreal devicePixelRatio = screen->devicePixelRatio();
    const auto absoluteCoordinate = [](qint64 value) {
        return value < 0 ? -value : value;
    };
    const qint64 width = absoluteCoordinate(scaledCoordinate(geometry.width(), devicePixelRatio));
    const qint64 height = absoluteCoordinate(scaledCoordinate(geometry.height(), devicePixelRatio));
    return std::max(width, height) >= 3840 && std::min(width, height) >= 2160;
}

bool anyScreenIsAtLeast4K(const QList<QScreen*>& screens)
{
    return std::ranges::any_of(screens, [](const QScreen* screen) {
        return screenIsAtLeast4K(screen);
    });
}

bool anyScreenScaleAtLeast(const QList<QScreen*>& screens, qreal minimumScale)
{
    return std::ranges::any_of(screens, [minimumScale](const QScreen* screen) {
        return screen != nullptr && screen->devicePixelRatio() >= minimumScale;
    });
}

bool screenMeetsTargetDisplayCandidate(const QScreen* screen)
{
    return screenIsAtLeast4K(screen) && screen != nullptr && screen->devicePixelRatio() >= 1.5;
}

bool anyScreenMeetsTargetDisplayCandidate(const QList<QScreen*>& screens)
{
    return std::ranges::any_of(screens, [](const QScreen* screen) {
        return screenMeetsTargetDisplayCandidate(screen);
    });
}

QString planAcceptanceStatus(bool targetPlatformCandidate, bool kataGoReady, bool targetDisplayCandidate)
{
    if (targetPlatformCandidate && kataGoReady && targetDisplayCandidate) {
        return QStringLiteral("ready-for-target-manual-verification");
    }
    if (targetPlatformCandidate && kataGoReady) {
        return QStringLiteral("target-and-katago-ready-needs-display-inspection");
    }
    if (targetPlatformCandidate) {
        return QStringLiteral("target-machine-needs-katago-env");
    }
    if (kataGoReady) {
        return QStringLiteral("katago-env-ready-needs-target-machine");
    }
    return QStringLiteral("needs-target-machine-and-katago-env");
}

QString planConfiguredAcceptanceStatus(bool targetPlatformCandidate, bool configuredKataGoReady, bool targetDisplayCandidate)
{
    if (targetPlatformCandidate && configuredKataGoReady && targetDisplayCandidate) {
        return QStringLiteral("ready-for-target-manual-verification");
    }
    if (targetPlatformCandidate && configuredKataGoReady) {
        return QStringLiteral("target-and-katago-config-ready-needs-display-inspection");
    }
    if (targetPlatformCandidate) {
        return QStringLiteral("target-machine-needs-katago-config");
    }
    if (configuredKataGoReady) {
        return QStringLiteral("katago-config-ready-needs-target-machine");
    }
    return QStringLiteral("needs-target-machine-and-katago-config");
}

QString configuredSourceText(bool environmentReady, bool savedReady)
{
    if (environmentReady && savedReady) {
        return QStringLiteral("env-and-saved");
    }
    if (environmentReady) {
        return QStringLiteral("env");
    }
    if (savedReady) {
        return QStringLiteral("saved");
    }
    return QStringLiteral("none");
}

QStringList linuxKdeWaylandNvidiaBlockers(
    const QString& osFamily,
    bool kdeSession,
    bool waylandSession,
    bool qtWaylandPlatform,
    bool nvidiaHintPresent)
{
    QStringList blockers;
    if (osFamily != "linux") {
        blockers.push_back(QStringLiteral("linuxOs"));
    }
    if (!kdeSession) {
        blockers.push_back(QStringLiteral("kdeSession"));
    }
    if (!waylandSession) {
        blockers.push_back(QStringLiteral("waylandSession"));
    }
    if (!qtWaylandPlatform) {
        blockers.push_back(QStringLiteral("qtWaylandPlatform"));
    }
    if (!nvidiaHintPresent) {
        blockers.push_back(QStringLiteral("nvidiaEnvironmentHint"));
    }
    return blockers;
}

QStringList windowsNvidiaBlockers(
    const QString& osFamily,
    bool windowsPlatformPlugin,
    bool nvidiaHintPresent)
{
    QStringList blockers;
    if (osFamily != "windows") {
        blockers.push_back(QStringLiteral("windowsOs"));
    }
    if (!windowsPlatformPlugin) {
        blockers.push_back(QStringLiteral("windowsPlatformPlugin"));
    }
    if (!nvidiaHintPresent) {
        blockers.push_back(QStringLiteral("nvidiaEnvironmentHint"));
    }
    return blockers;
}

QStringList firstReleaseNvidiaPlatformBlockers(bool linuxNvidiaCandidate, bool windowsNvidiaCandidate)
{
    if (linuxNvidiaCandidate || windowsNvidiaCandidate) {
        return {};
    }
    return {
        QStringLiteral("linuxKdeWaylandNvidia"),
        QStringLiteral("windowsNvidia"),
    };
}

QStringList targetDisplayBlockers(
    const QList<QScreen*>& screens,
    bool anyDisplay4K,
    bool anyHighDpiScale,
    bool anyTargetDisplayCandidate)
{
    QStringList blockers;
    if (screens.isEmpty()) {
        blockers.push_back(QStringLiteral("screen"));
    }
    if (!anyDisplay4K) {
        blockers.push_back(QStringLiteral("display4K"));
    }
    if (!anyHighDpiScale) {
        blockers.push_back(QStringLiteral("highDpiScale"));
    }
    if (anyDisplay4K && anyHighDpiScale && !anyTargetDisplayCandidate) {
        blockers.push_back(QStringLiteral("sameScreen4KHighDpi"));
    }
    return blockers;
}

QStringList multiDisplayBlockers(const QList<QScreen*>& screens)
{
    if (screens.size() > 1) {
        return {};
    }
    return {QStringLiteral("multiDisplay")};
}

QStringList manualVerificationBlockers(
    bool targetPlatformCandidate,
    bool configuredKataGoReady,
    bool targetDisplayCandidate,
    const QString& kataGoConfigBlocker)
{
    QStringList blockers;
    if (!targetPlatformCandidate) {
        blockers.push_back(QStringLiteral("targetPlatform"));
    }
    if (!configuredKataGoReady) {
        blockers.push_back(kataGoConfigBlocker);
    }
    if (!targetDisplayCandidate) {
        blockers.push_back(QStringLiteral("targetDisplay"));
    }
    return blockers;
}

QStringList dualEngineManualVerificationBlockers(
    bool targetPlatformCandidate,
    bool configuredMainReady,
    bool configuredCompareReady,
    bool targetDisplayCandidate)
{
    QStringList blockers;
    if (!targetPlatformCandidate) {
        blockers.push_back(QStringLiteral("targetPlatform"));
    }
    if (!configuredMainReady) {
        blockers.push_back(QStringLiteral("mainKataGoConfig"));
    }
    if (!configuredCompareReady) {
        blockers.push_back(QStringLiteral("compareKataGoConfig"));
    }
    if (!targetDisplayCandidate) {
        blockers.push_back(QStringLiteral("targetDisplay"));
    }
    return blockers;
}

QStringList finalAcceptanceBlockers(
    bool targetPlatformCandidate,
    bool configuredMainGtpReady,
    bool configuredMainAnalysisReady,
    bool configuredCompareGtpReady,
    bool configuredCompareAnalysisReady,
    bool targetDisplayCandidate,
    bool multiDisplayCandidate,
    bool externalTargetVerificationAccepted,
    bool manualRawEngineComparisonAccepted,
    bool manualUiInspectionAccepted,
    bool targetAcceptanceChecklistReady,
    bool targetAcceptanceMetadataReady,
    bool targetAcceptanceEvidenceReady,
    bool targetAcceptanceEvidenceSha256Ready,
    bool targetAcceptanceRecordTimestampReady,
    bool targetAcceptanceEvidenceTimestampReady,
    bool screenshotEvidence4KReady)
{
    QStringList blockers;
    if (!targetPlatformCandidate) {
        blockers.push_back(QStringLiteral("targetPlatform"));
    }
    if (!configuredMainGtpReady) {
        blockers.push_back(QStringLiteral("mainRealtimeGtpConfig"));
    }
    if (!configuredMainAnalysisReady) {
        blockers.push_back(QStringLiteral("mainBatchAnalysisConfig"));
    }
    if (!configuredCompareGtpReady) {
        blockers.push_back(QStringLiteral("compareRealtimeGtpConfig"));
    }
    if (!configuredCompareAnalysisReady) {
        blockers.push_back(QStringLiteral("compareBatchAnalysisConfig"));
    }
    if (!targetDisplayCandidate) {
        blockers.push_back(QStringLiteral("targetDisplay"));
    }
    if (!multiDisplayCandidate) {
        blockers.push_back(QStringLiteral("multiDisplay"));
    }
    if (!externalTargetVerificationAccepted) {
        blockers.push_back(QStringLiteral("externalTargetVerification"));
    }
    if (!manualRawEngineComparisonAccepted) {
        blockers.push_back(QStringLiteral("manualRawEngineComparison"));
    }
    if (!manualUiInspectionAccepted) {
        blockers.push_back(QStringLiteral("manualUiInspection"));
    }
    if (externalTargetVerificationAccepted &&
        manualRawEngineComparisonAccepted &&
        manualUiInspectionAccepted &&
        !targetAcceptanceChecklistReady) {
        blockers.push_back(QStringLiteral("acceptanceChecklist"));
    }
    if (externalTargetVerificationAccepted &&
        manualRawEngineComparisonAccepted &&
        manualUiInspectionAccepted &&
        !targetAcceptanceMetadataReady) {
        blockers.push_back(QStringLiteral("acceptanceMetadata"));
    }
    if (externalTargetVerificationAccepted &&
        manualRawEngineComparisonAccepted &&
        manualUiInspectionAccepted &&
        !targetAcceptanceEvidenceReady) {
        blockers.push_back(QStringLiteral("acceptanceEvidence"));
    }
    if (externalTargetVerificationAccepted &&
        manualRawEngineComparisonAccepted &&
        manualUiInspectionAccepted &&
        !targetAcceptanceEvidenceSha256Ready) {
        blockers.push_back(QStringLiteral("acceptanceEvidenceSha256"));
    }
    if (externalTargetVerificationAccepted &&
        manualRawEngineComparisonAccepted &&
        manualUiInspectionAccepted &&
        targetAcceptanceMetadataReady &&
        !targetAcceptanceRecordTimestampReady) {
        blockers.push_back(QStringLiteral("acceptanceRecordTimestamp"));
    }
    if (externalTargetVerificationAccepted &&
        manualRawEngineComparisonAccepted &&
        manualUiInspectionAccepted &&
        targetAcceptanceMetadataReady &&
        targetAcceptanceEvidenceReady &&
        !targetAcceptanceEvidenceTimestampReady) {
        blockers.push_back(QStringLiteral("acceptanceEvidenceTimestamp"));
    }
    if (externalTargetVerificationAccepted &&
        manualRawEngineComparisonAccepted &&
        manualUiInspectionAccepted &&
        targetAcceptanceMetadataReady &&
        targetAcceptanceRecordTimestampReady &&
        targetAcceptanceEvidenceReady &&
        targetAcceptanceEvidenceTimestampReady &&
        !screenshotEvidence4KReady) {
        blockers.push_back(QStringLiteral("screenshotEvidence4K"));
    }
    return blockers;
}

bool isManualFinalAcceptanceBlocker(const QString& blocker)
{
    return blocker == QStringLiteral("externalTargetVerification") ||
        blocker == QStringLiteral("manualRawEngineComparison") ||
        blocker == QStringLiteral("manualUiInspection");
}

const char* targetAcceptanceRecordEnvironmentName()
{
    return "LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE";
}

QString manualRecordRequiredStatus()
{
    return QStringLiteral("manual-record-required");
}

const QStringList& manualAcceptancePassValues()
{
    static const QStringList values = {
        QStringLiteral("pass"),
        QStringLiteral("passed"),
        QStringLiteral("accepted"),
        QStringLiteral("true"),
        QStringLiteral("yes"),
    };
    return values;
}

const QStringList& manualAcceptanceFailValues()
{
    static const QStringList values = {
        QStringLiteral("fail"),
        QStringLiteral("failed"),
        QStringLiteral("rejected"),
        QStringLiteral("false"),
        QStringLiteral("no"),
    };
    return values;
}

const QStringList& targetAcceptanceMetadataRequiredKeys()
{
    static const QStringList keys = {
        QStringLiteral("completedUtc"),
        QStringLiteral("appVersion"),
        QStringLiteral("appExecutableSha256"),
        QStringLiteral("osPrettyName"),
        QStringLiteral("osKernelType"),
        QStringLiteral("osKernelVersion"),
        QStringLiteral("qtRuntimeVersion"),
        QStringLiteral("qtBuildAbi"),
        QStringLiteral("cpuArchitecture"),
        QStringLiteral("reviewer"),
        QStringLiteral("targetMachine"),
        QStringLiteral("gpuDriver"),
        QStringLiteral("kataGoVersion"),
    };
    return keys;
}

const QStringList& targetAcceptanceMetadataExactMatchKeys()
{
    static const QStringList keys = {
        QStringLiteral("appVersion"),
        QStringLiteral("appExecutableSha256"),
        QStringLiteral("osPrettyName"),
        QStringLiteral("osKernelType"),
        QStringLiteral("osKernelVersion"),
        QStringLiteral("qtRuntimeVersion"),
        QStringLiteral("qtBuildAbi"),
        QStringLiteral("cpuArchitecture"),
        QStringLiteral("targetMachine"),
    };
    return keys;
}

const QStringList& targetAcceptanceMetadataValueCheckedKeys()
{
    static const QStringList keys = {
        QStringLiteral("completedUtc"),
        QStringLiteral("reviewer"),
        QStringLiteral("gpuDriver"),
        QStringLiteral("kataGoVersion"),
    };
    return keys;
}

QString targetAcceptanceCompletedUtcRequirement()
{
    return QStringLiteral("ISO-UTC-not-future");
}

QString normalizeManualAcceptanceResult(const QString& value)
{
    const QString normalized = value.trimmed().toLower();
    if (normalized.isEmpty()) {
        return manualRecordRequiredStatus();
    }
    if (manualAcceptancePassValues().contains(normalized)) {
        return QStringLiteral("pass");
    }
    if (manualAcceptanceFailValues().contains(normalized)) {
        return QStringLiteral("fail");
    }
    return QStringLiteral("invalid-manual-record");
}

QString manualAcceptanceSetting(QSettings* settings, const QString& key)
{
    if (settings == nullptr || !settings->contains(key)) {
        return manualRecordRequiredStatus();
    }
    return normalizeManualAcceptanceResult(settings->value(key).toString());
}

bool manualAcceptancePassed(const QString& status)
{
    return status == QStringLiteral("pass");
}

bool manualAcceptanceFailed(const QString& status)
{
    return status == QStringLiteral("fail");
}

bool manualAcceptanceInvalid(const QString& status)
{
    return status == QStringLiteral("invalid-manual-record");
}

QStringList externalTargetVerificationChecklist();
QStringList linuxKdeWaylandNvidiaVerificationChecklist();
QStringList windowsNvidiaVerificationChecklist();
QStringList physicalDisplayVerificationChecklist();
QStringList manualUiInspectionChecklist();
QStringList rawKataGoComparisonChecklist();

struct TargetAcceptanceRecords {
    bool fileSet = false;
    bool fileExists = false;
    bool fileReadable = false;
    QString fileName = QStringLiteral("(unset)");
    QString externalTargetVerification = manualRecordRequiredStatus();
    QString linuxKdeWaylandNvidia = manualRecordRequiredStatus();
    QString windowsNvidia = manualRecordRequiredStatus();
    QString physicalDisplay = manualRecordRequiredStatus();
    QString rawKataGoComparison = manualRecordRequiredStatus();
    QString manualUiInspection = manualRecordRequiredStatus();
    QString appVersion = QStringLiteral("(unset)");
    QString appExecutableSha256 = QStringLiteral("(unset)");
    QString osPrettyName = QStringLiteral("(unset)");
    QString osKernelType = QStringLiteral("(unset)");
    QString osKernelVersion = QStringLiteral("(unset)");
    QString qtRuntimeVersion = QStringLiteral("(unset)");
    QString qtBuildAbi = QStringLiteral("(unset)");
    QString cpuArchitecture = QStringLiteral("(unset)");
    QString completedUtc = QStringLiteral("(unset)");
    QString reviewer = QStringLiteral("(unset)");
    QString targetMachine = QStringLiteral("(unset)");
    QString gpuDriver = QStringLiteral("(unset)");
    QString kataGoVersion = QStringLiteral("(unset)");
    QString notes = QStringLiteral("(unset)");
    QString diagnosticsPath;
    QString targetAcceptanceReportPath;
    QString engineLogPath;
    QString rawKataGoComparisonLogPath;
    QString manualUiInspectionLogPath;
    QString externalTargetVerificationLogPath;
    QString screenshotPath;
    QString diagnosticsSha256;
    QString targetAcceptanceReportSha256;
    QString engineLogSha256;
    QString rawKataGoComparisonLogSha256;
    QString manualUiInspectionLogSha256;
    QString externalTargetVerificationLogSha256;
    QString screenshotSha256;
    QStringList passedChecklistItems;
    QStringList failedChecklistItems;
    QStringList invalidChecklistItems;
};

QString recordMetadataSetting(QSettings& settings, const QString& key)
{
    const QString value = settings.value(key).toString().trimmed();
    return value.isEmpty() ? QStringLiteral("(unset)") : value;
}

QString recordPathSetting(QSettings& settings, const QString& key)
{
    return settings.value(key).toString().trimmed();
}

QString recordPathDisplayText(const QString& path)
{
    return hasNonWhitespaceText(path) ? path : QStringLiteral("(unset)");
}

QString recordSha256DisplayText(const QString& sha256)
{
    return hasNonWhitespaceText(sha256) ? sha256 : QStringLiteral("(unset)");
}

QString recordSha256Setting(QSettings& settings, const QString& key)
{
    return settings.value(key).toString().trimmed().toLower();
}

QString sha256ForReadableNonEmptyFile(const QString& path);
bool isSha256HexDigest(const QString& digest);

bool targetAcceptanceMetadataValueReady(const QString& value)
{
    return hasNonWhitespaceText(value) && value != QStringLiteral("(unset)");
}

QString expectedTargetAcceptanceAppVersion()
{
    return QString::fromLatin1(LIZZIEYZY_QT_VERSION);
}

QString targetAcceptanceAppVersionStatus(const QString& value)
{
    if (!targetAcceptanceMetadataValueReady(value)) {
        return QStringLiteral("missing");
    }
    return value == expectedTargetAcceptanceAppVersion() ? QStringLiteral("match") : QStringLiteral("mismatch");
}

QString expectedTargetAcceptanceAppExecutableSha256()
{
    return sha256ForReadableNonEmptyFile(QCoreApplication::applicationFilePath());
}

QString targetAcceptanceAppExecutableSha256Status(const QString& value)
{
    if (!targetAcceptanceMetadataValueReady(value)) {
        return QStringLiteral("missing");
    }
    if (!isSha256HexDigest(value)) {
        return QStringLiteral("invalid");
    }

    const QString currentSha256 = expectedTargetAcceptanceAppExecutableSha256();
    if (!isSha256HexDigest(currentSha256)) {
        return QStringLiteral("current-unavailable");
    }
    return value == currentSha256 ? QStringLiteral("match") : QStringLiteral("mismatch");
}

QString targetAcceptanceExactMetadataStatus(const QString& value, const QString& expected)
{
    if (!targetAcceptanceMetadataValueReady(value)) {
        return QStringLiteral("missing");
    }
    if (!hasNonWhitespaceText(expected)) {
        return QStringLiteral("current-unavailable");
    }
    return value == expected ? QStringLiteral("match") : QStringLiteral("mismatch");
}

QString targetAcceptanceOsPrettyNameStatus(const QString& value)
{
    return targetAcceptanceExactMetadataStatus(value, QSysInfo::prettyProductName());
}

QString targetAcceptanceOsKernelTypeStatus(const QString& value)
{
    return targetAcceptanceExactMetadataStatus(value, QSysInfo::kernelType());
}

QString targetAcceptanceOsKernelVersionStatus(const QString& value)
{
    return targetAcceptanceExactMetadataStatus(value, QSysInfo::kernelVersion());
}

QString targetAcceptanceQtRuntimeVersionStatus(const QString& value)
{
    return targetAcceptanceExactMetadataStatus(value, QString::fromLatin1(qVersion()));
}

QString targetAcceptanceQtBuildAbiStatus(const QString& value)
{
    return targetAcceptanceExactMetadataStatus(value, QSysInfo::buildAbi());
}

QString targetAcceptanceCpuArchitectureStatus(const QString& value)
{
    return targetAcceptanceExactMetadataStatus(value, QSysInfo::currentCpuArchitecture());
}

QString targetAcceptanceTargetMachineStatus(const QString& value)
{
    return targetAcceptanceExactMetadataStatus(value, QSysInfo::machineHostName());
}

QString targetAcceptanceGpuDriverStatus(const QString& value)
{
    if (!targetAcceptanceMetadataValueReady(value)) {
        return QStringLiteral("missing");
    }
    return value.contains(QStringLiteral("NVIDIA"), Qt::CaseInsensitive) ? QStringLiteral("match")
                                                                         : QStringLiteral("mismatch");
}

QString targetAcceptanceKataGoVersionStatus(const QString& value)
{
    if (!targetAcceptanceMetadataValueReady(value)) {
        return QStringLiteral("missing");
    }
    return value.contains(QStringLiteral("KataGo"), Qt::CaseInsensitive) ? QStringLiteral("match")
                                                                         : QStringLiteral("mismatch");
}

std::optional<QDateTime> parsedTargetAcceptanceCompletedUtc(const QString& value)
{
    if (!targetAcceptanceMetadataValueReady(value)) {
        return std::nullopt;
    }

    const QString trimmed = value.trimmed();
    if (!trimmed.endsWith(QLatin1Char('Z'), Qt::CaseInsensitive)) {
        return std::nullopt;
    }

    QDateTime completedUtc = QDateTime::fromString(trimmed, Qt::ISODateWithMs);
    if (!completedUtc.isValid()) {
        completedUtc = QDateTime::fromString(trimmed, Qt::ISODate);
    }
    if (!completedUtc.isValid() || completedUtc.offsetFromUtc() != 0) {
        return std::nullopt;
    }
    return completedUtc.toUTC();
}

QString targetAcceptanceCompletedUtcStatus(const QString& value)
{
    if (!targetAcceptanceMetadataValueReady(value)) {
        return QStringLiteral("missing");
    }

    const std::optional<QDateTime> completedUtc = parsedTargetAcceptanceCompletedUtc(value);
    if (!completedUtc.has_value()) {
        return QStringLiteral("invalid-format");
    }

    if (*completedUtc > QDateTime::currentDateTimeUtc().addSecs(5 * 60)) {
        return QStringLiteral("future");
    }

    return QStringLiteral("ready");
}

bool targetAcceptanceCompletedUtcReady(const QString& value)
{
    return targetAcceptanceCompletedUtcStatus(value) == QStringLiteral("ready");
}

bool targetAcceptanceMetadataFieldReady(const QString& name, const QString& value)
{
    if (name == QStringLiteral("completedUtc")) {
        return targetAcceptanceCompletedUtcReady(value);
    }
    if (name == QStringLiteral("appVersion")) {
        return targetAcceptanceAppVersionStatus(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("appExecutableSha256")) {
        return targetAcceptanceAppExecutableSha256Status(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("osPrettyName")) {
        return targetAcceptanceOsPrettyNameStatus(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("osKernelType")) {
        return targetAcceptanceOsKernelTypeStatus(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("osKernelVersion")) {
        return targetAcceptanceOsKernelVersionStatus(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("qtRuntimeVersion")) {
        return targetAcceptanceQtRuntimeVersionStatus(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("qtBuildAbi")) {
        return targetAcceptanceQtBuildAbiStatus(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("cpuArchitecture")) {
        return targetAcceptanceCpuArchitectureStatus(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("targetMachine")) {
        return targetAcceptanceTargetMachineStatus(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("gpuDriver")) {
        return targetAcceptanceGpuDriverStatus(value) == QStringLiteral("match");
    }
    if (name == QStringLiteral("kataGoVersion")) {
        return targetAcceptanceKataGoVersionStatus(value) == QStringLiteral("match");
    }
    return targetAcceptanceMetadataValueReady(value);
}

void appendTargetAcceptanceMetadataBlocker(QStringList& blockers, const QString& name, const QString& value)
{
    if (!targetAcceptanceMetadataFieldReady(name, value)) {
        blockers.push_back(name);
    }
}

QStringList targetAcceptanceMetadataBlockers(const TargetAcceptanceRecords& records)
{
    QStringList blockers;
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("completedUtc"), records.completedUtc);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("appVersion"), records.appVersion);
    appendTargetAcceptanceMetadataBlocker(
        blockers,
        QStringLiteral("appExecutableSha256"),
        records.appExecutableSha256);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("osPrettyName"), records.osPrettyName);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("osKernelType"), records.osKernelType);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("osKernelVersion"), records.osKernelVersion);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("qtRuntimeVersion"), records.qtRuntimeVersion);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("qtBuildAbi"), records.qtBuildAbi);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("cpuArchitecture"), records.cpuArchitecture);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("reviewer"), records.reviewer);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("targetMachine"), records.targetMachine);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("gpuDriver"), records.gpuDriver);
    appendTargetAcceptanceMetadataBlocker(blockers, QStringLiteral("kataGoVersion"), records.kataGoVersion);
    return blockers;
}

bool targetAcceptanceMetadataReady(const TargetAcceptanceRecords& records)
{
    return targetAcceptanceMetadataBlockers(records).isEmpty();
}

QString targetAcceptanceRecordFileTimestampStatus(const TargetAcceptanceRecords& records)
{
    if (!records.fileReadable) {
        return QStringLiteral("unavailable");
    }

    const QString completedUtcStatus = targetAcceptanceCompletedUtcStatus(records.completedUtc);
    const std::optional<QDateTime> completedUtc = parsedTargetAcceptanceCompletedUtc(records.completedUtc);
    if (completedUtcStatus != QStringLiteral("ready") || !completedUtc.has_value()) {
        return QStringLiteral("completedUtc-") + completedUtcStatus;
    }

    const QFileInfo info(records.fileName);
    const QDateTime lastModifiedUtc = info.lastModified().toUTC();
    if (!info.exists() || !lastModifiedUtc.isValid()) {
        return QStringLiteral("unavailable");
    }

    return lastModifiedUtc > completedUtc->addSecs(5 * 60) ? QStringLiteral("after-completed-utc")
                                                           : QStringLiteral("not-after-completed-utc");
}

QStringList targetAcceptanceRecordTimestampBlockers(const TargetAcceptanceRecords& records)
{
    if (targetAcceptanceRecordFileTimestampStatus(records) == QStringLiteral("not-after-completed-utc")) {
        return {};
    }
    return {QStringLiteral("recordFileTimestamp")};
}

bool targetAcceptanceRecordTimestampReady(const TargetAcceptanceRecords& records)
{
    return targetAcceptanceRecordTimestampBlockers(records).isEmpty();
}

QString resolvedAcceptanceEvidencePath(const TargetAcceptanceRecords& records, const QString& path)
{
    if (!hasNonWhitespaceText(path)) {
        return QString();
    }

    const QFileInfo pathInfo(path);
    if (pathInfo.isAbsolute()) {
        return path;
    }

    const QFileInfo recordInfo(records.fileName);
    if (records.fileReadable && recordInfo.exists()) {
        return QDir(recordInfo.absolutePath()).absoluteFilePath(path);
    }

    return path;
}

bool acceptanceEvidencePathReady(const TargetAcceptanceRecords& records, const QString& path)
{
    if (!hasNonWhitespaceText(path)) {
        return false;
    }
    const QFileInfo info(resolvedAcceptanceEvidencePath(records, path));
    return info.exists() && info.isFile() && info.isReadable() && info.size() > 0;
}

QString sha256ForReadableNonEmptyFile(const QString& path)
{
    const QFileInfo info(path);
    if (!hasNonWhitespaceText(path) || !info.exists() || !info.isFile() || !info.isReadable() || info.size() <= 0) {
        return QStringLiteral("(unavailable)");
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QStringLiteral("(unavailable)");
    }

    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(1024 * 1024);
        if (chunk.isEmpty() && file.error() != QFileDevice::NoError) {
            return QStringLiteral("(unavailable)");
        }
        hash.addData(chunk);
    }
    if (file.error() != QFileDevice::NoError) {
        return QStringLiteral("(unavailable)");
    }

    return QString::fromLatin1(hash.result().toHex());
}

QString acceptanceEvidenceSha256(const TargetAcceptanceRecords& records, const QString& path)
{
    if (!acceptanceEvidencePathReady(records, path)) {
        return QStringLiteral("(unavailable)");
    }
    return sha256ForReadableNonEmptyFile(resolvedAcceptanceEvidencePath(records, path));
}

bool acceptanceEvidenceContainsMarker(
    const TargetAcceptanceRecords& records,
    const QString& path,
    const QByteArray& marker)
{
    if (!acceptanceEvidencePathReady(records, path) || marker.isEmpty()) {
        return false;
    }

    QFile file(resolvedAcceptanceEvidencePath(records, path));
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray tail;
    while (!file.atEnd()) {
        const QByteArray chunk = file.read(1024 * 1024);
        if (chunk.isEmpty() && file.error() != QFileDevice::NoError) {
            return false;
        }

        const QByteArray searchable = tail + chunk;
        if (searchable.contains(marker)) {
            return true;
        }
        const qsizetype tailSize = std::max<qsizetype>(0, marker.size() - 1);
        tail = searchable.right(tailSize);
    }

    return false;
}

bool acceptanceEvidenceContainsMarkers(
    const TargetAcceptanceRecords& records,
    const QString& path,
    const QList<QByteArray>& markers)
{
    if (markers.isEmpty()) {
        return false;
    }

    return std::ranges::all_of(markers, [&](const QByteArray& marker) {
        return acceptanceEvidenceContainsMarker(records, path, marker);
    });
}

bool acceptanceEvidenceContainsAnyMarker(
    const TargetAcceptanceRecords& records,
    const QString& path,
    const QList<QByteArray>& markers)
{
    if (markers.isEmpty()) {
        return false;
    }

    return std::ranges::any_of(markers, [&](const QByteArray& marker) {
        return acceptanceEvidenceContainsMarker(records, path, marker);
    });
}

QStringList acceptanceEvidenceMissingMarkers(
    const TargetAcceptanceRecords& records,
    const QString& path,
    const QList<QByteArray>& markers)
{
    QStringList missing;
    if (!acceptanceEvidencePathReady(records, path)) {
        return missing;
    }

    for (const QByteArray& marker : markers) {
        if (!acceptanceEvidenceContainsMarker(records, path, marker)) {
            missing.push_back(QString::fromUtf8(marker));
        }
    }
    return missing;
}

QStringList acceptanceEvidenceMissingAnyMarkers(
    const TargetAcceptanceRecords& records,
    const QString& path,
    const QList<QByteArray>& markers)
{
    QStringList missing;
    if (!acceptanceEvidencePathReady(records, path) ||
        acceptanceEvidenceContainsAnyMarker(records, path, markers)) {
        return missing;
    }

    for (const QByteArray& marker : markers) {
        missing.push_back(QString::fromUtf8(marker));
    }
    return missing;
}

QString acceptanceEvidenceContentStatus(
    const TargetAcceptanceRecords& records,
    const QString& path,
    const QList<QByteArray>& markers)
{
    if (!acceptanceEvidencePathReady(records, path)) {
        return QStringLiteral("unavailable");
    }
    return acceptanceEvidenceContainsMarkers(records, path, markers) ? QStringLiteral("match")
                                                                     : QStringLiteral("missing-marker");
}

QString acceptanceEvidenceAnyContentStatus(
    const TargetAcceptanceRecords& records,
    const QString& path,
    const QList<QByteArray>& markers)
{
    if (!acceptanceEvidencePathReady(records, path)) {
        return QStringLiteral("unavailable");
    }
    return acceptanceEvidenceContainsAnyMarker(records, path, markers) ? QStringLiteral("match")
                                                                       : QStringLiteral("missing-marker");
}

bool isSha256HexDigest(const QString& digest)
{
    if (digest.size() != 64) {
        return false;
    }
    return std::ranges::all_of(digest, [](const QChar ch) {
        return (ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) ||
            (ch >= QLatin1Char('a') && ch <= QLatin1Char('f'));
    });
}

QString acceptanceEvidenceSha256Status(
    const TargetAcceptanceRecords& records,
    const QString& path,
    const QString& expectedSha256)
{
    if (!hasNonWhitespaceText(expectedSha256)) {
        return QStringLiteral("missing-expected");
    }
    if (!isSha256HexDigest(expectedSha256)) {
        return QStringLiteral("invalid-expected");
    }
    if (!acceptanceEvidencePathReady(records, path)) {
        return QStringLiteral("unavailable");
    }
    return acceptanceEvidenceSha256(records, path) == expectedSha256 ? QStringLiteral("match") : QStringLiteral("mismatch");
}

QString acceptanceEvidenceTimestampStatus(const TargetAcceptanceRecords& records, const QString& path)
{
    if (!acceptanceEvidencePathReady(records, path)) {
        return QStringLiteral("unavailable");
    }

    const QString completedUtcStatus = targetAcceptanceCompletedUtcStatus(records.completedUtc);
    const std::optional<QDateTime> completedUtc = parsedTargetAcceptanceCompletedUtc(records.completedUtc);
    if (completedUtcStatus != QStringLiteral("ready") || !completedUtc.has_value()) {
        return QStringLiteral("completedUtc-") + completedUtcStatus;
    }

    const QFileInfo info(resolvedAcceptanceEvidencePath(records, path));
    const QDateTime lastModifiedUtc = info.lastModified().toUTC();
    if (!lastModifiedUtc.isValid()) {
        return QStringLiteral("unavailable");
    }

    return lastModifiedUtc > completedUtc->addSecs(5 * 60) ? QStringLiteral("after-completed-utc")
                                                           : QStringLiteral("not-after-completed-utc");
}

struct AcceptanceImageEvidenceInfo {
    bool readable = false;
    QString format = QStringLiteral("(unavailable)");
    int width = -1;
    int height = -1;
    bool pixelsAtLeast4K = false;
    bool hasPixelVariation = false;
};

bool imageSizeAtLeast4K(const QSize& size)
{
    const int longSide = std::max(size.width(), size.height());
    const int shortSide = std::min(size.width(), size.height());
    return longSide >= 3840 && shortSide >= 2160;
}

bool imageHasPixelVariation(const QImage& image)
{
    if (image.isNull() || image.width() <= 0 || image.height() <= 0) {
        return false;
    }

    const QRgb firstPixel = image.pixel(0, 0);
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            if (image.pixel(x, y) != firstPixel) {
                return true;
            }
        }
    }
    return false;
}

AcceptanceImageEvidenceInfo inspectAcceptanceImageEvidence(const TargetAcceptanceRecords& records, const QString& path)
{
    AcceptanceImageEvidenceInfo info;
    if (!acceptanceEvidencePathReady(records, path)) {
        return info;
    }

    QImageReader reader(resolvedAcceptanceEvidencePath(records, path));
    const QByteArray format = reader.format();
    if (!format.isEmpty()) {
        info.format = QString::fromLatin1(format);
    }
    const QSize size = reader.size();
    info.readable = reader.canRead() && size.isValid();
    if (size.isValid()) {
        info.width = size.width();
        info.height = size.height();
        info.pixelsAtLeast4K = imageSizeAtLeast4K(size);
    }
    if (info.readable) {
        const QImage image = reader.read();
        info.readable = !image.isNull();
        info.hasPixelVariation = imageHasPixelVariation(image);
    }
    return info;
}

void appendAcceptanceEvidenceBlocker(
    QStringList& blockers,
    const TargetAcceptanceRecords& records,
    const QString& name,
    const QString& path)
{
    if (!acceptanceEvidencePathReady(records, path)) {
        blockers.push_back(name);
    }
}

struct AcceptanceEvidencePathEntry {
    QString blockerName;
    QString path;
};

QString acceptanceEvidenceCanonicalPath(const TargetAcceptanceRecords& records, const QString& path)
{
    if (!acceptanceEvidencePathReady(records, path)) {
        return QString();
    }
    return QFileInfo(resolvedAcceptanceEvidencePath(records, path)).canonicalFilePath();
}

void appendDuplicateAcceptanceEvidencePathBlockers(
    QStringList& blockers,
    const TargetAcceptanceRecords& records,
    const QList<AcceptanceEvidencePathEntry>& entries)
{
    QMap<QString, QString> seenCanonicalPaths;
    for (const AcceptanceEvidencePathEntry& entry : entries) {
        const QString canonicalPath = acceptanceEvidenceCanonicalPath(records, entry.path);
        if (!hasNonWhitespaceText(canonicalPath)) {
            continue;
        }

        if (seenCanonicalPaths.contains(canonicalPath)) {
            blockers.push_back(QStringLiteral("duplicateEvidencePath.") + entry.blockerName);
            continue;
        }

        seenCanonicalPaths.insert(canonicalPath, entry.blockerName);
    }
}

constexpr std::string_view diagnosticsEvidenceMarker = "LizzieYzy Qt Diagnostics";
constexpr std::string_view diagnosticsEvidenceAppVersionMarker = "app.version";
constexpr std::string_view diagnosticsEvidenceAppExecutableMarker = "app.executable";
constexpr std::string_view diagnosticsRecordFileCanonicalPathMarker = "plan.acceptance.recordFile.canonicalPath";
constexpr std::string_view diagnosticsRecordFileSha256Marker = "plan.acceptance.recordFile.sha256";
constexpr std::string_view targetAcceptanceReportEvidenceMarker = "# Target Acceptance Report";
constexpr std::string_view targetAcceptanceReportRecordFileCanonicalPathMarker =
    "plan.acceptance.recordFile.canonicalPath";
constexpr std::string_view targetAcceptanceReportRecordFileSha256Marker = "plan.acceptance.recordFile.sha256";
constexpr std::string_view acceptanceFinalStatusMarker = "plan.acceptance.finalAcceptanceStatus";
constexpr std::string_view acceptanceFinalBlockerMarker = "plan.acceptance.finalAcceptanceBlocker";
constexpr std::string_view acceptanceFinalOutstandingBlockerMarker =
    "plan.acceptance.finalAcceptanceOutstandingBlocker";
constexpr std::string_view kataGoEvidenceMarker = "KataGo";
constexpr std::string_view engineLogCudaMarker = "CUDA";
constexpr std::string_view engineLogOpenClMarker = "OpenCL";
constexpr std::string_view engineLogTensorRtMarker = "TensorRT";
constexpr std::string_view engineLogGpuMarker = "GPU";
constexpr std::string_view engineLogLowerGpuMarker = "gpu";
constexpr std::string_view engineLogBackendMarker = "backend";
constexpr std::string_view engineLogTitleBackendMarker = "Backend";
constexpr std::string_view engineLogStderrMarker = "stderr:";
constexpr std::string_view engineLogTitleStderrMarker = "Stderr:";
constexpr std::string_view rawKataGoRealtimeGtpMarker = "kata-analyze";
constexpr std::string_view rawKataGoAnalysisJsonMarker = "\"moveInfos\"";
constexpr std::string_view rawKataGoRootInfoMarker = "\"rootInfo\"";
constexpr std::string_view rawKataGoWinrateMarker = "\"winrate\"";
constexpr std::string_view rawKataGoScoreMeanMarker = "\"scoreMean\"";
constexpr std::string_view rawKataGoScoreStdevMarker = "\"scoreStdev\"";
constexpr std::string_view rawKataGoVisitsMarker = "\"visits\"";
constexpr std::string_view rawKataGoPolicyMarker = "\"policy\"";
constexpr std::string_view rawKataGoPvMarker = "\"pv\"";
constexpr std::string_view rawKataGoPvVisitsMarker = "\"pvVisits\"";
constexpr std::string_view rawKataGoOwnershipMarker = "\"ownership\"";

QByteArray acceptanceEvidenceMarkerBytes(std::string_view marker)
{
    return QByteArray(marker.data(), static_cast<qsizetype>(marker.size()));
}

void appendCurrentAppEvidenceContentMarkers(QList<QByteArray>& markers, const TargetAcceptanceRecords& records)
{
    markers.push_back(acceptanceEvidenceMarkerBytes(diagnosticsEvidenceAppVersionMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(diagnosticsEvidenceAppExecutableMarker));
    if (targetAcceptanceAppVersionStatus(records.appVersion) == QStringLiteral("match")) {
        markers.push_back(records.appVersion.toUtf8());
    }
    if (targetAcceptanceAppExecutableSha256Status(records.appExecutableSha256) == QStringLiteral("match")) {
        const QString appExecutable = QCoreApplication::applicationFilePath();
        if (hasNonWhitespaceText(appExecutable)) {
            markers.push_back(appExecutable.toUtf8());
        }
    }
}

QList<QByteArray> diagnosticsEvidenceContentMarkers(const TargetAcceptanceRecords& records)
{
    QList<QByteArray> markers;
    markers.push_back(acceptanceEvidenceMarkerBytes(diagnosticsEvidenceMarker));
    appendCurrentAppEvidenceContentMarkers(markers, records);
    markers.push_back(acceptanceEvidenceMarkerBytes(diagnosticsRecordFileCanonicalPathMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(diagnosticsRecordFileSha256Marker));
    markers.push_back(acceptanceEvidenceMarkerBytes(acceptanceFinalStatusMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(acceptanceFinalBlockerMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(acceptanceFinalOutstandingBlockerMarker));
    if (records.fileReadable) {
        const QString canonicalRecordPath = QFileInfo(records.fileName).canonicalFilePath();
        if (hasNonWhitespaceText(canonicalRecordPath)) {
            markers.push_back(canonicalRecordPath.toUtf8());
        }
    }
    return markers;
}

QList<QByteArray> targetAcceptanceReportEvidenceContentMarkers(const TargetAcceptanceRecords& records)
{
    QList<QByteArray> markers;
    markers.push_back(acceptanceEvidenceMarkerBytes(targetAcceptanceReportEvidenceMarker));
    appendCurrentAppEvidenceContentMarkers(markers, records);
    markers.push_back(acceptanceEvidenceMarkerBytes(targetAcceptanceReportRecordFileCanonicalPathMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(targetAcceptanceReportRecordFileSha256Marker));
    markers.push_back(acceptanceEvidenceMarkerBytes(acceptanceFinalStatusMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(acceptanceFinalBlockerMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(acceptanceFinalOutstandingBlockerMarker));
    if (records.fileReadable) {
        const QString canonicalRecordPath = QFileInfo(records.fileName).canonicalFilePath();
        if (hasNonWhitespaceText(canonicalRecordPath)) {
            markers.push_back(canonicalRecordPath.toUtf8());
        }
    }
    return markers;
}

QList<QByteArray> kataGoEvidenceContentMarkers()
{
    QList<QByteArray> markers;
    markers.push_back(acceptanceEvidenceMarkerBytes(kataGoEvidenceMarker));
    return markers;
}

QList<QByteArray> engineLogEvidenceContentMarkers(const TargetAcceptanceRecords& records)
{
    QList<QByteArray> markers = kataGoEvidenceContentMarkers();
    if (targetAcceptanceKataGoVersionStatus(records.kataGoVersion) == QStringLiteral("match")) {
        markers.push_back(records.kataGoVersion.toUtf8());
    }
    return markers;
}

QList<QByteArray> engineLogGpuOrStderrEvidenceContentMarkers()
{
    return {
        acceptanceEvidenceMarkerBytes(engineLogCudaMarker),
        acceptanceEvidenceMarkerBytes(engineLogOpenClMarker),
        acceptanceEvidenceMarkerBytes(engineLogTensorRtMarker),
        acceptanceEvidenceMarkerBytes(engineLogGpuMarker),
        acceptanceEvidenceMarkerBytes(engineLogLowerGpuMarker),
        acceptanceEvidenceMarkerBytes(engineLogBackendMarker),
        acceptanceEvidenceMarkerBytes(engineLogTitleBackendMarker),
        acceptanceEvidenceMarkerBytes(engineLogStderrMarker),
        acceptanceEvidenceMarkerBytes(engineLogTitleStderrMarker),
    };
}

QList<QByteArray> rawKataGoComparisonLogEvidenceContentMarkers(const TargetAcceptanceRecords& records)
{
    QList<QByteArray> markers = kataGoEvidenceContentMarkers();
    if (targetAcceptanceKataGoVersionStatus(records.kataGoVersion) == QStringLiteral("match")) {
        markers.push_back(records.kataGoVersion.toUtf8());
    }
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoRealtimeGtpMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoAnalysisJsonMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoRootInfoMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoWinrateMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoScoreMeanMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoScoreStdevMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoVisitsMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoPolicyMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoPvMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoPvVisitsMarker));
    markers.push_back(acceptanceEvidenceMarkerBytes(rawKataGoOwnershipMarker));
    for (const QString& checklistItem : rawKataGoComparisonChecklist()) {
        markers.push_back(checklistItem.toUtf8());
    }
    return markers;
}

QList<QByteArray> manualUiInspectionLogEvidenceContentMarkers()
{
    QList<QByteArray> markers;
    for (const QString& checklistItem : manualUiInspectionChecklist()) {
        markers.push_back(checklistItem.toUtf8());
    }
    return markers;
}

QList<QByteArray> externalTargetVerificationLogEvidenceContentMarkers()
{
    QList<QByteArray> markers;
    for (const QString& checklistItem : externalTargetVerificationChecklist()) {
        markers.push_back(checklistItem.toUtf8());
    }
    return markers;
}

void appendAcceptanceEvidenceContentBlocker(
    QStringList& blockers,
    const TargetAcceptanceRecords& records,
    const QString& name,
    const QString& path,
    const QList<QByteArray>& markers)
{
    if (acceptanceEvidenceContentStatus(records, path, markers) != QStringLiteral("match")) {
        blockers.push_back(name);
    }
}

void appendAcceptanceEvidenceAnyContentBlocker(
    QStringList& blockers,
    const TargetAcceptanceRecords& records,
    const QString& name,
    const QString& path,
    const QList<QByteArray>& markers)
{
    if (acceptanceEvidenceAnyContentStatus(records, path, markers) != QStringLiteral("match")) {
        blockers.push_back(name);
    }
}

QStringList targetAcceptanceEvidenceBlockers(const TargetAcceptanceRecords& records)
{
    QStringList blockers;
    appendAcceptanceEvidenceBlocker(blockers, records, QStringLiteral("diagnosticsEvidence"), records.diagnosticsPath);
    appendAcceptanceEvidenceBlocker(
        blockers,
        records,
        QStringLiteral("targetAcceptanceReportEvidence"),
        records.targetAcceptanceReportPath);
    appendAcceptanceEvidenceBlocker(blockers, records, QStringLiteral("engineLogEvidence"), records.engineLogPath);
    appendAcceptanceEvidenceBlocker(
        blockers,
        records,
        QStringLiteral("rawKataGoComparisonLogEvidence"),
        records.rawKataGoComparisonLogPath);
    appendAcceptanceEvidenceBlocker(
        blockers,
        records,
        QStringLiteral("manualUiInspectionLogEvidence"),
        records.manualUiInspectionLogPath);
    appendAcceptanceEvidenceBlocker(
        blockers,
        records,
        QStringLiteral("externalTargetVerificationLogEvidence"),
        records.externalTargetVerificationLogPath);
    appendAcceptanceEvidenceBlocker(blockers, records, QStringLiteral("screenshotEvidence"), records.screenshotPath);
    appendDuplicateAcceptanceEvidencePathBlockers(
        blockers,
        records,
        {
            {QStringLiteral("diagnosticsEvidence"), records.diagnosticsPath},
            {QStringLiteral("targetAcceptanceReportEvidence"), records.targetAcceptanceReportPath},
            {QStringLiteral("engineLogEvidence"), records.engineLogPath},
            {QStringLiteral("rawKataGoComparisonLogEvidence"), records.rawKataGoComparisonLogPath},
            {QStringLiteral("manualUiInspectionLogEvidence"), records.manualUiInspectionLogPath},
            {QStringLiteral("externalTargetVerificationLogEvidence"), records.externalTargetVerificationLogPath},
            {QStringLiteral("screenshotEvidence"), records.screenshotPath},
        });
    appendAcceptanceEvidenceContentBlocker(
        blockers,
        records,
        QStringLiteral("diagnosticsEvidenceContent"),
        records.diagnosticsPath,
        diagnosticsEvidenceContentMarkers(records));
    appendAcceptanceEvidenceContentBlocker(
        blockers,
        records,
        QStringLiteral("targetAcceptanceReportEvidenceContent"),
        records.targetAcceptanceReportPath,
        targetAcceptanceReportEvidenceContentMarkers(records));
    appendAcceptanceEvidenceContentBlocker(
        blockers,
        records,
        QStringLiteral("engineLogEvidenceContent"),
        records.engineLogPath,
        engineLogEvidenceContentMarkers(records));
    appendAcceptanceEvidenceAnyContentBlocker(
        blockers,
        records,
        QStringLiteral("engineLogGpuOrStderrEvidenceContent"),
        records.engineLogPath,
        engineLogGpuOrStderrEvidenceContentMarkers());
    appendAcceptanceEvidenceContentBlocker(
        blockers,
        records,
        QStringLiteral("rawKataGoComparisonLogEvidenceContent"),
        records.rawKataGoComparisonLogPath,
        rawKataGoComparisonLogEvidenceContentMarkers(records));
    appendAcceptanceEvidenceContentBlocker(
        blockers,
        records,
        QStringLiteral("manualUiInspectionLogEvidenceContent"),
        records.manualUiInspectionLogPath,
        manualUiInspectionLogEvidenceContentMarkers());
    appendAcceptanceEvidenceContentBlocker(
        blockers,
        records,
        QStringLiteral("externalTargetVerificationLogEvidenceContent"),
        records.externalTargetVerificationLogPath,
        externalTargetVerificationLogEvidenceContentMarkers());
    return blockers;
}

bool targetAcceptanceEvidenceReady(const TargetAcceptanceRecords& records)
{
    return targetAcceptanceEvidenceBlockers(records).isEmpty();
}

void appendAcceptanceEvidenceTimestampBlocker(
    QStringList& blockers,
    const TargetAcceptanceRecords& records,
    const QString& name,
    const QString& path)
{
    if (acceptanceEvidenceTimestampStatus(records, path) != QStringLiteral("not-after-completed-utc")) {
        blockers.push_back(name);
    }
}

QStringList targetAcceptanceEvidenceTimestampBlockers(const TargetAcceptanceRecords& records)
{
    QStringList blockers;
    appendAcceptanceEvidenceTimestampBlocker(
        blockers,
        records,
        QStringLiteral("diagnosticsEvidenceTimestamp"),
        records.diagnosticsPath);
    appendAcceptanceEvidenceTimestampBlocker(
        blockers,
        records,
        QStringLiteral("targetAcceptanceReportEvidenceTimestamp"),
        records.targetAcceptanceReportPath);
    appendAcceptanceEvidenceTimestampBlocker(
        blockers,
        records,
        QStringLiteral("engineLogEvidenceTimestamp"),
        records.engineLogPath);
    appendAcceptanceEvidenceTimestampBlocker(
        blockers,
        records,
        QStringLiteral("rawKataGoComparisonLogEvidenceTimestamp"),
        records.rawKataGoComparisonLogPath);
    appendAcceptanceEvidenceTimestampBlocker(
        blockers,
        records,
        QStringLiteral("manualUiInspectionLogEvidenceTimestamp"),
        records.manualUiInspectionLogPath);
    appendAcceptanceEvidenceTimestampBlocker(
        blockers,
        records,
        QStringLiteral("externalTargetVerificationLogEvidenceTimestamp"),
        records.externalTargetVerificationLogPath);
    appendAcceptanceEvidenceTimestampBlocker(
        blockers,
        records,
        QStringLiteral("screenshotEvidenceTimestamp"),
        records.screenshotPath);
    return blockers;
}

bool targetAcceptanceEvidenceTimestampReady(const TargetAcceptanceRecords& records)
{
    return targetAcceptanceEvidenceTimestampBlockers(records).isEmpty();
}

void appendAcceptanceEvidenceSha256Blocker(
    QStringList& blockers,
    const TargetAcceptanceRecords& records,
    const QString& name,
    const QString& path,
    const QString& expectedSha256)
{
    const QString status = acceptanceEvidenceSha256Status(records, path, expectedSha256);
    if (status != QStringLiteral("match")) {
        blockers.push_back(name);
    }
}

QStringList targetAcceptanceEvidenceSha256Blockers(const TargetAcceptanceRecords& records)
{
    QStringList blockers;
    appendAcceptanceEvidenceSha256Blocker(
        blockers,
        records,
        QStringLiteral("diagnosticsEvidenceSha256"),
        records.diagnosticsPath,
        records.diagnosticsSha256);
    appendAcceptanceEvidenceSha256Blocker(
        blockers,
        records,
        QStringLiteral("targetAcceptanceReportEvidenceSha256"),
        records.targetAcceptanceReportPath,
        records.targetAcceptanceReportSha256);
    appendAcceptanceEvidenceSha256Blocker(
        blockers,
        records,
        QStringLiteral("engineLogEvidenceSha256"),
        records.engineLogPath,
        records.engineLogSha256);
    appendAcceptanceEvidenceSha256Blocker(
        blockers,
        records,
        QStringLiteral("rawKataGoComparisonLogEvidenceSha256"),
        records.rawKataGoComparisonLogPath,
        records.rawKataGoComparisonLogSha256);
    appendAcceptanceEvidenceSha256Blocker(
        blockers,
        records,
        QStringLiteral("manualUiInspectionLogEvidenceSha256"),
        records.manualUiInspectionLogPath,
        records.manualUiInspectionLogSha256);
    appendAcceptanceEvidenceSha256Blocker(
        blockers,
        records,
        QStringLiteral("externalTargetVerificationLogEvidenceSha256"),
        records.externalTargetVerificationLogPath,
        records.externalTargetVerificationLogSha256);
    appendAcceptanceEvidenceSha256Blocker(
        blockers,
        records,
        QStringLiteral("screenshotEvidenceSha256"),
        records.screenshotPath,
        records.screenshotSha256);
    return blockers;
}

QString checklistRecordId(const QString& section, const QString& item)
{
    return section + QStringLiteral(".") + item;
}

QString checklistRecordKey(const QString& section, const QString& item)
{
    return QStringLiteral("checklist.") + section + QStringLiteral("/") + item;
}

bool checklistRecordListContains(const QStringList& values, const QString& section, const QString& item)
{
    return values.contains(checklistRecordId(section, item));
}

QString checklistItemRecordStatus(const TargetAcceptanceRecords& records, const QString& section, const QString& item)
{
    if (checklistRecordListContains(records.passedChecklistItems, section, item)) {
        return QStringLiteral("pass");
    }
    if (checklistRecordListContains(records.failedChecklistItems, section, item)) {
        return QStringLiteral("fail");
    }
    if (checklistRecordListContains(records.invalidChecklistItems, section, item)) {
        return QStringLiteral("invalid-manual-record");
    }
    return manualRecordRequiredStatus();
}

void appendChecklistBlockers(
    QStringList& blockers,
    const TargetAcceptanceRecords& records,
    const QString& section,
    const QStringList& items)
{
    for (const QString& item : items) {
        if (!manualAcceptancePassed(checklistItemRecordStatus(records, section, item))) {
            blockers.push_back(checklistRecordId(section, item));
        }
    }
}

QStringList targetAcceptanceChecklistBlockers(const TargetAcceptanceRecords& records)
{
    QStringList blockers;
    appendChecklistBlockers(
        blockers,
        records,
        QStringLiteral("linuxKdeWaylandNvidia"),
        linuxKdeWaylandNvidiaVerificationChecklist());
    appendChecklistBlockers(blockers, records, QStringLiteral("windowsNvidia"), windowsNvidiaVerificationChecklist());
    appendChecklistBlockers(blockers, records, QStringLiteral("physicalDisplay"), physicalDisplayVerificationChecklist());
    appendChecklistBlockers(blockers, records, QStringLiteral("rawKataGoComparison"), rawKataGoComparisonChecklist());
    appendChecklistBlockers(
        blockers,
        records,
        QStringLiteral("externalTargetVerification"),
        externalTargetVerificationChecklist());
    appendChecklistBlockers(blockers, records, QStringLiteral("manualUiInspection"), manualUiInspectionChecklist());
    return blockers;
}

void appendMissingChecklistRecords(
    QStringList& missing,
    const TargetAcceptanceRecords& records,
    const QString& section,
    const QStringList& items)
{
    for (const QString& item : items) {
        if (checklistItemRecordStatus(records, section, item) == manualRecordRequiredStatus()) {
            missing.push_back(checklistRecordId(section, item));
        }
    }
}

QStringList targetAcceptanceMissingChecklistRecords(const TargetAcceptanceRecords& records)
{
    QStringList missing;
    appendMissingChecklistRecords(
        missing,
        records,
        QStringLiteral("linuxKdeWaylandNvidia"),
        linuxKdeWaylandNvidiaVerificationChecklist());
    appendMissingChecklistRecords(missing, records, QStringLiteral("windowsNvidia"), windowsNvidiaVerificationChecklist());
    appendMissingChecklistRecords(missing, records, QStringLiteral("physicalDisplay"), physicalDisplayVerificationChecklist());
    appendMissingChecklistRecords(missing, records, QStringLiteral("rawKataGoComparison"), rawKataGoComparisonChecklist());
    appendMissingChecklistRecords(
        missing,
        records,
        QStringLiteral("externalTargetVerification"),
        externalTargetVerificationChecklist());
    appendMissingChecklistRecords(missing, records, QStringLiteral("manualUiInspection"), manualUiInspectionChecklist());
    return missing;
}

void appendChecklistItemRecord(TargetAcceptanceRecords& records, const QString& section, const QString& item, const QString& status)
{
    const QString recordId = checklistRecordId(section, item);
    if (manualAcceptancePassed(status)) {
        records.passedChecklistItems.push_back(recordId);
    } else if (manualAcceptanceFailed(status)) {
        records.failedChecklistItems.push_back(recordId);
    } else if (manualAcceptanceInvalid(status)) {
        records.invalidChecklistItems.push_back(recordId);
    }
}

void readChecklistItemRecords(
    TargetAcceptanceRecords& records,
    QSettings& settings,
    const QString& section,
    const QStringList& items)
{
    for (const QString& item : items) {
        appendChecklistItemRecord(records, section, item, manualAcceptanceSetting(&settings, checklistRecordKey(section, item)));
    }
}

QString checklistSectionRecordStatus(const TargetAcceptanceRecords& records, const QString& section, const QStringList& items)
{
    if (items.isEmpty()) {
        return manualRecordRequiredStatus();
    }

    bool allPassed = true;
    bool anyFailed = false;
    bool anyInvalid = false;
    for (const QString& item : items) {
        const QString status = checklistItemRecordStatus(records, section, item);
        allPassed = allPassed && manualAcceptancePassed(status);
        anyFailed = anyFailed || manualAcceptanceFailed(status);
        anyInvalid = anyInvalid || manualAcceptanceInvalid(status);
    }
    if (allPassed) {
        return QStringLiteral("pass");
    }
    if (anyInvalid) {
        return QStringLiteral("invalid-manual-record");
    }
    if (anyFailed) {
        return QStringLiteral("fail");
    }
    return manualRecordRequiredStatus();
}

QString manualAcceptanceSettingOrChecklistStatus(
    QSettings& settings,
    const QString& key,
    const TargetAcceptanceRecords& records,
    const QString& section,
    const QStringList& items)
{
    const QString explicitStatus = manualAcceptanceSetting(&settings, key);
    if (explicitStatus != manualRecordRequiredStatus()) {
        return explicitStatus;
    }
    return checklistSectionRecordStatus(records, section, items);
}

TargetAcceptanceRecords inspectTargetAcceptanceRecords()
{
    TargetAcceptanceRecords records;
    records.fileSet = qEnvironmentVariableIsSet(targetAcceptanceRecordEnvironmentName());
    if (!records.fileSet) {
        return records;
    }

    const QByteArray fileName = qgetenv(targetAcceptanceRecordEnvironmentName());
    if (fileName.isEmpty()) {
        records.fileName = QStringLiteral("(empty)");
        return records;
    }

    records.fileName = QString::fromLocal8Bit(fileName);
    const QFileInfo info(records.fileName);
    records.fileExists = info.exists();
    records.fileReadable = info.isFile() && info.isReadable();
    if (!records.fileReadable) {
        return records;
    }

    QSettings settings(records.fileName, QSettings::IniFormat);
    records.completedUtc = recordMetadataSetting(settings, QStringLiteral("metadata/completedUtc"));
    records.appVersion = recordMetadataSetting(settings, QStringLiteral("metadata/appVersion"));
    records.appExecutableSha256 = recordSha256Setting(settings, QStringLiteral("metadata/appExecutableSha256"));
    records.osPrettyName = recordMetadataSetting(settings, QStringLiteral("metadata/osPrettyName"));
    records.osKernelType = recordMetadataSetting(settings, QStringLiteral("metadata/osKernelType"));
    records.osKernelVersion = recordMetadataSetting(settings, QStringLiteral("metadata/osKernelVersion"));
    records.qtRuntimeVersion = recordMetadataSetting(settings, QStringLiteral("metadata/qtRuntimeVersion"));
    records.qtBuildAbi = recordMetadataSetting(settings, QStringLiteral("metadata/qtBuildAbi"));
    records.cpuArchitecture = recordMetadataSetting(settings, QStringLiteral("metadata/cpuArchitecture"));
    records.reviewer = recordMetadataSetting(settings, QStringLiteral("metadata/reviewer"));
    records.targetMachine = recordMetadataSetting(settings, QStringLiteral("metadata/targetMachine"));
    records.gpuDriver = recordMetadataSetting(settings, QStringLiteral("metadata/gpuDriver"));
    records.kataGoVersion = recordMetadataSetting(settings, QStringLiteral("metadata/kataGoVersion"));
    records.notes = recordMetadataSetting(settings, QStringLiteral("metadata/notes"));
    records.diagnosticsPath = recordPathSetting(settings, QStringLiteral("evidence/diagnostics"));
    records.targetAcceptanceReportPath = recordPathSetting(settings, QStringLiteral("evidence/targetAcceptanceReport"));
    records.engineLogPath = recordPathSetting(settings, QStringLiteral("evidence/engineLog"));
    records.rawKataGoComparisonLogPath = recordPathSetting(settings, QStringLiteral("evidence/rawKataGoComparisonLog"));
    records.manualUiInspectionLogPath = recordPathSetting(settings, QStringLiteral("evidence/manualUiInspectionLog"));
    records.externalTargetVerificationLogPath =
        recordPathSetting(settings, QStringLiteral("evidence/externalTargetVerificationLog"));
    records.screenshotPath = recordPathSetting(settings, QStringLiteral("evidence/screenshot"));
    records.diagnosticsSha256 = recordSha256Setting(settings, QStringLiteral("evidenceSha256/diagnostics"));
    records.targetAcceptanceReportSha256 =
        recordSha256Setting(settings, QStringLiteral("evidenceSha256/targetAcceptanceReport"));
    records.engineLogSha256 = recordSha256Setting(settings, QStringLiteral("evidenceSha256/engineLog"));
    records.rawKataGoComparisonLogSha256 =
        recordSha256Setting(settings, QStringLiteral("evidenceSha256/rawKataGoComparisonLog"));
    records.manualUiInspectionLogSha256 =
        recordSha256Setting(settings, QStringLiteral("evidenceSha256/manualUiInspectionLog"));
    records.externalTargetVerificationLogSha256 =
        recordSha256Setting(settings, QStringLiteral("evidenceSha256/externalTargetVerificationLog"));
    records.screenshotSha256 = recordSha256Setting(settings, QStringLiteral("evidenceSha256/screenshot"));

    readChecklistItemRecords(
        records,
        settings,
        QStringLiteral("linuxKdeWaylandNvidia"),
        linuxKdeWaylandNvidiaVerificationChecklist());
    readChecklistItemRecords(records, settings, QStringLiteral("windowsNvidia"), windowsNvidiaVerificationChecklist());
    readChecklistItemRecords(records, settings, QStringLiteral("physicalDisplay"), physicalDisplayVerificationChecklist());
    readChecklistItemRecords(records, settings, QStringLiteral("rawKataGoComparison"), rawKataGoComparisonChecklist());
    readChecklistItemRecords(records, settings, QStringLiteral("externalTargetVerification"), externalTargetVerificationChecklist());
    readChecklistItemRecords(records, settings, QStringLiteral("manualUiInspection"), manualUiInspectionChecklist());

    records.externalTargetVerification = manualAcceptanceSettingOrChecklistStatus(
        settings,
        QStringLiteral("acceptance/externalTargetVerification"),
        records,
        QStringLiteral("externalTargetVerification"),
        externalTargetVerificationChecklist());
    records.linuxKdeWaylandNvidia = manualAcceptanceSettingOrChecklistStatus(
        settings,
        QStringLiteral("acceptance/linuxKdeWaylandNvidia"),
        records,
        QStringLiteral("linuxKdeWaylandNvidia"),
        linuxKdeWaylandNvidiaVerificationChecklist());
    records.windowsNvidia = manualAcceptanceSettingOrChecklistStatus(
        settings,
        QStringLiteral("acceptance/windowsNvidia"),
        records,
        QStringLiteral("windowsNvidia"),
        windowsNvidiaVerificationChecklist());
    records.physicalDisplay = manualAcceptanceSettingOrChecklistStatus(
        settings,
        QStringLiteral("acceptance/physicalDisplay"),
        records,
        QStringLiteral("physicalDisplay"),
        physicalDisplayVerificationChecklist());
    records.rawKataGoComparison = manualAcceptanceSettingOrChecklistStatus(
        settings,
        QStringLiteral("acceptance/rawKataGoComparison"),
        records,
        QStringLiteral("rawKataGoComparison"),
        rawKataGoComparisonChecklist());
    records.manualUiInspection = manualAcceptanceSettingOrChecklistStatus(
        settings,
        QStringLiteral("acceptance/manualUiInspection"),
        records,
        QStringLiteral("manualUiInspection"),
        manualUiInspectionChecklist());
    return records;
}

QString externalTargetVerificationStatus(const TargetAcceptanceRecords& records)
{
    if (records.externalTargetVerification != manualRecordRequiredStatus()) {
        return records.externalTargetVerification;
    }

    const QStringList componentStatuses = {
        records.linuxKdeWaylandNvidia,
        records.windowsNvidia,
        records.physicalDisplay,
        records.rawKataGoComparison,
    };
    if (std::ranges::all_of(componentStatuses, manualAcceptancePassed)) {
        return QStringLiteral("pass");
    }
    if (std::ranges::any_of(componentStatuses, manualAcceptanceInvalid)) {
        return QStringLiteral("invalid-manual-record");
    }
    if (std::ranges::any_of(componentStatuses, manualAcceptanceFailed)) {
        return QStringLiteral("fail");
    }
    return manualRecordRequiredStatus();
}

template <typename Predicate>
QStringList targetAcceptanceManualRecordsMatching(const TargetAcceptanceRecords& records, Predicate predicate)
{
    QStringList matches;
    const auto appendIfMatching = [&](const QString& name, const QString& status) {
        if (predicate(status)) {
            matches.push_back(name);
        }
    };

    appendIfMatching(QStringLiteral("externalTargetVerification"), externalTargetVerificationStatus(records));
    appendIfMatching(QStringLiteral("linuxKdeWaylandNvidia"), records.linuxKdeWaylandNvidia);
    appendIfMatching(QStringLiteral("windowsNvidia"), records.windowsNvidia);
    appendIfMatching(QStringLiteral("physicalDisplay"), records.physicalDisplay);
    appendIfMatching(QStringLiteral("rawKataGoComparison"), records.rawKataGoComparison);
    appendIfMatching(QStringLiteral("manualUiInspection"), records.manualUiInspection);
    return matches;
}

QStringList targetAcceptanceFailedManualRecords(const TargetAcceptanceRecords& records)
{
    return targetAcceptanceManualRecordsMatching(records, manualAcceptanceFailed);
}

QStringList targetAcceptanceInvalidManualRecords(const TargetAcceptanceRecords& records)
{
    return targetAcceptanceManualRecordsMatching(records, manualAcceptanceInvalid);
}

bool targetAcceptanceRecordsHaveInvalidManualResult(const TargetAcceptanceRecords& records)
{
    if (!records.invalidChecklistItems.isEmpty()) {
        return true;
    }

    const QStringList statuses = {
        externalTargetVerificationStatus(records),
        records.linuxKdeWaylandNvidia,
        records.windowsNvidia,
        records.physicalDisplay,
        records.rawKataGoComparison,
        records.manualUiInspection,
    };
    return std::ranges::any_of(statuses, manualAcceptanceInvalid);
}

bool targetAcceptanceRecordsHaveFailedManualResult(const TargetAcceptanceRecords& records)
{
    if (!records.failedChecklistItems.isEmpty()) {
        return true;
    }

    const QStringList statuses = {
        externalTargetVerificationStatus(records),
        records.linuxKdeWaylandNvidia,
        records.windowsNvidia,
        records.physicalDisplay,
        records.rawKataGoComparison,
        records.manualUiInspection,
    };
    return std::ranges::any_of(statuses, manualAcceptanceFailed);
}

QString finalAcceptanceStatus(const QStringList& blockers, const TargetAcceptanceRecords& records)
{
    if (targetAcceptanceRecordsHaveInvalidManualResult(records)) {
        return QStringLiteral("invalid-manual-acceptance-record");
    }
    if (targetAcceptanceRecordsHaveFailedManualResult(records)) {
        return QStringLiteral("manual-acceptance-failed");
    }
    if (blockers.isEmpty()) {
        return QStringLiteral("accepted");
    }
    for (const QString& blocker : blockers) {
        if (!isManualFinalAcceptanceBlocker(blocker)) {
            return QStringLiteral("needs-readiness-and-final-manual-acceptance");
        }
    }
    return QStringLiteral("ready-for-final-manual-acceptance");
}

QString targetRealtimeVerificationStatus(bool targetPlatformCandidate, bool configuredGtpReady, const QString& targetName)
{
    if (targetPlatformCandidate && configuredGtpReady) {
        return QStringLiteral("ready-for-target-realtime-verification");
    }
    if (targetPlatformCandidate) {
        return QStringLiteral("target-platform-ready-needs-katago-gtp-config");
    }
    if (configuredGtpReady) {
        return QStringLiteral("katago-gtp-config-ready-needs-") + targetName;
    }
    return QStringLiteral("needs-") + targetName + QStringLiteral("-and-katago-gtp-config");
}

QStringList targetRealtimeVerificationReadinessBlockers(QStringList platformBlockers, bool configuredGtpReady)
{
    if (!configuredGtpReady) {
        platformBlockers.push_back(QStringLiteral("kataGoGtpConfig"));
    }
    return platformBlockers;
}

QString physicalDisplayVerificationStatus(bool targetDisplayCandidate, bool multiDisplayCandidate)
{
    if (targetDisplayCandidate && multiDisplayCandidate) {
        return QStringLiteral("ready-for-physical-display-verification");
    }
    if (targetDisplayCandidate) {
        return QStringLiteral("target-display-ready-needs-multi-display");
    }
    if (multiDisplayCandidate) {
        return QStringLiteral("multi-display-ready-needs-target-display");
    }
    return QStringLiteral("needs-target-display-and-multi-display");
}

QStringList physicalDisplayVerificationReadinessBlockers(
    const QList<QScreen*>& screens,
    bool anyDisplay4K,
    bool anyHighDpiScale,
    bool anyTargetDisplayCandidate)
{
    QStringList blockers = targetDisplayBlockers(screens, anyDisplay4K, anyHighDpiScale, anyTargetDisplayCandidate);
    blockers.append(multiDisplayBlockers(screens));
    return blockers;
}

QStringList externalTargetVerificationChecklist()
{
    return {
        QStringLiteral("linuxKdeWaylandNvidiaRealtimeKataGo"),
        QStringLiteral("windowsNvidiaRealtimeKataGo"),
        QStringLiteral("physical4KHighDpiMultiDisplayUi"),
        QStringLiteral("rawKataGoUiComparison"),
    };
}

QStringList linuxKdeWaylandNvidiaVerificationChecklist()
{
    return {
        QStringLiteral("linuxOs"),
        QStringLiteral("kdeSession"),
        QStringLiteral("waylandSession"),
        QStringLiteral("qwaylandPlatformPlugin"),
        QStringLiteral("nvidiaEnvironmentHint"),
        QStringLiteral("packageStarts"),
        QStringLiteral("configureKataGoPaths"),
        QStringLiteral("realtimeAnalyzeCurrentPosition"),
        QStringLiteral("branchResync"),
        QStringLiteral("gpuStderrCaptured"),
    };
}

QStringList windowsNvidiaVerificationChecklist()
{
    return {
        QStringLiteral("windowsOs"),
        QStringLiteral("qwindowsPlatformPlugin"),
        QStringLiteral("packageExtracts"),
        QStringLiteral("appLocalQtRuntime"),
        QStringLiteral("nvidiaEnvironmentHint"),
        QStringLiteral("configureKataGoPaths"),
        QStringLiteral("nativeSettingsPathDialog"),
        QStringLiteral("realtimeAnalyzeCurrentPosition"),
        QStringLiteral("engineDiagnosticsCaptured"),
    };
}

QStringList physicalDisplayVerificationChecklist()
{
    return {
        QStringLiteral("physical4KPanel"),
        QStringLiteral("highDpiScale150Or200"),
        QStringLiteral("multiDisplayLayout"),
        QStringLiteral("boardTextSharpness"),
        QStringLiteral("candidateLabelsNoOverlap"),
        QStringLiteral("pvPreviewNoOverlap"),
        QStringLiteral("ownershipOverlayReadable"),
        QStringLiteral("winrateScoreGraphReadable"),
        QStringLiteral("restoredWindowVisible"),
    };
}

QStringList manualUiInspectionChecklist()
{
    return {
        QStringLiteral("mainWindow4KHighDpiLayout"),
        QStringLiteral("boardLinesCoordinatesAndStarPoints"),
        QStringLiteral("stoneRenderingAndCandidateLabels"),
        QStringLiteral("candidateTableColumns"),
        QStringLiteral("pvPreviewStones"),
        QStringLiteral("ownershipMainBoardOverlay"),
        QStringLiteral("ownershipMiniBoardDock"),
        QStringLiteral("winrateScoreGraph"),
        QStringLiteral("toolbarDockAndMenuLayout"),
        QStringLiteral("bilingualTextFit"),
        QStringLiteral("savedWindowRestore"),
        QStringLiteral("multiDisplayPlacement"),
    };
}

QStringList rawKataGoComparisonChecklist()
{
    return {
        QStringLiteral("analysisJsonRawResponse"),
        QStringLiteral("realtimeGtpRawLine"),
        QStringLiteral("candidateTableRendering"),
        QStringLiteral("pvPreview"),
        QStringLiteral("winrateScorePerspective"),
        QStringLiteral("ownershipDisplay"),
        QStringLiteral("genmove"),
        QStringLiteral("humanVsAi"),
        QStringLiteral("selfPlay"),
        QStringLiteral("engineVsEngine"),
    };
}

QString templateMarkerText(std::string_view marker)
{
    return QString::fromUtf8(marker.data(), static_cast<qsizetype>(marker.size()));
}

QStringList currentAppTemplateContentMarkers()
{
    return {
        templateMarkerText(diagnosticsEvidenceAppVersionMarker),
        QStringLiteral("<record appVersion>"),
        templateMarkerText(diagnosticsEvidenceAppExecutableMarker),
        QStringLiteral("<current app.executable>"),
    };
}

QStringList diagnosticsTemplateContentMarkers()
{
    QStringList markers = {
        templateMarkerText(diagnosticsEvidenceMarker),
    };
    markers.append(currentAppTemplateContentMarkers());
    markers.append(
        {
            templateMarkerText(diagnosticsRecordFileCanonicalPathMarker),
            QStringLiteral("<record canonical path>"),
            templateMarkerText(diagnosticsRecordFileSha256Marker),
            templateMarkerText(acceptanceFinalStatusMarker),
            templateMarkerText(acceptanceFinalBlockerMarker),
            templateMarkerText(acceptanceFinalOutstandingBlockerMarker),
        });
    return markers;
}

QStringList targetAcceptanceReportTemplateContentMarkers()
{
    QStringList markers = {
        templateMarkerText(targetAcceptanceReportEvidenceMarker),
    };
    markers.append(currentAppTemplateContentMarkers());
    markers.append(
        {
            templateMarkerText(targetAcceptanceReportRecordFileCanonicalPathMarker),
            QStringLiteral("<record canonical path>"),
            templateMarkerText(targetAcceptanceReportRecordFileSha256Marker),
            templateMarkerText(acceptanceFinalStatusMarker),
            templateMarkerText(acceptanceFinalBlockerMarker),
            templateMarkerText(acceptanceFinalOutstandingBlockerMarker),
        });
    return markers;
}

QStringList rawKataGoComparisonTemplateContentMarkers()
{
    QStringList markers = {
        templateMarkerText(kataGoEvidenceMarker),
        QStringLiteral("<record kataGoVersion>"),
        templateMarkerText(rawKataGoRealtimeGtpMarker),
        templateMarkerText(rawKataGoAnalysisJsonMarker),
        templateMarkerText(rawKataGoRootInfoMarker),
        templateMarkerText(rawKataGoWinrateMarker),
        templateMarkerText(rawKataGoScoreMeanMarker),
        templateMarkerText(rawKataGoScoreStdevMarker),
        templateMarkerText(rawKataGoVisitsMarker),
        templateMarkerText(rawKataGoPolicyMarker),
        templateMarkerText(rawKataGoPvMarker),
        templateMarkerText(rawKataGoPvVisitsMarker),
        templateMarkerText(rawKataGoOwnershipMarker),
    };
    markers.append(rawKataGoComparisonChecklist());
    return markers;
}

QStringList manualUiInspectionTemplateContentMarkers()
{
    return manualUiInspectionChecklist();
}

QStringList externalTargetVerificationTemplateContentMarkers()
{
    return externalTargetVerificationChecklist();
}

void printTargetAcceptanceRecordTemplateSection(const QString& sectionName, const QStringList& keys)
{
    std::cout << "\n[" << sectionName.toUtf8().constData() << "]\n";
    for (const QString& key : keys) {
        std::cout << key.toUtf8().constData() << "=\n";
    }
}

void printTargetAcceptanceRecordTemplateMarkerList(const QString& key, const QStringList& markers)
{
    std::cout << key.toUtf8().constData() << "=" << markers.join(QStringLiteral("; ")).toUtf8().constData()
              << '\n';
}

void printTargetAcceptanceRecordTemplateEvidenceContentMarkersSection()
{
    std::cout << "\n[evidenceContentMarkers]\n";
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("diagnostics"),
        diagnosticsTemplateContentMarkers());
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("targetAcceptanceReport"),
        targetAcceptanceReportTemplateContentMarkers());
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("engineLog"),
        {
            templateMarkerText(kataGoEvidenceMarker),
            QStringLiteral("<record kataGoVersion>"),
        });
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("engineLog.gpuOrStderrAny"),
        {
            templateMarkerText(engineLogCudaMarker),
            templateMarkerText(engineLogOpenClMarker),
            templateMarkerText(engineLogTensorRtMarker),
            templateMarkerText(engineLogGpuMarker),
            templateMarkerText(engineLogLowerGpuMarker),
            templateMarkerText(engineLogBackendMarker),
            templateMarkerText(engineLogTitleBackendMarker),
            templateMarkerText(engineLogStderrMarker),
            templateMarkerText(engineLogTitleStderrMarker),
        });
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("rawKataGoComparisonLog"),
        rawKataGoComparisonTemplateContentMarkers());
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("manualUiInspectionLog"),
        manualUiInspectionTemplateContentMarkers());
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("externalTargetVerificationLog"),
        externalTargetVerificationTemplateContentMarkers());
}

void printTargetAcceptanceRecordTemplateRecordHintsSection()
{
    std::cout << "\n[recordHints]\n";
    printTargetAcceptanceRecordTemplateMarkerList(QStringLiteral("passValues"), manualAcceptancePassValues());
    printTargetAcceptanceRecordTemplateMarkerList(QStringLiteral("failValues"), manualAcceptanceFailValues());
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("metadataKeysRequired"),
        targetAcceptanceMetadataRequiredKeys());
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("metadataExactMatchKeys"),
        targetAcceptanceMetadataExactMatchKeys());
    printTargetAcceptanceRecordTemplateMarkerList(
        QStringLiteral("metadataValueCheckedKeys"),
        targetAcceptanceMetadataValueCheckedKeys());
    std::cout << "completedUtcRequired=" << targetAcceptanceCompletedUtcRequirement().toUtf8().constData() << '\n';
    std::cout << "aggregateAcceptanceKeysRequired=pass\n";
    std::cout << "checklistItemsRequired=pass\n";
    std::cout << "evidencePathsRequired=readable-non-empty-distinct\n";
    std::cout << "evidenceSha256Required=true\n";
    std::cout << "evidenceContentMarkersRequired=true\n";
    std::cout << "recordAndEvidenceTimestampsMustNotAfterCompletedUtc=true\n";
}

void printTargetAcceptanceRecordTemplate(const QString& executablePath)
{
    std::cout << "[metadata]\n";
    std::cout << "completedUtc="
              << QDateTime::currentDateTimeUtc().toString(Qt::ISODate).toUtf8().constData() << '\n';
    std::cout << "appVersion=" << LIZZIEYZY_QT_VERSION << '\n';
    std::cout << "appExecutableSha256="
              << sha256ForReadableNonEmptyFile(QFileInfo(executablePath).absoluteFilePath()).toUtf8().constData()
              << '\n';
    std::cout << "osPrettyName=" << QSysInfo::prettyProductName().toUtf8().constData() << '\n';
    std::cout << "osKernelType=" << QSysInfo::kernelType().toUtf8().constData() << '\n';
    std::cout << "osKernelVersion=" << QSysInfo::kernelVersion().toUtf8().constData() << '\n';
    std::cout << "qtRuntimeVersion=" << qVersion() << '\n';
    std::cout << "qtBuildAbi=" << QSysInfo::buildAbi().toUtf8().constData() << '\n';
    std::cout << "cpuArchitecture=" << QSysInfo::currentCpuArchitecture().toUtf8().constData() << '\n';
    std::cout << "reviewer=\n";
    std::cout << "targetMachine=" << QSysInfo::machineHostName().toUtf8().constData() << '\n';
    std::cout << "gpuDriver=\n";
    std::cout << "kataGoVersion=\n";
    std::cout << "notes=\n";

    std::cout << "\n[evidence]\n";
    std::cout << "diagnostics=\n";
    std::cout << "targetAcceptanceReport=\n";
    std::cout << "engineLog=\n";
    std::cout << "rawKataGoComparisonLog=\n";
    std::cout << "manualUiInspectionLog=\n";
    std::cout << "externalTargetVerificationLog=\n";
    std::cout << "screenshot=\n";

    std::cout << "\n[evidenceSha256]\n";
    std::cout << "diagnostics=\n";
    std::cout << "targetAcceptanceReport=\n";
    std::cout << "engineLog=\n";
    std::cout << "rawKataGoComparisonLog=\n";
    std::cout << "manualUiInspectionLog=\n";
    std::cout << "externalTargetVerificationLog=\n";
    std::cout << "screenshot=\n";

    printTargetAcceptanceRecordTemplateEvidenceContentMarkersSection();
    printTargetAcceptanceRecordTemplateRecordHintsSection();

    printTargetAcceptanceRecordTemplateSection(
        QStringLiteral("acceptance"),
        {
            QStringLiteral("linuxKdeWaylandNvidia"),
            QStringLiteral("windowsNvidia"),
            QStringLiteral("physicalDisplay"),
            QStringLiteral("externalTargetVerification"),
            QStringLiteral("rawKataGoComparison"),
            QStringLiteral("manualUiInspection"),
        });
    printTargetAcceptanceRecordTemplateSection(
        QStringLiteral("checklist.linuxKdeWaylandNvidia"),
        linuxKdeWaylandNvidiaVerificationChecklist());
    printTargetAcceptanceRecordTemplateSection(QStringLiteral("checklist.windowsNvidia"), windowsNvidiaVerificationChecklist());
    printTargetAcceptanceRecordTemplateSection(QStringLiteral("checklist.physicalDisplay"), physicalDisplayVerificationChecklist());
    printTargetAcceptanceRecordTemplateSection(QStringLiteral("checklist.rawKataGoComparison"), rawKataGoComparisonChecklist());
    printTargetAcceptanceRecordTemplateSection(
        QStringLiteral("checklist.externalTargetVerification"),
        externalTargetVerificationChecklist());
    printTargetAcceptanceRecordTemplateSection(QStringLiteral("checklist.manualUiInspection"), manualUiInspectionChecklist());
}

void printMarkdownField(const char* key, const QString& value)
{
    const QByteArray bytes = value.toUtf8();
    std::cout << "- `" << key << "`: " << bytes.constData() << '\n';
}

void printMarkdownField(const char* key, bool value)
{
    printMarkdownField(key, value ? QStringLiteral("true") : QStringLiteral("false"));
}

void printMarkdownField(const char* key, int value)
{
    printMarkdownField(key, QString::number(value));
}

void printMarkdownListField(const char* key, const QStringList& values)
{
    printMarkdownField(key, values.isEmpty() ? QStringLiteral("(none)") : values.join(QStringLiteral(", ")));
}

QString acceptanceRecordHintValueText(const QStringList& values)
{
    return values.join(QStringLiteral("; "));
}

void printMarkdownAcceptanceRecordHintFields()
{
    printMarkdownField(
        "plan.acceptance.recordHint.passValues",
        acceptanceRecordHintValueText(manualAcceptancePassValues()));
    printMarkdownField(
        "plan.acceptance.recordHint.failValues",
        acceptanceRecordHintValueText(manualAcceptanceFailValues()));
    printMarkdownField(
        "plan.acceptance.recordHint.metadataKeysRequired",
        acceptanceRecordHintValueText(targetAcceptanceMetadataRequiredKeys()));
    printMarkdownField(
        "plan.acceptance.recordHint.metadataExactMatchKeys",
        acceptanceRecordHintValueText(targetAcceptanceMetadataExactMatchKeys()));
    printMarkdownField(
        "plan.acceptance.recordHint.metadataValueCheckedKeys",
        acceptanceRecordHintValueText(targetAcceptanceMetadataValueCheckedKeys()));
    printMarkdownField(
        "plan.acceptance.recordHint.completedUtcRequired",
        targetAcceptanceCompletedUtcRequirement());
    printMarkdownField("plan.acceptance.recordHint.aggregateAcceptanceKeysRequired", QStringLiteral("pass"));
    printMarkdownField("plan.acceptance.recordHint.checklistItemsRequired", QStringLiteral("pass"));
    printMarkdownField(
        "plan.acceptance.recordHint.evidencePathsRequired",
        QStringLiteral("readable-non-empty-distinct"));
    printMarkdownField("plan.acceptance.recordHint.evidenceSha256Required", true);
    printMarkdownField("plan.acceptance.recordHint.evidenceContentMarkersRequired", true);
    printMarkdownField(
        "plan.acceptance.recordHint.recordAndEvidenceTimestampsMustNotAfterCompletedUtc",
        true);
}

void printTextAcceptanceRecordHintFields()
{
    printText("plan.acceptance.recordHint.passValues", acceptanceRecordHintValueText(manualAcceptancePassValues()));
    printText("plan.acceptance.recordHint.failValues", acceptanceRecordHintValueText(manualAcceptanceFailValues()));
    printText(
        "plan.acceptance.recordHint.metadataKeysRequired",
        acceptanceRecordHintValueText(targetAcceptanceMetadataRequiredKeys()));
    printText(
        "plan.acceptance.recordHint.metadataExactMatchKeys",
        acceptanceRecordHintValueText(targetAcceptanceMetadataExactMatchKeys()));
    printText(
        "plan.acceptance.recordHint.metadataValueCheckedKeys",
        acceptanceRecordHintValueText(targetAcceptanceMetadataValueCheckedKeys()));
    printText("plan.acceptance.recordHint.completedUtcRequired", targetAcceptanceCompletedUtcRequirement());
    printText("plan.acceptance.recordHint.aggregateAcceptanceKeysRequired", QStringLiteral("pass"));
    printText("plan.acceptance.recordHint.checklistItemsRequired", QStringLiteral("pass"));
    printText("plan.acceptance.recordHint.evidencePathsRequired", QStringLiteral("readable-non-empty-distinct"));
    printBool("plan.acceptance.recordHint.evidenceSha256Required", true);
    printBool("plan.acceptance.recordHint.evidenceContentMarkersRequired", true);
    printBool("plan.acceptance.recordHint.recordAndEvidenceTimestampsMustNotAfterCompletedUtc", true);
}

void printMarkdownEvidencePath(
    const char* prefix,
    const TargetAcceptanceRecords& records,
    const QString& path,
    const QString& expectedSha256,
    const QList<QByteArray>& contentMarkers = QList<QByteArray>())
{
    const QFileInfo info(resolvedAcceptanceEvidencePath(records, path));
    const bool hasText = hasNonWhitespaceText(path);
    const bool exists = hasText && info.exists();
    const QByteArray prefixBytes(prefix);
    printMarkdownField((prefixBytes + ".value").constData(), recordPathDisplayText(path));
    printMarkdownField((prefixBytes + ".hasText").constData(), hasText);
    printMarkdownField((prefixBytes + ".exists").constData(), exists);
    printMarkdownField((prefixBytes + ".readable").constData(), exists && info.isReadable());
    printMarkdownField((prefixBytes + ".size").constData(), exists ? QString::number(info.size()) : QStringLiteral("-1"));
    printMarkdownField(
        (prefixBytes + ".lastModifiedUtc").constData(),
        exists ? info.lastModified().toUTC().toString(Qt::ISODateWithMs) : QStringLiteral("(missing)"));
    printMarkdownField((prefixBytes + ".sha256").constData(), acceptanceEvidenceSha256(records, path));
    printMarkdownField((prefixBytes + ".expectedSha256").constData(), recordSha256DisplayText(expectedSha256));
    printMarkdownField(
        (prefixBytes + ".sha256Status").constData(),
        acceptanceEvidenceSha256Status(records, path, expectedSha256));
    printMarkdownField((prefixBytes + ".timestampStatus").constData(), acceptanceEvidenceTimestampStatus(records, path));
    printMarkdownField(
        (prefixBytes + ".canonicalPath").constData(),
        exists ? info.canonicalFilePath() : QStringLiteral("(missing)"));
    if (!contentMarkers.isEmpty()) {
        printMarkdownField(
            (prefixBytes + ".contentStatus").constData(),
            acceptanceEvidenceContentStatus(records, path, contentMarkers));
        printMarkdownListField(
            (prefixBytes + ".missingContentMarker").constData(),
            acceptanceEvidenceMissingMarkers(records, path, contentMarkers));
    }
}

void printMarkdownImageEvidence(const char* prefix, const AcceptanceImageEvidenceInfo& info)
{
    const QByteArray prefixBytes(prefix);
    printMarkdownField((prefixBytes + ".imageReadable").constData(), info.readable);
    printMarkdownField((prefixBytes + ".imageFormat").constData(), info.format);
    printMarkdownField((prefixBytes + ".imageWidth").constData(), info.width);
    printMarkdownField((prefixBytes + ".imageHeight").constData(), info.height);
    printMarkdownField((prefixBytes + ".imagePixelsAtLeast4K").constData(), info.pixelsAtLeast4K);
    printMarkdownField((prefixBytes + ".imageHasPixelVariation").constData(), info.hasPixelVariation);
}

QString markdownEnvironmentValue(const char* environmentName)
{
    return QString::fromLocal8Bit(environmentValue(environmentName));
}

void printMarkdownSettingsValue(QSettings& settings, const char* key, const QString& settingKey)
{
    printMarkdownField(key, settings.contains(settingKey) ? settings.value(settingKey).toString() : QStringLiteral("(unset)"));
}

void printMarkdownSettingsCommandArgumentsSummary(QSettings& settings, const char* prefix, const QString& settingKey)
{
    const QString value = settings.contains(settingKey) ? settings.value(settingKey).toString() : QStringLiteral("(unset)");
    const std::vector<std::string> parsed =
        settings.contains(settingKey) ? lizzie::app::splitCommandArguments(value) : std::vector<std::string>{};

    const QByteArray prefixBytes(prefix);
    printMarkdownField((prefixBytes + ".value").constData(), value);
    printMarkdownField((prefixBytes + ".parsed.count").constData(), static_cast<int>(parsed.size()));
    for (std::size_t index = 0; index < parsed.size(); ++index) {
        const QString argument = QString::fromStdString(parsed[index]);
        printMarkdownField(
            (prefixBytes + ".parsed." + QByteArray::number(static_cast<qlonglong>(index))).constData(),
            argument.isEmpty() ? QStringLiteral("(empty)") : argument);
    }
}

void printMarkdownStoredEnginePathSummary(QSettings& settings, const char* prefix, const QString& group)
{
    const QByteArray prefixBytes(prefix);
    printMarkdownSettingsValue(
        settings,
        (prefixBytes + ".executable.value").constData(),
        group + QStringLiteral("/executable"));
    printMarkdownSettingsValue(settings, (prefixBytes + ".model.value").constData(), group + QStringLiteral("/model"));
    printMarkdownSettingsValue(
        settings,
        (prefixBytes + ".gtpConfig.value").constData(),
        group + QStringLiteral("/gtpConfig"));
    printMarkdownSettingsValue(
        settings,
        (prefixBytes + ".analysisConfig.value").constData(),
        group + QStringLiteral("/analysisConfig"));
    printMarkdownSettingsValue(
        settings,
        (prefixBytes + ".workingDirectory.value").constData(),
        group + QStringLiteral("/workingDirectory"));
    printMarkdownSettingsCommandArgumentsSummary(
        settings,
        (prefixBytes + ".extraArgs").constData(),
        group + QStringLiteral("/extraArgs"));
}

void printMarkdownConfiguredPathSummary()
{
    printMarkdownField("katago.env.executable", markdownEnvironmentValue("LIZZIE_KATAGO_EXECUTABLE"));
    printMarkdownField("katago.env.model", markdownEnvironmentValue("LIZZIE_KATAGO_MODEL"));
    printMarkdownField("katago.env.gtpConfig", markdownEnvironmentValue("LIZZIE_KATAGO_GTP_CONFIG"));
    printMarkdownField("katago.env.analysisConfig", markdownEnvironmentValue("LIZZIE_KATAGO_ANALYSIS_CONFIG"));

    const QByteArray diagnosticsSettingsFile = qgetenv("LIZZIE_DIAGNOSTICS_SETTINGS_FILE");
    printMarkdownField(
        "qt.settings.diagnosticsOverrideFile",
        diagnosticsSettingsFile.isEmpty() ? QStringLiteral("(unset)") : QString::fromLocal8Bit(diagnosticsSettingsFile));
    if (!diagnosticsSettingsFile.isEmpty()) {
        QSettings settings(QString::fromLocal8Bit(diagnosticsSettingsFile), QSettings::IniFormat);
        printMarkdownField("qt.settings.fileName", settings.fileName());
        printMarkdownStoredEnginePathSummary(settings, "qt.settings.engine", QStringLiteral("engine"));
        printMarkdownStoredEnginePathSummary(settings, "qt.settings.compare", QStringLiteral("compare"));
        return;
    }

    QSettings settings;
    printMarkdownField("qt.settings.fileName", settings.fileName());
    printMarkdownStoredEnginePathSummary(settings, "qt.settings.engine", QStringLiteral("engine"));
    printMarkdownStoredEnginePathSummary(settings, "qt.settings.compare", QStringLiteral("compare"));
}

void printMarkdownChecklist(const QStringList& values)
{
    for (const QString& value : values) {
        const QByteArray bytes = value.toUtf8();
        std::cout << "- [ ] " << bytes.constData() << '\n';
    }
}

void printMarkdownChecklist(const QStringList& values, const TargetAcceptanceRecords& records, const QString& section)
{
    for (const QString& value : values) {
        const QString status = checklistItemRecordStatus(records, section, value);
        const QByteArray bytes = value.toUtf8();
        const QByteArray statusBytes = status.toUtf8();
        std::cout << "- [" << (manualAcceptancePassed(status) ? "x" : " ") << "] " << bytes.constData();
        if (manualAcceptanceFailed(status) || manualAcceptanceInvalid(status)) {
            std::cout << " (`manual`: " << statusBytes.constData() << ")";
        }
        std::cout << '\n';
    }
}

void printManualAcceptancePlaceholders()
{
    std::cout << "- Result: pass / fail\n";
    std::cout << "- Notes:\n";
}

void printTargetAcceptanceReport()
{
    const QString platformName = QGuiApplication::platformName();
    const QList<QScreen*> screens = QGuiApplication::screens();
    const QScreen* primaryScreen = QGuiApplication::primaryScreen();
    const QString osFamily = targetOsFamily();
    const bool inFirstReleaseScope = osFamily == "linux" || osFamily == "windows";
    const bool kdeSession = kdeSessionDetected();
    const bool waylandSession = waylandSessionDetected();
    const bool qtWaylandPlatform = qtWaylandPlatformDetected(platformName);
    const bool windowsPlatformPlugin = platformName.startsWith(QStringLiteral("windows"), Qt::CaseInsensitive);
    const QStringList nvidiaSources = nvidiaEnvironmentHintSources();
    const bool nvidiaHintPresent = !nvidiaSources.isEmpty();
    const bool linuxKdeWaylandDetected = osFamily == "linux" && kdeSession && waylandSession && qtWaylandPlatform;
    const bool windowsDetected = osFamily == "windows" && windowsPlatformPlugin;
    const bool linuxNvidiaCandidate = linuxKdeWaylandDetected && nvidiaHintPresent;
    const bool windowsNvidiaCandidate = windowsDetected && nvidiaHintPresent;
    const bool targetPlatformCandidate = linuxNvidiaCandidate || windowsNvidiaCandidate;
    const bool anyDisplay4K = anyScreenIsAtLeast4K(screens);
    const bool anyHighDpiScale = anyScreenScaleAtLeast(screens, 1.5);
    const bool hasPrimaryScreen = primaryScreen != nullptr;
    const bool primaryTargetDisplayCandidate = screenMeetsTargetDisplayCandidate(primaryScreen);
    const bool anyTargetDisplayCandidate = anyScreenMeetsTargetDisplayCandidate(screens);
    const KataGoEnvironmentReadiness kataGo = inspectKataGoEnvironmentReadiness();
    const StoredEngineConfigReadiness storedConfig = inspectStoredEngineConfigReadiness();
    const bool envOrSavedMainConfigReady = kataGo.ready || storedConfig.main.ready;
    const bool envOrSavedMainGtpReady = kataGo.ready || storedConfig.main.gtpReady;
    const bool envOrSavedMainAnalysisReady = kataGo.ready || storedConfig.main.analysisReady;
    const bool configuredDualEngineConfigReady = envOrSavedMainConfigReady && storedConfig.compare.ready;
    const bool configuredDualEngineGtpReady = envOrSavedMainGtpReady && storedConfig.compare.gtpReady;
    const bool configuredDualEngineAnalysisReady =
        envOrSavedMainAnalysisReady && storedConfig.compare.analysisReady;
    const TargetAcceptanceRecords acceptanceRecords = inspectTargetAcceptanceRecords();
    const QString externalVerificationStatus = externalTargetVerificationStatus(acceptanceRecords);
    const QStringList metadataBlockers = targetAcceptanceMetadataBlockers(acceptanceRecords);
    const bool metadataReady = metadataBlockers.isEmpty();
    const QStringList evidenceBlockers = targetAcceptanceEvidenceBlockers(acceptanceRecords);
    const bool evidenceReady = evidenceBlockers.isEmpty();
    const QStringList evidenceSha256Blockers = targetAcceptanceEvidenceSha256Blockers(acceptanceRecords);
    const bool evidenceSha256Ready = evidenceSha256Blockers.isEmpty();
    const QStringList recordTimestampBlockers = targetAcceptanceRecordTimestampBlockers(acceptanceRecords);
    const bool recordTimestampReady = recordTimestampBlockers.isEmpty();
    const QStringList evidenceTimestampBlockers = targetAcceptanceEvidenceTimestampBlockers(acceptanceRecords);
    const bool evidenceTimestampReady = evidenceTimestampBlockers.isEmpty();
    const QStringList checklistBlockers = targetAcceptanceChecklistBlockers(acceptanceRecords);
    const bool checklistReady = checklistBlockers.isEmpty();
    const QStringList missingChecklistRecords = targetAcceptanceMissingChecklistRecords(acceptanceRecords);
    const QStringList failedManualRecords = targetAcceptanceFailedManualRecords(acceptanceRecords);
    const QStringList invalidManualRecords = targetAcceptanceInvalidManualRecords(acceptanceRecords);
    const AcceptanceImageEvidenceInfo screenshotImage =
        inspectAcceptanceImageEvidence(acceptanceRecords, acceptanceRecords.screenshotPath);
    const bool screenshotEvidence4KReady =
        screenshotImage.readable && screenshotImage.pixelsAtLeast4K && screenshotImage.hasPixelVariation;
    const QStringList finalBlockers = finalAcceptanceBlockers(
        targetPlatformCandidate,
        envOrSavedMainGtpReady,
        envOrSavedMainAnalysisReady,
        storedConfig.compare.gtpReady,
        storedConfig.compare.analysisReady,
        anyTargetDisplayCandidate,
        screens.size() > 1,
        manualAcceptancePassed(externalVerificationStatus),
        manualAcceptancePassed(acceptanceRecords.rawKataGoComparison),
        manualAcceptancePassed(acceptanceRecords.manualUiInspection),
        checklistReady,
        metadataReady,
        evidenceReady,
        evidenceSha256Ready,
        recordTimestampReady,
        evidenceTimestampReady,
        screenshotEvidence4KReady);
    const QStringList linuxTargetBlockers = linuxKdeWaylandNvidiaBlockers(
        osFamily,
        kdeSession,
        waylandSession,
        qtWaylandPlatform,
        nvidiaHintPresent);
    const QStringList windowsTargetBlockers =
        windowsNvidiaBlockers(osFamily, windowsPlatformPlugin, nvidiaHintPresent);

    std::cout << "# Target Acceptance Report\n\n";
    std::cout << "Generated by `lizzieyzy_qt --target-acceptance-report`.\n\n";

    std::cout << "## Package\n\n";
    printMarkdownField("app.version", QString::fromLatin1(LIZZIEYZY_QT_VERSION));
    printMarkdownField("generatedUtc", QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs));
    printMarkdownField("app.executable", QCoreApplication::applicationFilePath());
    printMarkdownField("app.dir", QCoreApplication::applicationDirPath());
    printMarkdownField("process.currentWorkingDirectory", QDir::currentPath());
    printMarkdownField("qt.version", QString::fromLatin1(qVersion()));
    printMarkdownField("qt.platform", platformName);
    printMarkdownField("os.prettyName", QSysInfo::prettyProductName());
    printMarkdownField("os.kernelType", QSysInfo::kernelType());
    printMarkdownField("os.kernelVersion", QSysInfo::kernelVersion());
    printMarkdownField("plan.target.osFamily", osFamily);
    printMarkdownField("plan.target.inFirstReleaseScope", inFirstReleaseScope);

    std::cout << "\n## Configured Paths\n\n";
    printMarkdownConfiguredPathSummary();

    std::cout << "\n## Diagnostics\n\n";
    std::cout << "Keep the raw `lizzieyzy_qt --diagnostics` output and real-engine logs with this report.\n\n";
    printMarkdownField("katago.env.status", kataGo.status);
    printMarkdownField("katago.env.ready", kataGo.ready);
    printMarkdownListField("katago.env.missing", kataGo.missing);
    printMarkdownListField("katago.env.invalid", kataGo.invalid);
    printMarkdownField("plan.acceptance.recordFile", acceptanceRecords.fileName);
    printMarkdownField("plan.acceptance.recordFile.set", acceptanceRecords.fileSet);
    printMarkdownField("plan.acceptance.recordFile.exists", acceptanceRecords.fileExists);
    printMarkdownField("plan.acceptance.recordFile.readable", acceptanceRecords.fileReadable);
    const QFileInfo recordFileInfo(acceptanceRecords.fileName);
    const bool recordFileAvailable = acceptanceRecords.fileReadable && recordFileInfo.exists();
    printMarkdownField(
        "plan.acceptance.recordFile.canonicalPath",
        recordFileAvailable ? recordFileInfo.canonicalFilePath() : QStringLiteral("(missing)"));
    printMarkdownField(
        "plan.acceptance.recordFile.size",
        recordFileAvailable ? QString::number(recordFileInfo.size()) : QStringLiteral("-1"));
    printMarkdownField("plan.acceptance.recordFile.sha256", sha256ForReadableNonEmptyFile(acceptanceRecords.fileName));
    printMarkdownField(
        "plan.acceptance.recordFile.lastModifiedUtc",
        recordFileAvailable ? recordFileInfo.lastModified().toUTC().toString(Qt::ISODateWithMs) : QStringLiteral("(missing)"));
    printMarkdownField(
        "plan.acceptance.recordFile.timestampStatus",
        targetAcceptanceRecordFileTimestampStatus(acceptanceRecords));
    printMarkdownField("plan.acceptance.record.completedUtc", acceptanceRecords.completedUtc);
    printMarkdownField(
        "plan.acceptance.record.completedUtcStatus",
        targetAcceptanceCompletedUtcStatus(acceptanceRecords.completedUtc));
    printMarkdownField("plan.acceptance.record.appVersion", acceptanceRecords.appVersion);
    printMarkdownField("plan.acceptance.record.appVersionStatus", targetAcceptanceAppVersionStatus(acceptanceRecords.appVersion));
    printMarkdownField(
        "plan.acceptance.record.appExecutableSha256",
        recordSha256DisplayText(acceptanceRecords.appExecutableSha256));
    printMarkdownField(
        "plan.acceptance.record.appExecutableSha256Status",
        targetAcceptanceAppExecutableSha256Status(acceptanceRecords.appExecutableSha256));
    printMarkdownField("plan.acceptance.record.osPrettyName", acceptanceRecords.osPrettyName);
    printMarkdownField(
        "plan.acceptance.record.osPrettyNameStatus",
        targetAcceptanceOsPrettyNameStatus(acceptanceRecords.osPrettyName));
    printMarkdownField("plan.acceptance.record.osKernelType", acceptanceRecords.osKernelType);
    printMarkdownField(
        "plan.acceptance.record.osKernelTypeStatus",
        targetAcceptanceOsKernelTypeStatus(acceptanceRecords.osKernelType));
    printMarkdownField("plan.acceptance.record.osKernelVersion", acceptanceRecords.osKernelVersion);
    printMarkdownField(
        "plan.acceptance.record.osKernelVersionStatus",
        targetAcceptanceOsKernelVersionStatus(acceptanceRecords.osKernelVersion));
    printMarkdownField("plan.acceptance.record.qtRuntimeVersion", acceptanceRecords.qtRuntimeVersion);
    printMarkdownField(
        "plan.acceptance.record.qtRuntimeVersionStatus",
        targetAcceptanceQtRuntimeVersionStatus(acceptanceRecords.qtRuntimeVersion));
    printMarkdownField("plan.acceptance.record.qtBuildAbi", acceptanceRecords.qtBuildAbi);
    printMarkdownField(
        "plan.acceptance.record.qtBuildAbiStatus",
        targetAcceptanceQtBuildAbiStatus(acceptanceRecords.qtBuildAbi));
    printMarkdownField("plan.acceptance.record.cpuArchitecture", acceptanceRecords.cpuArchitecture);
    printMarkdownField(
        "plan.acceptance.record.cpuArchitectureStatus",
        targetAcceptanceCpuArchitectureStatus(acceptanceRecords.cpuArchitecture));
    printMarkdownField("plan.acceptance.record.reviewer", acceptanceRecords.reviewer);
    printMarkdownField("plan.acceptance.record.targetMachine", acceptanceRecords.targetMachine);
    printMarkdownField(
        "plan.acceptance.record.targetMachineStatus",
        targetAcceptanceTargetMachineStatus(acceptanceRecords.targetMachine));
    printMarkdownField("plan.acceptance.record.gpuDriver", acceptanceRecords.gpuDriver);
    printMarkdownField(
        "plan.acceptance.record.gpuDriverStatus",
        targetAcceptanceGpuDriverStatus(acceptanceRecords.gpuDriver));
    printMarkdownField("plan.acceptance.record.kataGoVersion", acceptanceRecords.kataGoVersion);
    printMarkdownField(
        "plan.acceptance.record.kataGoVersionStatus",
        targetAcceptanceKataGoVersionStatus(acceptanceRecords.kataGoVersion));
    printMarkdownField("plan.acceptance.record.notes", acceptanceRecords.notes);
    printMarkdownField("plan.acceptance.recordMetadata.ready", metadataReady);
    printMarkdownListField("plan.acceptance.recordMetadata.blocker", metadataBlockers);
    printMarkdownAcceptanceRecordHintFields();
    printMarkdownEvidencePath(
        "plan.acceptance.evidence.diagnostics",
        acceptanceRecords,
        acceptanceRecords.diagnosticsPath,
        acceptanceRecords.diagnosticsSha256,
        diagnosticsEvidenceContentMarkers(acceptanceRecords));
    printMarkdownEvidencePath(
        "plan.acceptance.evidence.targetAcceptanceReport",
        acceptanceRecords,
        acceptanceRecords.targetAcceptanceReportPath,
        acceptanceRecords.targetAcceptanceReportSha256,
        targetAcceptanceReportEvidenceContentMarkers(acceptanceRecords));
    printMarkdownEvidencePath(
        "plan.acceptance.evidence.engineLog",
        acceptanceRecords,
        acceptanceRecords.engineLogPath,
        acceptanceRecords.engineLogSha256,
        engineLogEvidenceContentMarkers(acceptanceRecords));
    printMarkdownField(
        "plan.acceptance.evidence.engineLog.gpuOrStderrContentStatus",
        acceptanceEvidenceAnyContentStatus(
            acceptanceRecords,
            acceptanceRecords.engineLogPath,
            engineLogGpuOrStderrEvidenceContentMarkers()));
    printMarkdownListField(
        "plan.acceptance.evidence.engineLog.missingGpuOrStderrContentMarker",
        acceptanceEvidenceMissingAnyMarkers(
            acceptanceRecords,
            acceptanceRecords.engineLogPath,
            engineLogGpuOrStderrEvidenceContentMarkers()));
    printMarkdownEvidencePath(
        "plan.acceptance.evidence.rawKataGoComparisonLog",
        acceptanceRecords,
        acceptanceRecords.rawKataGoComparisonLogPath,
        acceptanceRecords.rawKataGoComparisonLogSha256,
        rawKataGoComparisonLogEvidenceContentMarkers(acceptanceRecords));
    printMarkdownEvidencePath(
        "plan.acceptance.evidence.manualUiInspectionLog",
        acceptanceRecords,
        acceptanceRecords.manualUiInspectionLogPath,
        acceptanceRecords.manualUiInspectionLogSha256,
        manualUiInspectionLogEvidenceContentMarkers());
    printMarkdownEvidencePath(
        "plan.acceptance.evidence.externalTargetVerificationLog",
        acceptanceRecords,
        acceptanceRecords.externalTargetVerificationLogPath,
        acceptanceRecords.externalTargetVerificationLogSha256,
        externalTargetVerificationLogEvidenceContentMarkers());
    printMarkdownEvidencePath(
        "plan.acceptance.evidence.screenshot",
        acceptanceRecords,
        acceptanceRecords.screenshotPath,
        acceptanceRecords.screenshotSha256);
    printMarkdownImageEvidence(
        "plan.acceptance.evidence.screenshot",
        screenshotImage);
    printMarkdownField("plan.acceptance.evidence.screenshot.readyFor4KAcceptance", screenshotEvidence4KReady);
    printMarkdownField("plan.acceptance.evidence.ready", evidenceReady);
    printMarkdownListField("plan.acceptance.evidence.blocker", evidenceBlockers);
    printMarkdownField("plan.acceptance.evidenceSha256.ready", evidenceSha256Ready);
    printMarkdownListField("plan.acceptance.evidenceSha256.blocker", evidenceSha256Blockers);
    printMarkdownField("plan.acceptance.recordTimestamp.ready", recordTimestampReady);
    printMarkdownListField("plan.acceptance.recordTimestamp.blocker", recordTimestampBlockers);
    printMarkdownField("plan.acceptance.evidenceTimestamp.ready", evidenceTimestampReady);
    printMarkdownListField("plan.acceptance.evidenceTimestamp.blocker", evidenceTimestampBlockers);
    printMarkdownField("plan.acceptance.linuxKdeWaylandNvidiaManualResult", acceptanceRecords.linuxKdeWaylandNvidia);
    printMarkdownField("plan.acceptance.windowsNvidiaManualResult", acceptanceRecords.windowsNvidia);
    printMarkdownField("plan.acceptance.physicalDisplayManualResult", acceptanceRecords.physicalDisplay);
    printMarkdownField("plan.acceptance.externalTargetVerificationManualResult", acceptanceRecords.externalTargetVerification);
    printMarkdownListField("plan.acceptance.checklistPassedRecord", acceptanceRecords.passedChecklistItems);
    printMarkdownListField("plan.acceptance.checklistFailedRecord", acceptanceRecords.failedChecklistItems);
    printMarkdownListField("plan.acceptance.checklistInvalidRecord", acceptanceRecords.invalidChecklistItems);
    printMarkdownListField("plan.acceptance.checklistMissingRecord", missingChecklistRecords);
    printMarkdownListField("plan.acceptance.manualFailedRecord", failedManualRecords);
    printMarkdownListField("plan.acceptance.manualInvalidRecord", invalidManualRecords);
    printMarkdownField("plan.acceptance.checklist.ready", checklistReady);
    printMarkdownListField("plan.acceptance.checklist.blocker", checklistBlockers);
    printMarkdownField("plan.acceptance.realKataGoEnvReady", kataGo.ready);
    printMarkdownField("plan.acceptance.realKataGoEnvStatus", kataGo.status);
    printMarkdownField("plan.acceptance.savedMainConfigReady", storedConfig.main.ready);
    printMarkdownField("plan.acceptance.savedMainGtpReady", storedConfig.main.gtpReady);
    printMarkdownField("plan.acceptance.savedMainAnalysisReady", storedConfig.main.analysisReady);
    printMarkdownField("plan.acceptance.savedMainConfigStatus", storedConfig.main.status);
    printMarkdownField("plan.acceptance.savedCompareConfigReady", storedConfig.compare.ready);
    printMarkdownField("plan.acceptance.savedCompareGtpReady", storedConfig.compare.gtpReady);
    printMarkdownField("plan.acceptance.savedCompareAnalysisReady", storedConfig.compare.analysisReady);
    printMarkdownField("plan.acceptance.savedCompareConfigStatus", storedConfig.compare.status);
    printMarkdownField("plan.acceptance.envOrSavedMainConfigReady", envOrSavedMainConfigReady);
    printMarkdownField("plan.acceptance.envOrSavedMainGtpReady", envOrSavedMainGtpReady);
    printMarkdownField("plan.acceptance.envOrSavedMainAnalysisReady", envOrSavedMainAnalysisReady);
    printMarkdownField(
        "plan.acceptance.configuredMainConfigSource",
        configuredSourceText(kataGo.ready, storedConfig.main.ready));
    printMarkdownField(
        "plan.acceptance.configuredMainGtpSource",
        configuredSourceText(kataGo.ready, storedConfig.main.gtpReady));
    printMarkdownField(
        "plan.acceptance.configuredMainAnalysisSource",
        configuredSourceText(kataGo.ready, storedConfig.main.analysisReady));
    printMarkdownField(
        "plan.acceptance.configuredCompareConfigSource",
        configuredSourceText(false, storedConfig.compare.ready));
    printMarkdownField(
        "plan.acceptance.configuredCompareGtpSource",
        configuredSourceText(false, storedConfig.compare.gtpReady));
    printMarkdownField(
        "plan.acceptance.configuredCompareAnalysisSource",
        configuredSourceText(false, storedConfig.compare.analysisReady));
    printMarkdownField("plan.acceptance.configuredDualEngineConfigReady", configuredDualEngineConfigReady);
    printMarkdownField("plan.acceptance.configuredDualEngineGtpReady", configuredDualEngineGtpReady);
    printMarkdownField("plan.acceptance.configuredDualEngineAnalysisReady", configuredDualEngineAnalysisReady);
    printMarkdownField(
        "plan.acceptance.configuredStatus",
        planConfiguredAcceptanceStatus(targetPlatformCandidate, envOrSavedMainConfigReady, anyTargetDisplayCandidate));
    printMarkdownField(
        "plan.acceptance.status",
        planAcceptanceStatus(targetPlatformCandidate, kataGo.ready, anyTargetDisplayCandidate));
    printMarkdownField("plan.acceptance.targetPlatformCandidate", targetPlatformCandidate);
    printMarkdownField("plan.acceptance.realKataGoTargetRunCandidate", targetPlatformCandidate && kataGo.ready);
    printMarkdownField("plan.acceptance.realKataGoManualVerificationCandidate",
                       targetPlatformCandidate && kataGo.ready && anyTargetDisplayCandidate);
    printMarkdownField(
        "plan.acceptance.configuredKataGoTargetRunCandidate",
        targetPlatformCandidate && envOrSavedMainConfigReady);
    printMarkdownField("plan.acceptance.configuredManualVerificationCandidate",
                       targetPlatformCandidate && envOrSavedMainConfigReady && anyTargetDisplayCandidate);
    printMarkdownField(
        "plan.acceptance.configuredRealtimeGtpManualVerificationCandidate",
        targetPlatformCandidate && envOrSavedMainGtpReady && anyTargetDisplayCandidate);
    printMarkdownField(
        "plan.acceptance.configuredBatchAnalysisManualVerificationCandidate",
        targetPlatformCandidate && envOrSavedMainAnalysisReady && anyTargetDisplayCandidate);
    printMarkdownField(
        "plan.acceptance.configuredDualEngineTargetRunCandidate",
        targetPlatformCandidate && configuredDualEngineConfigReady);
    printMarkdownField("plan.acceptance.configuredDualEngineManualVerificationCandidate",
                       targetPlatformCandidate && configuredDualEngineConfigReady && anyTargetDisplayCandidate);
    printMarkdownField("plan.acceptance.configuredRealtimeGtpTargetRunCandidate",
                       targetPlatformCandidate && envOrSavedMainGtpReady);
    printMarkdownField("plan.acceptance.configuredBatchAnalysisTargetRunCandidate",
                       targetPlatformCandidate && envOrSavedMainAnalysisReady);
    printMarkdownField("plan.acceptance.configuredDualRealtimeGtpTargetRunCandidate",
                       targetPlatformCandidate && configuredDualEngineGtpReady);
    printMarkdownField(
        "plan.acceptance.configuredDualRealtimeGtpManualVerificationCandidate",
        targetPlatformCandidate && configuredDualEngineGtpReady && anyTargetDisplayCandidate);
    printMarkdownField("plan.acceptance.configuredDualBatchAnalysisTargetRunCandidate",
                       targetPlatformCandidate && configuredDualEngineAnalysisReady);
    printMarkdownField(
        "plan.acceptance.configuredDualBatchAnalysisManualVerificationCandidate",
        targetPlatformCandidate && configuredDualEngineAnalysisReady && anyTargetDisplayCandidate);
    printMarkdownListField(
        "plan.acceptance.targetPlatformBlocker",
        firstReleaseNvidiaPlatformBlockers(linuxNvidiaCandidate, windowsNvidiaCandidate));
    printMarkdownField("plan.target.acceptance.linuxKdeWaylandNvidiaCandidate", linuxNvidiaCandidate);
    printMarkdownField("plan.target.acceptance.windowsNvidiaCandidate", windowsNvidiaCandidate);
    printMarkdownField(
        "plan.target.acceptance.firstReleaseNvidiaPlatformCandidate",
        linuxNvidiaCandidate || windowsNvidiaCandidate);
    printMarkdownListField(
        "plan.target.acceptance.firstReleaseNvidiaPlatformBlocker",
        firstReleaseNvidiaPlatformBlockers(linuxNvidiaCandidate, windowsNvidiaCandidate));
    printMarkdownField("plan.target.acceptance.display4KCandidate", anyDisplay4K);
    printMarkdownField("plan.target.acceptance.highDpiCandidate", anyHighDpiScale);
    printMarkdownField("plan.target.acceptance.targetDisplayCandidate", anyTargetDisplayCandidate);
    printMarkdownListField(
        "plan.target.acceptance.targetDisplayBlocker",
        targetDisplayBlockers(screens, anyDisplay4K, anyHighDpiScale, anyTargetDisplayCandidate));
    printMarkdownListField("plan.target.acceptance.multiDisplayBlocker", multiDisplayBlockers(screens));
    printMarkdownField("plan.acceptance.display4KCandidate", anyDisplay4K);
    printMarkdownField("plan.acceptance.highDpiCandidate", anyHighDpiScale);
    printMarkdownField("plan.acceptance.targetDisplayCandidate", anyTargetDisplayCandidate);
    printMarkdownField("plan.acceptance.sameScreenTargetDisplayCandidate", anyTargetDisplayCandidate);
    printMarkdownField("plan.acceptance.multiDisplayCandidate", screens.size() > 1);
    printMarkdownListField(
        "plan.acceptance.targetDisplayBlocker",
        targetDisplayBlockers(screens, anyDisplay4K, anyHighDpiScale, anyTargetDisplayCandidate));
    printMarkdownListField("plan.acceptance.multiDisplayBlocker", multiDisplayBlockers(screens));
    printMarkdownListField(
        "plan.acceptance.realKataGoManualVerificationBlocker",
        manualVerificationBlockers(
            targetPlatformCandidate,
            kataGo.ready,
            anyTargetDisplayCandidate,
            QStringLiteral("kataGoEnv")));
    printMarkdownListField(
        "plan.acceptance.configuredManualVerificationBlocker",
        manualVerificationBlockers(
            targetPlatformCandidate,
            envOrSavedMainConfigReady,
            anyTargetDisplayCandidate,
            QStringLiteral("kataGoConfig")));
    printMarkdownListField(
        "plan.acceptance.configuredDualEngineManualVerificationBlocker",
        dualEngineManualVerificationBlockers(
            targetPlatformCandidate,
            envOrSavedMainConfigReady,
            storedConfig.compare.ready,
            anyTargetDisplayCandidate));
    printMarkdownListField(
        "plan.acceptance.externalTargetVerificationChecklist",
        externalTargetVerificationChecklist());
    printMarkdownField("plan.acceptance.externalTargetVerificationRequired", true);
    printMarkdownField("plan.acceptance.externalTargetVerificationStatus", externalVerificationStatus);
    printMarkdownListField(
        "plan.acceptance.rawKataGoComparisonChecklist",
        rawKataGoComparisonChecklist());
    printMarkdownField("plan.acceptance.manualRawEngineComparisonRequired", true);
    printMarkdownField("plan.acceptance.rawKataGoComparisonStatus", acceptanceRecords.rawKataGoComparison);
    printMarkdownField("plan.acceptance.manualUiInspectionRequired", true);
    printMarkdownField("plan.acceptance.manualUiInspectionStatus", acceptanceRecords.manualUiInspection);
    printMarkdownField("plan.acceptance.finalAcceptanceStatus", finalAcceptanceStatus(finalBlockers, acceptanceRecords));
    printMarkdownListField("plan.acceptance.finalAcceptanceBlocker", finalBlockers);

    std::cout << "\n## Linux KDE Wayland + NVIDIA\n\n";
    printMarkdownField("plan.target.linuxKdeWayland.kdeSession", kdeSession);
    printMarkdownField("plan.target.linuxKdeWayland.waylandSession", waylandSession);
    printMarkdownField("plan.target.linuxKdeWayland.qtPlatformWayland", qtWaylandPlatform);
    printMarkdownField("plan.target.linuxKdeWayland.detected", linuxKdeWaylandDetected);
    printMarkdownField("plan.target.nvidiaEnvironmentHint.present", nvidiaHintPresent);
    printMarkdownListField("plan.target.nvidiaEnvironmentHint.source", nvidiaSources);
    printMarkdownField("plan.acceptance.linuxKdeWaylandNvidiaCandidate", linuxNvidiaCandidate);
    printMarkdownField("plan.acceptance.linuxKdeWaylandNvidiaManualResult", acceptanceRecords.linuxKdeWaylandNvidia);
    printMarkdownListField(
        "plan.target.acceptance.linuxKdeWaylandNvidiaBlocker",
        linuxTargetBlockers);
    printMarkdownField(
        "plan.acceptance.linuxKdeWaylandNvidiaVerificationStatus",
        targetRealtimeVerificationStatus(
            linuxNvidiaCandidate,
            envOrSavedMainGtpReady,
            QStringLiteral("linux-kde-wayland-nvidia")));
    printMarkdownListField(
        "plan.acceptance.linuxKdeWaylandNvidiaVerificationReadinessBlocker",
        targetRealtimeVerificationReadinessBlockers(linuxTargetBlockers, envOrSavedMainGtpReady));
    printMarkdownListField(
        "plan.acceptance.linuxKdeWaylandNvidiaVerificationChecklist",
        linuxKdeWaylandNvidiaVerificationChecklist());
    printMarkdownChecklist(
        linuxKdeWaylandNvidiaVerificationChecklist(),
        acceptanceRecords,
        QStringLiteral("linuxKdeWaylandNvidia"));
    printManualAcceptancePlaceholders();

    std::cout << "\n## Windows + NVIDIA\n\n";
    printMarkdownField("plan.target.windows.nativePlatformPlugin", windowsPlatformPlugin);
    printMarkdownField("plan.target.windows.detected", windowsDetected);
    printMarkdownField("plan.acceptance.windowsNvidiaCandidate", windowsNvidiaCandidate);
    printMarkdownField("plan.acceptance.windowsNvidiaManualResult", acceptanceRecords.windowsNvidia);
    printMarkdownListField(
        "plan.target.acceptance.windowsNvidiaBlocker",
        windowsTargetBlockers);
    printMarkdownField(
        "plan.acceptance.windowsNvidiaVerificationStatus",
        targetRealtimeVerificationStatus(
            windowsNvidiaCandidate,
            envOrSavedMainGtpReady,
            QStringLiteral("windows-nvidia")));
    printMarkdownListField(
        "plan.acceptance.windowsNvidiaVerificationReadinessBlocker",
        targetRealtimeVerificationReadinessBlockers(windowsTargetBlockers, envOrSavedMainGtpReady));
    printMarkdownListField(
        "plan.acceptance.windowsNvidiaVerificationChecklist",
        windowsNvidiaVerificationChecklist());
    printMarkdownChecklist(windowsNvidiaVerificationChecklist(), acceptanceRecords, QStringLiteral("windowsNvidia"));
    printManualAcceptancePlaceholders();

    std::cout << "\n## High DPI And Multi-Display\n\n";
    printMarkdownField("plan.target.display.screenCount", QString::number(static_cast<qlonglong>(screens.size())));
    printMarkdownField("plan.target.display.multiScreen", screens.size() > 1);
    printMarkdownField("plan.target.display.primaryDevicePixelsAtLeast4K", screenIsAtLeast4K(primaryScreen));
    printMarkdownField("plan.target.display.anyDevicePixelsAtLeast4K", anyDisplay4K);
    printMarkdownField("plan.target.display.primaryScaleAtLeast1_5",
                       hasPrimaryScreen && primaryScreen->devicePixelRatio() >= 1.5);
    printMarkdownField("plan.target.display.anyScaleAtLeast1_5", anyHighDpiScale);
    printMarkdownField("plan.target.display.primaryTargetScreenCandidate", primaryTargetDisplayCandidate);
    printMarkdownField("plan.target.display.anyTargetScreenCandidate", anyTargetDisplayCandidate);
    printMarkdownField(
        "plan.acceptance.physicalDisplayVerificationStatus",
        physicalDisplayVerificationStatus(anyTargetDisplayCandidate, screens.size() > 1));
    printMarkdownField("plan.acceptance.physicalDisplayManualResult", acceptanceRecords.physicalDisplay);
    printMarkdownListField(
        "plan.acceptance.physicalDisplayVerificationReadinessBlocker",
        physicalDisplayVerificationReadinessBlockers(
            screens,
            anyDisplay4K,
            anyHighDpiScale,
            anyTargetDisplayCandidate));
    printMarkdownListField(
        "plan.acceptance.physicalDisplayVerificationChecklist",
        physicalDisplayVerificationChecklist());
    printMarkdownChecklist(physicalDisplayVerificationChecklist(), acceptanceRecords, QStringLiteral("physicalDisplay"));
    printManualAcceptancePlaceholders();

    std::cout << "\n## Raw KataGo Comparison\n\n";
    printMarkdownField("plan.acceptance.rawKataGoComparisonStatus", acceptanceRecords.rawKataGoComparison);
    printMarkdownListField("plan.acceptance.rawKataGoComparisonChecklist", rawKataGoComparisonChecklist());
    printMarkdownChecklist(rawKataGoComparisonChecklist(), acceptanceRecords, QStringLiteral("rawKataGoComparison"));
    printManualAcceptancePlaceholders();

    std::cout << "\n## External Target Verification\n\n";
    printMarkdownListField(
        "plan.acceptance.externalTargetVerificationChecklist",
        externalTargetVerificationChecklist());
    printMarkdownField("plan.acceptance.externalTargetVerificationStatus", externalVerificationStatus);
    printMarkdownChecklist(
        externalTargetVerificationChecklist(),
        acceptanceRecords,
        QStringLiteral("externalTargetVerification"));
    printManualAcceptancePlaceholders();

    std::cout << "\n## Manual UI Inspection\n\n";
    printMarkdownField("plan.acceptance.manualUiInspectionStatus", acceptanceRecords.manualUiInspection);
    printMarkdownListField("plan.acceptance.manualUiInspectionChecklist", manualUiInspectionChecklist());
    printMarkdownChecklist(manualUiInspectionChecklist(), acceptanceRecords, QStringLiteral("manualUiInspection"));
    printManualAcceptancePlaceholders();

    std::cout << "\n## Final Acceptance\n\n";
    std::cout << "- KDE Wayland + NVIDIA acceptance: pass / fail\n";
    std::cout << "- Windows + NVIDIA acceptance: pass / fail\n";
    std::cout << "- 4K/high-DPI/multi-display UI acceptance: pass / fail\n";
    std::cout << "- Raw KataGo UI comparison acceptance: pass / fail\n";
    printMarkdownField("plan.acceptance.finalAcceptanceStatus", finalAcceptanceStatus(finalBlockers, acceptanceRecords));
    printMarkdownListField("plan.acceptance.finalAcceptanceBlocker", finalBlockers);
    printMarkdownChecklist(finalBlockers);
    printMarkdownListField("plan.acceptance.finalAcceptanceOutstandingBlocker", finalBlockers);
}

void printTargetPlatformSummaryDiagnostics(
    const QString& platformName,
    const QList<QScreen*>& screens,
    const QScreen* primaryScreen)
{
    const QString osFamily = targetOsFamily();
    const bool inFirstReleaseScope = osFamily == "linux" || osFamily == "windows";
    const bool kdeSession = kdeSessionDetected();
    const bool waylandSession = waylandSessionDetected();
    const bool qtWaylandPlatform = qtWaylandPlatformDetected(platformName);
    const bool windowsPlatformPlugin = platformName.startsWith(QStringLiteral("windows"), Qt::CaseInsensitive);
    const QStringList nvidiaSources = nvidiaEnvironmentHintSources();
    const bool nvidiaHintPresent = !nvidiaSources.isEmpty();
    const bool hasPrimaryScreen = primaryScreen != nullptr;
    const bool linuxKdeWaylandDetected = osFamily == "linux" && kdeSession && waylandSession && qtWaylandPlatform;
    const bool windowsDetected = osFamily == "windows" && windowsPlatformPlugin;
    const bool anyDisplay4K = anyScreenIsAtLeast4K(screens);
    const bool anyHighDpiScale = anyScreenScaleAtLeast(screens, 1.5);
    const bool primaryTargetDisplayCandidate = screenMeetsTargetDisplayCandidate(primaryScreen);
    const bool anyTargetDisplayCandidate = anyScreenMeetsTargetDisplayCandidate(screens);
    const bool linuxNvidiaCandidate = linuxKdeWaylandDetected && nvidiaHintPresent;
    const bool windowsNvidiaCandidate = windowsDetected && nvidiaHintPresent;

    printText("plan.target.osFamily", osFamily);
    printBool("plan.target.inFirstReleaseScope", inFirstReleaseScope);
    printBool("plan.target.linuxKdeWayland.kdeSession", kdeSession);
    printBool("plan.target.linuxKdeWayland.waylandSession", waylandSession);
    printBool("plan.target.linuxKdeWayland.qtPlatformWayland", qtWaylandPlatform);
    printBool("plan.target.linuxKdeWayland.detected", linuxKdeWaylandDetected);
    printBool("plan.target.windows.nativePlatformPlugin", windowsPlatformPlugin);
    printBool("plan.target.windows.detected", windowsDetected);
    printBool("plan.target.nvidiaEnvironmentHint.present", nvidiaHintPresent);
    printStringList("plan.target.nvidiaEnvironmentHint.source", nvidiaSources);
    std::cout << "plan.target.display.screenCount: " << screens.size() << '\n';
    printBool("plan.target.display.multiScreen", screens.size() > 1);
    printBool("plan.target.display.primaryDevicePixelsAtLeast4K", screenIsAtLeast4K(primaryScreen));
    printBool("plan.target.display.anyDevicePixelsAtLeast4K", anyDisplay4K);
    printBool(
        "plan.target.display.primaryScaleAtLeast1_5",
        hasPrimaryScreen && primaryScreen->devicePixelRatio() >= 1.5);
    printBool("plan.target.display.anyScaleAtLeast1_5", anyHighDpiScale);
    printBool("plan.target.display.primaryTargetScreenCandidate", primaryTargetDisplayCandidate);
    printBool("plan.target.display.anyTargetScreenCandidate", anyTargetDisplayCandidate);
    printBool("plan.target.acceptance.linuxKdeWaylandNvidiaCandidate", linuxNvidiaCandidate);
    printStringList(
        "plan.target.acceptance.linuxKdeWaylandNvidiaBlocker",
        linuxKdeWaylandNvidiaBlockers(
            osFamily,
            kdeSession,
            waylandSession,
            qtWaylandPlatform,
            nvidiaHintPresent));
    printBool("plan.target.acceptance.windowsNvidiaCandidate", windowsNvidiaCandidate);
    printStringList(
        "plan.target.acceptance.windowsNvidiaBlocker",
        windowsNvidiaBlockers(osFamily, windowsPlatformPlugin, nvidiaHintPresent));
    printBool(
        "plan.target.acceptance.firstReleaseNvidiaPlatformCandidate",
        linuxNvidiaCandidate || windowsNvidiaCandidate);
    printStringList(
        "plan.target.acceptance.firstReleaseNvidiaPlatformBlocker",
        firstReleaseNvidiaPlatformBlockers(linuxNvidiaCandidate, windowsNvidiaCandidate));
    printBool("plan.target.acceptance.display4KCandidate", anyDisplay4K);
    printBool("plan.target.acceptance.highDpiCandidate", anyHighDpiScale);
    printBool(
        "plan.target.acceptance.targetDisplayCandidate",
        anyTargetDisplayCandidate);
    printStringList(
        "plan.target.acceptance.targetDisplayBlocker",
        targetDisplayBlockers(screens, anyDisplay4K, anyHighDpiScale, anyTargetDisplayCandidate));
    printStringList("plan.target.acceptance.multiDisplayBlocker", multiDisplayBlockers(screens));
}

void printPlanAcceptanceSummaryDiagnostics(
    const QString& platformName,
    const QList<QScreen*>& screens,
    const QScreen* primaryScreen)
{
    const QString osFamily = targetOsFamily();
    const bool kdeSession = kdeSessionDetected();
    const bool waylandSession = waylandSessionDetected();
    const bool qtWaylandPlatform = qtWaylandPlatformDetected(platformName);
    const bool windowsPlatformPlugin = platformName.startsWith(QStringLiteral("windows"), Qt::CaseInsensitive);
    const bool nvidiaHintPresent = !nvidiaEnvironmentHintSources().isEmpty();
    const bool linuxKdeWaylandDetected = osFamily == "linux" && kdeSession && waylandSession && qtWaylandPlatform;
    const bool windowsDetected = osFamily == "windows" && windowsPlatformPlugin;
    const bool linuxNvidiaCandidate = linuxKdeWaylandDetected && nvidiaHintPresent;
    const bool windowsNvidiaCandidate = windowsDetected && nvidiaHintPresent;
    const bool targetPlatformCandidate = linuxNvidiaCandidate || windowsNvidiaCandidate;
    const bool anyDisplay4K = anyScreenIsAtLeast4K(screens);
    const bool anyHighDpiScale = anyScreenScaleAtLeast(screens, 1.5);
    const bool anyTargetDisplayCandidate = anyScreenMeetsTargetDisplayCandidate(screens);
    const KataGoEnvironmentReadiness kataGo = inspectKataGoEnvironmentReadiness();
    const StoredEngineConfigReadiness storedConfig = inspectStoredEngineConfigReadiness();
    const bool envOrSavedMainConfigReady = kataGo.ready || storedConfig.main.ready;
    const bool envOrSavedMainGtpReady = kataGo.ready || storedConfig.main.gtpReady;
    const bool envOrSavedMainAnalysisReady = kataGo.ready || storedConfig.main.analysisReady;
    const bool configuredDualEngineConfigReady = envOrSavedMainConfigReady && storedConfig.compare.ready;
    const bool configuredDualEngineGtpReady = envOrSavedMainGtpReady && storedConfig.compare.gtpReady;
    const bool configuredDualEngineAnalysisReady =
        envOrSavedMainAnalysisReady && storedConfig.compare.analysisReady;
    const TargetAcceptanceRecords acceptanceRecords = inspectTargetAcceptanceRecords();
    const QString externalVerificationStatus = externalTargetVerificationStatus(acceptanceRecords);
    const QStringList metadataBlockers = targetAcceptanceMetadataBlockers(acceptanceRecords);
    const bool metadataReady = metadataBlockers.isEmpty();
    const QStringList evidenceBlockers = targetAcceptanceEvidenceBlockers(acceptanceRecords);
    const bool evidenceReady = evidenceBlockers.isEmpty();
    const QStringList evidenceSha256Blockers = targetAcceptanceEvidenceSha256Blockers(acceptanceRecords);
    const bool evidenceSha256Ready = evidenceSha256Blockers.isEmpty();
    const QStringList recordTimestampBlockers = targetAcceptanceRecordTimestampBlockers(acceptanceRecords);
    const bool recordTimestampReady = recordTimestampBlockers.isEmpty();
    const QStringList evidenceTimestampBlockers = targetAcceptanceEvidenceTimestampBlockers(acceptanceRecords);
    const bool evidenceTimestampReady = evidenceTimestampBlockers.isEmpty();
    const QStringList checklistBlockers = targetAcceptanceChecklistBlockers(acceptanceRecords);
    const bool checklistReady = checklistBlockers.isEmpty();
    const QStringList missingChecklistRecords = targetAcceptanceMissingChecklistRecords(acceptanceRecords);
    const QStringList failedManualRecords = targetAcceptanceFailedManualRecords(acceptanceRecords);
    const QStringList invalidManualRecords = targetAcceptanceInvalidManualRecords(acceptanceRecords);

    printText("plan.acceptance.recordFile", acceptanceRecords.fileName);
    printBool("plan.acceptance.recordFile.set", acceptanceRecords.fileSet);
    printBool("plan.acceptance.recordFile.exists", acceptanceRecords.fileExists);
    printBool("plan.acceptance.recordFile.readable", acceptanceRecords.fileReadable);
    const QFileInfo recordFileInfo(acceptanceRecords.fileName);
    const bool recordFileAvailable = acceptanceRecords.fileReadable && recordFileInfo.exists();
    printText(
        "plan.acceptance.recordFile.canonicalPath",
        recordFileAvailable ? recordFileInfo.canonicalFilePath() : QStringLiteral("(missing)"));
    std::cout << "plan.acceptance.recordFile.size: " << (recordFileAvailable ? recordFileInfo.size() : -1) << '\n';
    printText("plan.acceptance.recordFile.sha256", sha256ForReadableNonEmptyFile(acceptanceRecords.fileName));
    printText(
        "plan.acceptance.recordFile.lastModifiedUtc",
        recordFileAvailable ? recordFileInfo.lastModified().toUTC().toString(Qt::ISODateWithMs) : QStringLiteral("(missing)"));
    printText(
        "plan.acceptance.recordFile.timestampStatus",
        targetAcceptanceRecordFileTimestampStatus(acceptanceRecords));
    printText("plan.acceptance.record.completedUtc", acceptanceRecords.completedUtc);
    printText("plan.acceptance.record.completedUtcStatus", targetAcceptanceCompletedUtcStatus(acceptanceRecords.completedUtc));
    printText("plan.acceptance.record.appVersion", acceptanceRecords.appVersion);
    printText("plan.acceptance.record.appVersionStatus", targetAcceptanceAppVersionStatus(acceptanceRecords.appVersion));
    printText(
        "plan.acceptance.record.appExecutableSha256",
        recordSha256DisplayText(acceptanceRecords.appExecutableSha256));
    printText(
        "plan.acceptance.record.appExecutableSha256Status",
        targetAcceptanceAppExecutableSha256Status(acceptanceRecords.appExecutableSha256));
    printText("plan.acceptance.record.osPrettyName", acceptanceRecords.osPrettyName);
    printText(
        "plan.acceptance.record.osPrettyNameStatus",
        targetAcceptanceOsPrettyNameStatus(acceptanceRecords.osPrettyName));
    printText("plan.acceptance.record.osKernelType", acceptanceRecords.osKernelType);
    printText(
        "plan.acceptance.record.osKernelTypeStatus",
        targetAcceptanceOsKernelTypeStatus(acceptanceRecords.osKernelType));
    printText("plan.acceptance.record.osKernelVersion", acceptanceRecords.osKernelVersion);
    printText(
        "plan.acceptance.record.osKernelVersionStatus",
        targetAcceptanceOsKernelVersionStatus(acceptanceRecords.osKernelVersion));
    printText("plan.acceptance.record.qtRuntimeVersion", acceptanceRecords.qtRuntimeVersion);
    printText(
        "plan.acceptance.record.qtRuntimeVersionStatus",
        targetAcceptanceQtRuntimeVersionStatus(acceptanceRecords.qtRuntimeVersion));
    printText("plan.acceptance.record.qtBuildAbi", acceptanceRecords.qtBuildAbi);
    printText(
        "plan.acceptance.record.qtBuildAbiStatus",
        targetAcceptanceQtBuildAbiStatus(acceptanceRecords.qtBuildAbi));
    printText("plan.acceptance.record.cpuArchitecture", acceptanceRecords.cpuArchitecture);
    printText(
        "plan.acceptance.record.cpuArchitectureStatus",
        targetAcceptanceCpuArchitectureStatus(acceptanceRecords.cpuArchitecture));
    printText("plan.acceptance.record.reviewer", acceptanceRecords.reviewer);
    printText("plan.acceptance.record.targetMachine", acceptanceRecords.targetMachine);
    printText(
        "plan.acceptance.record.targetMachineStatus",
        targetAcceptanceTargetMachineStatus(acceptanceRecords.targetMachine));
    printText("plan.acceptance.record.gpuDriver", acceptanceRecords.gpuDriver);
    printText(
        "plan.acceptance.record.gpuDriverStatus",
        targetAcceptanceGpuDriverStatus(acceptanceRecords.gpuDriver));
    printText("plan.acceptance.record.kataGoVersion", acceptanceRecords.kataGoVersion);
    printText(
        "plan.acceptance.record.kataGoVersionStatus",
        targetAcceptanceKataGoVersionStatus(acceptanceRecords.kataGoVersion));
    printText("plan.acceptance.record.notes", acceptanceRecords.notes);
    printBool("plan.acceptance.recordMetadata.ready", metadataReady);
    printStringList("plan.acceptance.recordMetadata.blocker", metadataBlockers);
    printTextAcceptanceRecordHintFields();
    printText("plan.acceptance.evidence.diagnostics.value", recordPathDisplayText(acceptanceRecords.diagnosticsPath));
    printFilesystemPathStatus(
        "plan.acceptance.evidence.diagnostics",
        resolvedAcceptanceEvidencePath(acceptanceRecords, acceptanceRecords.diagnosticsPath));
    printText(
        "plan.acceptance.evidence.diagnostics.sha256",
        acceptanceEvidenceSha256(acceptanceRecords, acceptanceRecords.diagnosticsPath));
    printText(
        "plan.acceptance.evidence.diagnostics.expectedSha256",
        recordSha256DisplayText(acceptanceRecords.diagnosticsSha256));
    printText(
        "plan.acceptance.evidence.diagnostics.sha256Status",
        acceptanceEvidenceSha256Status(
            acceptanceRecords,
            acceptanceRecords.diagnosticsPath,
            acceptanceRecords.diagnosticsSha256));
    printText(
        "plan.acceptance.evidence.diagnostics.timestampStatus",
        acceptanceEvidenceTimestampStatus(acceptanceRecords, acceptanceRecords.diagnosticsPath));
    printText(
        "plan.acceptance.evidence.diagnostics.contentStatus",
        acceptanceEvidenceContentStatus(
            acceptanceRecords,
            acceptanceRecords.diagnosticsPath,
            diagnosticsEvidenceContentMarkers(acceptanceRecords)));
    printStringList(
        "plan.acceptance.evidence.diagnostics.missingContentMarker",
        acceptanceEvidenceMissingMarkers(
            acceptanceRecords,
            acceptanceRecords.diagnosticsPath,
            diagnosticsEvidenceContentMarkers(acceptanceRecords)));
    printText(
        "plan.acceptance.evidence.targetAcceptanceReport.value",
        recordPathDisplayText(acceptanceRecords.targetAcceptanceReportPath));
    printFilesystemPathStatus(
        "plan.acceptance.evidence.targetAcceptanceReport",
        resolvedAcceptanceEvidencePath(acceptanceRecords, acceptanceRecords.targetAcceptanceReportPath));
    printText(
        "plan.acceptance.evidence.targetAcceptanceReport.sha256",
        acceptanceEvidenceSha256(acceptanceRecords, acceptanceRecords.targetAcceptanceReportPath));
    printText(
        "plan.acceptance.evidence.targetAcceptanceReport.expectedSha256",
        recordSha256DisplayText(acceptanceRecords.targetAcceptanceReportSha256));
    printText(
        "plan.acceptance.evidence.targetAcceptanceReport.sha256Status",
        acceptanceEvidenceSha256Status(
            acceptanceRecords,
            acceptanceRecords.targetAcceptanceReportPath,
            acceptanceRecords.targetAcceptanceReportSha256));
    printText(
        "plan.acceptance.evidence.targetAcceptanceReport.timestampStatus",
        acceptanceEvidenceTimestampStatus(acceptanceRecords, acceptanceRecords.targetAcceptanceReportPath));
    printText(
        "plan.acceptance.evidence.targetAcceptanceReport.contentStatus",
        acceptanceEvidenceContentStatus(
            acceptanceRecords,
            acceptanceRecords.targetAcceptanceReportPath,
            targetAcceptanceReportEvidenceContentMarkers(acceptanceRecords)));
    printStringList(
        "plan.acceptance.evidence.targetAcceptanceReport.missingContentMarker",
        acceptanceEvidenceMissingMarkers(
            acceptanceRecords,
            acceptanceRecords.targetAcceptanceReportPath,
            targetAcceptanceReportEvidenceContentMarkers(acceptanceRecords)));
    printText("plan.acceptance.evidence.engineLog.value", recordPathDisplayText(acceptanceRecords.engineLogPath));
    printFilesystemPathStatus(
        "plan.acceptance.evidence.engineLog",
        resolvedAcceptanceEvidencePath(acceptanceRecords, acceptanceRecords.engineLogPath));
    printText(
        "plan.acceptance.evidence.engineLog.sha256",
        acceptanceEvidenceSha256(acceptanceRecords, acceptanceRecords.engineLogPath));
    printText(
        "plan.acceptance.evidence.engineLog.expectedSha256",
        recordSha256DisplayText(acceptanceRecords.engineLogSha256));
    printText(
        "plan.acceptance.evidence.engineLog.sha256Status",
        acceptanceEvidenceSha256Status(
            acceptanceRecords,
            acceptanceRecords.engineLogPath,
            acceptanceRecords.engineLogSha256));
    printText(
        "plan.acceptance.evidence.engineLog.timestampStatus",
        acceptanceEvidenceTimestampStatus(acceptanceRecords, acceptanceRecords.engineLogPath));
    printText(
        "plan.acceptance.evidence.engineLog.contentStatus",
        acceptanceEvidenceContentStatus(
            acceptanceRecords,
            acceptanceRecords.engineLogPath,
            engineLogEvidenceContentMarkers(acceptanceRecords)));
    printStringList(
        "plan.acceptance.evidence.engineLog.missingContentMarker",
        acceptanceEvidenceMissingMarkers(
            acceptanceRecords,
            acceptanceRecords.engineLogPath,
            engineLogEvidenceContentMarkers(acceptanceRecords)));
    printText(
        "plan.acceptance.evidence.engineLog.gpuOrStderrContentStatus",
        acceptanceEvidenceAnyContentStatus(
            acceptanceRecords,
            acceptanceRecords.engineLogPath,
            engineLogGpuOrStderrEvidenceContentMarkers()));
    printStringList(
        "plan.acceptance.evidence.engineLog.missingGpuOrStderrContentMarker",
        acceptanceEvidenceMissingAnyMarkers(
            acceptanceRecords,
            acceptanceRecords.engineLogPath,
            engineLogGpuOrStderrEvidenceContentMarkers()));
    printText(
        "plan.acceptance.evidence.rawKataGoComparisonLog.value",
        recordPathDisplayText(acceptanceRecords.rawKataGoComparisonLogPath));
    printFilesystemPathStatus(
        "plan.acceptance.evidence.rawKataGoComparisonLog",
        resolvedAcceptanceEvidencePath(acceptanceRecords, acceptanceRecords.rawKataGoComparisonLogPath));
    printText(
        "plan.acceptance.evidence.rawKataGoComparisonLog.sha256",
        acceptanceEvidenceSha256(acceptanceRecords, acceptanceRecords.rawKataGoComparisonLogPath));
    printText(
        "plan.acceptance.evidence.rawKataGoComparisonLog.expectedSha256",
        recordSha256DisplayText(acceptanceRecords.rawKataGoComparisonLogSha256));
    printText(
        "plan.acceptance.evidence.rawKataGoComparisonLog.sha256Status",
        acceptanceEvidenceSha256Status(
            acceptanceRecords,
            acceptanceRecords.rawKataGoComparisonLogPath,
            acceptanceRecords.rawKataGoComparisonLogSha256));
    printText(
        "plan.acceptance.evidence.rawKataGoComparisonLog.timestampStatus",
        acceptanceEvidenceTimestampStatus(acceptanceRecords, acceptanceRecords.rawKataGoComparisonLogPath));
    printText(
        "plan.acceptance.evidence.rawKataGoComparisonLog.contentStatus",
        acceptanceEvidenceContentStatus(
            acceptanceRecords,
            acceptanceRecords.rawKataGoComparisonLogPath,
            rawKataGoComparisonLogEvidenceContentMarkers(acceptanceRecords)));
    printStringList(
        "plan.acceptance.evidence.rawKataGoComparisonLog.missingContentMarker",
        acceptanceEvidenceMissingMarkers(
            acceptanceRecords,
            acceptanceRecords.rawKataGoComparisonLogPath,
            rawKataGoComparisonLogEvidenceContentMarkers(acceptanceRecords)));
    printText(
        "plan.acceptance.evidence.manualUiInspectionLog.value",
        recordPathDisplayText(acceptanceRecords.manualUiInspectionLogPath));
    printFilesystemPathStatus(
        "plan.acceptance.evidence.manualUiInspectionLog",
        resolvedAcceptanceEvidencePath(acceptanceRecords, acceptanceRecords.manualUiInspectionLogPath));
    printText(
        "plan.acceptance.evidence.manualUiInspectionLog.sha256",
        acceptanceEvidenceSha256(acceptanceRecords, acceptanceRecords.manualUiInspectionLogPath));
    printText(
        "plan.acceptance.evidence.manualUiInspectionLog.expectedSha256",
        recordSha256DisplayText(acceptanceRecords.manualUiInspectionLogSha256));
    printText(
        "plan.acceptance.evidence.manualUiInspectionLog.sha256Status",
        acceptanceEvidenceSha256Status(
            acceptanceRecords,
            acceptanceRecords.manualUiInspectionLogPath,
            acceptanceRecords.manualUiInspectionLogSha256));
    printText(
        "plan.acceptance.evidence.manualUiInspectionLog.timestampStatus",
        acceptanceEvidenceTimestampStatus(acceptanceRecords, acceptanceRecords.manualUiInspectionLogPath));
    printText(
        "plan.acceptance.evidence.manualUiInspectionLog.contentStatus",
        acceptanceEvidenceContentStatus(
            acceptanceRecords,
            acceptanceRecords.manualUiInspectionLogPath,
            manualUiInspectionLogEvidenceContentMarkers()));
    printStringList(
        "plan.acceptance.evidence.manualUiInspectionLog.missingContentMarker",
        acceptanceEvidenceMissingMarkers(
            acceptanceRecords,
            acceptanceRecords.manualUiInspectionLogPath,
            manualUiInspectionLogEvidenceContentMarkers()));
    printText(
        "plan.acceptance.evidence.externalTargetVerificationLog.value",
        recordPathDisplayText(acceptanceRecords.externalTargetVerificationLogPath));
    printFilesystemPathStatus(
        "plan.acceptance.evidence.externalTargetVerificationLog",
        resolvedAcceptanceEvidencePath(acceptanceRecords, acceptanceRecords.externalTargetVerificationLogPath));
    printText(
        "plan.acceptance.evidence.externalTargetVerificationLog.sha256",
        acceptanceEvidenceSha256(acceptanceRecords, acceptanceRecords.externalTargetVerificationLogPath));
    printText(
        "plan.acceptance.evidence.externalTargetVerificationLog.expectedSha256",
        recordSha256DisplayText(acceptanceRecords.externalTargetVerificationLogSha256));
    printText(
        "plan.acceptance.evidence.externalTargetVerificationLog.sha256Status",
        acceptanceEvidenceSha256Status(
            acceptanceRecords,
            acceptanceRecords.externalTargetVerificationLogPath,
            acceptanceRecords.externalTargetVerificationLogSha256));
    printText(
        "plan.acceptance.evidence.externalTargetVerificationLog.timestampStatus",
        acceptanceEvidenceTimestampStatus(acceptanceRecords, acceptanceRecords.externalTargetVerificationLogPath));
    printText(
        "plan.acceptance.evidence.externalTargetVerificationLog.contentStatus",
        acceptanceEvidenceContentStatus(
            acceptanceRecords,
            acceptanceRecords.externalTargetVerificationLogPath,
            externalTargetVerificationLogEvidenceContentMarkers()));
    printStringList(
        "plan.acceptance.evidence.externalTargetVerificationLog.missingContentMarker",
        acceptanceEvidenceMissingMarkers(
            acceptanceRecords,
            acceptanceRecords.externalTargetVerificationLogPath,
            externalTargetVerificationLogEvidenceContentMarkers()));
    printText("plan.acceptance.evidence.screenshot.value", recordPathDisplayText(acceptanceRecords.screenshotPath));
    printFilesystemPathStatus(
        "plan.acceptance.evidence.screenshot",
        resolvedAcceptanceEvidencePath(acceptanceRecords, acceptanceRecords.screenshotPath));
    printText(
        "plan.acceptance.evidence.screenshot.sha256",
        acceptanceEvidenceSha256(acceptanceRecords, acceptanceRecords.screenshotPath));
    printText(
        "plan.acceptance.evidence.screenshot.expectedSha256",
        recordSha256DisplayText(acceptanceRecords.screenshotSha256));
    printText(
        "plan.acceptance.evidence.screenshot.sha256Status",
        acceptanceEvidenceSha256Status(
            acceptanceRecords,
            acceptanceRecords.screenshotPath,
            acceptanceRecords.screenshotSha256));
    printText(
        "plan.acceptance.evidence.screenshot.timestampStatus",
        acceptanceEvidenceTimestampStatus(acceptanceRecords, acceptanceRecords.screenshotPath));
    const AcceptanceImageEvidenceInfo screenshotImage =
        inspectAcceptanceImageEvidence(acceptanceRecords, acceptanceRecords.screenshotPath);
    printBool("plan.acceptance.evidence.screenshot.imageReadable", screenshotImage.readable);
    printText("plan.acceptance.evidence.screenshot.imageFormat", screenshotImage.format);
    std::cout << "plan.acceptance.evidence.screenshot.imageWidth: " << screenshotImage.width << '\n';
    std::cout << "plan.acceptance.evidence.screenshot.imageHeight: " << screenshotImage.height << '\n';
    printBool("plan.acceptance.evidence.screenshot.imagePixelsAtLeast4K", screenshotImage.pixelsAtLeast4K);
    printBool("plan.acceptance.evidence.screenshot.imageHasPixelVariation", screenshotImage.hasPixelVariation);
    const bool screenshotEvidence4KReady =
        screenshotImage.readable && screenshotImage.pixelsAtLeast4K && screenshotImage.hasPixelVariation;
    printBool("plan.acceptance.evidence.screenshot.readyFor4KAcceptance", screenshotEvidence4KReady);
    printBool("plan.acceptance.evidence.ready", evidenceReady);
    printStringList("plan.acceptance.evidence.blocker", evidenceBlockers);
    printBool("plan.acceptance.evidenceSha256.ready", evidenceSha256Ready);
    printStringList("plan.acceptance.evidenceSha256.blocker", evidenceSha256Blockers);
    printBool("plan.acceptance.recordTimestamp.ready", recordTimestampReady);
    printStringList("plan.acceptance.recordTimestamp.blocker", recordTimestampBlockers);
    printBool("plan.acceptance.evidenceTimestamp.ready", evidenceTimestampReady);
    printStringList("plan.acceptance.evidenceTimestamp.blocker", evidenceTimestampBlockers);
    printText("plan.acceptance.linuxKdeWaylandNvidiaManualResult", acceptanceRecords.linuxKdeWaylandNvidia);
    printText("plan.acceptance.windowsNvidiaManualResult", acceptanceRecords.windowsNvidia);
    printText("plan.acceptance.physicalDisplayManualResult", acceptanceRecords.physicalDisplay);
    printText("plan.acceptance.externalTargetVerificationManualResult", acceptanceRecords.externalTargetVerification);
    printStringList("plan.acceptance.checklistPassedRecord", acceptanceRecords.passedChecklistItems);
    printStringList("plan.acceptance.checklistFailedRecord", acceptanceRecords.failedChecklistItems);
    printStringList("plan.acceptance.checklistInvalidRecord", acceptanceRecords.invalidChecklistItems);
    printStringList("plan.acceptance.checklistMissingRecord", missingChecklistRecords);
    printStringList("plan.acceptance.manualFailedRecord", failedManualRecords);
    printStringList("plan.acceptance.manualInvalidRecord", invalidManualRecords);
    printBool("plan.acceptance.checklist.ready", checklistReady);
    printStringList("plan.acceptance.checklist.blocker", checklistBlockers);
    printBool("plan.acceptance.linuxKdeWaylandNvidiaCandidate", linuxNvidiaCandidate);
    printBool("plan.acceptance.windowsNvidiaCandidate", windowsNvidiaCandidate);
    printBool("plan.acceptance.targetPlatformCandidate", targetPlatformCandidate);
    printStringList(
        "plan.acceptance.targetPlatformBlocker",
        firstReleaseNvidiaPlatformBlockers(linuxNvidiaCandidate, windowsNvidiaCandidate));
    printBool("plan.acceptance.realKataGoEnvReady", kataGo.ready);
    printText("plan.acceptance.realKataGoEnvStatus", kataGo.status);
    printBool("plan.acceptance.realKataGoTargetRunCandidate", targetPlatformCandidate && kataGo.ready);
    printBool(
        "plan.acceptance.realKataGoManualVerificationCandidate",
        targetPlatformCandidate && kataGo.ready && anyTargetDisplayCandidate);
    printStringList(
        "plan.acceptance.realKataGoManualVerificationBlocker",
        manualVerificationBlockers(
            targetPlatformCandidate,
            kataGo.ready,
            anyTargetDisplayCandidate,
            QStringLiteral("kataGoEnv")));
    printBool("plan.acceptance.savedMainConfigReady", storedConfig.main.ready);
    printBool("plan.acceptance.savedMainGtpReady", storedConfig.main.gtpReady);
    printBool("plan.acceptance.savedMainAnalysisReady", storedConfig.main.analysisReady);
    printText("plan.acceptance.savedMainConfigStatus", storedConfig.main.status);
    printBool("plan.acceptance.savedCompareConfigReady", storedConfig.compare.ready);
    printBool("plan.acceptance.savedCompareGtpReady", storedConfig.compare.gtpReady);
    printBool("plan.acceptance.savedCompareAnalysisReady", storedConfig.compare.analysisReady);
    printText("plan.acceptance.savedCompareConfigStatus", storedConfig.compare.status);
    printBool("plan.acceptance.envOrSavedMainConfigReady", envOrSavedMainConfigReady);
    printBool("plan.acceptance.envOrSavedMainGtpReady", envOrSavedMainGtpReady);
    printBool("plan.acceptance.envOrSavedMainAnalysisReady", envOrSavedMainAnalysisReady);
    printText(
        "plan.acceptance.configuredMainConfigSource",
        configuredSourceText(kataGo.ready, storedConfig.main.ready));
    printText(
        "plan.acceptance.configuredMainGtpSource",
        configuredSourceText(kataGo.ready, storedConfig.main.gtpReady));
    printText(
        "plan.acceptance.configuredMainAnalysisSource",
        configuredSourceText(kataGo.ready, storedConfig.main.analysisReady));
    printText(
        "plan.acceptance.configuredCompareConfigSource",
        configuredSourceText(false, storedConfig.compare.ready));
    printText(
        "plan.acceptance.configuredCompareGtpSource",
        configuredSourceText(false, storedConfig.compare.gtpReady));
    printText(
        "plan.acceptance.configuredCompareAnalysisSource",
        configuredSourceText(false, storedConfig.compare.analysisReady));
    printBool("plan.acceptance.configuredDualEngineConfigReady", configuredDualEngineConfigReady);
    printBool("plan.acceptance.configuredDualEngineGtpReady", configuredDualEngineGtpReady);
    printBool("plan.acceptance.configuredDualEngineAnalysisReady", configuredDualEngineAnalysisReady);
    printBool(
        "plan.acceptance.configuredKataGoTargetRunCandidate",
        targetPlatformCandidate && envOrSavedMainConfigReady);
    printBool(
        "plan.acceptance.configuredManualVerificationCandidate",
        targetPlatformCandidate && envOrSavedMainConfigReady && anyTargetDisplayCandidate);
    printStringList(
        "plan.acceptance.configuredManualVerificationBlocker",
        manualVerificationBlockers(
            targetPlatformCandidate,
            envOrSavedMainConfigReady,
            anyTargetDisplayCandidate,
            QStringLiteral("kataGoConfig")));
    printBool(
        "plan.acceptance.configuredRealtimeGtpTargetRunCandidate",
        targetPlatformCandidate && envOrSavedMainGtpReady);
    printBool(
        "plan.acceptance.configuredRealtimeGtpManualVerificationCandidate",
        targetPlatformCandidate && envOrSavedMainGtpReady && anyTargetDisplayCandidate);
    printBool(
        "plan.acceptance.configuredBatchAnalysisTargetRunCandidate",
        targetPlatformCandidate && envOrSavedMainAnalysisReady);
    printBool(
        "plan.acceptance.configuredBatchAnalysisManualVerificationCandidate",
        targetPlatformCandidate && envOrSavedMainAnalysisReady && anyTargetDisplayCandidate);
    printBool(
        "plan.acceptance.configuredDualEngineTargetRunCandidate",
        targetPlatformCandidate && configuredDualEngineConfigReady);
    printBool(
        "plan.acceptance.configuredDualEngineManualVerificationCandidate",
        targetPlatformCandidate && configuredDualEngineConfigReady && anyTargetDisplayCandidate);
    printStringList(
        "plan.acceptance.configuredDualEngineManualVerificationBlocker",
        dualEngineManualVerificationBlockers(
            targetPlatformCandidate,
            envOrSavedMainConfigReady,
            storedConfig.compare.ready,
            anyTargetDisplayCandidate));
    printBool(
        "plan.acceptance.configuredDualRealtimeGtpTargetRunCandidate",
        targetPlatformCandidate && configuredDualEngineGtpReady);
    printBool(
        "plan.acceptance.configuredDualRealtimeGtpManualVerificationCandidate",
        targetPlatformCandidate && configuredDualEngineGtpReady && anyTargetDisplayCandidate);
    printBool(
        "plan.acceptance.configuredDualBatchAnalysisTargetRunCandidate",
        targetPlatformCandidate && configuredDualEngineAnalysisReady);
    printBool(
        "plan.acceptance.configuredDualBatchAnalysisManualVerificationCandidate",
        targetPlatformCandidate && configuredDualEngineAnalysisReady && anyTargetDisplayCandidate);
    printText(
        "plan.acceptance.configuredStatus",
        planConfiguredAcceptanceStatus(targetPlatformCandidate, envOrSavedMainConfigReady, anyTargetDisplayCandidate));
    printBool("plan.acceptance.display4KCandidate", anyDisplay4K);
    printBool("plan.acceptance.highDpiCandidate", anyHighDpiScale);
    printBool("plan.acceptance.targetDisplayCandidate", anyTargetDisplayCandidate);
    printBool("plan.acceptance.sameScreenTargetDisplayCandidate", anyTargetDisplayCandidate);
    printBool("plan.acceptance.multiDisplayCandidate", screens.size() > 1);
    printStringList(
        "plan.acceptance.targetDisplayBlocker",
        targetDisplayBlockers(screens, anyDisplay4K, anyHighDpiScale, anyTargetDisplayCandidate));
    printStringList("plan.acceptance.multiDisplayBlocker", multiDisplayBlockers(screens));
    printBool("plan.acceptance.manualUiInspectionRequired", true);
    printText("plan.acceptance.manualUiInspectionStatus", acceptanceRecords.manualUiInspection);
    printStringList("plan.acceptance.manualUiInspectionChecklist", manualUiInspectionChecklist());
    printBool("plan.acceptance.manualRawEngineComparisonRequired", true);
    printText("plan.acceptance.rawKataGoComparisonStatus", acceptanceRecords.rawKataGoComparison);
    printStringList("plan.acceptance.rawKataGoComparisonChecklist", rawKataGoComparisonChecklist());
    printBool("plan.acceptance.externalTargetVerificationRequired", true);
    printText("plan.acceptance.externalTargetVerificationStatus", externalVerificationStatus);
    printStringList(
        "plan.acceptance.externalTargetVerificationChecklist",
        externalTargetVerificationChecklist());
    const QStringList finalBlockers = finalAcceptanceBlockers(
        targetPlatformCandidate,
        envOrSavedMainGtpReady,
        envOrSavedMainAnalysisReady,
        storedConfig.compare.gtpReady,
        storedConfig.compare.analysisReady,
        anyTargetDisplayCandidate,
        screens.size() > 1,
        manualAcceptancePassed(externalVerificationStatus),
        manualAcceptancePassed(acceptanceRecords.rawKataGoComparison),
        manualAcceptancePassed(acceptanceRecords.manualUiInspection),
        checklistReady,
        metadataReady,
        evidenceReady,
        evidenceSha256Ready,
        recordTimestampReady,
        evidenceTimestampReady,
        screenshotEvidence4KReady);
    printText("plan.acceptance.finalAcceptanceStatus", finalAcceptanceStatus(finalBlockers, acceptanceRecords));
    printStringList("plan.acceptance.finalAcceptanceBlocker", finalBlockers);
    printStringList("plan.acceptance.finalAcceptanceOutstandingBlocker", finalBlockers);
    printStringList(
        "plan.acceptance.linuxKdeWaylandNvidiaVerificationChecklist",
        linuxKdeWaylandNvidiaVerificationChecklist());
    const QStringList linuxTargetBlockers = linuxKdeWaylandNvidiaBlockers(
        osFamily,
        kdeSession,
        waylandSession,
        qtWaylandPlatform,
        nvidiaHintPresent);
    printText(
        "plan.acceptance.linuxKdeWaylandNvidiaVerificationStatus",
        targetRealtimeVerificationStatus(
            linuxNvidiaCandidate,
            envOrSavedMainGtpReady,
            QStringLiteral("linux-kde-wayland-nvidia")));
    printStringList(
        "plan.acceptance.linuxKdeWaylandNvidiaVerificationReadinessBlocker",
        targetRealtimeVerificationReadinessBlockers(linuxTargetBlockers, envOrSavedMainGtpReady));
    printStringList(
        "plan.acceptance.windowsNvidiaVerificationChecklist",
        windowsNvidiaVerificationChecklist());
    const QStringList windowsTargetBlockers =
        windowsNvidiaBlockers(osFamily, windowsPlatformPlugin, nvidiaHintPresent);
    printText(
        "plan.acceptance.windowsNvidiaVerificationStatus",
        targetRealtimeVerificationStatus(
            windowsNvidiaCandidate,
            envOrSavedMainGtpReady,
            QStringLiteral("windows-nvidia")));
    printStringList(
        "plan.acceptance.windowsNvidiaVerificationReadinessBlocker",
        targetRealtimeVerificationReadinessBlockers(windowsTargetBlockers, envOrSavedMainGtpReady));
    printStringList(
        "plan.acceptance.physicalDisplayVerificationChecklist",
        physicalDisplayVerificationChecklist());
    printText(
        "plan.acceptance.physicalDisplayVerificationStatus",
        physicalDisplayVerificationStatus(anyTargetDisplayCandidate, screens.size() > 1));
    printStringList(
        "plan.acceptance.physicalDisplayVerificationReadinessBlocker",
        physicalDisplayVerificationReadinessBlockers(
            screens,
            anyDisplay4K,
            anyHighDpiScale,
            anyTargetDisplayCandidate));
    printText(
        "plan.acceptance.status",
        planAcceptanceStatus(targetPlatformCandidate, kataGo.ready, anyTargetDisplayCandidate));
}

void printDiagnostics()
{
    std::cout << "LizzieYzy Qt Diagnostics\n";
    std::cout << "app.version: " << LIZZIEYZY_QT_VERSION << '\n';
    printText("app.executable", QCoreApplication::applicationFilePath());
    printFilesystemPathStatus("app.executable", QCoreApplication::applicationFilePath());
    const QString appDir = QCoreApplication::applicationDirPath();
    printText("app.dir", appDir);
    printFilesystemPathStatus("app.dir", appDir);
    printAppLocalQtRuntimeDiagnostics(appDir);
    printText("process.currentWorkingDirectory", QDir::currentPath());
    printFilesystemPathStatus("process.currentWorkingDirectory", QDir::currentPath());
    printText("qt.version", QString::fromLatin1(qVersion()));
    printQtBuildRuntimeDiagnostics();
    printText("qt.platform", QGuiApplication::platformName());
    const QString pluginPath = QLibraryInfo::path(QLibraryInfo::PluginsPath);
    printText("qt.pluginPath", pluginPath);
    printFilesystemPathStatus("qt.pluginPath", pluginPath);
    const QStringList libraryPaths = QCoreApplication::libraryPaths();
    printStringList("qt.libraryPath", libraryPaths);
    for (int index = 0; index < libraryPaths.size(); ++index) {
        printFilesystemPathStatus("qt.libraryPath." + std::to_string(index), libraryPaths.at(index));
    }
    printStandardPathDiagnostics();
    printApplicationFontDiagnostics();
    printApplicationAppearanceDiagnostics();
    printDialogDiagnostics();
    printMainWindowUiDiagnostics();
    printLocaleDiagnostics();
    printSettingsStorageDiagnostics();
    printPlatformPluginDiagnostics(QGuiApplication::platformName(), pluginPath, libraryPaths);
    printText("qt.highDpiScaleFactorRoundingPolicy",
              highDpiPolicyText(QGuiApplication::highDpiScaleFactorRoundingPolicy()));
    printText("qt.quick.graphicsApi", quickGraphicsApiText(QQuickWindow::graphicsApi()));
    printText("qt.quick.sceneGraphBackend", QQuickWindow::sceneGraphBackend());
    printQmlImportPathDiagnostics();
    QQuickWindow diagnosticQuickWindow;
    const QSGRendererInterface* rendererInterface = diagnosticQuickWindow.rendererInterface();
    if (rendererInterface != nullptr) {
        const QSGRendererInterface::GraphicsApi rendererApi = rendererInterface->graphicsApi();
        printText("qt.quick.rendererInterface.graphicsApi", quickGraphicsApiText(rendererApi));
        printBool("qt.quick.rendererInterface.rhiBased", QSGRendererInterface::isApiRhiBased(rendererApi));
        printText("qt.quick.rendererInterface.shaderType", quickShaderTypeText(rendererInterface->shaderType()));
        printText(
            "qt.quick.rendererInterface.shaderCompilationTypes",
            shaderCompilationTypesText(rendererInterface->shaderCompilationType()));
        printText(
            "qt.quick.rendererInterface.shaderSourceTypes",
            shaderSourceTypesText(rendererInterface->shaderSourceType()));
    } else {
        printText("qt.quick.rendererInterface.graphicsApi", QStringLiteral("(none)"));
        printText("qt.quick.rendererInterface.rhiBased", QStringLiteral("(none)"));
        printText("qt.quick.rendererInterface.shaderType", QStringLiteral("(none)"));
        printText("qt.quick.rendererInterface.shaderCompilationTypes", QStringLiteral("(none)"));
        printText("qt.quick.rendererInterface.shaderSourceTypes", QStringLiteral("(none)"));
    }
    printOpenGlDiagnostics();
    printText("os.prettyName", QSysInfo::prettyProductName());
    printText("os.productType", QSysInfo::productType());
    printText("os.productVersion", QSysInfo::productVersion());
    printText("os.kernelType", QSysInfo::kernelType());
    printText("os.kernelVersion", QSysInfo::kernelVersion());
    printText("qt.buildAbi", QSysInfo::buildAbi());
    printText("cpu.buildArchitecture", QSysInfo::buildCpuArchitecture());
    printText("cpu.architecture", QSysInfo::currentCpuArchitecture());
    printText("env.PATH", environmentValue("PATH"));
    printPathListEnvironmentStatus("envPath.PATH", "PATH");
    printText("env.LD_LIBRARY_PATH", environmentValue("LD_LIBRARY_PATH"));
    printPathListEnvironmentStatus("envPath.LD_LIBRARY_PATH", "LD_LIBRARY_PATH");
    printText("env.DYLD_LIBRARY_PATH", environmentValue("DYLD_LIBRARY_PATH"));
    printPathListEnvironmentStatus("envPath.DYLD_LIBRARY_PATH", "DYLD_LIBRARY_PATH");
    printText("env.HOME", environmentValue("HOME"));
    printFilesystemPathStatus("envPath.HOME", QString::fromLocal8Bit(qgetenv("HOME")));
    printText("env.USERPROFILE", environmentValue("USERPROFILE"));
    printFilesystemPathStatus("envPath.USERPROFILE", QString::fromLocal8Bit(qgetenv("USERPROFILE")));
    printText("env.APPDATA", environmentValue("APPDATA"));
    printFilesystemPathStatus("envPath.APPDATA", QString::fromLocal8Bit(qgetenv("APPDATA")));
    printText("env.LOCALAPPDATA", environmentValue("LOCALAPPDATA"));
    printFilesystemPathStatus("envPath.LOCALAPPDATA", QString::fromLocal8Bit(qgetenv("LOCALAPPDATA")));
    printText("env.XDG_CONFIG_HOME", environmentValue("XDG_CONFIG_HOME"));
    printFilesystemPathStatus("envPath.XDG_CONFIG_HOME", QString::fromLocal8Bit(qgetenv("XDG_CONFIG_HOME")));
    printText("env.XDG_DATA_HOME", environmentValue("XDG_DATA_HOME"));
    printFilesystemPathStatus("envPath.XDG_DATA_HOME", QString::fromLocal8Bit(qgetenv("XDG_DATA_HOME")));
    printText("env.XDG_CACHE_HOME", environmentValue("XDG_CACHE_HOME"));
    printFilesystemPathStatus("envPath.XDG_CACHE_HOME", QString::fromLocal8Bit(qgetenv("XDG_CACHE_HOME")));
    printText("env.TEMP", environmentValue("TEMP"));
    printFilesystemPathStatus("envPath.TEMP", QString::fromLocal8Bit(qgetenv("TEMP")));
    printText("env.TMP", environmentValue("TMP"));
    printFilesystemPathStatus("envPath.TMP", QString::fromLocal8Bit(qgetenv("TMP")));
    printText("env.XDG_SESSION_TYPE", environmentValue("XDG_SESSION_TYPE"));
    printText("env.XDG_CURRENT_DESKTOP", environmentValue("XDG_CURRENT_DESKTOP"));
    printText("env.XDG_SESSION_DESKTOP", environmentValue("XDG_SESSION_DESKTOP"));
    printText("env.XDG_RUNTIME_DIR", environmentValue("XDG_RUNTIME_DIR"));
    printFilesystemPathStatus("envPath.XDG_RUNTIME_DIR", QString::fromLocal8Bit(qgetenv("XDG_RUNTIME_DIR")));
    printText("env.DESKTOP_SESSION", environmentValue("DESKTOP_SESSION"));
    printText("env.KDE_FULL_SESSION", environmentValue("KDE_FULL_SESSION"));
    printText("env.KDE_SESSION_VERSION", environmentValue("KDE_SESSION_VERSION"));
    printText("env.WAYLAND_DISPLAY", environmentValue("WAYLAND_DISPLAY"));
    printText("env.DISPLAY", environmentValue("DISPLAY"));
    printText("env.QT_QPA_PLATFORM", environmentValue("QT_QPA_PLATFORM"));
    printText("env.QT_QPA_PLATFORMTHEME", environmentValue("QT_QPA_PLATFORMTHEME"));
    printText("env.QT_QPA_PLATFORM_PLUGIN_PATH", environmentValue("QT_QPA_PLATFORM_PLUGIN_PATH"));
    printText("env.QT_PLUGIN_PATH", environmentValue("QT_PLUGIN_PATH"));
    printPathListEnvironmentStatus("envPath.QT_QPA_PLATFORM_PLUGIN_PATH", "QT_QPA_PLATFORM_PLUGIN_PATH");
    printPathListEnvironmentStatus("envPath.QT_PLUGIN_PATH", "QT_PLUGIN_PATH");
    printText("env.QT_DEBUG_PLUGINS", environmentValue("QT_DEBUG_PLUGINS"));
    printText("env.QT_LOGGING_RULES", environmentValue("QT_LOGGING_RULES"));
    printText("env.QT_STYLE_OVERRIDE", environmentValue("QT_STYLE_OVERRIDE"));
    printText("env.QT_SCALE_FACTOR", environmentValue("QT_SCALE_FACTOR"));
    printText("env.QT_SCREEN_SCALE_FACTORS", environmentValue("QT_SCREEN_SCALE_FACTORS"));
    printText("env.QT_AUTO_SCREEN_SCALE_FACTOR", environmentValue("QT_AUTO_SCREEN_SCALE_FACTOR"));
    printText("env.QT_ENABLE_HIGHDPI_SCALING", environmentValue("QT_ENABLE_HIGHDPI_SCALING"));
    printText("env.QT_SCALE_FACTOR_ROUNDING_POLICY", environmentValue("QT_SCALE_FACTOR_ROUNDING_POLICY"));
    printText("env.QT_USE_PHYSICAL_DPI", environmentValue("QT_USE_PHYSICAL_DPI"));
    printText("env.QT_DPI_ADJUSTMENT_POLICY", environmentValue("QT_DPI_ADJUSTMENT_POLICY"));
    printText("env.QT_FONT_DPI", environmentValue("QT_FONT_DPI"));
    printText("env.QSG_RHI_BACKEND", environmentValue("QSG_RHI_BACKEND"));
    printText("env.QT_RHI_BACKEND", environmentValue("QT_RHI_BACKEND"));
    printText("env.QSG_INFO", environmentValue("QSG_INFO"));
    printText("env.QT_QUICK_BACKEND", environmentValue("QT_QUICK_BACKEND"));
    printText("env.QT_QUICK_CONTROLS_STYLE", environmentValue("QT_QUICK_CONTROLS_STYLE"));
    printText("env.QT_QUICK_CONTROLS_CONF", environmentValue("QT_QUICK_CONTROLS_CONF"));
    printFilesystemPathStatus("envPath.QT_QUICK_CONTROLS_CONF", QString::fromLocal8Bit(qgetenv("QT_QUICK_CONTROLS_CONF")));
    printText("env.QT_QUICK_CONTROLS_STYLE_PATH", environmentValue("QT_QUICK_CONTROLS_STYLE_PATH"));
    printPathListEnvironmentStatus("envPath.QT_QUICK_CONTROLS_STYLE_PATH", "QT_QUICK_CONTROLS_STYLE_PATH");
    printText("env.QML_IMPORT_PATH", environmentValue("QML_IMPORT_PATH"));
    printPathListEnvironmentStatus("envPath.QML_IMPORT_PATH", "QML_IMPORT_PATH");
    printText("env.QML2_IMPORT_PATH", environmentValue("QML2_IMPORT_PATH"));
    printPathListEnvironmentStatus("envPath.QML2_IMPORT_PATH", "QML2_IMPORT_PATH");
    printText("env.QSG_RENDER_LOOP", environmentValue("QSG_RENDER_LOOP"));
    printText("env.QSG_VISUALIZE", environmentValue("QSG_VISUALIZE"));
    printText("env.QSG_RENDERER_DEBUG", environmentValue("QSG_RENDERER_DEBUG"));
    printText("env.QSG_RHI_DEBUG_LAYER", environmentValue("QSG_RHI_DEBUG_LAYER"));
    printText("env.QSG_RHI_PREFER_SOFTWARE_RENDERER", environmentValue("QSG_RHI_PREFER_SOFTWARE_RENDERER"));
    printText("env.QT_OPENGL", environmentValue("QT_OPENGL"));
    printText("env.QT_ANGLE_PLATFORM", environmentValue("QT_ANGLE_PLATFORM"));
    printText("env.QT_D3D_ADAPTER_INDEX", environmentValue("QT_D3D_ADAPTER_INDEX"));
    printText("env.QT_D3D_DEBUG", environmentValue("QT_D3D_DEBUG"));
    printText("env.QT_WAYLAND_DISABLE_WINDOWDECORATION", environmentValue("QT_WAYLAND_DISABLE_WINDOWDECORATION"));
    printText("env.QT_WAYLAND_SHELL_INTEGRATION", environmentValue("QT_WAYLAND_SHELL_INTEGRATION"));
    printText("env.KWIN_DRM_DEVICES", environmentValue("KWIN_DRM_DEVICES"));
    printPathListEnvironmentStatus("envPath.KWIN_DRM_DEVICES", "KWIN_DRM_DEVICES");
    printText("env.GBM_BACKEND", environmentValue("GBM_BACKEND"));
    printText("env.DRI_PRIME", environmentValue("DRI_PRIME"));
    printText("env.MESA_LOADER_DRIVER_OVERRIDE", environmentValue("MESA_LOADER_DRIVER_OVERRIDE"));
    printText("env.LIBGL_ALWAYS_SOFTWARE", environmentValue("LIBGL_ALWAYS_SOFTWARE"));
    printText("env.LIBGL_DEBUG", environmentValue("LIBGL_DEBUG"));
    printText("env.LIBVA_DRIVER_NAME", environmentValue("LIBVA_DRIVER_NAME"));
    printText("env.EGL_PLATFORM", environmentValue("EGL_PLATFORM"));
    printText("env.__EGL_VENDOR_LIBRARY_FILENAMES", environmentValue("__EGL_VENDOR_LIBRARY_FILENAMES"));
    printPathListEnvironmentStatus("envPath.__EGL_VENDOR_LIBRARY_FILENAMES", "__EGL_VENDOR_LIBRARY_FILENAMES");
    printText("env.__GLX_VENDOR_LIBRARY_NAME", environmentValue("__GLX_VENDOR_LIBRARY_NAME"));
    printText("env.__NV_PRIME_RENDER_OFFLOAD", environmentValue("__NV_PRIME_RENDER_OFFLOAD"));
    printText("env.__NV_PRIME_RENDER_OFFLOAD_PROVIDER", environmentValue("__NV_PRIME_RENDER_OFFLOAD_PROVIDER"));
    printText("env.__VK_LAYER_NV_optimus", environmentValue("__VK_LAYER_NV_optimus"));
    printText("env.VK_INSTANCE_LAYERS", environmentValue("VK_INSTANCE_LAYERS"));
    printText("env.VK_LOADER_DEBUG", environmentValue("VK_LOADER_DEBUG"));
    printText("env.VK_ICD_FILENAMES", environmentValue("VK_ICD_FILENAMES"));
    printText("env.VK_DRIVER_FILES", environmentValue("VK_DRIVER_FILES"));
    printPathListEnvironmentStatus("envPath.VK_ICD_FILENAMES", "VK_ICD_FILENAMES");
    printPathListEnvironmentStatus("envPath.VK_DRIVER_FILES", "VK_DRIVER_FILES");
    printText("env.NVIDIA_VISIBLE_DEVICES", environmentValue("NVIDIA_VISIBLE_DEVICES"));
    printText("env.CUDA_VISIBLE_DEVICES", environmentValue("CUDA_VISIBLE_DEVICES"));
    printText("env.CUDA_DEVICE_ORDER", environmentValue("CUDA_DEVICE_ORDER"));
    printText("env.NVIDIA_DRIVER_CAPABILITIES", environmentValue("NVIDIA_DRIVER_CAPABILITIES"));
    printText("env.LIZZIE_TARGET_ACCEPTANCE_RECORD_FILE", environmentValue(targetAcceptanceRecordEnvironmentName()));
    printPathEnvironmentStatus("katago.env.executable", "LIZZIE_KATAGO_EXECUTABLE", true);
    printPathEnvironmentStatus("katago.env.model", "LIZZIE_KATAGO_MODEL", false);
    printPathEnvironmentStatus("katago.env.analysisConfig", "LIZZIE_KATAGO_ANALYSIS_CONFIG", false);
    printPathEnvironmentStatus("katago.env.gtpConfig", "LIZZIE_KATAGO_GTP_CONFIG", false);
    printKataGoEnvironmentReadiness();

    const QList<QScreen*> screens = QGuiApplication::screens();
    const QScreen* primaryScreen = QGuiApplication::primaryScreen();
    printTargetPlatformSummaryDiagnostics(QGuiApplication::platformName(), screens, primaryScreen);
    printPlanAcceptanceSummaryDiagnostics(QGuiApplication::platformName(), screens, primaryScreen);
    int primaryScreenIndex = -1;
    for (int index = 0; index < screens.size(); ++index) {
        if (screens.at(index) == primaryScreen) {
            primaryScreenIndex = index;
            break;
        }
    }
    std::cout << "screen.primary.index: " << primaryScreenIndex << '\n';
    printScreenCoreDiagnostics("screen.primary.", primaryScreen);
    std::cout << "screen.count: " << screens.size() << '\n';
    for (int index = 0; index < screens.size(); ++index) {
        const QScreen* screen = screens.at(index);
        if (screen == nullptr) {
            continue;
        }
        const std::string prefix = "screen." + std::to_string(index) + ".";
        printScreenCoreDiagnostics(prefix, screen);
        const QList<QScreen*> siblings = screen->virtualSiblings();
        std::cout << prefix << "virtualSibling.count: " << siblings.size() << '\n';
        for (int siblingIndex = 0; siblingIndex < siblings.size(); ++siblingIndex) {
            QScreen* sibling = siblings.at(siblingIndex);
            const std::string siblingPrefix =
                prefix + "virtualSibling." + std::to_string(siblingIndex) + ".";
            std::cout << siblingPrefix << "index: " << screens.indexOf(sibling) << '\n';
            printText(siblingPrefix + "name", sibling != nullptr ? sibling->name() : QStringLiteral("(none)"));
        }
    }
}

}  // namespace

int main(int argc, char* argv[])
{
    bool diagnosticsRequested = false;
    bool targetAcceptanceReportRequested = false;
    bool targetAcceptanceRecordTemplateRequested = false;
    std::optional<QString> targetAcceptanceRecordFile;
    for (int index = 1; index < argc; ++index) {
        if (argumentEquals(argv[index], "--version")) {
            std::cout << "LizzieYzy Qt " << LIZZIEYZY_QT_VERSION << '\n';
            return 0;
        }
        if (argumentEquals(argv[index], "--help") || argumentEquals(argv[index], "-h")) {
            std::cout << "Usage: lizzieyzy_qt [--help] [--version] [--diagnostics] "
                         "[--target-acceptance-report] [--target-acceptance-record-template] "
                         "[--target-acceptance-record <ini>]\n"
                         "Use --target-acceptance-record -- <ini> when the record path starts with '-'.\n";
            return 0;
        }
        if (argumentEquals(argv[index], "--diagnostics")) {
            diagnosticsRequested = true;
            continue;
        }
        if (argumentEquals(argv[index], "--target-acceptance-report")) {
            targetAcceptanceReportRequested = true;
            continue;
        }
        if (argumentEquals(argv[index], "--target-acceptance-record-template")) {
            targetAcceptanceRecordTemplateRequested = true;
            continue;
        }
        if (argumentEquals(argv[index], "--target-acceptance-record")) {
            if (index + 1 >= argc) {
                std::cerr << "--target-acceptance-record requires an INI file path\n";
                return 2;
            }
            if (argumentEquals(argv[index + 1], "--")) {
                if (index + 2 >= argc) {
                    std::cerr << "--target-acceptance-record requires an INI file path\n";
                    return 2;
                }
                targetAcceptanceRecordFile = QString::fromLocal8Bit(argv[index + 2]);
                index += 2;
                continue;
            }
            if (argumentStartsWith(argv[index + 1], "-")) {
                std::cerr << "--target-acceptance-record requires an INI file path\n";
                return 2;
            }
            targetAcceptanceRecordFile = QString::fromLocal8Bit(argv[++index]);
            continue;
        }
        constexpr std::string_view recordArgumentPrefix = "--target-acceptance-record=";
        if (argumentStartsWith(argv[index], recordArgumentPrefix)) {
            const std::string_view argument(argv[index]);
            const std::string_view value = argument.substr(recordArgumentPrefix.size());
            if (value.empty()) {
                std::cerr << "--target-acceptance-record requires an INI file path\n";
                return 2;
            }
            targetAcceptanceRecordFile = QString::fromLocal8Bit(value.data(), static_cast<qsizetype>(value.size()));
            continue;
        }
        if (argumentStartsWith(argv[index], "--")) {
            std::cerr << "Unknown option: " << argv[index] << '\n';
            return 2;
        }
    }

    if (targetAcceptanceRecordTemplateRequested) {
        QCoreApplication app(argc, argv);
        printTargetAcceptanceRecordTemplate(QCoreApplication::applicationFilePath());
        return 0;
    }

    if (targetAcceptanceRecordFile.has_value()) {
        qputenv(targetAcceptanceRecordEnvironmentName(), targetAcceptanceRecordFile->toLocal8Bit());
    }

    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);

    QApplication app(argc, argv);
    QApplication::setApplicationName("LizzieYzy Qt");
    QApplication::setOrganizationName("LizzieYzy");

    if (diagnosticsRequested) {
        printDiagnostics();
        return 0;
    }
    if (targetAcceptanceReportRequested) {
        printTargetAcceptanceReport();
        return 0;
    }

    lizzie::ui::MainWindow window;

    window.show();
    return app.exec();
}

#include "MainWindow.h"

#include "BoardQuickItem.h"
#include "EngineAutomation.h"
#include "EngineCommandDispatcher.h"
#include "EngineCommandPlanner.h"
#include "EngineSettingsDialog.h"
#include "FileWrite.h"
#include "FirstRun.h"
#include "GameSetup.h"
#include "LegacyConfigImport.h"
#include "PositionSync.h"
#include "SettingsApply.h"
#include "SessionSettings.h"
#include "Sgf.h"
#include "Types.h"
#include "WindowPlacement.h"

#include <QAbstractButton>
#include <QAction>
#include <QActionGroup>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDockWidget>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeySequence>
#include <QLabel>
#include <QInputDialog>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QQuickWidget>
#include <QScreen>
#include <QSettings>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QStyle>
#include <QStringList>
#include <QTextStream>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QVariant>
#include <QApplication>

#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <string_view>
#include <utility>

namespace lizzie::ui {

namespace {

QTableWidget* createCandidateTable()
{
    auto* table = new QTableWidget(0, 8);
    table->setHorizontalHeaderLabels({"Move", "Visits", "Winrate", "Score", "Stdev", "Policy", "PV", "PV Visits"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setMouseTracking(true);
    return table;
}

QTableWidget* createCompareTable()
{
    auto* table = new QTableWidget(2, 9);
    table->setHorizontalHeaderLabels(
        {"Engine", "Move", "Visits", "Winrate", "Score", "Stdev", "Policy", "PV", "PV Visits"});
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    return table;
}

QString moveDisplayText(const lizzie::core::Move& move, LanguageMode language)
{
    if (move.type == lizzie::core::MoveType::Pass) {
        return language == LanguageMode::Chinese ? QString::fromUtf8("停一手") : QStringLiteral("pass");
    }
    if (move.type == lizzie::core::MoveType::Resign) {
        return language == LanguageMode::Chinese ? QString::fromUtf8("认输") : QStringLiteral("resign");
    }
    return QString::fromStdString(lizzie::core::pointToSgf(move.point));
}

QString nodeSummary(const lizzie::core::GameNode& node, LanguageMode language)
{
    if (!node.move.has_value()) {
        return language == LanguageMode::Chinese ? QString::fromUtf8("根节点") : QStringLiteral("Root");
    }
    const QString color = language == LanguageMode::Chinese
        ? (node.move->color == lizzie::core::Color::Black ? QString::fromUtf8("黑") : QString::fromUtf8("白"))
        : (node.move->color == lizzie::core::Color::Black ? QStringLiteral("B") : QStringLiteral("W"));
    const QString moveText =
        language == LanguageMode::Chinese || !node.move->isPlay()
        ? color + QStringLiteral(" ") + moveDisplayText(*node.move, language)
        : color + moveDisplayText(*node.move, language);
    if (node.moveNumber.has_value()) {
        return QString::number(*node.moveNumber) + ". " + moveText;
    }
    return moveText;
}

QString candidateMoveText(const lizzie::core::Move& move, LanguageMode language)
{
    return moveDisplayText(move, language);
}

QString pvText(const std::vector<lizzie::core::Move>& pv, LanguageMode language)
{
    QStringList values;
    for (const lizzie::core::Move& move : pv) {
        values.push_back(candidateMoveText(move, language));
    }
    return values.join(' ');
}

QString pvVisitsText(const std::vector<int>& pvVisits)
{
    QStringList values;
    for (int visits : pvVisits) {
        values.push_back(QString::number(visits));
    }
    return values.join(' ');
}

QString percentageText(double value, int decimals = 1)
{
    if (!std::isfinite(value)) {
        value = 0.0;
    }
    return QString("%1%").arg(QString::number(std::clamp(value, 0.0, 1.0) * 100.0, 'f', decimals));
}

lizzie::core::Color analysisPerspective(const lizzie::core::AnalysisSnapshot& snapshot)
{
    return snapshot.winratePerspective.value_or(lizzie::core::Color::Black);
}

double blackPerspectiveWinrate(const lizzie::core::AnalysisSnapshot& snapshot)
{
    return analysisPerspective(snapshot) == lizzie::core::Color::Black
        ? snapshot.rootWinrate
        : 1.0 - snapshot.rootWinrate;
}

double blackPerspectiveScoreMean(const lizzie::core::AnalysisSnapshot& snapshot)
{
    return analysisPerspective(snapshot) == lizzie::core::Color::Black
        ? snapshot.rootScoreMean
        : -snapshot.rootScoreMean;
}

QString yesNo(bool value, bool zh)
{
    if (zh) {
        return value ? QString::fromUtf8("是") : QString::fromUtf8("否");
    }
    return value ? "yes" : "no";
}

QString capabilitySummary(const lizzie::engine::EngineCapabilities& capabilities, LanguageMode language)
{
    const bool zh = language == LanguageMode::Chinese;
    const QString prefix = zh ? QString::fromUtf8("引擎能力：") : QStringLiteral("engine capabilities: ");
    return QString("%1kata-analyze=%2, kata-set-rules=%3, kata-raw-nn=%4, kata-stop=%5, kata-set-param=%6, genmove=%7")
        .arg(prefix)
        .arg(yesNo(capabilities.kataAnalyze, zh))
        .arg(yesNo(capabilities.kataSetRules, zh))
        .arg(yesNo(capabilities.kataRawNn, zh))
        .arg(yesNo(capabilities.kataStop, zh))
        .arg(yesNo(capabilities.kataSetParam, zh))
        .arg(yesNo(capabilities.genmove, zh));
}

QString localizedLegacyImportNote(const QString& note, LanguageMode language)
{
    if (language != LanguageMode::Chinese) {
        return note;
    }
    if (note == "Skipped Java SSH engine entry") {
        return QString::fromUtf8("已跳过 Java SSH 引擎条目");
    }
    if (note == "Skipped engine command that is not a KataGo gtp/analysis command") {
        return QString::fromUtf8("已跳过非 KataGo gtp/analysis 引擎命令");
    }
    return note;
}

QString localizedCoreError(std::string_view error, LanguageMode language)
{
    const QString fallback = QString::fromUtf8(error.data(), static_cast<qsizetype>(error.size()));
    if (language != LanguageMode::Chinese) {
        return fallback;
    }
    if (error == "move is outside the board") {
        return QString::fromUtf8("着法在棋盘外");
    }
    if (error == "point is already occupied") {
        return QString::fromUtf8("交叉点已有棋子");
    }
    if (error == "ko recapture is not legal") {
        return QString::fromUtf8("不允许立即提劫");
    }
    if (error == "suicide is not legal") {
        return QString::fromUtf8("不允许自杀");
    }
    if (error == "point is outside the board") {
        return QString::fromUtf8("点在棋盘外");
    }
    if (error == "setup stone is outside the board") {
        return QString::fromUtf8("摆子点在棋盘外");
    }
    if (error == "setup clear point is outside the board") {
        return QString::fromUtf8("清除摆子点在棋盘外");
    }
    return fallback;
}

QString localizedSgfError(std::string_view error, LanguageMode language)
{
    const QString fallback = QString::fromUtf8(error.data(), static_cast<qsizetype>(error.size()));
    if (language != LanguageMode::Chinese) {
        return fallback;
    }
    if (error == "unexpected end of SGF") {
        return QString::fromUtf8("SGF 意外结束");
    }
    if (error == "expected SGF property identifier") {
        return QString::fromUtf8("需要 SGF 属性标识符");
    }
    if (error == "expected SGF property value") {
        return QString::fromUtf8("需要 SGF 属性值");
    }
    if (error == "unterminated SGF property value") {
        return QString::fromUtf8("SGF 属性值未结束");
    }
    if (error == "invalid SZ property") {
        return QString::fromUtf8("SZ 属性无效");
    }
    if (error == "invalid rectangular SZ property") {
        return QString::fromUtf8("矩形 SZ 属性无效");
    }
    if (error == "invalid board size") {
        return QString::fromUtf8("棋盘大小无效");
    }
    if (error == "SGF collection is empty") {
        return QString::fromUtf8("SGF 合集为空");
    }

    constexpr std::string_view prefix = "expected '";
    constexpr std::string_view middle = "' but found '";
    if (error.starts_with(prefix)) {
        const std::size_t middlePosition = error.find(middle, prefix.size());
        if (middlePosition != std::string_view::npos && error.ends_with('\'')) {
            const std::string_view expected = error.substr(prefix.size(), middlePosition - prefix.size());
            const std::size_t foundBegin = middlePosition + middle.size();
            const std::string_view found = error.substr(foundBegin, error.size() - foundBegin - 1);
            return QString::fromUtf8("SGF 语法错误：需要“%1”，但遇到“%2”")
                .arg(QString::fromUtf8(expected.data(), static_cast<qsizetype>(expected.size())))
                .arg(QString::fromUtf8(found.data(), static_cast<qsizetype>(found.size())));
        }
    }
    return fallback;
}

QString localizedAppWarning(std::string_view warning, LanguageMode language)
{
    const QString fallback = QString::fromUtf8(warning.data(), static_cast<qsizetype>(warning.size()));
    if (language != LanguageMode::Chinese) {
        return fallback;
    }
    if (warning == "resign nodes are not replayed to KataGo GTP") {
        return QString::fromUtf8("认输节点不会回放到 KataGo GTP");
    }
    if (warning == "move cannot be represented as a GTP coordinate") {
        return QString::fromUtf8("着法无法表示为 GTP 坐标");
    }
    if (warning == "black setup stone cannot be represented as a GTP coordinate") {
        return QString::fromUtf8("黑棋摆子无法表示为 GTP 坐标");
    }
    if (warning == "white setup stone cannot be represented as a GTP coordinate") {
        return QString::fromUtf8("白棋摆子无法表示为 GTP 坐标");
    }
    if (warning == "AE setup points cannot be represented by basic GTP replay") {
        return QString::fromUtf8("AE 清除点无法用基础 GTP 回放表示");
    }
    if (warning == "board width cannot be represented by GTP coordinates; engine sync was skipped") {
        return QString::fromUtf8("棋盘宽度无法用 GTP 坐标表示；已跳过引擎同步");
    }
    if (warning == "cannot build GTP sync for missing game node") {
        return QString::fromUtf8("无法为缺失的棋谱节点构建 GTP 同步");
    }
    if (warning == "KataGo does not report kata-set-rules support; SGF rules were not sent") {
        return QString::fromUtf8("KataGo 未报告 kata-set-rules 支持；未发送 SGF 规则");
    }
    if (warning == "analysis sidecar is missing collectionGameIndex for SGF collection") {
        return QString::fromUtf8("分析附加文件缺少 SGF 合集的 collectionGameIndex");
    }
    if (warning == "analysis sidecar is missing a valid nodes array") {
        return QString::fromUtf8("分析附加文件缺少有效的 nodes 数组");
    }
    if (warning == "analysis sidecar node is missing a valid nodeId") {
        return QString::fromUtf8("分析附加文件节点缺少有效 nodeId");
    }
    constexpr std::string_view sidecarNodeEntryPrefix = "analysis sidecar node entry ";
    constexpr std::string_view sidecarNodeEntryNotObjectSuffix = " is not an object";
    if (warning.starts_with(sidecarNodeEntryPrefix) && warning.ends_with(sidecarNodeEntryNotObjectSuffix)) {
        const std::string_view index = warning.substr(
            sidecarNodeEntryPrefix.size(),
            warning.size() - sidecarNodeEntryPrefix.size() - sidecarNodeEntryNotObjectSuffix.size());
        return QString::fromUtf8("分析附加文件节点条目 %1 不是对象")
            .arg(QString::fromUtf8(index.data(), static_cast<qsizetype>(index.size())));
    }

    constexpr std::string_view invalidPositionPrefix = "cannot build GTP sync for invalid game position: ";
    if (warning.starts_with(invalidPositionPrefix)) {
        const std::string_view detail = warning.substr(invalidPositionPrefix.size());
        return QString::fromUtf8("无法为无效棋局局面构建 GTP 同步：") +
            localizedCoreError(detail, language);
    }
    constexpr std::string_view afterResignPrefix = "cannot build GTP sync for node after resign: ";
    if (warning.starts_with(afterResignPrefix)) {
        const std::string_view nodeId = warning.substr(afterResignPrefix.size());
        return QString::fromUtf8("无法为认输后的节点构建 GTP 同步：") +
            QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size()));
    }
    constexpr std::string_view midGameSetupPrefix =
        "cannot build GTP sync for node with mid-game setup stones: ";
    if (warning.starts_with(midGameSetupPrefix)) {
        const std::string_view nodeId = warning.substr(midGameSetupPrefix.size());
        return QString::fromUtf8("无法为包含中盘摆子的节点构建 GTP 同步：") +
            QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size()));
    }

    constexpr std::string_view nodePrefix = "node ";
    constexpr std::string_view missingNodeSuffix = " does not exist and was skipped";
    if (warning.starts_with(nodePrefix) && warning.ends_with(missingNodeSuffix)) {
        const std::string_view nodeId =
            warning.substr(nodePrefix.size(), warning.size() - nodePrefix.size() - missingNodeSuffix.size());
        return QString::fromUtf8("节点 %1 不存在，已跳过")
            .arg(QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size())));
    }
    constexpr std::string_view followsResignMiddle = " follows resign node ";
    constexpr std::string_view skippedSuffix = " and was skipped";
    if (warning.starts_with(nodePrefix) && warning.ends_with(skippedSuffix)) {
        const std::size_t middlePosition = warning.find(followsResignMiddle, nodePrefix.size());
        if (middlePosition != std::string_view::npos) {
            const std::string_view nodeId = warning.substr(nodePrefix.size(), middlePosition - nodePrefix.size());
            const std::size_t resignNodeBegin = middlePosition + followsResignMiddle.size();
            const std::string_view resignNodeId =
                warning.substr(resignNodeBegin, warning.size() - resignNodeBegin - skippedSuffix.size());
            return QString::fromUtf8("节点 %1 位于认输节点 %2 之后，已跳过")
                .arg(QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size())))
                .arg(QString::fromUtf8(resignNodeId.data(), static_cast<qsizetype>(resignNodeId.size())));
        }
    }
    constexpr std::string_view skippedAnalysisSuffix = "; analysis was skipped";
    if (warning.starts_with(nodePrefix) && warning.ends_with(skippedAnalysisSuffix)) {
        const std::size_t detailSeparator = warning.find(": ", nodePrefix.size());
        if (detailSeparator != std::string_view::npos) {
            const std::string_view nodeId = warning.substr(nodePrefix.size(), detailSeparator - nodePrefix.size());
            const std::size_t detailBegin = detailSeparator + 2;
            const std::string_view detail =
                warning.substr(detailBegin, warning.size() - detailBegin - skippedAnalysisSuffix.size());
            return QString::fromUtf8("节点 %1：%2；已跳过分析")
                .arg(QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size())))
                .arg(localizedCoreError(detail, language));
        }
    }
    constexpr std::string_view unsupportedSidecarFormatPrefix = "unsupported analysis sidecar format: ";
    if (warning.starts_with(unsupportedSidecarFormatPrefix)) {
        const std::string_view format = warning.substr(unsupportedSidecarFormatPrefix.size());
        return QString::fromUtf8("不支持的分析附加文件格式：") +
            QString::fromUtf8(format.data(), static_cast<qsizetype>(format.size()));
    }
    constexpr std::string_view sidecarCollectionIndexPrefix = "analysis sidecar collectionGameIndex ";
    constexpr std::string_view sidecarCollectionIndexMiddle = " does not match selected SGF game index ";
    if (warning.starts_with(sidecarCollectionIndexPrefix)) {
        const std::size_t middlePosition =
            warning.find(sidecarCollectionIndexMiddle, sidecarCollectionIndexPrefix.size());
        if (middlePosition != std::string_view::npos) {
            const std::string_view sidecarIndex =
                warning.substr(sidecarCollectionIndexPrefix.size(), middlePosition - sidecarCollectionIndexPrefix.size());
            const std::string_view selectedIndex =
                warning.substr(middlePosition + sidecarCollectionIndexMiddle.size());
            return QString::fromUtf8("分析附加文件 collectionGameIndex %1 与所选 SGF 对局索引 %2 不匹配")
                .arg(QString::fromUtf8(sidecarIndex.data(), static_cast<qsizetype>(sidecarIndex.size())))
                .arg(QString::fromUtf8(selectedIndex.data(), static_cast<qsizetype>(selectedIndex.size())));
        }
    }
    constexpr std::string_view sidecarNodePrefix = "analysis sidecar node ";
    constexpr std::string_view sidecarNodeMissingSuffix = " does not exist";
    if (warning.starts_with(sidecarNodePrefix) && warning.ends_with(sidecarNodeMissingSuffix)) {
        const std::string_view nodeId =
            warning.substr(sidecarNodePrefix.size(), warning.size() - sidecarNodePrefix.size() - sidecarNodeMissingSuffix.size());
        return QString::fromUtf8("分析附加文件节点 %1 不存在")
            .arg(QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size())));
    }
    constexpr std::string_view sidecarNodeMismatchSuffix = " does not match current position";
    if (warning.starts_with(sidecarNodePrefix) && warning.ends_with(sidecarNodeMismatchSuffix)) {
        const std::string_view nodeId =
            warning.substr(sidecarNodePrefix.size(), warning.size() - sidecarNodePrefix.size() - sidecarNodeMismatchSuffix.size());
        return QString::fromUtf8("分析附加文件节点 %1 与当前局面不匹配")
            .arg(QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size())));
    }
    constexpr std::string_view sidecarNodeNoAnalysisSuffix = " has no usable analysis";
    if (warning.starts_with(sidecarNodePrefix) && warning.ends_with(sidecarNodeNoAnalysisSuffix)) {
        const std::string_view nodeId =
            warning.substr(sidecarNodePrefix.size(), warning.size() - sidecarNodePrefix.size() - sidecarNodeNoAnalysisSuffix.size());
        return QString::fromUtf8("分析附加文件节点 %1 没有可用分析")
            .arg(QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size())));
    }
    constexpr std::string_view sidecarNodeDetailSeparator = ": ";
    if (warning.starts_with(sidecarNodePrefix)) {
        const std::size_t detailSeparator = warning.find(sidecarNodeDetailSeparator, sidecarNodePrefix.size());
        if (detailSeparator != std::string_view::npos) {
            const std::string_view nodeId = warning.substr(sidecarNodePrefix.size(), detailSeparator - sidecarNodePrefix.size());
            const std::string_view detail = warning.substr(detailSeparator + sidecarNodeDetailSeparator.size());
            return QString::fromUtf8("分析附加文件节点 %1：%2")
                .arg(QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size())))
                .arg(localizedCoreError(detail, language));
        }
    }
    constexpr std::string_view legacyNodePrefix = "legacy analysis node ";
    constexpr std::string_view legacyNodeNoDataSuffix = " has no usable LZ or comment data";
    if (warning.starts_with(legacyNodePrefix) && warning.ends_with(legacyNodeNoDataSuffix)) {
        const std::string_view nodeId =
            warning.substr(legacyNodePrefix.size(), warning.size() - legacyNodePrefix.size() - legacyNodeNoDataSuffix.size());
        return QString::fromUtf8("旧 SGF 分析节点 %1 没有可用 LZ 或注释数据")
            .arg(QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size())));
    }
    constexpr std::string_view legacyNodeDetailSeparator = ": ";
    if (warning.starts_with(legacyNodePrefix)) {
        const std::size_t detailSeparator = warning.find(legacyNodeDetailSeparator, legacyNodePrefix.size());
        if (detailSeparator != std::string_view::npos) {
            const std::string_view nodeId = warning.substr(legacyNodePrefix.size(), detailSeparator - legacyNodePrefix.size());
            const std::string_view detail = warning.substr(detailSeparator + legacyNodeDetailSeparator.size());
            return QString::fromUtf8("旧 SGF 分析节点 %1：%2")
                .arg(QString::fromUtf8(nodeId.data(), static_cast<qsizetype>(nodeId.size())))
                .arg(localizedCoreError(detail, language));
        }
    }
    return fallback;
}

QString localizedAnalysisError(std::string_view error, LanguageMode language)
{
    QString text = QString::fromUtf8(error.data(), static_cast<qsizetype>(error.size()));
    if (language != LanguageMode::Chinese) {
        return text;
    }

    constexpr std::string_view requestPrefix = "analysis request failed: ";
    constexpr std::string_view processPrefix = "analysis process failed: ";
    if (error.starts_with(requestPrefix)) {
        const std::string_view detail = error.substr(requestPrefix.size());
        text = QString::fromUtf8("分析请求失败：") +
            QString::fromUtf8(detail.data(), static_cast<qsizetype>(detail.size()));
    } else if (error.starts_with(processPrefix)) {
        const std::string_view detail = error.substr(processPrefix.size());
        text = QString::fromUtf8("分析进程失败：") +
            QString::fromUtf8(detail.data(), static_cast<qsizetype>(detail.size()));
    }
    text.replace(QStringLiteral("\nraw: "), QString::fromUtf8("\n原始响应："));
    return text;
}

QString rawResponsePrefix(LanguageMode language)
{
    return language == LanguageMode::Chinese ? QString::fromUtf8("原始响应：") : QStringLiteral("raw: ");
}

QString localizedEngineLogSource(const QString& source, LanguageMode language)
{
    if (language != LanguageMode::Chinese) {
        return source;
    }
    if (source == QStringLiteral("main")) {
        return QString::fromUtf8("主引擎");
    }
    if (source == QStringLiteral("compare")) {
        return QString::fromUtf8("对比引擎");
    }
    if (source == QStringLiteral("analysis")) {
        return QString::fromUtf8("分析");
    }
    return source;
}

QString localizedProcessDiagnostic(const QString& message, LanguageMode language)
{
    if (language != LanguageMode::Chinese) {
        return message;
    }

    QStringList lines = message.split('\n');
    for (QString& line : lines) {
        if (line.startsWith(QStringLiteral("KataGo analysis ended before all responses: "))) {
            line.remove(0, QStringLiteral("KataGo analysis ended before all responses: ").size());
            line.replace(QStringLiteral(" completed"), QString::fromUtf8(" 已完成"));
            line.prepend(QString::fromUtf8("KataGo 分析在收到全部响应前结束："));
        } else if (line.startsWith(QStringLiteral("KataGo analysis exited: code "))) {
            line.remove(0, QStringLiteral("KataGo analysis exited: code ").size());
            line.replace(QStringLiteral(", status "), QString::fromUtf8("，状态 "));
            line.prepend(QString::fromUtf8("KataGo 分析已退出：代码 "));
        } else if (line.startsWith(QStringLiteral("KataGo exited: code "))) {
            line.remove(0, QStringLiteral("KataGo exited: code ").size());
            line.replace(QStringLiteral(", status "), QString::fromUtf8("，状态 "));
            line.prepend(QString::fromUtf8("KataGo 已退出：代码 "));
        } else if (line.startsWith(QStringLiteral("Process error: "))) {
            line.replace(0, QStringLiteral("Process error: ").size(), QString::fromUtf8("进程错误："));
        } else if (line.startsWith(QStringLiteral("Command: "))) {
            line.replace(0, QStringLiteral("Command: ").size(), QString::fromUtf8("命令："));
        } else if (line.startsWith(QStringLiteral("Working directory: "))) {
            line.replace(0, QStringLiteral("Working directory: ").size(), QString::fromUtf8("工作目录："));
            line.replace(QStringLiteral(" (inherited)"), QString::fromUtf8("（继承）"));
        } else if (line == QStringLiteral("Stderr: (none)")) {
            line = QString::fromUtf8("标准错误：（无）");
        } else if (line.startsWith(QStringLiteral("Stderr: "))) {
            line.replace(0, QStringLiteral("Stderr: ").size(), QString::fromUtf8("标准错误："));
        } else if (line == QStringLiteral("KataGo executable, model, and analysis config are required")) {
            line = QString::fromUtf8("需要 KataGo 可执行文件、模型和分析配置");
        } else if (line == QStringLiteral("KataGo executable, model, and GTP config are required")) {
            line = QString::fromUtf8("需要 KataGo 可执行文件、模型和 GTP 配置");
        } else if (line == QStringLiteral("KataGo process is not running")) {
            line = QString::fromUtf8("KataGo 进程未运行");
        }
    }
    return lines.join('\n');
}

QString localizedCompareStatusMessage(const QString& message, LanguageMode language)
{
    if (language == LanguageMode::Chinese) {
        return QString::fromUtf8("对比：") + message;
    }
    return QStringLiteral("[compare] ") + message;
}

QString compareConsolePrefix(LanguageMode language)
{
    if (language == LanguageMode::Chinese) {
        return QString::fromUtf8("对比：");
    }
    return QStringLiteral("[compare] ");
}

const char* batchCompletionMessageKey(lizzie::app::BatchAnalysisCompletionMessage message)
{
    switch (message) {
    case lizzie::app::BatchAnalysisCompletionMessage::EstimateFailed:
        return "estimateFailed";
    case lizzie::app::BatchAnalysisCompletionMessage::BatchFailed:
        return "batchFailed";
    case lizzie::app::BatchAnalysisCompletionMessage::EstimateCancelled:
        return "estimateCancelled";
    case lizzie::app::BatchAnalysisCompletionMessage::BatchCancelled:
        return "batchCancelled";
    case lizzie::app::BatchAnalysisCompletionMessage::EstimateComplete:
        return "estimateComplete";
    case lizzie::app::BatchAnalysisCompletionMessage::BatchComplete:
        return "batchComplete";
    case lizzie::app::BatchAnalysisCompletionMessage::BatchFailedWithOutput:
        return "batchFailedPrefix";
    case lizzie::app::BatchAnalysisCompletionMessage::BatchCompleteWithOutput:
        return "batchCompletePrefix";
    }
    return "batchComplete";
}

bool sameMove(const lizzie::core::Move& left, const lizzie::core::Move& right)
{
    return left.color == right.color && left.type == right.type && (!left.isPlay() || left.point == right.point);
}

bool currentNodeMoveWasPass(const lizzie::core::GameModel& game)
{
    const lizzie::core::GameNode& node = game.node(game.currentId());
    return node.move.has_value() && node.move->type == lizzie::core::MoveType::Pass;
}

void setCompareRow(
    QTableWidget* table,
    int row,
    const QString& name,
    const std::optional<lizzie::core::AnalysisSnapshot>& snapshot,
    LanguageMode language)
{
    table->setItem(row, 0, new QTableWidgetItem(name));
    if (!snapshot.has_value() || snapshot->candidates.empty()) {
        for (int column = 1; column < table->columnCount(); ++column) {
            table->setItem(row, column, new QTableWidgetItem("-"));
        }
        return;
    }

    const lizzie::core::MoveCandidate& best = snapshot->candidates.front();
    table->setItem(row, 1, new QTableWidgetItem(candidateMoveText(best.move, language)));
    table->setItem(row, 2, new QTableWidgetItem(QString::number(snapshot->rootVisits)));
    table->setItem(row, 3, new QTableWidgetItem(percentageText(blackPerspectiveWinrate(*snapshot))));
    table->setItem(row, 4, new QTableWidgetItem(QString::number(blackPerspectiveScoreMean(*snapshot), 'f', 2)));
    table->setItem(row, 5, new QTableWidgetItem(QString::number(best.scoreStdev, 'f', 2)));
    table->setItem(row, 6, new QTableWidgetItem(percentageText(best.policy)));
    table->setItem(row, 7, new QTableWidgetItem(pvText(best.pv, language)));
    table->setItem(row, 8, new QTableWidgetItem(pvVisitsText(best.pvVisits)));
}

void setActionShortcut(QAction* action, const QKeySequence& sequence, const QString& label)
{
    if (action == nullptr) {
        return;
    }
    action->setShortcut(sequence);
    const QString shortcut = sequence.toString(QKeySequence::NativeText);
    action->setToolTip(shortcut.isEmpty() ? label : QString("%1 (%2)").arg(label, shortcut));
}

struct RuleComboOption {
    const char* canonical;
    const char* zhLabel;
};

constexpr std::array<RuleComboOption, 6> kRuleComboOptions{{
    {"Chinese", "中国规则"},
    {"Japanese", "日本规则"},
    {"Tromp-Taylor", "Tromp-Taylor"},
    {"AGA", "AGA"},
    {"New Zealand", "新西兰规则"},
    {"Korean", "韩国规则"},
}};

QString ruleDisplayText(LanguageMode language, const RuleComboOption& option)
{
    return language == LanguageMode::Chinese ? QString::fromUtf8(option.zhLabel) : QString::fromLatin1(option.canonical);
}

void populateRulesCombo(QComboBox* combo, LanguageMode language)
{
    combo->clear();
    for (const RuleComboOption& option : kRuleComboOptions) {
        combo->addItem(ruleDisplayText(language, option), QString::fromLatin1(option.canonical));
    }
}

void setRulesComboValue(QComboBox* combo, LanguageMode language, const QString& value)
{
    populateRulesCombo(combo, language);
    const QString trimmed = value.trimmed();
    const int canonicalIndex = combo->findData(trimmed);
    if (canonicalIndex >= 0) {
        combo->setCurrentIndex(canonicalIndex);
        return;
    }
    const int displayIndex = combo->findText(trimmed);
    if (displayIndex >= 0) {
        combo->setCurrentIndex(displayIndex);
        return;
    }
    const std::optional<lizzie::core::Ruleset> parsed = lizzie::core::rulesetFromString(trimmed.toStdString());
    if (parsed.has_value()) {
        for (int index = 0; index < combo->count(); ++index) {
            const std::optional<lizzie::core::Ruleset> option =
                lizzie::core::rulesetFromString(combo->itemData(index).toString().toStdString());
            if (option == parsed) {
                combo->setCurrentIndex(index);
                return;
            }
        }
    }
    if (trimmed.isEmpty()) {
        combo->setCurrentIndex(0);
        return;
    }
    combo->setCurrentText(trimmed);
}

QString currentRulesComboValue(const QComboBox* combo)
{
    if (combo->currentIndex() >= 0) {
        if (combo->isEditable() && combo->currentText().trimmed() != combo->itemText(combo->currentIndex()).trimmed()) {
            return combo->currentText().trimmed();
        }
        const QVariant value = combo->currentData();
        if (value.isValid() && !value.toString().isEmpty()) {
            return value.toString();
        }
    }
    return combo->currentText().trimmed();
}

bool ownershipShownOnMainBoard(OwnershipDisplayMode mode)
{
    return mode == OwnershipDisplayMode::MainBoard || mode == OwnershipDisplayMode::BothBoards;
}

bool ownershipShownOnMiniBoard(OwnershipDisplayMode mode)
{
    return mode == OwnershipDisplayMode::MiniBoard || mode == OwnershipDisplayMode::BothBoards;
}

void populateNewGameHandicapCombo(QComboBox* combo)
{
    combo->clear();
    combo->addItem(QStringLiteral("0"), 0);
    for (int handicap = 2; handicap <= 9; ++handicap) {
        combo->addItem(QString::number(handicap), handicap);
    }
}

bool supportsNewGameHandicap(lizzie::core::BoardSize boardSize)
{
    return !lizzie::app::standardHandicapStones(boardSize, 2).empty();
}

void updateNewGameHandicapCombo(QComboBox* combo, int width, int height)
{
    const bool supported = supportsNewGameHandicap(lizzie::core::BoardSize{width, height});
    combo->setEnabled(supported);
    if (!supported) {
        combo->setCurrentIndex(0);
    }
}

int currentHandicapComboValue(const QComboBox* combo)
{
    return combo->currentData().toInt();
}

int countMoveNodes(const lizzie::core::GameModel& model, lizzie::core::NodeId nodeId)
{
    int count = model.node(nodeId).move.has_value() ? 1 : 0;
    for (lizzie::core::NodeId childId : model.node(nodeId).children) {
        count += countMoveNodes(model, childId);
    }
    return count;
}

QString sgfCollectionGameLabel(const lizzie::core::GameModel& model, int index, LanguageMode language)
{
    const bool zh = language == LanguageMode::Chinese;
    const lizzie::core::GameInfo& info = model.gameInfo();
    const QString black = QString::fromStdString(info.blackPlayer).trimmed();
    const QString white = QString::fromStdString(info.whitePlayer).trimmed();
    const QString matchup = black.isEmpty() && white.isEmpty()
        ? (zh ? QString::fromUtf8("未命名") : QStringLiteral("Untitled"))
        : QStringLiteral("%1 vs %2").arg(black.isEmpty() ? QStringLiteral("?") : black,
                                         white.isEmpty() ? QStringLiteral("?") : white);
    const int moves = countMoveNodes(model, model.rootId());
    return zh ? QString::fromUtf8("第 %1 局：%2（%3 手）").arg(index + 1).arg(matchup).arg(moves)
              : QStringLiteral("Game %1: %2 (%3 moves)").arg(index + 1).arg(matchup).arg(moves);
}

QString localizedDialogButtonText(LanguageMode language, QDialogButtonBox::StandardButton button)
{
    if (language != LanguageMode::Chinese) {
        return {};
    }
    switch (button) {
    case QDialogButtonBox::Ok:
        return QString::fromUtf8("确定");
    case QDialogButtonBox::Cancel:
        return QString::fromUtf8("取消");
    default:
        return {};
    }
}

void localizeDialogButton(QDialogButtonBox* buttons, LanguageMode language, QDialogButtonBox::StandardButton button)
{
    if (buttons == nullptr) {
        return;
    }
    const QString text = localizedDialogButtonText(language, button);
    if (text.isEmpty()) {
        return;
    }
    if (QPushButton* pushButton = buttons->button(button)) {
        pushButton->setText(text);
    }
}

void localizeDialogButtons(QDialogButtonBox* buttons, LanguageMode language)
{
    localizeDialogButton(buttons, language, QDialogButtonBox::Ok);
    localizeDialogButton(buttons, language, QDialogButtonBox::Cancel);
}

QString localizedMessageBoxButtonText(LanguageMode language, QMessageBox::StandardButton button)
{
    if (language != LanguageMode::Chinese) {
        return {};
    }
    switch (button) {
    case QMessageBox::Ok:
        return QString::fromUtf8("确定");
    case QMessageBox::Cancel:
        return QString::fromUtf8("取消");
    case QMessageBox::Yes:
        return QString::fromUtf8("是");
    case QMessageBox::No:
        return QString::fromUtf8("否");
    default:
        return {};
    }
}

void localizeMessageBoxButton(QMessageBox& box, LanguageMode language, QMessageBox::StandardButton button)
{
    const QString text = localizedMessageBoxButtonText(language, button);
    if (text.isEmpty()) {
        return;
    }
    if (QAbstractButton* abstractButton = box.button(button)) {
        abstractButton->setText(text);
    }
}

void localizeMessageBoxButtons(QMessageBox& box, LanguageMode language)
{
    localizeMessageBoxButton(box, language, QMessageBox::Ok);
    localizeMessageBoxButton(box, language, QMessageBox::Cancel);
    localizeMessageBoxButton(box, language, QMessageBox::Yes);
    localizeMessageBoxButton(box, language, QMessageBox::No);
}

QMessageBox::StandardButton askYesNoQuestion(
    QWidget* parent,
    LanguageMode language,
    const QString& title,
    const QString& text,
    QMessageBox::StandardButton defaultButton = QMessageBox::No)
{
    QMessageBox box(QMessageBox::Question, title, text, QMessageBox::Yes | QMessageBox::No, parent);
    box.setDefaultButton(defaultButton);
    localizeMessageBoxButtons(box, language);
    return static_cast<QMessageBox::StandardButton>(box.exec());
}

void showWarning(QWidget* parent, LanguageMode language, const QString& title, const QString& text)
{
    QMessageBox box(QMessageBox::Warning, title, text, QMessageBox::Ok, parent);
    localizeMessageBoxButtons(box, language);
    box.exec();
}

std::optional<std::size_t> selectSgfCollectionGame(
    QWidget* parent,
    const std::vector<lizzie::core::GameModel>& games,
    LanguageMode language,
    const QString& title,
    const QString& label)
{
    QDialog dialog(parent);
    dialog.setWindowTitle(title);
    auto* layout = new QFormLayout(&dialog);
    auto* selector = new QComboBox(&dialog);
    for (std::size_t index = 0; index < games.size(); ++index) {
        selector->addItem(
            sgfCollectionGameLabel(games[index], static_cast<int>(index), language),
            static_cast<int>(index));
    }
    layout->addRow(label, selector);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    localizeDialogButtons(buttons, language);
    layout->addRow(buttons);
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return std::nullopt;
    }
    return static_cast<std::size_t>(selector->currentData().toInt());
}

QString translated(LanguageMode language, const char* key)
{
    const bool zh = language == LanguageMode::Chinese;
    const QString name(key);
    if (name == "appTitle") {
        return zh ? "LizzieYzy Qt" : "LizzieYzy Qt";
    }
    if (name == "new") {
        return zh ? "新建" : "New";
    }
    if (name == "newGameTitle") {
        return zh ? "新建棋局" : "New Game";
    }
    if (name == "open") {
        return zh ? "打开" : "Open";
    }
    if (name == "save") {
        return zh ? "保存" : "Save";
    }
    if (name == "saveAs") {
        return zh ? "另存为" : "Save As";
    }
    if (name == "openSgfTitle") {
        return zh ? "打开 SGF" : "Open SGF";
    }
    if (name == "selectSgfGameTitle") {
        return zh ? "选择 SGF 棋局" : "Select SGF Game";
    }
    if (name == "selectSgfGame") {
        return zh ? "棋局" : "Game";
    }
    if (name == "sgfCollectionLoadedFirst") {
        return zh ? "SGF 合集包含 %1 局；已加载第一局"
                  : "SGF collection contains %1 games; loaded the first game";
    }
    if (name == "sgfCollectionStoredIndexOutOfRange") {
        return zh ? "保存的 SGF 合集索引 %1 超出 %2 局范围；已加载第一局"
                  : "Stored SGF collection index %1 is outside the %2-game collection; loaded the first game";
    }
    if (name == "saveSgfTitle") {
        return zh ? "保存 SGF" : "Save SGF";
    }
    if (name == "sgfFileFilter") {
        return zh ? "SGF 棋谱 (*.sgf);;所有文件 (*)" : "Smart Game Format (*.sgf);;All files (*)";
    }
    if (name == "gameInfo") {
        return zh ? "对局信息" : "Game Info";
    }
    if (name == "gameInfoTooltip") {
        return zh ? "编辑 SGF 对局信息" : "Edit SGF game information";
    }
    if (name == "black") {
        return zh ? "黑方" : "Black";
    }
    if (name == "white") {
        return zh ? "白方" : "White";
    }
    if (name == "blackRank") {
        return zh ? "黑方段位" : "Black Rank";
    }
    if (name == "whiteRank") {
        return zh ? "白方段位" : "White Rank";
    }
    if (name == "result") {
        return zh ? "结果" : "Result";
    }
    if (name == "date") {
        return zh ? "日期" : "Date";
    }
    if (name == "source") {
        return zh ? "来源" : "Source";
    }
    if (name == "width") {
        return zh ? "宽度" : "Width";
    }
    if (name == "height") {
        return zh ? "高度" : "Height";
    }
    if (name == "komi") {
        return zh ? "贴目" : "Komi";
    }
    if (name == "rules") {
        return zh ? "规则" : "Rules";
    }
    if (name == "handicap") {
        return zh ? "让子" : "Handicap";
    }
    if (name == "gtpWidthTooltip") {
        return zh ? "SGF 和批量分析最多支持 52 列；实时 GTP 对超过 25 列会给出诊断。"
                  : "SGF and batch analysis support up to 52 columns; realtime GTP reports diagnostics above 25.";
    }
    if (name == "analyze") {
        return zh ? "分析" : "Analyze";
    }
    if (name == "estimate") {
        return zh ? "形势" : "Estimate";
    }
    if (name == "batch") {
        return zh ? "批量" : "Batch";
    }
    if (name == "batchAnalysis") {
        return zh ? "批量分析" : "Batch analysis";
    }
    if (name == "cancel") {
        return zh ? "取消" : "Cancel";
    }
    if (name == "estimateSettingsIncomplete") {
        return zh ? "形势判断需要 KataGo、模型和分析配置。"
                  : "Estimate requires KataGo, model, and analysis config.";
    }
    if (name == "batchSettingsIncomplete") {
        return zh ? "批量分析需要 KataGo、模型和分析配置。"
                  : "Batch analysis requires KataGo, model, and analysis config.";
    }
    if (name == "noCurrentNodeToEstimate") {
        return zh ? "没有可判断的当前节点" : "No current node to estimate";
    }
    if (name == "noNodesToAnalyze") {
        return zh ? "没有可分析的节点" : "No nodes to analyze";
    }
    if (name == "estimateFailed") {
        return zh ? "形势判断失败" : "Estimate failed";
    }
    if (name == "estimateCancelled") {
        return zh ? "形势判断已取消" : "Estimate cancelled";
    }
    if (name == "estimateComplete") {
        return zh ? "形势判断完成" : "Estimate complete";
    }
    if (name == "batchFailed") {
        return zh ? "批量分析失败" : "Batch analysis failed";
    }
    if (name == "batchCancelled") {
        return zh ? "批量分析已取消" : "Batch analysis cancelled";
    }
    if (name == "batchComplete") {
        return zh ? "批量分析完成" : "Batch analysis complete";
    }
    if (name == "batchFailedPrefix") {
        return zh ? "批量分析失败：" : "Batch analysis failed: ";
    }
    if (name == "batchCompletePrefix") {
        return zh ? "批量分析完成：" : "Batch analysis complete: ";
    }
    if (name == "engineSettingsIncomplete") {
        return zh ? "引擎设置不完整" : "engine settings incomplete";
    }
    if (name == "compareEngineSettingsIncomplete") {
        return zh ? "对比引擎设置不完整" : "compare engine settings incomplete";
    }
    if (name == "realtimeStoppedSettingsIncomplete") {
        return zh ? "实时分析已停止：引擎设置不完整"
                  : "realtime analysis stopped: engine settings incomplete";
    }
    if (name == "compareStoppedSettingsIncomplete") {
        return zh ? "对比分析已停止：引擎设置不完整"
                  : "compare analysis stopped: engine settings incomplete";
    }
    if (name == "restartingEngine") {
        return zh ? "正在重启引擎" : "Restarting engine";
    }
    if (name == "restartEngine") {
        return zh ? "重启引擎" : "Restart";
    }
    if (name == "compare") {
        return zh ? "对比" : "Compare";
    }
    if (name == "aiMove") {
        return zh ? "AI落子" : "AI Move";
    }
    if (name == "humanVsAi") {
        return zh ? "人机对局" : "Human vs AI";
    }
    if (name == "humanVsAiStarted") {
        return zh ? "人机对局已开始：请落子，KataGo 会回应。"
                  : "Human vs AI started: play a move and KataGo will reply.";
    }
    if (name == "humanVsAiRequiresMainGtp") {
        return zh ? "人机对局需要主引擎 GTP 设置" : "human-vs-ai requires main GTP settings";
    }
    if (name == "selfPlay") {
        return zh ? "自对局" : "Self Play";
    }
    if (name == "engineGame") {
        return zh ? "引擎对局" : "Engine Game";
    }
    if (name == "engineGameRequiresGtp") {
        return zh ? "引擎对局需要主引擎和对比引擎 GTP 设置"
                  : "engine game requires main and compare GTP settings";
    }
    if (name == "engineGameStarted") {
        return zh ? "引擎对局已开始：主引擎执黑" : "Engine game started: main engine plays black";
    }
    if (name == "noGenmoveSupport") {
        return zh ? "KataGo 未报告 genmove 支持。" : "KataGo does not report genmove support.";
    }
    if (name == "noAnalyzeSupport") {
        return zh ? "KataGo 未报告 kata-analyze 支持。" : "KataGo does not report kata-analyze support.";
    }
    if (name == "compareNoGenmoveSupport") {
        return zh ? "对比 KataGo 未报告 genmove 支持。"
                  : "Comparison KataGo does not report genmove support.";
    }
    if (name == "compareNoAnalyzeSupport") {
        return zh ? "对比 KataGo 未报告 kata-analyze 支持。"
                  : "Comparison KataGo does not report kata-analyze support.";
    }
    if (name == "listCommandsFailedPrefix") {
        return zh ? "list_commands 失败：" : "list_commands failed: ";
    }
    if (name == "gtpCommandFailedPrefix") {
        return zh ? "%1 失败：" : "%1 failed: ";
    }
    if (name == "realtimeSyncFailedPrefix") {
        return zh ? "实时分析同步失败：" : "realtime analysis sync failed: ";
    }
    if (name == "compareSyncFailedPrefix") {
        return zh ? "对比分析同步失败：" : "analysis sync failed: ";
    }
    if (name == "syncCommandDispatchFailed") {
        return zh ? "无法发送 GTP 同步命令" : "could not send GTP sync commands";
    }
    if (name == "analyzeCommandDispatchFailed") {
        return zh ? "无法发送 kata-analyze 命令" : "could not send kata-analyze command";
    }
    if (name == "genmoveSyncFailedPrefix") {
        return zh ? "引擎落子同步失败：" : "engine move sync failed: ";
    }
    if (name == "compareGenmoveSyncFailedPrefix") {
        return zh ? "对比引擎落子同步失败：" : "compare engine move sync failed: ";
    }
    if (name == "genmoveCommandDispatchFailed") {
        return zh ? "无法发送 genmove 命令" : "could not send genmove command";
    }
    if (name == "aiMoveSyncSkipped") {
        return zh ? "AI 落子已跳过：引擎同步失败" : "AI move skipped: engine sync failed";
    }
    if (name == "humanVsAiReplySyncSkipped") {
        return zh ? "人机对局 AI 回复已跳过：引擎同步失败"
                  : "Human-vs-AI reply skipped: engine sync failed";
    }
    if (name == "selfPlayMoveSyncSkipped") {
        return zh ? "自对局落子已跳过：引擎同步失败" : "Self-play move skipped: engine sync failed";
    }
    if (name == "engineGameMainMoveSyncSkipped") {
        return zh ? "主引擎落子已跳过：引擎同步失败" : "Main engine move skipped: engine sync failed";
    }
    if (name == "compareMoveSyncSkipped") {
        return zh ? "对比引擎落子已跳过：引擎同步失败" : "Compare move skipped: engine sync failed";
    }
    if (name == "realtimeSyncSkipped") {
        return zh ? "实时分析已跳过：引擎同步失败" : "Realtime analysis skipped: engine sync failed";
    }
    if (name == "compareSyncSkipped") {
        return zh ? "对比分析已跳过：引擎同步失败" : "Compare analysis skipped: engine sync failed";
    }
    if (name == "realtimeBoardSyncSkipped") {
        return zh ? "实时分析已跳过：局面不能同步到 GTP"
                  : "Realtime analysis skipped: board cannot be synced to GTP";
    }
    if (name == "compareBoardSyncSkipped") {
        return zh ? "对比分析已跳过：局面不能同步到 GTP"
                  : "Compare analysis skipped: board cannot be synced to GTP";
    }
    if (name == "aiMoveBoardSyncSkipped") {
        return zh ? "AI 落子已跳过：局面不能同步到 GTP" : "AI move skipped: board cannot be synced to GTP";
    }
    if (name == "humanVsAiReplyBoardSyncSkipped") {
        return zh ? "人机对局 AI 回复已跳过：局面不能同步到 GTP"
                  : "Human-vs-AI reply skipped: board cannot be synced to GTP";
    }
    if (name == "selfPlayMoveBoardSyncSkipped") {
        return zh ? "自对局落子已跳过：局面不能同步到 GTP"
                  : "Self-play move skipped: board cannot be synced to GTP";
    }
    if (name == "engineGameMainMoveBoardSyncSkipped") {
        return zh ? "主引擎落子已跳过：局面不能同步到 GTP"
                  : "Main engine move skipped: board cannot be synced to GTP";
    }
    if (name == "compareMoveBoardSyncSkipped") {
        return zh ? "对比引擎落子已跳过：局面不能同步到 GTP"
                  : "Compare move skipped: board cannot be synced to GTP";
    }
    if (name == "ignoredStaleEngineMove") {
        return zh ? "已忽略过期的引擎落子" : "Ignored stale engine move";
    }
    if (name == "ignoredStaleEngineMoveResponse") {
        return zh ? "已忽略过期的引擎落子响应" : "ignored stale engine move response";
    }
    if (name == "ignoredStaleCompareEngineMove") {
        return zh ? "已忽略过期的对比引擎落子" : "Ignored stale compare engine move";
    }
    if (name == "couldNotParseEngineMove") {
        return zh ? "无法解析引擎落子" : "Could not parse engine move";
    }
    if (name == "couldNotParseCompareEngineMove") {
        return zh ? "无法解析对比引擎落子" : "Could not parse compare engine move";
    }
    if (name == "engineMoveRejectedPrefix") {
        return zh ? "引擎落子已拒绝：" : "engine move rejected: ";
    }
    if (name == "engineMovePrefix") {
        return zh ? "引擎落子：" : "Engine move: ";
    }
    if (name == "katagoExited") {
        return zh ? "KataGo 已退出：代码 %1，状态 %2" : "KataGo exited: code %1, status %2";
    }
    if (name == "engineNotRunning") {
        return zh ? "GTP 命令已跳过：引擎未运行" : "GTP command skipped: engine is not running";
    }
    if (name == "consoleCommandRejected") {
        return zh ? "GTP 命令已拒绝：语法无效或发送失败" : "GTP command rejected: invalid syntax or send failure";
    }
    if (name == "engineNamePrefix") {
        return zh ? "引擎名称：" : "engine name: ";
    }
    if (name == "engineVersionPrefix") {
        return zh ? "引擎版本：" : "engine version: ";
    }
    if (name == "analysisJsonParseFailedPrefix") {
        return zh ? "分析 JSON 解析失败：" : "analysis JSON parse failed: ";
    }
    if (name == "unknownAnalysisResponsePrefix") {
        return zh ? "已忽略未知 analysis 响应：" : "unknown analysis response ignored: ";
    }
    if (name == "duplicateAnalysisResponsePrefix") {
        return zh ? "已忽略重复 analysis 响应：" : "duplicate analysis response ignored: ";
    }
    if (name == "loadedAnalysisSidecar") {
        return zh ? "已加载分析附加文件：%1 个节点" : "Loaded analysis sidecar: %1 nodes";
    }
    if (name == "importedLegacySgfAnalysis") {
        return zh ? "已导入旧 SGF 分析：%1 个节点" : "imported legacy SGF analysis: %1 nodes";
    }
    if (name == "openSgfLegacyAnalysisStatus") {
        return zh ? "%1（旧 SGF 分析：%2 个节点）" : "%1 (legacy analysis: %2 nodes)";
    }
    if (name == "openSgfCollectionGameStatus") {
        return zh ? "%1（第 %2/%3 局）" : "%1 (game %2/%3)";
    }
    if (name == "legacySgfAnalysisImportDisabled") {
        return zh ? "旧 SGF 分析导入已禁用" : "legacy SGF analysis import disabled";
    }
    if (name == "analysisSidecarLoadDisabled") {
        return zh ? "分析附加文件加载已禁用" : "analysis sidecar load disabled";
    }
    if (name == "lastSessionSgfMissingPrefix") {
        return zh ? "上次会话 SGF 缺失：" : "last session SGF is missing: ";
    }
    if (name == "sidecarReadFailedPrefix") {
        return zh ? "附加文件读取失败：" : "sidecar read failed: ";
    }
    if (name == "sidecarParseFailedPrefix") {
        return zh ? "附加文件解析失败：" : "sidecar parse failed: ";
    }
    if (name == "sidecarWriteFailedPrefix") {
        return zh ? "附加文件写入失败：" : "sidecar write failed: ";
    }
    if (name == "sgfWriteFailedPrefix") {
        return zh ? "SGF 写入失败：" : "SGF write failed: ";
    }
    if (name == "unmatchedStaleBatchResponsePrefix") {
        return zh ? "未匹配或过期的批量分析响应：" : "unmatched or stale batch response: ";
    }
    if (name == "analysisRequestFailedPrefix") {
        return zh ? "分析请求失败：" : "analysis request failed: ";
    }
    if (name == "analysisProcessFailedPrefix") {
        return zh ? "分析进程失败：" : "analysis process failed: ";
    }
    if (name == "ignoredStaleAnalysisFailurePrefix") {
        return zh ? "已忽略过期的分析失败：" : "ignored stale analysis failure: ";
    }
    if (name == "ignoredStaleAnalysisProcessFailurePrefix") {
        return zh ? "已忽略过期的分析进程失败：" : "ignored stale analysis process failure: ";
    }
    if (name == "previous") {
        return zh ? "后退" : "Previous";
    }
    if (name == "next") {
        return zh ? "前进" : "Next";
    }
    if (name == "undo") {
        return zh ? "撤销" : "Undo";
    }
    if (name == "cannotUndoRootNode") {
        return zh ? "不能撤销根节点" : "Cannot undo the root node";
    }
    if (name == "pass") {
        return zh ? "停一手" : "Pass";
    }
    if (name == "resign") {
        return zh ? "认输" : "Resign";
    }
    if (name == "resignTooltip") {
        return zh ? "当前一方认输" : "Resign current side";
    }
    if (name == "resignQuestion") {
        return zh ? "%1 要认输吗？" : "%1 resigns?";
    }
    if (name == "promoteVariation") {
        return zh ? "设为主线" : "Promote Variation";
    }
    if (name == "promoteVariationTooltip") {
        return zh ? "将当前分支设为主线" : "Promote current branch to main line";
    }
    if (name == "deleteBranch") {
        return zh ? "删除分支" : "Delete Branch";
    }
    if (name == "deleteBranchTooltip") {
        return zh ? "删除当前分支" : "Delete current branch";
    }
    if (name == "deleteBranchQuestion") {
        return zh ? "删除当前分支及其所有后续节点？" : "Delete the current branch and all descendants?";
    }
    if (name == "cannotPromoteNode") {
        return zh ? "当前节点不能设为主线" : "Cannot promote this node";
    }
    if (name == "cannotDeleteRootNode") {
        return zh ? "不能删除根节点" : "Cannot delete root node";
    }
    if (name == "cannotDeleteBranch") {
        return zh ? "不能删除分支" : "Cannot delete branch";
    }
    if (name == "playMode") {
        return zh ? "落子" : "Play";
    }
    if (name == "playModeTooltip") {
        return zh ? "落子模式" : "Play moves";
    }
    if (name == "tryMode") {
        return zh ? "试下" : "Try";
    }
    if (name == "tryModeTooltip") {
        return zh ? "以 SGF 分支试下" : "Try moves as SGF variations";
    }
    if (name == "labelMode") {
        return zh ? "标签" : "Label";
    }
    if (name == "labelModeTooltip") {
        return zh ? "编辑 SGF 标签" : "Edit SGF labels";
    }
    if (name == "labelDialogPrompt") {
        return zh ? "文本" : "Text";
    }
    if (name == "circleMark") {
        return zh ? "圆" : "Circle";
    }
    if (name == "circleMarkTooltip") {
        return zh ? "切换圆形标记" : "Toggle circle marks";
    }
    if (name == "squareMark") {
        return zh ? "方" : "Square";
    }
    if (name == "squareMarkTooltip") {
        return zh ? "切换方形标记" : "Toggle square marks";
    }
    if (name == "triangleMark") {
        return zh ? "三角" : "Triangle";
    }
    if (name == "triangleMarkTooltip") {
        return zh ? "切换三角标记" : "Toggle triangle marks";
    }
    if (name == "crossMark") {
        return zh ? "叉" : "Cross";
    }
    if (name == "crossMarkTooltip") {
        return zh ? "切换叉形标记" : "Toggle cross marks";
    }
    if (name == "clearMarkup") {
        return zh ? "清标记" : "Clear";
    }
    if (name == "clearMarkupTooltip") {
        return zh ? "清除标签和标记" : "Clear labels and marks";
    }
    if (name == "setupBlack") {
        return zh ? "放黑子" : "Setup Black";
    }
    if (name == "setupBlackTooltip") {
        return zh ? "编辑 SGF AB 放置黑子" : "Edit SGF AB setup stones";
    }
    if (name == "setupWhite") {
        return zh ? "放白子" : "Setup White";
    }
    if (name == "setupWhiteTooltip") {
        return zh ? "编辑 SGF AW 放置白子" : "Edit SGF AW setup stones";
    }
    if (name == "setupClear") {
        return zh ? "清空交叉点" : "Setup Clear";
    }
    if (name == "setupClearTooltip") {
        return zh ? "编辑 SGF AE 清空交叉点" : "Edit SGF AE setup points";
    }
    if (name == "settings") {
        return zh ? "设置" : "Settings";
    }
    if (name == "katago") {
        return "KataGo";
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
    if (name == "firstRunTitle") {
        return zh ? "首次启动" : "First Run";
    }
    if (name == "firstRunMessage") {
        return zh ? "现在配置 KataGo，或以无引擎模式继续。无引擎时仍可编辑和复盘 SGF。"
                  : "Configure KataGo now, or continue in no-engine mode. SGF editing and review remain available without an engine.";
    }
    if (name == "configureEngine") {
        return zh ? "配置引擎" : "Configure Engine";
    }
    if (name == "selectFile") {
        return zh ? "选择文件" : "Select File";
    }
    if (name == "importJavaConfig") {
        return zh ? "导入 Java 配置" : "Import Java Config";
    }
    if (name == "importJavaConfigTooltip") {
        return zh ? "从 Java 版 config.txt 导入设置" : "Import settings from the Java version's config.txt";
    }
    if (name == "importJavaConfigFilter") {
        return zh ? "Lizzie 配置 (config.txt);;JSON 文件 (*.json);;所有文件 (*)"
                  : "Lizzie config (config.txt);;JSON files (*.json);;All files (*)";
    }
    if (name == "invalidJavaConfig") {
        return zh ? "config.txt 不是有效 JSON" : "config.txt is not valid JSON";
    }
    if (name == "noUsableKataGo") {
        return zh ? "config.txt 中没有可用的本地 KataGo 命令。"
                  : "No usable local KataGo command was found in config.txt.";
    }
    if (name == "importedJavaConfigSummary") {
        return zh ? "已导入 Java 配置：%1 个引擎" : "Imported Java config: %1 engine";
    }
    if (name == "incompleteEngineSettings") {
        return zh ? "需要 KataGo、模型、GTP 配置和分析配置。"
                  : "KataGo, model, GTP config, and analysis config are required.";
    }
    if (name == "noEngineMode") {
        return zh ? "无引擎模式" : "No Engine Mode";
    }
    if (name == "noEngineModeEnabled") {
        return zh ? "已启用无引擎模式" : "No-engine mode enabled";
    }
    if (name == "fileMenu") {
        return zh ? "文件" : "File";
    }
    if (name == "engineMenu") {
        return zh ? "引擎" : "Engine";
    }
    if (name == "navigateMenu") {
        return zh ? "导航" : "Navigate";
    }
    if (name == "markupMenu") {
        return zh ? "标记" : "Markup";
    }
    if (name == "viewMenu") {
        return zh ? "视图" : "View";
    }
    if (name == "toolbar") {
        return zh ? "工具栏" : "Toolbar";
    }
    if (name == "candidates") {
        return zh ? "候选点" : "Candidates";
    }
    if (name == "ownershipBoard") {
        return zh ? "归属小棋盘" : "Ownership Board";
    }
    if (name == "gameTree") {
        return zh ? "棋谱树" : "Game Tree";
    }
    if (name == "comment") {
        return zh ? "注释" : "Comment";
    }
    if (name == "commentPlaceholder") {
        return zh ? "节点注释" : "Node comment";
    }
    if (name == "graph") {
        return zh ? "黑胜率 / 黑目差" : "Black Winrate / Black Score";
    }
    if (name == "console") {
        return zh ? "GTP 控制台" : "GTP Console";
    }
    if (name == "engineLog") {
        return zh ? "引擎日志" : "Engine Log";
    }
    if (name == "compareDock") {
        return zh ? "双引擎对比" : "Engine Compare";
    }
    if (name == "engineHeader") {
        return zh ? "引擎" : "Engine";
    }
    if (name == "mainEngineRow") {
        return zh ? "主引擎" : "Main";
    }
    if (name == "compareEngineRow") {
        return zh ? "对比引擎" : "Compare";
    }
    if (name == "moveHeader") {
        return zh ? "着法" : "Move";
    }
    if (name == "visitsHeader") {
        return zh ? "访问" : "Visits";
    }
    if (name == "winrateHeader") {
        return zh ? "胜率" : "Winrate";
    }
    if (name == "scoreHeader") {
        return zh ? "目差" : "Score";
    }
    if (name == "blackWinrateHeader") {
        return zh ? "黑胜率" : "Black Winrate";
    }
    if (name == "blackScoreHeader") {
        return zh ? "黑目差" : "Black Score";
    }
    if (name == "stdevHeader") {
        return zh ? "标准差" : "Stdev";
    }
    if (name == "policyHeader") {
        return zh ? "策略" : "Policy";
    }
    if (name == "pvHeader") {
        return zh ? "变化" : "PV";
    }
    if (name == "pvVisitsHeader") {
        return zh ? "变化访问" : "PV Visits";
    }
    if (name == "analysisError") {
        return zh ? "错误" : "Error";
    }
    return name;
}

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , analysisStore_(&game_)
    , gameEditor_(&game_)
{
    openSgfPathSelector_ = [](QWidget* parent, const QString& title, const QString& filter) {
        return QFileDialog::getOpenFileName(parent, title, QString(), filter);
    };
    saveSgfPathSelector_ = [](QWidget* parent, const QString& title, const QString& filter) {
        return QFileDialog::getSaveFileName(parent, title, QString(), filter);
    };
    javaConfigPathSelector_ = [](QWidget* parent, const QString& title, const QString& path, const QString& filter) {
        return QFileDialog::getOpenFileName(parent, title, path, filter);
    };
    firstRunPathSelector_ = [](QWidget* parent, const QString& title, const QString& path) {
        return QFileDialog::getOpenFileName(parent, title, path);
    };

    setWindowTitle("LizzieYzy Qt");
    resize(1280, 860);

    auto* quickWidget = new QQuickWidget(this);
    quickWidget->setObjectName("lizzieMainBoardQuickWidget");
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    boardItem_ = new BoardQuickItem;
    connect(boardItem_, &BoardQuickItem::pointClicked, this, &MainWindow::playMoveAt);
    quickWidget->setContent(QUrl(), nullptr, boardItem_);
    setCentralWidget(quickWidget);

    buildToolbar();
    buildDocks();
    buildMenus();
    connectEngine();
    loadSettings();
    applyAppearance();
    applyBoardDisplaySettings();
    retranslateUi();
    if (!restoreLastSession()) {
        updateBoard();
    }
    QTimer::singleShot(0, this, &MainWindow::maybeShowFirstRunWizard);
}

MainWindow::~MainWindow()
{
    engines_.stopAll();
}

void MainWindow::setSgfPathSelectors(SgfPathSelector openSelector, SgfPathSelector saveSelector)
{
    if (openSelector) {
        openSgfPathSelector_ = std::move(openSelector);
    }
    if (saveSelector) {
        saveSgfPathSelector_ = std::move(saveSelector);
    }
}

void MainWindow::setJavaConfigPathSelector(ConfigPathSelector selector)
{
    if (selector) {
        javaConfigPathSelector_ = std::move(selector);
    }
}

void MainWindow::setFirstRunPathSelector(FirstRunPathSelector selector)
{
    if (selector) {
        firstRunPathSelector_ = std::move(selector);
    }
}

void MainWindow::newGame()
{
    stopAutomatedPlayModes();
    cancelBatchAnalysisForGameChange();
    QDialog dialog(this);
    dialog.setWindowTitle(uiText("newGameTitle"));
    auto* layout = new QFormLayout(&dialog);

    auto* widthEdit = new QSpinBox(&dialog);
    widthEdit->setRange(1, 52);
    widthEdit->setToolTip(uiText("gtpWidthTooltip"));
    widthEdit->setValue(19);
    auto* heightEdit = new QSpinBox(&dialog);
    heightEdit->setRange(1, 52);
    heightEdit->setValue(19);
    auto* komiEdit = new QDoubleSpinBox(&dialog);
    komiEdit->setRange(-100.0, 100.0);
    komiEdit->setDecimals(1);
    komiEdit->setValue(7.5);
    auto* rulesEdit = new QComboBox(&dialog);
    populateRulesCombo(rulesEdit, engineSettings_.appearance.language);
    auto* handicapEdit = new QComboBox(&dialog);
    populateNewGameHandicapCombo(handicapEdit);
    updateNewGameHandicapCombo(handicapEdit, widthEdit->value(), heightEdit->value());
    auto* blackEdit = new QLineEdit(&dialog);
    auto* whiteEdit = new QLineEdit(&dialog);

    layout->addRow(uiText("width"), widthEdit);
    layout->addRow(uiText("height"), heightEdit);
    layout->addRow(uiText("komi"), komiEdit);
    layout->addRow(uiText("rules"), rulesEdit);
    layout->addRow(uiText("handicap"), handicapEdit);
    layout->addRow(uiText("black"), blackEdit);
    layout->addRow(uiText("white"), whiteEdit);

    const auto updateHandicapOptions = [widthEdit, heightEdit, handicapEdit](int) {
        updateNewGameHandicapCombo(handicapEdit, widthEdit->value(), heightEdit->value());
    };
    connect(widthEdit, qOverload<int>(&QSpinBox::valueChanged), &dialog, updateHandicapOptions);
    connect(heightEdit, qOverload<int>(&QSpinBox::valueChanged), &dialog, updateHandicapOptions);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    localizeDialogButtons(buttons, engineSettings_.appearance.language);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    cancelBatchAnalysisForGameChange();
    stopRealtimeAnalysis();
    stopCompareAnalysis();
    compareAnalysis_.reset();

    lizzie::core::GameInfo info;
    info.komi = komiEdit->value();
    info.rules = currentRulesComboValue(rulesEdit).toStdString();
    const int handicap = currentHandicapComboValue(handicapEdit);
    if (handicap > 1 && handicapEdit->isEnabled()) {
        info.handicap = handicap;
    }
    info.blackPlayer = blackEdit->text().toStdString();
    info.whitePlayer = whiteEdit->text().toStdString();
    game_ = lizzie::app::createNewGameModel(
        lizzie::core::BoardSize{widthEdit->value(), heightEdit->value()},
        std::move(info));
    documentSession_ = lizzie::app::GameDocumentSession{};
    appliedAnalysisSidecarForDocument_ = false;
    analysisSidecarSyncPending_ = false;
    updateBoard();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    stopAutomatedPlayModes();
    saveSettings();
    cancelBatchAnalysisForGameChange();
    stopRealtimeAnalysis();
    stopCompareAnalysis();
    QMainWindow::closeEvent(event);
}

bool MainWindow::loadSgfFromPath(
    const QString& path,
    std::optional<lizzie::core::NodeId> restoreNode,
    bool interactiveErrors,
    std::optional<int> restoreCollectionGameIndex)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString message = uiText("openSgfTitle") + ": " + file.errorString();
        if (interactiveErrors) {
            showWarning(this, engineSettings_.appearance.language, uiText("openSgfTitle"), file.errorString());
        } else if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message);
        }
        statusBar()->showMessage(message, 5000);
        return false;
    }

    try {
        lizzie::core::SgfParser parser;
        std::vector<lizzie::core::GameModel> collection =
            parser.parseCollection(QString::fromUtf8(file.readAll()).toStdString());
        if (collection.empty()) {
            throw lizzie::core::SgfError("SGF collection is empty");
        }
        std::size_t selectedGameIndex = 0;
        if (collection.size() > 1) {
            if (restoreCollectionGameIndex.has_value() &&
                *restoreCollectionGameIndex >= 0 &&
                *restoreCollectionGameIndex < static_cast<int>(collection.size())) {
                selectedGameIndex = static_cast<std::size_t>(*restoreCollectionGameIndex);
            } else if (restoreCollectionGameIndex.has_value() && !interactiveErrors) {
                if (consoleWidget_ != nullptr) {
                    consoleWidget_->appendSystemLine(
                        uiText("sgfCollectionStoredIndexOutOfRange")
                            .arg(*restoreCollectionGameIndex)
                            .arg(static_cast<int>(collection.size())));
                }
            } else if (interactiveErrors) {
                const std::optional<std::size_t> selected = selectSgfCollectionGame(
                    this,
                    collection,
                    engineSettings_.appearance.language,
                    uiText("selectSgfGameTitle"),
                    uiText("selectSgfGame"));
                if (!selected.has_value()) {
                    return false;
                }
                selectedGameIndex = *selected;
            } else if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(uiText("sgfCollectionLoadedFirst").arg(static_cast<int>(collection.size())));
            }
        }
        lizzie::core::GameModel loadedGame = collection[selectedGameIndex];
        cancelBatchAnalysisForGameChange();
        stopAutomatedPlayModes();
        game_ = std::move(loadedGame);
        if (!gameEditor_.setCurrent(game_.rootId())) {
            return false;
        }
        if (restoreNode.has_value() && game_.findNode(*restoreNode) != nullptr) {
            if (!gameEditor_.setCurrent(*restoreNode)) {
                return false;
            }
        }
        compareAnalysis_.reset();
        documentSession_.currentFile = path;
        documentSession_.collection = std::move(collection);
        documentSession_.collectionGameIndex = static_cast<int>(selectedGameIndex);
        documentSession_.collectionGameCount = static_cast<int>(documentSession_.collection.size());
        appliedAnalysisSidecarForDocument_ = false;
        analysisSidecarSyncPending_ = false;
        std::vector<std::string> legacyWarnings;
        int legacyImported = 0;
        if (engineSettings_.fileBehavior.importLegacySgfAnalysis) {
            legacyImported = lizzie::engine::importLegacyLizzieAnalysis(&game_, &legacyWarnings);
            for (const std::string& warning : legacyWarnings) {
                if (consoleWidget_ != nullptr) {
                    consoleWidget_->appendSystemLine(localizedAppWarning(warning, engineSettings_.appearance.language));
                }
            }
            if (legacyImported > 0 && consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(uiText("importedLegacySgfAnalysis").arg(legacyImported));
            }
        } else if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(uiText("legacySgfAnalysisImportDisabled"));
        }
        if (engineSettings_.fileBehavior.loadAnalysisSidecar) {
            appliedAnalysisSidecarForDocument_ = loadAnalysisSidecar(path) > 0;
            analysisSidecarSyncPending_ = false;
        } else if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(uiText("analysisSidecarLoadDisabled"));
        }
        updateBoard();
        if (realtimeAnalysisRequested()) {
            syncAndStartRealtimeAnalysis();
        }
        if (compareAnalysisRequested()) {
            syncAndStartCompareAnalysis();
        }
        QString openStatus = path;
        if (legacyImported > 0) {
            openStatus = uiText("openSgfLegacyAnalysisStatus").arg(path).arg(legacyImported);
        } else if (documentSession_.collection.size() > 1) {
            openStatus = uiText("openSgfCollectionGameStatus")
                             .arg(path)
                             .arg(static_cast<int>(selectedGameIndex + 1))
                             .arg(static_cast<int>(documentSession_.collection.size()));
        }
        statusBar()->showMessage(openStatus, 3000);
        return true;
    } catch (const std::exception& error) {
        const QString errorText = localizedSgfError(error.what(), engineSettings_.appearance.language);
        const QString message = uiText("openSgfTitle") + ": " + errorText;
        if (interactiveErrors) {
            showWarning(this, engineSettings_.appearance.language, uiText("openSgfTitle"), errorText);
        } else if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message);
        }
        statusBar()->showMessage(message, 5000);
        return false;
    }
}

bool MainWindow::restoreLastSession()
{
    QSettings settings;
    const lizzie::app::SessionSettings session = lizzie::app::loadSessionSettings(settings);
    const QString path = session.lastFile;
    if (path.isEmpty()) {
        return false;
    }
    if (!QFileInfo::exists(path)) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(uiText("lastSessionSgfMissingPrefix") + path);
        }
        lizzie::app::saveSessionSettings(settings, std::nullopt, game_.currentId());
        settings.sync();
        return false;
    }

    if (!loadSgfFromPath(path, session.currentNodeId, false, session.collectionGameIndex)) {
        lizzie::app::saveSessionSettings(settings, std::nullopt, game_.currentId());
        settings.sync();
        return false;
    }
    return true;
}

void MainWindow::openSgf()
{
    stopAutomatedPlayModes();
    cancelBatchAnalysisForGameChange();
    const QString path = openSgfPathSelector_(this, uiText("openSgfTitle"), uiText("sgfFileFilter"));
    if (path.isEmpty()) {
        return;
    }

    loadSgfFromPath(path);
}

void MainWindow::saveSgf()
{
    stopAutomatedPlayModes();
    if (!documentSession_.currentFile.has_value()) {
        saveSgfAsInteractive();
        return;
    }
    if (saveSgfTo(*documentSession_.currentFile)) {
        if (documentSession_.collection.empty()) {
            documentSession_.collectionGameIndex = 0;
            documentSession_.collectionGameCount = 1;
        }
    }
}

void MainWindow::saveSgfAs()
{
    stopAutomatedPlayModes();
    static_cast<void>(saveSgfAsInteractive());
}

bool MainWindow::saveSgfAsInteractive()
{
    cancelBatchAnalysisForGameChange();
    const QString path = saveSgfPathSelector_(this, uiText("saveSgfTitle"), uiText("sgfFileFilter"));
    if (path.isEmpty()) {
        return false;
    }
    if (saveSgfTo(path)) {
        documentSession_.currentFile = path;
        if (documentSession_.collection.empty()) {
            documentSession_.collectionGameIndex = 0;
            documentSession_.collectionGameCount = 1;
        }
        return true;
    }
    return false;
}

void MainWindow::previousMove()
{
    stopAutomatedPlayModes();
    if (gameEditor_.selectParent()) {
        updateBoard();
        if (realtimeAnalysisRequested()) {
            syncAndStartRealtimeAnalysis();
        }
        if (compareAnalysisRequested()) {
            syncAndStartCompareAnalysis();
        }
    }
}

void MainWindow::nextMove()
{
    stopAutomatedPlayModes();
    if (gameEditor_.selectFirstChild()) {
        updateBoard();
        if (realtimeAnalysisRequested()) {
            syncAndStartRealtimeAnalysis();
        }
        if (compareAnalysisRequested()) {
            syncAndStartCompareAnalysis();
        }
    }
}

void MainWindow::undoMove()
{
    const lizzie::core::NodeId current = game_.currentId();
    if (current == game_.rootId()) {
        statusBar()->showMessage(uiText("cannotUndoRootNode"), 3000);
        return;
    }

    stopAutomatedPlayModes();
    if (!gameEditor_.deleteCurrentSubtree()) {
        statusBar()->showMessage(uiText("cannotDeleteBranch"), 3000);
        return;
    }
    markAnalysisSidecarSyncPending();
    compareAnalysis_.reset();
    updateBoard();
    if (realtimeAnalysisRequested()) {
        syncAndStartRealtimeAnalysis();
    }
    if (compareAnalysisRequested()) {
        syncAndStartCompareAnalysis();
    }
}

void MainWindow::passMove()
{
    if (engineGameActive_) {
        stopEngineGame();
    }
    if (selfPlayActive_) {
        stopSelfPlay();
    }
    const bool interruptPendingHumanVsAiReply =
        humanVsAiActive_ && (pendingGenMoveStart_ || genMoveGuard_.hasPendingRequest());
    const bool previousMoveWasPass = currentNodeMoveWasPass(game_);
    const lizzie::app::MoveEditResult result = gameEditor_.tryPass();
    if (!result.accepted) {
        statusBar()->showMessage(localizedCoreError(result.error, engineSettings_.appearance.language), 5000);
        return;
    }
    cancelBatchAnalysisForGameChange();
    markAnalysisSidecarSyncPending();
    if (interruptPendingHumanVsAiReply) {
        stopHumanVsAi();
    }
    updateBoard();
    if (realtimeAnalysisRequested()) {
        syncAndStartRealtimeAnalysis();
    }
    if (compareAnalysisRequested()) {
        syncAndStartCompareAnalysis();
    }
    if (humanVsAiActive_) {
        const lizzie::app::HumanMoveAutomationPlan automationPlan = lizzie::app::planAfterHumanMove(
            lizzie::core::Move::pass(lizzie::core::Color::Black),
            lizzie::app::EngineAutomationState{
                .humanVsAiActive = humanVsAiActive_,
                .previousMoveWasPass = previousMoveWasPass,
            });
        if (automationPlan.stopHumanVsAi) {
            stopHumanVsAi();
        } else if (automationPlan.requestHumanVsAiReply) {
            requestAiMove();
        }
    }
}

void MainWindow::resignMove()
{
    if (engineGameActive_) {
        stopEngineGame();
    }
    if (selfPlayActive_) {
        stopSelfPlay();
    }
    std::string error;
    const std::optional<lizzie::core::Color> sideToMove = gameEditor_.currentSideToMove(&error);
    if (!sideToMove.has_value()) {
        statusBar()->showMessage(localizedCoreError(error, engineSettings_.appearance.language), 5000);
        return;
    }

    const QString color = *sideToMove == lizzie::core::Color::Black ? uiText("black") : uiText("white");
    stopHumanVsAi();
    cancelBatchAnalysisForGameChange();
    const QMessageBox::StandardButton choice = askYesNoQuestion(
        this,
        engineSettings_.appearance.language,
        uiText("resign"),
        uiText("resignQuestion").arg(color),
        QMessageBox::No);
    if (choice != QMessageBox::Yes) {
        return;
    }

    const lizzie::app::MoveEditResult result = gameEditor_.tryResign();
    if (!result.accepted) {
        statusBar()->showMessage(localizedCoreError(result.error, engineSettings_.appearance.language), 5000);
        return;
    }
    markAnalysisSidecarSyncPending();
    updateBoard();
    if (realtimeAnalysisRequested()) {
        stopRealtimeAnalysis();
    }
    if (compareAnalysisRequested()) {
        stopCompareAnalysis();
    }
}

void MainWindow::playMoveAt(int x, int y)
{
    const lizzie::core::Point point{x, y};
    if (boardEditMode_ != BoardEditMode::Play && boardEditMode_ != BoardEditMode::Try) {
        editMarkupAt(point);
        return;
    }

    if (engineGameActive_) {
        stopEngineGame();
    }
    if (selfPlayActive_) {
        stopSelfPlay();
    }
    const bool interruptPendingHumanVsAiReply =
        humanVsAiActive_ && (pendingGenMoveStart_ || genMoveGuard_.hasPendingRequest());
    const lizzie::app::MoveEditResult result = gameEditor_.tryPlayAt(point);
    if (!result.accepted) {
        statusBar()->showMessage(localizedCoreError(result.error, engineSettings_.appearance.language), 3000);
        return;
    }
    cancelBatchAnalysisForGameChange();
    markAnalysisSidecarSyncPending();
    if (interruptPendingHumanVsAiReply) {
        stopHumanVsAi();
    }
    updateBoard();
    if (realtimeAnalysisRequested()) {
        syncAndStartRealtimeAnalysis();
    }
    if (compareAnalysisRequested()) {
        syncAndStartCompareAnalysis();
    }
    if (humanVsAiActive_) {
        requestAiMove();
    }
}

void MainWindow::setBoardEditMode(BoardEditMode mode)
{
    if (mode != BoardEditMode::Play) {
        stopAutomatedPlayModes();
    }
    boardEditMode_ = mode;
    QString modeText = "Play";
    switch (mode) {
    case BoardEditMode::Play:
        modeText = uiText("playMode");
        break;
    case BoardEditMode::Try:
        modeText = uiText("tryMode");
        break;
    case BoardEditMode::Label:
        modeText = uiText("labelMode");
        break;
    case BoardEditMode::Circle:
        modeText = uiText("circleMark");
        break;
    case BoardEditMode::Square:
        modeText = uiText("squareMark");
        break;
    case BoardEditMode::Triangle:
        modeText = uiText("triangleMark");
        break;
    case BoardEditMode::Cross:
        modeText = uiText("crossMark");
        break;
    case BoardEditMode::Clear:
        modeText = uiText("clearMarkup");
        break;
    case BoardEditMode::SetupBlack:
        modeText = uiText("setupBlack");
        break;
    case BoardEditMode::SetupWhite:
        modeText = uiText("setupWhite");
        break;
    case BoardEditMode::SetupClear:
        modeText = uiText("setupClear");
        break;
    }
    statusBar()->showMessage(modeText, 1500);
}

void MainWindow::editMarkupAt(lizzie::core::Point point)
{
    if (boardEditMode_ == BoardEditMode::Clear) {
        if (gameEditor_.clearMarkupAt(game_.currentId(), point)) {
            updateBoard();
        }
        return;
    }

    if (boardEditMode_ == BoardEditMode::SetupBlack ||
        boardEditMode_ == BoardEditMode::SetupWhite ||
        boardEditMode_ == BoardEditMode::SetupClear) {
        lizzie::app::SetupStoneEdit edit = lizzie::app::SetupStoneEdit::Empty;
        if (boardEditMode_ == BoardEditMode::SetupBlack) {
            edit = lizzie::app::SetupStoneEdit::Black;
        } else if (boardEditMode_ == BoardEditMode::SetupWhite) {
            edit = lizzie::app::SetupStoneEdit::White;
        }

        if (gameEditor_.editSetupStone(game_.currentId(), point, edit)) {
            stopAutomatedPlayModes();
            cancelBatchAnalysisForGameChange();
            analysisStore_.clearFrom(game_.currentId());
            markAnalysisSidecarSyncPending();
            compareAnalysis_.reset();
            updateBoard();
            if (realtimeAnalysisRequested()) {
                syncAndStartRealtimeAnalysis();
            }
            if (compareAnalysisRequested()) {
                syncAndStartCompareAnalysis();
            }
        }
        return;
    }

    if (boardEditMode_ == BoardEditMode::Label) {
        const std::optional<std::string> existing = gameEditor_.labelAt(game_.currentId(), point);
        const QString currentText = existing.has_value() ? QString::fromStdString(*existing) : QString();
        bool accepted = false;
        const QString text = QInputDialog::getText(
            this,
            uiText("labelMode"),
            uiText("labelDialogPrompt"),
            QLineEdit::Normal,
            currentText,
            &accepted);
        if (!accepted) {
            return;
        }
        if (gameEditor_.setLabel(game_.currentId(), point, text.toStdString())) {
            updateBoard();
        }
        return;
    }

    lizzie::core::BoardMarkShape shape = lizzie::core::BoardMarkShape::Circle;
    switch (boardEditMode_) {
    case BoardEditMode::Circle:
        shape = lizzie::core::BoardMarkShape::Circle;
        break;
    case BoardEditMode::Square:
        shape = lizzie::core::BoardMarkShape::Square;
        break;
    case BoardEditMode::Triangle:
        shape = lizzie::core::BoardMarkShape::Triangle;
        break;
    case BoardEditMode::Cross:
        shape = lizzie::core::BoardMarkShape::Cross;
        break;
    case BoardEditMode::Play:
    case BoardEditMode::Try:
    case BoardEditMode::Label:
    case BoardEditMode::Clear:
    case BoardEditMode::SetupBlack:
    case BoardEditMode::SetupWhite:
    case BoardEditMode::SetupClear:
        return;
    }

    if (gameEditor_.toggleMark(game_.currentId(), point, shape)) {
        updateBoard();
    }
}

void MainWindow::promoteCurrentVariation()
{
    if (!gameEditor_.promoteCurrentVariation()) {
        statusBar()->showMessage(uiText("cannotPromoteNode"), 3000);
        return;
    }
    stopAutomatedPlayModes();
    cancelBatchAnalysisForGameChange();
    markAnalysisSidecarSyncPending();
    updateBoard();
    if (realtimeAnalysisRequested()) {
        syncAndStartRealtimeAnalysis();
    }
    if (compareAnalysisRequested()) {
        syncAndStartCompareAnalysis();
    }
    statusBar()->showMessage(uiText("promoteVariation"), 3000);
}

void MainWindow::deleteCurrentBranch()
{
    const lizzie::core::NodeId current = game_.currentId();
    if (current == game_.rootId()) {
        statusBar()->showMessage(uiText("cannotDeleteRootNode"), 3000);
        return;
    }

    stopAutomatedPlayModes();
    cancelBatchAnalysisForGameChange();
    const QMessageBox::StandardButton choice = askYesNoQuestion(
        this,
        engineSettings_.appearance.language,
        uiText("deleteBranch"),
        uiText("deleteBranchQuestion"),
        QMessageBox::No);
    if (choice != QMessageBox::Yes) {
        return;
    }

    if (!gameEditor_.deleteCurrentSubtree()) {
        statusBar()->showMessage(uiText("cannotDeleteBranch"), 3000);
        return;
    }
    markAnalysisSidecarSyncPending();
    compareAnalysis_.reset();
    updateBoard();
    if (realtimeAnalysisRequested()) {
        syncAndStartRealtimeAnalysis();
    }
    if (compareAnalysisRequested()) {
        syncAndStartCompareAnalysis();
    }
    statusBar()->showMessage(uiText("deleteBranch"), 3000);
}

void MainWindow::showSettings()
{
    editSettings();
}

void MainWindow::editGameInfo()
{
    QDialog dialog(this);
    dialog.setWindowTitle(uiText("gameInfo"));
    auto* layout = new QFormLayout(&dialog);
    const lizzie::core::GameInfo& info = game_.gameInfo();

    auto* blackEdit = new QLineEdit(QString::fromStdString(info.blackPlayer), &dialog);
    auto* whiteEdit = new QLineEdit(QString::fromStdString(info.whitePlayer), &dialog);
    auto* blackRankEdit = new QLineEdit(QString::fromStdString(info.blackRank), &dialog);
    auto* whiteRankEdit = new QLineEdit(QString::fromStdString(info.whiteRank), &dialog);
    auto* resultEdit = new QLineEdit(QString::fromStdString(info.result), &dialog);
    auto* dateEdit = new QLineEdit(QString::fromStdString(info.date), &dialog);
    auto* sourceEdit = new QLineEdit(QString::fromStdString(info.source), &dialog);
    auto* rulesEdit = new QComboBox(&dialog);
    rulesEdit->setEditable(true);
    setRulesComboValue(
        rulesEdit,
        engineSettings_.appearance.language,
        QString::fromStdString(info.rules.empty() ? std::string("Chinese") : info.rules));
    auto* rulesEnabled = new QCheckBox(&dialog);
    rulesEnabled->setChecked(!info.rules.empty());
    auto* rulesField = new QWidget(&dialog);
    auto* rulesLayout = new QHBoxLayout(rulesField);
    rulesLayout->setContentsMargins(0, 0, 0, 0);
    rulesLayout->addWidget(rulesEnabled);
    rulesLayout->addWidget(rulesEdit, 1);
    rulesEdit->setEnabled(rulesEnabled->isChecked());
    connect(rulesEnabled, &QCheckBox::toggled, rulesEdit, &QComboBox::setEnabled);
    connect(rulesEdit, &QComboBox::currentTextChanged, &dialog, [rulesEnabled] {
        if (!rulesEnabled->isChecked()) {
            rulesEnabled->setChecked(true);
        }
    });
    auto* komiEdit = new QDoubleSpinBox(&dialog);
    komiEdit->setRange(-100.0, 100.0);
    komiEdit->setDecimals(1);
    komiEdit->setValue(info.komi.value_or(7.5));
    auto* komiEnabled = new QCheckBox(&dialog);
    komiEnabled->setChecked(info.komi.has_value());
    auto* komiField = new QWidget(&dialog);
    auto* komiLayout = new QHBoxLayout(komiField);
    komiLayout->setContentsMargins(0, 0, 0, 0);
    komiLayout->addWidget(komiEnabled);
    komiLayout->addWidget(komiEdit, 1);
    komiEdit->setEnabled(komiEnabled->isChecked());
    connect(komiEnabled, &QCheckBox::toggled, komiEdit, &QDoubleSpinBox::setEnabled);
    connect(komiEdit, qOverload<double>(&QDoubleSpinBox::valueChanged), &dialog, [komiEnabled](double) {
        if (!komiEnabled->isChecked()) {
            komiEnabled->setChecked(true);
        }
    });
    auto* handicapEdit = new QSpinBox(&dialog);
    handicapEdit->setRange(0, 99);
    handicapEdit->setValue(info.handicap.value_or(0));

    layout->addRow(uiText("black"), blackEdit);
    layout->addRow(uiText("white"), whiteEdit);
    layout->addRow(uiText("blackRank"), blackRankEdit);
    layout->addRow(uiText("whiteRank"), whiteRankEdit);
    layout->addRow(uiText("result"), resultEdit);
    layout->addRow(uiText("date"), dateEdit);
    layout->addRow(uiText("source"), sourceEdit);
    layout->addRow(uiText("rules"), rulesField);
    layout->addRow(uiText("komi"), komiField);
    layout->addRow(uiText("handicap"), handicapEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    localizeDialogButtons(buttons, engineSettings_.appearance.language);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    stopAutomatedPlayModes();
    cancelBatchAnalysisForGameChange();

    if (dialog.exec() != QDialog::Accepted) {
        return;
    }

    lizzie::core::GameInfo editable = info;
    editable.blackPlayer = blackEdit->text().toStdString();
    editable.whitePlayer = whiteEdit->text().toStdString();
    editable.blackRank = blackRankEdit->text().toStdString();
    editable.whiteRank = whiteRankEdit->text().toStdString();
    editable.result = resultEdit->text().toStdString();
    editable.date = dateEdit->text().toStdString();
    editable.source = sourceEdit->text().toStdString();
    if (rulesEnabled->isChecked()) {
        editable.rules = currentRulesComboValue(rulesEdit).toStdString();
    } else {
        editable.rules.clear();
    }
    if (komiEnabled->isChecked()) {
        editable.komi = komiEdit->value();
    } else {
        editable.komi.reset();
    }
    if (handicapEdit->value() > 0) {
        editable.handicap = handicapEdit->value();
    } else {
        editable.handicap.reset();
    }
    const lizzie::app::GameInfoEditResult editResult = gameEditor_.setGameInfo(std::move(editable));
    if (editResult.analysisInvalidated) {
        analysisStore_.clearAll();
        markAnalysisSidecarSyncPending();
        compareAnalysis_.reset();
    }

    updateBoard();
    if (editResult.analysisInvalidated && realtimeAnalysisRequested()) {
        syncAndStartRealtimeAnalysis();
    }
    if (editResult.analysisInvalidated && compareAnalysisRequested()) {
        syncAndStartCompareAnalysis();
    }
    statusBar()->showMessage(uiText("gameInfo"), 3000);
}

bool MainWindow::editSettings()
{
    cancelBatchAnalysisForGameChange();
    EngineSettingsDialog dialog(this);
    dialog.setSettings(engineSettings_);
    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }
    applyEngineSettingsUpdate(dialog.settings());
    return true;
}

void MainWindow::applyEngineSettingsUpdate(lizzie::app::EngineUiSettings nextSettings)
{
    const SettingsRuntimePlan runtimePlan = planSettingsRuntimeUpdate(
        engineSettings_,
        nextSettings,
        SettingsRuntimeState{
            realtimeAnalysisRequested(),
            compareAnalysisRequested(),
            engines_.mainEngine().isRunning(),
            engines_.compareEngine().isRunning(),
        });

    engineSettings_ = std::move(nextSettings);
    saveSettings();
    applyAppearance();
    applyBoardDisplaySettings();
    retranslateUi();
    updateBoard();

    if (runtimePlan.restartMainEngine && engines_.mainEngine().isRunning()) {
        mainCapabilitiesKnown_ = false;
        engines_.resetMainCapabilities();
        engines_.mainEngine().stop();
    }
    if (runtimePlan.disableRealtime) {
        stopRealtimeAnalysis();
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(uiText("realtimeStoppedSettingsIncomplete"));
        }
    } else if (runtimePlan.startMainEngine) {
        realtimeAnalysisActive_ = false;
        pendingAnalysisStart_ = true;
        if (analyzeAction_ != nullptr) {
            analyzeAction_->setChecked(true);
        }
        if (engines_.mainEngine().isRunning()) {
            syncAndStartRealtimeAnalysis();
        } else {
            mainCapabilitiesKnown_ = false;
            engines_.resetMainCapabilities();
            engines_.mainEngine().startGtp(engineSettings_.config);
        }
    } else if (runtimePlan.resyncRealtime) {
        syncAndStartRealtimeAnalysis();
    }

    if (runtimePlan.restartCompareEngine && engines_.compareEngine().isRunning()) {
        compareCapabilitiesKnown_ = false;
        engines_.resetCompareCapabilities();
        engines_.compareEngine().stop();
    }
    if (runtimePlan.disableCompare) {
        stopCompareAnalysis();
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(uiText("compareStoppedSettingsIncomplete"));
        }
    } else if (runtimePlan.startCompareEngine) {
        compareAnalysisActive_ = false;
        pendingCompareStart_ = true;
        compareAnalysis_.reset();
        if (compareAction_ != nullptr) {
            compareAction_->setChecked(true);
        }
        if (engines_.compareEngine().isRunning()) {
            syncAndStartCompareAnalysis();
        } else {
            compareCapabilitiesKnown_ = false;
            engines_.resetCompareCapabilities();
            engines_.compareEngine().startGtp(resolvedComparisonGtpConfig(engineSettings_));
        }
    } else if (runtimePlan.resyncCompare) {
        syncAndStartCompareAnalysis();
    }
}

bool MainWindow::editFirstRunEngineSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle(uiText("configureEngine"));
    auto* layout = new QFormLayout(&dialog);

    auto* executableEdit = new QLineEdit(QString::fromStdString(engineSettings_.config.executable.string()), &dialog);
    auto* modelEdit = new QLineEdit(QString::fromStdString(engineSettings_.config.model.string()), &dialog);
    auto* gtpConfigEdit = new QLineEdit(QString::fromStdString(engineSettings_.config.gtpConfig.string()), &dialog);
    auto* analysisConfigEdit =
        new QLineEdit(QString::fromStdString(engineSettings_.config.analysisConfig.string()), &dialog);

    const auto addPathRow = [this, &dialog, layout](const QString& label, const QString& key, QLineEdit* edit) {
        auto* row = new QWidget(&dialog);
        auto* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(0, 0, 0, 0);
        auto* browseButton = new QPushButton("...", row);
        browseButton->setObjectName("lizzieFirstRunPathBrowseButton");
        browseButton->setProperty("lizzieFirstRunPathKey", key);
        browseButton->setToolTip(uiText("selectFile"));
        browseButton->setAccessibleName(uiText("selectFile"));
        browseButton->setFixedWidth(34);
        rowLayout->addWidget(edit);
        rowLayout->addWidget(browseButton);
        layout->addRow(label, row);
        connect(browseButton, &QPushButton::clicked, &dialog, [this, &dialog, edit] {
            const QString path = firstRunPathSelector_(&dialog, uiText("selectFile"), edit->text());
            if (!path.isEmpty()) {
                edit->setText(path);
            }
        });
    };
    addPathRow(uiText("katago"), QStringLiteral("katago"), executableEdit);
    addPathRow(uiText("model"), QStringLiteral("model"), modelEdit);
    addPathRow(uiText("gtpConfig"), QStringLiteral("gtpConfig"), gtpConfigEdit);
    addPathRow(uiText("analysisConfig"), QStringLiteral("analysisConfig"), analysisConfigEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    localizeDialogButtons(buttons, engineSettings_.appearance.language);
    layout->addRow(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) {
        return false;
    }

    lizzie::engine::KataGoConfig nextConfig = engineSettings_.config;
    nextConfig.executable = executableEdit->text().trimmed().toStdString();
    nextConfig.model = modelEdit->text().trimmed().toStdString();
    nextConfig.gtpConfig = gtpConfigEdit->text().trimmed().toStdString();
    nextConfig.analysisConfig = analysisConfigEdit->text().trimmed().toStdString();
    if (!nextConfig.hasGtpMinimum() || !nextConfig.hasAnalysisMinimum()) {
        showWarning(this, engineSettings_.appearance.language, uiText("configureEngine"), uiText("incompleteEngineSettings"));
        return false;
    }

    engineSettings_.config = std::move(nextConfig);
    saveSettings();
    return true;
}

bool MainWindow::importLegacyJavaConfigFromDialog()
{
    stopAutomatedPlayModes();
    cancelBatchAnalysisForGameChange();
    const QString defaultPath = QDir::current().absoluteFilePath("config.txt");
    const QString path = javaConfigPathSelector_(
        this,
        uiText("importJavaConfig"),
        QFileInfo::exists(defaultPath) ? defaultPath : QString(),
        uiText("importJavaConfigFilter"));
    if (path.isEmpty()) {
        return false;
    }
    if (!importLegacyJavaConfig(path)) {
        return false;
    }

    return true;
}

bool MainWindow::importLegacyJavaConfig(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        showWarning(this, engineSettings_.appearance.language, uiText("importJavaConfig"), file.errorString());
        return false;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        showWarning(
            this,
            engineSettings_.appearance.language,
            uiText("importJavaConfig"),
            uiText("invalidJavaConfig") + ": " + error.errorString());
        return false;
    }

    LegacyConfigImportResult imported =
        importLegacyConfigObject(document.object(), QFileInfo(path).absoluteDir(), engineSettings_);

    if (!imported.hasUsableKataGo) {
        showWarning(this, engineSettings_.appearance.language, uiText("importJavaConfig"), uiText("noUsableKataGo"));
        return false;
    }

    applyEngineSettingsUpdate(std::move(imported.settings));

    for (const QString& note : imported.notes) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(
                localizedLegacyImportNote(note, engineSettings_.appearance.language));
        }
    }
    QString message = uiText("importedJavaConfigSummary").arg(imported.importedEngines);
    if (engineSettings_.appearance.language == lizzie::app::LanguageMode::English && imported.importedEngines != 1) {
        message += "s";
    }
    if (consoleWidget_ != nullptr) {
        consoleWidget_->appendSystemLine(message);
    }
    statusBar()->showMessage(message, 5000);
    return true;
}

void MainWindow::maybeShowFirstRunWizard()
{
    QSettings settings;
    const lizzie::app::FirstRunStartupAction startupAction =
        lizzie::app::planFirstRunStartup(lizzie::app::firstRunComplete(settings), engineSettings_);
    if (startupAction == lizzie::app::FirstRunStartupAction::Skip) {
        return;
    }
    if (startupAction == lizzie::app::FirstRunStartupAction::MarkComplete) {
        lizzie::app::setFirstRunComplete(settings, true);
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle(uiText("firstRunTitle"));
    auto* layout = new QVBoxLayout(&dialog);
    auto* label = new QLabel(uiText("firstRunMessage"), &dialog);
    label->setWordWrap(true);
    layout->addWidget(label);

    auto* buttons = new QDialogButtonBox(&dialog);
    auto* configureButton = buttons->addButton(uiText("configureEngine"), QDialogButtonBox::AcceptRole);
    auto* importButton = buttons->addButton(uiText("importJavaConfig"), QDialogButtonBox::ActionRole);
    auto* noEngineButton = buttons->addButton(uiText("noEngineMode"), QDialogButtonBox::RejectRole);
    layout->addWidget(buttons);
    int choice = 0;
    connect(configureButton, &QPushButton::clicked, &dialog, [&] {
        choice = 1;
        dialog.accept();
    });
    connect(importButton, &QPushButton::clicked, &dialog, [&] {
        choice = 3;
        dialog.accept();
    });
    connect(noEngineButton, &QPushButton::clicked, &dialog, [&] {
        choice = 2;
        dialog.reject();
    });

    const int result = dialog.exec();
    if (result == QDialog::Accepted && choice == 1) {
        if (editFirstRunEngineSettings()) {
            lizzie::app::setFirstRunComplete(settings, true);
        }
        return;
    }
    if (result == QDialog::Accepted && choice == 3) {
        if (importLegacyJavaConfigFromDialog()) {
            if (engineSettings_.config.hasGtpMinimum() && engineSettings_.config.hasAnalysisMinimum()) {
                lizzie::app::setFirstRunComplete(settings, true);
            } else {
                showWarning(this, engineSettings_.appearance.language, uiText("importJavaConfig"), uiText("incompleteEngineSettings"));
            }
        }
        return;
    }
    if (choice == 2) {
        lizzie::app::setFirstRunComplete(settings, true);
        statusBar()->showMessage(uiText("noEngineModeEnabled"), 3000);
    }
}

void MainWindow::startOrStopAnalysis()
{
    if (realtimeAnalysisRequested()) {
        stopRealtimeAnalysis();
        return;
    }

    stopAutomatedPlayModes();
    if (!engineSettings_.config.hasGtpMinimum()) {
        showSettings();
    }
    if (!engineSettings_.config.hasGtpMinimum()) {
        reportSystemDiagnostic(uiText("engineSettingsIncomplete"), QStringLiteral("main"));
        analyzeAction_->setChecked(false);
        return;
    }

    pendingAnalysisStart_ = true;
    analyzeAction_->setChecked(true);
    if (engines_.mainEngine().isRunning() && mainCapabilitiesKnown_) {
        if (!engines_.mainCapabilities().kataAnalyze) {
            showWarning(this, engineSettings_.appearance.language, uiText("analyze"), uiText("noAnalyzeSupport"));
            stopRealtimeAnalysis();
            return;
        }
        syncAndStartRealtimeAnalysis();
    } else if (engines_.mainEngine().isRunning()) {
        statusBar()->showMessage(uiText("restartingEngine"), 1500);
    } else {
        mainCapabilitiesKnown_ = false;
        engines_.resetMainCapabilities();
        engines_.mainEngine().startGtp(engineSettings_.config);
    }
}

void MainWindow::startEstimate()
{
    if (engines_.batchAnalysis().isRunning()) {
        return;
    }
    batchTracker_.clear();
    stopAutomatedPlayModes();
    if (!engineSettings_.config.hasAnalysisMinimum()) {
        showSettings();
    }
    if (!engineSettings_.config.hasAnalysisMinimum()) {
        reportSystemDiagnostic(uiText("estimateSettingsIncomplete"), QStringLiteral("analysis"));
        return;
    }

    const lizzie::app::BatchAnalysisRunPlan plan =
        lizzie::app::buildEstimateRunPlan(game_, engineSettings_.realtimeOptions);
    for (const std::string& warning : plan.warnings) {
        const QString warningText = localizedAppWarning(warning, engineSettings_.appearance.language);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(warningText);
        }
        appendEngineLogLine(warningText, QStringLiteral("analysis"));
    }
    if (!plan.hasRequests()) {
        statusBar()->showMessage(uiText("noCurrentNodeToEstimate"), 3000);
        return;
    }

    analysisRunMode_ = AnalysisRunMode::Estimate;
    batchRunFailed_ = false;
    for (lizzie::core::NodeId nodeId : plan.nodesToClear) {
        analysisStore_.clearError(nodeId);
    }
    batchTracker_.reset(plan.positionKeys);
    batchProgress_ = new QProgressDialog(uiText("estimate"), uiText("cancel"), 0, 1, this);
    batchProgress_->setWindowModality(Qt::WindowModal);
    batchProgress_->setMinimumDuration(0);
    connect(batchProgress_, &QProgressDialog::canceled, &engines_.batchAnalysis(), &lizzie::engine::ThreadedAnalysisProcess::cancel);
    if (estimateAction_ != nullptr) {
        estimateAction_->setEnabled(false);
    }
    if (batchAction_ != nullptr) {
        batchAction_->setEnabled(false);
    }
    engines_.batchAnalysis().startAnalysis(engineSettings_.config, plan.requests);
}

void MainWindow::startBatchAnalysis()
{
    if (engines_.batchAnalysis().isRunning()) {
        return;
    }
    batchTracker_.clear();
    stopAutomatedPlayModes();
    if (!engineSettings_.config.hasAnalysisMinimum()) {
        showSettings();
    }
    if (!engineSettings_.config.hasAnalysisMinimum()) {
        reportSystemDiagnostic(uiText("batchSettingsIncomplete"), QStringLiteral("analysis"));
        return;
    }

    const lizzie::app::BatchAnalysisRunPlan plan =
        lizzie::app::buildBatchRunPlan(game_, engineSettings_.realtimeOptions);
    for (const std::string& warning : plan.warnings) {
        const QString warningText = localizedAppWarning(warning, engineSettings_.appearance.language);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(warningText);
        }
        appendEngineLogLine(warningText, QStringLiteral("analysis"));
    }

    if (!plan.hasRequests()) {
        statusBar()->showMessage(uiText("noNodesToAnalyze"), 3000);
        return;
    }
    if (!documentSession_.currentFile.has_value() && !saveSgfAsInteractive()) {
        return;
    }

    for (lizzie::core::NodeId nodeId : plan.nodesToClear) {
        analysisStore_.clearError(nodeId);
    }
    batchTracker_.reset(plan.positionKeys);
    analysisRunMode_ = AnalysisRunMode::Batch;
    batchRunFailed_ = false;
    batchProgress_ = new QProgressDialog(
        uiText("batchAnalysis"),
        uiText("cancel"),
        0,
        static_cast<int>(plan.requests.size()),
        this);
    batchProgress_->setWindowModality(Qt::WindowModal);
    batchProgress_->setMinimumDuration(0);
    connect(batchProgress_, &QProgressDialog::canceled, &engines_.batchAnalysis(), &lizzie::engine::ThreadedAnalysisProcess::cancel);
    if (batchAction_ != nullptr) {
        batchAction_->setEnabled(false);
    }
    if (estimateAction_ != nullptr) {
        estimateAction_->setEnabled(false);
    }
    engines_.batchAnalysis().startAnalysis(engineSettings_.config, plan.requests);
}

void MainWindow::restartEngine()
{
    if (!engineSettings_.config.hasGtpMinimum()) {
        showSettings();
    }
    if (!engineSettings_.config.hasGtpMinimum()) {
        reportSystemDiagnostic(uiText("engineSettingsIncomplete"), QStringLiteral("main"));
        stopHumanVsAi();
        stopSelfPlay();
        return;
    }

    const bool resumeRealtime = realtimeAnalysisRequested();
    stopEngineGame();
    stopHumanVsAi();
    stopSelfPlay();
    realtimeAnalysisActive_ = false;
    pendingAnalysisStart_ = false;
    pendingRealtimeAnalyzeCommand_.reset();
    mainCapabilitiesKnown_ = false;
    engines_.resetMainCapabilities();
    engines_.mainEngine().stop();

    pendingAnalysisStart_ = resumeRealtime;
    engines_.mainEngine().startGtp(engineSettings_.config);
    statusBar()->showMessage(uiText("restartingEngine"), 3000);
}

void MainWindow::startOrStopCompare()
{
    if (engineGameActive_) {
        stopEngineGame();
    }
    if (compareAnalysisRequested()) {
        stopCompareAnalysis();
        return;
    }

    stopAutomatedPlayModes();
    lizzie::engine::KataGoConfig config = resolvedComparisonGtpConfig(engineSettings_);
    if (!config.hasGtpMinimum()) {
        showSettings();
        config = resolvedComparisonGtpConfig(engineSettings_);
    }
    if (!config.hasGtpMinimum()) {
        reportSystemDiagnostic(
            uiText("compareEngineSettingsIncomplete"),
            QStringLiteral("compare"),
            compareConsolePrefix(engineSettings_.appearance.language));
        if (compareAction_ != nullptr) {
            compareAction_->setChecked(false);
        }
        return;
    }

    pendingCompareStart_ = true;
    compareAnalysis_.reset();
    updateCompareTable();
    if (compareAction_ != nullptr) {
        compareAction_->setChecked(true);
    }
    if (engines_.compareEngine().isRunning()) {
        syncAndStartCompareAnalysis();
    } else {
        compareCapabilitiesKnown_ = false;
        engines_.resetCompareCapabilities();
        engines_.compareEngine().startGtp(config);
    }
}

void MainWindow::requestAiMove()
{
    if (pendingGenMoveStart_) {
        return;
    }
    if (engineGameActive_) {
        stopEngineGame();
    }
    if (!engineSettings_.config.hasGtpMinimum()) {
        showSettings();
    }
    if (!engineSettings_.config.hasGtpMinimum()) {
        reportSystemDiagnostic(uiText("engineSettingsIncomplete"), QStringLiteral("main"));
        stopHumanVsAi();
        stopSelfPlay();
        return;
    }

    stopRealtimeAnalysis();
    pendingGenMoveStart_ = true;
    if (engines_.mainEngine().isRunning()) {
        syncAndRequestGenMove();
    } else {
        mainCapabilitiesKnown_ = false;
        engines_.resetMainCapabilities();
        engines_.mainEngine().startGtp(engineSettings_.config);
    }
}

void MainWindow::toggleHumanVsAi()
{
    const AutomatedPlayTogglePlan plan = planToggleHumanVsAi(
        EngineAutomationState{
            .selfPlayActive = selfPlayActive_,
            .engineGameActive = engineGameActive_,
            .humanVsAiActive = humanVsAiActive_,
        });
    if (plan.stopHumanVsAi) {
        stopHumanVsAi();
        return;
    }

    if (!engineSettings_.config.hasGtpMinimum()) {
        showSettings();
    }
    if (!engineSettings_.config.hasGtpMinimum()) {
        reportSystemDiagnostic(uiText("humanVsAiRequiresMainGtp"), QStringLiteral("main"));
        if (humanVsAiAction_ != nullptr) {
            humanVsAiAction_->setChecked(false);
        }
        return;
    }

    if (plan.stopEngineGame) {
        stopEngineGame();
    }
    if (plan.stopSelfPlay) {
        stopSelfPlay();
    }

    if (plan.startHumanVsAi) {
        humanVsAiActive_ = true;
        if (humanVsAiAction_ != nullptr) {
            humanVsAiAction_->setChecked(true);
        }
        statusBar()->showMessage(uiText("humanVsAiStarted"), 3000);
    }
}

void MainWindow::toggleSelfPlay()
{
    const AutomatedPlayTogglePlan plan = planToggleSelfPlay(
        EngineAutomationState{
            .selfPlayActive = selfPlayActive_,
            .engineGameActive = engineGameActive_,
            .humanVsAiActive = humanVsAiActive_,
        });
    if (plan.stopSelfPlay) {
        stopSelfPlay();
        return;
    }

    if (plan.stopEngineGame) {
        stopEngineGame();
    }
    if (plan.stopHumanVsAi) {
        stopHumanVsAi();
    }
    if (plan.startSelfPlay) {
        selfPlayActive_ = true;
        if (selfPlayAction_ != nullptr) {
            selfPlayAction_->setChecked(true);
        }
    }
    if (plan.requestSelfPlayMove) {
        requestAiMove();
    }
}

void MainWindow::toggleEngineGame()
{
    const AutomatedPlayTogglePlan plan = planToggleEngineGame(
        EngineAutomationState{
            .selfPlayActive = selfPlayActive_,
            .engineGameActive = engineGameActive_,
            .humanVsAiActive = humanVsAiActive_,
        });
    if (plan.stopEngineGame) {
        stopEngineGame();
        return;
    }

    lizzie::engine::KataGoConfig comparisonConfig = resolvedComparisonGtpConfig(engineSettings_);
    if (!engineSettings_.config.hasGtpMinimum() || !comparisonConfig.hasGtpMinimum()) {
        showSettings();
        comparisonConfig = resolvedComparisonGtpConfig(engineSettings_);
    }
    if (!engineSettings_.config.hasGtpMinimum() || !comparisonConfig.hasGtpMinimum()) {
        reportSystemDiagnostic(uiText("engineGameRequiresGtp"), QStringLiteral("main"));
        if (engineGameAction_ != nullptr) {
            engineGameAction_->setChecked(false);
        }
        return;
    }

    if (plan.stopRealtimeAnalysis) {
        stopRealtimeAnalysis();
    }
    if (plan.stopCompareAnalysis) {
        stopCompareAnalysis();
    }
    if (plan.stopHumanVsAi) {
        stopHumanVsAi();
    }
    if (plan.stopSelfPlay) {
        stopSelfPlay();
    }
    if (plan.clearCompareMoveRequest) {
        pendingCompareGenMoveStart_ = false;
        compareGenMoveGuard_.clear();
    }

    if (plan.startEngineGame) {
        engineGameActive_ = true;
        if (engineGameAction_ != nullptr) {
            engineGameAction_->setChecked(true);
        }
        statusBar()->showMessage(uiText("engineGameStarted"), 3000);
    }
    if (plan.requestEngineGameMove) {
        requestEngineGameMove();
    }
}

void MainWindow::handleCommandResponse(const lizzie::engine::QueuedGtpResponse& response)
{
    if (!response.command.has_value()) {
        return;
    }
    const AnalysisStreamGateEvent gateEvent =
        realtimeStreamGate_.handleSyncResponse(response.command->id, response.response.success);
    if (gateEvent == AnalysisStreamGateEvent::Failed) {
        const QString message =
            uiText("realtimeSyncFailedPrefix") +
            uiText("gtpCommandFailedPrefix").arg(QString::fromStdString(response.command->name)) +
            QString::fromStdString(response.response.payload);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message);
        }
        appendEngineLogLine(message, QStringLiteral("main"));
        stopRealtimeAnalysis();
        statusBar()->showMessage(uiText("realtimeSyncSkipped"), 5000);
        return;
    }
    if (gateEvent == AnalysisStreamGateEvent::Waiting) {
        return;
    }
    if (gateEvent == AnalysisStreamGateEvent::Activated) {
        if (pendingRealtimeAnalyzeCommand_.has_value()) {
            if (!lizzie::app::dispatchGtpCommand(engines_.mainEngine(), *pendingRealtimeAnalyzeCommand_)) {
                failRealtimeAnalysisStartup(
                    uiText("realtimeSyncFailedPrefix") + uiText("analyzeCommandDispatchFailed"));
                return;
            }
        }
        pendingRealtimeAnalyzeCommand_.reset();
        pendingAnalysisStart_ = false;
        realtimeAnalysisActive_ = true;
        if (analyzeAction_ != nullptr) {
            analyzeAction_->setChecked(true);
        }
        updateBoard();
        return;
    }
    if (response.command->name == "genmove") {
        pendingGenMoveStart_ = false;
        if (!response.response.success) {
            genMoveGuard_.clear();
            if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(QString::fromStdString(response.response.payload));
            }
            if (engineGameActive_) {
                stopEngineGame();
            }
            stopHumanVsAi();
            stopSelfPlay();
            return;
        }

        std::string error;
        const lizzie::core::BoardPosition position = game_.currentPosition(&error);
        if (!error.empty()) {
            genMoveGuard_.clear();
            statusBar()->showMessage(localizedCoreError(error, engineSettings_.appearance.language), 5000);
            if (engineGameActive_) {
                stopEngineGame();
            }
            stopHumanVsAi();
            stopSelfPlay();
            return;
        }
        const std::string currentPositionKey = lizzie::engine::analysisPositionKey(game_, game_.currentId());
        if (!genMoveGuard_.consumeIfMatches(currentPositionKey)) {
            if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(uiText("ignoredStaleEngineMoveResponse"));
            }
            if (engineGameActive_) {
                stopEngineGame();
            }
            stopHumanVsAi();
            stopSelfPlay();
            statusBar()->showMessage(uiText("ignoredStaleEngineMove"), 3000);
            return;
        }
        const std::optional<lizzie::core::Move> move =
            lizzie::engine::moveFromGtpToken(response.response.payload, game_.boardSize(), position.sideToMove());
        if (!move.has_value()) {
            const QString message =
                uiText("couldNotParseEngineMove") + ": " + QString::fromStdString(response.response.payload);
            if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(message);
            }
            appendEngineLogLine(message, QStringLiteral("main"));
            statusBar()->showMessage(uiText("couldNotParseEngineMove"), 5000);
            if (engineGameActive_) {
                stopEngineGame();
            }
            stopHumanVsAi();
            stopSelfPlay();
            return;
        }
        const bool previousMoveWasPass = currentNodeMoveWasPass(game_);
        if (!applyEngineMove(*move, QStringLiteral("main"), {})) {
            stopHumanVsAi();
            return;
        }
        const EngineAutomationPlan automationPlan = planAfterEngineMove(
            *move,
            EngineAutomationState{
                .selfPlayActive = selfPlayActive_,
                .engineGameActive = engineGameActive_,
                .humanVsAiActive = humanVsAiActive_,
                .previousMoveWasPass = previousMoveWasPass,
            });
        if (automationPlan.stopHumanVsAi) {
            stopHumanVsAi();
        }
        if (automationPlan.stopEngineGame) {
            stopEngineGame();
        }
        if (automationPlan.stopSelfPlay) {
            stopSelfPlay();
        }
        if (automationPlan.requestEngineGameMove) {
            requestEngineGameMove();
        } else if (automationPlan.requestSelfPlayMove) {
            requestAiMove();
        }
        return;
    }

    if (response.command->name == "name" || response.command->name == "version") {
        const bool isName = response.command->name == "name";
        const QString prefix = isName ? uiText("engineNamePrefix") : uiText("engineVersionPrefix");
        const QString message = response.response.success
            ? prefix + QString::fromStdString(response.response.payload)
            : uiText("gtpCommandFailedPrefix").arg(QString::fromStdString(response.command->name)) +
                QString::fromStdString(response.response.payload);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message);
        }
        appendEngineLogLine(message, QStringLiteral("main"));
        return;
    }

    if (response.command->name != "list_commands") {
        return;
    }
    if (!response.response.success) {
        const QString message = uiText("listCommandsFailedPrefix") + QString::fromStdString(response.response.payload);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message);
        }
        appendEngineLogLine(message, QStringLiteral("main"));
        if (pendingAnalysisStart_) {
            stopRealtimeAnalysis();
        }
        stopSelfPlay();
        if (engineGameActive_) {
            stopEngineGame();
        }
        stopHumanVsAi();
        statusBar()->showMessage(message, 5000);
        return;
    }

    engines_.mainCapabilities() = lizzie::engine::EngineCapabilities::fromListCommandsPayload(response.response.payload);
    mainCapabilitiesKnown_ = true;
    const QString capabilitiesMessage = capabilitySummary(engines_.mainCapabilities(), engineSettings_.appearance.language);
    if (consoleWidget_ != nullptr) {
        consoleWidget_->appendSystemLine(capabilitiesMessage);
    }
    appendEngineLogLine(capabilitiesMessage, QStringLiteral("main"));
    if (pendingGenMoveStart_) {
        if (!engines_.mainCapabilities().genmove) {
            showWarning(
                this,
                engineSettings_.appearance.language,
                engineGameActive_ ? uiText("engineGame") : uiText("aiMove"),
                uiText("noGenmoveSupport"));
            if (engineGameActive_) {
                stopEngineGame();
            }
            stopHumanVsAi();
            stopSelfPlay();
            return;
        }
        syncAndRequestGenMove();
        return;
    }

    if (!pendingAnalysisStart_) {
        return;
    }
    if (!engines_.mainCapabilities().kataAnalyze) {
        showWarning(this, engineSettings_.appearance.language, uiText("analyze"), uiText("noAnalyzeSupport"));
        stopRealtimeAnalysis();
        return;
    }
    syncAndStartRealtimeAnalysis();
}

void MainWindow::handleCompareCommandResponse(const lizzie::engine::QueuedGtpResponse& response)
{
    if (!response.command.has_value()) {
        return;
    }
    const AnalysisStreamGateEvent gateEvent =
        compareStreamGate_.handleSyncResponse(response.command->id, response.response.success);
    if (gateEvent == AnalysisStreamGateEvent::Failed) {
        const QString message =
            uiText("compareSyncFailedPrefix") +
            uiText("gtpCommandFailedPrefix").arg(QString::fromStdString(response.command->name)) +
            QString::fromStdString(response.response.payload);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message, compareConsolePrefix(engineSettings_.appearance.language));
        }
        appendEngineLogLine(message, QStringLiteral("compare"));
        stopCompareAnalysis();
        statusBar()->showMessage(uiText("compareSyncSkipped"), 5000);
        return;
    }
    if (gateEvent == AnalysisStreamGateEvent::Waiting) {
        return;
    }
    if (gateEvent == AnalysisStreamGateEvent::Activated) {
        if (pendingCompareAnalyzeCommand_.has_value()) {
            if (!lizzie::app::dispatchGtpCommand(engines_.compareEngine(), *pendingCompareAnalyzeCommand_)) {
                failCompareAnalysisStartup(
                    uiText("compareSyncFailedPrefix") + uiText("analyzeCommandDispatchFailed"));
                return;
            }
        }
        pendingCompareAnalyzeCommand_.reset();
        pendingCompareStart_ = false;
        compareAnalysisActive_ = true;
        if (compareAction_ != nullptr) {
            compareAction_->setChecked(true);
        }
        updateCompareTable();
        return;
    }
    if (response.command->name == "genmove") {
        pendingCompareGenMoveStart_ = false;
        if (!response.response.success) {
            compareGenMoveGuard_.clear();
            if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(
                    QString::fromStdString(response.response.payload),
                    compareConsolePrefix(engineSettings_.appearance.language));
            }
            if (engineGameActive_) {
                stopEngineGame();
            }
            return;
        }

        std::string error;
        const lizzie::core::BoardPosition position = game_.currentPosition(&error);
        if (!error.empty()) {
            compareGenMoveGuard_.clear();
            statusBar()->showMessage(localizedCoreError(error, engineSettings_.appearance.language), 5000);
            stopEngineGame();
            return;
        }
        const std::string currentPositionKey = lizzie::engine::analysisPositionKey(game_, game_.currentId());
        if (!compareGenMoveGuard_.consumeIfMatches(currentPositionKey)) {
            if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(
                    uiText("ignoredStaleEngineMoveResponse"),
                    compareConsolePrefix(engineSettings_.appearance.language));
            }
            stopEngineGame();
            statusBar()->showMessage(uiText("ignoredStaleCompareEngineMove"), 3000);
            return;
        }
        const std::optional<lizzie::core::Move> move =
            lizzie::engine::moveFromGtpToken(response.response.payload, game_.boardSize(), position.sideToMove());
        if (!move.has_value()) {
            const QString message =
                uiText("couldNotParseCompareEngineMove") + ": " + QString::fromStdString(response.response.payload);
            if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(message, compareConsolePrefix(engineSettings_.appearance.language));
            }
            appendEngineLogLine(message, QStringLiteral("compare"));
            statusBar()->showMessage(uiText("couldNotParseCompareEngineMove"), 5000);
            stopEngineGame();
            return;
        }
        const bool previousMoveWasPass = currentNodeMoveWasPass(game_);
        if (!applyEngineMove(*move, QStringLiteral("compare"), compareConsolePrefix(engineSettings_.appearance.language))) {
            return;
        }
        const EngineAutomationPlan automationPlan = planAfterEngineMove(
            *move,
            EngineAutomationState{
                .selfPlayActive = selfPlayActive_,
                .engineGameActive = engineGameActive_,
                .humanVsAiActive = humanVsAiActive_,
                .previousMoveWasPass = previousMoveWasPass,
            });
        if (automationPlan.stopHumanVsAi) {
            stopHumanVsAi();
        }
        if (automationPlan.stopEngineGame) {
            stopEngineGame();
        }
        if (automationPlan.stopSelfPlay) {
            stopSelfPlay();
        }
        if (automationPlan.requestEngineGameMove) {
            requestEngineGameMove();
        }
        return;
    }

    if (response.command->name == "name" || response.command->name == "version") {
        const bool isName = response.command->name == "name";
        const QString prefix = isName ? uiText("engineNamePrefix") : uiText("engineVersionPrefix");
        const QString message = response.response.success
            ? prefix + QString::fromStdString(response.response.payload)
            : uiText("gtpCommandFailedPrefix").arg(QString::fromStdString(response.command->name)) +
                QString::fromStdString(response.response.payload);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message, compareConsolePrefix(engineSettings_.appearance.language));
        }
        appendEngineLogLine(message, QStringLiteral("compare"));
        return;
    }

    if (response.command->name != "list_commands") {
        return;
    }
    if (!response.response.success) {
        const QString message = uiText("listCommandsFailedPrefix") + QString::fromStdString(response.response.payload);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message, compareConsolePrefix(engineSettings_.appearance.language));
        }
        appendEngineLogLine(message, QStringLiteral("compare"));
        if (pendingCompareGenMoveStart_ || engineGameActive_) {
            stopEngineGame();
        }
        stopCompareAnalysis();
        statusBar()->showMessage(
            localizedCompareStatusMessage(message, engineSettings_.appearance.language),
            5000);
        return;
    }

    engines_.compareCapabilities() = lizzie::engine::EngineCapabilities::fromListCommandsPayload(response.response.payload);
    compareCapabilitiesKnown_ = true;
    const QString capabilitiesMessage =
        capabilitySummary(engines_.compareCapabilities(), engineSettings_.appearance.language);
    if (consoleWidget_ != nullptr) {
        consoleWidget_->appendSystemLine(capabilitiesMessage, compareConsolePrefix(engineSettings_.appearance.language));
    }
    appendEngineLogLine(capabilitiesMessage, QStringLiteral("compare"));
    if (pendingCompareGenMoveStart_) {
        if (!engines_.compareCapabilities().genmove) {
            showWarning(this, engineSettings_.appearance.language, uiText("engineGame"), uiText("compareNoGenmoveSupport"));
            pendingCompareGenMoveStart_ = false;
            compareGenMoveGuard_.clear();
            stopEngineGame();
            return;
        }
        syncAndRequestCompareGenMove();
        return;
    }
    if (!pendingCompareStart_) {
        return;
    }
    if (!engines_.compareCapabilities().kataAnalyze) {
        showWarning(this, engineSettings_.appearance.language, uiText("compare"), uiText("compareNoAnalyzeSupport"));
        stopCompareAnalysis();
        return;
    }
    syncAndStartCompareAnalysis();
}

void MainWindow::handleAnalyzeLine(const lizzie::engine::KataAnalyzeLine& line)
{
    if (!realtimeStreamGate_.acceptsAnalysis()) {
        return;
    }

    const std::optional<lizzie::core::AnalysisSnapshot> snapshot = realtimeAccumulator_.processUpdate(line);
    if (!snapshot.has_value()) {
        return;
    }
    if (analysisStore_.setSnapshot(game_.currentId(), *snapshot)) {
        updateBoard();
    }
}

void MainWindow::handleCompareAnalyzeLine(const lizzie::engine::KataAnalyzeLine& line)
{
    if (!compareStreamGate_.acceptsAnalysis()) {
        return;
    }

    const std::optional<lizzie::core::AnalysisSnapshot> snapshot = compareAccumulator_.processUpdate(line);
    if (!snapshot.has_value()) {
        return;
    }
    compareAnalysis_ = *snapshot;
    updateCompareTable();
}

void MainWindow::handleBatchResponse(const lizzie::engine::AnalysisResponse& response)
{
    const std::string* expectedPositionKey = batchTracker_.positionKeyFor(response.id);
    if (expectedPositionKey == nullptr) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(
                uiText("unmatchedStaleBatchResponsePrefix") + QString::fromStdString(response.id));
        }
        return;
    }
    const lizzie::app::AnalysisStoreUpdateResult result =
        analysisStore_.applyResponse(response, std::string_view(*expectedPositionKey));
    if (!result.applied()) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(
                uiText("unmatchedStaleBatchResponsePrefix") + QString::fromStdString(response.id));
        }
        batchTracker_.remove(response.id);
        return;
    }
    batchTracker_.remove(response.id);
    updateBoard();
}

void MainWindow::handleBatchRequestFailed(const QString& id, const QString& message, const QString& rawLine)
{
    const QString summary = uiText("analysisRequestFailedPrefix") + QString("%1: %2").arg(id, message);
    if (consoleWidget_ != nullptr) {
        consoleWidget_->appendSystemLine(summary);
    }
    appendEngineLogLine(summary, QStringLiteral("analysis"));

    const std::string requestId = id.toStdString();
    const std::string* trackedPositionKey = batchTracker_.positionKeyFor(requestId);
    std::optional<std::string_view> expectedPositionKey;
    if (trackedPositionKey != nullptr) {
        expectedPositionKey = std::string_view(*trackedPositionKey);
    }

    QString diagnostic = uiText("analysisRequestFailedPrefix") + message;
    if (!rawLine.isEmpty()) {
        diagnostic += QStringLiteral("\n") + rawResponsePrefix(engineSettings_.appearance.language) + rawLine;
    }
    const lizzie::app::AnalysisStoreUpdateResult result =
        analysisStore_.setErrorForRequest(requestId, diagnostic.toStdString(), expectedPositionKey);
    if (result.status == lizzie::app::AnalysisStoreUpdateStatus::InvalidRequestId) {
        return;
    }
    if (result.status == lizzie::app::AnalysisStoreUpdateStatus::MissingNode ||
        result.status == lizzie::app::AnalysisStoreUpdateStatus::MissingModel) {
        batchTracker_.remove(requestId);
        return;
    }
    if (result.status == lizzie::app::AnalysisStoreUpdateStatus::StalePosition) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(uiText("ignoredStaleAnalysisFailurePrefix") + id);
        }
        batchTracker_.remove(requestId);
        return;
    }
    batchTracker_.remove(requestId);
    batchRunFailed_ = true;
    if (result.nodeId == game_.currentId()) {
        updateBoard();
    }
}

void MainWindow::markOutstandingBatchNodesFailed(const QString& message)
{
    if (batchTracker_.empty()) {
        return;
    }

    const QString diagnostic = uiText("analysisProcessFailedPrefix") + message;
    const std::vector<lizzie::app::TrackedAnalysisRequest> pendingRequests = batchTracker_.pendingRequests();
    for (const lizzie::app::TrackedAnalysisRequest& request : pendingRequests) {
        const lizzie::app::AnalysisStoreUpdateResult result = analysisStore_.setErrorForRequest(
            request.requestId,
            diagnostic.toStdString(),
            std::string_view(request.positionKey));
        if (result.status == lizzie::app::AnalysisStoreUpdateStatus::StalePosition) {
            if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(
                    uiText("ignoredStaleAnalysisProcessFailurePrefix") + QString::fromStdString(request.requestId));
            }
        }
    }
    updateBoard();
}

void MainWindow::handleBatchProgress(int completed, int total)
{
    const AnalysisRunMode mode = analysisRunMode_;
    if (mode == AnalysisRunMode::None) {
        return;
    }
    if (batchProgress_ != nullptr) {
        batchProgress_->setMaximum(total);
        batchProgress_->setValue(completed);
    }
    if (analysisRunMode_ != mode) {
        return;
    }
    const QString label = mode == AnalysisRunMode::Estimate ? uiText("estimate") : uiText("batchAnalysis");
    statusBar()->showMessage(QString("%1 %2/%3").arg(label).arg(completed).arg(total));
}

void MainWindow::handleBatchFinished(bool cancelled)
{
    if (batchProgress_ != nullptr) {
        batchProgress_->close();
        batchProgress_->deleteLater();
        batchProgress_ = nullptr;
    }
    if (batchAction_ != nullptr) {
        batchAction_->setEnabled(true);
    }
    if (estimateAction_ != nullptr) {
        estimateAction_->setEnabled(true);
    }
    updateBoard();
    const AnalysisRunMode mode = analysisRunMode_;
    const bool failed = batchRunFailed_;
    analysisRunMode_ = AnalysisRunMode::None;
    batchRunFailed_ = false;
    batchTracker_.clear();

    const lizzie::app::BatchAnalysisCompletionPlan completion = lizzie::app::planBatchAnalysisCompletion(
        mode,
        cancelled,
        failed,
        documentSession_,
        engineSettings_.fileBehavior.writeAnalysisSidecarAfterBatch);
    if (completion.output.has_value()) {
        const lizzie::app::BatchAnalysisOutputPlan& output = *completion.output;
        if ((output.writeSidecar ? writeAnalysisSidecar(output.path) : saveSgfTo(output.path, false, false))) {
            if (output.writeSidecar) {
                appliedAnalysisSidecarForDocument_ = true;
                analysisSidecarSyncPending_ = false;
            }
            if (output.normalizeSingleGameCollectionAfterSgfWrite) {
                documentSession_.collectionGameIndex = 0;
                documentSession_.collectionGameCount = 1;
            }
            statusBar()->showMessage(uiText(batchCompletionMessageKey(output.message)) + output.path);
            return;
        }
        statusBar()->showMessage(uiText("batchFailed"));
        return;
    }
    statusBar()->showMessage(uiText(batchCompletionMessageKey(completion.fallbackMessage)));
}

void MainWindow::sendConsoleCommand(const QString& command)
{
    if (command.trimmed().isEmpty()) {
        return;
    }
    if (!engines_.mainEngine().isRunning()) {
        const QString message = uiText("engineNotRunning");
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message);
        }
        appendEngineLogLine(message, QStringLiteral("main"));
        statusBar()->showMessage(message, 3000);
        return;
    }

    if (!lizzie::app::dispatchConsoleCommand(engines_.mainEngine(), command)) {
        reportSystemDiagnostic(uiText("consoleCommandRejected"), QStringLiteral("main"), QString(), 3000);
    }
}

void MainWindow::selectTreeNode()
{
    if (updatingUi_ || gameTreeWidget_ == nullptr || gameTreeWidget_->currentItem() == nullptr) {
        return;
    }
    const lizzie::core::NodeId nodeId =
        gameTreeWidget_->currentItem()->data(0, Qt::UserRole).toULongLong();
    if (gameEditor_.setCurrent(nodeId)) {
        stopAutomatedPlayModes();
        updateBoard();
        if (realtimeAnalysisRequested()) {
            syncAndStartRealtimeAnalysis();
        }
        if (compareAnalysisRequested()) {
            syncAndStartCompareAnalysis();
        }
    }
}

void MainWindow::updateCurrentComment()
{
    if (updatingUi_ || commentEditor_ == nullptr) {
        return;
    }

    if (gameEditor_.setComment(game_.currentId(), commentEditor_->toPlainText().toStdString())) {
        updateGameTree();
    }
}

void MainWindow::buildToolbar()
{
    mainToolbar_ = addToolBar("Main");
    mainToolbar_->setObjectName("lizzieMainToolbar");
    mainToolbar_->setMovable(false);

    newAction_ = new QAction(style()->standardIcon(QStyle::SP_FileIcon), "New", this);
    connect(newAction_, &QAction::triggered, this, &MainWindow::newGame);
    openAction_ = new QAction(style()->standardIcon(QStyle::SP_DialogOpenButton), "Open", this);
    mainToolbar_->addAction(openAction_);
    connect(openAction_, &QAction::triggered, this, &MainWindow::openSgf);
    saveAction_ = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Save", this);
    mainToolbar_->addAction(saveAction_);
    connect(saveAction_, &QAction::triggered, this, &MainWindow::saveSgf);
    saveAsAction_ = new QAction(style()->standardIcon(QStyle::SP_DialogSaveButton), "Save As", this);
    connect(saveAsAction_, &QAction::triggered, this, &MainWindow::saveSgfAs);
    gameInfoAction_ = new QAction(style()->standardIcon(QStyle::SP_FileDialogInfoView), "Game Info", this);
    gameInfoAction_->setToolTip(uiText("gameInfoTooltip"));
    connect(gameInfoAction_, &QAction::triggered, this, &MainWindow::editGameInfo);
    importJavaConfigAction_ = new QAction("Import Java Config", this);
    importJavaConfigAction_->setToolTip(uiText("importJavaConfigTooltip"));
    connect(importJavaConfigAction_, &QAction::triggered, this, [this] {
        importLegacyJavaConfigFromDialog();
    });
    mainToolbar_->addSeparator();
    analyzeAction_ = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), "Analyze", this);
    analyzeAction_->setCheckable(true);
    mainToolbar_->addAction(analyzeAction_);
    connect(analyzeAction_, &QAction::triggered, this, &MainWindow::startOrStopAnalysis);
    estimateAction_ = new QAction(style()->standardIcon(QStyle::SP_DialogApplyButton), "Estimate", this);
    mainToolbar_->addAction(estimateAction_);
    connect(estimateAction_, &QAction::triggered, this, &MainWindow::startEstimate);
    batchAction_ = new QAction("Batch", this);
    connect(batchAction_, &QAction::triggered, this, &MainWindow::startBatchAnalysis);
    restartEngineAction_ = new QAction(style()->standardIcon(QStyle::SP_BrowserReload), "Restart", this);
    connect(restartEngineAction_, &QAction::triggered, this, &MainWindow::restartEngine);
    compareAction_ = new QAction("Compare", this);
    compareAction_->setCheckable(true);
    connect(compareAction_, &QAction::triggered, this, &MainWindow::startOrStopCompare);
    aiMoveAction_ = new QAction("AI Move", this);
    connect(aiMoveAction_, &QAction::triggered, this, [this] {
        stopAutomatedPlayModes();
        requestAiMove();
    });
    humanVsAiAction_ = new QAction("Human vs AI", this);
    humanVsAiAction_->setCheckable(true);
    connect(humanVsAiAction_, &QAction::triggered, this, &MainWindow::toggleHumanVsAi);
    selfPlayAction_ = new QAction("Self Play", this);
    selfPlayAction_->setCheckable(true);
    connect(selfPlayAction_, &QAction::triggered, this, &MainWindow::toggleSelfPlay);
    engineGameAction_ = new QAction("Engine Game", this);
    engineGameAction_->setCheckable(true);
    connect(engineGameAction_, &QAction::triggered, this, &MainWindow::toggleEngineGame);
    mainToolbar_->addSeparator();
    previousAction_ = new QAction(style()->standardIcon(QStyle::SP_ArrowBack), "Previous", this);
    mainToolbar_->addAction(previousAction_);
    connect(previousAction_, &QAction::triggered, this, &MainWindow::previousMove);
    nextAction_ = new QAction(style()->standardIcon(QStyle::SP_ArrowForward), "Next", this);
    mainToolbar_->addAction(nextAction_);
    connect(nextAction_, &QAction::triggered, this, &MainWindow::nextMove);
    undoAction_ = new QAction("Undo", this);
    connect(undoAction_, &QAction::triggered, this, &MainWindow::undoMove);
    passAction_ = new QAction("Pass", this);
    connect(passAction_, &QAction::triggered, this, &MainWindow::passMove);
    resignAction_ = new QAction("Resign", this);
    resignAction_->setToolTip(uiText("resignTooltip"));
    connect(resignAction_, &QAction::triggered, this, &MainWindow::resignMove);
    promoteVariationAction_ = new QAction("Promote Variation", this);
    promoteVariationAction_->setToolTip(uiText("promoteVariationTooltip"));
    connect(promoteVariationAction_, &QAction::triggered, this, &MainWindow::promoteCurrentVariation);
    deleteBranchAction_ = new QAction("Delete Branch", this);
    deleteBranchAction_->setToolTip(uiText("deleteBranchTooltip"));
    connect(deleteBranchAction_, &QAction::triggered, this, &MainWindow::deleteCurrentBranch);

    auto* editModeGroup = new QActionGroup(this);
    editModeGroup->setExclusive(true);
    playModeAction_ = new QAction("Play", this);
    playModeAction_->setCheckable(true);
    playModeAction_->setChecked(true);
    playModeAction_->setToolTip(uiText("playModeTooltip"));
    editModeGroup->addAction(playModeAction_);
    connect(playModeAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::Play);
    });
    tryModeAction_ = new QAction(style()->standardIcon(QStyle::SP_FileDialogContentsView), "Try", this);
    tryModeAction_->setCheckable(true);
    tryModeAction_->setToolTip(uiText("tryModeTooltip"));
    mainToolbar_->addAction(tryModeAction_);
    editModeGroup->addAction(tryModeAction_);
    connect(tryModeAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::Try);
    });
    labelModeAction_ = new QAction("Label", this);
    labelModeAction_->setCheckable(true);
    labelModeAction_->setToolTip(uiText("labelModeTooltip"));
    editModeGroup->addAction(labelModeAction_);
    connect(labelModeAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::Label);
    });
    setupBlackAction_ = new QAction("Setup Black", this);
    setupBlackAction_->setCheckable(true);
    setupBlackAction_->setToolTip(uiText("setupBlackTooltip"));
    editModeGroup->addAction(setupBlackAction_);
    connect(setupBlackAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::SetupBlack);
    });
    setupWhiteAction_ = new QAction("Setup White", this);
    setupWhiteAction_->setCheckable(true);
    setupWhiteAction_->setToolTip(uiText("setupWhiteTooltip"));
    editModeGroup->addAction(setupWhiteAction_);
    connect(setupWhiteAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::SetupWhite);
    });
    setupClearAction_ = new QAction("Setup Clear", this);
    setupClearAction_->setCheckable(true);
    setupClearAction_->setToolTip(uiText("setupClearTooltip"));
    editModeGroup->addAction(setupClearAction_);
    connect(setupClearAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::SetupClear);
    });
    circleMarkAction_ = new QAction("Circle", this);
    circleMarkAction_->setCheckable(true);
    circleMarkAction_->setToolTip(uiText("circleMarkTooltip"));
    editModeGroup->addAction(circleMarkAction_);
    connect(circleMarkAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::Circle);
    });
    squareMarkAction_ = new QAction("Square", this);
    squareMarkAction_->setCheckable(true);
    squareMarkAction_->setToolTip(uiText("squareMarkTooltip"));
    editModeGroup->addAction(squareMarkAction_);
    connect(squareMarkAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::Square);
    });
    triangleMarkAction_ = new QAction("Tri", this);
    triangleMarkAction_->setCheckable(true);
    triangleMarkAction_->setToolTip(uiText("triangleMarkTooltip"));
    editModeGroup->addAction(triangleMarkAction_);
    connect(triangleMarkAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::Triangle);
    });
    crossMarkAction_ = new QAction("Cross", this);
    crossMarkAction_->setCheckable(true);
    crossMarkAction_->setToolTip(uiText("crossMarkTooltip"));
    editModeGroup->addAction(crossMarkAction_);
    connect(crossMarkAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::Cross);
    });
    clearMarkupAction_ = new QAction("Clear", this);
    clearMarkupAction_->setCheckable(true);
    clearMarkupAction_->setToolTip(uiText("clearMarkupTooltip"));
    editModeGroup->addAction(clearMarkupAction_);
    connect(clearMarkupAction_, &QAction::triggered, this, [this] {
        setBoardEditMode(BoardEditMode::Clear);
    });
    mainToolbar_->addSeparator();
    settingsAction_ = new QAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), "Settings", this);
    mainToolbar_->addAction(settingsAction_);
    connect(settingsAction_, &QAction::triggered, this, [this] {
        stopAutomatedPlayModes();
        showSettings();
    });
    applyShortcuts();
    applyActionTooltips();
}

void MainWindow::buildDocks()
{
    candidatesDock_ = new QDockWidget("Candidates", this);
    candidatesDock_->setObjectName("lizzieCandidatesDock");
    candidatesTable_ = createCandidateTable();
    candidatesTable_->setObjectName("lizzieCandidatesTable");
    connect(candidatesTable_, &QTableWidget::currentCellChanged, this, [this](int currentRow, int, int, int) {
        showCandidatePreview(currentRow);
    });
    connect(candidatesTable_, &QTableWidget::cellEntered, this, [this](int row, int) {
        showCandidatePreview(row);
    });
    candidatesDock_->setWidget(candidatesTable_);
    addDockWidget(Qt::RightDockWidgetArea, candidatesDock_);

    ownershipDock_ = new QDockWidget("Ownership Board", this);
    ownershipDock_->setObjectName("lizzieOwnershipDock");
    auto* ownershipQuickWidget = new QQuickWidget(this);
    ownershipQuickWidget->setObjectName("lizzieOwnershipQuickWidget");
    ownershipQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    ownershipQuickWidget->setMinimumSize(220, 220);
    ownershipBoardItem_ = new BoardQuickItem;
    ownershipQuickWidget->setContent(QUrl(), nullptr, ownershipBoardItem_);
    ownershipDock_->setWidget(ownershipQuickWidget);
    addDockWidget(Qt::RightDockWidgetArea, ownershipDock_);
    tabifyDockWidget(candidatesDock_, ownershipDock_);

    treeDock_ = new QDockWidget("Game Tree", this);
    treeDock_->setObjectName("lizzieTreeDock");
    gameTreeWidget_ = new QTreeWidget;
    gameTreeWidget_->setObjectName("lizzieGameTree");
    gameTreeWidget_->setColumnCount(2);
    gameTreeWidget_->setHeaderLabels({"Move"});
    gameTreeWidget_->hideColumn(1);
    gameTreeWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(gameTreeWidget_, &QTreeWidget::itemSelectionChanged, this, &MainWindow::selectTreeNode);
    treeDock_->setWidget(gameTreeWidget_);
    addDockWidget(Qt::LeftDockWidgetArea, treeDock_);

    commentDock_ = new QDockWidget("Comment", this);
    commentDock_->setObjectName("lizzieCommentDock");
    commentEditor_ = new QPlainTextEdit;
    commentEditor_->setObjectName("lizzieCommentEditor");
    commentEditor_->setPlaceholderText("Node comment");
    connect(commentEditor_, &QPlainTextEdit::textChanged, this, &MainWindow::updateCurrentComment);
    commentDock_->setWidget(commentEditor_);
    addDockWidget(Qt::LeftDockWidgetArea, commentDock_);
    tabifyDockWidget(treeDock_, commentDock_);

    graphDock_ = new QDockWidget("Winrate / Score", this);
    graphDock_->setObjectName("lizzieGraphDock");
    graphWidget_ = new AnalysisGraphWidget;
    graphWidget_->setObjectName("lizzieAnalysisGraph");
    connect(graphWidget_, &AnalysisGraphWidget::nodeClicked, this, [this](lizzie::core::NodeId nodeId) {
        if (gameEditor_.setCurrent(nodeId)) {
            stopAutomatedPlayModes();
            updateBoard();
            if (realtimeAnalysisRequested()) {
                syncAndStartRealtimeAnalysis();
            }
            if (compareAnalysisRequested()) {
                syncAndStartCompareAnalysis();
            }
        }
    });
    graphDock_->setWidget(graphWidget_);
    addDockWidget(Qt::BottomDockWidgetArea, graphDock_);

    compareDock_ = new QDockWidget("Engine Compare", this);
    compareDock_->setObjectName("lizzieCompareDock");
    compareTable_ = createCompareTable();
    compareTable_->setObjectName("lizzieCompareTable");
    compareDock_->setWidget(compareTable_);
    addDockWidget(Qt::RightDockWidgetArea, compareDock_);
    tabifyDockWidget(candidatesDock_, compareDock_);

    consoleDock_ = new QDockWidget("GTP Console", this);
    consoleDock_->setObjectName("lizzieConsoleDock");
    consoleWidget_ = new GtpConsoleWidget;
    consoleWidget_->setObjectName("lizzieGtpConsole");
    connect(consoleWidget_, &GtpConsoleWidget::commandSubmitted, this, &MainWindow::sendConsoleCommand);
    consoleDock_->setWidget(consoleWidget_);
    addDockWidget(Qt::BottomDockWidgetArea, consoleDock_);
    tabifyDockWidget(graphDock_, consoleDock_);

    engineLogDock_ = new QDockWidget("Engine Log", this);
    engineLogDock_->setObjectName("lizzieEngineLogDock");
    engineLogOutput_ = new QPlainTextEdit;
    engineLogOutput_->setObjectName("lizzieEngineLogOutput");
    engineLogOutput_->setReadOnly(true);
    engineLogOutput_->setMaximumBlockCount(5000);
    engineLogDock_->setWidget(engineLogOutput_);
    addDockWidget(Qt::BottomDockWidgetArea, engineLogDock_);
    tabifyDockWidget(consoleDock_, engineLogDock_);
}

void MainWindow::buildMenus()
{
    fileMenu_ = menuBar()->addMenu("File");
    fileMenu_->addAction(newAction_);
    fileMenu_->addAction(openAction_);
    fileMenu_->addAction(saveAction_);
    fileMenu_->addAction(saveAsAction_);
    fileMenu_->addAction(gameInfoAction_);
    fileMenu_->addAction(importJavaConfigAction_);
    fileMenu_->addSeparator();
    fileMenu_->addAction(settingsAction_);

    engineMenu_ = menuBar()->addMenu("Engine");
    engineMenu_->addAction(analyzeAction_);
    engineMenu_->addAction(estimateAction_);
    engineMenu_->addAction(batchAction_);
    engineMenu_->addSeparator();
    engineMenu_->addAction(restartEngineAction_);
    engineMenu_->addSeparator();
    engineMenu_->addAction(compareAction_);
    engineMenu_->addAction(aiMoveAction_);
    engineMenu_->addAction(humanVsAiAction_);
    engineMenu_->addAction(selfPlayAction_);
    engineMenu_->addAction(engineGameAction_);

    navigateMenu_ = menuBar()->addMenu("Navigate");
    navigateMenu_->addAction(previousAction_);
    navigateMenu_->addAction(nextAction_);
    navigateMenu_->addAction(undoAction_);
    navigateMenu_->addAction(passAction_);
    navigateMenu_->addAction(resignAction_);
    navigateMenu_->addSeparator();
    navigateMenu_->addAction(promoteVariationAction_);
    navigateMenu_->addAction(deleteBranchAction_);

    markupMenu_ = menuBar()->addMenu("Markup");
    markupMenu_->addAction(playModeAction_);
    markupMenu_->addAction(tryModeAction_);
    markupMenu_->addAction(labelModeAction_);
    markupMenu_->addSeparator();
    markupMenu_->addAction(setupBlackAction_);
    markupMenu_->addAction(setupWhiteAction_);
    markupMenu_->addAction(setupClearAction_);
    markupMenu_->addSeparator();
    markupMenu_->addAction(circleMarkAction_);
    markupMenu_->addAction(squareMarkAction_);
    markupMenu_->addAction(triangleMarkAction_);
    markupMenu_->addAction(crossMarkAction_);
    markupMenu_->addSeparator();
    markupMenu_->addAction(clearMarkupAction_);

    viewMenu_ = menuBar()->addMenu("View");
    viewMenu_->addAction(mainToolbar_->toggleViewAction());
    viewMenu_->addSeparator();
    viewMenu_->addAction(candidatesDock_->toggleViewAction());
    viewMenu_->addAction(ownershipDock_->toggleViewAction());
    viewMenu_->addAction(compareDock_->toggleViewAction());
    viewMenu_->addAction(treeDock_->toggleViewAction());
    viewMenu_->addAction(commentDock_->toggleViewAction());
    viewMenu_->addAction(graphDock_->toggleViewAction());
    viewMenu_->addAction(consoleDock_->toggleViewAction());
    viewMenu_->addAction(engineLogDock_->toggleViewAction());
}

void MainWindow::connectEngine()
{
    connect(&engines_.mainEngine(), &lizzie::engine::ThreadedKataGoProcess::protocolLineReceived, this, [this](const QString& line) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendProtocolLine(line);
        }
    });
    connect(&engines_.mainEngine(), &lizzie::engine::ThreadedKataGoProcess::stderrLineReceived, this, [this](const QString& line) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendStderrLine(line);
        }
        appendEngineLogLine(line, QStringLiteral("main"));
    });
    connect(&engines_.mainEngine(), &lizzie::engine::ThreadedKataGoProcess::startupFailed, this, [this](const QString& message) {
        mainCapabilitiesKnown_ = false;
        engines_.resetMainCapabilities();
        realtimeStreamGate_.stop();
        pendingAnalysisStart_ = false;
        pendingRealtimeAnalyzeCommand_.reset();
        stopSelfPlay();
        stopEngineGame();
        stopHumanVsAi();
        realtimeAnalysisActive_ = false;
        if (analyzeAction_ != nullptr) {
            analyzeAction_->setChecked(false);
        }
        const QString displayMessage = localizedProcessDiagnostic(message, engineSettings_.appearance.language);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(displayMessage);
        }
        appendEngineLogLine(displayMessage, QStringLiteral("main"));
        statusBar()->showMessage(displayMessage, 5000);
    });
    connect(&engines_.mainEngine(), &lizzie::engine::ThreadedKataGoProcess::exited, this, [this](int exitCode, lizzie::engine::ProcessExitStatus status) {
        mainCapabilitiesKnown_ = false;
        engines_.resetMainCapabilities();
        realtimeStreamGate_.stop();
        pendingAnalysisStart_ = false;
        pendingRealtimeAnalyzeCommand_.reset();
        stopSelfPlay();
        stopEngineGame();
        stopHumanVsAi();
        realtimeAnalysisActive_ = false;
        if (analyzeAction_ != nullptr) {
            analyzeAction_->setChecked(false);
        }
        const QString message = uiText("katagoExited").arg(exitCode).arg(static_cast<int>(status));
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message);
        }
        appendEngineLogLine(message, QStringLiteral("main"));
        statusBar()->showMessage(message, 5000);
    });
    connect(
        &engines_.mainEngine(),
        &lizzie::engine::ThreadedKataGoProcess::commandResponseReceived,
        this,
        &MainWindow::handleCommandResponse);
    connect(
        &engines_.mainEngine(),
        &lizzie::engine::ThreadedKataGoProcess::kataAnalyzeLineReceived,
        this,
        &MainWindow::handleAnalyzeLine);

    connect(&engines_.compareEngine(), &lizzie::engine::ThreadedKataGoProcess::protocolLineReceived, this, [this](const QString& line) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendProtocolLine(compareConsolePrefix(engineSettings_.appearance.language) + line);
        }
    });
    connect(&engines_.compareEngine(), &lizzie::engine::ThreadedKataGoProcess::stderrLineReceived, this, [this](const QString& line) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendStderrLine(line, compareConsolePrefix(engineSettings_.appearance.language));
        }
        appendEngineLogLine(line, QStringLiteral("compare"));
    });
    connect(&engines_.compareEngine(), &lizzie::engine::ThreadedKataGoProcess::startupFailed, this, [this](const QString& message) {
        compareCapabilitiesKnown_ = false;
        engines_.resetCompareCapabilities();
        const bool shouldStopEngineGame = engineGameActive_ || pendingCompareGenMoveStart_;
        compareStreamGate_.stop();
        pendingCompareStart_ = false;
        pendingCompareAnalyzeCommand_.reset();
        compareAnalysisActive_ = false;
        if (shouldStopEngineGame) {
            stopEngineGame();
        } else {
            pendingCompareGenMoveStart_ = false;
            compareGenMoveGuard_.clear();
        }
        if (compareAction_ != nullptr) {
            compareAction_->setChecked(false);
        }
        const QString displayMessage = localizedProcessDiagnostic(message, engineSettings_.appearance.language);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(displayMessage, compareConsolePrefix(engineSettings_.appearance.language));
        }
        appendEngineLogLine(displayMessage, QStringLiteral("compare"));
        statusBar()->showMessage(
            localizedCompareStatusMessage(displayMessage, engineSettings_.appearance.language),
            5000);
    });
    connect(&engines_.compareEngine(), &lizzie::engine::ThreadedKataGoProcess::exited, this, [this](int exitCode, lizzie::engine::ProcessExitStatus status) {
        compareCapabilitiesKnown_ = false;
        engines_.resetCompareCapabilities();
        const bool shouldStopEngineGame = engineGameActive_ || pendingCompareGenMoveStart_;
        compareStreamGate_.stop();
        pendingCompareStart_ = false;
        pendingCompareAnalyzeCommand_.reset();
        compareAnalysisActive_ = false;
        if (shouldStopEngineGame) {
            stopEngineGame();
        } else {
            pendingCompareGenMoveStart_ = false;
            compareGenMoveGuard_.clear();
        }
        if (compareAction_ != nullptr) {
            compareAction_->setChecked(false);
        }
        const QString message = uiText("katagoExited").arg(exitCode).arg(static_cast<int>(status));
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message, compareConsolePrefix(engineSettings_.appearance.language));
        }
        appendEngineLogLine(message, QStringLiteral("compare"));
        statusBar()->showMessage(
            localizedCompareStatusMessage(message, engineSettings_.appearance.language),
            5000);
    });
    connect(
        &engines_.compareEngine(),
        &lizzie::engine::ThreadedKataGoProcess::commandResponseReceived,
        this,
        &MainWindow::handleCompareCommandResponse);
    connect(
        &engines_.compareEngine(),
        &lizzie::engine::ThreadedKataGoProcess::kataAnalyzeLineReceived,
        this,
        &MainWindow::handleCompareAnalyzeLine);

    connect(&engines_.batchAnalysis(), &lizzie::engine::ThreadedAnalysisProcess::responseReceived, this, &MainWindow::handleBatchResponse);
    connect(
        &engines_.batchAnalysis(),
        &lizzie::engine::ThreadedAnalysisProcess::requestFailed,
        this,
        &MainWindow::handleBatchRequestFailed);
    connect(&engines_.batchAnalysis(), &lizzie::engine::ThreadedAnalysisProcess::progressChanged, this, &MainWindow::handleBatchProgress);
    connect(&engines_.batchAnalysis(), &lizzie::engine::ThreadedAnalysisProcess::finished, this, &MainWindow::handleBatchFinished);
    connect(&engines_.batchAnalysis(), &lizzie::engine::ThreadedAnalysisProcess::stderrLineReceived, this, [this](const QString& line) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendStderrLine(line);
        }
        appendEngineLogLine(line, QStringLiteral("analysis"));
    });
    connect(&engines_.batchAnalysis(), &lizzie::engine::ThreadedAnalysisProcess::parseFailed, this, [this](const QString& line) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(uiText("analysisJsonParseFailedPrefix") + line);
        }
        appendEngineLogLine(uiText("analysisJsonParseFailedPrefix") + line, QStringLiteral("analysis"));
    });
    connect(
        &engines_.batchAnalysis(),
        &lizzie::engine::ThreadedAnalysisProcess::unknownResponseIgnored,
        this,
        [this](const QString& id, const QString& line) {
            const QString message =
                uiText("unknownAnalysisResponsePrefix") + id + QStringLiteral("\n") +
                rawResponsePrefix(engineSettings_.appearance.language) + line;
            if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(message);
            }
            appendEngineLogLine(message, QStringLiteral("analysis"));
        });
    connect(
        &engines_.batchAnalysis(),
        &lizzie::engine::ThreadedAnalysisProcess::duplicateResponseIgnored,
        this,
        [this](const QString& id, const QString& line) {
            const QString message =
                uiText("duplicateAnalysisResponsePrefix") + id + QStringLiteral("\n") +
                rawResponsePrefix(engineSettings_.appearance.language) + line;
            if (consoleWidget_ != nullptr) {
                consoleWidget_->appendSystemLine(message);
            }
            appendEngineLogLine(message, QStringLiteral("analysis"));
        });
    connect(&engines_.batchAnalysis(), &lizzie::engine::ThreadedAnalysisProcess::failed, this, [this](const QString& message) {
        const QString displayMessage = localizedProcessDiagnostic(message, engineSettings_.appearance.language);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(displayMessage);
        }
        appendEngineLogLine(displayMessage, QStringLiteral("analysis"));
        batchRunFailed_ = true;
        markOutstandingBatchNodesFailed(displayMessage);
        if (batchProgress_ != nullptr) {
            batchProgress_->close();
            batchProgress_->deleteLater();
            batchProgress_ = nullptr;
        }
        if (batchAction_ != nullptr) {
            batchAction_->setEnabled(true);
        }
        if (estimateAction_ != nullptr) {
            estimateAction_->setEnabled(true);
        }
        statusBar()->showMessage(displayMessage, 5000);
    });
}

void MainWindow::loadSettings()
{
    QSettings settings;
    engineSettings_ = lizzie::app::loadEngineUiSettings(settings);

    if (settings.contains("window/geometry")) {
        restoreGeometry(settings.value("window/geometry").toByteArray());
        const std::optional<QRect> adjustedGeometry =
            lizzie::app::adjustedWindowGeometryForAvailableScreens(frameGeometry(), size());
        if (adjustedGeometry.has_value()) {
            resize(adjustedGeometry->size());
            move(adjustedGeometry->topLeft());
        }
    }
    if (settings.contains("window/state")) {
        restoreState(settings.value("window/state").toByteArray());
    }
}

void MainWindow::saveSettings()
{
    QSettings settings;
    lizzie::app::saveEngineUiSettings(settings, engineSettings_);
    settings.setValue("window/geometry", saveGeometry());
    settings.setValue("window/state", saveState());
    lizzie::app::saveSessionSettings(
        settings,
        documentSession_.currentFile,
        game_.currentId(),
        lizzie::app::selectedCollectionGameIndex(documentSession_));
}

void MainWindow::applyShortcuts()
{
    setActionShortcut(newAction_, engineSettings_.shortcuts.newGame, uiText("new"));
    setActionShortcut(openAction_, engineSettings_.shortcuts.openSgf, uiText("open"));
    setActionShortcut(saveAction_, engineSettings_.shortcuts.saveSgf, uiText("save"));
    setActionShortcut(saveAsAction_, engineSettings_.shortcuts.saveSgfAs, uiText("saveAs"));
    setActionShortcut(analyzeAction_, engineSettings_.shortcuts.analyze, uiText("analyze"));
    setActionShortcut(estimateAction_, engineSettings_.shortcuts.estimate, uiText("estimate"));
    setActionShortcut(batchAction_, engineSettings_.shortcuts.batchAnalyze, uiText("batch"));
    setActionShortcut(restartEngineAction_, engineSettings_.shortcuts.restartEngine, uiText("restartEngine"));
    setActionShortcut(compareAction_, engineSettings_.shortcuts.compare, uiText("compare"));
    setActionShortcut(aiMoveAction_, engineSettings_.shortcuts.aiMove, uiText("aiMove"));
    setActionShortcut(humanVsAiAction_, engineSettings_.shortcuts.humanVsAi, uiText("humanVsAi"));
    setActionShortcut(selfPlayAction_, engineSettings_.shortcuts.selfPlay, uiText("selfPlay"));
    setActionShortcut(engineGameAction_, engineSettings_.shortcuts.engineGame, uiText("engineGame"));
    setActionShortcut(previousAction_, engineSettings_.shortcuts.previousMove, uiText("previous"));
    setActionShortcut(nextAction_, engineSettings_.shortcuts.nextMove, uiText("next"));
    setActionShortcut(undoAction_, engineSettings_.shortcuts.undoMove, uiText("undo"));
    setActionShortcut(passAction_, engineSettings_.shortcuts.passMove, uiText("pass"));
    setActionShortcut(resignAction_, engineSettings_.shortcuts.resignMove, uiText("resign"));
    setActionShortcut(settingsAction_, engineSettings_.shortcuts.settings, uiText("settings"));
}

void MainWindow::applyActionTooltips()
{
    const auto setTooltip = [this](QAction* action, const char* key) {
        if (action != nullptr) {
            action->setToolTip(uiText(key));
        }
    };

    setTooltip(gameInfoAction_, "gameInfoTooltip");
    setTooltip(importJavaConfigAction_, "importJavaConfigTooltip");
    setTooltip(promoteVariationAction_, "promoteVariationTooltip");
    setTooltip(deleteBranchAction_, "deleteBranchTooltip");
    setTooltip(playModeAction_, "playModeTooltip");
    setTooltip(tryModeAction_, "tryModeTooltip");
    setTooltip(labelModeAction_, "labelModeTooltip");
    setTooltip(setupBlackAction_, "setupBlackTooltip");
    setTooltip(setupWhiteAction_, "setupWhiteTooltip");
    setTooltip(setupClearAction_, "setupClearTooltip");
    setTooltip(circleMarkAction_, "circleMarkTooltip");
    setTooltip(squareMarkAction_, "squareMarkTooltip");
    setTooltip(triangleMarkAction_, "triangleMarkTooltip");
    setTooltip(crossMarkAction_, "crossMarkTooltip");
    setTooltip(clearMarkupAction_, "clearMarkupTooltip");
}

void MainWindow::applyAppearance()
{
    static const QFont baseFont = qApp->font();
    QFont font = baseFont;
    if (font.pointSizeF() > 0.0) {
        font.setPointSizeF(font.pointSizeF() * engineSettings_.appearance.fontScale);
    }
    qApp->setFont(font);
    if (graphWidget_ != nullptr) {
        const bool darkGraphTheme = engineSettings_.appearance.theme == ThemeMode::Dark ||
            (engineSettings_.appearance.theme == ThemeMode::System &&
             qApp->palette().color(QPalette::Window).lightness() < 128);
        graphWidget_->setDarkTheme(darkGraphTheme);
    }

    if (engineSettings_.appearance.theme == ThemeMode::Dark) {
        qApp->setStyleSheet(
            "QWidget { background: #202328; color: #e6e8eb; }"
            "QMainWindow, QDockWidget { background: #202328; }"
            "QToolBar { background: #292d33; border: 0; spacing: 4px; }"
            "QToolButton, QPushButton { background: #343a42; border: 1px solid #4c535d; padding: 5px 8px; }"
            "QToolButton:checked { background: #315f6a; }"
            "QLineEdit, QPlainTextEdit, QTableWidget, QTreeWidget, QComboBox, QSpinBox, QDoubleSpinBox {"
            " background: #15181c; color: #f1f3f5; border: 1px solid #4c535d; }"
            "QHeaderView::section { background: #343a42; color: #f1f3f5; border: 1px solid #4c535d; }");
        return;
    }
    if (engineSettings_.appearance.theme == ThemeMode::Light) {
        qApp->setStyleSheet(
            "QWidget { background: #f6f7f8; color: #202428; }"
            "QToolBar { background: #eef0f2; border: 0; spacing: 4px; }"
            "QToolButton, QPushButton { background: #ffffff; border: 1px solid #c7cdd4; padding: 5px 8px; }"
            "QToolButton:checked { background: #d8edf0; }"
            "QLineEdit, QPlainTextEdit, QTableWidget, QTreeWidget, QComboBox, QSpinBox, QDoubleSpinBox {"
            " background: #ffffff; color: #202428; border: 1px solid #c7cdd4; }"
            "QHeaderView::section { background: #eef0f2; color: #202428; border: 1px solid #c7cdd4; }");
        return;
    }
    qApp->setStyleSheet({});
}

void MainWindow::applyBoardDisplaySettings()
{
    const QString blackTexture = QString::fromStdString(engineSettings_.boardDisplay.blackStoneTexture.string());
    const QString whiteTexture = QString::fromStdString(engineSettings_.boardDisplay.whiteStoneTexture.string());
    if (boardItem_ != nullptr) {
        boardItem_->setOwnershipOpacity(engineSettings_.boardDisplay.ownershipOpacity);
        boardItem_->setStoneTextures(blackTexture, whiteTexture);
    }
    if (ownershipBoardItem_ != nullptr) {
        ownershipBoardItem_->setOwnershipOpacity(engineSettings_.boardDisplay.ownershipOpacity);
        ownershipBoardItem_->setStoneTextures(blackTexture, whiteTexture);
    }
    if (ownershipDock_ != nullptr) {
        ownershipDock_->setVisible(
            engineSettings_.boardDisplay.showOwnership &&
            ownershipShownOnMiniBoard(engineSettings_.boardDisplay.ownershipDisplayMode));
    }
}

void MainWindow::retranslateUi()
{
    setWindowTitle(uiText("appTitle"));
    if (newAction_ != nullptr) {
        newAction_->setText(uiText("new"));
    }
    if (openAction_ != nullptr) {
        openAction_->setText(uiText("open"));
    }
    if (saveAction_ != nullptr) {
        saveAction_->setText(uiText("save"));
    }
    if (saveAsAction_ != nullptr) {
        saveAsAction_->setText(uiText("saveAs"));
    }
    if (gameInfoAction_ != nullptr) {
        gameInfoAction_->setText(uiText("gameInfo"));
    }
    if (importJavaConfigAction_ != nullptr) {
        importJavaConfigAction_->setText(uiText("importJavaConfig"));
    }
    if (analyzeAction_ != nullptr) {
        analyzeAction_->setText(uiText("analyze"));
    }
    if (estimateAction_ != nullptr) {
        estimateAction_->setText(uiText("estimate"));
    }
    if (batchAction_ != nullptr) {
        batchAction_->setText(uiText("batch"));
    }
    if (restartEngineAction_ != nullptr) {
        restartEngineAction_->setText(uiText("restartEngine"));
    }
    if (compareAction_ != nullptr) {
        compareAction_->setText(uiText("compare"));
    }
    if (aiMoveAction_ != nullptr) {
        aiMoveAction_->setText(uiText("aiMove"));
    }
    if (humanVsAiAction_ != nullptr) {
        humanVsAiAction_->setText(uiText("humanVsAi"));
    }
    if (selfPlayAction_ != nullptr) {
        selfPlayAction_->setText(uiText("selfPlay"));
    }
    if (engineGameAction_ != nullptr) {
        engineGameAction_->setText(uiText("engineGame"));
    }
    if (previousAction_ != nullptr) {
        previousAction_->setText(uiText("previous"));
    }
    if (nextAction_ != nullptr) {
        nextAction_->setText(uiText("next"));
    }
    if (undoAction_ != nullptr) {
        undoAction_->setText(uiText("undo"));
    }
    if (passAction_ != nullptr) {
        passAction_->setText(uiText("pass"));
    }
    if (resignAction_ != nullptr) {
        resignAction_->setText(uiText("resign"));
    }
    if (promoteVariationAction_ != nullptr) {
        promoteVariationAction_->setText(uiText("promoteVariation"));
    }
    if (deleteBranchAction_ != nullptr) {
        deleteBranchAction_->setText(uiText("deleteBranch"));
    }
    if (playModeAction_ != nullptr) {
        playModeAction_->setText(uiText("playMode"));
    }
    if (tryModeAction_ != nullptr) {
        tryModeAction_->setText(uiText("tryMode"));
    }
    if (labelModeAction_ != nullptr) {
        labelModeAction_->setText(uiText("labelMode"));
    }
    if (setupBlackAction_ != nullptr) {
        setupBlackAction_->setText(uiText("setupBlack"));
    }
    if (setupWhiteAction_ != nullptr) {
        setupWhiteAction_->setText(uiText("setupWhite"));
    }
    if (setupClearAction_ != nullptr) {
        setupClearAction_->setText(uiText("setupClear"));
    }
    if (circleMarkAction_ != nullptr) {
        circleMarkAction_->setText(uiText("circleMark"));
    }
    if (squareMarkAction_ != nullptr) {
        squareMarkAction_->setText(uiText("squareMark"));
    }
    if (triangleMarkAction_ != nullptr) {
        triangleMarkAction_->setText(uiText("triangleMark"));
    }
    if (crossMarkAction_ != nullptr) {
        crossMarkAction_->setText(uiText("crossMark"));
    }
    if (clearMarkupAction_ != nullptr) {
        clearMarkupAction_->setText(uiText("clearMarkup"));
    }
    if (settingsAction_ != nullptr) {
        settingsAction_->setText(uiText("settings"));
    }
    if (fileMenu_ != nullptr) {
        fileMenu_->setTitle(uiText("fileMenu"));
    }
    if (engineMenu_ != nullptr) {
        engineMenu_->setTitle(uiText("engineMenu"));
    }
    if (navigateMenu_ != nullptr) {
        navigateMenu_->setTitle(uiText("navigateMenu"));
    }
    if (markupMenu_ != nullptr) {
        markupMenu_->setTitle(uiText("markupMenu"));
    }
    if (viewMenu_ != nullptr) {
        viewMenu_->setTitle(uiText("viewMenu"));
    }
    applyShortcuts();
    applyActionTooltips();

    if (candidatesDock_ != nullptr) {
        candidatesDock_->setWindowTitle(uiText("candidates"));
        candidatesDock_->toggleViewAction()->setText(uiText("candidates"));
    }
    if (ownershipDock_ != nullptr) {
        ownershipDock_->setWindowTitle(uiText("ownershipBoard"));
        ownershipDock_->toggleViewAction()->setText(uiText("ownershipBoard"));
    }
    if (treeDock_ != nullptr) {
        treeDock_->setWindowTitle(uiText("gameTree"));
        treeDock_->toggleViewAction()->setText(uiText("gameTree"));
    }
    if (commentDock_ != nullptr) {
        commentDock_->setWindowTitle(uiText("comment"));
        commentDock_->toggleViewAction()->setText(uiText("comment"));
    }
    if (graphDock_ != nullptr) {
        graphDock_->setWindowTitle(uiText("graph"));
        graphDock_->toggleViewAction()->setText(uiText("graph"));
    }
    if (graphWidget_ != nullptr) {
        graphWidget_->setChineseText(engineSettings_.appearance.language == LanguageMode::Chinese);
    }
    if (consoleDock_ != nullptr) {
        consoleDock_->setWindowTitle(uiText("console"));
        consoleDock_->toggleViewAction()->setText(uiText("console"));
    }
    if (engineLogDock_ != nullptr) {
        engineLogDock_->setWindowTitle(uiText("engineLog"));
        engineLogDock_->toggleViewAction()->setText(uiText("engineLog"));
    }
    if (consoleWidget_ != nullptr) {
        consoleWidget_->setChineseText(engineSettings_.appearance.language == LanguageMode::Chinese);
    }
    if (compareDock_ != nullptr) {
        compareDock_->setWindowTitle(uiText("compareDock"));
        compareDock_->toggleViewAction()->setText(uiText("compareDock"));
    }
    if (mainToolbar_ != nullptr) {
        mainToolbar_->setWindowTitle(uiText("toolbar"));
        mainToolbar_->toggleViewAction()->setText(uiText("toolbar"));
    }
    if (candidatesTable_ != nullptr) {
        candidatesTable_->setHorizontalHeaderLabels({
            uiText("moveHeader"),
            uiText("visitsHeader"),
            uiText("winrateHeader"),
            uiText("scoreHeader"),
            uiText("stdevHeader"),
            uiText("policyHeader"),
            uiText("pvHeader"),
            uiText("pvVisitsHeader"),
        });
    }
    if (gameTreeWidget_ != nullptr) {
        gameTreeWidget_->setHeaderLabels({uiText("moveHeader"), "id"});
        gameTreeWidget_->hideColumn(1);
    }
    if (compareTable_ != nullptr) {
        compareTable_->setHorizontalHeaderLabels({
            uiText("engineHeader"),
            uiText("moveHeader"),
            uiText("visitsHeader"),
            uiText("blackWinrateHeader"),
            uiText("blackScoreHeader"),
            uiText("stdevHeader"),
            uiText("policyHeader"),
            uiText("pvHeader"),
            uiText("pvVisitsHeader"),
        });
    }
    if (commentEditor_ != nullptr) {
        commentEditor_->setPlaceholderText(uiText("commentPlaceholder"));
    }
    updateGameTree();
    updateCandidates();
    updateCompareTable();
    updateGraph();
}

QString MainWindow::uiText(const char* key) const
{
    return translated(engineSettings_.appearance.language, key);
}

void MainWindow::updateBoard()
{
    updatingUi_ = true;
    std::string error;
    lizzie::core::BoardPosition position = game_.currentPosition(&error);
    boardItem_->setPosition(position);
    boardItem_->clearOverlay();
    if (ownershipBoardItem_ != nullptr) {
        ownershipBoardItem_->setPosition(position);
        ownershipBoardItem_->clearOverlay();
    }
    const lizzie::core::GameNode& current = game_.node(game_.currentId());
    boardItem_->setLabels(current.labels);
    boardItem_->setMarks(current.marks);
    if (ownershipBoardItem_ != nullptr) {
        ownershipBoardItem_->setLabels(current.labels);
        ownershipBoardItem_->setMarks(current.marks);
    }
    if (current.analysis.has_value()) {
        const bool showOwnership =
            engineSettings_.boardDisplay.showOwnership && !current.analysis->ownership.empty();
        boardItem_->setCandidates(current.analysis->candidates);
        boardItem_->setOwnership(current.analysis->ownership);
        boardItem_->setShowOwnership(
            showOwnership && ownershipShownOnMainBoard(engineSettings_.boardDisplay.ownershipDisplayMode));
        if (ownershipBoardItem_ != nullptr) {
            ownershipBoardItem_->setOwnership(current.analysis->ownership);
            ownershipBoardItem_->setShowOwnership(
                showOwnership && ownershipShownOnMiniBoard(engineSettings_.boardDisplay.ownershipDisplayMode));
        }
    }

    updateGameTree();
    updateCommentEditor();
    updateCandidates();
    updateCompareTable();
    updateGraph();
    updatingUi_ = false;

    if (!error.empty()) {
        statusBar()->showMessage(localizedCoreError(error, engineSettings_.appearance.language), 5000);
    }
}

void MainWindow::updateCandidates()
{
    if (candidatesTable_ == nullptr) {
        return;
    }

    const QSignalBlocker blocker(candidatesTable_);
    candidatesTable_->setRowCount(0);
    const lizzie::core::GameNode& current = game_.node(game_.currentId());
    if (!current.analysis.has_value()) {
        previewCandidateMove_.reset();
        refreshCandidatePreview();
        if (!current.analysisError.empty()) {
            candidatesTable_->setRowCount(1);
            candidatesTable_->setItem(0, 0, new QTableWidgetItem(uiText("analysisError")));
            for (int column = 1; column < candidatesTable_->columnCount() - 1; ++column) {
                candidatesTable_->setItem(0, column, new QTableWidgetItem("-"));
            }
            candidatesTable_->setItem(
                0,
                candidatesTable_->columnCount() - 1,
                new QTableWidgetItem(localizedAnalysisError(current.analysisError, engineSettings_.appearance.language)));
        }
        return;
    }

    const auto& candidates = current.analysis->candidates;
    candidatesTable_->setRowCount(static_cast<int>(candidates.size()));
    int previewRow = -1;
    for (int row = 0; row < static_cast<int>(candidates.size()); ++row) {
        const lizzie::core::MoveCandidate& candidate = candidates[static_cast<std::size_t>(row)];
        candidatesTable_->setItem(
            row,
            0,
            new QTableWidgetItem(candidateMoveText(candidate.move, engineSettings_.appearance.language)));
        candidatesTable_->setItem(row, 1, new QTableWidgetItem(QString::number(candidate.visits)));
        candidatesTable_->setItem(row, 2, new QTableWidgetItem(percentageText(candidate.winrate)));
        candidatesTable_->setItem(row, 3, new QTableWidgetItem(QString::number(candidate.scoreMean, 'f', 2)));
        candidatesTable_->setItem(row, 4, new QTableWidgetItem(QString::number(candidate.scoreStdev, 'f', 2)));
        candidatesTable_->setItem(row, 5, new QTableWidgetItem(percentageText(candidate.policy)));
        candidatesTable_->setItem(
            row,
            6,
            new QTableWidgetItem(pvText(candidate.pv, engineSettings_.appearance.language)));
        candidatesTable_->setItem(row, 7, new QTableWidgetItem(pvVisitsText(candidate.pvVisits)));
        if (previewCandidateMove_.has_value() && sameMove(*previewCandidateMove_, candidate.move)) {
            previewRow = row;
        }
    }

    if (previewRow < 0 && !candidates.empty()) {
        previewCandidateMove_ = candidates.front().move;
        previewRow = 0;
    }
    if (previewRow >= 0) {
        candidatesTable_->setCurrentCell(previewRow, 0);
        candidatesTable_->selectRow(previewRow);
    } else {
        candidatesTable_->clearSelection();
    }
    refreshCandidatePreview();
}

void MainWindow::showCandidatePreview(int row)
{
    const lizzie::core::GameNode& current = game_.node(game_.currentId());
    if (!current.analysis.has_value() ||
        row < 0 ||
        row >= static_cast<int>(current.analysis->candidates.size())) {
        previewCandidateMove_.reset();
        refreshCandidatePreview();
        return;
    }

    previewCandidateMove_ = current.analysis->candidates[static_cast<std::size_t>(row)].move;
    refreshCandidatePreview();
}

void MainWindow::refreshCandidatePreview()
{
    if (boardItem_ == nullptr) {
        return;
    }

    const lizzie::core::GameNode& current = game_.node(game_.currentId());
    if (!current.analysis.has_value()) {
        boardItem_->setPreviewMoves({});
        return;
    }

    const auto& candidates = current.analysis->candidates;
    if (candidates.empty()) {
        previewCandidateMove_.reset();
        boardItem_->setPreviewMoves({});
        return;
    }

    const lizzie::core::MoveCandidate* preview = nullptr;
    if (previewCandidateMove_.has_value()) {
        const auto iter = std::ranges::find_if(candidates, [this](const lizzie::core::MoveCandidate& candidate) {
            return sameMove(*previewCandidateMove_, candidate.move);
        });
        if (iter != candidates.end()) {
            preview = &*iter;
        }
    }
    if (preview == nullptr) {
        preview = &candidates.front();
        previewCandidateMove_ = preview->move;
    }

    boardItem_->setPreviewMoves(preview->pv);
}

void MainWindow::updateCompareTable()
{
    if (compareTable_ == nullptr) {
        return;
    }

    const lizzie::core::GameNode& current = game_.node(game_.currentId());
    setCompareRow(compareTable_, 0, uiText("mainEngineRow"), current.analysis, engineSettings_.appearance.language);

    std::optional<lizzie::core::AnalysisSnapshot> visibleCompare;
    if (compareAnalysis_.has_value() && compareAnalysisNode_ == game_.currentId()) {
        visibleCompare = compareAnalysis_;
    }
    setCompareRow(compareTable_, 1, uiText("compareEngineRow"), visibleCompare, engineSettings_.appearance.language);
}

void MainWindow::updateGraph()
{
    if (graphWidget_ != nullptr) {
        graphWidget_->setPoints(graphPoints(), game_.currentId());
    }
}

void MainWindow::updateGameTree()
{
    if (gameTreeWidget_ == nullptr) {
        return;
    }

    QSignalBlocker blocker(gameTreeWidget_);
    gameTreeWidget_->clear();
    appendGameTreeNode(nullptr, game_.rootId());
    gameTreeWidget_->expandAll();

    const QList<QTreeWidgetItem*> items =
        gameTreeWidget_->findItems(QString::number(game_.currentId()), Qt::MatchExactly | Qt::MatchRecursive, 1);
    if (!items.empty()) {
        gameTreeWidget_->setCurrentItem(items.front());
    }
}

void MainWindow::appendGameTreeNode(QTreeWidgetItem* parent, lizzie::core::NodeId nodeId)
{
    const lizzie::core::GameNode& node = game_.node(nodeId);
    QStringList columns;
    columns << nodeSummary(node, engineSettings_.appearance.language) << QString::number(nodeId);
    auto* item = parent == nullptr ? new QTreeWidgetItem(gameTreeWidget_, columns) : new QTreeWidgetItem(parent, columns);
    item->setData(0, Qt::UserRole, QVariant::fromValue<qulonglong>(nodeId));
    if (nodeId == game_.currentId()) {
        QFont font = item->font(0);
        font.setBold(true);
        item->setFont(0, font);
    }
    for (lizzie::core::NodeId child : node.children) {
        appendGameTreeNode(item, child);
    }
}

void MainWindow::updateCommentEditor()
{
    if (commentEditor_ == nullptr) {
        return;
    }
    QSignalBlocker blocker(commentEditor_);
    commentEditor_->setPlainText(QString::fromStdString(game_.node(game_.currentId()).comment));
}

void MainWindow::appendEngineLogLine(const QString& line, const QString& source)
{
    if (engineLogOutput_ == nullptr) {
        return;
    }
    QString text = line;
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    const QString prefix = source.isEmpty()
        ? QString()
        : localizedEngineLogSource(source, engineSettings_.appearance.language) + QStringLiteral(": ");
    const QStringList lines = text.split('\n');
    for (const QString& entry : lines) {
        engineLogOutput_->appendPlainText(prefix + entry);
    }
}

void MainWindow::reportSystemDiagnostic(
    const QString& message,
    const QString& source,
    const QString& consolePrefix,
    int timeoutMs)
{
    if (consoleWidget_ != nullptr) {
        consoleWidget_->appendSystemLine(message, consolePrefix);
    }
    appendEngineLogLine(message, source);
    statusBar()->showMessage(message, timeoutMs);
}

void MainWindow::syncAndStartRealtimeAnalysis()
{
    if (!engines_.mainEngine().isRunning()) {
        return;
    }
    if (!mainCapabilitiesKnown_) {
        pendingAnalysisStart_ = true;
        if (analyzeAction_ != nullptr) {
            analyzeAction_->setChecked(true);
        }
        return;
    }
    if (!engines_.mainCapabilities().kataAnalyze) {
        showWarning(this, engineSettings_.appearance.language, uiText("analyze"), uiText("noAnalyzeSupport"));
        stopRealtimeAnalysis();
        return;
    }

    realtimeStreamGate_.beginSync();
    realtimeAnalysisActive_ = false;
    pendingAnalysisStart_ = true;
    pendingRealtimeAnalyzeCommand_.reset();

    const lizzie::app::EngineCommandPlan plan = lizzie::app::buildRealtimeAnalysisCommandPlan(
        game_,
        game_.currentId(),
        engines_.mainCapabilities(),
        engineSettings_.realtimeOptions);
    const lizzie::app::EngineCommandDispatchResult dispatchResult =
        lizzie::app::dispatchGtpCommands(engines_.mainEngine(), plan.preparationCommands, &realtimeStreamGate_);
    for (const std::string& warning : plan.warnings) {
        const QString warningText = localizedAppWarning(warning, engineSettings_.appearance.language);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(warningText);
        }
        appendEngineLogLine(warningText, QStringLiteral("main"));
    }
    if (plan.fatal) {
        realtimeStreamGate_.stop();
        pendingAnalysisStart_ = false;
        pendingRealtimeAnalyzeCommand_.reset();
        realtimeAnalysisActive_ = false;
        if (analyzeAction_ != nullptr) {
            analyzeAction_->setChecked(false);
        }
        statusBar()->showMessage(uiText("realtimeBoardSyncSkipped"), 5000);
        return;
    }
    if (!dispatchResult.ok()) {
        failRealtimeAnalysisStartup(uiText("realtimeSyncFailedPrefix") + uiText("syncCommandDispatchFailed"));
        return;
    }
    realtimeAccumulator_.reset(game_.boardSize(), plan.sideToMove);
    analysisStore_.clearAt(game_.currentId());
    pendingRealtimeAnalyzeCommand_ = plan.finalCommand;

    if (analyzeAction_ != nullptr) {
        analyzeAction_->setChecked(true);
    }
    updateBoard();
    if (!plan.positionError.empty()) {
        statusBar()->showMessage(localizedCoreError(plan.positionError, engineSettings_.appearance.language), 5000);
    }
}

void MainWindow::failRealtimeAnalysisStartup(const QString& message)
{
    stopRealtimeAnalysis();
    if (consoleWidget_ != nullptr) {
        consoleWidget_->appendSystemLine(message);
    }
    appendEngineLogLine(message, QStringLiteral("main"));
    statusBar()->showMessage(uiText("realtimeSyncSkipped"), 5000);
}

void MainWindow::stopRealtimeAnalysis()
{
    realtimeStreamGate_.stop();
    pendingAnalysisStart_ = false;
    pendingRealtimeAnalyzeCommand_.reset();
    realtimeAnalysisActive_ = false;
    if (engines_.mainEngine().isRunning()) {
        lizzie::app::dispatchStopAnalyze(engines_.mainEngine());
    }
    if (analyzeAction_ != nullptr) {
        analyzeAction_->setChecked(false);
    }
}

void MainWindow::syncAndStartCompareAnalysis()
{
    if (!engines_.compareEngine().isRunning()) {
        return;
    }
    if (!compareCapabilitiesKnown_) {
        pendingCompareStart_ = true;
        if (compareAction_ != nullptr) {
            compareAction_->setChecked(true);
        }
        return;
    }
    if (!engines_.compareCapabilities().kataAnalyze) {
        showWarning(this, engineSettings_.appearance.language, uiText("compare"), uiText("compareNoAnalyzeSupport"));
        stopCompareAnalysis();
        return;
    }

    compareStreamGate_.beginSync();
    compareAnalysisActive_ = false;
    pendingCompareStart_ = true;
    pendingCompareAnalyzeCommand_.reset();

    const lizzie::app::EngineCommandPlan plan = lizzie::app::buildRealtimeAnalysisCommandPlan(
        game_,
        game_.currentId(),
        engines_.compareCapabilities(),
        engineSettings_.realtimeOptions);
    compareAccumulator_.reset(game_.boardSize(), plan.sideToMove);
    compareAnalysis_.reset();
    compareAnalysisNode_ = game_.currentId();

    const lizzie::app::EngineCommandDispatchResult dispatchResult =
        lizzie::app::dispatchGtpCommands(engines_.compareEngine(), plan.preparationCommands, &compareStreamGate_);
    for (const std::string& warning : plan.warnings) {
        const QString warningText = localizedAppWarning(warning, engineSettings_.appearance.language);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(warningText, compareConsolePrefix(engineSettings_.appearance.language));
        }
        appendEngineLogLine(warningText, QStringLiteral("compare"));
    }
    if (plan.fatal) {
        compareStreamGate_.stop();
        pendingCompareStart_ = false;
        pendingCompareAnalyzeCommand_.reset();
        compareAnalysisActive_ = false;
        if (compareAction_ != nullptr) {
            compareAction_->setChecked(false);
        }
        statusBar()->showMessage(uiText("compareBoardSyncSkipped"), 5000);
        return;
    }
    if (!dispatchResult.ok()) {
        failCompareAnalysisStartup(uiText("compareSyncFailedPrefix") + uiText("syncCommandDispatchFailed"));
        return;
    }
    pendingCompareAnalyzeCommand_ = plan.finalCommand;

    if (compareAction_ != nullptr) {
        compareAction_->setChecked(true);
    }
    updateCompareTable();
    if (!plan.positionError.empty()) {
        statusBar()->showMessage(localizedCoreError(plan.positionError, engineSettings_.appearance.language), 5000);
    }
}

void MainWindow::failCompareAnalysisStartup(const QString& message)
{
    stopCompareAnalysis();
    if (consoleWidget_ != nullptr) {
        consoleWidget_->appendSystemLine(message, compareConsolePrefix(engineSettings_.appearance.language));
    }
    appendEngineLogLine(message, QStringLiteral("compare"));
    statusBar()->showMessage(uiText("compareSyncSkipped"), 5000);
}

void MainWindow::stopCompareAnalysis()
{
    compareStreamGate_.stop();
    pendingCompareStart_ = false;
    pendingCompareAnalyzeCommand_.reset();
    compareAnalysisActive_ = false;
    if (engines_.compareEngine().isRunning()) {
        lizzie::app::dispatchStopAnalyze(engines_.compareEngine());
    }
    if (compareAction_ != nullptr) {
        compareAction_->setChecked(false);
    }
}

void MainWindow::syncAndRequestGenMove()
{
    if (!engines_.mainEngine().isRunning()) {
        return;
    }
    if (!mainCapabilitiesKnown_) {
        pendingGenMoveStart_ = true;
        return;
    }
    if (!engines_.mainCapabilities().genmove) {
        showWarning(
            this,
            engineSettings_.appearance.language,
            engineGameActive_ ? uiText("engineGame") : uiText("aiMove"),
            uiText("noGenmoveSupport"));
        if (engineGameActive_) {
            stopEngineGame();
        }
        stopHumanVsAi();
        stopSelfPlay();
        return;
    }
    genMoveGuard_.clear();

    const lizzie::app::EngineCommandPlan plan = lizzie::app::buildGenMoveCommandPlan(
        game_,
        game_.currentId(),
        engines_.mainCapabilities(),
        engineSettings_.realtimeOptions);
    if (!plan.positionError.empty()) {
        statusBar()->showMessage(localizedCoreError(plan.positionError, engineSettings_.appearance.language), 5000);
        if (engineGameActive_) {
            stopEngineGame();
        }
        stopHumanVsAi();
        stopSelfPlay();
        return;
    }

    const lizzie::app::EngineCommandDispatchResult dispatchResult =
        lizzie::app::dispatchGtpCommands(engines_.mainEngine(), plan.preparationCommands);
    for (const std::string& warning : plan.warnings) {
        const QString warningText = localizedAppWarning(warning, engineSettings_.appearance.language);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(warningText);
        }
        appendEngineLogLine(warningText, QStringLiteral("main"));
    }
    if (plan.fatal) {
        const QString statusMessage = mainGenMoveSkippedStatus(true);
        if (engineGameActive_) {
            stopEngineGame();
        }
        stopHumanVsAi();
        stopSelfPlay();
        statusBar()->showMessage(statusMessage, 5000);
        return;
    }
    if (!dispatchResult.ok()) {
        failMainGenMoveRequest(uiText("genmoveSyncFailedPrefix") + uiText("syncCommandDispatchFailed"));
        return;
    }
    genMoveGuard_.start(plan.positionKey);
    if (!lizzie::app::dispatchGtpCommand(engines_.mainEngine(), plan.finalCommand)) {
        failMainGenMoveRequest(uiText("genmoveSyncFailedPrefix") + uiText("genmoveCommandDispatchFailed"));
    }
}

QString MainWindow::mainGenMoveSkippedStatus(bool boardSync) const
{
    if (engineGameActive_) {
        return uiText(boardSync ? "engineGameMainMoveBoardSyncSkipped" : "engineGameMainMoveSyncSkipped");
    }
    if (selfPlayActive_) {
        return uiText(boardSync ? "selfPlayMoveBoardSyncSkipped" : "selfPlayMoveSyncSkipped");
    }
    if (humanVsAiActive_) {
        return uiText(boardSync ? "humanVsAiReplyBoardSyncSkipped" : "humanVsAiReplySyncSkipped");
    }
    return uiText(boardSync ? "aiMoveBoardSyncSkipped" : "aiMoveSyncSkipped");
}

void MainWindow::failMainGenMoveRequest(const QString& message)
{
    const QString statusMessage = mainGenMoveSkippedStatus(false);
    pendingGenMoveStart_ = false;
    genMoveGuard_.clear();
    if (consoleWidget_ != nullptr) {
        consoleWidget_->appendSystemLine(message);
    }
    appendEngineLogLine(message, QStringLiteral("main"));
    if (engineGameActive_) {
        stopEngineGame();
    }
    stopHumanVsAi();
    stopSelfPlay();
    statusBar()->showMessage(statusMessage, 5000);
}

void MainWindow::syncAndRequestCompareGenMove()
{
    if (!engines_.compareEngine().isRunning()) {
        return;
    }
    if (!compareCapabilitiesKnown_) {
        pendingCompareGenMoveStart_ = true;
        return;
    }
    if (!engines_.compareCapabilities().genmove) {
        showWarning(this, engineSettings_.appearance.language, uiText("engineGame"), uiText("compareNoGenmoveSupport"));
        pendingCompareGenMoveStart_ = false;
        compareGenMoveGuard_.clear();
        stopEngineGame();
        return;
    }
    compareGenMoveGuard_.clear();

    const lizzie::app::EngineCommandPlan plan = lizzie::app::buildGenMoveCommandPlan(
        game_,
        game_.currentId(),
        engines_.compareCapabilities(),
        engineSettings_.realtimeOptions);
    if (!plan.positionError.empty()) {
        statusBar()->showMessage(localizedCoreError(plan.positionError, engineSettings_.appearance.language), 5000);
        pendingCompareGenMoveStart_ = false;
        compareGenMoveGuard_.clear();
        if (engineGameActive_) {
            stopEngineGame();
        }
        return;
    }

    const lizzie::app::EngineCommandDispatchResult dispatchResult =
        lizzie::app::dispatchGtpCommands(engines_.compareEngine(), plan.preparationCommands);
    for (const std::string& warning : plan.warnings) {
        const QString warningText = localizedAppWarning(warning, engineSettings_.appearance.language);
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(warningText, compareConsolePrefix(engineSettings_.appearance.language));
        }
        appendEngineLogLine(warningText, QStringLiteral("compare"));
    }
    if (plan.fatal) {
        pendingCompareGenMoveStart_ = false;
        compareGenMoveGuard_.clear();
        if (engineGameActive_) {
            stopEngineGame();
        }
        statusBar()->showMessage(uiText("compareMoveBoardSyncSkipped"), 5000);
        return;
    }
    if (!dispatchResult.ok()) {
        failCompareGenMoveRequest(
            uiText("compareGenmoveSyncFailedPrefix") + uiText("syncCommandDispatchFailed"));
        return;
    }
    compareGenMoveGuard_.start(plan.positionKey);
    if (!lizzie::app::dispatchGtpCommand(engines_.compareEngine(), plan.finalCommand)) {
        failCompareGenMoveRequest(
            uiText("compareGenmoveSyncFailedPrefix") + uiText("genmoveCommandDispatchFailed"));
    }
}

void MainWindow::failCompareGenMoveRequest(const QString& message)
{
    pendingCompareGenMoveStart_ = false;
    compareGenMoveGuard_.clear();
    if (consoleWidget_ != nullptr) {
        consoleWidget_->appendSystemLine(message, compareConsolePrefix(engineSettings_.appearance.language));
    }
    appendEngineLogLine(message, QStringLiteral("compare"));
    if (engineGameActive_) {
        stopEngineGame();
    }
    statusBar()->showMessage(uiText("compareMoveSyncSkipped"), 5000);
}

void MainWindow::requestEngineGameMove()
{
    if (!engineGameActive_) {
        return;
    }

    std::string error;
    const lizzie::core::BoardPosition position = game_.currentPosition(&error);
    if (!error.empty()) {
        statusBar()->showMessage(localizedCoreError(error, engineSettings_.appearance.language), 5000);
        stopEngineGame();
        return;
    }

    if (position.sideToMove() == lizzie::core::Color::Black) {
        pendingGenMoveStart_ = true;
        if (engines_.mainEngine().isRunning()) {
            syncAndRequestGenMove();
        } else {
            mainCapabilitiesKnown_ = false;
            engines_.resetMainCapabilities();
            engines_.mainEngine().startGtp(engineSettings_.config);
        }
        return;
    }

    pendingCompareGenMoveStart_ = true;
    if (engines_.compareEngine().isRunning()) {
        syncAndRequestCompareGenMove();
    } else {
        compareCapabilitiesKnown_ = false;
        engines_.resetCompareCapabilities();
        engines_.compareEngine().startGtp(resolvedComparisonGtpConfig(engineSettings_));
    }
}

void MainWindow::cancelBatchAnalysisForGameChange()
{
    if (engines_.batchAnalysis().isRunning()) {
        engines_.batchAnalysis().cancelAndWait();
    }
    batchTracker_.clear();
    batchRunFailed_ = false;
    analysisRunMode_ = AnalysisRunMode::None;
    if (batchProgress_ != nullptr) {
        batchProgress_->close();
        batchProgress_->deleteLater();
        batchProgress_ = nullptr;
    }
    if (batchAction_ != nullptr) {
        batchAction_->setEnabled(true);
    }
    if (estimateAction_ != nullptr) {
        estimateAction_->setEnabled(true);
    }
}

void MainWindow::stopAutomatedPlayModes()
{
    stopEngineGame();
    stopHumanVsAi();
    stopSelfPlay();
    pendingCompareGenMoveStart_ = false;
    compareGenMoveGuard_.clear();
}

void MainWindow::stopSelfPlay()
{
    selfPlayActive_ = false;
    pendingGenMoveStart_ = false;
    genMoveGuard_.clear();
    if (selfPlayAction_ != nullptr) {
        selfPlayAction_->setChecked(false);
    }
}

void MainWindow::stopEngineGame()
{
    engineGameActive_ = false;
    pendingCompareGenMoveStart_ = false;
    compareGenMoveGuard_.clear();
    if (!selfPlayActive_) {
        pendingGenMoveStart_ = false;
        genMoveGuard_.clear();
    }
    if (engineGameAction_ != nullptr) {
        engineGameAction_->setChecked(false);
    }
}

void MainWindow::stopHumanVsAi()
{
    humanVsAiActive_ = false;
    if (!selfPlayActive_ && !engineGameActive_) {
        pendingGenMoveStart_ = false;
        genMoveGuard_.clear();
    }
    if (humanVsAiAction_ != nullptr) {
        humanVsAiAction_->setChecked(false);
    }
}

bool MainWindow::applyEngineMove(const lizzie::core::Move& move, const QString& source, const QString& consolePrefix)
{
    const lizzie::app::MoveEditResult result = gameEditor_.tryAddMove(move);
    if (!result.accepted) {
        const QString errorText = localizedCoreError(result.error, engineSettings_.appearance.language);
        const QString message =
            uiText("engineMoveRejectedPrefix") +
            candidateMoveText(move, engineSettings_.appearance.language) + ": " + errorText;
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message, consolePrefix);
        }
        appendEngineLogLine(message, source);
        statusBar()->showMessage(errorText, 5000);
        stopSelfPlay();
        if (engineGameActive_) {
            stopEngineGame();
        }
        stopHumanVsAi();
        return false;
    }
    cancelBatchAnalysisForGameChange();
    markAnalysisSidecarSyncPending();
    updateBoard();
    if (move.type == lizzie::core::MoveType::Resign) {
        if (realtimeAnalysisRequested()) {
            stopRealtimeAnalysis();
        }
        if (compareAnalysisRequested()) {
            stopCompareAnalysis();
        }
    } else {
        if (realtimeAnalysisRequested()) {
            syncAndStartRealtimeAnalysis();
        }
        if (compareAnalysisRequested()) {
            syncAndStartCompareAnalysis();
        }
    }
    statusBar()->showMessage(
        uiText("engineMovePrefix") + candidateMoveText(move, engineSettings_.appearance.language),
        3000);
    return true;
}

void MainWindow::markAnalysisSidecarSyncPending()
{
    if (appliedAnalysisSidecarForDocument_) {
        analysisSidecarSyncPending_ = true;
    }
}

std::vector<AnalysisGraphPoint> MainWindow::graphPoints() const
{
    std::vector<AnalysisGraphPoint> points;
    const std::vector<lizzie::core::NodeId> path = game_.pathTo(game_.currentId());
    int displayMoveNumber = 0;
    for (lizzie::core::NodeId nodeId : path) {
        const lizzie::core::GameNode& node = game_.node(nodeId);
        if (node.move.has_value()) {
            ++displayMoveNumber;
            if (node.moveNumber.has_value()) {
                displayMoveNumber = *node.moveNumber;
            }
        }
        if (!node.analysis.has_value()) {
            continue;
        }
        points.push_back(AnalysisGraphPoint{
            nodeId,
            displayMoveNumber,
            blackPerspectiveWinrate(*node.analysis),
            blackPerspectiveScoreMean(*node.analysis),
            node.analysis->rootVisits,
            node.move.has_value() ? std::optional<lizzie::core::Color>(node.move->color) : std::nullopt,
        });
    }
    return points;
}

int MainWindow::loadAnalysisSidecar(const QString& sgfPath)
{
    const QString sidecarPath = sgfPath + ".analysis.json";
    QFile file(sidecarPath);
    if (!file.exists()) {
        return 0;
    }
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(uiText("sidecarReadFailedPrefix") + file.errorString());
        }
        return 0;
    }

    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(uiText("sidecarParseFailedPrefix") + error.errorString());
        }
        return 0;
    }

    std::vector<std::string> warnings;
    const int applied =
        lizzie::engine::applyAnalysisSidecarJson(
            &game_,
            document.object(),
            &warnings,
            lizzie::app::analysisSidecarCollectionIndex(documentSession_));
    for (const std::string& warning : warnings) {
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(localizedAppWarning(warning, engineSettings_.appearance.language));
        }
    }
    if (applied > 0) {
        statusBar()->showMessage(uiText("loadedAnalysisSidecar").arg(applied), 5000);
    }
    return applied;
}

bool MainWindow::writeAnalysisSidecar(
    const QString& path,
    std::optional<QString> sgfPathOverride,
    QString* errorMessage) const
{
    lizzie::app::GameDocumentSession sidecarSession = documentSession_;
    if (sgfPathOverride.has_value()) {
        sidecarSession.currentFile = *sgfPathOverride;
    }
    const QJsonObject root = lizzie::app::analysisSidecarJsonForSession(game_, sidecarSession);
    const QByteArray json = QJsonDocument(root).toJson(QJsonDocument::Indented);
    QString error;
    if (!writeFileAtomically(path, json, &error)) {
        const QString message = uiText("sidecarWriteFailedPrefix") + error;
        if (errorMessage != nullptr) {
            *errorMessage = message;
        }
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message);
        }
        return false;
    }
    return true;
}

bool MainWindow::saveSgfTo(const QString& path, bool interactiveError, bool syncAppliedSidecar)
{
    const std::string sgf = lizzie::app::serializeSgfForSession(game_, documentSession_);
    QString error;
    if (!writeFileAtomically(path, QByteArray::fromStdString(sgf), &error)) {
        const QString message = uiText("sgfWriteFailedPrefix") + error;
        if (consoleWidget_ != nullptr) {
            consoleWidget_->appendSystemLine(message);
        }
        if (interactiveError) {
            showWarning(this, engineSettings_.appearance.language, uiText("saveSgfTitle"), message);
        }
        return false;
    }
    const bool savingAsDifferentPath =
        documentSession_.currentFile.has_value() && *documentSession_.currentFile != path;
    const bool shouldWriteSidecar =
        syncAppliedSidecar &&
        (analysisSidecarSyncPending_ || (appliedAnalysisSidecarForDocument_ && savingAsDifferentPath));
    QString sidecarError;
    if (shouldWriteSidecar && !writeAnalysisSidecar(path + QStringLiteral(".analysis.json"), path, &sidecarError)) {
        if (sidecarError.isEmpty()) {
            sidecarError = uiText("sidecarWriteFailedPrefix") + path + QStringLiteral(".analysis.json");
        }
        statusBar()->showMessage(sidecarError, 5000);
        if (interactiveError) {
            showWarning(this, engineSettings_.appearance.language, uiText("saveSgfTitle"), sidecarError);
        }
        return false;
    }
    if (shouldWriteSidecar) {
        analysisSidecarSyncPending_ = false;
        appliedAnalysisSidecarForDocument_ = true;
    }
    statusBar()->showMessage(path, 3000);
    return true;
}

bool MainWindow::realtimeAnalysisRequested() const
{
    return realtimeAnalysisActive_ || pendingAnalysisStart_;
}

bool MainWindow::compareAnalysisRequested() const
{
    return compareAnalysisActive_ || pendingCompareStart_;
}

}  // namespace lizzie::ui

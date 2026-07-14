#include "AnalysisGraphWidget.h"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QToolTip>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string_view>

namespace lizzie::ui {

namespace {

QString graphText(bool zh, const char* name)
{
    const std::string_view key(name);
    if (key == "noAnalysis") {
        return zh ? QString::fromUtf8("无分析") : QStringLiteral("No analysis");
    }
    if (key == "move") {
        return zh ? QString::fromUtf8("手数") : QStringLiteral("Move");
    }
    if (key == "winrate") {
        return zh ? QString::fromUtf8("黑胜率") : QStringLiteral("Black winrate");
    }
    if (key == "score") {
        return zh ? QString::fromUtf8("黑目差") : QStringLiteral("Black score");
    }
    if (key == "visits") {
        return zh ? QString::fromUtf8("访问") : QStringLiteral("Visits");
    }
    if (key == "loss") {
        return zh ? QString::fromUtf8("损失") : QStringLiteral("Loss");
    }
    return QString::fromLatin1(name);
}

QColor graphColor(bool dark, const char* name)
{
    const std::string_view key(name);
    if (dark) {
        if (key == "background") {
            return QColor(25, 27, 30);
        }
        if (key == "border") {
            return QColor(76, 80, 86);
        }
        if (key == "text") {
            return QColor(180, 184, 190);
        }
        if (key == "hover") {
            return QColor(244, 246, 248);
        }
    } else {
        if (key == "background") {
            return QColor(247, 248, 250);
        }
        if (key == "border") {
            return QColor(188, 196, 204);
        }
        if (key == "text") {
            return QColor(68, 76, 86);
        }
        if (key == "hover") {
            return QColor(32, 38, 45);
        }
    }
    return QColor();
}

}  // namespace

AnalysisGraphWidget::AnalysisGraphWidget(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(128);
    setMouseTracking(true);
}

void AnalysisGraphWidget::setPoints(std::vector<AnalysisGraphPoint> points, lizzie::core::NodeId currentNode)
{
    points_ = std::move(points);
    currentNode_ = currentNode;
    update();
}

void AnalysisGraphWidget::setChineseText(bool enabled)
{
    if (chineseText_ == enabled) {
        return;
    }
    chineseText_ = enabled;
    QToolTip::hideText();
    update();
}

void AnalysisGraphWidget::setDarkTheme(bool enabled)
{
    if (darkTheme_ == enabled) {
        return;
    }
    darkTheme_ = enabled;
    update();
}

void AnalysisGraphWidget::paintEvent(QPaintEvent*)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), graphColor(darkTheme_, "background"));

    const QRectF plot = plotRect();
    painter.setPen(QPen(graphColor(darkTheme_, "border"), 1.0));
    painter.drawRect(plot);
    painter.drawLine(QPointF(plot.left(), plot.center().y()), QPointF(plot.right(), plot.center().y()));

    if (points_.empty()) {
        painter.setPen(graphColor(darkTheme_, "text"));
        painter.drawText(plot, Qt::AlignCenter, graphText(chineseText_, "noAnalysis"));
        return;
    }

    int maxVisits = 0;
    for (const AnalysisGraphPoint& point : points_) {
        maxVisits = std::max(maxVisits, point.visits);
    }
    if (maxVisits > 0) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(94, 102, 116, 150));
        QPainterPath visitsPath;
        for (const AnalysisGraphPoint& point : points_) {
            const QPointF pos = pointPosition(point, plot);
            const qreal height = plot.height() * 0.18 * (static_cast<qreal>(point.visits) / maxVisits);
            painter.drawRect(QRectF(pos.x() - 2.5, plot.bottom() - height, 5.0, height));
            const QPointF visitsPos = visitsPosition(point, plot, maxVisits);
            if (visitsPath.isEmpty()) {
                visitsPath.moveTo(visitsPos);
            } else {
                visitsPath.lineTo(visitsPos);
            }
        }
        QPen visitsPen(QColor(238, 169, 66), 2.2);
        painter.setPen(visitsPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(visitsPath);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(207, 76, 74, 170));
    for (std::size_t index = 1; index < points_.size(); ++index) {
        const double loss = winrateLossAt(index);
        if (loss <= 0.005) {
            continue;
        }
        const QPointF pos = pointPosition(points_[index], plot);
        const qreal height = plot.height() * std::min(loss, 0.5) * 0.8;
        painter.drawRect(QRectF(pos.x() - 3.5, plot.top(), 7.0, height));
    }

    QPainterPath winratePath;
    QPainterPath scorePath;
    for (std::size_t index = 0; index < points_.size(); ++index) {
        const QPointF pos = pointPosition(points_[index], plot);
        const QPointF scorePos = scorePosition(points_[index], plot);
        if (index == 0) {
            winratePath.moveTo(pos);
            scorePath.moveTo(scorePos);
        } else {
            winratePath.lineTo(pos);
            scorePath.lineTo(scorePos);
        }
    }
    QPen scorePen(QColor(111, 149, 221), 1.6);
    scorePen.setStyle(Qt::DashLine);
    painter.setPen(scorePen);
    painter.drawPath(scorePath);

    painter.setPen(QPen(QColor(69, 179, 157), 2.0));
    painter.drawPath(winratePath);

    painter.setPen(Qt::NoPen);
    for (const AnalysisGraphPoint& point : points_) {
        const QPointF pos = pointPosition(point, plot);
        painter.setBrush(point.nodeId == currentNode_ ? QColor(255, 210, 92) : QColor(69, 179, 157));
        painter.drawEllipse(pos, 4.0, 4.0);
    }

    if (hoveredIndex_ >= 0 && hoveredIndex_ < static_cast<int>(points_.size())) {
        const QPointF pos = pointPosition(points_[static_cast<std::size_t>(hoveredIndex_)], plot);
        QColor hoverLine = graphColor(darkTheme_, "hover");
        hoverLine.setAlpha(180);
        painter.setPen(QPen(hoverLine, 1.0));
        painter.drawLine(QPointF(pos.x(), plot.top()), QPointF(pos.x(), plot.bottom()));
        painter.setBrush(graphColor(darkTheme_, "hover"));
        painter.drawEllipse(pos, 5.5, 5.5);
    }
}

void AnalysisGraphWidget::mousePressEvent(QMouseEvent* event)
{
    const int index = nearestPointIndex(event->position());
    if (index >= 0) {
        emit nodeClicked(points_[static_cast<std::size_t>(index)].nodeId);
    }
}

void AnalysisGraphWidget::mouseMoveEvent(QMouseEvent* event)
{
    const int index = nearestPointIndex(event->position());
    if (index != hoveredIndex_) {
        hoveredIndex_ = index;
        update();
    }
    if (index < 0) {
        QToolTip::hideText();
        return;
    }

    const AnalysisGraphPoint& point = points_[static_cast<std::size_t>(index)];
    const double loss = winrateLossAt(static_cast<std::size_t>(index));
    const QString text = QString("%1 %2\n%3 %4%\n%5 %6\n%7 %8\n%9 %10%")
                             .arg(graphText(chineseText_, "move"))
                             .arg(point.moveNumber)
                             .arg(graphText(chineseText_, "winrate"))
                             .arg(QString::number(point.winrate * 100.0, 'f', 1))
                             .arg(graphText(chineseText_, "score"))
                             .arg(QString::number(point.scoreMean, 'f', 2))
                             .arg(graphText(chineseText_, "visits"))
                             .arg(point.visits)
                             .arg(graphText(chineseText_, "loss"))
                             .arg(QString::number(loss * 100.0, 'f', 1));
    QToolTip::showText(event->globalPosition().toPoint(), text, this);
}

void AnalysisGraphWidget::leaveEvent(QEvent*)
{
    hoveredIndex_ = -1;
    QToolTip::hideText();
    update();
}

QRectF AnalysisGraphWidget::plotRect() const
{
    return QRectF(rect()).adjusted(16.0, 12.0, -16.0, -16.0);
}

QPointF AnalysisGraphWidget::pointPosition(const AnalysisGraphPoint& point, const QRectF& rect) const
{
    const double maxMove = std::max(1, points_.empty() ? 1 : points_.back().moveNumber);
    const double x = rect.left() + rect.width() * (point.moveNumber / maxMove);
    const double clampedWinrate = std::clamp(point.winrate, 0.0, 1.0);
    const double y = rect.bottom() - rect.height() * clampedWinrate;
    return QPointF(x, y);
}

QPointF AnalysisGraphWidget::scorePosition(const AnalysisGraphPoint& point, const QRectF& rect) const
{
    const double maxMove = std::max(1, points_.empty() ? 1 : points_.back().moveNumber);
    const double x = rect.left() + rect.width() * (point.moveNumber / maxMove);
    const double normalizedScore = 0.5 + std::clamp(point.scoreMean, -20.0, 20.0) / 40.0;
    const double y = rect.bottom() - rect.height() * normalizedScore;
    return QPointF(x, y);
}

QPointF AnalysisGraphWidget::visitsPosition(const AnalysisGraphPoint& point, const QRectF& rect, int maxVisits) const
{
    const double maxMove = std::max(1, points_.empty() ? 1 : points_.back().moveNumber);
    const double x = rect.left() + rect.width() * (point.moveNumber / maxMove);
    const double normalizedVisits = maxVisits <= 0
        ? 0.0
        : std::log1p(std::max(0, point.visits)) / std::log1p(maxVisits);
    const double y = rect.bottom() - rect.height() * std::clamp(normalizedVisits, 0.0, 1.0);
    return QPointF(x, y);
}

double AnalysisGraphWidget::winrateLossAt(std::size_t index) const
{
    if (index == 0 || index >= points_.size()) {
        return 0.0;
    }
    const double previous = std::clamp(points_[index - 1].winrate, 0.0, 1.0);
    const double current = std::clamp(points_[index].winrate, 0.0, 1.0);
    const std::optional<lizzie::core::Color> moveColor = points_[index].moveColor;
    if (!moveColor.has_value() || *moveColor == lizzie::core::Color::Black) {
        return std::max(0.0, previous - current);
    }
    return std::max(0.0, current - previous);
}

int AnalysisGraphWidget::nearestPointIndex(QPointF pos) const
{
    if (points_.empty()) {
        return -1;
    }

    const QRectF plot = plotRect();
    int bestIndex = -1;
    double bestDistance = 12.0;
    int bestPlotIndex = -1;
    double bestPlotXDistance = std::numeric_limits<double>::max();
    for (std::size_t index = 0; index < points_.size(); ++index) {
        const QPointF point = pointPosition(points_[index], plot);
        if (plot.contains(pos)) {
            const double xDistance = std::abs(point.x() - pos.x());
            if (xDistance < bestPlotXDistance) {
                bestPlotXDistance = xDistance;
                bestPlotIndex = static_cast<int>(index);
            }
        }

        const double distance = std::hypot(point.x() - pos.x(), point.y() - pos.y());
        if (distance < bestDistance) {
            bestDistance = distance;
            bestIndex = static_cast<int>(index);
        }
    }
    if (bestPlotIndex >= 0) {
        return bestPlotIndex;
    }
    return bestIndex;
}

}  // namespace lizzie::ui

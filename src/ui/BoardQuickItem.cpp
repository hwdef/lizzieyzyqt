#include "BoardQuickItem.h"

#include <QFontMetricsF>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QRadialGradient>

#include <algorithm>
#include <cmath>
#include <optional>

namespace lizzie::ui {

namespace {

constexpr int kMaxBoardCandidateMarkers = 10;

QColor ownershipColor(double value, qreal opacity)
{
    const double clamped = std::clamp(value, -1.0, 1.0);
    QColor color = clamped >= 0.0 ? QColor(55, 92, 168) : QColor(214, 226, 238);
    color.setAlphaF(std::abs(clamped) * opacity);
    return color;
}

bool pointOnBoard(lizzie::core::Point point, lizzie::core::BoardSize boardSize)
{
    return point.x >= 0 && point.y >= 0 && point.x < boardSize.width && point.y < boardSize.height;
}

QString compactVisitCount(int visits)
{
    if (visits < 1000) {
        return QString::number(visits);
    }

    const auto compact = [](double value, QLatin1String suffix) {
        QString text = QString::number(value, 'f', value < 10.0 ? 1 : 0);
        if (text.endsWith(QLatin1String(".0"))) {
            text.chop(2);
        }
        return text + suffix;
    };

    if (visits < 1000000) {
        return compact(static_cast<double>(visits) / 1000.0, QLatin1String("k"));
    }
    return compact(static_cast<double>(visits) / 1000000.0, QLatin1String("M"));
}

QString candidateLabel(const lizzie::core::MoveCandidate& candidate)
{
    if (candidate.visits > 0) {
        return compactVisitCount(candidate.visits);
    }
    if (candidate.winrate > 0.0) {
        return QString::number(candidate.winrate * 100.0, 'f', 1);
    }
    return {};
}

void fitFontToBounds(QFont& font, const QString& text, const QSizeF& bounds)
{
    if (text.isEmpty()) {
        return;
    }

    QFontMetricsF metrics(font);
    while ((metrics.horizontalAdvance(text) > bounds.width() || metrics.height() > bounds.height()) &&
           font.pointSizeF() > 5.0) {
        font.setPointSizeF(std::max<qreal>(5.0, font.pointSizeF() - 0.5));
        metrics = QFontMetricsF(font);
    }
}

QColor markupColor(
    lizzie::core::Point point,
    lizzie::core::BoardSize boardSize,
    const std::vector<lizzie::core::Stone>& stones)
{
    const std::size_t index = static_cast<std::size_t>(point.y * boardSize.width + point.x);
    if (index < stones.size() && stones[index] == lizzie::core::Stone::Black) {
        return QColor(245, 247, 248);
    }
    return QColor(28, 32, 35);
}

}  // namespace

BoardQuickItem::BoardQuickItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setAntialiasing(true);
    setOpaquePainting(false);
    setAcceptedMouseButtons(Qt::LeftButton);
    candidatePulseTimer_.setInterval(80);
    connect(&candidatePulseTimer_, &QTimer::timeout, this, [this] {
        candidatePulsePhase_ = std::fmod(candidatePulsePhase_ + 0.32, 6.28318530717958647692);
        update();
    });
    stones_.assign(static_cast<std::size_t>(boardSize_.pointCount()), lizzie::core::Stone::Empty);
}

int BoardQuickItem::boardWidth() const
{
    return boardSize_.width;
}

int BoardQuickItem::boardHeight() const
{
    return boardSize_.height;
}

bool BoardQuickItem::showOwnership() const
{
    return showOwnership_;
}

qreal BoardQuickItem::ownershipOpacity() const
{
    return ownershipOpacity_;
}

void BoardQuickItem::setShowOwnership(bool value)
{
    if (showOwnership_ == value) {
        return;
    }
    showOwnership_ = value;
    emit showOwnershipChanged();
    update();
}

void BoardQuickItem::setOwnershipOpacity(qreal value)
{
    const qreal clamped = std::clamp(value, 0.0, 1.0);
    if (qFuzzyCompare(ownershipOpacity_, clamped)) {
        return;
    }
    ownershipOpacity_ = clamped;
    emit ownershipOpacityChanged();
    update();
}

void BoardQuickItem::setPosition(const lizzie::core::BoardPosition& position)
{
    const lizzie::core::BoardSize previousSize = boardSize_;
    boardSize_ = position.size();
    stones_.clear();
    stones_.reserve(static_cast<std::size_t>(boardSize_.pointCount()));
    for (int y = 0; y < boardSize_.height; ++y) {
        for (int x = 0; x < boardSize_.width; ++x) {
            stones_.push_back(position.at(lizzie::core::Point{x, y}));
        }
    }

    if (previousSize.width != boardSize_.width || previousSize.height != boardSize_.height) {
        emit boardSizeChanged();
    }
    updateCandidateAnimation();
    update();
}

void BoardQuickItem::setCandidates(const std::vector<lizzie::core::MoveCandidate>& candidates)
{
    candidates_ = candidates;
    updateCandidateAnimation();
    update();
}

void BoardQuickItem::setOwnership(std::vector<double> ownership)
{
    ownership_ = std::move(ownership);
    update();
}

void BoardQuickItem::setStoneTextures(const QString& blackPath, const QString& whitePath)
{
    blackStoneTexture_ = blackPath.isEmpty() ? QImage{} : QImage(blackPath);
    whiteStoneTexture_ = whitePath.isEmpty() ? QImage{} : QImage(whitePath);
    update();
}

void BoardQuickItem::setPreviewMoves(std::vector<lizzie::core::Move> moves)
{
    previewMoves_ = std::move(moves);
    update();
}

void BoardQuickItem::setLabels(const std::vector<lizzie::core::BoardLabel>& labels)
{
    labels_ = labels;
    update();
}

void BoardQuickItem::setMarks(const std::vector<lizzie::core::BoardMark>& marks)
{
    marks_ = marks;
    update();
}

void BoardQuickItem::clearOverlay()
{
    candidates_.clear();
    ownership_.clear();
    previewMoves_.clear();
    updateCandidateAnimation();
    setShowOwnership(false);
    update();
}

int BoardQuickItem::previewMoveCount() const
{
    return static_cast<int>(previewMoves_.size());
}

void BoardQuickItem::paint(QPainter* painter)
{
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);

    const QRectF grid = gridRect();
    paintBackground(painter, grid);
    paintGrid(painter, grid);
    paintStarPoints(painter, grid);
    paintOwnership(painter, grid);
    paintStones(painter, grid);
    paintMarkup(painter, grid);
    paintPreview(painter, grid);
    paintCandidates(painter, grid);
}

void BoardQuickItem::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        event->ignore();
        return;
    }
    const std::optional<lizzie::core::Point> point = pointAt(event->position(), gridRect());
    if (!point.has_value()) {
        event->ignore();
        return;
    }
    emit pointClicked(point->x, point->y);
    event->accept();
}

QRectF BoardQuickItem::gridRect() const
{
    const QRectF area = boundingRect().adjusted(18.0, 18.0, -18.0, -18.0);
    const int columns = std::max(1, boardSize_.width - 1);
    const int rows = std::max(1, boardSize_.height - 1);
    const qreal cell = std::min(area.width() / columns, area.height() / rows);
    const QSizeF gridSize(cell * columns, cell * rows);
    return QRectF(
        QPointF(area.center().x() - gridSize.width() / 2.0, area.center().y() - gridSize.height() / 2.0),
        gridSize);
}

QPointF BoardQuickItem::pointCenter(lizzie::core::Point point, const QRectF& grid) const
{
    return QPointF(
        grid.left() + point.x * cellSize(grid),
        grid.top() + point.y * cellSize(grid));
}

std::optional<lizzie::core::Point> BoardQuickItem::pointAt(QPointF position, const QRectF& grid) const
{
    const qreal cell = cellSize(grid);
    if (cell <= 0.0) {
        return std::nullopt;
    }
    const int x = static_cast<int>(std::llround((position.x() - grid.left()) / cell));
    const int y = static_cast<int>(std::llround((position.y() - grid.top()) / cell));
    lizzie::core::Point point{x, y};
    if (x < 0 || y < 0 || x >= boardSize_.width || y >= boardSize_.height) {
        return std::nullopt;
    }
    const QPointF center = pointCenter(point, grid);
    if (std::hypot(center.x() - position.x(), center.y() - position.y()) > cell * 0.45) {
        return std::nullopt;
    }
    return point;
}

qreal BoardQuickItem::cellSize(const QRectF& grid) const
{
    const int columns = std::max(1, boardSize_.width - 1);
    return grid.width() / columns;
}

void BoardQuickItem::paintBackground(QPainter* painter, const QRectF& grid)
{
    const qreal margin = cellSize(grid) * 0.65;
    const QRectF board = grid.adjusted(-margin, -margin, margin, margin);
    QLinearGradient gradient(board.topLeft(), board.bottomRight());
    gradient.setColorAt(0.0, QColor(226, 184, 111));
    gradient.setColorAt(1.0, QColor(191, 139, 73));
    painter->setPen(Qt::NoPen);
    painter->setBrush(gradient);
    painter->drawRoundedRect(board, 6.0, 6.0);
}

void BoardQuickItem::paintGrid(QPainter* painter, const QRectF& grid)
{
    QPen pen(QColor(58, 38, 21), std::max<qreal>(1.0, cellSize(grid) / 80.0));
    painter->setPen(pen);
    for (int x = 0; x < boardSize_.width; ++x) {
        const qreal px = grid.left() + x * cellSize(grid);
        painter->drawLine(QPointF(px, grid.top()), QPointF(px, grid.bottom()));
    }
    for (int y = 0; y < boardSize_.height; ++y) {
        const qreal py = grid.top() + y * cellSize(grid);
        painter->drawLine(QPointF(grid.left(), py), QPointF(grid.right(), py));
    }
}

void BoardQuickItem::paintStarPoints(QPainter* painter, const QRectF& grid)
{
    if (!boardSize_.isSquare()) {
        return;
    }

    std::vector<int> points;
    if (boardSize_.width >= 15) {
        points = {3, boardSize_.width / 2, boardSize_.width - 4};
    } else if (boardSize_.width >= 11) {
        points = {3, boardSize_.width / 2, boardSize_.width - 4};
    } else if (boardSize_.width >= 7) {
        points = {2, boardSize_.width / 2, boardSize_.width - 3};
    }
    if (points.empty()) {
        return;
    }

    painter->setPen(Qt::NoPen);
    painter->setBrush(QColor(58, 38, 21));
    const qreal radius = std::max<qreal>(2.0, cellSize(grid) * 0.065);
    for (int y : points) {
        for (int x : points) {
            painter->drawEllipse(pointCenter(lizzie::core::Point{x, y}, grid), radius, radius);
        }
    }
}

void BoardQuickItem::paintOwnership(QPainter* painter, const QRectF& grid)
{
    if (!showOwnership_ || ownership_.size() != static_cast<std::size_t>(boardSize_.pointCount())) {
        return;
    }

    painter->setPen(Qt::NoPen);
    const qreal cell = cellSize(grid);
    const qreal square = cell * 0.78;
    for (int y = 0; y < boardSize_.height; ++y) {
        for (int x = 0; x < boardSize_.width; ++x) {
            const double value = ownership_[static_cast<std::size_t>(y * boardSize_.width + x)];
            if (std::abs(value) < 0.01) {
                continue;
            }
            const QPointF center = pointCenter(lizzie::core::Point{x, y}, grid);
            painter->setBrush(ownershipColor(value, ownershipOpacity_));
            painter->drawRect(QRectF(center.x() - square / 2.0, center.y() - square / 2.0, square, square));
        }
    }
}

void BoardQuickItem::paintStones(QPainter* painter, const QRectF& grid)
{
    const qreal radius = cellSize(grid) * 0.43;
    for (int y = 0; y < boardSize_.height; ++y) {
        for (int x = 0; x < boardSize_.width; ++x) {
            const lizzie::core::Stone stone = stones_[static_cast<std::size_t>(y * boardSize_.width + x)];
            if (stone == lizzie::core::Stone::Empty) {
                continue;
            }

            const QPointF center = pointCenter(lizzie::core::Point{x, y}, grid);
            const QImage& texture = stone == lizzie::core::Stone::Black ? blackStoneTexture_ : whiteStoneTexture_;
            if (!texture.isNull()) {
                const QRectF stoneRect(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0);
                QPainterPath clip;
                clip.addEllipse(stoneRect);
                painter->save();
                painter->setClipPath(clip);
                painter->drawImage(stoneRect, texture);
                painter->restore();
                painter->setPen(stone == lizzie::core::Stone::Black
                    ? QPen(QColor(5, 5, 5), 1.0)
                    : QPen(QColor(154, 154, 145), 1.0));
                painter->setBrush(Qt::NoBrush);
                painter->drawEllipse(center, radius, radius);
                continue;
            }

            QRadialGradient gradient(center - QPointF(radius * 0.25, radius * 0.3), radius * 1.3);
            if (stone == lizzie::core::Stone::Black) {
                gradient.setColorAt(0.0, QColor(80, 80, 78));
                gradient.setColorAt(1.0, QColor(8, 9, 10));
                painter->setPen(QPen(QColor(5, 5, 5), 1.0));
            } else {
                gradient.setColorAt(0.0, QColor(255, 255, 250));
                gradient.setColorAt(1.0, QColor(204, 204, 196));
                painter->setPen(QPen(QColor(154, 154, 145), 1.0));
            }
            painter->setBrush(gradient);
            painter->drawEllipse(center, radius, radius);
        }
    }
}

void BoardQuickItem::paintMarkup(QPainter* painter, const QRectF& grid)
{
    const qreal cell = cellSize(grid);
    const qreal radius = cell * 0.27;
    QPen pen(QColor(28, 32, 35), std::max<qreal>(1.4, cell / 24.0));
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setBrush(Qt::NoBrush);

    for (const lizzie::core::BoardMark& mark : marks_) {
        if (mark.point.x < 0 || mark.point.y < 0 || mark.point.x >= boardSize_.width || mark.point.y >= boardSize_.height) {
            continue;
        }
        const QPointF center = pointCenter(mark.point, grid);
        pen.setColor(markupColor(mark.point, boardSize_, stones_));
        painter->setPen(pen);
        switch (mark.shape) {
        case lizzie::core::BoardMarkShape::Circle:
            painter->drawEllipse(center, radius, radius);
            break;
        case lizzie::core::BoardMarkShape::Square:
            painter->drawRect(QRectF(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0));
            break;
        case lizzie::core::BoardMarkShape::Triangle: {
            QPolygonF triangle;
            triangle << QPointF(center.x(), center.y() - radius * 1.15)
                     << QPointF(center.x() - radius, center.y() + radius * 0.75)
                     << QPointF(center.x() + radius, center.y() + radius * 0.75);
            painter->drawPolygon(triangle);
            break;
        }
        case lizzie::core::BoardMarkShape::Cross:
            painter->drawLine(
                QPointF(center.x() - radius, center.y() - radius),
                QPointF(center.x() + radius, center.y() + radius));
            painter->drawLine(
                QPointF(center.x() + radius, center.y() - radius),
                QPointF(center.x() - radius, center.y() + radius));
            break;
        }
    }

    if (labels_.empty()) {
        return;
    }
    QFont font = painter->font();
    font.setBold(true);
    font.setPointSizeF(std::max<qreal>(8.0, cell * 0.36));
    painter->setFont(font);
    const qreal labelRadius = cell * 0.42;
    for (const lizzie::core::BoardLabel& label : labels_) {
        if (label.point.x < 0 || label.point.y < 0 || label.point.x >= boardSize_.width || label.point.y >= boardSize_.height) {
            continue;
        }
        const QString text = QString::fromStdString(label.text);
        if (text.isEmpty()) {
            continue;
        }
        const QPointF center = pointCenter(label.point, grid);
        QFont labelFont = font;
        fitFontToBounds(labelFont, text, QSizeF(labelRadius * 1.65, labelRadius * 1.65));
        painter->setFont(labelFont);
        painter->setPen(markupColor(label.point, boardSize_, stones_));
        painter->drawText(
            QRectF(center.x() - labelRadius, center.y() - labelRadius, labelRadius * 2.0, labelRadius * 2.0),
            Qt::AlignCenter,
            text);
        painter->setFont(font);
    }
}

void BoardQuickItem::paintPreview(QPainter* painter, const QRectF& grid)
{
    if (previewMoves_.empty()) {
        return;
    }

    const qreal cell = cellSize(grid);
    const qreal radius = cell * 0.34;
    const qreal lineWidth = std::max<qreal>(1.3, cell / 22.0);
    std::vector<QPointF> centers;
    centers.reserve(previewMoves_.size());

    for (const lizzie::core::Move& move : previewMoves_) {
        if (move.type != lizzie::core::MoveType::Play ||
            move.point.x < 0 || move.point.y < 0 ||
            move.point.x >= boardSize_.width || move.point.y >= boardSize_.height) {
            continue;
        }
        centers.push_back(pointCenter(move.point, grid));
    }

    if (centers.empty()) {
        return;
    }

    QPen pathPen(QColor(248, 205, 86, 170), lineWidth);
    pathPen.setCapStyle(Qt::RoundCap);
    pathPen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pathPen);
    for (std::size_t index = 1; index < centers.size(); ++index) {
        painter->drawLine(centers[index - 1], centers[index]);
    }

    QFont font = painter->font();
    font.setBold(true);
    font.setPointSizeF(std::max<qreal>(7.0, radius * 0.72));
    painter->setFont(font);

    int visibleIndex = 0;
    for (const lizzie::core::Move& move : previewMoves_) {
        if (move.type != lizzie::core::MoveType::Play ||
            move.point.x < 0 || move.point.y < 0 ||
            move.point.x >= boardSize_.width || move.point.y >= boardSize_.height) {
            continue;
        }

        ++visibleIndex;
        const QPointF center = pointCenter(move.point, grid);
        const bool black = move.color == lizzie::core::Color::Black;
        painter->setPen(QPen(QColor(248, 205, 86, 235), visibleIndex == 1 ? lineWidth * 1.25 : lineWidth));
        painter->setBrush(black ? QColor(15, 18, 20, 220) : QColor(250, 250, 244, 230));
        painter->drawEllipse(center, radius, radius);

        QString label = QString::number(visibleIndex);
        QFont labelFont = font;
        fitFontToBounds(labelFont, label, QSizeF(radius * 1.45, radius * 1.45));
        painter->setFont(labelFont);
        painter->setPen(black ? QColor(245, 247, 248) : QColor(18, 22, 25));
        painter->drawText(QRectF(center.x() - radius, center.y() - radius, radius * 2.0, radius * 2.0), Qt::AlignCenter, label);
        painter->setFont(font);
    }
}

void BoardQuickItem::paintCandidates(QPainter* painter, const QRectF& grid)
{
    const qreal pulse = candidatePulseTimer_.isActive() ? (0.5 + 0.5 * std::sin(candidatePulsePhase_)) : 0.0;
    const qreal radius = cellSize(grid) * (0.26 + pulse * 0.04);
    const int alpha = static_cast<int>(190 + pulse * 45);
    QFont font = painter->font();
    font.setPointSizeF(std::max<qreal>(8.0, radius * 0.7));
    painter->setFont(font);

    int visibleCandidates = 0;
    for (const lizzie::core::MoveCandidate& candidate : candidates_) {
        if (candidate.move.type != lizzie::core::MoveType::Play ||
            !pointOnBoard(candidate.move.point, boardSize_)) {
            continue;
        }
        if (visibleCandidates >= kMaxBoardCandidateMarkers) {
            break;
        }
        const bool primaryCandidate = visibleCandidates == 0;
        ++visibleCandidates;
        const QPointF center = pointCenter(candidate.move.point, grid);
        const qreal markerRadius = primaryCandidate ? radius * 1.22 : radius;
        if (primaryCandidate) {
            painter->setPen(QPen(QColor(255, 221, 83, 230), std::max<qreal>(2.4, markerRadius * 0.16)));
            painter->setBrush(Qt::NoBrush);
            painter->drawEllipse(center, markerRadius * 1.16, markerRadius * 1.16);
        }
        painter->setPen(QPen(
            primaryCandidate ? QColor(255, 235, 126) : QColor(13, 92, 86),
            primaryCandidate ? std::max<qreal>(2.0, markerRadius * 0.11) : 1.2));
        painter->setBrush(primaryCandidate ? QColor(56, 224, 204, alpha) : QColor(39, 190, 171, alpha));
        painter->drawEllipse(center, markerRadius, markerRadius);

        const QString label = candidateLabel(candidate);
        if (!label.isEmpty()) {
            QFont labelFont = font;
            fitFontToBounds(labelFont, label, QSizeF(markerRadius * 1.55, markerRadius * 1.55));
            painter->setFont(labelFont);
            painter->setPen(QColor(12, 43, 41));
            painter->drawText(
                QRectF(
                    center.x() - markerRadius,
                    center.y() - markerRadius,
                    markerRadius * 2,
                    markerRadius * 2),
                Qt::AlignCenter,
                label);
            painter->setFont(font);
        }
    }
}

void BoardQuickItem::updateCandidateAnimation()
{
    const bool hasVisibleCandidates = std::ranges::any_of(candidates_, [this](const lizzie::core::MoveCandidate& candidate) {
        return candidate.move.type == lizzie::core::MoveType::Play &&
            pointOnBoard(candidate.move.point, boardSize_);
    });
    if (hasVisibleCandidates) {
        if (!candidatePulseTimer_.isActive()) {
            candidatePulseTimer_.start();
        }
        return;
    }

    candidatePulseTimer_.stop();
    candidatePulsePhase_ = 0.0;
}

}  // namespace lizzie::ui

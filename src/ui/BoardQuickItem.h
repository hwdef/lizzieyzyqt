#pragma once

#include "BoardPosition.h"
#include "GameModel.h"
#include "Types.h"

#include <QColor>
#include <QImage>
#include <QQuickPaintedItem>
#include <QString>
#include <QTimer>

#include <optional>
#include <vector>

class QMouseEvent;

namespace lizzie::ui {

class BoardQuickItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(int boardWidth READ boardWidth NOTIFY boardSizeChanged)
    Q_PROPERTY(int boardHeight READ boardHeight NOTIFY boardSizeChanged)
    Q_PROPERTY(bool showOwnership READ showOwnership WRITE setShowOwnership NOTIFY showOwnershipChanged)
    Q_PROPERTY(qreal ownershipOpacity READ ownershipOpacity WRITE setOwnershipOpacity NOTIFY ownershipOpacityChanged)

public:
    explicit BoardQuickItem(QQuickItem* parent = nullptr);

    [[nodiscard]] int boardWidth() const;
    [[nodiscard]] int boardHeight() const;
    [[nodiscard]] bool showOwnership() const;
    [[nodiscard]] qreal ownershipOpacity() const;

    void setShowOwnership(bool value);
    void setOwnershipOpacity(qreal value);
    void setPosition(const lizzie::core::BoardPosition& position);
    void setCandidates(const std::vector<lizzie::core::MoveCandidate>& candidates);
    void setOwnership(std::vector<double> ownership);
    void setStoneTextures(const QString& blackPath, const QString& whitePath);
    void setPreviewMoves(std::vector<lizzie::core::Move> moves);
    void setLabels(const std::vector<lizzie::core::BoardLabel>& labels);
    void setMarks(const std::vector<lizzie::core::BoardMark>& marks);
    void clearOverlay();
    [[nodiscard]] int previewMoveCount() const;

    void paint(QPainter* painter) override;

signals:
    void boardSizeChanged();
    void showOwnershipChanged();
    void ownershipOpacityChanged();
    void pointClicked(int x, int y);

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
    [[nodiscard]] QRectF gridRect() const;
    [[nodiscard]] QPointF pointCenter(lizzie::core::Point point, const QRectF& grid) const;
    [[nodiscard]] std::optional<lizzie::core::Point> pointAt(QPointF position, const QRectF& grid) const;
    [[nodiscard]] qreal cellSize(const QRectF& grid) const;
    void paintBackground(QPainter* painter, const QRectF& grid);
    void paintGrid(QPainter* painter, const QRectF& grid);
    void paintStarPoints(QPainter* painter, const QRectF& grid);
    void paintOwnership(QPainter* painter, const QRectF& grid);
    void paintStones(QPainter* painter, const QRectF& grid);
    void paintMarkup(QPainter* painter, const QRectF& grid);
    void paintPreview(QPainter* painter, const QRectF& grid);
    void paintCandidates(QPainter* painter, const QRectF& grid);
    void updateCandidateAnimation();

    lizzie::core::BoardSize boardSize_ = lizzie::core::BoardSize::square(19);
    std::vector<lizzie::core::Stone> stones_;
    std::vector<lizzie::core::MoveCandidate> candidates_;
    std::vector<double> ownership_;
    std::vector<lizzie::core::Move> previewMoves_;
    std::vector<lizzie::core::BoardLabel> labels_;
    std::vector<lizzie::core::BoardMark> marks_;
    QImage blackStoneTexture_;
    QImage whiteStoneTexture_;
    QTimer candidatePulseTimer_;
    qreal candidatePulsePhase_ = 0.0;
    bool showOwnership_ = false;
    qreal ownershipOpacity_ = 0.45;
};

}  // namespace lizzie::ui

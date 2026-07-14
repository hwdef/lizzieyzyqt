#pragma once

#include "Types.h"

#include <QWidget>

#include <cstddef>
#include <optional>
#include <vector>

namespace lizzie::ui {

struct AnalysisGraphPoint {
    lizzie::core::NodeId nodeId = 0;
    int moveNumber = 0;
    double winrate = 0.5;
    double scoreMean = 0.0;
    int visits = 0;
    std::optional<lizzie::core::Color> moveColor;
};

class AnalysisGraphWidget : public QWidget {
    Q_OBJECT

public:
    explicit AnalysisGraphWidget(QWidget* parent = nullptr);

    void setPoints(std::vector<AnalysisGraphPoint> points, lizzie::core::NodeId currentNode);
    void setChineseText(bool enabled);
    void setDarkTheme(bool enabled);

signals:
    void nodeClicked(lizzie::core::NodeId nodeId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    [[nodiscard]] QRectF plotRect() const;
    [[nodiscard]] QPointF pointPosition(const AnalysisGraphPoint& point, const QRectF& rect) const;
    [[nodiscard]] QPointF scorePosition(const AnalysisGraphPoint& point, const QRectF& rect) const;
    [[nodiscard]] QPointF visitsPosition(const AnalysisGraphPoint& point, const QRectF& rect, int maxVisits) const;
    [[nodiscard]] double winrateLossAt(std::size_t index) const;
    [[nodiscard]] int nearestPointIndex(QPointF pos) const;

    std::vector<AnalysisGraphPoint> points_;
    lizzie::core::NodeId currentNode_ = 0;
    int hoveredIndex_ = -1;
    bool chineseText_ = false;
    bool darkTheme_ = true;
};

}  // namespace lizzie::ui

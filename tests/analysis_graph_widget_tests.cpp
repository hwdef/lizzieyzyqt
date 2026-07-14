#include "AnalysisGraphWidget.h"

#include <QApplication>
#include <QByteArray>
#include <QColor>
#include <QEvent>
#include <QImage>
#include <QMouseEvent>
#include <QToolTip>

#include <algorithm>
#include <cmath>
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

int countLossBarPixels(const QImage& image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.red() > 120 && color.red() > color.green() * 2 && color.red() > color.blue() * 2) {
                ++count;
            }
        }
    }
    return count;
}

int countWinrateLinePixels(const QImage& image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.green() > 130 && color.red() < 120 && color.blue() > 100) {
                ++count;
            }
        }
    }
    return count;
}

int countScoreLinePixels(const QImage& image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.blue() > 150 && color.green() > 90 && color.red() > 50 &&
                color.blue() > color.red() + 35 && color.blue() > color.green() + 20) {
                ++count;
            }
        }
    }
    return count;
}

int countVisitsTrendPixels(const QImage& image)
{
    int count = 0;
    for (int y = 0; y < image.height(); ++y) {
        for (int x = 0; x < image.width(); ++x) {
            const QColor color = image.pixelColor(x, y);
            if (color.red() > 150 && color.green() > 80 && color.green() < 220 && color.blue() < 150) {
                ++count;
            }
        }
    }
    return count;
}

bool isDarkBackground(const QColor& color)
{
    return color.red() < 50 && color.green() < 55 && color.blue() < 65;
}

bool isLightBackground(const QColor& color)
{
    return color.red() > 230 && color.green() > 230 && color.blue() > 230;
}

QImage renderGraph(lizzie::ui::AnalysisGraphWidget* graph, qreal devicePixelRatio = 1.0)
{
    const QSize pixelSize(
        std::max(1, static_cast<int>(std::ceil(graph->width() * devicePixelRatio))),
        std::max(1, static_cast<int>(std::ceil(graph->height() * devicePixelRatio))));
    QImage image(pixelSize, QImage::Format_ARGB32_Premultiplied);
    image.setDevicePixelRatio(devicePixelRatio);
    image.fill(Qt::transparent);
    graph->render(&image);
    return image;
}

QPointF graphPointPosition(const QWidget& graph, int moveNumber, double winrate, int maxMove)
{
    const QRectF plot = QRectF(graph.rect()).adjusted(16.0, 12.0, -16.0, -16.0);
    return QPointF(
        plot.left() + plot.width() * (static_cast<double>(moveNumber) / maxMove),
        plot.bottom() - plot.height() * winrate);
}

QRectF graphPlotRect(const QWidget& graph)
{
    return QRectF(graph.rect()).adjusted(16.0, 12.0, -16.0, -16.0);
}

}  // namespace

int main(int argc, char* argv[])
{
    if (qgetenv("QT_QPA_PLATFORM").isEmpty()) {
        qputenv("QT_QPA_PLATFORM", QByteArray("offscreen"));
    }

    QApplication app(argc, argv);
    lizzie::ui::AnalysisGraphWidget graph;
    graph.resize(640, 220);
    graph.setPoints(
        {
            lizzie::ui::AnalysisGraphPoint{1, 1, 0.72, 4.0, 50, lizzie::core::Color::Black},
            lizzie::ui::AnalysisGraphPoint{2, 2, 0.38, -3.0, 120, lizzie::core::Color::Black},
            lizzie::ui::AnalysisGraphPoint{3, 3, 0.56, 1.0, 80, lizzie::core::Color::White},
        },
        2);
    graph.show();
    app.processEvents();

    QImage image = renderGraph(&graph);

    try {
        require(isDarkBackground(image.pixelColor(2, 2)), "analysis graph should default to a dark theme background");
        require(countLossBarPixels(image) > 20, "analysis graph should render red loss bars");
        require(countScoreLinePixels(image) > 80, "analysis graph should render the score mean line");

        graph.setDarkTheme(false);
        app.processEvents();
        const QImage lightImage = renderGraph(&graph);
        require(isLightBackground(lightImage.pixelColor(2, 2)), "analysis graph should render a light theme background");
        require(countWinrateLinePixels(lightImage) > 200, "light graph should still render winrate line");
        require(countScoreLinePixels(lightImage) > 80, "light graph should still render score mean line");
        graph.setDarkTheme(true);

        graph.resize(3840, 360);
        app.processEvents();
        const QImage largeImage = renderGraph(&graph);
        require(countLossBarPixels(largeImage) > 100, "4K graph should render loss bars");
        require(countWinrateLinePixels(largeImage) > 1000, "4K graph should render winrate line");
        require(countScoreLinePixels(largeImage) > 800, "4K graph should render score mean line");
        require(countVisitsTrendPixels(largeImage) > 500, "4K graph should render visits trend line");

        graph.resize(960, 260);
        app.processEvents();
        const QImage highDpiImage = renderGraph(&graph, 2.0);
        require(countLossBarPixels(highDpiImage) > 80, "2x high-DPI graph should render loss bars");
        require(countWinrateLinePixels(highDpiImage) > 600, "2x high-DPI graph should render winrate line");
        require(countScoreLinePixels(highDpiImage) > 400, "2x high-DPI graph should render score mean line");
        require(countVisitsTrendPixels(highDpiImage) > 300, "2x high-DPI graph should render visits trend line");

        lizzie::core::NodeId clickedNode = 0;
        QObject::connect(&graph, &lizzie::ui::AnalysisGraphWidget::nodeClicked, [&](lizzie::core::NodeId nodeId) {
            clickedNode = nodeId;
        });
        graph.resize(640, 220);
        app.processEvents();
        const QPointF secondPoint = graphPointPosition(graph, 2, 0.38, 3);
        QMouseEvent click(
            QEvent::MouseButtonPress,
            secondPoint,
            secondPoint,
            secondPoint,
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier);
        QApplication::sendEvent(&graph, &click);
        require(clickedNode == 2, "clicking a graph point should emit the corresponding node id");

        clickedNode = 0;
        const QPointF secondMoveColumn(secondPoint.x(), graphPlotRect(graph).bottom() - 2.0);
        QMouseEvent columnClick(
            QEvent::MouseButtonPress,
            secondMoveColumn,
            secondMoveColumn,
            secondMoveColumn,
            Qt::LeftButton,
            Qt::LeftButton,
            Qt::NoModifier);
        QApplication::sendEvent(&graph, &columnClick);
        require(clickedNode == 2, "clicking the graph plot column should jump to the nearest move");

        QMouseEvent move(
            QEvent::MouseMove,
            secondPoint,
            secondPoint,
            secondPoint,
            Qt::NoButton,
            Qt::NoButton,
            Qt::NoModifier);
        QApplication::sendEvent(&graph, &move);
        app.processEvents();
        const QString tooltip = QToolTip::text();
        require(tooltip.contains("Move 2"), "graph hover should show move number");
        require(tooltip.contains("Black winrate 38.0%"), "graph hover should show black winrate");
        require(tooltip.contains("Black score -3.00"), "graph hover should show black score");
        require(tooltip.contains("Visits 120"), "graph hover should show visits");
        require(tooltip.contains("Loss 34.0%"), "graph hover should show black move loss");

        QMouseEvent columnMove(
            QEvent::MouseMove,
            secondMoveColumn,
            secondMoveColumn,
            secondMoveColumn,
            Qt::NoButton,
            Qt::NoButton,
            Qt::NoModifier);
        QApplication::sendEvent(&graph, &columnMove);
        app.processEvents();
        require(
            QToolTip::text().contains("Move 2"),
            "graph hover in a plot column should show the nearest move number");

        const QPointF thirdPoint = graphPointPosition(graph, 3, 0.56, 3);
        QMouseEvent thirdMove(
            QEvent::MouseMove,
            thirdPoint,
            thirdPoint,
            thirdPoint,
            Qt::NoButton,
            Qt::NoButton,
            Qt::NoModifier);
        QApplication::sendEvent(&graph, &thirdMove);
        app.processEvents();
        require(
            QToolTip::text().contains("Loss 18.0%"),
            "graph hover should compute white move loss from black winrate gain");

        graph.setChineseText(true);
        QApplication::sendEvent(&graph, &move);
        app.processEvents();
        const QString zhTooltip = QToolTip::text();
        require(zhTooltip.contains(QString::fromUtf8("手数 2")), "Chinese graph hover should show move number");
        require(zhTooltip.contains(QString::fromUtf8("黑胜率 38.0%")), "Chinese graph hover should show black winrate");
        require(zhTooltip.contains(QString::fromUtf8("黑目差 -3.00")), "Chinese graph hover should show black score");
        require(zhTooltip.contains(QString::fromUtf8("访问 120")), "Chinese graph hover should show visits");
        require(zhTooltip.contains(QString::fromUtf8("损失 34.0%")), "Chinese graph hover should show loss");

        QEvent leave(QEvent::Leave);
        QApplication::sendEvent(&graph, &leave);
        app.processEvents();
    } catch (const std::exception& error) {
        std::cerr << "analysis_graph_widget_tests failed: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "analysis_graph_widget_tests passed\n";
    return EXIT_SUCCESS;
}

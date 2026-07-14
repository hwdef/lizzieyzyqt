#include "WindowPlacement.h"

#include <QGuiApplication>
#include <QScreen>

#include <algorithm>

namespace lizzie::app {

std::int64_t windowVisibleAreaOnAvailableScreens(const QRect& frameGeometry)
{
    std::int64_t visibleArea = 0;
    for (QScreen* screen : QGuiApplication::screens()) {
        if (screen == nullptr) {
            continue;
        }
        const QRect visible = screen->availableGeometry().intersected(frameGeometry);
        if (visible.isValid()) {
            visibleArea += static_cast<std::int64_t>(visible.width()) * visible.height();
        }
    }
    return visibleArea;
}

std::int64_t requiredVisibleAreaForWindowFrame(const QRect& frameGeometry)
{
    const int requiredWidth = std::min(std::max(frameGeometry.width(), 1), 320);
    const int requiredHeight = std::min(std::max(frameGeometry.height(), 1), 240);
    return static_cast<std::int64_t>(requiredWidth) * requiredHeight;
}

bool windowFrameIsSubstantiallyVisible(const QRect& frameGeometry)
{
    return windowVisibleAreaOnAvailableScreens(frameGeometry) >= requiredVisibleAreaForWindowFrame(frameGeometry);
}

std::optional<QRect> adjustedWindowGeometryForAvailableScreens(
    const QRect& frameGeometry,
    const QSize& windowSize,
    const QSize& minimumSize)
{
    if (windowFrameIsSubstantiallyVisible(frameGeometry)) {
        return std::nullopt;
    }

    QScreen* targetScreen = QGuiApplication::primaryScreen();
    if (targetScreen == nullptr) {
        return std::nullopt;
    }

    const QRect target = targetScreen->availableGeometry();
    const QSize size = windowSize.expandedTo(minimumSize).boundedTo(target.size());
    QRect restored(QPoint{}, size);
    restored.moveCenter(target.center());
    return restored;
}

}  // namespace lizzie::app

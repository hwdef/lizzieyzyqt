#pragma once

#include <QRect>
#include <QSize>

#include <cstdint>
#include <optional>

namespace lizzie::app {

[[nodiscard]] std::int64_t windowVisibleAreaOnAvailableScreens(const QRect& frameGeometry);
[[nodiscard]] std::int64_t requiredVisibleAreaForWindowFrame(const QRect& frameGeometry);
[[nodiscard]] bool windowFrameIsSubstantiallyVisible(const QRect& frameGeometry);
[[nodiscard]] std::optional<QRect> adjustedWindowGeometryForAvailableScreens(
    const QRect& frameGeometry,
    const QSize& windowSize,
    const QSize& minimumSize = QSize(640, 480));

}  // namespace lizzie::app

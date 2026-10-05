// SPDX-License-Identifier: GPL-3.0-or-later

#include "selectiongeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
double availableSize(double anchor, double fraction, double low, double high)
{
    double available = std::numeric_limits<double>::max();
    if (fraction > 0) {
        available = std::min(available, (anchor - low) / fraction);
    }
    if (fraction < 1) {
        available = std::min(available, (high - anchor) / (1 - fraction));
    }
    return std::max(0.0, available);
}
}

QRect SelectionGeometry::constrain(QSizeF requestedSize,
                                   QPointF anchor,
                                   QPointF anchorFraction,
                                   double ratio,
                                   const QRect& bounds,
                                   Dimension dimension)
{
    if (!bounds.isValid() || !std::isfinite(ratio) || ratio <= 0) {
        return {};
    }
    const double maxWidth = availableSize(
      anchor.x(), anchorFraction.x(), bounds.x(), bounds.x() + bounds.width());
    const double maxHeight = availableSize(
      anchor.y(), anchorFraction.y(), bounds.y(), bounds.y() + bounds.height());
    double width = std::max(1.0, requestedSize.width());
    double height = std::max(1.0, requestedSize.height());
    if (dimension == Dimension::Width) {
        height = width / ratio;
    } else if (dimension == Dimension::Height) {
        width = height * ratio;
    } else {
        width = std::max(width, height * ratio);
        height = width / ratio;
    }
    // Scale both dimensions together; clipping one edge would lose the ratio.
    const double scale =
      std::min({ 1.0, maxWidth / width, maxHeight / height });
    const int w = std::max(
      1,
      std::min(qRound(width * scale), static_cast<int>(std::floor(maxWidth))));
    const int h = std::max(1,
                           std::min(qRound(height * scale),
                                    static_cast<int>(std::floor(maxHeight))));
    QRect result(qRound(anchor.x() - anchorFraction.x() * w),
                 qRound(anchor.y() - anchorFraction.y() * h),
                 w,
                 h);
    return moveWithinBounds(result, bounds);
}

QRect SelectionGeometry::fit(const QRect& rectangle,
                             double ratio,
                             const QRect& bounds)
{
    const QRect inside = moveWithinBounds(rectangle.normalized(), bounds);
    const QPointF centre(inside.x() + inside.width() / 2.0,
                         inside.y() + inside.height() / 2.0);
    return constrain(
      inside.size(), centre, { 0.5, 0.5 }, ratio, bounds, Dimension::Width);
}

QRect SelectionGeometry::moveWithinBounds(QRect rectangle, const QRect& bounds)
{
    if (!bounds.isValid()) {
        return {};
    }
    rectangle.setSize({ std::clamp(rectangle.width(), 1, bounds.width()),
                        std::clamp(rectangle.height(), 1, bounds.height()) });
    rectangle.moveLeft(
      std::clamp(rectangle.x(),
                 bounds.x(),
                 bounds.x() + bounds.width() - rectangle.width()));
    rectangle.moveTop(
      std::clamp(rectangle.y(),
                 bounds.y(),
                 bounds.y() + bounds.height() - rectangle.height()));
    return rectangle;
}

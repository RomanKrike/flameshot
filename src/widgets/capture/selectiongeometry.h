// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QPointF>
#include <QRect>
#include <QSizeF>

namespace SelectionGeometry {
enum class Dimension
{
    Both,
    Width,
    Height
};

// anchorFraction identifies the fixed point within the result: 0 is the
// left/top boundary, 1 is the right/bottom boundary, 0.5 is the centre.
// Coordinates use outer pixel boundaries (right/bottom are exclusive).
QRect constrain(QSizeF requestedSize,
                QPointF anchor,
                QPointF anchorFraction,
                double ratio,
                const QRect& bounds,
                Dimension dimension = Dimension::Both);
QRect fit(const QRect& rectangle, double ratio, const QRect& bounds);
QRect moveWithinBounds(QRect rectangle, const QRect& bounds);
}

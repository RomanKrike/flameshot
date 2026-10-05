// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#include "selectionwidget.h"
#include "selectiongeometry.h"
#include "utils/globalvalues.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPropertyAnimation>
#include <QTimer>
#include <cmath>
#include <utility>

#define MARGIN (m_THandle.width())

SelectionWidget::SelectionWidget(QColor c, QWidget* parent)
  : QWidget(parent)
  , m_color(std::move(c))
  , m_activeSide(NO_SIDE)
  , m_ignoreMouse(false)
{
    // prevents this widget from consuming CaptureToolButton mouse events
    setAttribute(Qt::WA_TransparentForMouseEvents);
    parent->installEventFilter(this);

    m_animation = new QPropertyAnimation(this, "geometry", this);
    m_animation->setEasingCurve(QEasingCurve::InOutQuad);
    m_animation->setDuration(200);
    connect(m_animation, &QPropertyAnimation::finished, this, [this]() {
        emit geometrySettled();
    });

    int sideVal = GlobalValues::buttonBaseSize() * 0.6;
    int handleSide = sideVal / 2;
    const QRect areaRect(0, 0, sideVal, sideVal);

    const QRect handleRect(0, 0, handleSide, handleSide);
    m_TLHandle = m_TRHandle = m_BLHandle = m_BRHandle = m_LHandle = m_THandle =
      m_RHandle = m_BHandle = handleRect;
    m_TLArea = m_TRArea = m_BLArea = m_BRArea = areaRect;

    m_areaOffset = QPoint(-sideVal / 2, -sideVal / 2);
    m_handleOffset = QPoint(-handleSide / 2, -handleSide / 2);
}

/**
 * @brief Get the side where the mouse cursor is.
 * @param mousePos Mouse cursor position relative to the parent widget.
 */
SelectionWidget::SideType SelectionWidget::getMouseSide(
  const QPoint& mousePos) const
{
    if (!isVisible()) {
        return NO_SIDE;
    }
    QPoint localPos = mapFromParent(mousePos);
    if (m_TLArea.contains(localPos)) {
        return TOPLEFT_SIDE;
    } else if (m_TRArea.contains(localPos)) {
        return TOPRIGHT_SIDE;
    } else if (m_BLArea.contains(localPos)) {
        return BOTTOMLEFT_SIDE;
    } else if (m_BRArea.contains(localPos)) {
        return BOTTOMRIGHT_SIDE;
    } else if (m_LArea.contains(localPos)) {
        return LEFT_SIDE;
    } else if (m_TArea.contains(localPos)) {
        return TOP_SIDE;
    } else if (m_RArea.contains(localPos)) {
        return RIGHT_SIDE;
    } else if (m_BArea.contains(localPos)) {
        return BOTTOM_SIDE;
    } else if (rect().contains(localPos)) {
        return CENTER;
    } else {
        return NO_SIDE;
    }
}

QVector<QRect> SelectionWidget::handlerAreas()
{
    QVector<QRect> areas;
    areas << m_TLHandle << m_TRHandle << m_BLHandle << m_BRHandle << m_LHandle
          << m_THandle << m_RHandle << m_BHandle;
    return areas;
}

void SelectionWidget::setIgnoreMouse(bool ignore)
{
    m_ignoreMouse = ignore;
    updateCursor();
}

/**
 * Set the cursor that will be active when the mouse is inside the selection and
 * the mouse is not clicked.
 */
void SelectionWidget::setIdleCentralCursor(const QCursor& cursor)
{
    m_idleCentralCursor = cursor;
}

void SelectionWidget::setGeometryAnimated(const QRect& r)
{
    if (m_lockedAspectRatio > 0) {
        setGeometry(r);
        emit geometrySettled();
    } else if (isVisible()) {
        m_animation->setStartValue(geometry());
        m_animation->setEndValue(r);
        m_animation->start();
    }
}

void SelectionWidget::setGeometry(const QRect& r)
{
    applyGeometry(
      m_lockedAspectRatio > 0
        ? SelectionGeometry::fit(r, m_lockedAspectRatio, parentWidget()->rect())
        : r);
}

void SelectionWidget::applyGeometry(const QRect& r)
{
    QWidget::setGeometry(r + QMargins(MARGIN, MARGIN, MARGIN, MARGIN));
    updateCursor();
    if (isVisible()) {
        emit geometryChanged();
    }
}

QRect SelectionWidget::geometry() const
{
    return QWidget::geometry() - QMargins(MARGIN, MARGIN, MARGIN, MARGIN);
}

QRect SelectionWidget::fullGeometry() const
{
    return QWidget::geometry();
}

QRect SelectionWidget::rect() const
{
    return QWidget::rect() - QMargins(MARGIN, MARGIN, MARGIN, MARGIN);
}

bool SelectionWidget::eventFilter(QObject* obj, QEvent* event)
{
    if (m_ignoreMouse && dynamic_cast<QMouseEvent*>(event)) {
        m_activeSide = NO_SIDE;
        m_resizeSide = NO_SIDE;
        unsetCursor();
    } else if (event->type() == QEvent::MouseButtonRelease) {
        parentMouseReleaseEvent(static_cast<QMouseEvent*>(event));
    } else if (event->type() == QEvent::MouseButtonPress) {
        parentMousePressEvent(static_cast<QMouseEvent*>(event));
    } else if (event->type() == QEvent::MouseMove) {
        parentMouseMoveEvent(static_cast<QMouseEvent*>(event));
    }
    return false;
}

void SelectionWidget::setAspectRatio(double ratio)
{
    if (!std::isfinite(ratio) || ratio < 0 || ratio == m_lockedAspectRatio) {
        return;
    }
    m_animation->stop();
    m_lockedAspectRatio = ratio;
    if (isVisible() && ratio > 0) {
        applyGeometry(
          SelectionGeometry::fit(geometry(), ratio, parentWidget()->rect()));
        emit geometrySettled();
    }
    emit aspectRatioChanged(ratio);
}

double SelectionWidget::aspectRatio() const
{
    return m_lockedAspectRatio;
}

void SelectionWidget::parentMousePressEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        return;
    }
    m_animation->stop();
    m_dragStartPos = e->pos();
    m_activeSide = getMouseSide(e->pos());
    m_resizeSide = m_activeSide;
    m_creatingSelection = m_resizeSide == NO_SIDE;
    m_dragGeometry = geometry();
    if (m_dragGeometry.height() > 0 && m_dragGeometry.width() > 0) {
        m_dragAspectRatio =
          double(m_dragGeometry.width()) / m_dragGeometry.height();
    }
}

void SelectionWidget::parentMouseReleaseEvent(QMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) {
        return;
    }
    // The pointer may lie outside a constrained rectangle, especially at the
    // capture boundary. Releasing a resize must still keep the selection.
    if (!m_activeSide && !getMouseSide(e->pos())) {
        hide();
    }
    m_activeSide = NO_SIDE;
    m_resizeSide = NO_SIDE;
    updateCursor();
    emit geometrySettled();
}

QRect SelectionWidget::resizedGeometry(const QPoint& pos,
                                       bool symmetric,
                                       double ratio,
                                       SideType& activeSide) const
{
    const QRect& r = m_dragGeometry;
    const bool left = m_resizeSide & LEFT_SIDE;
    const bool right = m_resizeSide & RIGHT_SIDE;
    const bool top = m_resizeSide & TOP_SIDE;
    const bool bottom = m_resizeSide & BOTTOM_SIDE;
    const bool horizontal = left || right;
    const bool vertical = top || bottom;
    const QPointF centre(r.x() + r.width() / 2.0, r.y() + r.height() / 2.0);
    QPointF anchor(right ? r.x() : r.x() + r.width(),
                   bottom ? r.y() : r.y() + r.height());
    if (!horizontal) {
        anchor.setX(r.x());
    }
    if (!vertical) {
        anchor.setY(r.y());
    }
    if (symmetric) {
        anchor = centre;
    }
    if (m_creatingSelection && !symmetric) {
        // A newly selected area includes the press pixel in every direction.
        anchor.setX(r.x() + (pos.x() < r.x() ? 1 : 0));
        anchor.setY(r.y() + (pos.y() < r.y() ? 1 : 0));
    }
    const double dx =
      horizontal ? pos.x() + (pos.x() >= anchor.x() ? 1 : 0) - anchor.x() : 0;
    const double dy =
      vertical ? pos.y() + (pos.y() >= anchor.y() ? 1 : 0) - anchor.y() : 0;
    activeSide = static_cast<SideType>(
      (horizontal ? (dx < 0 ? LEFT_SIDE : RIGHT_SIDE) : 0) |
      (vertical ? (dy < 0 ? TOP_SIDE : BOTTOM_SIDE) : 0));
    const QSizeF requested(
      horizontal ? std::abs(dx) * (symmetric ? 2 : 1) : r.width(),
      vertical ? std::abs(dy) * (symmetric ? 2 : 1) : r.height());
    const QPointF fraction(symmetric ? 0.5 : (horizontal && dx < 0 ? 1 : 0),
                           symmetric ? 0.5 : (vertical && dy < 0 ? 1 : 0));
    if (ratio > 0) {
        const auto dimension =
          horizontal && vertical ? SelectionGeometry::Dimension::Both
          : horizontal           ? SelectionGeometry::Dimension::Width
                                 : SelectionGeometry::Dimension::Height;
        return SelectionGeometry::constrain(requested,
                                            anchor,
                                            fraction,
                                            ratio,
                                            parentWidget()->rect(),
                                            dimension);
    }
    const QSize size(qMax(1, qRound(requested.width())),
                     qMax(1, qRound(requested.height())));
    return { qRound(anchor.x() - fraction.x() * size.width()),
             qRound(anchor.y() - fraction.y() * size.height()),
             size.width(),
             size.height() };
}

void SelectionWidget::parentMouseMoveEvent(QMouseEvent* e)
{
    updateCursor();
    if (e->buttons() != Qt::LeftButton) {
        return;
    }
    if (!m_resizeSide) {
        // Start a fresh selection from the press point, in any drag direction.
        m_dragGeometry = QRect(m_dragStartPos, QSize(1, 1));
        m_resizeSide = BOTTOMRIGHT_SIDE;
        m_activeSide = BOTTOMRIGHT_SIDE;
        m_dragAspectRatio = 1;
        show();
    }
    if (m_resizeSide == CENTER) {
        const QRect moved =
          m_dragGeometry.translated(e->pos() - m_dragStartPos);
        applyGeometry(
          m_lockedAspectRatio > 0
            ? SelectionGeometry::moveWithinBounds(moved, parentWidget()->rect())
            : moved);
        return;
    }
    const bool symmetric = e->modifiers() & Qt::ShiftModifier;
    const double ratio = m_lockedAspectRatio > 0 ? m_lockedAspectRatio
                         : (e->modifiers() & Qt::ControlModifier)
                           ? m_dragAspectRatio
                           : 0;
    applyGeometry(resizedGeometry(e->pos(), symmetric, ratio, m_activeSide));
}

void SelectionWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    if (!p.isActive()) {
        return;
    }
    p.setPen(m_color);
    p.drawRect(rect() + QMargins(0, 0, -1, -1));
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(m_color);
    for (auto rectangle : handlerAreas()) {
        p.drawEllipse(rectangle);
    }
}

void SelectionWidget::resizeEvent(QResizeEvent*)
{
    updateAreas();
    if (isVisible()) {
        emit geometryChanged();
    }
}

void SelectionWidget::moveEvent(QMoveEvent*)
{
    updateAreas();
    if (isVisible()) {
        emit geometryChanged();
    }
}

void SelectionWidget::showEvent(QShowEvent*)
{
    emit visibilityChanged();
}

void SelectionWidget::hideEvent(QHideEvent*)
{
    emit visibilityChanged();
}

void SelectionWidget::updateColor(const QColor& c)
{
    m_color = c;
}

void SelectionWidget::moveLeft()
{
    setGeometryByKeyboard(geometry().adjusted(-1, 0, -1, 0));
}

void SelectionWidget::moveRight()
{
    setGeometryByKeyboard(geometry().adjusted(1, 0, 1, 0));
}

void SelectionWidget::moveUp()
{
    setGeometryByKeyboard(geometry().adjusted(0, -1, 0, -1));
}

void SelectionWidget::moveDown()
{
    setGeometryByKeyboard(geometry().adjusted(0, 1, 0, 1));
}

void SelectionWidget::resizeLeft()
{
    setGeometryByKeyboard(geometry().adjusted(0, 0, -1, 0));
}

void SelectionWidget::resizeRight()
{
    setGeometryByKeyboard(geometry().adjusted(0, 0, 1, 0));
}

void SelectionWidget::resizeUp()
{
    setGeometryByKeyboard(geometry().adjusted(0, 0, 0, -1));
}

void SelectionWidget::resizeDown()
{
    setGeometryByKeyboard(geometry().adjusted(0, 0, 0, 1));
}

void SelectionWidget::symResizeLeft()
{
    setGeometryByKeyboard(geometry().adjusted(1, 0, -1, 0));
}

void SelectionWidget::symResizeRight()
{
    setGeometryByKeyboard(geometry().adjusted(-1, 0, 1, 0));
}

void SelectionWidget::symResizeUp()
{
    setGeometryByKeyboard(geometry().adjusted(0, -1, 0, 1));
}

void SelectionWidget::symResizeDown()
{
    setGeometryByKeyboard(geometry().adjusted(0, 1, 0, -1));
}

void SelectionWidget::updateAreas()
{
    QRect r = rect();
    m_TLArea.moveTo(r.topLeft() + m_areaOffset);
    m_TRArea.moveTo(r.topRight() + m_areaOffset);
    m_BLArea.moveTo(r.bottomLeft() + m_areaOffset);
    m_BRArea.moveTo(r.bottomRight() + m_areaOffset);

    m_LArea = QRect(m_TLArea.bottomLeft(), m_BLArea.topRight());
    m_TArea = QRect(m_TLArea.topRight(), m_TRArea.bottomLeft());
    m_RArea = QRect(m_TRArea.bottomLeft(), m_BRArea.topRight());
    m_BArea = QRect(m_BLArea.topRight(), m_BRArea.bottomLeft());

    m_TLHandle.moveTo(m_TLArea.center() + m_handleOffset);
    m_BLHandle.moveTo(m_BLArea.center() + m_handleOffset);
    m_TRHandle.moveTo(m_TRArea.center() + m_handleOffset);
    m_BRHandle.moveTo(m_BRArea.center() + m_handleOffset);
    m_LHandle.moveTo(m_LArea.center() + m_handleOffset);
    m_THandle.moveTo(m_TArea.center() + m_handleOffset);
    m_RHandle.moveTo(m_RArea.center() + m_handleOffset);
    m_BHandle.moveTo(m_BArea.center() + m_handleOffset);
}

void SelectionWidget::updateCursor()
{
    SideType mouseSide = m_activeSide;
    if (!m_activeSide) {
        mouseSide = getMouseSide(parentWidget()->mapFromGlobal(QCursor::pos()));
    }

    switch (mouseSide) {
        case TOPLEFT_SIDE:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case BOTTOMRIGHT_SIDE:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TOPRIGHT_SIDE:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case BOTTOMLEFT_SIDE:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case LEFT_SIDE:
            setCursor(Qt::SizeHorCursor);
            break;
        case RIGHT_SIDE:
            setCursor(Qt::SizeHorCursor);
            break;
        case TOP_SIDE:
            setCursor(Qt::SizeVerCursor);
            break;
        case BOTTOM_SIDE:
            setCursor(Qt::SizeVerCursor);
            break;
        default:
            if (m_activeSide) {
                setCursor(Qt::ClosedHandCursor);
            } else {
                setCursor(m_idleCentralCursor);
                return;
            }
            break;
    }
}

void SelectionWidget::setGeometryByKeyboard(const QRect& r)
{
    static QTimer timer;
    QRect rectangle;
    const QRect current = geometry();
    if (m_lockedAspectRatio > 0 && r.size() != current.size()) {
        const bool symmetric = r.topLeft() != current.topLeft();
        const QPointF anchor = symmetric
                                 ? QPointF(current.x() + current.width() / 2.0,
                                           current.y() + current.height() / 2.0)
                                 : QPointF(current.topLeft());
        const auto dimension = r.width() != current.width()
                                 ? SelectionGeometry::Dimension::Width
                                 : SelectionGeometry::Dimension::Height;
        rectangle = SelectionGeometry::constrain(r.size(),
                                                 anchor,
                                                 symmetric ? QPointF(0.5, 0.5)
                                                           : QPointF(0, 0),
                                                 m_lockedAspectRatio,
                                                 parentWidget()->rect(),
                                                 dimension);
    } else if (m_lockedAspectRatio > 0) {
        rectangle =
          SelectionGeometry::moveWithinBounds(r, parentWidget()->rect());
    } else {
        rectangle = r.intersected(parentWidget()->rect());
        rectangle.setWidth(qMax(1, rectangle.width()));
        rectangle.setHeight(qMax(1, rectangle.height()));
    }
    applyGeometry(rectangle);
    connect(&timer,
            &QTimer::timeout,
            this,
            &SelectionWidget::geometrySettled,
            Qt::UniqueConnection);
    timer.start(400);
}

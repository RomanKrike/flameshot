// SPDX-License-Identifier: GPL-3.0-or-later

#include "widgets/capture/selectiongeometry.h"
#include "widgets/capture/selectionwidget.h"

#include <QApplication>
#include <QMouseEvent>
#include <QSignalSpy>
#include <QtTest>
#include <cmath>
#include <limits>

class SelectionTest : public QObject
{
    Q_OBJECT
private:
    static void mouse(QWidget& parent,
                      QEvent::Type type,
                      QPoint position,
                      Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        const bool moving = type == QEvent::MouseMove;
        QMouseEvent event(type,
                          position,
                          parent.mapToGlobal(position),
                          moving ? Qt::NoButton : Qt::LeftButton,
                          type == QEvent::MouseButtonRelease ? Qt::NoButton
                                                             : Qt::LeftButton,
                          modifiers);
        QApplication::sendEvent(&parent, &event);
    }
    static void drag(QWidget& parent,
                     QPoint start,
                     QPoint end,
                     Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        mouse(parent, QEvent::MouseButtonPress, start, modifiers);
        mouse(parent, QEvent::MouseMove, end, modifiers);
        mouse(parent, QEvent::MouseButtonRelease, end, modifiers);
    }
    static bool hasRatio(QRect r, double ratio)
    {
        // At most half a rounded pixel on each dimension; 1px selections have
        // an unavoidable minimum size and may approximate extreme ratios.
        return std::abs(r.width() - r.height() * ratio) <=
               (1 + ratio) / 2.0 + 0.01;
    }

private slots:
    void boundedGeometry_data()
    {
        QTest::addColumn<double>("ratio");
        QTest::newRow("square") << 1.0;
        QTest::newRow("landscape-4-3") << 4.0 / 3;
        QTest::newRow("landscape-3-2") << 3.0 / 2;
        QTest::newRow("landscape-16-9") << 16.0 / 9;
        QTest::newRow("portrait") << 9.0 / 16;
    }
    void boundedGeometry()
    {
        QFETCH(double, ratio);
        // Includes a multi-monitor-style capture rectangle with negative
        // origin.
        const QRect bounds(-1280, -200, 3200, 1200);
        for (double fractionX : { 0.0, 0.5, 1.0 }) {
            for (double fractionY : { 0.0, 0.5, 1.0 }) {
                for (auto dimension :
                     { SelectionGeometry::Dimension::Both,
                       SelectionGeometry::Dimension::Width,
                       SelectionGeometry::Dimension::Height }) {
                    for (QSize size :
                         { QSize(1, 1), QSize(301, 199), QSize(9000, 8000) }) {
                        const QPointF anchor(-300.5, 250.5);
                        QRect r =
                          SelectionGeometry::constrain(size,
                                                       anchor,
                                                       { fractionX, fractionY },
                                                       ratio,
                                                       bounds,
                                                       dimension);
                        QVERIFY(bounds.contains(r));
                        QVERIFY(hasRatio(r, ratio));
                        QVERIFY(std::abs(r.x() + fractionX * r.width() -
                                         anchor.x()) <= 0.5);
                        QVERIFY(std::abs(r.y() + fractionY * r.height() -
                                         anchor.y()) <= 0.5);
                    }
                }
            }
        }
    }
    void initialSelection_data()
    {
        QTest::addColumn<QPoint>("end");
        QTest::newRow("bottom-right") << QPoint(600, 450);
        QTest::newRow("top-left") << QPoint(30, 50);
        QTest::newRow("top-right") << QPoint(900, 50);
        QTest::newRow("bottom-left") << QPoint(30, 700);
        QTest::newRow("outside-capture") << QPoint(1800, 1400);
        QTest::newRow("horizontal") << QPoint(600, 300);
        QTest::newRow("vertical") << QPoint(400, 500);
    }
    void initialSelection()
    {
        QFETCH(QPoint, end);
        QWidget parent;
        parent.resize(1000, 800);
        SelectionWidget selection(Qt::red, &parent);
        parent.show();
        selection.hide();
        selection.setAspectRatio(16.0 / 9);
        drag(parent, { 400, 300 }, end);
        QVERIFY(selection.isVisible());
        QVERIFY(parent.rect().contains(selection.geometry()));
        QVERIFY(hasRatio(selection.geometry(), 16.0 / 9));
    }
    void resizeHandles_data()
    {
        QTest::addColumn<QPoint>("start");
        QTest::addColumn<QPoint>("end");
        QTest::addColumn<bool>("symmetric");
        const QList<QPoint> handles = { { 200, 200 }, { 599, 200 },
                                        { 200, 424 }, { 599, 424 },
                                        { 200, 312 }, { 599, 312 },
                                        { 400, 200 }, { 400, 424 } };
        for (int i = 0; i < handles.size(); ++i) {
            for (bool symmetric : { false, true }) {
                for (QPoint end :
                     { QPoint(40, 40), QPoint(920, 730), QPoint(410, 310) }) {
                    const QByteArray name = QByteArray::number(i) + "-" +
                                            QByteArray::number(symmetric) +
                                            "-" + QByteArray::number(end.x());
                    QTest::newRow(name.constData())
                      << handles[i] << end << symmetric;
                }
            }
        }
    }
    void resizeHandles()
    {
        QFETCH(QPoint, start);
        QFETCH(QPoint, end);
        QFETCH(bool, symmetric);
        QWidget parent;
        parent.resize(1000, 800);
        SelectionWidget selection(Qt::red, &parent);
        parent.show();
        selection.setGeometry({ 200, 200, 400, 225 });
        selection.show();
        selection.setAspectRatio(16.0 / 9);
        drag(
          parent, start, end, symmetric ? Qt::ShiftModifier : Qt::NoModifier);
        QVERIFY(selection.isVisible());
        QVERIFY(parent.rect().contains(selection.geometry()));
        QVERIFY(hasRatio(selection.geometry(), 16.0 / 9));
        if (symmetric) {
            QVERIFY(std::abs(selection.geometry().x() +
                             selection.geometry().width() / 2.0 - 400) <= 0.5);
            QVERIFY(std::abs(selection.geometry().y() +
                             selection.geometry().height() / 2.0 - 312.5) <=
                    0.5);
        }
    }
    void keyboardAndExternalGeometry()
    {
        QWidget parent;
        parent.resize(800, 600);
        SelectionWidget selection(Qt::red, &parent);
        parent.show();
        selection.setGeometry({ 100, 100, 320, 180 });
        selection.show();
        selection.setAspectRatio(16.0 / 9);
        for (int i = 0; i < 300; ++i) {
            selection.resizeRight();
            selection.resizeDown();
            selection.symResizeRight();
            selection.symResizeUp();
            selection.moveRight();
            selection.moveDown();
            QVERIFY(parent.rect().contains(selection.geometry()));
            QVERIFY(hasRatio(selection.geometry(), 16.0 / 9));
        }
        selection.setGeometry(parent.rect());
        QVERIFY(parent.rect().contains(selection.geometry()));
        QVERIFY(hasRatio(selection.geometry(), 16.0 / 9));
        selection.setGeometryAnimated(parent.rect());
        QVERIFY(hasRatio(selection.geometry(), 16.0 / 9));
        selection.setAspectRatio(0);
        selection.setGeometry({ 10, 20, 500, 100 });
        QCOMPARE(selection.geometry(), QRect(10, 20, 500, 100));
    }
    void modifiersAndFreeMode()
    {
        QWidget parent;
        parent.resize(1000, 800);
        SelectionWidget selection(Qt::red, &parent);
        parent.show();
        selection.hide();
        drag(parent, { 100, 100 }, { 450, 250 });
        QCOMPARE(selection.geometry(), QRect(100, 100, 351, 151));
        drag(parent, { 450, 250 }, { 700, 550 }, Qt::ControlModifier);
        QVERIFY(hasRatio(selection.geometry(), 351.0 / 151));
        selection.setAspectRatio(1);
        QRect r = selection.geometry();
        drag(parent, r.bottomRight(), { 700, 600 }, Qt::ControlModifier);
        QVERIFY(hasRatio(selection.geometry(), 1));
        selection.setAspectRatio(0);
        selection.hide();
        drag(parent, { 100, 100 }, { 450, 250 }, Qt::ControlModifier);
        QVERIFY(hasRatio(selection.geometry(), 1));
    }
    void freeSelectionPixelBounds()
    {
        QWidget parent;
        parent.resize(1000, 800);
        SelectionWidget selection(Qt::red, &parent);
        parent.show();
        const QPoint start(400, 300);
        for (QPoint end : { QPoint(600, 500),
                            QPoint(100, 50),
                            QPoint(100, 500),
                            QPoint(600, 50) }) {
            selection.hide();
            drag(parent, start, end);
            // QRect::normalized() uses outer edges when dimensions are
            // negative; construct ordered pixel endpoints to include both.
            const QPoint low(qMin(start.x(), end.x()),
                             qMin(start.y(), end.y()));
            const QPoint high(qMax(start.x(), end.x()),
                              qMax(start.y(), end.y()));
            QCOMPARE(selection.geometry(), QRect(low, high));
            QVERIFY(selection.geometry().contains(start));
            QVERIFY(selection.geometry().contains(end));
        }
    }

    void sessionAndInvalidRatios()
    {
        QWidget parent;
        parent.resize(800, 600);
        SelectionWidget selection(Qt::red, &parent);
        QSignalSpy changed(&selection, &SelectionWidget::aspectRatioChanged);
        QCOMPARE(selection.aspectRatio(), 0.0);
        selection.setAspectRatio(1);
        selection.setAspectRatio(1);
        selection.setAspectRatio(-1);
        selection.setAspectRatio(std::numeric_limits<double>::infinity());
        selection.setAspectRatio(std::numeric_limits<double>::quiet_NaN());
        QCOMPARE(changed.count(), 1);
        QCOMPARE(selection.aspectRatio(), 1.0);
        SelectionWidget nextCapture(Qt::red, &parent);
        QCOMPARE(nextCapture.aspectRatio(), 0.0);
    }
};
QTEST_MAIN(SelectionTest)
#include "selectionwidget_test.moc"

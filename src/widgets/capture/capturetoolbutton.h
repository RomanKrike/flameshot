// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2017-2019 Alejandro Sirgo Rica & Contributors

#pragma once

#include "capturebutton.h"
#include "tools/capturetool.h"

#include <QMap>
#include <QVector>

class QWidget;
class QPropertyAnimation;

class CaptureToolButton : public CaptureButton
{
    Q_OBJECT

public:
    explicit CaptureToolButton(const CaptureTool::Type,
                               QWidget* parent = nullptr);
    ~CaptureToolButton();

    static const QList<CaptureTool::Type>& getIterableButtonTypes();
    static int getPriorityByButton(CaptureTool::Type);

    QString name() const;
    QString description() const;
    QIcon icon() const;
    CaptureTool* tool() const;

    void setColor(const QColor& c);
    void animatedShow();
    void setIconLabel(const QString& label);

protected:
    void mousePressEvent(QMouseEvent* e) override;
    void wheelEvent(QWheelEvent* e) override;
    static QList<CaptureTool::Type> iterableButtonTypes;

    CaptureTool* m_tool;

signals:
    void pressedButtonLeftClick(CaptureToolButton*);
    void pressedButtonRightClick(CaptureToolButton*);
    void wheelSteps(int steps);

private:
    CaptureToolButton(QWidget* parent = nullptr);
    CaptureTool::Type m_buttonType;
    QString m_iconLabel;
    int m_wheelDelta{ 0 };

    QPropertyAnimation* m_emergeAnimation;

    static QColor m_mainColor;

    void initButton();
    void updateIcon();
};

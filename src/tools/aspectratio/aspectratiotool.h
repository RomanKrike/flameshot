// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "tools/abstractactiontool.h"

class AspectRatioTool : public AbstractActionTool
{
    Q_OBJECT
public:
    explicit AspectRatioTool(QObject* parent = nullptr);
    bool closeOnButtonPressed() const override;
    QIcon icon(const QColor& background, bool inEditor) const override;
    QString name() const override;
    QString description() const override;
    CaptureTool::Type type() const override;
    CaptureTool* copy(QObject* parent = nullptr) override;

public slots:
    void pressed(CaptureContext& context) override;
};

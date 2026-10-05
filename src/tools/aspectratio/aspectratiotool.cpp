// SPDX-License-Identifier: GPL-3.0-or-later

#include "aspectratiotool.h"

AspectRatioTool::AspectRatioTool(QObject* parent)
  : AbstractActionTool(parent)
{}

bool AspectRatioTool::closeOnButtonPressed() const
{
    return false;
}

QIcon AspectRatioTool::icon(const QColor& background, bool inEditor) const
{
    Q_UNUSED(inEditor)
    return QIcon(iconPath(background) + "aspect-ratio.svg");
}

QString AspectRatioTool::name() const
{
    return tr("Selection Aspect Ratio");
}

QString AspectRatioTool::description() const
{
    return tr("Choose the selection aspect ratio");
}

CaptureTool::Type AspectRatioTool::type() const
{
    return CaptureTool::TYPE_ASPECTRATIO;
}

CaptureTool* AspectRatioTool::copy(QObject* parent)
{
    return new AspectRatioTool(parent);
}

void AspectRatioTool::pressed(CaptureContext& context)
{
    Q_UNUSED(context)
    emit requestAction(REQ_SHOW_ASPECT_RATIO_MENU);
}

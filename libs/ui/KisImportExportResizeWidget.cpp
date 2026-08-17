/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisImportExportResizeWidget.h>

#include "imagesize/wdg_imagesize.h"

namespace
{
class ImageSizeResizeWidget final : public KisImportExportResizeWidget
{
public:
    ImageSizeResizeWidget(QWidget *parent, int width, int height, double resolution)
        : m_widget(new WdgImageSize(parent, width, height, resolution))
    {
    }

    QWidget *widget() const override
    {
        return m_widget;
    }

    QSize desiredSize() const override
    {
        return QSize(m_widget->desiredWidth(), m_widget->desiredHeight());
    }

    double desiredResolution() const override
    {
        return m_widget->desiredResolution();
    }

    KisFilterStrategy *filterType() const override
    {
        return m_widget->filterType();
    }

private:
    WdgImageSize *m_widget;
};
}

std::unique_ptr<KisImportExportResizeWidget> createImportExportResizeWidget(
    QWidget *parent,
    int width,
    int height,
    double resolution)
{
    return std::make_unique<ImageSizeResizeWidget>(parent, width, height, resolution);
}

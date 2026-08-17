/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_IMPORT_EXPORT_RESIZE_WIDGET_H
#define KIS_IMPORT_EXPORT_RESIZE_WIDGET_H

#include <memory>

#include <QSize>

#include <kritaui_export.h>

class KisFilterStrategy;
class QWidget;

class KRITAUI_EXPORT KisImportExportResizeWidget
{
public:
    virtual ~KisImportExportResizeWidget() = default;

    virtual QWidget *widget() const = 0;
    virtual QSize desiredSize() const = 0;
    virtual double desiredResolution() const = 0;
    virtual KisFilterStrategy *filterType() const = 0;
};

KRITAUI_EXPORT std::unique_ptr<KisImportExportResizeWidget> createImportExportResizeWidget(
    QWidget *parent,
    int width,
    int height,
    double resolution);

#endif

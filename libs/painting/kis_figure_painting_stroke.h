/*
 *  SPDX-FileCopyrightText: 2011 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_FIGURE_PAINTING_STROKE_H
#define KIS_FIGURE_PAINTING_STROKE_H

#include <QPainterPath>
#include <QPen>
#include <QTransform>
#include <optional>
#include <QLineF>

#include <KisFigurePaintingOptions.h>
#include <brushengine/kis_paint_information.h>
#include <kis_resources_snapshot.h>
#include <kis_paint_device.h>
#include <kis_types.h>
#include <kritapainting_export.h>

class KoCanvasResourceProvider;
class KUndo2MagicString;
class KisStrokesFacade;

/**
 * Owns one figure-painting stroke from creation through final update.
 *
 * The image and node are retained by the resource snapshot. The canvas
 * resource provider is borrowed only while the constructor captures its
 * resource interface. Destruction completes the stroke.
 */
class KRITAPAINTING_EXPORT KisFigurePaintingStroke
{
public:
    KisFigurePaintingStroke(
        const KUndo2MagicString &name,
        KisImageWSP image,
        KisNodeSP currentNode,
        KoCanvasResourceProvider *resourceManager,
        KisFigurePaintingOptions::StrokeStyle strokeStyle,
        KisFigurePaintingOptions::FillStyle fillStyle,
        QTransform fillTransform = QTransform());
    /** Creates a stroke from an independent copy of the captured resources. */
    KisFigurePaintingStroke(
        const KUndo2MagicString &name,
        const KisResourcesSnapshot &resources,
        std::optional<int> dabRandomSeed = std::nullopt);
    ~KisFigurePaintingStroke();

    KisFigurePaintingStroke(const KisFigurePaintingStroke &) = delete;
    KisFigurePaintingStroke &operator=(const KisFigurePaintingStroke &) = delete;

    void paintLine(const KisPaintInformation &pi0,
                   const KisPaintInformation &pi1);
    void paintPolyline(const vQPointF &points);
    void paintPolygon(const vQPointF &points);
    void paintRect(const QRectF &rect);
    void paintEllipse(const QRectF &rect);
    void paintPainterPath(const QPainterPath &path);
    /** Queues brush segments with the supplied input held constant along the path. */
    void paintStrokePath(const QPainterPath &path, const KisPaintInformation &information);
    struct PreviewSample {
        KisPaintDeviceSP device;
        QRect bounds;
        QLineF line;
        qreal imageUnitsPerPixel;
    };
    /** Builds one bounded appearance sample on a scratch device, without document jobs or Undo. */
    static PreviewSample createPreviewSample(const KisResourcesSnapshot &resources,
                                              const KisPaintInformation &information, int randomSeed);
    void setFGColorOverride(const KoColor &color);
    void setBGColorOverride(const KoColor &color);
    void setSelectionOverride(KisSelectionSP m_selection);
    void setBrush(const KisPaintOpPresetSP &brush);
    void paintPainterPathQPen(const QPainterPath path,
                              const QPen &pen,
                              const KoColor &color);
    void paintPainterPathQPenFill(const QPainterPath path,
                                  const QPen &pen,
                                  const KoColor &color);

private:
    void startStroke(const KUndo2MagicString &name, std::optional<int> dabRandomSeed = std::nullopt);
    void setupPaintStyles(KisResourcesSnapshotSP resources,
                          KisFigurePaintingOptions::StrokeStyle strokeStyle,
                          KisFigurePaintingOptions::FillStyle fillStyle,
                          QTransform fillTransform);

private:
    KisStrokeId m_strokeId;
    KisResourcesSnapshotSP m_resources;
    KisStrokesFacade *m_strokesFacade;
};

#endif

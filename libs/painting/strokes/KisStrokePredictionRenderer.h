/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_STROKE_PREDICTION_RENDERER_H
#define KIS_STROKE_PREDICTION_RENDERER_H

#include <QPointF>
#include <QRect>
#include <QScopedPointer>
#include <QString>
#include <QVector>

#include "kis_types.h"
#include "kis_resources_snapshot.h"
#include "kritapainting_export.h"

class KisDistanceInformation;
class KisPaintInformation;
class KisSelection;
class KisStrokeRandomSource;

/**
 * Draws the predicted part of a freehand stroke with the same brush
 * processing and the same layer composition path as the confirmed part.
 *
 * The prediction continues the last confirmed state: the accumulated
 * distance, the time and the random state are copied from the confirmed
 * stroke before every render. The copy is advanced by the rendering and
 * the confirmed stroke state is not changed.
 *
 * A copy of KisStrokeRandomSource shares its generators, so the renderer
 * copies the generators themselves and leaves the confirmed stroke's
 * random sequence unadvanced.
 *
 * Every render discards the previous prediction and draws all points again
 * from the confirmed pixels. Drawing on top of the previous prediction
 * would change the density of semi-transparent brushes and leave marks of
 * a wrong prediction.
 *
 * The brush processing is the same as the confirmed stroke: the painter is
 * configured from the same resources snapshot and paints with a paint op
 * created from the same preset.
 *
 * The renderer alternates between two devices because the layer projection
 * may still read the result of the previous render while the next result is
 * written.
 */
class KRITAPAINTING_EXPORT KisStrokePredictionRenderer
{
public:
    struct Result {
        KisPaintDeviceSP device; // holds the source pixels of rect plus the predicted dabs
        QRect rect;              // empty when nothing was predicted
        int dabCount = 0;
    };

    /// source is the device the stroke paints into: the temporary target with
    /// indirect painting, the layer device otherwise.
    KisStrokePredictionRenderer(KisResourcesSnapshotSP resources,
                                KisPaintDeviceSP source,
                                KisSelectionSP selection,
                                bool indirectPainting,
                                const QString &indirectPaintingCompositeOp);
    ~KisStrokePredictionRenderer();

    Result render(const KisPaintInformation &lastReal,
                  const KisDistanceInformation &distance,
                  const KisStrokeRandomSource &random,
                  const QVector<QPointF> &points,
                  const QVector<qreal> &dtMs);

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KIS_STROKE_PREDICTION_RENDERER_H

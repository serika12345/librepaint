/*
 *  SPDX-FileCopyrightText: 2007 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_SELECTION_TOOL_HELPER_H
#define KIS_SELECTION_TOOL_HELPER_H

#include <kritaui_export.h>
#include <QList>
#include <QPainterPath>
#include <QPointer>
#include <QSize>

#include <gpu/KisGpuSelectionMask.h>

#include "kundo2magicstring.h"
#include "kis_selection.h"

class KoCanvasBase;
class KoShape;
class KisCanvas2;
class KisProcessingApplicator;
class QMenu;

/**
 * XXX: Doc!
 */
class KRITAUI_EXPORT KisSelectionToolHelper
{
public:

    KisSelectionToolHelper(KisCanvas2* canvas, const KUndo2MagicString& name);
    virtual ~KisSelectionToolHelper();

    void selectPixelSelection(KisProcessingApplicator& applicator, KisPixelSelectionSP selection, SelectionAction action);
    void selectPixelSelection(KisPixelSelectionSP selection, SelectionAction action);

    void addSelectionShape(KoShape* shape, SelectionAction action = SELECTION_DEFAULT);
    void addSelectionShapes(QList<KoShape*> shapes, SelectionAction action = SELECTION_DEFAULT);

    bool canShortcutToDeselect(const QRect &rect, SelectionAction action);
    bool canShortcutToNoop(const QRect &rect, SelectionAction action);

    bool tryDeselectCurrentSelection(const QRectF selectionViewRect, SelectionAction action);
    static QMenu* getSelectionContextMenu(KoCanvasBase* canvas);

    SelectionMode tryOverrideSelectionMode(KisSelectionSP activeSelection, SelectionMode currentMode, SelectionAction currentAction) const;

    /** Rasterizes the closed polygon `vertices`, in document pixels, into
     * frozen binary GPU coverage clipped to `imageSize`. Null reports fewer
     * than three vertices, non-finite coordinates or no selected pixel.
     */
    static LibrePaint::Image::Gpu::SelectionMaskSP gpuSelectionMaskFromPolygon(
        const QList<QPointF> &vertices, const QSize &imageSize);

    /** Flattens the closed `path`, in document pixels, into one polygon of
     * vertices for gpuSelectionMaskFromPolygon(). Empty reports no contour, a
     * path with several contours, or a degenerate polygon instead of silently
     * approximating part of the path.
     */
    static QList<QPointF> gpuSelectionPolygonVertices(const QPainterPath &path);


private:
    QPointer<KisCanvas2> m_canvas;
    KisImageSP m_image;
    KisLayerSP m_layer;
    KUndo2MagicString m_name;
};


#endif

/*
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_TOOL_CANVAS_H
#define KIS_TOOL_CANVAS_H

#include <kritacanvas_export.h>
#include <kis_types.h>

class KisCoordinatesConverter;
class KisOptimizedBrushOutline;
class QCursor;
class QObject;
class QPainter;
class QString;

/**
 * Canvas operations required by the concrete tool implementation.
 *
 * The canvas owns this object. Tools borrow it for their activation lifetime;
 * the KoCanvasBase passed to a KisTool implements this contract.
 */
class KRITACANVAS_EXPORT KisToolCanvas
{
public:
    virtual ~KisToolCanvas() = default;

    virtual const KisCoordinatesConverter *coordinatesConverter() const = 0;
    virtual KisImageWSP currentImage() const = 0;
    virtual KisSelectionSP currentSelectionForTool() const = 0;
    virtual KisNodeList selectedNodesForTool() const = 0;
    virtual void attachPriorityEventFilterForTool(QObject *filter, int priority = 0) = 0;
    virtual void detachPriorityEventFilterForTool(QObject *filter) = 0;
    virtual void requestStrokeEndForTool() = 0;
    virtual void requestStrokeCancellationForTool() = 0;
    virtual bool blockUntilOperationsFinishedForTool(KisImageSP image) = 0;
    virtual void blockUntilOperationsFinishedForToolForced(KisImageSP image) = 0;
    virtual bool selectionEditableForTool() const = 0;
    virtual bool selectionModifierMappingSwapsCtrlAndAltForTool() const = 0;
    virtual QCursor moveSelectionCursorForTool() const = 0;
    virtual void showToolMessage(const QString &message, const QString &iconName) = 0;
    virtual void drawToolOutline(QPainter *painter,
                                 const KisOptimizedBrushOutline &path,
                                 int thickness) = 0;
    virtual QObject *toolConfigNotifier() const = 0;
};

#endif

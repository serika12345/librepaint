/*
 *  SPDX-FileCopyrightText: 2003 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_TOOL_PAINT_INTERACTION_H
#define KIS_TOOL_PAINT_INTERACTION_H

#include <QScopedPointer>

#include <KisOptimizedBrushOutline.h>
#include <brushengine/kis_paintop_settings.h>
#include <kis_tool.h>
#include <kis_tool_utils.h>
#include <kritatools_export.h>

/**
 * Input state and brush operations shared by painting tools.
 *
 * The UI-owned subclass supplies outline presentation and optional color
 * sampling while this class owns pointer tracking, brush size and rotation,
 * and brush-outline generation.
 */
class KRITATOOLS_EXPORT KisToolPaintInteraction : public KisTool
{
    Q_OBJECT

public:
    KisToolPaintInteraction(KoCanvasBase *canvas, const QCursor &cursor);
    ~KisToolPaintInteraction() override;

    int flags() const override;

    void mousePressEvent(KoPointerEvent *event) override;
    void mouseReleaseEvent(KoPointerEvent *event) override;
    void mouseMoveEvent(KoPointerEvent *event) override;

public Q_SLOTS:
    void activate(const QSet<KoShape *> &shapes) override;
    void deactivate() override;

protected:
    void setMode(ToolMode mode) override;
    void canvasResourceChanged(int key, const QVariant &value) override;

    void activatePrimaryAction() override;
    void deactivatePrimaryAction() override;

    virtual void requestUpdateOutline(const QPointF &outlineDocPoint,
                                      const KoPointerEvent *event) = 0;

    void setSupportOutline(bool supportOutline);
    bool supportsOutline() const;

    virtual KisOptimizedBrushOutline getOutlinePath(
        const QPointF &documentPos,
        const KoPointerEvent *event,
        KisPaintOpSettings::OutlineMode outlineMode);

    bool isOutlineEnabled() const;
    void setOutlineEnabled(bool enabled);
    bool isOutlineVisible() const;
    void setOutlineVisible(bool visible);

    bool isEraser() const;

    QPointF m_outlineDocPoint;
    KisOptimizedBrushOutline m_currentOutline;

private Q_SLOTS:
    void increaseBrushSize();
    void decreaseBrushSize();
    void rotateBrushTipClockwise();
    void rotateBrushTipClockwisePrecise();
    void rotateBrushTipCounterClockwise();
    void rotateBrushTipCounterClockwisePrecise();

private:
    bool m_supportOutline {false};
    bool m_isOutlineEnabled {true};
    bool m_isOutlineVisible {true};
    KisToolUtils::StandardBrushSizes m_standardBrushSizes;

    struct Private;
    QScopedPointer<Private> m_d;

Q_SIGNALS:
    void sigPaintingFinished();
};

#endif

/*
 *  SPDX-FileCopyrightText: 2003 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_TOOL_PAINT_H_
#define KIS_TOOL_PAINT_H_

#include <QGridLayout>

#include <KisAsyncColorSamplerHelper.h>
#include <kis_tool_paint_interaction.h>
#include <kritaui_export.h>

class QLayout;
class QWidget;

/**
 * Painting-tool presentation and color-sampling integration.
 *
 * Pointer state, brush operations, and outline generation are owned by
 * KisToolPaintInteraction in kritatools.
 */
class KRITAUI_EXPORT KisToolPaint : public KisToolPaintInteraction
{
    Q_OBJECT

public:
    KisToolPaint(KoCanvasBase *canvas, const QCursor &cursor);
    ~KisToolPaint() override;

    KisPopupWidgetInterface *popupWidget() override;

public Q_SLOTS:
    void activate(const QSet<KoShape *> &shapes) override;
    void deactivate() override;

protected:
    void paint(QPainter &gc, const KoViewConverter &converter) override;

    void activateAlternateAction(AlternateAction action) override;
    void deactivateAlternateAction(AlternateAction action) override;

    void beginAlternateAction(KoPointerEvent *event,
                              AlternateAction action) override;
    void continueAlternateAction(KoPointerEvent *event,
                                 AlternateAction action) override;
    void endAlternateAction(KoPointerEvent *event,
                            AlternateAction action) override;

    void requestUpdateOutline(const QPointF &outlineDocPoint,
                              const KoPointerEvent *event) override;

    void addOptionWidgetLayout(QLayout *layout);
    virtual void addOptionWidgetOption(QWidget *control,
                                       QWidget *label = nullptr);

    void showControl(QWidget *control, bool value);
    void enableControl(QWidget *control, bool value);

    QWidget *createOptionWidget() override;

    virtual QString quickHelp() const
    {
        return QString();
    }

private Q_SLOTS:
    void slotColorPickerRequestedCursor(const QCursor &cursor);
    void slotColorPickerRequestedCursorReset();
    void slotColorPickerRequestedOutlineUpdate();
    void slotPopupQuickHelp();
    void showBrushSize();

private:
    KisOptimizedBrushOutline tryFixBrushOutline(
        const KisOptimizedBrushOutline &originalOutline);
    bool isSamplingAction(AlternateAction action) const;

    QGridLayout *m_optionsWidgetLayout {nullptr};
    QRectF m_oldOutlineRect;
    QRectF m_oldColorPreviewUpdateRect;
    KisAsyncColorSamplerHelper m_colorSamplerHelper;
};

#endif

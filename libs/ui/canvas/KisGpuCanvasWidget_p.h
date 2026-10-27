/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CANVAS_WIDGET_P_H
#define KIS_GPU_CANVAS_WIDGET_P_H

#include "kis_canvas_widget_base.h"
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuTilePresentation.h>
#include <QWidget>

namespace LibrePaint::Canvas::Gpu { class CanvasWindow; }
class KisOptimizedBrushOutline;

/** Embeds the shared GPU presentation in the ordinary document canvas.
 * Native input uses the existing canvas input dispatcher. The borrowed
 * execution owner outlives the widget; native surfaces are released before
 * the widget releases its shared image reference.
 */
class KisGpuCanvasWidget final : public QWidget, public KisCanvasWidgetBase
{
public:
    KisGpuCanvasWidget(KisCanvas2 *canvas, KisCoordinatesConverter *converter,
                       LibrePaint::Image::Gpu::PaintImageSP image, QWidget *parent);
    ~KisGpuCanvasWidget() override;
    void disconnectImage();
    void collectToolOutline(const KisOptimizedBrushOutline &path, int thickness);
    bool collectCropPreview(LibrePaint::Canvas::Gpu::CropPreview crop);
    QWidget *widget() override { return this; }
    void setDisplayFilter(QSharedPointer<KisDisplayFilter>) override;
    void notifyImageColorSpaceChanged(const KoColorSpace *) override;
    void setWrapAroundViewingMode(bool) override;
    bool wrapAroundViewingMode() const override { return false; }
    void setWrapAroundViewingModeAxis(WrapAroundAxis) override;
    WrapAroundAxis wrapAroundViewingModeAxis() const override { return WRAPAROUND_BOTH; }
    void channelSelectionChanged(const QBitArray &) override;
    void setDisplayConfig(const KisDisplayConfig &) override;
    void finishResizingImage(qint32, qint32) override;
    KisUpdateInfoSP startUpdateCanvasProjection(const QRect &rect) override;
    QRect updateCanvasProjection(KisUpdateInfoSP info) override;
    using KisCanvasWidgetBase::updateCanvasProjection;
    void updateCanvasImage(const QRect &) override;
    void updateCanvasDecorations(const QRect &) override;
    bool isBusy() const override { return false; }
    void setLodResetInProgress(bool) override {}

protected:
    bool eventFilter(QObject *object, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;
    void inputMethodEvent(QInputMethodEvent *event) override;
    bool callFocusNextPrevChild(bool next) override;

private:
    void refresh();
    void updateBackground();
    bool updateDisplayConversion();
    bool m_displayConversionDirty = true;
    std::optional<KoColorConversionStages::Encoding> m_displayEncoding;
    bool updateToolDecorations();
    LibrePaint::Canvas::Gpu::ToolOutline m_toolOutline;
    std::optional<LibrePaint::Canvas::Gpu::CropPreview> m_cropPreview;
    bool m_collectingToolDecorations = false;
    LibrePaint::Image::Gpu::PaintImageSP m_image;
    LibrePaint::Canvas::Gpu::CanvasWindow *m_window;
    QWidget *m_container;
};
#endif

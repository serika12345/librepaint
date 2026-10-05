/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KRITA_CANVAS_WGPU_CANVAS_H
#define KRITA_CANVAS_WGPU_CANVAS_H

#include "../kis_qpainter_canvas.h"
#include <QImage>
#include <memory>

namespace Krita::Canvas
{
/** Experimental 8-bit canvas. CPU projection/color conversion and decoration
 * composition feed dirty display regions to the native GPU presentation window.
 */
class KRITAUI_EXPORT WgpuCanvas final : public KisQPainterCanvas
{
public:
    WgpuCanvas(KisCanvas2 *canvas, KisCoordinatesConverter *converter, QWidget *parent);
    ~WgpuCanvas() override;
    QImage capturedFrame();
    QImage composedFrame() const;
    quint64 uploadedBytes() const;
    quint64 submittedFrames() const;
    QString presentationError() const;

    void updateCanvasImage(const QRect &rect) override;
    void updateCanvasDecorations(const QRect &rect) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    KisUpdateInfoSP startUpdateCanvasProjection(const QRect &rect) override;
    QRect updateCanvasProjection(KisUpdateInfoSP info) override;

private:
    KisUpdateInfoSP captureProjection(const QRect &rect,bool forceCapture);
    void scheduleFrame(const QRect &rect);
    void compose();
    struct Private;
    std::unique_ptr<Private> m_wgpu;
};
}
#endif

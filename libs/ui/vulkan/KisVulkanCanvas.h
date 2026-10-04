/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_VULKAN_CANVAS_H
#define KIS_VULKAN_CANVAS_H

#include "canvas/kis_qpainter_canvas.h"
#include <memory>

class KisVulkanFrameWindow;

/** Experimental presentation of the CPU canvas. Widget owns instance and window. */
class KisVulkanCanvas final : public KisQPainterCanvas
{
    Q_OBJECT
public:
    KisVulkanCanvas(KisCanvas2 *canvas, KisCoordinatesConverter *converter, QWidget *parent);
    ~KisVulkanCanvas() override;
    QString initializationError() const;
    void updateCanvasImage(const QRect &rect) override;
    void updateCanvasDecorations(const QRect &rect) override;
    QString currentBitDepthUserReport() const override;

Q_SIGNALS:
    void presentationFailed(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    void requestFrame();
    void composeFrame();
    struct Private;
    std::unique_ptr<Private> m_d;
};

#endif

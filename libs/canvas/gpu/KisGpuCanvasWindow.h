/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_CANVAS_WINDOW_H
#define KIS_GPU_CANVAS_WINDOW_H

#include <kritacanvas_export.h>
#include <gpu/KisGpuExecution.h>
#include <QTransform>
#include <QWindow>

namespace LibrePaint::Canvas::Gpu {
/**
 * Presents a document using the borrowed shared execution owner, which must
 * outlive this window. All operations and destruction use the GUI/GPU thread.
 * Qt update requests coalesce presentation; accepted input remains in the
 * document owner. Transform coordinates are logical window pixels.
 *
 * Surface resources retain their device through native window destruction.
 * resetSurface() recreates that connection after device replacement, including
 * hidden windows. Active frames reject reset. Frame notifications identify
 * the document and device generation that actually completed submission.
 */
class KRITACANVAS_EXPORT CanvasWindow : public QWindow
{
    Q_OBJECT
public:
    explicit CanvasWindow(Image::Gpu::Execution &execution, QWindow *parent = nullptr);
    ~CanvasWindow() override;

    /** Zero suspends drawing; a closed or unknown handle preserves selection. */
    bool setDocument(Image::Gpu::DocumentHandle handle);
    void refresh();
    bool setDocumentToWindow(const QTransform &transform);
    QTransform documentToWindow() const;
    QRect documentViewport() const;
    bool resetSurface();

Q_SIGNALS:
    void frameSubmitted(quint64 handle, quint64 revision, quint64 generation);
    void renderingFailed(quint64 handle, quint64 generation,
                         LibrePaint::Image::Gpu::ExecutionCode code, const QString &diagnostic);

protected:
    bool event(QEvent *event) override;
    void exposeEvent(QExposeEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    struct Private;
    std::unique_ptr<Private> m_d;
    void renderFrame();
    void connectSurface();
};
}
#endif

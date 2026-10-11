/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_CANVAS_SESSION_H
#define KIS_GPU_CANVAS_SESSION_H
#include "KisGpuApplicationState.h"
#include "KisGpuDocumentDevice.h"
#include "KisGpuStrokeInput.h"
#include "DocumentCanvas.h"
#include <QTimer>
class KisGpuPointerInput;

/** Application ownership of one window's GPU input and display resources.
 * The shared document owner, application notifications and window outlive this
 * session. Calls and destruction use their GUI/submitting thread. Closing the
 * session cancels its unfinished input; closing the document detaches all views.
 */
class KisGpuCanvasSession : public QObject
{
    Q_OBJECT
public:
    enum class Result { Idle, Updated, Busy, Closed, GpuRejected, WindowUnavailable, DeviceLost };
    /** Unknown document or invalid canvas bounds throw std::invalid_argument. */
    KisGpuCanvasSession(KisGpuDocumentDevice &documents, KisGpuApplicationState &application,
                        KisGpuDocumentDevice::DocumentId id, QWindow &window, QRect bounds,
                        QObject *parent = nullptr);
    ~KisGpuCanvasSession() override;
    KisGpuStrokeInput::Started begin(KisGpuLayerDocument::LayerId layer,
        KisGpuStrokeInput::Configuration configuration,
        KisGpuLayerDocument::Target target = KisGpuLayerDocument::Target::Pixels);
    KisGpuStrokeInput::Result append(const KisGpuStrokeInput::Token &token,
                                    const QVector<KisGpuBrushStroke::Sample> &samples);
    KisGpuStrokeInput::Result replace(const KisGpuStrokeInput::Token &token,
                                     const QVector<KisGpuBrushStroke::Sample> &samples);
    KisGpuStrokeInput::Result commit(const KisGpuStrokeInput::Token &token);
    KisGpuStrokeInput::Result cancel(const KisGpuStrokeInput::Token &token);
    KisGpuStrokeInput::State inputState() const;
    bool hasPendingInput() const;
    void finishPointerInput();
    bool setPointerTool(KisGpuLayerDocument::LayerId layer, KisGpuStrokeInput::Configuration configuration,
                        KisGpuLayerDocument::Target target = KisGpuLayerDocument::Target::Pixels);
    /** Invalid transforms preserve the input mapping and displayed view. */
    bool setView(const Krita::Canvas::GpuRenderer::View &view);
    /** The exposed window also advances automatically through a 16ms GUI timer. */
    Result poll();
    Krita::Canvas::SurfaceRenderer::Frame lastFrame() const;
    /** Statistics belong to the current GPU lifetime. */
    Krita::Canvas::SurfaceRenderer::Statistics statistics() const;
    QString lastError() const { return m_error; }
Q_SIGNALS:
    void documentClosed();
    void inputRejected(KisGpuStrokeInput::Result result);
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    void releaseGpu();
    void updatePolling();
    KisGpuDocumentDevice &m_documents;
    KisGpuApplicationState &m_application;
    const KisGpuDocumentDevice::DocumentId m_id;
    QWindow &m_window;
    const QRect m_bounds;
    std::shared_ptr<KisGpuLayerDocument> m_model;
    std::unique_ptr<KisGpuStrokeInput> m_input;
    std::unique_ptr<KisGpuPointerInput> m_pointer;
    std::unique_ptr<Krita::Canvas::DocumentCanvas> m_canvas;
    Krita::Canvas::GpuRenderer::View m_view;
    QTimer m_poll;
    QString m_error;
};
#endif

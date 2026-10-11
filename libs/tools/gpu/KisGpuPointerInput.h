/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_POINTER_INPUT_H
#define KIS_GPU_POINTER_INPUT_H
#include "KisGpuStrokeInput.h"
#include <QObject>
#include <QTransform>
#include <QTimer>
class QWindow;
class KisQuickShapeTracker;

/** Mouse and tablet interpretation with bounded CPU waiting before an edit begins.
 * The window and stroke input outlive this object on the GUI/submitting thread.
 * poll() queues normalized input; the caller advances the concrete stroke input.
 */
class KisGpuPointerInput : public QObject
{
    Q_OBJECT
public:
    KisGpuPointerInput(KisGpuStrokeInput &input, QWindow &window, qsizetype maximumSamples = 1024,
                       QObject *parent = nullptr);
    ~KisGpuPointerInput() override;
    /** A valid tool change cancels this object's unfinished operation. Busy preserves the old tool. */
    bool setTool(KisGpuLayerDocument::LayerId layer, KisGpuStrokeInput::Configuration configuration,
                 KisGpuLayerDocument::Target target = KisGpuLayerDocument::Target::Pixels);
    /** Finite, invertible affine mapping from logical window coordinates to canvas coordinates. */
    bool setWindowToCanvas(const QTransform &transform);
    /** A mode change cancels unfinished input; a committing operation preserves the mode. */
    bool setQuickShapeEnabled(bool enabled);
    bool quickShapeActive() const;
    KisGpuStrokeInput::Result poll();
    KisGpuStrokeInput::Result cancel();
    /** Finish accepted input before a document command such as save. */
    void requestCommit();
    bool active() const { return m_active; }
    /** Queued events or completion that the input scheduler must advance. */
    bool hasPendingWork() const {
        return m_active && (!m_started || !m_queue.empty() || m_shapePending || m_released);
    }
Q_SIGNALS:
    /** Start and further input allow the application owner to resume its GPU and timers. */
    void inputReady();
    void inputRejected(KisGpuStrokeInput::Result result);
protected:
    bool eventFilter(QObject *object, QEvent *event) override;
private:
    enum class Source { None, Mouse, Tablet };
    struct Tool {
        KisGpuLayerDocument::LayerId layer = 0;
        KisGpuStrokeInput::Configuration configuration;
        KisGpuLayerDocument::Target target = KisGpuLayerDocument::Target::Pixels;
    };
    void start(KisGpuBrushStroke::Sample sample, Source source, bool eraser = false);
    void queue(KisGpuBrushStroke::Sample sample);
    void finish();
    void reject(KisGpuStrokeInput::Result result);
    void recognizeQuickShape();
    QVector<KisGpuBrushStroke::Sample> quickShapeSamples() const;
    KisGpuStrokeInput &m_input;
    QWindow &m_window;
    const qsizetype m_maximumSamples;
    QTransform m_transform;
    Tool m_tool, m_frozen;
    KisGpuStrokeInput::Token m_token;
    std::deque<KisGpuBrushStroke::Sample> m_queue;
    std::unique_ptr<KisQuickShapeTracker> m_quickShape;
    QTimer m_hold;
    qreal m_shapePressure = 1;
    bool m_quickShapeEnabled = false, m_shapePending = false;
    Source m_source = Source::None;
    bool m_active = false, m_started = false, m_released = false, m_committed = false;
};
#endif

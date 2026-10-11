/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_APPLICATION_STATE_H
#define KIS_GPU_APPLICATION_STATE_H
#include <QObject>
#include <QTimer>
#include <memory>

class KisGpuDocumentDevice;

/** GUI-thread application notifications for a borrowed, longer-lived document owner.
 * Native memory notifications are disconnected before this object is destroyed.
 * Input consumers call resumeForInput() before retrying after critical pressure.
 */
class KisGpuApplicationState : public QObject
{
    Q_OBJECT
public:
    explicit KisGpuApplicationState(KisGpuDocumentDevice &documents, QObject *parent = nullptr);
    ~KisGpuApplicationState() override;
    bool resumeForInput();
    bool isPolling() const { return m_poll.isActive(); }
public Q_SLOTS:
    void applicationStateChanged(Qt::ApplicationState state);
    /** 0: recovered, 50..254: release history, 255: suspend GPU resources. */
    void memoryPressure(uchar level);
    void shutdown();
private:
    struct MemoryNotifications;
    void updatePolling();
    void poll();
    KisGpuDocumentDevice &m_documents;
    QTimer m_poll;
    std::unique_ptr<MemoryNotifications> m_memory;
    Qt::ApplicationState m_applicationState = Qt::ApplicationInactive;
    bool m_critical = false, m_stopped = false, m_resumeFailed = false;
};
#endif

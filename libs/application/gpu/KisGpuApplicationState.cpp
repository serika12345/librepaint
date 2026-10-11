/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuApplicationState.h"
#include "KisGpuDocumentDevice.h"
#include <QGuiApplication>

#if defined(Q_OS_DARWIN)
#include <dispatch/dispatch.h>
#elif defined(Q_OS_WIN)
#include <windows.h>
#include <QWinEventNotifier>
#elif defined(KIS_GPU_MEMORY_DBUS)
#include <QDBusConnection>
#endif

struct KisGpuApplicationState::MemoryNotifications {
#if defined(Q_OS_DARWIN)
    KisGpuApplicationState *owner;
    dispatch_source_t source = nullptr;
    explicit MemoryNotifications(KisGpuApplicationState *state) : owner(state) {
        source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MEMORYPRESSURE, 0,
            DISPATCH_MEMORYPRESSURE_NORMAL | DISPATCH_MEMORYPRESSURE_WARN | DISPATCH_MEMORYPRESSURE_CRITICAL,
            dispatch_get_main_queue());
        if (!source) return;
        dispatch_set_context(source, this);
        dispatch_source_set_event_handler_f(source, [](void *context) {
            auto self = static_cast<MemoryNotifications *>(context);
            const auto flags = dispatch_source_get_data(self->source);
            const uchar level = flags & DISPATCH_MEMORYPRESSURE_CRITICAL ? 255
                : flags & DISPATCH_MEMORYPRESSURE_WARN ? 50 : 0;
            self->owner->memoryPressure(level);
        });
        dispatch_resume(source);
    }
    ~MemoryNotifications() {
        if (source) { dispatch_source_cancel(source); dispatch_release(source); }
    }
#elif defined(Q_OS_WIN)
    HANDLE event = CreateMemoryResourceNotification(LowMemoryResourceNotification);
    std::unique_ptr<QWinEventNotifier> notifier;
    explicit MemoryNotifications(KisGpuApplicationState *state) {
        if (!event) return;
        notifier = std::make_unique<QWinEventNotifier>(event);
        QObject::connect(notifier.get(), &QWinEventNotifier::activated, state, [state, this] {
            BOOL low = FALSE;
            if (QueryMemoryResourceNotification(event, &low) && low) state->memoryPressure(50);
            notifier->setEnabled(false);
            QTimer::singleShot(1000, notifier.get(), [this] { notifier->setEnabled(true); });
        });
    }
    ~MemoryNotifications() { notifier.reset(); if (event) CloseHandle(event); }
#elif defined(KIS_GPU_MEMORY_DBUS)
    QDBusConnection bus = QDBusConnection::systemBus();
    KisGpuApplicationState *owner;
    explicit MemoryNotifications(KisGpuApplicationState *state) : owner(state) {
        bus.connect(QStringLiteral("org.freedesktop.LowMemoryMonitor"), QStringLiteral("/org/freedesktop/LowMemoryMonitor"),
            QStringLiteral("org.freedesktop.LowMemoryMonitor"), QStringLiteral("LowMemoryWarning"),
            owner, SLOT(memoryPressure(uchar)));
    }
    ~MemoryNotifications() {
        bus.disconnect(QStringLiteral("org.freedesktop.LowMemoryMonitor"), QStringLiteral("/org/freedesktop/LowMemoryMonitor"),
            QStringLiteral("org.freedesktop.LowMemoryMonitor"), QStringLiteral("LowMemoryWarning"),
            owner, SLOT(memoryPressure(uchar)));
    }
#else
    explicit MemoryNotifications(KisGpuApplicationState *) {}
#endif
};

KisGpuApplicationState::KisGpuApplicationState(KisGpuDocumentDevice &documents, QObject *parent)
    : QObject(parent), m_documents(documents)
{
    m_poll.setInterval(16);
    connect(&m_poll, &QTimer::timeout, this, &KisGpuApplicationState::poll);
    connect(&documents, &KisGpuDocumentDevice::documentsChanged, this, &KisGpuApplicationState::updatePolling);
    connect(&documents, &KisGpuDocumentDevice::gpuRestored, this, &KisGpuApplicationState::updatePolling);
    connect(&documents, &KisGpuDocumentDevice::aboutToReleaseGpu, this, [this] { m_poll.stop(); });
    if (auto app = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) {
        connect(app, &QGuiApplication::applicationStateChanged, this, &KisGpuApplicationState::applicationStateChanged);
        connect(app, &QCoreApplication::aboutToQuit, this, &KisGpuApplicationState::shutdown);
        m_applicationState = app->applicationState();
    }
    m_memory = std::make_unique<MemoryNotifications>(this);
    applicationStateChanged(m_applicationState);
}

KisGpuApplicationState::~KisGpuApplicationState()
{
    shutdown();
}

void KisGpuApplicationState::updatePolling()
{
    if (!m_stopped && !m_resumeFailed && !m_critical && m_applicationState != Qt::ApplicationSuspended
        && m_documents.hasDocuments() && !m_documents.isSuspended()) m_poll.start();
    else m_poll.stop();
}

void KisGpuApplicationState::applicationStateChanged(Qt::ApplicationState state)
{
    if (m_stopped) return;
    m_applicationState = state;
    if (state == Qt::ApplicationSuspended) m_documents.suspend();
    else if (state == Qt::ApplicationActive) {
        m_critical = false;
        m_resumeFailed = !m_documents.resume();
    }
    updatePolling();
}

void KisGpuApplicationState::memoryPressure(uchar level)
{
    if (m_stopped) return;
    if (level == 255) {
        m_critical = true;
        m_documents.suspend();
    } else if (level >= 50) m_documents.requestHistoryEviction();
    else if (level == 0) {
        m_critical = false;
        if (m_applicationState == Qt::ApplicationActive) m_resumeFailed = !m_documents.resume();
    }
    updatePolling();
}

bool KisGpuApplicationState::resumeForInput()
{
    if (m_stopped || m_applicationState != Qt::ApplicationActive) return false;
    m_critical = false;
    m_resumeFailed = !m_documents.resume();
    updatePolling();
    return !m_resumeFailed;
}

void KisGpuApplicationState::poll()
{
    if (m_stopped) return;
    m_documents.poll();
    if (m_documents.isSuspended() && !m_resumeFailed && !m_critical && m_applicationState == Qt::ApplicationActive)
        m_resumeFailed = !m_documents.resume();
    updatePolling();
}

void KisGpuApplicationState::shutdown()
{
    if (m_stopped) return;
    m_stopped = true;
    m_poll.stop();
    m_memory.reset();
    m_documents.suspend();
}

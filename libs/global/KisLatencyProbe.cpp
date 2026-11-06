/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */
#include "KisLatencyProbe.h"

#include <QAbstractNativeEventFilter>
#include <QCoreApplication>
#include <QFile>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace
{
struct Entry {
    qint64 time;
    const char *event;
    double x;
    double y;
    qint64 extra;
    quint64 thread;
};

qint64 now()
{
#ifdef Q_OS_WIN
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return value.QuadPart;
#else
    return std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
#endif
}

quint64 threadId()
{
#ifdef Q_OS_WIN
    return GetCurrentThreadId();
#else
    return static_cast<quint64>(std::hash<std::thread::id>()(std::this_thread::get_id()) & 0xffff);
#endif
}

class Recorder
{
public:
    Recorder()
    {
        const QByteArray path = qgetenv("LIBREPAINT_LATENCY_LOG");
        if (path.isEmpty()) return;
        m_file = std::fopen(path.constData(), "w");
        if (!m_file) return;
#ifdef Q_OS_WIN
        LARGE_INTEGER frequency;
        QueryPerformanceFrequency(&frequency);
        std::fprintf(m_file, "# frequency=%lld\n", static_cast<long long>(frequency.QuadPart));
#else
        std::fprintf(m_file, "# frequency=1000000000\n");
#endif
        std::fprintf(m_file, "time,event,x,y,extra,thread\n");
        m_pending.reserve(1 << 16);
        m_enabled = true;
        m_writer = std::thread([this] { run(); });
    }

    ~Recorder()
    {
        if (!m_enabled) return;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stop = true;
        }
        m_wake.notify_all();
        m_writer.join();
        std::fclose(m_file);
    }

    bool enabled() const { return m_enabled; }

    void mark(const char *event, double x, double y, qint64 extra)
    {
        const Entry entry{now(), event, x, y, extra, threadId()};
        std::lock_guard<std::mutex> lock(m_mutex);
        m_pending.push_back(entry);
    }

private:
    void run()
    {
        std::vector<Entry> batch;
        batch.reserve(1 << 16);
        while (true) {
            bool stop = false;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_wake.wait_for(lock, std::chrono::milliseconds(500), [this] { return m_stop; });
                batch.swap(m_pending);
                stop = m_stop;
            }
            for (const Entry &entry : batch) {
                std::fprintf(m_file, "%lld,%s,%.2f,%.2f,%lld,%llu\n", static_cast<long long>(entry.time), entry.event,
                             entry.x, entry.y, static_cast<long long>(entry.extra),
                             static_cast<unsigned long long>(entry.thread));
            }
            std::fflush(m_file);
            batch.clear();
            if (stop) return;
        }
    }

    bool m_enabled = false;
    bool m_stop = false;
    std::FILE *m_file = nullptr;
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::vector<Entry> m_pending;
    std::thread m_writer;
};

Recorder &recorder()
{
    static Recorder instance;
    return instance;
}

#ifdef Q_OS_WIN
class NativeFilter : public QAbstractNativeEventFilter
{
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *) override
    {
        if (eventType != "windows_generic_MSG") return false;
        const MSG *msg = static_cast<const MSG *>(message);
        switch (msg->message) {
        case 0x0245: // WM_POINTERUPDATE
        case 0x0246: // WM_POINTERDOWN
        case 0x0247: { // WM_POINTERUP
            const quint32 flags = HIWORD(msg->wParam);
            const auto x = static_cast<short>(LOWORD(msg->lParam));
            const auto y = static_cast<short>(HIWORD(msg->lParam));
            recorder().mark("native_pointer", x, y, (qint64(msg->message) << 32) | flags);
            break;
        }
        case 0x7FF0: // WT_PACKET
            recorder().mark("native_wintab", 0, 0, 0);
            break;
        case WM_MOUSEMOVE:
            recorder().mark("native_mouse", static_cast<short>(LOWORD(msg->lParam)),
                            static_cast<short>(HIWORD(msg->lParam)), 0);
            break;
        default:
            break;
        }
        return false;
    }
};
#endif
} // namespace

namespace KisLatencyProbe
{
bool enabled()
{
    return recorder().enabled();
}

void mark(const char *event, double x, double y, qint64 extra)
{
    Recorder &instance = recorder();
    if (instance.enabled()) instance.mark(event, x, y, extra);
}

void installNativeFilter(QCoreApplication *app)
{
#ifdef Q_OS_WIN
    if (enabled()) app->installNativeEventFilter(new NativeFilter);
#else
    Q_UNUSED(app);
#endif
}
}

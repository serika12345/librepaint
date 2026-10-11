/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_GPU_WINDOW_TEST_EVENT_LOOP_H
#define KIS_GPU_WINDOW_TEST_EVENT_LOOP_H
#include <QEventLoop>
#include <QTimer>
#include <exception>

/** Native presentation callbacks require the GUI event loop's platform entry. */
template<typename Predicate>
bool waitForGpuWindow(Predicate ready, int timeoutMilliseconds = 5000)
{
    if (ready()) return true;
    QEventLoop loop;
    QTimer poll, timeout;
    poll.setTimerType(Qt::PreciseTimer);
    timeout.setSingleShot(true);
    bool completed = false;
    std::exception_ptr failure;
    QObject::connect(&poll, &QTimer::timeout, &loop, [&] {
        try {
            completed = ready();
            if (completed) loop.quit();
        } catch (...) { failure = std::current_exception(); loop.quit(); }
    });
    QObject::connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    poll.start(1); timeout.start(timeoutMilliseconds);
    loop.exec();
    if (failure) std::rethrow_exception(failure);
    return completed;
}
#endif

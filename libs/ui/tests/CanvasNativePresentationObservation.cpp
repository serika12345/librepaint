// SPDX-License-Identifier: GPL-2.0-or-later
#include "CanvasNativePresentationObservation.h"
#include "../canvas/kis_canvas_performance_measurement_p.h"
#include <QPointer>
#include <QThread>
#include <QCoreApplication>
#ifdef Q_OS_MACOS
#include <OpenGL/gl.h>
#include <objc/runtime.h>
namespace {
QPointer<QObject> observed;
Method flushMethod=nullptr;
IMP originalFlush=nullptr;
void experimentFlushBuffer(id context,SEL command) {
    namespace Perf=Krita::Canvas::Performance;
    const bool enabled=observed && QThread::currentThread()==observed->thread()
        && observed->property(Perf::enabledProperty).toBool();
    const qint64 start=enabled ? Perf::nowNs() : 0;
    reinterpret_cast<void (*)(id,SEL)>(originalFlush)(context,command);
    if(enabled) {
        if(qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_GPU_COMPLETION")) glFinish();
        const qint64 end=Perf::nowNs();
        Perf::record(*observed,QStringLiteral("parent_present"),end-start,
            observed->property(Perf::inputProperty).toLongLong(),end);
    }
}
}
void observeCanvasNativePresentation(QObject *owner) {
    observed=owner;
    if(owner && !originalFlush) {
        flushMethod=class_getInstanceMethod(objc_getClass("NSOpenGLContext"),sel_registerName("flushBuffer"));
        if(flushMethod) originalFlush=method_setImplementation(flushMethod,reinterpret_cast<IMP>(&experimentFlushBuffer));
    } else if(!owner && originalFlush) {
        method_setImplementation(flushMethod,originalFlush);
        originalFlush=nullptr;
    }
}
#else
void observeCanvasNativePresentation(QObject *) {}
#endif

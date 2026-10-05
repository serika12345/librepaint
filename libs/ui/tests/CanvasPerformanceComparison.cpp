/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "../canvas/kis_canvas_performance_measurement_p.h"
#include "CanvasFrameReturnObservation.h"
#include "CanvasNativePresentationObservation.h"
#include <canvas/kis_canvas2.h>
#include <canvas/kis_canvas_controller.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <canvas/kis_coordinates_converter.h>
#include <canvas/kis_zoom_manager.h>
#include <canvas/wgpu/WgpuCanvas.h>
#include <application/ui/workspace/KisView.h>
#include <document/KisDocument.h>
#include <brushengine/kis_paintop_preset.h>
#include <KisGlobalResourcesInterface.h>
#include <KoColorSpaceRegistry.h>
#include <KoColor.h>
#include <kis_image.h>
#include <KisExperimentCpuProfile.h>
#include <kis_projection_updates_filter.h>
#include <kis_group_layer.h>
#include <kis_paint_layer.h>
#include <kis_paint_device.h>
#include <kundo2stack.h>
#include <QCryptographicHash>
#include <QFocusEvent>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QOpenGLWidget>
#include <QScopeGuard>
#include <QScopedValueRollback>
#include <QTest>
#include <QThread>
#include <QWindow>
#include <KoTestConfig.h>
#include <sys/resource.h>
#include <time.h>
#include <optional>
#include <mach/mach.h>
#include <mach/thread_info.h>

namespace Perf = Krita::Canvas::Performance;
namespace
{
// Keep unrequested native and Qt-generated input out of the fixed sequence.
// The explicit QWindow dispatch retains the complete widget/tool route.
class ExternalInputFilter : public QObject
{
public:
    bool delivering = false;
    bool tracking = false;
    QWidget *canvas = nullptr;
    QWindow *window = nullptr;
    QString interruption;
private:
    bool eventFilter(QObject *object, QEvent *event) override
    {
        if (tracking && (object == canvas || object == window || object == canvas->window()
                         || object == canvas->window()->windowHandle())) {
            if (event->type() == QEvent::FocusOut || event->type() == QEvent::WindowDeactivate
                || (event->type() == QEvent::Expose && !window->isExposed())) {
                interruption = QStringLiteral("event %1 on %2").arg(int(event->type())).arg(QString::fromLatin1(object->metaObject()->className()));
            }
        }
        if (delivering) return false;
        switch (event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::TabletMove:
        case QEvent::TabletPress:
        case QEvent::TabletRelease:
        case QEvent::Wheel:
        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::ShortcutOverride:
        case QEvent::Enter:
        case QEvent::Leave:
            return true;
        default:
            return false;
        }
    }
};
QVariantMap stage(const QObject &owner, const char *name)
{
    return owner.property(Perf::samplesProperty).toMap().value(QString::fromLatin1(name)).toMap();
}
QImage documentPixels(KisPaintLayer &layer)
{
    const auto *cs = KoColorSpaceRegistry::instance()->rgb8();
    const int dim = qMax(1024, qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_DIMENSION"));
    return layer.paintDevice()->convertToQImage(cs->profile(), 0, 0, dim, dim).convertToFormat(QImage::Format_RGBA8888);
}
qint64 processCpuNs() {
    rusage usage{}; getrusage(RUSAGE_SELF, &usage);
    return (usage.ru_utime.tv_sec+usage.ru_stime.tv_sec)*1000000000LL
        + (usage.ru_utime.tv_usec+usage.ru_stime.tv_usec)*1000LL;
}
struct CpuThread { qint64 cpuNs=0; QString name; };
QHash<quint64,CpuThread> cpuThreadSnapshot() {
    thread_act_array_t list=nullptr; mach_msg_type_number_t length=0;
    QHash<quint64,CpuThread> result;
    if(task_threads(mach_task_self(),&list,&length)!=KERN_SUCCESS) return result;
    for(mach_msg_type_number_t i=0;i<length;++i) {
        thread_basic_info_data_t basic{};thread_identifier_info_data_t ident{};thread_extended_info_data_t extended{};
        mach_msg_type_number_t nb=THREAD_BASIC_INFO_COUNT,ni=THREAD_IDENTIFIER_INFO_COUNT,ne=THREAD_EXTENDED_INFO_COUNT;
        const bool ok=thread_info(list[i],THREAD_BASIC_INFO,reinterpret_cast<thread_info_t>(&basic),&nb)==KERN_SUCCESS
            && thread_info(list[i],THREAD_IDENTIFIER_INFO,reinterpret_cast<thread_info_t>(&ident),&ni)==KERN_SUCCESS;
        if(ok) {
            thread_info(list[i],THREAD_EXTENDED_INFO,reinterpret_cast<thread_info_t>(&extended),&ne);
            result.insert(ident.thread_id,{(basic.user_time.seconds+basic.system_time.seconds)*1000000000LL
                +(basic.user_time.microseconds+basic.system_time.microseconds)*1000LL,QString::fromUtf8(extended.pth_name)});
        }
        mach_port_deallocate(mach_task_self(),list[i]);
    }
    vm_deallocate(mach_task_self(),reinterpret_cast<vm_address_t>(list),length*sizeof(thread_t));
    return result;
}
qint64 guiCpuNs() {
    timespec stamp{}; clock_gettime(CLOCK_THREAD_CPUTIME_ID, &stamp);
    return stamp.tv_sec*1000000000LL+stamp.tv_nsec;
}
}

/** Fixed, paced native-Qt input; observation is CPU return after the last
 * document projection update. GPU readback and undo run outside measurement.
 */
class GpuLayerUpdatesFilter final : public KisProjectionUpdatesFilter
{
public:
    bool filter(KisImage *image,KisNode *,const QVector<QRect> &rects,KisProjectionUpdateFlags) override {
        for (const auto &rect : rects) image->notifyProjectionUpdated(rect);
        return true;
    }
    bool filterRefreshGraph(KisImage *image,KisNode *node,const QVector<QRect> &rects,const QRect &,KisProjectionUpdateFlags flags) override {
        return filter(image,node,rects,flags);
    }
};

void compareCanvasPerformance(KisView &view, KisDocument &document)
{
    const QString backend = qEnvironmentVariable("LIBREPAINT_CANVAS_PERFORMANCE");
    QWidget *widget = view.canvasBase()->canvasWidget();
    auto *gpu = dynamic_cast<Krita::Canvas::WgpuCanvas *>(widget);
    auto *gl = qobject_cast<QOpenGLWidget *>(widget);
    QCOMPARE(bool(gpu), backend == QStringLiteral("wgpu"));
    QCOMPARE(bool(gl), backend == QStringLiteral("opengl"));
    const int width = qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_VIEW_WIDTH");
    const int height = qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_VIEW_HEIGHT");
    view.canvasController()->setFixedSize(width > 0 ? width : 640, height > 0 ? height : 480);
    // Existing canvases keep their ordinary widget composition route.
    QWindow *window = gpu ? widget->windowHandle() : widget->window()->windowHandle();
    QVERIFY(window);
    ExternalInputFilter externalInput;
    externalInput.canvas = widget;
    externalInput.window = window;
    qApp->installEventFilter(&externalInput);
    const auto sendInputEvent = [&](QEvent &event) {
        QScopedValueRollback<bool> allow(externalInput.delivering, true);
        QCoreApplication::sendEvent(window, &event);
    };
    CanvasFrameReturnObservation frames(gpu ? static_cast<QObject &>(*window) : static_cast<QObject &>(*widget),
                                        gpu || gl ? "submit" : "compose");
    CanvasFrameReturnObservation uploads(*window, "upload");
    CanvasFrameReturnObservation nativePresent(*window,"parent_present");
    observeCanvasNativePresentation(window);
    auto stopNativeObservation=qScopeGuard([] { observeCanvasNativePresentation(nullptr); });
    const int threads = qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_THREADS");
    document.image()->setWorkingThreadsLimit(threads > 0 ? threads : 1);
    view.canvasBase()->setLodPreferredInCanvas(false);
    view.zoomManager()->slotZoomToFit();
    document.image()->waitForDone();
    QTest::qWait(200);
    QVERIFY(window->isExposed());
    auto *layer = dynamic_cast<KisPaintLayer *>(document.image()->rootLayer()->firstChild().data());
    QVERIFY(layer);
    auto preset = KisPaintOpPresetSP::create(QStringLiteral(KRITA_SOURCE_DIR "/sdk/tests/data/autobrush_300px.kpp"));
    QVERIFY(preset->load(KisGlobalResourcesInterface::instance()));
    view.resourceProvider()->setPaintOpPreset(preset);
    const int brush = qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_BRUSH");
    view.resourceProvider()->setSize(brush > 0 ? brush : 32.0);
    view.resourceProvider()->setFGColor(KoColor(Qt::black, KoColorSpaceRegistry::instance()->rgb8()));
    QTest::qWait(100);

    widget->setProperty(Perf::enabledProperty, true);
    window->setProperty(Perf::enabledProperty, true);
    auto disable = qScopeGuard([widget, window, &document] {
        widget->setProperty(Perf::enabledProperty, false);
        window->setProperty(Perf::enabledProperty, false);
        QFocusEvent focusOut(QEvent::FocusOut, Qt::OtherFocusReason);
        QCoreApplication::sendEvent(widget, &focusOut);
        document.setModified(false);
    });
    const QImage blank = documentPixels(*layer);
    QImage expected;
    QHash<int,QImage> expectedVariants;
    const bool varyBrush=qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_VARY_BRUSH");
    QJsonArray results;
    QJsonArray discardedInputs;
    QJsonObject displayDifference;
    qint64 attemptedInputs = 0;
    const int requested = qEnvironmentVariableIntValue("LIBREPAINT_CANVAS_PERFORMANCE_SAMPLES");
    const int samples = requested > 0 ? requested : 20;
    QVERIFY(samples <= 200);
    KisProjectionUpdatesFilterSP layerFilter;
    KisProjectionUpdatesFilterCookie layerFilterCookie=nullptr;
    if (qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_SKIP_CPU_COMPOSE")) {
        QVERIFY(gpu && qEnvironmentVariableIntValue("LIBREPAINT_WGPU_LAYER_COMPOSE"));
        layerFilter=KisProjectionUpdatesFilterSP(new GpuLayerUpdatesFilter);
        layerFilterCookie=document.image()->addProjectionUpdatesFilter(layerFilter);
    }
    auto removeLayerFilter=qScopeGuard([&] {
        document.image()->waitForDone();
        if (layerFilterCookie) document.image()->removeProjectionUpdatesFilter(layerFilterCookie);
    });
    const int intervalRequest=qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_INPUT_INTERVAL_MS");
    const int inputInterval=intervalRequest>0 ? intervalRequest : 4;
    const bool cpuProfile=qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_CPU_PROFILE");
    constexpr int warmup = 3;
    for (int iteration = 0; iteration < samples + warmup; ++iteration) {
        const int actualBrush=varyBrush ? (brush>0 ? brush : 32)*(8-iteration%3)/8 : (brush>0 ? brush : 32);
        if(varyBrush) view.resourceProvider()->setSize(actualBrush);
        widget->window()->raise();
        widget->window()->activateWindow();
        QVERIFY(QTest::qWaitForWindowActive(widget->window()->windowHandle()));
        document.image()->waitForDone();
        QTest::qWait(100);
        QTRY_VERIFY2_WITH_TIMEOUT(window->isExposed(),qPrintable(QStringLiteral("Window exposure lost before input %1").arg(iteration)),3000);
        const qint64 id = ++attemptedInputs;
        widget->setProperty(Perf::samplesProperty, QVariantMap());
        window->setProperty(Perf::samplesProperty, QVariantMap());
        widget->setProperty(Perf::inputProperty, id);
        window->setProperty(Perf::inputProperty, id);
        frames.reset(id);
        uploads.reset(id);
        nativePresent.reset(id);
        const quint64 bytesBefore = gpu ? gpu->uploadedBytes() : 0;
        widget->setFocus(Qt::OtherFocusReason);
        QFocusEvent focusIn(QEvent::FocusIn, Qt::OtherFocusReason);
        QCoreApplication::sendEvent(widget, &focusIn);
        const auto point = [&view, &document](int index) {
            // No random brush options; fixed coordinates/times are shared by all routes.
            return view.canvasBase()->coordinatesConverter()->imageToWidget(
                QPointF((180 + index * 28)*document.image()->width()/1024.0,
                        (300 + (index % 6) * 35)*document.image()->height()/1024.0)).toPoint();
        };
        const auto windowPoint = [window, widget](QPoint local) {
            return window->mapFromGlobal(widget->mapToGlobal(local));
        };
        for (int index = 0; index < 24; ++index) {
            QVERIFY2(widget->visibleRegion().contains(point(index)),
                     qPrintable(QStringLiteral("Input point %1 maps outside the visible canvas: %2,%3 (visible %4x%5, view %6x%7)")
                         .arg(index).arg(point(index).x()).arg(point(index).y())
                         .arg(widget->visibleRegion().boundingRect().width()).arg(widget->visibleRegion().boundingRect().height())
                         .arg(view.width()).arg(view.height())));
        }
        QEnterEvent enter(windowPoint(point(0)), windowPoint(point(0)), widget->mapToGlobal(point(0)));
        sendInputEvent(enter);
        externalInput.interruption.clear();
        externalInput.tracking = true;
        const auto threadsBefore=cpuProfile ? cpuThreadSnapshot() : QHash<quint64,CpuThread>();
        std::optional<KisExperimentCpuProfile::Scope> guiProfile;
        if (cpuProfile) { KisExperimentCpuProfile::start(); guiProfile.emplace(KisExperimentCpuProfile::GuiLoop); }
        const qint64 begin = Perf::nowNs();
        const qint64 processCpuBegin = processCpuNs(), guiCpuBegin = guiCpuNs();
        qint64 release = begin;
        qint64 dispatchLateness = 0;
        for (int index = 0; index <= 24; ++index) {
            const qint64 deadline = begin + index * inputInterval * 1000000LL;
            while (Perf::nowNs() < deadline) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 1);
                if (deadline - Perf::nowNs() > 1000000) QThread::msleep(1);
            }
            dispatchLateness = qMax(dispatchLateness, Perf::nowNs() - deadline);
            const auto type = index == 0 ? QEvent::MouseButtonPress
                : index == 24 ? QEvent::MouseButtonRelease : QEvent::MouseMove;
            const QPoint local = point(qMin(index, 23));
            QMouseEvent event(type, windowPoint(local), widget->mapToGlobal(local),
                              type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton,
                              type == QEvent::MouseButtonRelease ? Qt::NoButton : Qt::LeftButton, Qt::NoModifier);
            event.setTimestamp(1000 + id * 1000 + index * inputInterval);
            if (type == QEvent::MouseButtonRelease) release = Perf::nowNs();
            sendInputEvent(event);
            if (qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_FULL_REPAINT")) widget->update();
            if (qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_PAN")) view.canvasController()->pan(QPoint(index%2 ? 24 : -24, 0));
        }
        // Keep the ordinary event loop running while document jobs finish.
        // waitForDone() requests stroke termination and invokes busy-wait UI.
        const qint64 jobsWaitBegin = Perf::nowNs();
        while (!document.image()->isIdle() && Perf::nowNs() - jobsWaitBegin < 5000000000LL) QTest::qWait(1);
        QVERIFY(document.image()->isIdle());
        const qint64 jobsDone = Perf::nowNs();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::MetaCall);
        const auto discardInterrupted = [&] {
            if (!window->isExposed()) externalInput.interruption = QStringLiteral("native window became unexposed");
            if (externalInput.interruption.isEmpty()) return false;
            guiProfile.reset();
            if (cpuProfile) KisExperimentCpuProfile::stop();
            externalInput.tracking = false;
            discardedInputs.append(QJsonObject{{QStringLiteral("input"), double(id)},
                {QStringLiteral("reason"), externalInput.interruption}});
            if (document.undoStack()->index() > 0) document.undoStack()->undo();
            document.image()->waitForDone();
            return true;
        };
        if (discardInterrupted()) {
            QVERIFY2(discardedInputs.size() <= 10, "Repeated external focus/exposure interruptions prevent comparison");
            QCOMPARE(documentPixels(*layer), blank);
            --iteration;
            continue;
        }
        QTRY_VERIFY2_WITH_TIMEOUT(stage(*widget, "projection").value(QStringLiteral("count")).toLongLong() > 0,
                                 "Canvas projection CPU timing is missing", 2000);
        const auto completed = [&] {
            if (gpu) return frames.firstReturnContaining(widget->property("librepaintCanvasProjectionGeneration").toULongLong()) != 0;
            const auto projection = stage(*widget, "projection");
            const qint64 projected = projection.value(QStringLiteral("finish_ns")).toLongLong();
            const qint64 ready = gpu ? uploads.firstReturnAtOrAfter(projected) : projected;
            if (gl) {
                const qint64 drawn=frames.firstReturnAtOrAfter(projected);
                return drawn && nativePresent.firstReturnAtOrAfter(drawn);
            }
            return ready && frames.firstReturnAtOrAfter(ready);
        };
        const auto diagnostic = [&] {
            return QString::fromUtf8(QJsonDocument(QJsonObject{
                {QStringLiteral("widget_exposed"), widget->windowHandle() ? widget->windowHandle()->isExposed() : false},
                {QStringLiteral("window_exposed"), window->isExposed()},
                {QStringLiteral("window_visible"), window->isVisible()},
                {QStringLiteral("widget_visible"), widget->isVisible()},
                {QStringLiteral("main_hidden"), widget->window()->isHidden()},
                {QStringLiteral("window_state"), int(window->windowState())},
                {QStringLiteral("widget_stages"), QJsonObject::fromVariantMap(widget->property(Perf::samplesProperty).toMap())},
                {QStringLiteral("window_stages"), QJsonObject::fromVariantMap(window->property(Perf::samplesProperty).toMap())}
            }).toJson(QJsonDocument::Compact));
        };
        const qint64 waitBegin = Perf::nowNs();
        qint64 stableSince = waitBegin;
        qint64 projectionCount = -1;
        while (Perf::nowNs() - waitBegin < 5000000000LL) {
            if (!externalInput.interruption.isEmpty()) break;
            const qint64 count = stage(*widget, "projection").value(QStringLiteral("count")).toLongLong();
            if (count != projectionCount) {
                projectionCount = count;
                stableSince = Perf::nowNs();
            }
            // Three 100-fps projection intervals drain the final coalesced
            // update. The endpoint remains the recorded CPU return time.
            if (completed() && Perf::nowNs() - stableSince >= 30000000LL) break;
            QTest::qWait(1);
        }
        if (discardInterrupted()) {
            QVERIFY2(discardedInputs.size() <= 10, "Repeated external focus/exposure interruptions prevent comparison");
            QCOMPARE(documentPixels(*layer), blank);
            --iteration;
            continue;
        }
        QVERIFY2(completed() && Perf::nowNs() - stableSince >= 30000000LL, qPrintable(diagnostic()));
        QVERIFY(window->isExposed());
        const qint64 projected = stage(*widget, "projection").value(QStringLiteral("finish_ns")).toLongLong();
        const qint64 ready = gpu ? uploads.firstReturnAtOrAfter(projected) : projected;
        const qint64 drawn = gpu ? frames.firstReturnContaining(widget->property("librepaintCanvasProjectionGeneration").toULongLong())
                                : frames.firstReturnAtOrAfter(ready);
        const qint64 end = gl ? nativePresent.firstReturnAtOrAfter(drawn) : drawn;
        guiProfile.reset();
        const qint64 processCpuEnd = processCpuNs(), guiCpuEnd = guiCpuNs();
        const auto cpuStages=cpuProfile ? KisExperimentCpuProfile::stop() : KisExperimentCpuProfile::Snapshot{};
        const auto threadsAfter=cpuProfile ? cpuThreadSnapshot() : QHash<quint64,CpuThread>();
        const auto scopedThreads=cpuProfile ? KisExperimentCpuProfile::threadCpu() : KisExperimentCpuProfile::ThreadCpu();
        if(cpuProfile) qInfo()<<"CPU_PROFILE_INPUT"<<iteration<<"threads"<<threadsAfter.size()<<"process_cpu_ms"<<(processCpuEnd-processCpuBegin)/1e6;
        externalInput.tracking = false;
        const bool inspectDisplay=expected.isNull() || iteration == samples + warmup - 1;
        const bool inspectPixels=inspectDisplay || varyBrush;
        const QImage pixels = inspectPixels ? documentPixels(*layer) : QImage();
        if (inspectPixels) QVERIFY(pixels != blank);
        const QImage expectedForBrush=expectedVariants.value(actualBrush);
        if(inspectPixels && expectedForBrush.isNull()) { expectedVariants.insert(actualBrush,pixels); if(expected.isNull()) expected=pixels; }
        else if (inspectPixels && pixels != expectedForBrush) {
            const QString directory = qEnvironmentVariable("LIBREPAINT_CANVAS_PERFORMANCE_ARTIFACTS", QDir::tempPath());
            const QString prefix = directory + QStringLiteral("/canvas-%1-%2").arg(backend).arg(iteration);
            pixels.save(prefix + QStringLiteral("-actual.png"));
            expectedForBrush.save(prefix + QStringLiteral("-expected.png"));
            QFAIL(qPrintable(QStringLiteral("Repeated input produced different pixels; inspect %1-{actual,expected}.png (brush size %2, interruption %3)")
                .arg(prefix).arg(view.resourceProvider()->size()).arg(externalInput.interruption)));
        }
        if (iteration >= warmup) {
            QJsonObject sample{
                {QStringLiteral("actual_brush_size"),actualBrush},
                {QStringLiteral("input"), double(id)},
                {QStringLiteral("input_begin_ns"), double(begin)},
                {QStringLiteral("input_release_ns"), double(release)},
                {QStringLiteral("document_jobs_done_ns"), double(jobsDone)},
                {QStringLiteral("frame_return_ns"), double(end)},
                {QStringLiteral("draw_return_ns"), double(drawn)},
                {QStringLiteral("stroke_ms"), (end - begin) / 1e6},
                {QStringLiteral("input_span_ms"), (release - begin) / 1e6},
                {QStringLiteral("dispatch_lateness_ms_max"), dispatchLateness / 1e6},
                {QStringLiteral("release_to_frame_ms"), qMax(qint64(0), end - release) / 1e6},
                {QStringLiteral("process_cpu_ms"), (processCpuEnd-processCpuBegin)/1e6},
                {QStringLiteral("gui_cpu_ms"), (guiCpuEnd-guiCpuBegin)/1e6},
                {QStringLiteral("widget_stages"), QJsonObject::fromVariantMap(widget->property(Perf::samplesProperty).toMap())},
                {QStringLiteral("window_stages"), QJsonObject::fromVariantMap(window->property(Perf::samplesProperty).toMap())}};
            if (gpu) sample[QStringLiteral("uploaded_bytes")] = double(gpu->uploadedBytes() - bytesBefore);
            if (cpuProfile) {
                const char *names[]={"dab_generation","brush_apply","brush_composite","tile_access","tile_allocate","tile_copy","device_blend","image_extract","read_pixels","write_pixels","worker_loop","stroke_jobs","projection_jobs","gpu_frame","gui_loop","dab_raster","dab_cache_lookup","dab_cache_hit"};
                QJsonObject stages;
                for(int i=0;i<KisExperimentCpuProfile::Count;++i) {
                    const auto &v=cpuStages[i];
                    stages[QString::fromLatin1(names[i])]=QJsonObject{{"cpu_ms",v.cpu/1e6},{"exclusive_cpu_ms",v.exclusiveCpu/1e6},{"wall_ms",v.wall/1e6},{"calls",double(v.calls)},{"units",double(v.units)},{"peak_units",double(v.peakUnits)}};
                }
                sample["cpu_profile"]=stages;
                QJsonArray cpuThreads;
                for(auto it=threadsAfter.cbegin();it!=threadsAfter.cend();++it) {
                    const qint64 cpu=it.value().cpuNs-threadsBefore.value(it.key()).cpuNs;
                    if(cpu<=0) continue;
                    auto scoped=scopedThreads.find(it.key());
                    const qint64 tracked=scoped==scopedThreads.end() ? 0 : scoped->second;
                    cpuThreads.append(QJsonObject{{"id",QString::number(it.key())},{"name",it.value().name},{"cpu_ms",cpu/1e6},{"scoped_cpu_ms",tracked/1e6},{"unscoped_cpu_ms",(cpu-tracked)/1e6}});
                }
                sample["cpu_threads"]=cpuThreads;
            }
            results.append(sample);
        }
        if (gpu && inspectDisplay) {
            QVERIFY2(gpu->presentationError().isEmpty(), qPrintable(gpu->presentationError()));
            const QImage actual = gpu->capturedFrame().convertToFormat(QImage::Format_RGBA8888);
            if (layerFilterCookie) {
                document.image()->removeProjectionUpdatesFilter(layerFilterCookie);
                layerFilterCookie=nullptr;
                document.image()->refreshGraphAsync();
                document.image()->waitForDone();
            }
            const QImage reference = gpu->composedFrame().convertToFormat(QImage::Format_RGBA8888);
            if (layerFilter) layerFilterCookie=document.image()->addProjectionUpdatesFilter(layerFilter);
            QCOMPARE(actual.size(), reference.size());
            if (qEnvironmentVariableIntValue("LIBREPAINT_WGPU_GPU_PROJECTION")) {
                quint64 changed=0,total=0; int maximum=0;
                for (int y=0;y<actual.height();++y) for(int x=0;x<actual.width()*4;++x) {
                    const int error=qAbs(int(actual.constScanLine(y)[x])-int(reference.constScanLine(y)[x]));
                    changed+=error>0;total+=error;maximum=qMax(maximum,error);
                }
                displayDifference={{QStringLiteral("max_channel_error"),maximum},
                    {QStringLiteral("mean_channel_error"),double(total)/(actual.width()*actual.height()*4)},
                    {QStringLiteral("changed_channel_fraction"),double(changed)/(actual.width()*actual.height()*4)}};
                const QString directory=qEnvironmentVariable("LIBREPAINT_CANVAS_PERFORMANCE_ARTIFACTS");
                const QString label = qEnvironmentVariable("LIBREPAINT_EXPERIMENT_LABEL", "gpu-projection");
                if(iteration==samples+warmup-1) {actual.save(directory+"/"+label+"-actual.png");reference.save(directory+"/"+label+"-reference.png");}
            } else QCOMPARE(actual,reference);
        }
        document.undoStack()->undo();
        document.image()->waitForDone();
        if (qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_PAN")) view.canvasController()->pan(QPoint(24, 0));
    }
    const QByteArray hash = QCryptographicHash::hash(
        QByteArrayView(reinterpret_cast<const char *>(expected.constBits()), expected.sizeInBytes()), QCryptographicHash::Sha256).toHex();
    QJsonObject variantHashes;
    for(auto it=expectedVariants.cbegin();it!=expectedVariants.cend();++it) {
        const auto &image=it.value();
        variantHashes[QString::number(it.key())]=QString::fromLatin1(QCryptographicHash::hash(QByteArrayView(reinterpret_cast<const char*>(image.constBits()),image.sizeInBytes()),QCryptographicHash::Sha256).toHex());
    }
    const QJsonObject report{
        {QStringLiteral("schema"), 3}, {QStringLiteral("measurement_revision"), 3}, {QStringLiteral("backend"), backend},
        {QStringLiteral("gpu_completion_waited"), bool(qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_GPU_COMPLETION"))},
        {QStringLiteral("endpoint"), gl || gpu ? "native_present_cpu_return" : "cpu_paint_return"},
        {QStringLiteral("endpoint_selection"), gpu ? "first_return_containing_final_projection_generation" : "first_return_after_final_projection"},
        {QStringLiteral("document"), QStringLiteral("%1x%2/RGBA8/%3-layers").arg(document.image()->width()).arg(document.image()->height()).arg(qMax(1,qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_LAYERS")))},
        {QStringLiteral("brush_variant_sha256"),variantHashes},
        {QStringLiteral("preset"), "sdk/tests/data/autobrush_300px.kpp"},
        {QStringLiteral("brush_size"), brush > 0 ? brush : 32}, {QStringLiteral("smoothing"), "none"},
        {QStringLiteral("input_points"), 25}, {QStringLiteral("input_interval_ms"), inputInterval},
        {QStringLiteral("working_threads"), threads > 0 ? threads : 1}, {QStringLiteral("warmup_strokes"), warmup},
        {QStringLiteral("full_repaint"), bool(qEnvironmentVariableIntValue("LIBREPAINT_EXPERIMENT_FULL_REPAINT"))},
        {QStringLiteral("frame_limit_fps"), 100},
        {QStringLiteral("widget_width"), widget->width()}, {QStringLiteral("widget_height"), widget->height()},
        {QStringLiteral("device_pixel_ratio"), widget->devicePixelRatioF()},
        {QStringLiteral("pixel_sha256"), QString::fromLatin1(hash)},
        {QStringLiteral("qt_version"), qVersion()}, {QStringLiteral("samples"), results},
        {QStringLiteral("discarded_inputs"), discardedInputs}};
    QJsonObject reportWithDifference = report;
    reportWithDifference[QStringLiteral("display_difference")] = displayDifference;
    qInfo().noquote() << "CANVAS_PERFORMANCE" << QJsonDocument(reportWithDifference).toJson(QJsonDocument::Compact);
}

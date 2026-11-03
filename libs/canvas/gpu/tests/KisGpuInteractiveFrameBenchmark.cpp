/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QDataStream>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QScreen>
#include <QTest>
#include <QTextStream>
#include <QWindow>
#include <QThread>
#include "../KisGpuWindowSurface_p.h"
#include "../KisGpuPresentationResources_p.h"
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuTilePresentation.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <atomic>
#include <cstring>
#include "KisGpuBenchmarkFrameSchedule_p.h"

using namespace LibrePaint::Image::Gpu;
using LibrePaint::Canvas::Gpu::TilePresentation;

namespace {
KoColorConversionStages identityDisplay(const Snapshot &source)
{
    KoColorConversionStages value;
    value.input.depth = source->format == PixelFormat::Rgba16
        ? KoColorConversionStages::Encoding::U16 : KoColorConversionStages::Encoding::U8;
    value.input.iccProfile = source->colorProfile;
    value.output = value.input;
    return value;
}
}

namespace {
void require(bool accepted, const QString &diagnostic)
{
    if (!accepted) throw std::runtime_error(diagnostic.toStdString());
}

double percentile95(QList<double> values)
{
    if (values.isEmpty()) return 0;
    std::sort(values.begin(), values.end());
    return values[qsizetype(std::ceil(values.size() * 0.95)) - 1];
}

QByteArray timestamps(Device &device, WGPUQuerySet queries, quint32 count)
{
    using Buffer = LibrePaint::Canvas::Gpu::Detail::Buffer;
    struct Read {
        Buffer buffer;
        QByteArray bytes;
        std::atomic<bool> ready{false};
    };
    const quint64 bytes = quint64(count) * sizeof(quint64);
    WGPUBufferDescriptor descriptor{};
    descriptor.size = bytes;
    descriptor.usage = WGPUBufferUsage_QueryResolve | WGPUBufferUsage_CopySrc;
    Buffer resolved(wgpuDeviceCreateBuffer(device.nativeDevice(), &descriptor));
    auto read = std::make_shared<Read>();
    descriptor.usage = WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    read->buffer.reset(wgpuDeviceCreateBuffer(device.nativeDevice(), &descriptor));
    require(resolved && read->buffer, QStringLiteral("Timestamp readback allocation failed"));
    const auto commands = device.beginCommands();
    require(bool(commands), QStringLiteral("Timestamp resolve commands are unavailable"));
    wgpuCommandEncoderResolveQuerySet(commands, queries, 0, count, resolved.get(), 0);
    wgpuCommandEncoderCopyBufferToBuffer(commands, resolved.get(), 0, read->buffer.get(), 0, bytes);
    require(device.submitCommands(true), QStringLiteral("Timestamp resolve failed"));
    WGPUBufferMapCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.userdata1 = new std::shared_ptr<Read>(read);
    callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *context, void *) noexcept {
        std::unique_ptr<std::shared_ptr<Read>> retained(static_cast<std::shared_ptr<Read> *>(context));
        auto &read = **retained;
        if (status == WGPUMapAsyncStatus_Success) {
            try {
                const auto size = wgpuBufferGetSize(read.buffer.get());
                const auto data = wgpuBufferGetConstMappedRange(read.buffer.get(), 0, size);
                if (data) read.bytes = QByteArray(static_cast<const char *>(data), qsizetype(size));
            } catch (...) {}
            wgpuBufferUnmap(read.buffer.get());
        }
        read.ready.store(true, std::memory_order_release);
    };
    wgpuBufferMapAsync(read->buffer.get(), WGPUMapMode_Read, 0, bytes, callback);
    require(device.finish() && read->ready.load(std::memory_order_acquire) && read->bytes.size() == qsizetype(bytes),
            QStringLiteral("Timestamp readback failed"));
    return read->bytes;
}

QJsonArray array(const QList<double> &values)
{
    QJsonArray result;
    for (const auto value : values) result.append(value);
    return result;
}
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Measure native GPU input, painting, composition and presentation"));
    parser.addHelpOption();
    parser.addOption({QStringLiteral("gpu-timestamps"), "Record diagnostic GPU timestamps and read them after measurement"});
    parser.addOption({QStringLiteral("high-load"), "Use 4096 square pixels, 24 occupied layers and a 512 pixel brush"});
    parser.addOption({QStringLiteral("offscreen"), "Measure rendering to a texture without native presentation waits"});
    parser.addOption({QStringLiteral("crop-preview"), "Include the crop mask, handles and eight guide lines in each display frame"});
    parser.addOptions({
        {QStringLiteral("input-hz"), "Generate all input samples on a fixed real-time schedule", "rate"},
        {QStringLiteral("display-hz"), "Scheduled presentation rate", "rate", "120"},
        {QStringLiteral("warmup-seconds"), "Real-time warmup duration", "seconds", "2"},
        {QStringLiteral("duration-seconds"), "Real-time measured duration", "seconds", "10"},
        {QStringLiteral("frames"), "Measured input frames", "count", "240"},
        {QStringLiteral("warmup"), "Input frames before measurement", "count", "32"},
        {QStringLiteral("history-strokes"), "Retained background strokes", "count", "250"},
        {QStringLiteral("paint-layer"), "Zero-based bottom-to-top drawing layer index", "index", "0"},
        {QStringLiteral("maximum-p95-ms"), "Require CPU submission and GPU P95 within this budget", "milliseconds"},
        {QStringLiteral("output"), "Write the measurements and final image digest as JSON", "file"}
    });
    parser.process(application);
    try {
        const auto count = [&](const QString &name, int minimum, int maximum) {
            bool valid = false;
            const int value = parser.value(name).toInt(&valid);
            require(valid && value >= minimum && value <= maximum, QStringLiteral("Invalid count: ") + name);
            return value;
        };
        const int frames = count(QStringLiteral("frames"), 1, 10000);
        const int warmup = count(QStringLiteral("warmup"), 1, 1000);
        const int history = count(QStringLiteral("history-strokes"), 0, 250);
        bool validBudget = false;
        const double maximum = parser.value(QStringLiteral("maximum-p95-ms")).toDouble(&validBudget);
        require(!parser.isSet(QStringLiteral("maximum-p95-ms")) || (validBudget && std::isfinite(maximum) && maximum > 0),
                QStringLiteral("The P95 budget must be a finite positive number"));
        const bool paced = parser.isSet(QStringLiteral("input-hz"));
        const int inputHz = paced ? count(QStringLiteral("input-hz"), 1, 1000) : 0;
        const int displayHz = count(QStringLiteral("display-hz"), 1, 240);
        const int warmupSeconds = count(QStringLiteral("warmup-seconds"), 1, 60);
        const int durationSeconds = count(QStringLiteral("duration-seconds"), 1, 60);
        require(paced || (!parser.isSet(QStringLiteral("display-hz")) &&
                         !parser.isSet(QStringLiteral("warmup-seconds")) &&
                         !parser.isSet(QStringLiteral("duration-seconds"))),
                QStringLiteral("Real-time durations and display rate require --input-hz"));
        require(!paced || (!parser.isSet(QStringLiteral("frames")) && !parser.isSet(QStringLiteral("warmup"))),
                QStringLiteral("Real-time input uses seconds instead of frame counts"));
        const bool highLoad = parser.isSet(QStringLiteral("high-load"));
        const int paintLayer = count(QStringLiteral("paint-layer"), 0, highLoad ? 23 : 0);
        const bool offscreen = parser.isSet(QStringLiteral("offscreen"));
        const bool cropPreview = parser.isSet(QStringLiteral("crop-preview"));
        QString diagnostic;
        const bool timestampQueries = parser.isSet(QStringLiteral("gpu-timestamps"));
        const auto device = Device::createNative(diagnostic, timestampQueries);
        require(bool(device), diagnostic);
        QWindow window;
        LibrePaint::Canvas::Gpu::Detail::setNativeSurfaceType(window);
        window.resize(512, 512);
        if (!offscreen) {
            window.show();
            require(QTest::qWaitForWindowExposed(&window), QStringLiteral("The native benchmark window must be exposed"));
        }
        RasterImage initial;
        initial.activeLayer = paintLayer;
        initial.size = highLoad ? QSize(4096, 4096) : QSize(2048, 2048);
        const QRect viewport = highLoad ? QRect(1536, 1536, 1024, 1024) : QRect(0, 0, 512, 512);
        for (int index = 0; index < (highLoad ? 24 : 1); ++index) {
            RasterLayer layer;
            layer.name = QStringLiteral("Layer %1").arg(index + 1);
            if (highLoad) {
                QByteArray rgba;
                for (const auto channel : {20 + index * 7, 80 + index * 3, 160 - index * 5, 128}) rgba.append(char(channel));
                const auto tileBytes = rgba.repeated(RgbaTile::ByteCount / 4);
                for (int y = 6; y < 10; ++y)
                    for (int x = 6; x < 10; ++x) layer.tiles.append({{x, y}, tileBytes});
            }
            initial.layers.append(layer);
        }
        OperationDocument document(initial);
        initial = {};

        Brush brush;
        brush.rgba = {20, 80, 160, 128};
        brush.diameter = highLoad ? 512 : 16;
        for (int i = 0; i < history; ++i) {
            const auto stroke = document.beginStroke(document.activeLayer(), brush, quint64(i + 1));
            const QPointF first = viewport.topLeft() + QPointF(32 + (i * 37) % 448, 32 + (i * 53) % 448);
            require(document.appendSamples(stroke, {{first, 0.5}, {first + QPointF(16, 8), 1}}) == Result::Success,
                    QStringLiteral("Background input was rejected"));
            require(document.commitStroke(stroke) == Result::Success, QStringLiteral("Background commit failed"));
        }
        Execution execution(device, (highLoad ? 512 : 128) * 1024 * 1024);
        const auto admitted = execution.open(document.snapshot());
        require(admitted.code == ExecutionCode::Success, admitted.diagnostic);
        TilePresentation presentation(device);
        LibrePaint::Canvas::Gpu::Detail::Texture texture;
        LibrePaint::Canvas::Gpu::Detail::TextureView textureView;
        std::unique_ptr<PresentationSurface> surface;
        const QSize targetSize = QSizeF(window.size() * window.devicePixelRatio()).toSize();
        LibrePaint::Canvas::Gpu::PresentationTarget target;
        if (offscreen) {
            WGPUTextureDescriptor descriptor{};
            descriptor.dimension = WGPUTextureDimension_2D;
            descriptor.size = {quint32(targetSize.width()), quint32(targetSize.height()), 1};
            descriptor.format = WGPUTextureFormat_RGBA8Unorm;
            descriptor.mipLevelCount = descriptor.sampleCount = 1;
            descriptor.usage = WGPUTextureUsage_RenderAttachment;
            texture.reset(wgpuDeviceCreateTexture(device->nativeDevice(), &descriptor));
            require(bool(texture), QStringLiteral("Benchmark texture allocation failed"));
            textureView.reset(wgpuTextureCreateView(texture.get(), nullptr));
            require(bool(textureView), QStringLiteral("Benchmark texture view allocation failed"));
            target = {device, textureView.get(), targetSize, WGPUTextureFormat_RGBA8Unorm};
        } else {
            surface = LibrePaint::Canvas::Gpu::Detail::createWindowSurface(device, window);
            require(surface->configure(targetSize), QStringLiteral("Native benchmark surface creation failed"));
        }
        using QuerySet = LibrePaint::Canvas::Gpu::Detail::Resource<WGPUQuerySet, wgpuQuerySetRelease>;
        QuerySet queries;
        using ComputePipeline = LibrePaint::Canvas::Gpu::Detail::Resource<WGPUComputePipeline, wgpuComputePipelineRelease>;
        ComputePipeline timestampMarker;
        quint32 timestampFrames = 0, timestampCapacity = 0;
        QList<quint32> measuredTimestampFrames;
        if (timestampQueries) {
            const int capacity = paced ? (warmupSeconds + durationSeconds) * displayHz + 1 : warmup + frames;
            require(capacity <= 4096, QStringLiteral("Diagnostic timestamps support at most 4096 submitted frames"));
            timestampCapacity = quint32(capacity);
            WGPUQuerySetDescriptor descriptor{};
            descriptor.type = WGPUQueryType_Timestamp;
            descriptor.count = timestampCapacity * 2;
            queries.reset(wgpuDeviceCreateQuerySet(device->nativeDevice(), &descriptor));
            require(bool(queries), QStringLiteral("GPU timestamp allocation failed"));
            WGPUShaderSourceWGSL code{};
            code.chain.sType = WGPUSType_ShaderSourceWGSL;
            code.code = {"@compute @workgroup_size(1) fn main() {}", WGPU_STRLEN};
            WGPUShaderModuleDescriptor shaderDescriptor{};
            shaderDescriptor.nextInChain = &code.chain;
            LibrePaint::Canvas::Gpu::Detail::Shader shader(wgpuDeviceCreateShaderModule(device->nativeDevice(), &shaderDescriptor));
            WGPUComputePipelineDescriptor pipelineDescriptor{};
            pipelineDescriptor.compute.module = shader.get();
            pipelineDescriptor.compute.entryPoint = {"main", WGPU_STRLEN};
            timestampMarker.reset(wgpuDeviceCreateComputePipeline(device->nativeDevice(), &pipelineDescriptor));
            require(bool(timestampMarker), QStringLiteral("GPU timestamp marker pipeline failed"));
        }
        auto stamp = [&](WGPUCommandEncoder commands, quint32 index) {
            WGPUComputePassTimestampWrites writes{queries.get(), index, WGPU_QUERY_SET_INDEX_UNDEFINED};
            WGPUComputePassDescriptor descriptor{};
            descriptor.timestampWrites = &writes;
            const auto pass = wgpuCommandEncoderBeginComputePass(commands, &descriptor);
            require(bool(pass), QStringLiteral("GPU timestamp marker failed"));
            wgpuComputePassEncoderSetPipeline(pass, timestampMarker.get());
            wgpuComputePassEncoderDispatchWorkgroups(pass, 1, 1, 1);
            wgpuComputePassEncoderEnd(pass);
            wgpuComputePassEncoderRelease(pass);
        };
        const auto stroke = document.beginStroke(document.activeLayer(), brush, quint64(history + 1));
        QList<double> cpu, gpu, inputTimes, acquireTimes, recordTimes, submitTimes;
        QList<double> newestInputAges, oldestInputAges, pendingCounts;
        qint64 peakResidentBytes = execution.residentBytes();
        const auto initialSamples = document.snapshot()->sampleCount;
        qint64 accepted = 0, submittedSamples = 0, missedFrames = 0, pendingInputCpuNs = 0;
        QElapsedTimer timeline;
        auto append = [&](qint64 index) {
            QElapsedTimer inputTimer;
            inputTimer.start();
            const InputSample input{viewport.topLeft() + QPointF(16 + (index * 7) % 480, 16 + (index * 11) % 480),
                qreal(1 + index % 8) / 8, 0, 0, 0, paced ? quint64(index * 1000 / inputHz) : quint64(index)};
            require(document.appendSamples(stroke, {input}) == Result::Success, QStringLiteral("Interactive input was rejected"));
            ++accepted;
            pendingInputCpuNs += inputTimer.nsecsElapsed();
        };
        auto render = [&](QElapsedTimer &timer, bool measured) {
            const auto frameStart = timer.nsecsElapsed();
            const auto requested = execution.request(admitted.handle, document.preview());
            require(requested.code == ExecutionCode::Success, requested.diagnostic);
            const auto inputFinished = timer.nsecsElapsed();
            const auto frame = offscreen ? execution.beginReadbackFrame() : execution.beginPresentationFrame(*surface);
            require(frame.code == ExecutionCode::Success, frame.diagnostic);
            const auto acquired = timer.nsecsElapsed();
            try {
                if (queries) {
                    require(timestampFrames < timestampCapacity, QStringLiteral("GPU timestamp capacity exhausted"));
                    stamp(frame.commands, timestampFrames * 2);
                }
                const auto projected = execution.project(admitted.handle, viewport);
                require(projected.code == RenderOutcome::Recorded, projected.diagnostic);
                if (!offscreen) target = {device, frame.target, targetSize, surface->format()};
                const qreal sx = qreal(targetSize.width()) / viewport.width();
                const qreal sy = qreal(targetSize.height()) / viewport.height();
                const QTransform transform(sx, 0, 0, sy, -viewport.x() * sx, -viewport.y() * sy);
                std::optional<LibrePaint::Canvas::Gpu::CropPreview> crop;
                if (cropPreview) {
                    crop.emplace();
                    crop->image = transform.mapRect(QRectF(QPointF(), document.snapshot()->size));
                    crop->crop = QRectF(targetSize.width() * 0.25, targetSize.height() * 0.25,
                                        targetSize.width() * 0.5, targetSize.height() * 0.5);
                    const auto rect = crop->crop;
                    for (const auto center : {rect.topLeft(), rect.topRight(), rect.bottomLeft(), rect.bottomRight(),
                                              QPointF(rect.center().x(), rect.top()), QPointF(rect.center().x(), rect.bottom()),
                                              QPointF(rect.left(), rect.center().y()), QPointF(rect.right(), rect.center().y())})
                        crop->handles.addRect(QRectF(center - QPointF(6.5, 6.5), QSizeF(13, 13)));
                    for (const qreal fraction : {0.2, 0.4, 0.6, 0.8}) {
                        crop->guides.append(QLineF(rect.left(), rect.top() + fraction * rect.height(), rect.right(), rect.top() + fraction * rect.height()));
                        crop->guides.append(QLineF(rect.left() + fraction * rect.width(), rect.top(), rect.left() + fraction * rect.width(), rect.bottom()));
                    }
                }
                const auto drawn = presentation.record(frame.commands, projected.version, target, transform, Qt::white, identityDisplay(projected.version->document()), {}, {}, {}, crop);
                require(drawn.recorded, drawn.diagnostic);
                if (queries) stamp(frame.commands, timestampFrames * 2 + 1);
                const auto recorded = timer.nsecsElapsed();
                const auto submitted = execution.submitFrame();
                require(submitted.code == ExecutionCode::Success, submitted.diagnostic);
                peakResidentBytes = std::max(peakResidentBytes, qint64(execution.residentBytes()));
                if (measured) {
                    cpu.append((timer.nsecsElapsed() - frameStart + pendingInputCpuNs) / 1e6);
                    inputTimes.append((inputFinished - frameStart + pendingInputCpuNs) / 1e6);
                    acquireTimes.append((acquired - inputFinished) / 1e6);
                    recordTimes.append((recorded - acquired) / 1e6);
                    submitTimes.append((timer.nsecsElapsed() - recorded) / 1e6);
                    if (paced && accepted > submittedSamples) {
                        const auto now = timeline.nsecsElapsed();
                        newestInputAges.append((now - (accepted - 1) * 1000000000LL / inputHz) / 1e6);
                        oldestInputAges.append((now - submittedSamples * 1000000000LL / inputHz) / 1e6);
                        pendingCounts.append(accepted - submittedSamples);
                    }
                }
                if (queries) {
                    if (measured) measuredTimestampFrames.append(timestampFrames);
                    ++timestampFrames;
                }
                submittedSamples = accepted;
                pendingInputCpuNs = 0;
            } catch (...) {
                execution.cancelFrame();
                throw;
            }
            QCoreApplication::processEvents();
        };
        if (paced) {
            const qint64 warmupNs = qint64(warmupSeconds) * 1000000000LL;
            const qint64 endNs = qint64(warmupSeconds + durationSeconds) * 1000000000LL;
            const qint64 totalSamples = qint64(warmupSeconds + durationSeconds) * inputHz;
            qint64 frameIndex = 0;
            timeline.start();
            while (timeline.nsecsElapsed() < endNs) {
                QElapsedTimer timer;
                timer.start();
                const auto now = timeline.nsecsElapsed();
                while (accepted < totalSamples && accepted * 1000000000LL / inputHz <= now) append(accepted);
                if (frameIndex * 1000000000LL / displayHz <= now) {
                    render(timer, now >= warmupNs);
                    const auto advance = LibrePaint::Canvas::Gpu::Benchmark::advanceFrame(
                        frameIndex, timeline.nsecsElapsed(), displayHz);
                    if (now >= warmupNs) missedFrames += advance.elapsedDeadlines;
                    frameIndex = advance.nextIndex;
                }
                const auto nextInput = accepted < totalSamples ? accepted * 1000000000LL / inputHz : endNs;
                const auto nextFrame = frameIndex * 1000000000LL / displayHz;
                const auto remaining = std::min(nextInput, nextFrame) - timeline.nsecsElapsed();
                if (remaining > 0) QThread::usleep(ulong(std::min(remaining / 1000, qint64(2000))));
            }
            while (accepted < totalSamples) append(accepted);
            QElapsedTimer timer;
            timer.start();
            render(timer, false);
            require(accepted == totalSamples, QStringLiteral("Scheduled input count differs from accepted input"));
        } else {
            for (int i = 0; i < warmup + frames; ++i) {
                QElapsedTimer timer;
                timer.start();
                append(i);
                render(timer, i >= warmup);
            }
        }
        require(document.preview()->sampleCount == initialSamples + accepted,
                QStringLiteral("Accepted input was lost from the retained operation"));
        require(document.commitStroke(stroke) == Result::Success, QStringLiteral("Interactive commit failed"));
        if (queries) {
            const auto values = timestamps(*device, queries.get(), timestampFrames * 2);
            const float period = wgpuQueueGetTimestampPeriod(device->queue());
            require(std::isfinite(period) && period > 0, QStringLiteral("GPU timestamp period is invalid"));
            for (const auto index : measuredTimestampFrames) {
                quint64 start = 0, end = 0;
                std::memcpy(&start, values.constData() + index * 2 * sizeof(quint64), sizeof(start));
                std::memcpy(&end, values.constData() + (index * 2 + 1) * sizeof(quint64), sizeof(end));
                require(end > start, QStringLiteral("GPU timestamp markers did not produce a positive elapsed time"));
                gpu.append(double(end - start) * period / 1e6);
            }
            require(gpu.size() == cpu.size(), QStringLiteral("GPU timestamps are missing for measured frames"));
        }
        const auto pixels = execution.readPixels(admitted.handle, document.snapshot(), viewport);
        require(pixels.code == ExecutionCode::Success, pixels.diagnostic);
        QByteArray image;
        QDataStream stream(&image, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        for (const auto &tile : pixels.tiles) {
            stream << qint32(tile.coordinate.x()) << qint32(tile.coordinate.y());
            stream.writeRawData(tile.rgba.constData(), int(tile.rgba.size()));
        }
        WGPUAdapterInfo info{};
        require(wgpuAdapterGetInfo(device->adapter(), &info) == WGPUStatus_Success, QStringLiteral("Native GPU information is unavailable"));
        const QString deviceName = QString::fromUtf8(info.device.data, qsizetype(info.device.length));
        const auto backend = info.backendType == WGPUBackendType_Metal ? QStringLiteral("Metal") : QStringLiteral("Vulkan");
        const int deviceType = int(info.adapterType);
        wgpuAdapterInfoFreeMembers(info);
        const QJsonObject result{
            {"schemaVersion", 3}, {"diagnosticGpuTimestamps", timestampQueries}, {"target", offscreen ? "texture" : "window"},
            {"completionSynchronized", offscreen},
            {"displayRefreshRateHz", window.screen() ? window.screen()->refreshRate() : 0}, {"backend", backend},
            {"cropPreview", cropPreview},
            {"deviceName", deviceName}, {"deviceType", deviceType},
            {"workload", highLoad ? "high-load" : "small"}, {"pacedInput", paced},
            {"inputHz", inputHz}, {"scheduledDisplayHz", paced ? displayHz : 0},
            {"warmupSeconds", paced ? warmupSeconds : 0}, {"durationSeconds", paced ? durationSeconds : 0},
            {"acceptedInputSamples", accepted}, {"submittedInputSamples", submittedSamples},
            {"missedScheduledFrames", missedFrames}, {"actualDisplayP95Ms", QJsonValue(QJsonValue::Null)},
            {"newestInputToSubmissionMs", array(newestInputAges)},
            {"oldestInputToSubmissionMs", array(oldestInputAges)}, {"pendingInputSamples", array(pendingCounts)},
            {"documentSize", QJsonArray{document.snapshot()->size.width(), document.snapshot()->size.height()}},
            {"layerCount", document.snapshot()->layers.size()}, {"paintLayerIndex", paintLayer}, {"brushDiameter", brush.diameter},
            {"brushSpacing", brush.spacing}, {"brushRgba", QJsonArray{20, 80, 160, 128}},
            {"initialRasterTiles", highLoad ? 384 : 0}, {"gpuTileBudgetBytes", (highLoad ? 512 : 128) * 1024 * 1024},
            {"viewport", QJsonArray{viewport.x(), viewport.y(), viewport.width(), viewport.height()}},
            {"surfaceSize", QJsonArray{targetSize.width(), targetSize.height()}},
            {"historyStrokes", history}, {"frames", cpu.size()}, {"warmupFrames", paced ? QJsonValue(QJsonValue::Null) : QJsonValue(warmup)},
            {"cpuSubmissionMs", array(cpu)}, {"completedGpuMs", array(gpu)},
            {"inputMs", array(inputTimes)}, {"acquireMs", array(acquireTimes)},
            {"recordMs", array(recordTimes)}, {"submitMs", array(submitTimes)},
            {"cpuSubmissionP95Ms", percentile95(cpu)},
            {"gpuP95Ms", gpu.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(percentile95(gpu))},
            {"residentTileBytes", qint64(execution.residentBytes())}, {"peakResidentTileBytes", peakResidentBytes},
            {"projectionSha256", QString::fromLatin1(QCryptographicHash::hash(image, QCryptographicHash::Sha256).toHex())}
        };
        const auto json = QJsonDocument(result).toJson();
        if (parser.isSet(QStringLiteral("output"))) {
            QSaveFile output(parser.value(QStringLiteral("output")));
            require(output.open(QIODevice::WriteOnly), output.errorString());
            require(output.write(json) == json.size() && output.commit(), output.errorString());
        } else QTextStream(stdout) << json;
        device->finish();
        if (parser.isSet(QStringLiteral("maximum-p95-ms")) &&
            ((!offscreen && gpu.isEmpty()) || percentile95(cpu) > maximum || percentile95(gpu) > maximum)) {
            QTextStream(stderr) << "Frame P95 exceeds the requested budget or window GPU timestamps are unavailable\n";
            return 2;
        }
        return 0;
    } catch (const std::exception &error) {
        QTextStream(stderr) << error.what() << '\n';
        return 1;
    }
}

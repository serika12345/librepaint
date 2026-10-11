/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "SurfaceRenderer.h"
#include "KisGpuBrushStroke.h"
#include "KisGpuBrushInputWorkloads.h"
#include "KisGpuLayerDocument.h"
#include "KisGpuLayerProjection.h"
#include "KisGpuTestDevice.h"
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWindow>
#include "KisGpuWindowTestEventLoop.h"
#include <QTimer>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <stdexcept>
#include <exception>

namespace {
using Store = KisGpuTileStore;
using Document = KisGpuLayerDocument;
using Session = KisGpuEditSession;
using Brush = KisGpuBrushStroke;
using Surface = Krita::Canvas::SurfaceRenderer;

template<typename Predicate>
void until(Predicate ready)
{
    if (!waitForGpuWindow(ready)) throw std::runtime_error("Input/display completion timed out or interrupted");
}

void require(bool valid, const char *diagnostic)
{
    if (!valid) throw std::runtime_error(diagnostic);
}

QJsonValue distribution(const QJsonArray &samples, const QString &metric)
{
    QVector<double> values;
    for (const auto &sample : samples) {
        const auto value = sample.toObject().value(metric);
        if (value.isDouble()) values.push_back(value.toDouble());
    }
    if (values.isEmpty()) return QJsonValue::Null;
    std::sort(values.begin(), values.end());
    const auto middle = values.size() / 2;
    return QJsonObject {
        {"count", values.size()},
        {"median", values.size() % 2 ? values[middle] : (values[middle - 1] + values[middle]) / 2},
        {"p95", values[qsizetype(std::ceil(values.size() * .95)) - 1]}
    };
}

struct Run {
    QJsonObject report;
    bool displayedEverySample = true;
};

Run execute(KisGpuTestDevice &gpu, QWindow &window, Surface &surface,
            bool high, bool replace, bool reuseImage, int sampleCount, const QString &imageDirectory)
{
    const KisGpuBrushInputWorkload workload(high);
    const auto bounds = workload.bounds, occupied = workload.occupied;
    const auto layerCount = workload.layerCount;
    auto store = std::make_shared<Store>(gpu.owner, 384 * 1024 * 1024);
    Document document(store, 8);
    Document::LayerId target = 0;
    for (int i = 0; i < layerCount; ++i) {
        const auto added = document.addLayer(QStringLiteral("Layer %1").arg(i));
        require(added.result == Document::Result::Accepted, "Cannot add benchmark layer");
        target = added.id;
        const auto edit = document.begin(target);
        require(edit.result == Document::Result::Accepted, "Cannot seed benchmark layer");
        require(edit.session->append(edit.token, {workload.seed(i)}, occupied) == Session::Result::Accepted,
                "Cannot paint benchmark layer");
        require(document.commit(edit.token) == Document::Result::Accepted, "Cannot commit benchmark layer");
        until([&] {
            document.poll();
            require(document.state() != Document::State::Failed, "Benchmark layer failed");
            return document.state() == Document::State::Idle;
        });
    }
    KisGpuLayerProjection projection(*store, bounds);
    const auto edit = document.begin(target);
    require(edit.result == Document::Result::Accepted, "Cannot begin measured input");
    const auto settings = workload.settings();
    Brush brush(*edit.session, edit.token, settings, bounds);
    Krita::Canvas::GpuRenderer::View view;
    const auto physicalWidth = qRound(window.width() * window.devicePixelRatio());
    const auto physicalHeight = qRound(window.height() * window.devicePixelRatio());
    view.canvasToTarget.scale(double(physicalWidth) / bounds.width(), double(physicalHeight) / bounds.height());
    Store::TextureSnapshot image;
    QJsonArray samples;
    Run result;
    const bool presentationTimingAvailable = Surface::presentationClockNanoseconds().has_value();
    quint64 maximumObservedBytes = gpu.owner.memoryStatistics().reservedBytes;
    const auto observeMemory = [&] {
        maximumObservedBytes = std::max(maximumObservedBytes, gpu.owner.memoryStatistics().reservedBytes);
    };
    for (int iteration = 0; iteration < sampleCount + 2; ++iteration) {
        const auto before = store->statistics();
        const auto beforeDisplay = surface.statistics();
        const auto cpuStart = std::clock();
        QElapsedTimer elapsed; elapsed.start();
        qint64 presentCallNs = 0, pollCallNs = 0;
        const auto pollSurface = [&] {
            const auto start = elapsed.nsecsElapsed();
            surface.poll();
            pollCallNs += elapsed.nsecsElapsed() - start;
        };
        const auto inputTime = Surface::presentationClockNanoseconds();
        const auto input = workload.samples(iteration, replace);
        const auto update = replace ? brush.replace(input) : brush.append(input);
        require(update.result == Brush::Result::Accepted, "Measured brush input was rejected");
        observeMemory();
        require(projection.setLayers(document.projectionLayers()) == Store::Error::None,
                "Measured projection was rejected");
        observeMemory();
        until([&] {
            document.poll(); projection.poll();
            require(projection.status() != Store::Status::Failed, "Measured projection failed");
            return projection.status() == Store::Status::Succeeded;
        });
        const double projectionMs = elapsed.nsecsElapsed() / 1.0e6;
        auto nextImage = reuseImage
            ? store->textureSnapshot(projection.snapshot().pixels, bounds, std::move(image))
            : store->textureSnapshot(projection.snapshot().pixels, bounds);
        require(nextImage.error == Store::Error::None, "Measured texture snapshot was rejected");
        image = std::move(nextImage);
        observeMemory();
        until([&] {
            store->poll();
            require(image.completion.status() != Store::Status::Failed, "Measured texture snapshot failed");
            return image.completion.status() == Store::Status::Succeeded;
        });
        const double textureMs = elapsed.nsecsElapsed() / 1.0e6;
        Surface::Frame frame;
        int retries = 0;
        until([&] {
            pollSurface();
            const auto start = elapsed.nsecsElapsed();
            frame = surface.present(image, view);
            presentCallNs += elapsed.nsecsElapsed() - start;
            if (frame.accepted()) return true;
            require(frame.error == Surface::Error::ResizePending || frame.error == Surface::Error::QueueFull,
                    "Measured presentation was rejected");
            ++retries;
            return false;
        });
        const double submissionMs = elapsed.nsecsElapsed() / 1.0e6;
        observeMemory();
        until([&] {
            pollSurface();
            require(frame.rendering.status() != Store::Status::Failed, "Measured canvas rendering failed");
            return frame.rendering.status() == Store::Status::Succeeded;
        });
        const double renderingMs = elapsed.nsecsElapsed() / 1.0e6;
        until([&] {
            pollSurface();
            return frame.presentation.status() != Surface::Presentation::Status::Pending;
        });
        pollSurface();
        const double terminalMs = elapsed.nsecsElapsed() / 1.0e6;
        const double cpuMs = double(std::clock() - cpuStart) * 1000 / CLOCKS_PER_SEC;
        const auto after = store->statistics();
        const auto afterDisplay = surface.statistics();
        require(after.pixelUploadBytes == before.pixelUploadBytes
                && after.pixelReadbackBytes == before.pixelReadbackBytes, "Measured drawing transferred CPU pixels");
        const auto displayedTime = frame.presentation.hostTimeNanoseconds();
        const auto observedTime = Surface::presentationClockNanoseconds();
        const bool displayed = frame.presentation.status() == Surface::Presentation::Status::Presented;
        if (displayed) require(inputTime && displayedTime && observedTime
                               && *displayedTime >= *inputTime && *displayedTime <= *observedTime,
                               "Presentation timestamp is outside the input/observation interval");
        if (iteration < 2) continue;
        result.displayedEverySample &= displayed;
        const auto status = frame.presentation.status();
        const QString statusName = status == Surface::Presentation::Status::Presented ? QStringLiteral("presented")
            : status == Surface::Presentation::Status::Skipped ? QStringLiteral("skipped")
            : status == Surface::Presentation::Status::Abandoned ? QStringLiteral("abandoned")
            : QStringLiteral("unavailable");
        samples.append(QJsonObject {
            {"inputSamples", input.size()}, {"inputToProjectionMs", projectionMs},
            {"inputToTextureReadyMs", textureMs}, {"inputToSubmissionMs", submissionMs},
            {"surfacePresentCallMs", presentCallNs / 1.0e6}, {"surfacePollCallMs", pollCallNs / 1.0e6},
            {"surfaceAcquireCallMs", (afterDisplay.acquireCallNanoseconds - beforeDisplay.acquireCallNanoseconds) / 1.0e6},
            {"surfaceNativePresentCallMs", (afterDisplay.nativePresentCallNanoseconds - beforeDisplay.nativePresentCallNanoseconds) / 1.0e6},
            {"inputToRenderCompletionMs", renderingMs},
            {"inputToPresentationMs", displayed ? QJsonValue(double(*displayedTime - *inputTime) / 1.0e6) : QJsonValue(QJsonValue::Null)},
            {"inputToTerminalNotificationMs", terminalMs}, {"cpuMs", cpuMs},
            {"presentationStatus", statusName}, {"presentationRetries", retries},
            {"submissions", qint64(after.submissions - before.submissions)},
            {"computeDispatches", qint64(after.computeDispatches - before.computeDispatches)},
            {"commandUploadBytes", qint64(after.commandUploadBytes - before.commandUploadBytes)},
            {"pixelUploadBytes", qint64(after.pixelUploadBytes - before.pixelUploadBytes)},
            {"pixelReadbackBytes", qint64(after.pixelReadbackBytes - before.pixelReadbackBytes)},
            {"tileCopyBytes", qint64(after.tileCopyBytes - before.tileCopyBytes)},
            {"textureCopyBytes", qint64(after.textureCopyBytes - before.textureCopyBytes)},
            {"imageTextureAllocationBytes", qint64(after.imageTextureAllocationBytes - before.imageTextureAllocationBytes)},
            {"canvasSubmissions", qint64(afterDisplay.rendering.submissions - beforeDisplay.rendering.submissions)},
            {"canvasParameterUploadBytes", qint64(afterDisplay.rendering.parameterUploadBytes - beforeDisplay.rendering.parameterUploadBytes)},
            {"deviceReservedBytes", qint64(gpu.owner.memoryStatistics().reservedBytes)},
            {"storeResidentBytes", qint64(after.residentBytes)}, {"cpuRecoveryBytes", qint64(document.recoveryBytes())}
        });
    }
    // The sole verification read is outside every measured interval and counter delta.
    const auto pixels = gpu.read(image.texture(), bounds.size());
    require(pixels.size() == bounds.width() * bounds.height() * 4, "Cannot identify the measured final image");
    QJsonObject summary;
    for (const auto &metric : {"inputToProjectionMs", "inputToTextureReadyMs", "inputToSubmissionMs",
                               "surfacePresentCallMs", "surfacePollCallMs", "surfaceAcquireCallMs",
                               "surfaceNativePresentCallMs", "inputToRenderCompletionMs",
                               "inputToPresentationMs", "inputToTerminalNotificationMs", "cpuMs"}) {
        summary.insert(QString::fromLatin1(metric), distribution(samples, QString::fromLatin1(metric)));
    }
    result.report = {
        {"workload", high ? "layer-projection" : "small"}, {"mode", replace ? "replace" : "append"},
        {"canvasSize", QJsonArray{bounds.width(), bounds.height()}},
        {"initialOccupiedBounds", QJsonArray{occupied.x(), occupied.y(), occupied.width(), occupied.height()}},
        {"layerCount", layerCount}, {"brushDiameter", settings.diameter.width()}, {"brushSpacing", settings.spacing},
        {"physicalTargetSize", QJsonArray{physicalWidth, physicalHeight}},
        {"presentationTimingAvailable", presentationTimingAvailable}, {"displayedEverySample", result.displayedEverySample},
        {"maximumObservedDeviceReservedBytes", qint64(maximumObservedBytes)},
        {"finalImageSha256", QString::fromLatin1(QCryptographicHash::hash(pixels, QCryptographicHash::Sha256).toHex())},
        {"samples", samples}, {"summary", summary}
    };
    if (!imageDirectory.isEmpty()) {
        const auto name = QStringLiteral("%1-%2.rgba.zlib")
            .arg(high ? QStringLiteral("layer-projection") : QStringLiteral("small"),
                 replace ? QStringLiteral("replace") : QStringLiteral("append"));
        const auto compressed = qCompress(pixels, 1);
        QSaveFile file(QDir(imageDirectory).filePath(name));
        require(compressed.size() > 4 && file.open(QIODevice::WriteOnly), "Cannot create verification image");
        // Remove Qt's uncompressed-length prefix, leaving a standard zlib stream of RGBA8 bytes.
        require(file.write(compressed.constData() + 4, compressed.size() - 4) == compressed.size() - 4
                    && file.commit(), "Cannot save verification image");
        result.report.insert(QStringLiteral("verificationImage"), name);
    }
    require(document.cancel(edit.token) == Document::Result::Accepted, "Cannot close measured input");
    return result;
}
}

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Generated brush input through GPU document, projection and native display"));
    parser.addHelpOption();
    const QCommandLineOption samples(QStringLiteral("samples"), QStringLiteral("Measured updates after two warmups (1..1000)"), QStringLiteral("count"), QStringLiteral("15"));
    const QCommandLineOption workload(QStringLiteral("workload"), QStringLiteral("small, layer-projection or all"), QStringLiteral("name"), QStringLiteral("all"));
    const QCommandLineOption mode(QStringLiteral("mode"), QStringLiteral("append, replace or all"), QStringLiteral("name"), QStringLiteral("all"));
    const QCommandLineOption snapshot(QStringLiteral("snapshot-policy"), QStringLiteral("fresh or reuse"), QStringLiteral("name"), QStringLiteral("reuse"));
    const QCommandLineOption images(QStringLiteral("image-directory"), QStringLiteral("Save final RGBA8 verification images outside measurement"), QStringLiteral("directory"));
    parser.addOptions({samples, workload, mode, snapshot, images});
    if (!parser.parse(application.arguments())) {
        std::fprintf(stderr, "%s\n", qPrintable(parser.errorText()));
        return 2;
    }
    if (parser.isSet(QStringLiteral("help"))) parser.showHelp();
    bool validCount = false;
    const int count = parser.value(samples).toInt(&validCount);
    const auto selectedWorkload = parser.value(workload), selectedMode = parser.value(mode), selectedSnapshot = parser.value(snapshot);
    if (!validCount || count < 1 || count > 1000
        || (selectedWorkload != "small" && selectedWorkload != "layer-projection" && selectedWorkload != "all")
        || (selectedMode != "append" && selectedMode != "replace" && selectedMode != "all")
        || (selectedSnapshot != "fresh" && selectedSnapshot != "reuse")
        || (parser.isSet(images) && parser.value(images).isEmpty())
        || !parser.positionalArguments().isEmpty()) {
        std::fputs("Expected --samples 1..1000, --workload small|layer-projection|all, --mode append|replace|all, --snapshot-policy fresh|reuse, --image-directory nonempty-directory\n", stderr);
        return 2;
    }
    try {
        const auto imageDirectory = parser.value(images);
        require(imageDirectory.isEmpty() || QDir().mkpath(imageDirectory), "Cannot create verification image directory");
        KisGpuTestDevice gpu;
        QWindow window;
        Surface::prepareWindow(window);
        window.resize(512, 512);
        window.show();
        until([&] { return window.isExposed(); });
        Surface surface(gpu.owner, window);
        QJsonArray runs;
        bool displayedEverySample = true;
        for (const bool high : {false, true}) for (const bool replace : {false, true}) {
            if (selectedWorkload != "all" && selectedWorkload != (high ? "layer-projection" : "small")) continue;
            if (selectedMode != "all" && selectedMode != (replace ? "replace" : "append")) continue;
            const auto run = execute(gpu, window, surface, high, replace, selectedSnapshot == "reuse", count, imageDirectory);
            runs.append(run.report);
            displayedEverySample &= run.displayedEverySample;
        }
        require(gpu.owner.errorCount() == 0, "Uncaptured GPU validation errors");
        QFile executable(QCoreApplication::applicationFilePath());
        require(executable.open(QIODevice::ReadOnly), "Cannot identify measurement executable");
        const QJsonDocument report(QJsonObject {
            {"schemaVersion", 1}, {"adapter", gpu.name}, {"gpuDependencyVersion", "27.0.4.0"},
            {"executableSha256", QString::fromLatin1(QCryptographicHash::hash(executable.readAll(), QCryptographicHash::Sha256).toHex())},
            {"inputSource", "generated normalized position and pressure"},
            {"scheduling", "one update at a time; Qt event loop with 1 ms GPU polling"},
            {"snapshotPolicy", selectedSnapshot},
            {"colorSpace", "RGBA8 sRGB; Over"}, {"memoryAccounting", "logical reservations; physical driver memory excluded"},
            {"samplesPerRun", count}, {"warmupsPerRun", 2}, {"displayedEverySample", displayedEverySample}, {"runs", runs}
        });
        const auto json = report.toJson();
        require(std::fwrite(json.constData(), 1, size_t(json.size()), stdout) == size_t(json.size()), "Cannot write measurement JSON");
        return displayedEverySample ? 0 : 3;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}

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
#include <rhi/qrhi.h>
#include <gpu/KisGpuExecution.h>
#include <gpu/KisGpuTilePresentation.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>

using namespace Krita::Image::Gpu;
using Krita::Canvas::Gpu::TilePresentation;

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
    parser.addOption({QStringLiteral("offscreen"), "Measure rendering to a texture without native presentation waits"});
    parser.addOptions({
        {QStringLiteral("frames"), "Measured input frames", "count", "240"},
        {QStringLiteral("warmup"), "Input frames before measurement", "count", "32"},
        {QStringLiteral("history-strokes"), "Retained background strokes", "count", "250"},
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
        const bool offscreen = parser.isSet(QStringLiteral("offscreen"));
        QString diagnostic;
        const auto device = Device::createNative(diagnostic);
        require(bool(device), diagnostic);
        auto &rhi = device->rhi();
        QWindow window;
        window.setSurfaceType(device->vulkanInstance() ? QSurface::VulkanSurface : QSurface::MetalSurface);
#if QT_CONFIG(vulkan)
        window.setVulkanInstance(device->vulkanInstance());
#endif
        window.resize(512, 512);
        if (!offscreen) {
            window.show();
            require(QTest::qWaitForWindowExposed(&window), QStringLiteral("The native benchmark window must be exposed"));
        }
        OperationDocument document(QSize(2048, 2048));
        Brush brush;
        brush.rgba = {20, 80, 160, 128};
        brush.diameter = 16;
        for (int i = 0; i < history; ++i) {
            const auto stroke = document.beginStroke(document.activeLayer(), brush, quint64(i + 1));
            const QPointF first(32 + (i * 37) % 448, 32 + (i * 53) % 448);
            require(document.appendSamples(stroke, {{first, 0.5}, {first + QPointF(16, 8), 1}}) == Result::Success,
                    QStringLiteral("Background input was rejected"));
            require(document.commitStroke(stroke) == Result::Success, QStringLiteral("Background commit failed"));
        }
        Execution execution(device);
        const auto admitted = execution.open(document.snapshot());
        require(admitted.code == ExecutionCode::Success, admitted.diagnostic);
        TilePresentation presentation(device);
        std::unique_ptr<QRhiTexture> texture;
        std::unique_ptr<QRhiRenderPassDescriptor> pass;
        std::unique_ptr<QRhiSwapChain> surface;
        std::unique_ptr<QRhiTextureRenderTarget> textureTarget;
        if (offscreen) {
            const int side = qRound(512 * window.devicePixelRatio());
            texture.reset(rhi.newTexture(QRhiTexture::RGBA8, {side, side}, 1, QRhiTexture::RenderTarget));
            require(texture->create(), QStringLiteral("Benchmark texture allocation failed"));
            textureTarget.reset(rhi.newTextureRenderTarget({texture.get()}));
            pass.reset(textureTarget->newCompatibleRenderPassDescriptor());
            textureTarget->setRenderPassDescriptor(pass.get());
            require(textureTarget->create(), QStringLiteral("Benchmark texture target creation failed"));
        } else {
            surface.reset(rhi.newSwapChain());
            surface->setWindow(&window);
            surface->setFlags(QRhiSwapChain::NoVSync | QRhiSwapChain::SurfaceHasPreMulAlpha);
            pass.reset(surface->newCompatibleRenderPassDescriptor());
            surface->setRenderPassDescriptor(pass.get());
            require(surface->createOrResize(), QStringLiteral("Native benchmark surface creation failed"));
        }
        const auto stroke = document.beginStroke(document.activeLayer(), brush, quint64(history + 1));
        QList<double> cpu, gpu, inputTimes, acquireTimes, recordTimes, submitTimes;
        const QRect viewport(0, 0, 512, 512);
        for (int i = 0; i < warmup + frames; ++i) {
            QElapsedTimer timer;
            timer.start();
            const InputSample input{{qreal(16 + (i * 7) % 480), qreal(16 + (i * 11) % 480)}, qreal(1 + i % 8) / 8, 0, 0, 0, quint64(i)};
            require(document.appendSamples(stroke, {input}) == Result::Success, QStringLiteral("Interactive input was rejected"));
            const auto requested = execution.request(admitted.handle, document.preview());
            require(requested.code == ExecutionCode::Success, requested.diagnostic);
            const auto inputFinished = timer.nsecsElapsed();
            const auto frame = offscreen ? execution.beginReadbackFrame() : execution.beginPresentationFrame(*surface);
            require(frame.code == ExecutionCode::Success, frame.diagnostic);
            const auto acquired = timer.nsecsElapsed();
            try {
                const double completedGpuMs = frame.commands->lastCompletedGpuTime() * 1000;
                const auto projected = execution.project(admitted.handle, viewport);
                require(projected.code == RenderOutcome::Recorded, projected.diagnostic);
                const auto target = offscreen ? static_cast<QRhiRenderTarget *>(textureTarget.get()) : frame.target;
                const auto drawn = presentation.record(*frame.commands, projected.version, *target,
                                                       QTransform::fromScale(window.devicePixelRatio(), window.devicePixelRatio()), Qt::white);
                require(drawn.recorded, drawn.diagnostic);
                const auto recorded = timer.nsecsElapsed();
                const auto submitted = execution.submitFrame();
                require(submitted.code == ExecutionCode::Success, submitted.diagnostic);
                if (offscreen) rhi.finish();
                if (i >= warmup) {
                    cpu.append(timer.nsecsElapsed() / 1e6);
                    inputTimes.append(inputFinished / 1e6);
                    acquireTimes.append((acquired - inputFinished) / 1e6);
                    recordTimes.append((recorded - acquired) / 1e6);
                    submitTimes.append(cpu.back() - recorded / 1e6);
                    if (completedGpuMs > 0) gpu.append(completedGpuMs);
                }
            } catch (...) {
                execution.cancelFrame();
                throw;
            }
            QCoreApplication::processEvents();
        }
        require(document.commitStroke(stroke) == Result::Success, QStringLiteral("Interactive commit failed"));
        const auto pixels = execution.readPixels(admitted.handle, document.snapshot(), viewport);
        require(pixels.code == ExecutionCode::Success, pixels.diagnostic);
        QByteArray image;
        QDataStream stream(&image, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        for (const auto &tile : pixels.tiles) {
            stream << qint32(tile.coordinate.x()) << qint32(tile.coordinate.y());
            stream.writeRawData(tile.rgba.constData(), int(tile.rgba.size()));
        }
        const auto driver = rhi.driverInfo();
        const auto targetSize = offscreen ? textureTarget->pixelSize() : surface->currentPixelSize();
        const QJsonObject result{
            {"schemaVersion", 1}, {"target", offscreen ? "texture" : "window"},
            {"completionSynchronized", offscreen},
            {"displayRefreshRateHz", window.screen() ? window.screen()->refreshRate() : 0}, {"backend", QString::fromLatin1(rhi.backendName())},
            {"deviceName", QString::fromUtf8(driver.deviceName)}, {"deviceType", int(driver.deviceType)},
            {"documentSize", QJsonArray{2048, 2048}}, {"viewport", QJsonArray{0, 0, 512, 512}},
            {"surfaceSize", QJsonArray{targetSize.width(), targetSize.height()}},
            {"historyStrokes", history}, {"frames", frames}, {"warmupFrames", warmup},
            {"cpuSubmissionMs", array(cpu)}, {"completedGpuMs", array(gpu)},
            {"inputMs", array(inputTimes)}, {"acquireMs", array(acquireTimes)},
            {"recordMs", array(recordTimes)}, {"submitMs", array(submitTimes)},
            {"cpuSubmissionP95Ms", percentile95(cpu)},
            {"gpuP95Ms", gpu.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(percentile95(gpu))},
            {"residentTileBytes", qint64(execution.residentBytes())},
            {"projectionSha256", QString::fromLatin1(QCryptographicHash::hash(image, QCryptographicHash::Sha256).toHex())}
        };
        const auto json = QJsonDocument(result).toJson();
        if (parser.isSet(QStringLiteral("output"))) {
            QSaveFile output(parser.value(QStringLiteral("output")));
            require(output.open(QIODevice::WriteOnly), output.errorString());
            require(output.write(json) == json.size() && output.commit(), output.errorString());
        } else QTextStream(stdout) << json;
        rhi.finish();
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

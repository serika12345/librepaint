/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuPaintWorkloads.h"
#include "KisGpuTestDevice.h"
#include "KoColor.h"
#include "KoColorProfile.h"
#include "KoColorSpace.h"
#include "KoColorSpaceRegistry.h"
#include "KoCompositeOpRegistry.h"
#include "KoTestConfig.h"
#include "kis_paint_device.h"
#include "kis_painter.h"
#include "kis_pixel_selection.h"
#include "kis_selection.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRunnable>
#include <QSemaphore>
#include <QStandardPaths>
#include <QThreadPool>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <memory>
#include <stdexcept>

namespace {
using Store = KisGpuTileStore;
using namespace KisGpuPaintMeasurements;
KoColor bgraColor(quint32 rgba, const KoColorSpace *space)
{
    const quint8 bgra[]{ quint8(rgba >> 16), quint8(rgba >> 8), quint8(rgba), quint8(rgba >> 24) };
    return KoColor(bgra, space);
}
void waitForGpu(KisGpuTestDevice &gpu, Store &store, const Store::Completion &completion)
{
    wgpuDevicePoll(gpu.device, true, nullptr);
    QElapsedTimer timer;
    timer.start();
    for (;;) {
        store.poll();
        if (completion.status() == Store::Status::Succeeded)
            return;
        if (completion.status() == Store::Status::Failed || timer.elapsed() > 5000)
            throw std::runtime_error("GPU completion failed");
    }
}
struct CpuInput {
    KisPaintDeviceSP base;
    QVector<KisPaintDeviceSP> sources;
    KisSelectionSP selection;
};
CpuInput prepareCpu(const Workload &workload, const KoColorSpace *space)
{
    CpuInput input;
    input.base = new KisPaintDevice(space);
    if (!workload.layerCount)
        input.base->fill(workload.bounds, bgraColor(BaseColor, space));
    for (int i = 0; i < (workload.layerCount ? workload.layerCount : workload.commands.size()); ++i) {
        KisPaintDeviceSP source = new KisPaintDevice(space);
        if (workload.layerCount)
            source->fill(workload.layerBounds, bgraColor(layerColor(i), space));
        else
            source->setDefaultPixel(bgraColor(workload.commands[i].rgba, space));
        input.sources.push_back(source);
    }
    if (!workload.layerCount && workload.commands.front().coverage != 255) {
        input.selection = new KisSelection;
        input.selection->pixelSelection()->select(workload.bounds, workload.commands.front().coverage);
    }
    return input;
}
struct CpuRun {
    KisPaintDeviceSP output;
    QJsonObject measurement;
};
CpuRun executeCpu(const Workload &workload, const CpuInput &input, QThreadPool &pool, int workers)
{
    const auto cpuStart = std::clock();
    QElapsedTimer timer;
    timer.start();
    CpuRun run;
    run.output = new KisPaintDevice(*input.base);
    // A job keeps its completion value alive until release() has returned.
    auto finished = std::make_shared<QSemaphore>();
    for (int worker = 0; worker < workers; ++worker)
        pool.start(QRunnable::create([&, worker, finished] {
            {
                KisPainter painter(run.output, input.selection);
                for (int y = workload.bounds.y() + worker * 64; y < workload.bounds.y() + workload.bounds.height();
                    y += workers * 64) {
                    const QRect stripe(workload.bounds.x(), y, workload.bounds.width(),
                        qMin(64, workload.bounds.y() + workload.bounds.height() - y));
                    for (qsizetype i = 0; i < input.sources.size(); ++i) {
                        const auto rect = stripe.intersected(
                            workload.layerCount ? workload.layerBounds : workload.commands[i].rectangle);
                        if (rect.isEmpty())
                            continue;
                        painter.setCompositeOpId(
                            workload.layerCount || workload.commands[i].operation == Store::CompositeOp::Over
                                ? COMPOSITE_OVER
                                : COMPOSITE_ERASE);
                        painter.setOpacityU8(workload.layerCount ? 255 : workload.commands[i].opacity);
                        painter.bitBlt(rect.topLeft(), input.sources[i], rect);
                    }
                }
            }
            // Finish painter notifications and release borrowed output references first.
            finished->release();
        }));
    finished->acquire(workers);
    run.measurement = { { "wallMs", timer.nsecsElapsed() / 1e6 },
        { "cpuMs", double(std::clock() - cpuStart) * 1000 / CLOCKS_PER_SEC } };
    return run;
}
QByteArray readRgba(KisPaintDeviceSP image, QRect bounds)
{
    QByteArray pixels(bounds.width() * bounds.height() * 4, Qt::Uninitialized);
    image->readBytes(reinterpret_cast<quint8 *>(pixels.data()), bounds);
    for (qsizetype i = 0; i < pixels.size(); i += 4)
        std::swap(pixels[i], pixels[i + 2]);
    return pixels;
}
QJsonObject comparePixels(const QByteArray &actual, const QByteArray &expected, QRect bounds)
{
    if (actual.size() != expected.size())
        throw std::runtime_error("Image sizes differ");
    quint64 different = 0;
    int maximum[4]{};
    QJsonObject firstDifference;
    for (qsizetype i = 0; i < actual.size(); i += 4) {
        bool differs = false;
        for (int c = 0; c < 4; ++c) {
            const int delta = std::abs(int(quint8(actual[i + c])) - int(quint8(expected[i + c])));
            maximum[c] = std::max(maximum[c], delta);
            differs |= delta != 0;
        }
        if (differs && firstDifference.isEmpty()) {
            QJsonArray cpuPixel, gpuPixel;
            for (int c = 0; c < 4; ++c) {
                cpuPixel.append(quint8(expected[i + c]));
                gpuPixel.append(quint8(actual[i + c]));
            }
            firstDifference = { { "x", bounds.x() + i / 4 % bounds.width() },
                { "y", bounds.y() + i / 4 / bounds.width() }, { "cpuRgba", cpuPixel }, { "gpuRgba", gpuPixel } };
        }
        different += differs;
    }
    return { { "exact", !different }, { "differentPixels", qint64(different) },
        { "maximumChannelDifference", QJsonArray{ maximum[0], maximum[1], maximum[2], maximum[3] } },
        { "firstDifference", firstDifference },
        { "cpuImageSha256",
            QString::fromLatin1(QCryptographicHash::hash(expected, QCryptographicHash::Sha256).toHex()) },
        { "gpuImageSha256",
            QString::fromLatin1(QCryptographicHash::hash(actual, QCryptographicHash::Sha256).toHex()) } };
}
QJsonObject summarize(const QJsonArray &samples)
{
    QJsonObject result{ { "samples", samples } };
    QStringList metrics{ QStringLiteral("wallMs"), QStringLiteral("cpuMs") };
    if (!samples.isEmpty() && samples.first().toObject().contains(QStringLiteral("gpuComputeMs")))
        metrics << QStringLiteral("gpuComputeMs");
    for (const auto &metric : metrics) {
        QVector<double> values;
        for (const auto &sample : samples)
            values.push_back(sample.toObject()[metric].toDouble());
        std::sort(values.begin(), values.end());
        const auto middle = values.size() / 2;
        result[metric]
            = QJsonObject{ { "median", values.size() % 2 ? values[middle] : (values[middle - 1] + values[middle]) / 2 },
                  { "p95", values[qsizetype(std::ceil(values.size() * .95)) - 1] } };
    }
    return result;
}
}
int main(int argc, char **argv)
{
    QStandardPaths::setTestModeEnabled(true);
    qputenv("KRITA_PLUGIN_PATH", KRITA_PLUGINS_DIR_FOR_TESTS);
    qputenv("EXTRA_RESOURCE_DIRS", KRITA_RESOURCE_DIRS_FOR_TESTS);
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({ "samples", "Samples after two warmups", "count", "15" });
    parser.addOption({ "workload", "Run one named workload", "name" });
    parser.addOption({ "gpu-timing", "Measure compute in a separate profiling run" });
    parser.process(app);
    bool valid = false;
    const int count = parser.value("samples").toInt(&valid);
    if (!valid || count < 1 || count > 1000) {
        std::fputs("--samples must be between 1 and 1000\n", stderr);
        return 2;
    }
    const auto inputs = workloads();
    if (parser.isSet("workload") && std::none_of(inputs.begin(), inputs.end(), [&](const auto &input) {
            return input.name == parser.value("workload");
        })) {
        std::fputs("Unknown workload\n", stderr);
        return 2;
    }
    bool imagesExact = true;
    try {
        const auto *space = KoColorSpaceRegistry::instance()->rgb8("sRGB-elle-V2-srgbtrc.icc");
        if (!space || !space->profile() || space->profile()->name() != "sRGB-elle-V2-srgbtrc.icc")
            throw std::runtime_error("Required CPU color profile unavailable");
        KisGpuTestDevice gpu(0, parser.isSet("gpu-timing"));
        QThreadPool pool;
        pool.setMaxThreadCount(4);
        QJsonArray reports;
        for (const auto &workload : inputs) {
            if (parser.isSet("workload") && parser.value("workload") != workload.name)
                continue;
            const auto input = prepareCpu(workload, space);
            Store store(gpu.owner, BudgetBytes);
            auto base = workload.layerCount ? store.paint(store.emptyVersion(), QVector<Store::PaintCommand>())
                                            : store.fill(store.emptyVersion(), workload.bounds, BaseColor);
            waitForGpu(gpu, store, base.completion);
            QVector<Store::Layer> layers;
            for (int i = 0; i < workload.layerCount; ++i) {
                auto layer = store.fill(store.emptyVersion(), workload.layerBounds, layerColor(i));
                waitForGpu(gpu, store, layer.completion);
                layers.push_back({ layer.version });
            }
            const auto executeGpu = [&] {
                const auto before = store.statistics();
                const auto start = std::clock();
                QElapsedTimer timer;
                timer.start();
                const auto edit = workload.layerCount ? store.project(base.version, layers, workload.bounds)
                                                      : store.paint(base.version, workload.commands);
                if (edit.error != Store::Error::None)
                    throw std::runtime_error("GPU operation rejected");
                const auto residentBeforeWait = store.statistics().residentBytes;
                waitForGpu(gpu, store, edit.completion);
                QJsonObject sample{ { "wallMs", timer.nsecsElapsed() / 1e6 },
                    { "cpuMs", double(std::clock() - start) * 1000 / CLOCKS_PER_SEC } };
                const auto after = store.statistics();
                sample["residentBytesBeforeWait"] = qint64(residentBeforeWait);
                sample["residentBytesAfterWait"] = qint64(after.residentBytes);
                sample["submissions"] = qint64(after.submissions - before.submissions);
                sample["computeDispatches"] = qint64(after.computeDispatches - before.computeDispatches);
                sample["commandUploadBytes"] = qint64(after.commandUploadBytes - before.commandUploadBytes);
                sample["tileCopyBytes"] = qint64(after.tileCopyBytes - before.tileCopyBytes);
                sample["pixelUploadBytes"] = qint64(after.pixelUploadBytes - before.pixelUploadBytes);
                sample["pixelReadbackBytes"] = qint64(after.pixelReadbackBytes - before.pixelReadbackBytes);
                sample["timingReadbackBytes"] = qint64(after.timingReadbackBytes - before.timingReadbackBytes);
                if (const auto time = edit.completion.gpuComputeNanoseconds())
                    sample["gpuComputeMs"] = double(*time) / 1e6;
                return std::make_pair(edit.version, sample);
            };
            QJsonObject comparison;
            {
                const auto single = executeCpu(workload, input, pool, 1),
                           parallel = executeCpu(workload, input, pool, 4);
                const auto image = readRgba(single.output, workload.bounds);
                if (image != readRgba(parallel.output, workload.bounds))
                    throw std::runtime_error("CPU worker counts produce different images");
                const auto deviceImage = executeGpu();
                comparison = comparePixels(gpu.read(deviceImage.first, workload.bounds), image, workload.bounds);
                imagesExact = imagesExact && comparison["exact"].toBool();
            }
            QJsonArray samples[3];
            for (int i = 0; i < count + 2; ++i)
                for (int order = 0; order < 3; ++order) {
                    const int mode = (i + order) % 3;
                    auto sample = mode == 2 ? executeGpu().second
                                            : executeCpu(workload, input, pool, mode == 0 ? 1 : 4).measurement;
                    if (i >= 2) {
                        sample["iteration"] = i - 2;
                        samples[mode].append(sample);
                    }
                }
            reports.push_back(QJsonObject{ { "workload", workload.name }, { "width", workload.bounds.width() },
                { "height", workload.bounds.height() },
                { "commands", workload.layerCount ? workload.layerCount : workload.commands.size() },
                { "layers", workload.layerCount }, { "cpuGpuComparison", comparison },
                { "cpu1", summarize(samples[0]) }, { "cpu4", summarize(samples[1]) },
                { "gpu", summarize(samples[2]) } });
        }
        if (reports.isEmpty())
            throw std::runtime_error("Unknown workload");
        if (gpu.owner.errorCount())
            throw std::runtime_error("Uncaptured GPU validation errors");
        QFile executable(QCoreApplication::applicationFilePath());
        if (!executable.open(QIODevice::ReadOnly))
            throw std::runtime_error("Cannot identify benchmark executable");
        const auto digest = QCryptographicHash::hash(executable.readAll(), QCryptographicHash::Sha256).toHex();
        const auto json = QJsonDocument(
            QJsonObject{ { "schema", 1 }, { "executableSha256", QString::fromLatin1(digest) }, { "adapter", gpu.name },
                { "wgpuNativeVersion", QString::number(wgpuGetVersion(), 16) }, { "validationEnabled", true },
                { "samplesPerMode", count }, { "warmupsPerMode", 2 }, { "cpuWorkers", QJsonArray{ 1, 4 } },
                { "colorProfile", QStringLiteral("sRGB-elle-V2-srgbtrc.icc") },
                { "gpuBudgetBytes", qint64(BudgetBytes) },
                { "cpuOperation", "KisPainter::bitBlt; immutable KisPaintDevice copy" }, { "cpuStripeHeight", 64 },
                { "preparedSourcesOutsideTiming", true }, { "includesDisplay", false },
                { "gpuTimingEnabled", parser.isSet("gpu-timing") }, { "workloads", reports } })
                              .toJson();
        if (std::fwrite(json.constData(), 1, size_t(json.size()), stdout) != size_t(json.size())
            || std::fflush(stdout) != 0)
            return 1;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
    return imagesExact ? 0 : 3;
}

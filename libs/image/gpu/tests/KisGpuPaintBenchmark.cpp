/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTestDevice.h"
#include "KisGpuPaintWorkloads.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <stdexcept>

namespace {
using namespace KisGpuPaintMeasurements;


struct Run {
    KisGpuTileStore::Version version;
    QJsonObject measurement;
};

void wait(KisGpuTestDevice &gpu, KisGpuTileStore &store, const QVector<KisGpuTileStore::Completion> &completions)
{
    wgpuDevicePoll(gpu.device, true, nullptr);
    QElapsedTimer timeout;
    timeout.start();
    for (;;) {
        store.poll();
        bool finished = true;
        for (const auto &completion : completions) {
            if (completion.status() == KisGpuTileStore::Status::Failed) throw std::runtime_error("GPU operation failed");
            finished &= completion.status() == KisGpuTileStore::Status::Succeeded;
        }
        if (finished) return;
        if (timeout.elapsed() > 5000) throw std::runtime_error("GPU completion timed out");
    }
}

Run execute(KisGpuTestDevice &gpu, KisGpuTileStore &store, const KisGpuTileStore::Version &base,
            const Workload &workload, const QVector<KisGpuTileStore::Layer> &layers, bool batched)
{
    Run result;
    result.version = base;
    QVector<KisGpuTileStore::Completion> completions;
    completions.reserve(batched ? 1 : layers.isEmpty() ? workload.commands.size() : layers.size());
    const auto before = store.statistics();
    const std::clock_t cpuStart = std::clock();
    QElapsedTimer elapsed;
    elapsed.start();
    auto retain = [&](const KisGpuTileStore::Edit &edit) {
        if (edit.error != KisGpuTileStore::Error::None) {
            throw std::runtime_error(QStringLiteral("GPU update rejected: workload=%1 mode=%2 error=%3 resident=%4 diagnostic=%5")
                .arg(workload.name, batched ? QStringLiteral("batched") : QStringLiteral("individual"))
                .arg(int(edit.error)).arg(store.statistics().residentBytes).arg(gpu.owner.lastError()).toStdString());
        }
        result.version = edit.version;
        completions.push_back(edit.completion);
    };
    if (!layers.isEmpty() && batched) {
        retain(store.project(base, layers, workload.bounds));
    } else if (!layers.isEmpty()) {
        for (const auto &layer : layers) {
            retain(store.composite(result.version, layer.pixels, workload.bounds, layer.operation, layer.opacity));
        }
    } else if (batched) {
        retain(store.paint(base, workload.commands));
    } else {
        for (const auto &command : workload.commands) {
            retain(store.paint(result.version, command.rectangle, command.rgba, command.operation,
                               command.opacity, command.coverage));
        }
    }
    const double enqueueMs = elapsed.nsecsElapsed() / 1.0e6;
    const quint64 residentBeforeWait = store.statistics().residentBytes;
    // Both modes wait once, after the complete command sequence, without pixel readback.
    wait(gpu, store, completions);
    const double wallMs = elapsed.nsecsElapsed() / 1.0e6;
    const double cpuMs = double(std::clock() - cpuStart) * 1000.0 / CLOCKS_PER_SEC;
    const auto after = store.statistics();
    result.measurement = {
        {"wallMs", wallMs}, {"cpuMs", cpuMs}, {"enqueueMs", enqueueMs},
        {"residentBytesBeforeWait", qint64(residentBeforeWait)},
        {"residentBytesAfterWait", qint64(after.residentBytes)},
        {"commandUploadBytes", qint64(after.commandUploadBytes - before.commandUploadBytes)},
        {"tileCopyBytes", qint64(after.tileCopyBytes - before.tileCopyBytes)},
        {"pixelUploadBytes", qint64(after.pixelUploadBytes - before.pixelUploadBytes)},
        {"pixelReadbackBytes", qint64(after.pixelReadbackBytes - before.pixelReadbackBytes)},
        {"submissions", qint64(after.submissions - before.submissions)},
        {"computeDispatches", qint64(after.computeDispatches - before.computeDispatches)}
    };
    if (!completions.isEmpty() && completions.front().gpuComputeNanoseconds()) {
        quint64 total = 0;
        for (const auto &completion : completions) {
            const auto time = completion.gpuComputeNanoseconds();
            if (!time) throw std::runtime_error("Missing GPU compute duration");
            total += *time;
        }
        result.measurement.insert(QStringLiteral("gpuComputeMs"), double(total) / 1e6);
    }
    result.measurement.insert(QStringLiteral("timingReadbackBytes"), qint64(after.timingReadbackBytes - before.timingReadbackBytes));
    return result;
}

QJsonObject summarize(const char *mode, const QJsonArray &samples)
{
    QJsonObject result {{"mode", mode}, {"samples", samples}};
    QStringList metrics {QStringLiteral("wallMs"), QStringLiteral("cpuMs"), QStringLiteral("enqueueMs")};
    if (!samples.isEmpty() && samples.first().toObject().contains(QStringLiteral("gpuComputeMs"))) metrics << QStringLiteral("gpuComputeMs");
    for (const QString &metric : metrics) {
        QVector<double> values;
        for (const auto &sample : samples) values.push_back(sample.toObject()[metric].toDouble());
        std::sort(values.begin(), values.end());
        const qsizetype middle = values.size() / 2;
        const double median = values.size() % 2 ? values[middle] : (values[middle - 1] + values[middle]) / 2;
        result.insert(metric, QJsonObject {{"median", median}, {"p95", values[qsizetype(std::ceil(values.size() * 0.95)) - 1]}});
    }
    return result;
}
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Compare individual and batched GPU raster submissions"));
    parser.addHelpOption();
    QCommandLineOption sampleOption(QStringLiteral("samples"), QStringLiteral("Samples per mode after two warmups"),
                                   QStringLiteral("count"), QStringLiteral("15"));
    parser.addOption(sampleOption);
    QCommandLineOption timingOption(QStringLiteral("gpu-timing"), QStringLiteral("Measure GPU compute passes in a separate profiling run"));
    parser.addOption(timingOption);
    parser.process(application);
    bool valid = false;
    const int sampleCount = parser.value(sampleOption).toInt(&valid);
    if (!valid || sampleCount < 1 || sampleCount > 1000) {
        std::fputs("--samples must be between 1 and 1000\n", stderr);
        return 2;
    }
    try {
        KisGpuTestDevice gpu(0, parser.isSet(timingOption));
        QJsonArray reports;
        for (const auto &workload : workloads()) {
            KisGpuTileStore store(gpu.owner, BudgetBytes);
            const auto base = workload.layerCount
                ? store.paint(store.emptyVersion(), QVector<KisGpuTileStore::PaintCommand>())
                : store.fill(store.emptyVersion(), workload.bounds, BaseColor);
            if (base.error != KisGpuTileStore::Error::None) throw std::runtime_error("Cannot initialize benchmark tiles");
            wait(gpu, store, {base.completion});
            QVector<KisGpuTileStore::Layer> layers;
            const QRect layerBounds = workload.layerBounds;
            for (int i = 0; i < workload.layerCount; ++i) {
                const auto layer = store.fill(store.emptyVersion(), layerBounds, layerColor(i));
                if (layer.error != KisGpuTileStore::Error::None) throw std::runtime_error("Cannot initialize benchmark layers");
                wait(gpu, store, {layer.completion});
                layers.push_back({layer.version});
            }
            QByteArray digest;
            {
                const auto individual = execute(gpu, store, base.version, workload, layers, false);
                const auto batch = execute(gpu, store, base.version, workload, layers, true);
                const QByteArray image = gpu.read(batch.version, workload.bounds);
                if (image.size() != workload.bounds.width() * workload.bounds.height() * 4
                    || image != gpu.read(individual.version, workload.bounds)) {
                    throw std::runtime_error("Individual and batched images differ");
                }
                digest = QCryptographicHash::hash(image, QCryptographicHash::Sha256).toHex();
            }
            QJsonArray individualSamples, batchSamples;
            for (int i = -2; i < sampleCount; ++i) {
                // Alternate which mode runs first to balance order and warmup effects.
                for (int mode = 0; mode < 2; ++mode) {
                    const bool batched = (i + mode) % 2 == 0;
                    auto run = execute(gpu, store, base.version, workload, layers, batched);
                    if (i >= 0) {
                        run.measurement.insert(QStringLiteral("iteration"), i);
                        (batched ? batchSamples : individualSamples).append(run.measurement);
                    }
                }
            }
            reports.append(QJsonObject {
                {"workload", workload.name}, {"width", workload.bounds.width()}, {"height", workload.bounds.height()},
                {"commands", layers.isEmpty() ? workload.commands.size() : layers.size()},
                {"baseTiles", base.version.tileCount()}, {"layers", layers.size()},
                {"layerBounds", workload.layerCount ? QJsonArray{layerBounds.x(), layerBounds.y(), layerBounds.width(), layerBounds.height()} : QJsonArray{}},
                {"imageSha256", QString::fromLatin1(digest)},
                {"modes", QJsonArray {summarize("individual", individualSamples), summarize("batched", batchSamples)}}
            });
        }
        if (gpu.owner.errorCount() != 0) throw std::runtime_error("Uncaptured GPU validation errors");
        QFile executable(QCoreApplication::applicationFilePath());
        if (!executable.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot identify benchmark executable");
        const QByteArray executableDigest = QCryptographicHash::hash(executable.readAll(), QCryptographicHash::Sha256).toHex();
        const QJsonDocument report(QJsonObject {
            {"schema", 2}, {"adapter", gpu.name}, {"wgpuNativeVersion", QString::number(wgpuGetVersion(), 16)},
            {"gpuTimingEnabled", parser.isSet(timingOption)},
            {"executableSha256", QString::fromLatin1(executableDigest)},
            {"validationEnabled", true}, {"budgetBytes", qint64(BudgetBytes)},
            {"samplesPerMode", sampleCount}, {"warmupsPerMode", 2}, {"workloads", reports}
        });
        const QByteArray json = report.toJson();
        if (std::fwrite(json.constData(), 1, size_t(json.size()), stdout) != size_t(json.size())
            || std::fflush(stdout) != 0) return 1;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
    return 0;
}

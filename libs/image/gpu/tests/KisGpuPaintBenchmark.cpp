/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuTestDevice.h"

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
constexpr quint64 BudgetBytes = 256 * 1024 * 1024;

struct Workload {
    QString name;
    QRect bounds;
    QVector<KisGpuTileStore::PaintCommand> commands;
};

QVector<Workload> workloads()
{
    using Op = KisGpuTileStore::CompositeOp;
    QVector<Workload> result {
        {QStringLiteral("small"), QRect(0, 0, 128, 128), {{QRect(5, 7, 8, 8), 0x800000FF}}},
        {QStringLiteral("overlapping"), QRect(0, 0, 128, 128), {}},
        {QStringLiteral("scattered"), QRect(0, 0, 2048, 1024), {}}
    };
    for (int i = 0; i < 128; ++i) {
        const quint32 color = 0x80000000 | (quint32(i * 123457) & 0x00FFFFFF);
        const Op operation = i % 7 == 0 ? Op::Erase : Op::Over;
        result[1].commands.push_back({QRect(5 + i * 7 % 24, 7 + i * 11 % 24, 32, 32), color, operation, 192, 128});
        result[2].commands.push_back({QRect(i % 16 * 128 + 5, i / 16 * 128 + 7, 64, 64), color, operation, 192, 128});
    }
    return result;
}

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
            const Workload &workload, bool batched)
{
    Run result;
    result.version = base;
    QVector<KisGpuTileStore::Completion> completions;
    completions.reserve(batched ? 1 : workload.commands.size());
    const auto before = store.statistics();
    const std::clock_t cpuStart = std::clock();
    QElapsedTimer elapsed;
    elapsed.start();
    auto retain = [&](const KisGpuTileStore::Edit &edit) {
        if (edit.error != KisGpuTileStore::Error::None) throw std::runtime_error("GPU edit was rejected");
        result.version = edit.version;
        completions.push_back(edit.completion);
    };
    if (batched) {
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
        {"submissions", qint64(after.submissions - before.submissions)},
        {"computeDispatches", qint64(after.computeDispatches - before.computeDispatches)}
    };
    return result;
}

QJsonObject summarize(const char *mode, const QJsonArray &samples)
{
    QJsonObject result {{"mode", mode}, {"samples", samples}};
    for (const QString &metric : {QStringLiteral("wallMs"), QStringLiteral("cpuMs"), QStringLiteral("enqueueMs")}) {
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
    parser.process(application);
    bool valid = false;
    const int sampleCount = parser.value(sampleOption).toInt(&valid);
    if (!valid || sampleCount < 1 || sampleCount > 1000) {
        std::fputs("--samples must be between 1 and 1000\n", stderr);
        return 2;
    }
    try {
        KisGpuTestDevice gpu;
        QJsonArray reports;
        for (const auto &workload : workloads()) {
            KisGpuTileStore store(gpu.device, BudgetBytes);
            const auto base = store.fill(store.emptyVersion(), workload.bounds, 0xC0102030);
            if (base.error != KisGpuTileStore::Error::None) throw std::runtime_error("Cannot initialize benchmark tiles");
            wait(gpu, store, {base.completion});
            QByteArray digest;
            {
                const auto individual = execute(gpu, store, base.version, workload, false);
                const auto batch = execute(gpu, store, base.version, workload, true);
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
                    auto run = execute(gpu, store, base.version, workload, batched);
                    if (i >= 0) {
                        run.measurement.insert(QStringLiteral("iteration"), i);
                        (batched ? batchSamples : individualSamples).append(run.measurement);
                    }
                }
            }
            reports.append(QJsonObject {
                {"workload", workload.name}, {"width", workload.bounds.width()}, {"height", workload.bounds.height()},
                {"commands", workload.commands.size()}, {"baseTiles", base.version.tileCount()},
                {"imageSha256", QString::fromLatin1(digest)},
                {"modes", QJsonArray {summarize("individual", individualSamples), summarize("batched", batchSamples)}}
            });
        }
        if (gpu.errors.load() != 0) throw std::runtime_error("Uncaptured GPU validation errors");
        QFile executable(QCoreApplication::applicationFilePath());
        if (!executable.open(QIODevice::ReadOnly)) throw std::runtime_error("Cannot identify benchmark executable");
        const QByteArray executableDigest = QCryptographicHash::hash(executable.readAll(), QCryptographicHash::Sha256).toHex();
        const QJsonDocument report(QJsonObject {
            {"schema", 1}, {"adapter", gpu.name}, {"wgpuNativeVersion", QString::number(wgpuGetVersion(), 16)},
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

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCanvasSession.h"
#include "KisGpuWindowTestEventLoop.h"
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWindow>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <stdexcept>

namespace {
using Document = KisGpuLayerDocument;
using Input = KisGpuStrokeInput;
using Store = KisGpuTileStore;
void require(bool valid, const char *message)
{
    if (!valid) throw std::runtime_error(message);
}
QJsonObject distribution(QVector<double> values)
{
    std::sort(values.begin(), values.end());
    return {{"count", values.size()}, {"median", values.at(values.size() / 2)},
        {"p95", values.at(qsizetype(std::ceil(values.size() * .95)) - 1)}};
}
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"output", "Write measurement results as JSON.", "path"});
    parser.addOption({"samples", "Number of measured replacements after two warmups.", "count", "30"});
    parser.process(app);
    bool validCount = false;
    const int sampleCount = parser.value("samples").toInt(&validCount);
    if (!validCount || sampleCount < 3 || sampleCount > 10000) {
        std::fprintf(stderr, "--samples must be between 3 and 10000\n");
        return 2;
    }
    try {
        KisGpuDocumentDevice documents;
        KisGpuApplicationState activity(documents);
        activity.applicationStateChanged(Qt::ApplicationActive);
        const auto id = documents.createDocument({});
        auto model = documents.document(id);
        require(bool(model), "Cannot create product document");
        require(model->addLayer(QStringLiteral("Paint")).result == Document::Result::Accepted, "Cannot add layer");
        QWindow window;
        Krita::Canvas::SurfaceRenderer::prepareWindow(window);
        window.resize(64, 64);
        const QRect bounds(0, 0, 64, 64);
        KisGpuCanvasSession canvas(documents, activity, id, window, bounds);
        Input::Configuration configuration;
        configuration.clip = bounds;
        configuration.brush.diameter = {8, 8};
        configuration.brush.spacing = .25;
        configuration.brush.rgba = 0xFF123456;
        const auto edit = canvas.begin(1, configuration);
        require(edit.result == Input::Result::Accepted, "Cannot begin product input");
        QVector<double> wall, cpu;
        QJsonArray samples;
        for (int iteration = 0; iteration < sampleCount + 2; ++iteration) {
            const auto before = model->currentEditing().session->preview();
            QVector<KisGpuBrushStroke::Sample> shape;
            const double radius = 18 + iteration % 5;
            for (int i = 0; i < 256; ++i) {
                const double angle = 2 * M_PI * i / 256;
                shape.append({{32 + radius * std::cos(angle), 32 + 12 * std::sin(angle)}, .65});
            }
            shape.append(shape.first());
            QElapsedTimer elapsed;
            elapsed.start();
            const auto cpuStart = std::clock();
            require(canvas.replace(edit.token, shape) == Input::Result::Accepted, "Product replacement refused");
            require(waitForGpuWindow([&] {
                require(model->state() != Document::State::Failed, "Product document failed");
                return !(model->currentEditing().session->preview() == before);
            }), "Product publication timed out");
            const double milliseconds = elapsed.nsecsElapsed() / 1e6;
            const double cpuMilliseconds = double(std::clock() - cpuStart) * 1000 / CLOCKS_PER_SEC;
            if (iteration >= 2) {
                wall.append(milliseconds);
                cpu.append(cpuMilliseconds);
                samples.append(QJsonObject{{"input_to_preview_ms", milliseconds}, {"cpu_ms", cpuMilliseconds}});
            }
        }
        require(canvas.commit(edit.token) == Input::Result::Accepted, "Commit refused");
        require(waitForGpuWindow([&] { return canvas.inputState() == Input::State::Finished; }), "Commit timed out");
        const auto store = documents.store(id);
        const auto statistics = store->statistics();
        require(statistics.pixelUploadBytes == 0 && statistics.pixelReadbackBytes == 0, "Unexpected ordinary pixel transfer");
        require(documents.device()->errorCount() == 0, "GPU validation failure");
        const auto image = store->readback(model->layers().first().paint.pixels, bounds);
        wgpuDevicePoll(documents.device()->device(), true, nullptr);
        store->poll();
        require(image.completion.status() == Store::Status::Succeeded, "Verification readback failed");
        const auto imageHash = QString::fromLatin1(QCryptographicHash::hash(image.bytes(), QCryptographicHash::Sha256).toHex());
        const QJsonObject report{{"source", "product KisGpuCanvasSession, automatic GUI timers"},
            {"window", "hidden; no presentation"}, {"shape_samples", 257},
            {"input_to_preview_ms", distribution(wall)}, {"process_cpu_ms_including_observer", distribution(cpu)},
            {"image_sha256", imageHash},
            {"verification_readback_bytes", qint64(store->statistics().pixelReadbackBytes-statistics.pixelReadbackBytes)},
            {"pixel_upload_bytes", qint64(statistics.pixelUploadBytes)},
            {"pixel_readback_bytes", qint64(statistics.pixelReadbackBytes)}, {"samples", samples}};
        const auto json = QJsonDocument(report).toJson();
        if (parser.isSet("output")) {
            QFile file(parser.value("output"));
            require(file.open(QIODevice::WriteOnly) && file.write(json) == json.size(), "Cannot write result");
        }
        std::fwrite(json.constData(), 1, size_t(json.size()), stdout);
        return 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }
}

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuCanvasSession.h"
#include "KisGpuWindowTestEventLoop.h"
#include "KisGpuBrushInputWorkloads.h"
#include <QCommandLineParser>
#include <QCryptographicHash>
#include <QElapsedTimer>
#include <QFile>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSysInfo>
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
using Surface = Krita::Canvas::SurfaceRenderer;
void require(bool valid, const char *message)
{
    if (!valid) throw std::runtime_error(message);
}
QJsonObject distribution(QVector<double> values)
{
    std::sort(values.begin(), values.end());
    const auto middle = values.size() / 2;
    return {{"count", values.size()},
        {"median", values.size() % 2 ? values[middle] : (values[middle - 1] + values[middle]) / 2},
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
    parser.addOption({"show-window", "Measure the exposed product canvas through native presentation."});
    parser.addOption({"workload", "Input workload: shape, small or layer-projection.", "name", "shape"});
    parser.addOption({"mode", "Input operation: append or replace (shape requires replace).", "name", "replace"});
    parser.process(app);
    bool validCount = false;
    const int sampleCount = parser.value("samples").toInt(&validCount);
    if (!validCount || sampleCount < 3 || sampleCount > 10000) {
        std::fprintf(stderr, "--samples must be between 3 and 10000\n");
        return 2;
    }
    const auto workloadName = parser.value("workload"), mode = parser.value("mode");
    const bool shapeWorkload = workloadName == QStringLiteral("shape");
    const bool high = workloadName == QStringLiteral("layer-projection");
    const bool replace = mode == QStringLiteral("replace");
    const bool display = parser.isSet("show-window");
    if ((!shapeWorkload && !high && workloadName != QStringLiteral("small"))
        || (!replace && mode != QStringLiteral("append")) || (shapeWorkload && !replace)) {
        std::fprintf(stderr, "Invalid --workload or --mode combination\n");
        return 2;
    }
    try {
        if (display) Surface::presentationClockNanoseconds();
        KisGpuDocumentDevice documents;
        KisGpuApplicationState activity(documents);
        activity.applicationStateChanged(Qt::ApplicationActive);
        KisGpuDocumentDevice::Limits limits;
        limits.documentBytes = high ? 384 * 1024 * 1024 : 64 * 1024 * 1024;
        limits.retainedEdits = 8;
        const auto id = documents.createDocument(limits);
        auto model = documents.document(id);
        require(bool(model), "Cannot create product document");
        const KisGpuBrushInputWorkload workload(high);
        const QRect bounds = shapeWorkload ? QRect(0, 0, 64, 64) : workload.bounds;
        Document::LayerId target = 0;
        for (int i = 0; i < (shapeWorkload ? 1 : workload.layerCount); ++i) {
            const auto added = model->addLayer(QStringLiteral("Paint %1").arg(i));
            require(added.result == Document::Result::Accepted, "Cannot add layer");
            target = added.id;
            if (shapeWorkload) continue;
            const auto seed = model->begin(target);
            require(seed.result == Document::Result::Accepted, "Cannot begin seed");
            require(seed.session->append(seed.token, {workload.seed(i)}, workload.occupied)
                == KisGpuEditSession::Result::Accepted, "Cannot seed layer");
            require(model->commit(seed.token) == Document::Result::Accepted, "Cannot commit seed");
            require(waitForGpuWindow([&] {
                model->poll();
                require(model->state() != Document::State::Failed, "Seed failed");
                return model->state() == Document::State::Idle;
            }), "Seed timed out");
        }
        QWindow window;
        Surface::prepareWindow(window);
        window.resize(shapeWorkload ? 64 : 640, shapeWorkload ? 64 : 640);
        KisGpuCanvasSession canvas(documents, activity, id, window, bounds);
        Input::Configuration configuration;
        configuration.clip = bounds;
        if (shapeWorkload) {
            configuration.brush.diameter = {8, 8};
            configuration.brush.spacing = .25;
            configuration.brush.rgba = 0xFF123456;
        } else configuration.brush = workload.settings();
        if (display) {
            Krita::Canvas::GpuRenderer::View view;
            view.canvasToTarget.scale(window.width() * window.devicePixelRatio() / bounds.width(),
                                     window.height() * window.devicePixelRatio() / bounds.height());
            require(canvas.setView(view), "Cannot set product view");
            window.show();
            require(waitForGpuWindow([&] {
                return window.isExposed() && canvas.lastFrame().accepted()
                    && canvas.lastFrame().rendering.status() == Store::Status::Succeeded
                    && !canvas.statistics().rendering.pendingFrames && !canvas.statistics().pendingPresentations;
            }), "Initial product display timed out");
        }
        const auto edit = canvas.begin(target, configuration);
        require(edit.result == Input::Result::Accepted, "Cannot begin product input");
        QVector<double> wall, cpu, rendered, presented, nativePresent;
        bool displayedEverySample = true;
        quint64 maximumReserved = 0, imageAllocationBytes = 0;
        QJsonArray samples;
        for (int iteration = 0; iteration < sampleCount + 2; ++iteration) {
            const auto before = model->currentEditing().session->preview();
            QVector<KisGpuBrushStroke::Sample> shape;
            if (shapeWorkload) {
                const double radius = 18 + iteration % 5;
                for (int i = 0; i < 256; ++i) {
                    const double angle = 2 * M_PI * i / 256;
                    shape.append({{32 + radius * std::cos(angle), 32 + 12 * std::sin(angle)}, .65});
                }
                shape.append(shape.first());
            } else shape = workload.samples(iteration, replace);
            const auto beforeDisplay = canvas.statistics();
            const auto beforeFrame = canvas.lastFrame().rendering.sequence();
            const auto beforeStore = documents.store(id)->statistics();
            const auto inputTime = Surface::presentationClockNanoseconds();
            QElapsedTimer elapsed;
            elapsed.start();
            const auto cpuStart = std::clock();
            require((replace ? canvas.replace(edit.token, shape) : canvas.append(edit.token, shape))
                == Input::Result::Accepted, "Product input refused");
            require(waitForGpuWindow([&] {
                require(model->state() != Document::State::Failed, "Product document failed");
                return !(model->currentEditing().session->preview() == before);
            }), "Product publication timed out");
            const double milliseconds = elapsed.nsecsElapsed() / 1e6;
            double renderingMilliseconds = 0;
            QJsonValue presentationMilliseconds;
            QString presentationStatus = QStringLiteral("hidden");
            bool displayed = false;
            if (display) {
                require(waitForGpuWindow([&] {
                    require(canvas.lastError().isEmpty(), "Product display failed");
                    const auto frame = canvas.lastFrame();
                    return frame.rendering.sequence() > beforeFrame
                        && frame.rendering.status() == Store::Status::Succeeded;
                }, 10000), "Product rendering timed out");
                renderingMilliseconds = elapsed.nsecsElapsed() / 1e6;
                const auto frame = canvas.lastFrame();
                const bool presentationFinished = waitForGpuWindow([&] {
                    return frame.presentation.status() != Surface::Presentation::Status::Pending
                        && !canvas.statistics().rendering.pendingFrames && !canvas.statistics().pendingPresentations;
                }, 10000);
                if (!presentationFinished) {
                    const auto pending = canvas.statistics();
                    std::fprintf(stderr, "Presentation wait: exposed=%d, rendering=%u, presentation=%u, status=%d\n",
                        int(window.isExposed()), pending.rendering.pendingFrames, pending.pendingPresentations,
                        int(frame.presentation.status()));
                }
                require(presentationFinished, "Product presentation timed out");
                const auto status = frame.presentation.status();
                displayed = status == Surface::Presentation::Status::Presented;
                presentationStatus = displayed ? QStringLiteral("presented")
                    : status == Surface::Presentation::Status::Skipped ? QStringLiteral("skipped")
                    : status == Surface::Presentation::Status::Abandoned ? QStringLiteral("abandoned")
                    : QStringLiteral("unavailable");
                if (displayed) {
                    const auto time = frame.presentation.hostTimeNanoseconds();
                    const auto observed = Surface::presentationClockNanoseconds();
                    const bool validTime = inputTime && time && observed && *time >= *inputTime && *time <= *observed;
                    if (!validTime)
                        std::fprintf(stderr, "Presentation clocks: input=%llu, display=%llu, observed=%llu\n",
                            static_cast<unsigned long long>(inputTime.value_or(0)),
                            static_cast<unsigned long long>(time.value_or(0)),
                            static_cast<unsigned long long>(observed.value_or(0)));
                    require(validTime, "Product presentation timestamp outside input/observation interval");
                    presentationMilliseconds = double(*time - *inputTime) / 1e6;
                }
            }
            const double cpuMilliseconds = double(std::clock() - cpuStart) * 1000 / CLOCKS_PER_SEC;
            maximumReserved = std::max(maximumReserved, documents.device()->memoryStatistics().reservedBytes);
            if (iteration >= 2) {
                wall.append(milliseconds);
                cpu.append(cpuMilliseconds);
                QJsonObject sample{{"input_to_preview_ms", milliseconds}, {"cpu_ms", cpuMilliseconds}};
                if (display) {
                    displayedEverySample &= displayed;
                    rendered.append(renderingMilliseconds);
                    if (displayed) presented.append(presentationMilliseconds.toDouble());
                    const double presentMs = (canvas.statistics().nativePresentCallNanoseconds
                        - beforeDisplay.nativePresentCallNanoseconds) / 1e6;
                    nativePresent.append(presentMs);
                    sample.insert("input_to_render_completion_ms", renderingMilliseconds);
                    sample.insert("input_to_presentation_ms", presentationMilliseconds);
                    sample.insert("presentation_status", presentationStatus);
                    sample.insert("native_present_call_ms", presentMs);
                    const auto allocated = documents.store(id)->statistics().imageTextureAllocationBytes
                        - beforeStore.imageTextureAllocationBytes;
                    imageAllocationBytes += allocated;
                    sample.insert("image_allocation_bytes", qint64(allocated));
                }
                samples.append(sample);
            }
        }
        require(canvas.commit(edit.token) == Input::Result::Accepted, "Commit refused");
        require(waitForGpuWindow([&] { return canvas.inputState() == Input::State::Finished; }), "Commit timed out");
        const auto store = documents.store(id);
        const auto statistics = store->statistics();
        require(statistics.pixelUploadBytes == 0 && statistics.pixelReadbackBytes == 0, "Unexpected ordinary pixel transfer");
        require(documents.device()->errorCount() == 0, "GPU validation failure");
        const auto image = store->readback(model->layers().last().paint.pixels, bounds);
        wgpuDevicePoll(documents.device()->device(), true, nullptr);
        store->poll();
        require(image.completion.status() == Store::Status::Succeeded, "Verification readback failed");
        const auto imageHash = QString::fromLatin1(QCryptographicHash::hash(image.bytes(), QCryptographicHash::Sha256).toHex());
        QJsonObject report{{"source", "product KisGpuCanvasSession, automatic GUI timers"},
            {"cpu_build_architecture", QSysInfo::buildCpuArchitecture()},
            {"cpu_runtime_architecture", QSysInfo::currentCpuArchitecture()},
            {"kernel_type", QSysInfo::kernelType()}, {"adapter", documents.device()->adapterName()},
            {"wgpu_native_version", QString::number(wgpuGetVersion(), 16)},
            {"window", display ? "exposed; automatic product presentation" : "hidden; no presentation"},
            {"workload", workloadName}, {"mode", mode}, {"layer_count", model->layers().size()},
            {"canvas_size", QJsonArray{bounds.width(), bounds.height()}},
            {"shape_samples", shapeWorkload ? 257 : (replace ? 9 : 1)},
            {"maximum_observed_device_reserved_bytes", qint64(maximumReserved)},
            {"input_to_preview_ms", distribution(wall)}, {"process_cpu_ms_including_observer", distribution(cpu)},
            {"image_sha256", imageHash},
            {"verification_readback_bytes", qint64(store->statistics().pixelReadbackBytes-statistics.pixelReadbackBytes)},
            {"pixel_upload_bytes", qint64(statistics.pixelUploadBytes)},
            {"pixel_readback_bytes", qint64(statistics.pixelReadbackBytes)}, {"samples", samples}};
        QFile executable(QCoreApplication::applicationFilePath());
        require(executable.open(QIODevice::ReadOnly), "Cannot identify benchmark executable");
        QCryptographicHash executableHash(QCryptographicHash::Sha256);
        require(executableHash.addData(&executable), "Cannot hash benchmark executable");
        report.insert("executable_sha256", QString::fromLatin1(executableHash.result().toHex()));
        if (display) {
            report.insert("input_to_render_completion_ms", distribution(rendered));
            report.insert("input_to_presentation_ms", presented.isEmpty() ? QJsonValue{} : QJsonValue(distribution(presented)));
            report.insert("native_present_call_ms", distribution(nativePresent));
            report.insert("displayed_every_sample", displayedEverySample);
            report.insert("image_allocation_bytes", qint64(imageAllocationBytes));
        }
        const auto json = QJsonDocument(report).toJson();
        if (parser.isSet("output")) {
            QFile file(parser.value("output"));
            require(file.open(QIODevice::WriteOnly) && file.write(json) == json.size(), "Cannot write result");
        }
        std::fwrite(json.constData(), 1, size_t(json.size()), stdout);
        return display && !displayedEverySample ? 3 : 0;
    } catch (const std::exception &error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 2;
    }
}

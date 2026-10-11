/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuStrokeInput.h"
#include "KisGpuBrushStroke.h"
#include "KisGpuTestDevice.h"
#include <QtTest>
#include <limits>

namespace {
using Input = KisGpuStrokeInput;
using Brush = KisGpuBrushStroke;
using Document = KisGpuLayerDocument;
using Store = KisGpuTileStore;
using Session = KisGpuEditSession;
const QRect Bounds(-64, -64, 128, 128);
const QVector<Brush::Sample> Path{{{-31.25, 2.5}, .3}, {{32.75, 17.5}, .9}, {{17.25, -29.5}, .5}};
bool drain(Input &input, bool finish = false)
{
    QElapsedTimer timer; timer.start();
    do { input.poll(); QTest::qWait(1); }
    while ((input.pendingSamples() || (finish && input.state() != Input::State::Finished)) && timer.elapsed() < 5000);
    return !input.pendingSamples() && (!finish || input.state() == Input::State::Finished);
}
Input::Configuration configuration(bool texture = false, bool mixed = false)
{
    Input::Configuration config;
    config.clip = Bounds;
    config.brush.diameter = {10, 7}; config.brush.spacing = .3; config.brush.rgba = 0xBE746852;
    if (mixed) config.brush.smudgeRate = 213;
    if (texture) config.texture = std::make_shared<const Session::ReplayTexture>(
        Session::ReplayTexture{{3, 2}, QByteArray::fromHex("0080ff0140fe"), {-2, 3}});
    Store::DabCommand mask;
    mask.center = {2, 0}; mask.diameter = {88, 76}; mask.rgba = 0xCC000000;
    config.selection = std::make_shared<const Session::ReplaySelection>(Session::ReplaySelection{{mask}, Bounds});
    return config;
}
QByteArray directImage(KisGpuTestDevice &gpu, Store &store, const Input::Configuration &config,
                      const QVector<Brush::Sample> &samples)
{
    Session session(store, 0);
    auto settings = config.brush;
    if (config.texture) settings.texture = session.createTexture(config.texture->size, config.texture->alpha, config.texture->origin).texture;
    std::optional<Session::Selection> selection;
    if (config.selection) selection = session.createSelection(config.selection->commands, config.selection->clip).selection;
    const auto token = session.begin();
    Brush brush(session, token, settings, config.clip);
    for (const auto &sample : samples) {
        bool complete = false;
        for (int attempt = 0; !complete && attempt < 100; ++attempt) {
            store.poll(); session.poll();
            const auto update = brush.appendPartial(sample, selection);
            if (update.result == Brush::Result::Accepted) complete = update.sampleComplete;
            else if (update.result != Brush::Result::EditRejected) return {};
            wgpuDevicePoll(gpu.device, true, nullptr); session.poll();
        }
        if (!complete) return {};
    }
    session.commit(token);
    QElapsedTimer timer; timer.start();
    while (session.state() == Session::State::Committing && timer.elapsed() < 5000) {
        session.poll(); QTest::qWait(1);
    }
    return gpu.read(session.head(), Bounds);
}
}

/** Tool-owned input remains ordered, bounded and cancellable across GPU replacement. */
class KisGpuStrokeInputTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void refusedGpuInputStaysQueuedUntilCapacityReturns() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto document = std::make_shared<Document>(store, 16);
        document->addLayer(QStringLiteral("Paint"));
        Input input(document);
        auto config = configuration(); config.selection.reset();
        const auto started = input.begin(1, config);
        QCOMPARE(started.result, Input::Result::Accepted);
        QCOMPARE(input.append(started.token, Path), Input::Result::Accepted);
        QCOMPARE(input.poll(), Input::Result::Accepted);
        wgpuDevicePoll(gpu.device, true, nullptr); document->poll();
        const auto pending = input.pendingSamples();
        const auto working = document->recovery().working->data.working->size();
        const auto before = store->statistics();
        auto held = gpu.owner.reserveMemory(gpu.owner.availableMemory());
        QVERIFY(held);
        QCOMPARE(input.commit(started.token), Input::Result::Accepted);
        QCOMPARE(input.poll(), Input::Result::GpuRejected);
        QCOMPARE(input.lastGpuError(), Store::Error::BudgetExceeded);
        QCOMPARE(input.pendingSamples(), pending);
        QCOMPARE(document->recovery().working->data.working->size(), working);
        QCOMPARE(store->statistics().submissions, before.submissions);
        held = {};
        QVERIFY(drain(input, true));
        QCOMPARE(gpu.read(document->layers().first().paint.pixels, Bounds), directImage(gpu, *store, config, Path));
        QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
    }
    void replacementSupersedesQueuedInputAndCommitAdoptsPreview() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes, 1);
        auto document = std::make_shared<Document>(store, 16);
        document->addLayer(QStringLiteral("Paint"));
        Input input(document, 4);
        auto config = configuration(false, true);
        const auto started = input.begin(1, config);
        QCOMPARE(started.result, Input::Result::Accepted);
        const auto before = store->statistics();
        QCOMPARE(input.append(started.token, QVector<Brush::Sample>(5, Path.first())), Input::Result::QueueFull);
        QCOMPARE(input.append(started.token, {{{0, 0}, std::numeric_limits<qreal>::quiet_NaN()}}), Input::Result::InvalidInput);
        QCOMPARE(store->statistics().submissions, before.submissions);
        QCOMPARE(input.append(started.token, {{{-50, -50}, 1}, {{50, -50}, 1}, {{50, 50}, 1}}), Input::Result::Accepted);
        input.poll();
        QCOMPARE(input.replace(started.token, Path), Input::Result::Accepted);
        QVERIFY(drain(input));
        QCOMPARE(input.state(), Input::State::Drawing);
        wgpuDevicePoll(gpu.device, true, nullptr); document->poll();
        const auto preview = document->currentEditing().session->preview();
        const auto pixels = gpu.read(preview, Bounds);
        QCOMPARE(pixels, directImage(gpu, *store, config, Path));
        const auto ready = store->statistics();
        QCOMPARE(input.commit(started.token), Input::Result::Accepted);
        QCOMPARE(input.append(started.token, Path), Input::Result::Stale);
        QCOMPARE(input.replace(started.token, Path), Input::Result::Stale);
        QVERIFY(drain(input, true));
        QCOMPARE(document->layers().first().paint.pixels, preview);
        QCOMPARE(store->statistics().submissions, ready.submissions);
        QCOMPARE(input.cancel(started.token), Input::Result::Stale);
        QVERIFY(document->undo());
        QVERIFY(document->redo());
        QCOMPARE(gpu.read(document->layers().first().paint.pixels, Bounds), pixels);
        QCOMPARE(store->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(store->statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void gpuReplacementRetainsQueuedInput_data() {
        QTest::addColumn<bool>("texture"); QTest::addColumn<bool>("mixed");
        QTest::newRow("solid-selected") << false << false;
        QTest::newRow("textured-selected") << true << false;
        QTest::newRow("mixed-selected") << false << true;
    }
    void gpuReplacementRetainsQueuedInput() {
        QFETCH(bool, texture); QFETCH(bool, mixed);
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto document = std::make_shared<Document>(store, 16);
        document->addLayer(QStringLiteral("Paint"));
        Input input(document);
        const auto config = configuration(texture, mixed);
        const auto started = input.begin(1, config);
        QCOMPARE(started.result, Input::Result::Accepted);
        QCOMPARE(input.append(started.token, Path), Input::Result::Accepted);
        input.poll(); input.poll();
        QVERIFY(input.pendingSamples());
        input.releaseGpu();
        QCOMPARE(document->suspend(), Document::Result::Accepted);
        gpu.owner.destroy();
        KisGpuTestDevice replacement;
        auto nextStore = std::make_shared<Store>(replacement.owner, 128 * Store::TileBytes);
        QCOMPARE(document->resume(nextStore), Document::Result::Accepted);
        QCOMPARE(input.commit(started.token), Input::Result::Accepted);
        QVERIFY(drain(input, true));
        QCOMPARE(replacement.read(document->layers().first().paint.pixels, Bounds), directImage(replacement, *nextStore, config, Path));
        QCOMPARE(nextStore->statistics().pixelReadbackBytes, quint64(0));
        QCOMPARE(replacement.owner.errorCount(), 0);
    }
    void cancellationAndDestructionDiscardOnlyOwnedWorkingInput_data() {
        QTest::addColumn<int>("phase"); QTest::addColumn<bool>("destroy");
        for (int phase = 0; phase < 3; ++phase) for (bool destroy : {false, true})
            QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(phase).arg(destroy))) << phase << destroy;
    }
    void cancellationAndDestructionDiscardOnlyOwnedWorkingInput() {
        QFETCH(int, phase); QFETCH(bool, destroy);
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto document = std::make_shared<Document>(store, 16);
        document->addLayer(QStringLiteral("Paint"));
        auto input = std::make_unique<Input>(document);
        const auto started = input->begin(1, configuration(false, true));
        QCOMPARE(started.result, Input::Result::Accepted);
        QCOMPARE(input->append(started.token, Path), Input::Result::Accepted);
        input->poll(); input->poll();
        if (phase) {
            input->releaseGpu();
            QCOMPARE(document->suspend(), Document::Result::Accepted);
            if (phase == 2) {
                QCOMPARE(document->resume(store), Document::Result::Accepted);
                document->poll();
                QCOMPARE(document->state(), Document::State::Restoring);
            }
        }
        const auto before = store->statistics();
        if (destroy) input.reset();
        else {
            QCOMPARE(input->cancel(started.token), Input::Result::Accepted);
            QCOMPARE(input->append(started.token, Path), Input::Result::Stale);
            QCOMPARE(input->pendingSamples(), qsizetype(0));
        }
        QVERIFY(!document->currentEditLease());
        QVERIFY(!document->recovery().working);
        QCOMPARE(store->statistics().submissions, before.submissions);
        if (phase == 1) QCOMPARE(document->resume(store), Document::Result::Accepted);
        QTRY_VERIFY((document->poll(), document->state() == Document::State::Idle));
        QCOMPARE(gpu.read(document->layers().first().paint.pixels, Bounds), QByteArray(Bounds.width() * Bounds.height() * 4, 0));
        QCOMPARE(gpu.owner.errorCount(), 0);
    }
    void foreignAndReplacedOperationsAreStale() {
        KisGpuTestDevice gpu;
        auto store = std::make_shared<Store>(gpu.owner, 128 * Store::TileBytes);
        auto first = std::make_shared<Document>(store, 16), second = std::make_shared<Document>(store, 16);
        first->addLayer(QStringLiteral("First")); second->addLayer(QStringLiteral("Second"));
        Input a(first), b(second);
        auto invalid = configuration(); invalid.brush.spacing = -1;
        QCOMPARE(a.begin(1, invalid).result, Input::Result::InvalidInput);
        QVERIFY(!first->currentEditLease());
        auto oversized = configuration(true); oversized.texture = std::make_shared<const Session::ReplayTexture>(
            Session::ReplayTexture{{2, 2}, QByteArray(3, char(255)), {}});
        QCOMPARE(a.begin(1, oversized).result, Input::Result::InvalidInput);
        Input limited(first, 16, 1);
        QCOMPARE(limited.begin(1, configuration(true)).result, Input::Result::InvalidInput);
        const auto old = a.begin(1, configuration());
        const auto foreign = b.begin(1, configuration());
        QCOMPARE(old.result, Input::Result::Accepted); QCOMPARE(foreign.result, Input::Result::Accepted);
        QCOMPARE(a.append(foreign.token, Path), Input::Result::Stale);
        QCOMPARE(a.cancel(old.token), Input::Result::Accepted);
        const auto next = a.begin(1, configuration());
        QCOMPARE(next.result, Input::Result::Accepted);
        QCOMPARE(a.replace(old.token, Path), Input::Result::Stale);
        QCOMPARE(a.cancel(old.token), Input::Result::Stale);
        QCOMPARE(a.append(next.token, Path), Input::Result::Accepted);
    }
};
QTEST_GUILESS_MAIN(KisGpuStrokeInputTest)
#include "KisGpuStrokeInputTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QtTest>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "KisGpuBrushStroke.h"
#include "KisGpuEditSession.h"
#include "KisGpuLayerDocument.h"
#include "KisDabSpacing.h"
#include "KisGpuTestDevice.h"
#include <limits>
#include <cmath>
#include <stdexcept>

namespace {
using Stroke = KisGpuBrushStroke;
using Session = KisGpuEditSession;
using Store = KisGpuTileStore;
const QRect clip(-128, -128, 256, 256);
void settle(Session &session)
{
    QElapsedTimer timer;
    timer.start();
    do {
        session.poll();
        QTest::qWait(1);
    } while ((session.state() == Session::State::Committing || session.state() == Session::State::Restoring)
             && timer.elapsed() < 5000);
}
Store::DabCommand dab(const Stroke::Settings &settings, QPointF position, QSizeF diameter)
{
    Store::DabCommand command;
    command.center = position;
    command.diameter = diameter;
    command.fade = settings.fade;
    command.rgba = settings.rgba;
    command.operation = settings.operation;
    command.opacity = settings.opacity;
    command.coverage = settings.coverage;
    command.textureStrength = settings.textureStrength;
    return command;
}
}

/** Pressure/spacing input consumes the same placement fixture as the CPU paint loop. */
class KisGpuBrushStrokeTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase() { m_gpu = std::make_unique<KisGpuTestDevice>(); }
    void cleanup() {
        wgpuDevicePoll(m_gpu->device, true, nullptr);
        QCOMPARE(m_gpu->owner.errorCount(), 0);
    }
    void placementMatchesCpuContract();
    void queueRejectionPreservesInputProgress();
    void invalidInputAndGenerationLimitAreAtomic();
    void textureStrengthMatchesDirectDab();
    void mixingProgressAndReplacementPreservePickup();
    void boundedInputContinuesAfterRefusal_data();
    void boundedInputContinuesAfterRefusal();
    void selectedReplacementAndRecoveryUseIdenticalDabs_data();
    void selectedReplacementAndRecoveryUseIdenticalDabs();
    void deviceReplacementContinuesAcceptedInput_data();
    void deviceReplacementContinuesAcceptedInput();
private:
    std::unique_ptr<KisGpuTestDevice> m_gpu;
};

void KisGpuBrushStrokeTest::boundedInputContinuesAfterRefusal_data()
{
    QTest::addColumn<bool>("mixed");
    QTest::newRow("solid") << false;
    QTest::newRow("mixed") << true;
}

void KisGpuBrushStrokeTest::boundedInputContinuesAfterRefusal()
{
    QFETCH(bool, mixed);
    Store store(m_gpu->owner, 128 * Store::TileBytes, 1);
    Stroke::Settings settings;
    settings.diameter = {9, 7}; settings.spacing = .31;
    settings.rgba = 0xCC746852;
    const QVector<Stroke::Sample> samples{{{-61.25, 7.13}, .23}, {{36.8, 29.75}, .91}, {{64.3, -11.4}, .37}};
    Session generation(store, 0);
    auto token = generation.begin();
    Stroke unbounded(generation, token, settings, clip);
    QCOMPARE(unbounded.append(samples).result, Stroke::Result::Accepted);
    QVector<Store::DabCommand> expected;
    for (const auto &batch : *generation.recovery()->working) expected += batch.commands;
    generation.cancel(token);
    wgpuDevicePoll(m_gpu->device, true, nullptr); store.poll();
    if (mixed) {
        settings.smudgeRate = 213;
        std::optional<QPoint> previous;
        for (auto &command : expected) {
            const QPoint pixel(int(std::floor(command.center.x())), int(std::floor(command.center.y())));
            command.pickup = Store::DabCommand::Pickup{previous.value_or(pixel), *settings.smudgeRate};
            previous = pixel;
        }
    }
    Session actual(store, 0);
    token = actual.begin();
    Stroke stroke(actual, token, settings, clip, mixed ? 4096 : 7);
    const auto first = stroke.appendPartial({samples.first()});
    QCOMPARE(first.result, Stroke::Result::Accepted);
    QVERIFY(first.sampleComplete);
    QCOMPARE(first.completedSamples, qsizetype(1));
    QCOMPARE(stroke.appendPartial(samples.mid(1)).result, Stroke::Result::EditRejected);
    wgpuDevicePoll(m_gpu->device, true, nullptr); actual.poll();
    auto progress = stroke.progress();
    int partials = 0;
    qsizetype consumed = 1;
    for (int attempt = 0; consumed < samples.size() && attempt < 100; ++attempt) {
        Stroke resumed(actual, token, settings, clip, progress, mixed ? 4096 : 7);
        const auto update = resumed.appendPartial(samples.mid(consumed));
        QCOMPARE(update.result, Stroke::Result::Accepted);
        QVERIFY(update.completedSamples >= 0 && update.completedSamples <= samples.size() - consumed);
        QCOMPARE(update.sampleComplete, update.completedSamples == samples.size() - consumed);
        consumed += update.completedSamples;
        if (!update.sampleComplete) ++partials;
        progress = resumed.progress();
        wgpuDevicePoll(m_gpu->device, true, nullptr); actual.poll();
    }
    QCOMPARE(consumed, samples.size());
    QVERIFY(partials > 1);
    QVector<Store::DabCommand> generated;
    for (const auto &batch : *actual.recovery()->working) {
        QVERIFY(batch.commands.size() <= (mixed ? 16 : 7));
        generated += batch.commands;
    }
    QCOMPARE(generated.size(), expected.size());
    for (qsizetype i = 0; i < expected.size(); ++i) {
        QCOMPARE(generated[i].center, expected[i].center);
        QCOMPARE(generated[i].diameter, expected[i].diameter);
        if (mixed) QCOMPARE(generated[i].pickup->pixel, expected[i].pickup->pixel);
    }
    QCOMPARE(actual.commit(token), Session::Result::Accepted); settle(actual);
    Session reference(store, 0);
    const auto referenceToken = reference.begin();
    for (qsizetype i = 0; i < expected.size(); i += 16) {
        QCOMPARE(reference.append(referenceToken, expected.mid(i, 16), clip), Session::Result::Accepted);
        wgpuDevicePoll(m_gpu->device, true, nullptr); reference.poll();
    }
    QCOMPARE(reference.commit(referenceToken), Session::Result::Accepted); settle(reference);
    QCOMPARE(m_gpu->read(actual.head(), clip), m_gpu->read(reference.head(), clip));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
}

void KisGpuBrushStrokeTest::mixingProgressAndReplacementPreservePickup()
{
    const QRect bounds(-2, 0, 6, 1);
    Store store(m_gpu->owner, 64 * Store::TileBytes);
    Session session(store, 2);
    auto token = session.begin();
    Stroke::Settings solid;
    solid.diameter = {.5, .5}; solid.rgba = 0xFF0000FF;
    QCOMPARE(session.append(token, {dab(solid, {-1, 0}, solid.diameter)}, bounds), Session::Result::Accepted);
    session.commit(token); settle(session);
    token = session.begin();
    auto mixed = solid; mixed.spacing = 2; mixed.smudgeRate = 255;
    Stroke stroke(session, token, mixed, bounds);
    QCOMPARE(stroke.append({{{-1, 0}, 1}, {{0, 0}, 1}}).result, Stroke::Result::Accepted);
    const auto progress = stroke.progress();
    QCOMPARE(stroke.append({{{1, 0}, -1}}).result, Stroke::Result::InvalidInput);
    Stroke resumed(session, token, mixed, bounds, progress);
    QCOMPARE(resumed.append({{{1, 0}, 1}}).result, Stroke::Result::Accepted);
    session.commit(token); settle(session);
    QCOMPARE(m_gpu->read(session.head(), bounds), QByteArray::fromHex("00000000ff0000ffff0000ccff0000a30000000000000000"));
    const auto accepted = session.head();
    QVERIFY(session.undo());
    token = session.begin();
    Stroke replacement(session, token, mixed, bounds);
    QCOMPARE(replacement.append({{{2, 0}, 1}, {{3, 0}, 1}}).result, Stroke::Result::Accepted);
    QCOMPARE(replacement.replace({{{-1, 0}, 1}, {{0, 0}, 1}, {{1, 0}, 1}}).result, Stroke::Result::Accepted);
    session.commit(token); settle(session);
    QCOMPARE(m_gpu->read(session.head(), bounds), m_gpu->read(accepted, bounds));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
}

void KisGpuBrushStrokeTest::placementMatchesCpuContract()
{
    QFile file(QStringLiteral(SPACING_FIXTURE));
    QVERIFY(file.open(QIODevice::ReadOnly));
    const auto root = QJsonDocument::fromJson(file.readAll()).object();
    QCOMPARE(root.value("schema").toInt(), 1);
    for (const auto &value : root.value("cases").toArray()) {
        const auto data = value.toObject();
        const auto size = data.value("diameter").toArray();
        Stroke::Settings settings;
        settings.diameter = QSizeF(size[0].toDouble(), size[1].toDouble());
        settings.spacing = data.value("spacing").toDouble();
        settings.pressureSize = data.value("pressureSize").toBool();
        settings.rgba = 0x80765432;
        Store store(m_gpu->owner, 64 * Store::TileBytes);
        Session session(store, 2);
        const auto token = session.begin();
        Stroke stroke(session, token, settings, clip);
        for (const auto &entry : data.value("samples").toArray()) {
            const auto sample = entry.toArray();
            const auto update = stroke.append({{QPointF(sample[0].toDouble(), sample[1].toDouble()), sample[2].toDouble()}});
            QCOMPARE(update.result, Stroke::Result::Accepted);
        }
        session.commit(token);
        settle(session);
        QCOMPARE(session.state(), Session::State::Idle);
        QVector<Store::DabCommand> expected;
        for (const auto &entry : data.value("dabs").toArray()) {
            const auto item = entry.toArray();
            expected.push_back(dab(settings, QPointF(item[0].toDouble(), item[1].toDouble()),
                                   QSizeF(item[2].toDouble(), item[3].toDouble())));
        }
        const auto direct = store.paintDabs(store.emptyVersion(), expected, clip);
        QCOMPARE(direct.error, Store::Error::None);
        wgpuDevicePoll(m_gpu->device, true, nullptr);
        store.poll();
        QVERIFY2(m_gpu->read(session.head(), clip) == m_gpu->read(direct.version, clip), qPrintable(data.value("id").toString()));
        QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    }
}

void KisGpuBrushStrokeTest::queueRejectionPreservesInputProgress()
{
    Store store(m_gpu->owner, 64 * Store::TileBytes, 1);
    Session session(store, 2);
    const auto token = session.begin();
    Stroke::Settings settings;
    settings.diameter = QSizeF(10, 10);
    settings.rgba = 0x800000FF;
    Stroke stroke(session, token, settings, clip);
    QCOMPARE(stroke.append({{{16.25, 16.25}, 1}}).result, Stroke::Result::Accepted);
    const auto rejected = stroke.append({{{26.25, 16.25}, 1}});
    QCOMPARE(rejected.result, Stroke::Result::EditRejected);
    QCOMPARE(rejected.editResult, Session::Result::GpuRejected);
    QCOMPARE(session.lastGpuError(), Store::Error::QueueFull);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    session.poll();
    Stroke resumed(session, token, settings, clip, stroke.progress());
    QCOMPARE(resumed.append({{{26.25, 16.25}, 1}}).result, Stroke::Result::Accepted);
    session.commit(token);
    settle(session);
    QVector<Store::DabCommand> expected;
    for (qreal x = 16.25; x <= 26.25; x += 2.5) expected.push_back(dab(settings, QPointF(x, 16.25), settings.diameter));
    const auto direct = store.paintDabs(store.emptyVersion(), expected, clip);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    store.poll();
    QCOMPARE(m_gpu->read(session.head(), clip), m_gpu->read(direct.version, clip));
    const auto before = store.statistics().submissions;
    QCOMPARE(stroke.append({{{30, 30}, 1}}).editResult, Session::Result::Stale);
    QCOMPARE(store.statistics().submissions, before);
}

void KisGpuBrushStrokeTest::invalidInputAndGenerationLimitAreAtomic()
{
    Store store(m_gpu->owner, 64 * Store::TileBytes);
    Session session(store, 2);
    const auto token = session.begin();
    Stroke::Settings settings;
    settings.diameter = QSizeF(4, 4);
    Stroke stroke(session, token, settings, clip, 3);
    QCOMPARE(stroke.append({{{0, 0}, 1}}).result, Stroke::Result::Accepted);
    const auto before = store.statistics().submissions;
    QCOMPARE(stroke.append({{{1, 0}, 1}, {{2, 0}, std::numeric_limits<qreal>::quiet_NaN()}}).result,
             Stroke::Result::InvalidInput);
    QCOMPARE(stroke.append({{{10, 0}, 1}}).result, Stroke::Result::InputLimitExceeded);
    QCOMPARE(stroke.append({{{1, 0}, -1}}).result, Stroke::Result::InvalidInput);
    QCOMPARE(stroke.append({{{std::numeric_limits<qreal>::infinity(), 0}, 1}}).result, Stroke::Result::InvalidInput);
    QCOMPARE(stroke.replace({{{1e20, 0}, 1}, {{2e20, 0}, 1}}).result, Stroke::Result::InvalidInput);
    QCOMPARE(store.statistics().submissions, before);
    QCOMPARE(session.recovery()->working->size(), qsizetype(1));
    QCOMPARE(stroke.append({{{1, 0}, 1}}).result, Stroke::Result::Accepted);
    const auto saved = *session.recovery();
    QCOMPARE(saved.working->last().commands.size(), qsizetype(1));
    QCOMPARE(saved.working->last().commands.front().center, QPointF(1, 0));
    settings.spacing = -1;
    QVERIFY_THROWS_EXCEPTION(std::invalid_argument, Stroke(session, token, settings, clip));
}

void KisGpuBrushStrokeTest::textureStrengthMatchesDirectDab()
{
    Store store(m_gpu->owner, 128 * Store::TileBytes);
    Session session(store, 2);
    Stroke::Settings settings;
    settings.diameter = QSizeF(20, 16);
    settings.rgba = 0x800000FF;
    settings.textureStrength = 77;
    settings.texture = session.createTexture({3, 2}, QByteArray::fromHex("0080ff0140fe"), {-2, 3}).texture;
    const auto token = session.begin();
    Stroke stroke(session, token, settings, clip);
    QCOMPARE(stroke.append({{{0, 0}, 1}}).result, Stroke::Result::Accepted);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    session.poll();
    const auto fromInput = m_gpu->read(session.preview(), clip);
    QCOMPARE(session.replace(token, {dab(settings, QPointF(0, 0), settings.diameter)}, clip, *settings.texture),
             Session::Result::Accepted);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    session.poll();
    QCOMPARE(m_gpu->read(session.preview(), clip), fromInput);
}

void KisGpuBrushStrokeTest::selectedReplacementAndRecoveryUseIdenticalDabs_data()
{
    QTest::addColumn<bool>("textured");
    QTest::newRow("solid") << false;
    QTest::newRow("prepared-pattern") << true;
}

void KisGpuBrushStrokeTest::selectedReplacementAndRecoveryUseIdenticalDabs()
{
    QFETCH(bool, textured);
    Store store(m_gpu->owner, 128 * Store::TileBytes);
    Session session(store, 2);
    Stroke::Settings settings;
    settings.diameter = QSizeF(20, 16);
    settings.rgba = 0x800000FF;
    if (textured) settings.textureStrength = 77;
    if (textured) settings.texture = session.createTexture({3, 2}, QByteArray::fromHex("0080ff0140fe"), {-2, 3}).texture;
    const auto selection = session.createSelection({dab(settings, QPointF(0, 0), QSizeF(60, 60))}, clip).selection;
    const auto token = session.begin();
    Stroke stroke(session, token, settings, clip);
    QCOMPARE(stroke.append({{{-30, -20}, 1}, {{30, -20}, 1}}).result, Stroke::Result::Accepted);
    QCOMPARE(stroke.replace({{{-10, 0}, .25}, {{10, 0}, 1}}, selection).result, Stroke::Result::Accepted);
    wgpuDevicePoll(m_gpu->device, true, nullptr);
    session.poll();
    const auto preview = session.preview();
    const auto pixels = m_gpu->read(preview, clip);
    session.commit(token);
    settle(session);
    QVERIFY(session.head() == preview);
    Store destination(m_gpu->owner, 128 * Store::TileBytes);
    Session restored(destination, 2, *session.recovery());
    settle(restored);
    QCOMPARE(restored.state(), Session::State::Idle);
    QCOMPARE(m_gpu->read(restored.head(), clip), pixels);
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(textured ? 8 : 0));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(destination.statistics().pixelUploadBytes, quint64(textured ? 8 : 0));
    QCOMPARE(destination.statistics().pixelReadbackBytes, quint64(0));
}

void KisGpuBrushStrokeTest::deviceReplacementContinuesAcceptedInput_data()
{
    QTest::addColumn<bool>("textured");
    QTest::addColumn<bool>("replace");
    QTest::addColumn<bool>("mixed");
    QTest::newRow("append-solid") << false << false << false;
    QTest::newRow("append-textured") << true << false << false;
    QTest::newRow("replace-solid") << false << true << false;
    QTest::newRow("replace-textured") << true << true << false;
    QTest::newRow("append-mixed") << false << false << true;
    QTest::newRow("replace-mixed") << false << true << true;
}

void KisGpuBrushStrokeTest::deviceReplacementContinuesAcceptedInput()
{
    QFETCH(bool, textured);
    QFETCH(bool, replace);
    QFETCH(bool, mixed);
    using Document = KisGpuLayerDocument;
    const QVector<Stroke::Sample> prefix{{{-14.25, 4.75}, .23}, {{-5.6, 11.1}, .82}, {{3.13, 11.43}, .57}};
    const QVector<Stroke::Sample> suffix{{{8.4, 22.6}, .31}, {{23.75, 29.15}, .93}, {{19.2, 40.63}, .46}};
    const auto settingsFor = [&](Session &session) {
        Stroke::Settings settings;
        settings.diameter = {18, 13}; settings.fade = {.7, .9}; settings.spacing = .43;
        settings.rgba = 0xBE746852; settings.opacity = 203; settings.coverage = 181;
        if (mixed) { settings.smudgeRate = 213; settings.spacing = .8; }
        if (textured) {
            settings.textureStrength = 77;
            settings.texture = session.createTexture({3, 2}, QByteArray::fromHex("0080ff0140fe"), {-2, 3}).texture;
        }
        return settings;
    };
    const auto selectionFor = [&](Session &session) {
        Stroke::Settings settings;
        settings.rgba = 0xCC000000;
        return session.createSelection({dab(settings, {5, 17}, {80, 70})}, clip).selection;
    };
    Stroke::Settings red, blue;
    red.rgba = 0xCC0000FF; blue.rgba = 0xFFFF0000;
    const QVector<Store::DabCommand> seed{dab(red, {0, 0}, {80, 80}), dab(blue, {30, 30}, {25, 25})};
    QByteArray expected;
    {
        Store store(m_gpu->owner, 128 * Store::TileBytes);
        Session session(store, 2);
        auto preparation = session.begin();
        QCOMPARE(session.append(preparation, seed, clip), Session::Result::Accepted);
        session.commit(preparation); settle(session);
        const auto settings = settingsFor(session);
        const auto selection = selectionFor(session);
        QVERIFY(selection);
        const auto token = session.begin();
        Stroke stroke(session, token, settings, clip);
        if (replace) QCOMPARE(stroke.append({{{-40, -40}, 1}, {{-30, -30}, .5}}).result, Stroke::Result::Accepted);
        QCOMPARE((replace ? stroke.replace(prefix, selection) : stroke.append(prefix, selection)).result, Stroke::Result::Accepted);
        QCOMPARE(stroke.append(suffix, selection).result, Stroke::Result::Accepted);
        QCOMPARE(session.commit(token), Session::Result::Accepted);
        settle(session);
        QCOMPARE(session.state(), Session::State::Idle);
        expected = m_gpu->read(session.head(), clip);
    }

    KisGpuTestDevice original;
    auto store = std::make_shared<Store>(original.owner, 128 * Store::TileBytes);
    Document document(store, 8);
    QCOMPARE(document.addLayer(QStringLiteral("Paint")).result, Document::Result::Accepted);
    {
        const auto preparation = document.begin(1);
        QCOMPARE(preparation.session->append(preparation.token, seed, clip), Session::Result::Accepted);
        document.commit(preparation.token);
        QTRY_COMPARE_WITH_TIMEOUT((document.poll(), document.state()), Document::State::Idle, 5000);
    }
    Stroke::Progress progress;
    {
        auto edit = document.begin(1);
        QCOMPARE(edit.result, Document::Result::Accepted);
        const auto settings = settingsFor(*edit.session);
        const auto selection = selectionFor(*edit.session);
        QVERIFY(selection);
        Stroke stroke(*edit.session, edit.token, settings, clip);
        if (replace) QCOMPARE(stroke.append({{{-40, -40}, 1}, {{-30, -30}, .5}}).result, Stroke::Result::Accepted);
        QCOMPARE((replace ? stroke.replace(prefix, selection) : stroke.append(prefix, selection)).result, Stroke::Result::Accepted);
        QCOMPARE(stroke.append({{{0, 0}, -1}}).result, Stroke::Result::InvalidInput);
        progress = stroke.progress();
    }
    const auto before = store->statistics();
    QCOMPARE(document.suspend(), Document::Result::Accepted);
    wgpuDevicePoll(original.device, true, nullptr);
    store->poll();
    QCOMPARE(store->statistics().residentBytes, quint64(0));
    QCOMPARE(store->statistics().pixelReadbackBytes, before.pixelReadbackBytes);
    QCOMPARE(store->statistics().submissions, before.submissions);
    original.owner.destroy();

    KisGpuTestDevice replacement;
    auto nextStore = std::make_shared<Store>(replacement.owner, 128 * Store::TileBytes);
    QCOMPARE(document.resume(nextStore), Document::Result::Accepted);
    QElapsedTimer timer; timer.start();
    while (document.state() == Document::State::Restoring && timer.elapsed() < 5000) {
        document.poll(); QTest::qWait(1);
    }
    QCOMPARE(document.state(), Document::State::Editing);
    auto edit = document.currentEditing();
    const auto settings = settingsFor(*edit.session);
    const auto selection = selectionFor(*edit.session);
    QVERIFY(selection);
    const auto restored = nextStore->statistics();
    Stroke resumed(*edit.session, edit.token, settings, clip, progress);
    QCOMPARE(resumed.append(suffix, selection).result, Stroke::Result::Accepted);
    QCOMPARE(document.commit(edit.token), Document::Result::Accepted);
    timer.restart();
    while (document.state() == Document::State::Committing && timer.elapsed() < 5000) {
        document.poll(); QTest::qWait(1);
    }
    QCOMPARE(document.state(), Document::State::Idle);
    QCOMPARE(replacement.read(document.layers()[0].paint.pixels, clip), expected);
    QCOMPARE(nextStore->statistics().pixelUploadBytes, restored.pixelUploadBytes);
    QCOMPARE(nextStore->statistics().pixelReadbackBytes, restored.pixelReadbackBytes);
    QCOMPARE(original.owner.errorCount(), 0);
    QCOMPARE(replacement.owner.errorCount(), 0);
}

QTEST_GUILESS_MAIN(KisGpuBrushStrokeTest)
#include "KisGpuBrushStrokeTest.moc"

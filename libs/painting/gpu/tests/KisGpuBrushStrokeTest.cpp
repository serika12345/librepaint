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
#include "KisDabSpacing.h"
#include "KisGpuTestDevice.h"
#include <limits>
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
    void selectedReplacementAndRecoveryUseIdenticalDabs();
private:
    std::unique_ptr<KisGpuTestDevice> m_gpu;
};

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
    QCOMPARE(stroke.append({{{26.25, 16.25}, 1}}).result, Stroke::Result::Accepted);
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

void KisGpuBrushStrokeTest::selectedReplacementAndRecoveryUseIdenticalDabs()
{
    Store store(m_gpu->owner, 128 * Store::TileBytes);
    Session session(store, 2);
    Stroke::Settings settings;
    settings.diameter = QSizeF(20, 16);
    settings.rgba = 0x800000FF;
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
    QCOMPARE(store.statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(store.statistics().pixelReadbackBytes, quint64(0));
    QCOMPARE(destination.statistics().pixelUploadBytes, quint64(0));
    QCOMPARE(destination.statistics().pixelReadbackBytes, quint64(0));
}

QTEST_GUILESS_MAIN(KisGpuBrushStrokeTest)
#include "KisGpuBrushStrokeTest.moc"

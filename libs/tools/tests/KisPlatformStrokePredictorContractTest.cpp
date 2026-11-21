/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPlatformPredictionBuffer.h"
#include "KisPlatformStrokePredictor.h"
#include "KisStrokePrediction.h"
#include "KisStrokePredictor.h"

#include <QPointF>
#include <QTest>
#include <QThread>
#include <QTransform>
#include <QVector>

namespace
{
constexpr qreal HorizonMs = 16.7;
constexpr qreal DtMs = 8.0;

KisStrokePredictionSample sampleAt(const QPointF &position, qreal timeMs)
{
    KisStrokePredictionSample sample;
    sample.documentPos = position;
    sample.timeMs = timeMs;
    sample.pressure = 1.0;
    sample.tablet = true;
    return sample;
}

KisPlatformPredictedPoint predictedPoint(qreal x, qreal y, qreal dtMs)
{
    KisPlatformPredictedPoint point;
    point.widgetDelta = QPointF(x, y);
    point.dtMs = dtMs;
    return point;
}

KisPlatformPredictionEntry entryAt(quint64 keyMs, int coalescedCount,
                                   const QVector<KisPlatformPredictedPoint> &points)
{
    KisPlatformPredictionEntry entry;
    entry.keyMs = keyMs;
    entry.coalescedCount = coalescedCount;
    entry.points = points;
    return entry;
}

/**
 * Pushes 1000 entries from another thread while the test thread looks them up,
 * so the lock of the buffer is taken from both threads at once.
 */
class PushingThread : public QThread
{
public:
    explicit PushingThread(KisPlatformPredictionBuffer &buffer)
        : m_buffer(buffer)
    {
    }

protected:
    void run() override
    {
        for (int i = 0; i < 1000; ++i) {
            KisPlatformPredictionEntry entry;
            entry.keyMs = quint64(i + 1);
            entry.coalescedCount = 1;
            entry.points.append(predictedPoint(qreal(i), 0.0, DtMs));
            m_buffer.push(entry);
        }
    }

private:
    KisPlatformPredictionBuffer &m_buffer;
};
} // namespace

/**
 * The contract of the predictor that follows the prediction points of the
 * operating system, and of the buffer that carries those points from the
 * supplier to the predictor.
 */
class KisPlatformStrokePredictorContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void anEntryPredictsFromTheLatestRealPoint();
    void aCoalescedEntryChangesTheResultAtTheLastTouch();
    void aShortCoalescedEntryReportsIncompleteEvent();
    void aMissingEntryKeepsThePreviousResult();
    void theLinearTransformMapsTheDelta();
    void theHorizonDropsLongPredictedPoints();
    void theBufferReplacesKeysAndDropsOldEntries();
    void pushingAndFindingFromTwoThreadsFinishes();
};

void KisPlatformStrokePredictorContractTest::anEntryPredictsFromTheLatestRealPoint()
{
    KisPlatformPredictionBuffer buffer;
    buffer.push(entryAt(1000, 1, {predictedPoint(5.0, 0.0, DtMs)}));

    KisPlatformStrokePredictor predictor(KisStrokePredictionSettings(),
                                         KisStrokePredictionSource::IosSystem,
                                         buffer,
                                         QTransform());
    predictor.addSample(sampleAt(QPointF(100.0, 50.0), 1000.0));

    const KisStrokePredictionResult result = predictor.predict(HorizonMs);

    QVERIFY(result.valid);
    QCOMPARE(result.source, KisStrokePredictionSource::IosSystem);
    QCOMPARE(result.confidence, -1.0);
    QCOMPARE(result.polyline.size(), 2);
    QCOMPARE(result.polyline.constLast(), QPointF(105.0, 50.0));
    QCOMPARE(result.latestIndex, 0);
    QCOMPARE(result.velocity, QPointF(0.625, 0.0));
    QCOMPARE(result.speed, 0.625);
    QCOMPARE(result.moveLength, 5.0);
}

void KisPlatformStrokePredictorContractTest::aCoalescedEntryChangesTheResultAtTheLastTouch()
{
    KisPlatformPredictionBuffer buffer;
    buffer.push(entryAt(992, 1, {predictedPoint(4.0, 0.0, DtMs)}));
    buffer.push(entryAt(1000, 3, {predictedPoint(9.0, 0.0, DtMs)}));

    KisPlatformStrokePredictor predictor(KisStrokePredictionSettings(),
                                         KisStrokePredictionSource::IosSystem,
                                         buffer,
                                         QTransform());

    predictor.addSample(sampleAt(QPointF(0.0, 0.0), 992.0));
    const KisStrokePredictionResult previous = predictor.predict(HorizonMs);
    QVERIFY(previous.valid);
    QCOMPARE(previous.polyline.constLast(), QPointF(4.0, 0.0));

    predictor.addSample(sampleAt(QPointF(10.0, 0.0), 1000.0));
    QCOMPARE(predictor.predict(HorizonMs).polyline, previous.polyline);

    predictor.addSample(sampleAt(QPointF(11.0, 0.0), 1000.0));
    QCOMPARE(predictor.predict(HorizonMs).polyline, previous.polyline);

    predictor.addSample(sampleAt(QPointF(12.0, 0.0), 1000.0));
    const KisStrokePredictionResult updated = predictor.predict(HorizonMs);
    QVERIFY(updated.valid);
    QCOMPARE(updated.polyline.constLast(), QPointF(21.0, 0.0));
}

void KisPlatformStrokePredictorContractTest::aShortCoalescedEntryReportsIncompleteEvent()
{
    KisPlatformPredictionBuffer buffer;
    buffer.push(entryAt(992, 2, {predictedPoint(4.0, 0.0, DtMs)}));
    buffer.push(entryAt(1000, 1, {predictedPoint(4.0, 0.0, DtMs)}));

    KisPlatformStrokePredictor predictor(KisStrokePredictionSettings(),
                                         KisStrokePredictionSource::IosSystem,
                                         buffer,
                                         QTransform());

    predictor.addSample(sampleAt(QPointF(0.0, 0.0), 992.0));
    QCOMPARE(predictor.lastSampleMiss(), KisStrokePredictionMiss::None);

    predictor.addSample(sampleAt(QPointF(4.0, 0.0), 1000.0));
    QCOMPARE(predictor.lastSampleMiss(), KisStrokePredictionMiss::IncompleteEvent);
}

void KisPlatformStrokePredictorContractTest::aMissingEntryKeepsThePreviousResult()
{
    KisPlatformPredictionBuffer buffer;
    buffer.push(entryAt(992, 1, {predictedPoint(4.0, 0.0, DtMs)}));

    KisPlatformStrokePredictor predictor(KisStrokePredictionSettings(),
                                         KisStrokePredictionSource::IosSystem,
                                         buffer,
                                         QTransform());

    predictor.addSample(sampleAt(QPointF(0.0, 0.0), 992.0));
    const KisStrokePredictionResult previous = predictor.predict(HorizonMs);
    QVERIFY(previous.valid);

    predictor.addSample(sampleAt(QPointF(4.0, 0.0), 1000.0));
    QCOMPARE(predictor.lastSampleMiss(), KisStrokePredictionMiss::MissingEntry);
    QCOMPARE(predictor.predict(HorizonMs).polyline, previous.polyline);
}

void KisPlatformStrokePredictorContractTest::theLinearTransformMapsTheDelta()
{
    QTransform rotation;
    rotation.rotate(90.0);
    QTransform mirror;
    mirror.scale(-1.0, 1.0);
    QTransform enlargement;
    enlargement.scale(2.0, 2.0);
    QTransform reduction;
    reduction.scale(0.5, 0.5);

    const QPointF latestPosition(100.0, 100.0);
    const QPointF widgetDelta(10.0, 0.0);

    for (const QTransform &transform : {rotation, mirror, enlargement, reduction}) {
        KisPlatformPredictionBuffer buffer;
        buffer.push(entryAt(1000, 1, {predictedPoint(widgetDelta.x(), widgetDelta.y(), DtMs)}));

        KisPlatformStrokePredictor predictor(KisStrokePredictionSettings(),
                                             KisStrokePredictionSource::IosSystem,
                                             buffer,
                                             transform);
        predictor.addSample(sampleAt(latestPosition, 1000.0));

        const KisStrokePredictionResult result = predictor.predict(HorizonMs);
        QVERIFY(result.valid);
        QCOMPARE(result.polyline.constLast(), latestPosition + transform.map(widgetDelta));

        const QTransform withTranslation(transform.m11(), transform.m12(),
                                         transform.m21(), transform.m22(),
                                         37.0, -11.0);
        KisPlatformStrokePredictor translatedPredictor(KisStrokePredictionSettings(),
                                                       KisStrokePredictionSource::IosSystem,
                                                       buffer,
                                                       withTranslation);
        translatedPredictor.addSample(sampleAt(latestPosition, 1000.0));

        const KisStrokePredictionResult translated = translatedPredictor.predict(HorizonMs);
        QVERIFY(translated.valid);
        QCOMPARE(translated.polyline.constLast(), result.polyline.constLast());
    }

    QCOMPARE(rotation.map(widgetDelta), QPointF(0.0, 10.0));
    QCOMPARE(mirror.map(widgetDelta), QPointF(-10.0, 0.0));
}

void KisPlatformStrokePredictorContractTest::theHorizonDropsLongPredictedPoints()
{
    KisPlatformPredictionBuffer buffer;
    buffer.push(entryAt(1000, 1, {predictedPoint(1.0, 0.0, 5.0),
                                  predictedPoint(2.0, 0.0, 10.0),
                                  predictedPoint(3.0, 0.0, 30.0)}));

    KisPlatformStrokePredictor predictor(KisStrokePredictionSettings(),
                                         KisStrokePredictionSource::IosSystem,
                                         buffer,
                                         QTransform());
    predictor.addSample(sampleAt(QPointF(10.0, 0.0), 1000.0));

    const KisStrokePredictionResult result = predictor.predict(HorizonMs);
    QVERIFY(result.valid);
    QCOMPARE(result.polyline.size(), 3);
    QCOMPARE(result.polyline.constLast(), QPointF(12.0, 0.0));
    QCOMPARE(result.velocity, QPointF(0.2, 0.0));
    QCOMPARE(result.moveLength, 2.0);

    QVERIFY(!predictor.predict(4.0).valid);
}

void KisPlatformStrokePredictorContractTest::theBufferReplacesKeysAndDropsOldEntries()
{
    KisPlatformPredictionBuffer buffer;

    buffer.push(entryAt(1000, 1, {predictedPoint(1.0, 0.0, DtMs)}));
    buffer.push(entryAt(1000, 2, {predictedPoint(2.0, 0.0, DtMs)}));

    KisPlatformPredictionEntry entry;
    QVERIFY(buffer.find(1000, &entry));
    QCOMPARE(entry.coalescedCount, 2);
    QCOMPARE(entry.points.size(), 1);
    QCOMPARE(entry.points.constFirst().widgetDelta, QPointF(2.0, 0.0));

    buffer.push(entryAt(1250, 1, {predictedPoint(3.0, 0.0, DtMs)}));
    QVERIFY(buffer.find(1000, &entry));

    buffer.push(entryAt(1251, 1, {predictedPoint(4.0, 0.0, DtMs)}));
    QVERIFY(!buffer.find(1000, &entry));
    QVERIFY(buffer.find(1250, &entry));

    buffer.clear();
    QVERIFY(!buffer.find(1250, &entry));

    for (quint64 key = 2000; key < 2000 + 257; ++key) {
        buffer.push(entryAt(key, 1, {predictedPoint(5.0, 0.0, DtMs)}));
    }

    int foundCount = 0;
    for (quint64 key = 2000; key < 2000 + 257; ++key) {
        if (buffer.find(key, &entry)) {
            ++foundCount;
        }
    }
    QVERIFY(!buffer.find(2000, &entry));
    QVERIFY(buffer.find(2256, &entry));
    QVERIFY(foundCount <= KisPlatformPredictionBuffer::maxEntries);
}

void KisPlatformStrokePredictorContractTest::pushingAndFindingFromTwoThreadsFinishes()
{
    KisPlatformPredictionBuffer buffer;
    PushingThread thread(buffer);
    thread.start();

    KisPlatformPredictionEntry entry;
    for (quint64 key = 1; key <= 1000; ++key) {
        buffer.find(key, &entry);
    }

    QVERIFY(thread.wait(30000));
    QVERIFY(buffer.find(1000, &entry));
}

QTEST_GUILESS_MAIN(KisPlatformStrokePredictorContractTest)

#include "KisPlatformStrokePredictorContractTest.moc"

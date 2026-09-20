/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisSpatialContainer.h"

#include <QTest>

#include <memory>

void kis_safe_assert_recoverable(const char *, const char *, int)
{
}

namespace
{
QStringList *capturedMessages = nullptr;

void captureMessage(QtMsgType, const QMessageLogContext &, const QString &message)
{
    if (capturedMessages) {
        capturedMessages->append(message);
    }
}

class MessageCapture
{
public:
    explicit MessageCapture(QStringList *messages)
        : m_previousHandler(qInstallMessageHandler(captureMessage))
    {
        capturedMessages = messages;
    }

    ~MessageCapture()
    {
        capturedMessages = nullptr;
        qInstallMessageHandler(m_previousHandler);
    }

private:
    QtMessageHandler m_previousHandler;
};
} // namespace

class KisSpatialContainerContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionProvidesEmptyOwnedStorage();
    void vectorInitializationAndCopyPreserveIndependentDenseData();
    void plannedStorageAddsMovesAndFindsCurrentRangeResults();
    void gridInitializationProducesIndexedBoundaryPoints();
    void boundariesTrackAllStoredPoints();
    void removalClearAndReinitializationRetainStaleCount();
    void diagnosticsDescribeStoredTree();
};

void KisSpatialContainerContractTest::constructionProvidesEmptyOwnedStorage()
{
    auto container = std::make_unique<KisSpatialContainer>(QRectF(0.0, 0.0, 100.0, 80.0));

    QCOMPARE(container->count(), 0);
    QVERIFY(container->toVector().isEmpty());

    container.reset();
    QVERIFY(!container);
}

void KisSpatialContainerContractTest::vectorInitializationAndCopyPreserveIndependentDenseData()
{
    QVector<QPointF> points{QPointF(3.0, 4.0), QPointF(20.0, 12.0), QPointF(50.0, 70.0)};
    auto source = std::make_unique<KisSpatialContainer>(QRectF(0.0, 0.0, 100.0, 100.0), points);

    QCOMPARE(source->count(), points.size());
    QCOMPARE(source->toVector(), points);

    KisSpatialContainer copy(*source);
    source->movePoint(1, points[1], QPointF(24.0, 12.0));

    QCOMPARE(copy.count(), points.size());
    QCOMPARE(copy.toVector(), points);

    source.reset();
    QCOMPARE(copy.toVector(), points);

    KisSpatialContainer initialized(QRectF(0.0, 0.0, 100.0, 100.0));
    initialized.initializeWith(points);
    QCOMPARE(initialized.toVector(), points);
}

void KisSpatialContainerContractTest::plannedStorageAddsMovesAndFindsCurrentRangeResults()
{
    KisSpatialContainer container(QRectF(0.0, 0.0, 100.0, 100.0));
    container.initializeFor(2, QRectF(0.0, 0.0, 100.0, 100.0));
    container.addPoint(0, QPointF(10.0, 10.0));
    container.addPoint(1, QPointF(20.0, 20.0));

    QCOMPARE(container.count(), 2);
    QCOMPARE(container.toVector(), QVector<QPointF>({QPointF(10.0, 10.0), QPointF(20.0, 20.0)}));

    container.movePoint(1, QPointF(20.0, 20.0), QPointF(25.0, 20.0));
    QCOMPARE(container.count(), 2);
    QCOMPARE(container.toVector(), QVector<QPointF>({QPointF(10.0, 10.0), QPointF(25.0, 20.0)}));

    QVector<int> indexes{-1};
    container.findAllInRange(indexes, QPointF(10.0, 10.0), 12.0);
    QCOMPARE(indexes, QVector<int>({-1, 0}));
}

void KisSpatialContainerContractTest::gridInitializationProducesIndexedBoundaryPoints()
{
    KisSpatialContainer container(QRectF(0.0, 0.0, 9.0, 9.0));
    container.initializeWithGridPoints(QRectF(0.0, 0.0, 9.0, 9.0), 4);

    const QVector<QPointF> expected{
        QPointF(0.0, 0.0),
        QPointF(4.0, 0.0),
        QPointF(8.0, 0.0),
        QPointF(0.0, 4.0),
        QPointF(4.0, 4.0),
        QPointF(8.0, 4.0),
        QPointF(0.0, 8.0),
        QPointF(4.0, 8.0),
        QPointF(8.0, 8.0),
    };

    QCOMPARE(container.count(), expected.size());
    QCOMPARE(container.toVector(), expected);
}

void KisSpatialContainerContractTest::boundariesTrackAllStoredPoints()
{
    KisSpatialContainer container(QRectF(-10.0, -10.0, 30.0, 30.0));
    container.addPoint(0, QPointF(-4.0, 3.0));
    container.addPoint(1, QPointF(2.0, -5.0));
    container.addPoint(2, QPointF(7.0, 8.0));

    QCOMPARE(container.getTopLeft(), QPointF(-4.0, -5.0));
    QCOMPARE(container.exactBounds(), QRectF(QPointF(-4.0, -5.0), QPointF(7.0, 8.0)));
}

void KisSpatialContainerContractTest::removalClearAndReinitializationRetainStaleCount()
{
    KisSpatialContainer container(QRectF(0.0, 0.0, 100.0, 100.0));
    container.addPoint(0, QPointF(8.0, 9.0));
    container.addPoint(1, QPointF(18.0, 19.0));

    container.removePoint(1, QPointF(18.0, 19.0));
    QCOMPARE(container.count(), 2);
    QCOMPARE(container.toVector(), QVector<QPointF>({QPointF(8.0, 9.0), QPointF()}));

    container.clear();
    QCOMPARE(container.count(), 2);
    QCOMPARE(container.toVector(), QVector<QPointF>({QPointF(), QPointF()}));

    const QVector<QPointF> replacement{QPointF(30.0, 40.0)};
    container.initializeWith(replacement);
    QCOMPARE(container.count(), 3);
    QCOMPARE(container.toVector(), QVector<QPointF>({QPointF(30.0, 40.0), QPointF(), QPointF()}));
}

void KisSpatialContainerContractTest::diagnosticsDescribeStoredTree()
{
    KisSpatialContainer container(QRectF(0.0, 0.0, 100.0, 100.0));
    container.addPoint(0, QPointF(8.0, 9.0));

    QStringList messages;
    {
        MessageCapture capture(&messages);
        container.debugWriteOut();
    }

    QCOMPARE(messages.size(), 2);
    QVERIFY(messages[0].contains(QStringLiteral("is Leaf? true")));
    QVERIFY(messages[0].contains(QStringLiteral("pointsCount = 1")));
    QVERIFY(messages[0].contains(QStringLiteral("pointsData.length() = 1")));
    QVERIFY(messages[1].contains(QStringLiteral(" -  0  |  QPointF(8,9)")));
    QCOMPARE(container.toVector(), QVector<QPointF>({QPointF(8.0, 9.0)}));
}

QTEST_GUILESS_MAIN(KisSpatialContainerContractTest)

#include "KisSpatialContainerContractTest.moc"

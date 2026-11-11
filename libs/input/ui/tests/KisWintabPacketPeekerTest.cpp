/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <QPointF>
#include <QRect>
#include <QVector>

#include <input/ui/wintab/KisWintabMapping.h>
#include <input/ui/wintab/KisWintabPacketPeeker.h>

namespace
{

class FakeWintabApi : public KisWintabApi
{
public:
    KisWintabApi::Context context;
    QVector<KisWintabApi::Packet> packets;
    int peekCalls = 0;
    int contextCalls = 0;

    bool getContext(quintptr contextHandle,
                    KisWintabApi::Context *data) override
    {
        Q_UNUSED(contextHandle);
        ++contextCalls;
        *data = context;
        return true;
    }

    int packetsPeek(quintptr contextHandle,
                    KisWintabApi::Packet *out,
                    int maxPackets) override
    {
        Q_UNUSED(contextHandle);
        ++peekCalls;
        const int count = qMin(maxPackets, packets.size());
        for (int i = 0; i < count; ++i) {
            out[i] = packets.at(i);
        }
        return count;
    }
};

KisWintabApi::Context contextFor(int inExtX, int inExtY)
{
    KisWintabApi::Context context;
    context.packetData = KisWintabPacketPeeker::PacketData;
    context.inOrgX = 0;
    context.inOrgY = 0;
    context.inExtX = inExtX;
    context.inExtY = inExtY;
    context.sysOrgX = 0;
    context.sysOrgY = 0;
    context.sysExtX = inExtX;
    context.sysExtY = inExtY;
    return context;
}

KisWintabMapping::Screen screen(const QRect &nativeRect, qreal dpr)
{
    KisWintabMapping::Screen value;
    value.nativeRect = nativeRect;
    value.logicalRect = QRect(nativeRect.topLeft(),
                              QSize(qRound(nativeRect.width() / dpr),
                                    qRound(nativeRect.height() / dpr)));
    value.devicePixelRatio = dpr;
    return value;
}

} // namespace

class KisWintabPacketPeekerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testIgnoresOtherPacketData();
    void testLooksUpByPacketTime();
    void testOverwritesOldEntries();
    void testVirtualScreenKeepsNoCorrection();
};

void KisWintabPacketPeekerTest::testIgnoresOtherPacketData()
{
    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::Custom,
                                        QRect(0, 0, 500, 250));

    FakeWintabApi api;
    api.context = contextFor(1000, 500);
    api.context.packetData = 0;
    api.packets = {KisWintabApi::Packet{10, 20, 1234}};

    KisWintabPacketPeeker peeker(&api);
    peeker.setScreensForTesting({screen(QRect(0, 0, 1000, 500), 1.0)});

    QVERIFY(!peeker.handleNativeMessage(KisWintabPacketPeeker::PacketMessage,
                                        0, 1));
    QCOMPARE(api.peekCalls, 0);

    QPointF position;
    QVERIFY(!peeker.lookup(1234, &position));

    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::Wintab,
                                        QRect());
}

void KisWintabPacketPeekerTest::testLooksUpByPacketTime()
{
    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::Custom,
                                        QRect(0, 0, 500, 250));

    FakeWintabApi api;
    api.context = contextFor(1000, 500);
    api.packets = {
        KisWintabApi::Packet{0, 0, 100},
        KisWintabApi::Packet{500, 250, 101},
        KisWintabApi::Packet{1000, 500, 102}
    };

    KisWintabPacketPeeker peeker(&api);
    peeker.setScreensForTesting({screen(QRect(0, 0, 1000, 500), 1.0)});

    QVERIFY(peeker.handleNativeMessage(KisWintabPacketPeeker::PacketMessage,
                                       0, 1));
    QCOMPARE(api.peekCalls, 1);

    const auto expectPosition = [&peeker](quint64 time, const QPointF &expected) {
        QPointF position;
        QVERIFY2(peeker.lookup(time, &position),
                 qPrintable(QString("no packet for time %1").arg(time)));
        QCOMPARE(position, expected);
    };

    expectPosition(100, QPointF(0, 0));
    expectPosition(101, QPointF(250, 125));
    expectPosition(102, QPointF(500, 250));

    QPointF position;
    QVERIFY(!peeker.lookup(103, &position));

    // A message that is not WT_PACKET peeks nothing.
    QVERIFY(!peeker.handleNativeMessage(0x7FF1, 0, 1));
    QCOMPARE(api.peekCalls, 1);

    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::Wintab,
                                        QRect());
}

void KisWintabPacketPeekerTest::testOverwritesOldEntries()
{
    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::Custom,
                                        QRect(0, 0, 500, 250));

    constexpr int entryCount = 256;
    FakeWintabApi api;
    api.context = contextFor(1000, 500);

    KisWintabPacketPeeker peeker(&api);
    peeker.setScreensForTesting({screen(QRect(0, 0, 1000, 500), 1.0)});

    for (int i = 1; i <= entryCount + 44; ++i) {
        api.packets = {KisWintabApi::Packet{i % 1000, i % 500, quint32(i)}};
        QVERIFY(peeker.handleNativeMessage(
            KisWintabPacketPeeker::PacketMessage, 0, 1));
    }

    QPointF position;
    QVERIFY(!peeker.lookup(1, &position));
    QVERIFY(!peeker.lookup(44, &position));
    QVERIFY(peeker.lookup(45, &position));
    QVERIFY(peeker.lookup(quint64(entryCount + 44), &position));

    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::Wintab,
                                        QRect());
}

void KisWintabPacketPeekerTest::testVirtualScreenKeepsNoCorrection()
{
    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::VirtualScreen,
                                        QRect());

    FakeWintabApi api;
    api.context = contextFor(1000, 500);
    api.context.sysOrgX = -10;
    api.context.sysOrgY = -20;
    api.context.sysExtX = 900;
    api.context.sysExtY = 400;
    api.packets = {KisWintabApi::Packet{10, 20, 77}};

    KisWintabPacketPeeker peeker(&api);
    peeker.setScreensForTesting({screen(QRect(0, 0, 1000, 500), 1.0)});

    QVERIFY(peeker.handleNativeMessage(KisWintabPacketPeeker::PacketMessage,
                                       0, 1));

    QPointF position;
    QVERIFY(!peeker.lookup(77, &position));

    KisWintabMapping::setConfiguredMode(KisWintabMapping::Mode::Wintab,
                                        QRect());
}

QTEST_GUILESS_MAIN(KisWintabPacketPeekerTest)

#include "KisWintabPacketPeekerTest.moc"

/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "virtual_channel_info.h"

#include <KoColorSpace.h>

#include <QtTest>


QList<KoChannelInfo *> KoColorSpace::channels() const
{
    qFatal("VirtualChannelInfoContractTest must not enter the real color-space path");
}

class VirtualChannelInfoContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void constructionDefaultsRemainStable();
    void typePositionAndBorrowedChannelRemainStable();
    void channelRepresentationRemainStable();
    void alphaClassificationRemainsStable();
};

void VirtualChannelInfoContractTest::constructionDefaultsRemainStable()
{

    const VirtualChannelInfo defaultChannel;
    QCOMPARE(int(defaultChannel.type()), int(VirtualChannelInfo::LIGHTNESS));
    QCOMPARE(defaultChannel.pixelIndex(), -1);
    QCOMPARE(defaultChannel.channelInfo(), nullptr);
    QVERIFY(defaultChannel.name().isEmpty());
    QCOMPARE(int(defaultChannel.valueType()), int(KoChannelInfo::FLOAT32));
    QCOMPARE(defaultChannel.channelSize(), 4);
    QVERIFY(!defaultChannel.isAlpha());
}

void VirtualChannelInfoContractTest::typePositionAndBorrowedChannelRemainStable()
{
    KoChannelInfo realChannel(QStringLiteral("Cyan"), 2, 0, KoChannelInfo::COLOR, KoChannelInfo::UINT16);
    const VirtualChannelInfo info(VirtualChannelInfo::REAL, 3, &realChannel, nullptr);

    QCOMPARE(int(info.type()), int(VirtualChannelInfo::REAL));
    QCOMPARE(info.pixelIndex(), 3);
    QCOMPARE(info.channelInfo(), &realChannel);
}

void VirtualChannelInfoContractTest::channelRepresentationRemainStable()
{
    KoChannelInfo realChannel(QStringLiteral("Depth"), 4, 0, KoChannelInfo::COLOR, KoChannelInfo::FLOAT64);
    const VirtualChannelInfo info(VirtualChannelInfo::REAL, 1, &realChannel, nullptr);

    QCOMPARE(info.name(), QStringLiteral("Depth"));
    QCOMPARE(int(info.valueType()), int(KoChannelInfo::FLOAT64));
    QCOMPARE(info.channelSize(), 8);
}

void VirtualChannelInfoContractTest::alphaClassificationRemainsStable()
{
    KoChannelInfo colorChannel(QStringLiteral("Red"), 0, 0, KoChannelInfo::COLOR, KoChannelInfo::UINT8);
    KoChannelInfo alphaChannel(QStringLiteral("Alpha"), 1, 1, KoChannelInfo::ALPHA, KoChannelInfo::UINT8);
    const VirtualChannelInfo colorInfo(VirtualChannelInfo::REAL, 0, &colorChannel, nullptr);
    const VirtualChannelInfo alphaInfo(VirtualChannelInfo::REAL, 1, &alphaChannel, nullptr);
    const VirtualChannelInfo virtualInfo;

    QVERIFY(!colorInfo.isAlpha());
    QVERIFY(alphaInfo.isAlpha());
    QVERIFY(!virtualInfo.isAlpha());
}

QTEST_APPLESS_MAIN(VirtualChannelInfoContractTest)

#include "VirtualChannelInfoContractTest.moc"

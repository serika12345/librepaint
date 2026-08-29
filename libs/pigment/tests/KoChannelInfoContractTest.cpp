/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoChannelInfo.h"

#include <QTest>

#include <limits>

class KoChannelInfoContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rangesDescribeValidityAndCustomBounds();
    void valueTypesDeriveStorageAndDefaultUiRanges_data();
    void valueTypesDeriveStorageAndDefaultUiRanges();
    void channelMetadataPreservesPixelAndPresentationIdentity();
    void displayOrderMapsPixelStorageToPresentation();
    void bytePositionComparisonOrdersChannelsIndependentlyOfDisplayOrder();
    void defaultConstructionProvidesSafePartialState();
};

void KoChannelInfoContractTest::rangesDescribeValidityAndCustomBounds()
{
    const KoChannelInfo::DoubleRange emptyRange;

    QCOMPARE(emptyRange.minVal, 0.0);
    QCOMPARE(emptyRange.maxVal, 0.0);
    QVERIFY(!emptyRange.isValid());

    const KoChannelInfo::DoubleRange customRange(-2.0, 3.0);

    QCOMPARE(customRange.minVal, -2.0);
    QCOMPARE(customRange.maxVal, 3.0);
    QVERIFY(customRange.isValid());

    const KoChannelInfo channel(QStringLiteral("Temperature"),
                                0,
                                0,
                                KoChannelInfo::COLOR,
                                KoChannelInfo::FLOAT32,
                                -1,
                                QColor(40, 80, 120),
                                customRange);

    QCOMPARE(channel.getUIMin(), -2.0);
    QCOMPARE(channel.getUIMax(), 3.0);
    QCOMPARE(channel.getUIUnitValue(), 5.0);
}

void KoChannelInfoContractTest::valueTypesDeriveStorageAndDefaultUiRanges_data()
{
    QTest::addColumn<int>("valueType");
    QTest::addColumn<int>("explicitSize");
    QTest::addColumn<int>("expectedSize");
    QTest::addColumn<double>("expectedMinimum");
    QTest::addColumn<double>("expectedMaximum");

    QTest::newRow("uint8") << int(KoChannelInfo::UINT8) << -1 << 1 << double(std::numeric_limits<quint8>::min())
                           << double(std::numeric_limits<quint8>::max());
    QTest::newRow("uint16") << int(KoChannelInfo::UINT16) << -1 << 2 << double(std::numeric_limits<quint16>::min())
                            << double(std::numeric_limits<quint16>::max());
    QTest::newRow("uint32") << int(KoChannelInfo::UINT32) << -1 << 4 << double(std::numeric_limits<quint32>::min())
                            << double(std::numeric_limits<quint32>::max());
    QTest::newRow("float16") << int(KoChannelInfo::FLOAT16) << -1 << 2 << 0.0 << 1.0;
    QTest::newRow("float32") << int(KoChannelInfo::FLOAT32) << -1 << 4 << 0.0 << 1.0;
    QTest::newRow("float64") << int(KoChannelInfo::FLOAT64) << -1 << 8 << 0.0 << 1.0;
    QTest::newRow("int8") << int(KoChannelInfo::INT8) << -1 << 1 << double(std::numeric_limits<qint8>::min())
                          << double(std::numeric_limits<qint8>::max());
    QTest::newRow("int16") << int(KoChannelInfo::INT16) << -1 << 2 << double(std::numeric_limits<qint16>::min())
                           << double(std::numeric_limits<qint16>::max());
    QTest::newRow("other") << int(KoChannelInfo::OTHER) << 3 << 3 << 0.0 << 1.0;
}

void KoChannelInfoContractTest::valueTypesDeriveStorageAndDefaultUiRanges()
{
    QFETCH(int, valueType);
    QFETCH(int, explicitSize);
    QFETCH(int, expectedSize);
    QFETCH(double, expectedMinimum);
    QFETCH(double, expectedMaximum);

    const auto channelValueType = static_cast<KoChannelInfo::enumChannelValueType>(valueType);
    const KoChannelInfo channel(QStringLiteral("Value"), 0, 0, KoChannelInfo::COLOR, channelValueType, explicitSize);

    QCOMPARE(int(channel.channelValueType()), valueType);
    QCOMPARE(channel.size(), expectedSize);
    QCOMPARE(channel.getUIMin(), expectedMinimum);
    QCOMPARE(channel.getUIMax(), expectedMaximum);
    QCOMPARE(channel.getUIUnitValue(), expectedMaximum - expectedMinimum);
}

void KoChannelInfoContractTest::channelMetadataPreservesPixelAndPresentationIdentity()
{
    const KoChannelInfo
        red(QStringLiteral("Red"), 4, 1, KoChannelInfo::COLOR, KoChannelInfo::UINT16, -1, QColor(220, 10, 20));
    const KoChannelInfo alpha(QStringLiteral("Alpha"), 6, 3, KoChannelInfo::ALPHA, KoChannelInfo::FLOAT32);

    QCOMPARE(red.name(), QStringLiteral("Red"));
    QCOMPARE(red.pos(), 4);
    QCOMPARE(red.displayPosition(), 1);
    QCOMPARE(int(red.channelType()), int(KoChannelInfo::COLOR));
    QCOMPARE(red.color(), QColor(220, 10, 20));

    QCOMPARE(alpha.name(), QStringLiteral("Alpha"));
    QCOMPARE(alpha.pos(), 6);
    QCOMPARE(alpha.displayPosition(), 3);
    QCOMPARE(int(alpha.channelType()), int(KoChannelInfo::ALPHA));
    QCOMPARE(alpha.color(), QColor(0, 0, 0));
}

void KoChannelInfoContractTest::displayOrderMapsPixelStorageToPresentation()
{
    KoChannelInfo blue(QStringLiteral("Blue"), 0, 2, KoChannelInfo::COLOR, KoChannelInfo::UINT8);
    KoChannelInfo green(QStringLiteral("Green"), 1, 1, KoChannelInfo::COLOR, KoChannelInfo::UINT8);
    KoChannelInfo red(QStringLiteral("Red"), 2, 0, KoChannelInfo::COLOR, KoChannelInfo::UINT8);
    KoChannelInfo alpha(QStringLiteral("Alpha"), 3, 3, KoChannelInfo::ALPHA, KoChannelInfo::UINT8);
    const QList<KoChannelInfo *> pixelOrder{&blue, &green, &red, &alpha};

    QCOMPARE(KoChannelInfo::displayPositionToChannelIndex(0, pixelOrder), 2);
    QCOMPARE(KoChannelInfo::displayPositionToChannelIndex(1, pixelOrder), 1);
    QCOMPARE(KoChannelInfo::displayPositionToChannelIndex(2, pixelOrder), 0);
    QCOMPARE(KoChannelInfo::displayPositionToChannelIndex(3, pixelOrder), 3);
    QCOMPARE(KoChannelInfo::displayPositionToChannelIndex(4, pixelOrder), -1);

    const QList<KoChannelInfo *> displayOrder = KoChannelInfo::displayOrderSorted(pixelOrder);

    QCOMPARE(displayOrder.size(), 4);
    QCOMPARE(displayOrder.at(0), &red);
    QCOMPARE(displayOrder.at(1), &green);
    QCOMPARE(displayOrder.at(2), &blue);
    QCOMPARE(displayOrder.at(3), &alpha);
    QCOMPARE(pixelOrder.at(0), &blue);
    QVERIFY(KoChannelInfo::displayOrderSorted({}).isEmpty());
}

void KoChannelInfoContractTest::bytePositionComparisonOrdersChannelsIndependentlyOfDisplayOrder()
{
    KoChannelInfo last(QStringLiteral("Last"), 8, 0, KoChannelInfo::COLOR, KoChannelInfo::UINT16);
    KoChannelInfo first(QStringLiteral("First"), 0, 2, KoChannelInfo::COLOR, KoChannelInfo::UINT16);
    KoChannelInfo middle(QStringLiteral("Middle"), 4, 1, KoChannelInfo::COLOR, KoChannelInfo::UINT16);

    QVERIFY(first < middle);
    QVERIFY(middle < last);
    QVERIFY(!(last < first));
}

void KoChannelInfoContractTest::defaultConstructionProvidesSafePartialState()
{
    const KoChannelInfo channel;

    QVERIFY(channel.name().isEmpty());
    QVERIFY(!channel.color().isValid());
    QCOMPARE(channel.getUIMin(), 0.0);
    QCOMPARE(channel.getUIMax(), 0.0);
    QCOMPARE(channel.getUIUnitValue(), 0.0);
}

QTEST_GUILESS_MAIN(KoChannelInfoContractTest)

#include "KoChannelInfoContractTest.moc"

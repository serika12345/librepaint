/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <psd_additional_layer_info_block.h>
#include <psd_layer_record.h>
#include <psd_layer_section.h>
#include <psd_pixel_utils.h>
#include <psd_resource_block.h>

#include <QTest>


namespace
{
} // namespace

class PSDLayerRecordSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pixelChannelWritingInfoAndIoSignaturesRemainStable();
};

void PSDLayerRecordSchemaContractTest::pixelChannelWritingInfoAndIoSignaturesRemainStable()
{
    using Info = PsdPixelUtils::ChannelWritingInfo;


    const Info defaults;
    QCOMPARE(defaults.channelId, qint16(0));
    QCOMPARE(defaults.sizeFieldOffset, -1);
    QCOMPARE(defaults.rleBlockOffset, -1);

    const Info externalSizeTag(-3, 17);
    QCOMPARE(externalSizeTag.channelId, qint16(-3));
    QCOMPARE(externalSizeTag.sizeFieldOffset, 17);
    QCOMPARE(externalSizeTag.rleBlockOffset, -1);

    const Info externalOffsets(7, 23, 31);
    QCOMPARE(externalOffsets.channelId, qint16(7));
    QCOMPARE(externalOffsets.sizeFieldOffset, 23);
    QCOMPARE(externalOffsets.rleBlockOffset, 31);
}

QTEST_APPLESS_MAIN(PSDLayerRecordSchemaContractTest)

#include "PSDLayerRecordSchemaContractTest.moc"

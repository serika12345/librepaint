/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_png_converter.h>

#include <QTest>

#include <utility>

namespace
{
} // namespace

class KisPNGConverterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pngOptionCoreValueSchemaRemainsStable();
    void pngOptionMetadataAndProfileFlagsRemainStable();
    void pngOptionIndexedFilterAndTransparencyValuesRemainStable();
};

void KisPNGConverterSchemaContractTest::pngOptionCoreValueSchemaRemainsStable()
{

    KisPNGOptions original;
    QCOMPARE(original.compression, 0);
    QCOMPARE(original.interlace, false);
    QCOMPARE(original.alpha, true);
    QCOMPARE(original.downsample, false);

    KisPNGOptions copy = original;
    copy.compression = 7;
    copy.interlace = true;
    copy.alpha = false;
    copy.downsample = true;
    QCOMPARE(original.compression, 0);
    QCOMPARE(original.interlace, false);
    QCOMPARE(original.alpha, true);
    QCOMPARE(original.downsample, false);
}

void KisPNGConverterSchemaContractTest::pngOptionMetadataAndProfileFlagsRemainStable()
{

    KisPNGOptions original;
    QCOMPARE(original.exif, true);
    QCOMPARE(original.iptc, true);
    QCOMPARE(original.xmp, true);
    QCOMPARE(original.saveSRGBProfile, false);
    QCOMPARE(original.forceSRGB, false);
    QCOMPARE(original.storeMetaData, false);
    QCOMPARE(original.storeAuthor, false);
    QCOMPARE(original.saveAsHDR, false);

    KisPNGOptions copy = original;
    copy.exif = false;
    copy.iptc = false;
    copy.xmp = false;
    copy.saveSRGBProfile = true;
    copy.forceSRGB = true;
    copy.storeMetaData = true;
    copy.storeAuthor = true;
    copy.saveAsHDR = true;
    QCOMPARE(original.exif, true);
    QCOMPARE(original.iptc, true);
    QCOMPARE(original.xmp, true);
    QCOMPARE(original.saveSRGBProfile, false);
    QCOMPARE(original.forceSRGB, false);
    QCOMPARE(original.storeMetaData, false);
    QCOMPARE(original.storeAuthor, false);
    QCOMPARE(original.saveAsHDR, false);
}

void KisPNGConverterSchemaContractTest::pngOptionIndexedFilterAndTransparencyValuesRemainStable()
{

    KisPNGOptions original;
    QCOMPARE(original.tryToSaveAsIndexed, true);
    QVERIFY(original.filters.isEmpty());
    QCOMPARE(original.transparencyFillColor, QColor(Qt::white));

    KisPNGOptions copy = original;
    copy.tryToSaveAsIndexed = false;
    copy.filters.append(nullptr);
    copy.transparencyFillColor = QColor(Qt::black);
    QCOMPARE(original.tryToSaveAsIndexed, true);
    QVERIFY(original.filters.isEmpty());
    QCOMPARE(original.transparencyFillColor, QColor(Qt::white));
}

QTEST_APPLESS_MAIN(KisPNGConverterSchemaContractTest)

#include "KisPNGConverterSchemaContractTest.moc"

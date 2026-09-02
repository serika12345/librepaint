/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_png_converter.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_PNG_CONVERTER_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPNGConverter::method)), signature>)
} // namespace

class KisPNGConverterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pngOptionCoreValueSchemaRemainsStable();
    void pngOptionMetadataAndProfileFlagsRemainStable();
    void pngOptionIndexedFilterAndTransparencyValuesRemainStable();
    void pngConverterIdentityAndInputSignaturesRemainStable();
    void pngConverterOutputAndControlSignaturesRemainStable();
};

void KisPNGConverterSchemaContractTest::pngOptionCoreValueSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisPNGOptions>);
    static_assert(std::is_default_constructible_v<KisPNGOptions>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::compression), int>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::interlace), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::alpha), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::downsample), bool>);

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
    static_assert(std::is_same_v<decltype(KisPNGOptions::exif), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::iptc), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::xmp), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::saveSRGBProfile), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::forceSRGB), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::storeMetaData), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::storeAuthor), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::saveAsHDR), bool>);

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
    static_assert(std::is_same_v<decltype(KisPNGOptions::tryToSaveAsIndexed), bool>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::filters), QList<const KisMetaData::Filter *>>);
    static_assert(std::is_same_v<decltype(KisPNGOptions::transparencyFillColor), QColor>);

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

void KisPNGConverterSchemaContractTest::pngConverterIdentityAndInputSignaturesRemainStable()
{
    static_assert(std::is_class_v<KisPNGConverter>);
    static_assert(std::is_constructible_v<KisPNGConverter, KisDocument *>);
    static_assert(std::is_constructible_v<KisPNGConverter, KisDocument *, bool>);
    static_assert(std::has_virtual_destructor_v<KisPNGConverter>);
    ASSERT_PNG_CONVERTER_SIGNATURE(buildImage, KisImportExportErrorCode (KisPNGConverter::*)(QIODevice *));
    ASSERT_PNG_CONVERTER_SIGNATURE(buildImage, KisImportExportErrorCode (KisPNGConverter::*)(const QString &));
    ASSERT_PNG_CONVERTER_SIGNATURE(image, KisImageSP (KisPNGConverter::*)());

    static_assert(std::is_same_v<decltype(KisPNGConverter(std::declval<KisDocument *>())), KisPNGConverter>);
}

void KisPNGConverterSchemaContractTest::pngConverterOutputAndControlSignaturesRemainStable()
{
    using BuildFileWithDevice = KisImportExportErrorCode (KisPNGConverter::*)(QIODevice *,
                                                                              const QRect &,
                                                                              qreal,
                                                                              qreal,
                                                                              KisPaintDeviceSP,
                                                                              vKisAnnotationSP_it,
                                                                              vKisAnnotationSP_it,
                                                                              KisPNGOptions,
                                                                              KisMetaData::Store *);
    using BuildFileWithName = KisImportExportErrorCode (KisPNGConverter::*)(const QString &,
                                                                            const QRect &,
                                                                            qreal,
                                                                            qreal,
                                                                            KisPaintDeviceSP,
                                                                            vKisAnnotationSP_it,
                                                                            vKisAnnotationSP_it,
                                                                            KisPNGOptions,
                                                                            KisMetaData::Store *);
    using SaveDeviceToStore =
        bool (*)(const QString &, const QRect &, qreal, qreal, KisPaintDeviceSP, KoStore *, KisMetaData::Store *);

    ASSERT_PNG_CONVERTER_SIGNATURE(buildFile, BuildFileWithDevice);
    ASSERT_PNG_CONVERTER_SIGNATURE(buildFile, BuildFileWithName);
    ASSERT_PNG_CONVERTER_SIGNATURE(cancel, void (KisPNGConverter::*)());
    ASSERT_PNG_CONVERTER_SIGNATURE(isColorSpaceSupported, bool (*)(const KoColorSpace *));
    ASSERT_PNG_CONVERTER_SIGNATURE(saveDeviceToStore, SaveDeviceToStore);

    static_assert(std::is_same_v<decltype(KisPNGConverter::saveDeviceToStore(std::declval<const QString &>(),
                                                                             std::declval<const QRect &>(),
                                                                             std::declval<qreal>(),
                                                                             std::declval<qreal>(),
                                                                             std::declval<KisPaintDeviceSP>(),
                                                                             std::declval<KoStore *>())),
                                 bool>);
}

QTEST_APPLESS_MAIN(KisPNGConverterSchemaContractTest)

#include "KisPNGConverterSchemaContractTest.moc"

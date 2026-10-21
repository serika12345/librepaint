/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "TestKoLcmsColorProfile.h"

#include <cmath>
#include <ctime>
#include <lcms2.h>
#include <array>
#include <memory>
#include <QCryptographicHash>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QScopeGuard>
#include <QTime>
#include <QtEndian>

#include <KoColor.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <simpletest.h>
#include "../colorprofiles/BuiltinLcmsProfile_p.h"
#include "../colorprofiles/IccColorProfile.h"
#include "../colorprofiles/LcmsColorProfileContainer.h"

void TestKoLcmsColorProfile::builtInDefinitionsKeepCanonicalBytesColorsAndImportedProfiles_data()
{
    QTest::addColumn<QString>("kind");
    QTest::newRow("sRGB") << QStringLiteral("rgb");
    QTest::newRow("Lab") << QStringLiteral("lab");
    QTest::newRow("Gray") << QStringLiteral("gray");
    QTest::newRow("XYZ") << QStringLiteral("xyz");
}

void TestKoLcmsColorProfile::builtInDefinitionsKeepCanonicalBytesColorsAndImportedProfiles()
{
    QFETCH(QString, kind);
    cmsHPROFILE native = nullptr;
    if (kind == QStringLiteral("rgb")) native = cmsCreate_sRGBProfile();
    else if (kind == QStringLiteral("lab")) native = cmsCreateLab4Profile(nullptr);
    else if (kind == QStringLiteral("xyz")) native = cmsCreateXYZProfile();
    else {
        auto curve = cmsBuildGamma(nullptr, 2.2);
        QVERIFY(curve);
        native = cmsCreateGrayProfile(cmsD50_xyY(), curve);
        cmsFreeToneCurve(curve);
    }
    QVERIFY(native);
    const auto closeNative = qScopeGuard([&] { cmsCloseProfile(native); });
    cmsUInt32Number size = 0;
    QVERIFY(cmsSaveProfileToMem(native, nullptr, &size));
    QByteArray definition(int(size), '\0');
    QVERIFY(cmsSaveProfileToMem(native, definition.data(), &size));
    const std::array<quint16, 6> firstDate{2001, 2, 3, 4, 5, 6}, secondDate{2021, 7, 8, 9, 10, 11};
    const auto withMetadata = [&](const auto &date, quint32 platform) {
        QByteArray bytes = definition;
        for (size_t i = 0; i < date.size(); ++i) qToBigEndian(date[i], bytes.data() + 24 + i * 2);
        qToBigEndian(platform, bytes.data() + 40);
        return bytes;
    };
    const auto firstDefinition = withMetadata(firstDate, quint32(0x4150504c));
    const auto secondDefinition = withMetadata(secondDate, quint32(0x4d534654));
    const auto open = [](const QByteArray &bytes) {
        return cmsOpenProfileFromMem(bytes.constData(), cmsUInt32Number(bytes.size()));
    };
    std::unique_ptr<IccColorProfile> first(createBuiltinLcmsProfile(open(firstDefinition)));
    std::unique_ptr<IccColorProfile> second(createBuiltinLcmsProfile(open(secondDefinition)));
    QVERIFY(first && first->valid());
    QVERIFY(second && second->valid());
    const auto bytes = first->rawData();
    QCOMPARE(bytes, second->rawData());
    QCOMPARE(first->uniqueId(), second->uniqueId());
    QCOMPARE(qFromBigEndian<quint32>(bytes.constData() + 40), quint32(0));
    QByteArray checksum = bytes;
    checksum.replace(44, 4, QByteArray(4, '\0'));
    checksum.replace(64, 4, QByteArray(4, '\0'));
    checksum.replace(84, 16, QByteArray(16, '\0'));
    // ICC.1:2022 section 7.2.18 defines the identity embedded in exported profiles.
    QCOMPARE(bytes.mid(84, 16), QCryptographicHash::hash(checksum, QCryptographicHash::Md5));
    QCOMPARE(first->uniqueId(), bytes.mid(84, 16));

    cmsHPROFILE canonical = open(bytes);
    QVERIFY(canonical);
    const auto closeCanonical = qScopeGuard([&] { cmsCloseProfile(canonical); });
    std::tm created{};
    QVERIFY(cmsGetHeaderCreationDateTime(canonical, &created));
    QVERIFY(QDate(created.tm_year + 1900, created.tm_mon + 1, created.tm_mday).isValid());
    QVERIFY(QTime(created.tm_hour, created.tm_min, created.tm_sec).isValid());
    QCOMPARE(cmsGetTagCount(canonical), cmsGetTagCount(native));
    for (cmsInt32Number i = 0; i < cmsGetTagCount(native); ++i) {
        const auto tag = cmsGetTagSignature(native, i);
        const auto length = cmsReadRawTag(native, tag, nullptr, 0);
        QByteArray original(int(length), '\0'), actual(int(length), '\0');
        QCOMPARE(cmsReadRawTag(native, tag, original.data(), length), length);
        QCOMPARE(cmsReadRawTag(canonical, tag, actual.data(), length), length);
        QCOMPARE(actual, original);
    }
    cmsHPROFILE source = cmsCreate_sRGBProfile();
    QVERIFY(source);
    const auto closeSource = qScopeGuard([&] { cmsCloseProfile(source); });
    const auto format = cmsFormatterForColorspaceOfProfile(native, 2, false);
    QVERIFY(format);
    // The existing constructor consumes an encoded ICC definition, including
    // the channel representation precision retained in its tags.
    std::unique_ptr<IccColorProfile> encoded(LcmsColorProfileContainer::createFromLcmsProfile(open(firstDefinition)));
    QVERIFY(encoded->valid());
    const auto encodedBytes = encoded->rawData();
    cmsHPROFILE reference = open(encodedBytes);
    QVERIFY(reference);
    const auto closeReference = qScopeGuard([&] { cmsCloseProfile(reference); });
    auto originalTransform = cmsCreateTransform(source, TYPE_RGB_16, reference, format,
        INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_NOOPTIMIZE);
    auto actualTransform = cmsCreateTransform(source, TYPE_RGB_16, canonical, format,
        INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_NOOPTIMIZE);
    const auto closeTransforms = qScopeGuard([&] {
        if (originalTransform) cmsDeleteTransform(originalTransform);
        if (actualTransform) cmsDeleteTransform(actualTransform);
    });
    QVERIFY(originalTransform && actualTransform);
    const std::array<quint16, 9> samples{0, 0, 0, 16384, 32768, 49152, 65535, 65535, 65535};
    std::array<quint16, 9> originalPixels{}, actualPixels{};
    cmsDoTransform(originalTransform, samples.data(), originalPixels.data(), 3);
    cmsDoTransform(actualTransform, samples.data(), actualPixels.data(), 3);
    QCOMPARE(actualPixels, originalPixels);

    // Imported definitions keep their own creation metadata and complete payload.
    IccColorProfile imported(firstDefinition);
    QVERIFY(imported.valid());
    QCOMPARE(imported.rawData(), firstDefinition);
    std::unique_ptr<IccColorProfile> fromNative(LcmsColorProfileContainer::createFromLcmsProfile(open(secondDefinition)));
    QVERIFY(fromNative->valid());
    QCOMPARE(fromNative->rawData().mid(24, 12), secondDefinition.mid(24, 12));
    if (kind == QStringLiteral("rgb")) {
        auto *registry = KoColorSpaceRegistry::instance();
        const auto builtin = registry->rgb8("sRGB built-in")->profile()->rawData();
        const auto importedProfile = registry->createColorProfileFromData("RGBA", "U8", firstDefinition);
        QVERIFY(importedProfile);
        const auto space = registry->colorSpace("RGBA", "U8", importedProfile);
        QVERIFY(space);
        QCOMPARE(space->profile()->rawData(), firstDefinition);
        QCOMPARE(registry->rgb8("sRGB built-in")->profile()->rawData(), builtin);
    }
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        for (const auto &entry : {qMakePair(QStringLiteral("builtin-") + kind, bytes),
                                  qMakePair(QStringLiteral("imported-") + kind, firstDefinition)}) {
            QFile file(QDir(directory).filePath(entry.first + QStringLiteral(".icc")));
            QVERIFY(file.open(QIODevice::WriteOnly));
            QCOMPARE(file.write(entry.second), qint64(entry.second.size()));
        }
    }
}

qreal testRounding(qreal value)
{
    qreal factor;
    int temp;
    const int numPlaces = 3;

    factor = pow(10.0, numPlaces);
    temp = (int)(value * factor + 0.5);
    return temp / factor;
}

void TestKoLcmsColorProfile::testConversion()
{
    const KoColorSpace *sRgb = KoColorSpaceRegistry::instance()->rgb16("sRGB built-in");
    Q_ASSERT(sRgb);
    const KoColorSpace *linearRgb = KoColorSpaceRegistry::instance()->rgb16("scRGB (linear)");
    Q_ASSERT(linearRgb);

    quint16 src[4];
    src[0] = 257;
    src[1] = 257;
    src[2] = 257;
    src[3] = 65535;

    quint16 dst[4];
    memset(&dst, 0, 8);

    linearRgb->convertPixelsTo((quint8 *)&src, (quint8 *)&dst, sRgb, 1, KoColorConversionTransformation::IntentRelativeColorimetric, KoColorConversionTransformation::BlackpointCompensation);

    quint16 dst2[4];
    memset(&dst2, 0, 8);

    cmsHPROFILE sRgbProfile = cmsCreate_sRGBProfile();
    QByteArray rawData = linearRgb->profile()->rawData();
    cmsHPROFILE linearRgbProfile = cmsOpenProfileFromMem((void *)rawData.constData(), rawData.size());

    cmsHTRANSFORM tf = cmsCreateTransform(linearRgbProfile,
                                          TYPE_BGRA_16,
                                          sRgbProfile,
                                          TYPE_BGRA_16,
                                          INTENT_RELATIVE_COLORIMETRIC,
                                          cmsFLAGS_NOOPTIMIZE);

    cmsDoTransform(tf, (quint8 *)&src, (quint8 *)&dst2, 1);

    Q_ASSERT(dst[0] == dst2[0]);

}

void TestKoLcmsColorProfile::testProofingConversion()
{
    const KoColorSpace *sRgb = KoColorSpaceRegistry::instance()->rgb16("sRGB built-in");
    Q_ASSERT(sRgb);
    const KoColorSpace *lab = KoColorSpaceRegistry::instance()->lab16();//there's only one lab profile, replace with it's name.
    Q_ASSERT(lab);

    quint16 src[4];//the following ought to give us a purple only possible in lab. I can't seem to proof this away, somehow...
    src[0] = 32896;
    src[1] = 65535;
    src[2] = 0;
    src[3] = 65535;

    quint16 dst[4];
    memset(&dst, 0, 8);

    cmsHPROFILE sRgbProfile = cmsCreate_sRGBProfile();
    cmsHPROFILE LabProfile = cmsCreateLab4Profile(NULL);

    quint16 alarm[cmsMAXCHANNELS]={0};
    alarm[0] = 65535;
    alarm[1] = 0;
    alarm[2] = 0;
    alarm[3] = 65535;
    cmsSetAlarmCodes(alarm);

    cmsHTRANSFORM tf = cmsCreateProofingTransform(LabProfile,
                                          TYPE_Lab_16,
                                          LabProfile,
                                          TYPE_Lab_16,
                                          sRgbProfile,
                                          INTENT_ABSOLUTE_COLORIMETRIC,
                                          INTENT_ABSOLUTE_COLORIMETRIC,
                                          cmsFLAGS_SOFTPROOFING|cmsFLAGS_GAMUTCHECK);

    cmsDoTransform(tf, (quint8 *)&src, (quint8 *)&dst, 1);

    qDebug()<<dst[0]<<","<<dst[1]<<","<<dst[2]<<","<<dst[3];
    Q_ASSERT((dst[0] == alarm[0]) && (dst[1] == alarm[1]) && (dst[2] == alarm[2]));

}
SIMPLE_TEST_MAIN(TestKoLcmsColorProfile)

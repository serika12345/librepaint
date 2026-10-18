/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "TestColorSpaceRegistry.h"

#include <lcms2.h>
#include <QColorSpace>
#include <QScopeGuard>
#include <QTemporaryFile>
#include <array>
#include <memory>

#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <simpletest.h>
#include <testpigment.h>

#include <kis_debug.h>
#include <qimage_test_util.h>
#include <KoColorSpaceEngine.h>
#include <KisAdaptedLock.h>

namespace {
QByteArray renamedProfileData(const QByteArray &original, const QByteArray &name)
{
    cmsHPROFILE profile = cmsOpenProfileFromMem(original.constData(), original.size());
    if (!profile) return QByteArray();
    auto closeProfile = qScopeGuard([&] { cmsCloseProfile(profile); });
    cmsMLU *description = cmsMLUalloc(nullptr, 1);
    if (!description) return QByteArray();
    auto freeDescription = qScopeGuard([&] { cmsMLUfree(description); });
    if (!cmsMLUsetASCII(description, "en", "US", name.constData()) ||
        !cmsWriteTag(profile, cmsSigProfileDescriptionTag, description)) {
        return QByteArray();
    }
    cmsUInt32Number size = 0;
    if (!cmsSaveProfileToMem(profile, nullptr, &size)) return QByteArray();
    QByteArray data(size, '\0');
    if (!cmsSaveProfileToMem(profile, data.data(), &size)) return QByteArray();
    data.replace(24, 12, QByteArray::fromHex("07ea00010001000000000000"));
    data.replace(84, 16, QByteArray(16, '\0'));
    cmsHPROFILE dated = cmsOpenProfileFromMem(data.constData(), data.size());
    if (!dated) return QByteArray();
    auto closeDated = qScopeGuard([&] { cmsCloseProfile(dated); });
    if (!cmsMD5computeID(dated) || !cmsSaveProfileToMem(dated, data.data(), &size)) return QByteArray();
    return data;
}
}

void TestColorSpaceRegistry::testConstruction()
{
    KoColorSpaceRegistry *instance = KoColorSpaceRegistry::instance();
    Q_ASSERT(instance);
}

void TestColorSpaceRegistry::testRgbU8()
{
    const QString colorSpaceId = KoColorSpaceRegistry::instance()->colorSpaceId(RGBAColorModelID,
                                                                                Integer8BitsColorDepthID);

    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb8();
    QVERIFY(colorSpace != 0);

    const KoColorProfile *profile = colorSpace->profile();
    QVERIFY(profile != 0);

    QCOMPARE(profile->name(), KoColorSpaceRegistry::instance()->defaultProfileForColorSpace(colorSpaceId));

    cmsHPROFILE lcmsProfile = cmsCreate_sRGBProfile();
    QString testProfileName = "TestRGBU8ProfileName";

    cmsWriteTag(lcmsProfile, cmsSigProfileDescriptionTag, testProfileName.toLatin1().constData());
    cmsWriteTag(lcmsProfile, cmsSigDeviceModelDescTag, testProfileName.toLatin1().constData());
    cmsWriteTag(lcmsProfile, cmsSigDeviceMfgDescTag, "");

}

void TestColorSpaceRegistry::testRgbU16()
{
    const QString colorSpaceId = KoColorSpaceRegistry::instance()->colorSpaceId(RGBAColorModelID,
                                                                                Integer16BitsColorDepthID);

    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->rgb16();
    QVERIFY(colorSpace != 0);

    const KoColorProfile *profile = colorSpace->profile();
    QVERIFY(profile != 0);

    QCOMPARE(profile->name(), KoColorSpaceRegistry::instance()->defaultProfileForColorSpace(colorSpaceId));

    cmsHPROFILE lcmsProfile = cmsCreate_sRGBProfile();
    QString testProfileName = "TestRGBU16ProfileName";

    cmsWriteTag(lcmsProfile, cmsSigProfileDescriptionTag, testProfileName.toLatin1().constData());
    cmsWriteTag(lcmsProfile, cmsSigDeviceModelDescTag, testProfileName.toLatin1().constData());
    cmsWriteTag(lcmsProfile, cmsSigDeviceMfgDescTag, "");

}

void TestColorSpaceRegistry::testLab()
{
    const QString colorSpaceId = KoColorSpaceRegistry::instance()->colorSpaceId(LABAColorModelID,
                                                                                Integer16BitsColorDepthID);

    const KoColorSpace *colorSpace = KoColorSpaceRegistry::instance()->lab16();
    QVERIFY(colorSpace != 0);

    const KoColorProfile *profile = colorSpace->profile();
    QVERIFY(profile != 0);

    QCOMPARE(profile->name(), KoColorSpaceRegistry::instance()->defaultProfileForColorSpace(colorSpaceId));

    cmsCIExyY whitepoint;
    whitepoint.x = 0.33;
    whitepoint.y = 0.33;
    whitepoint.Y = 1.0;

    cmsHPROFILE lcmsProfile = cmsCreateLab4Profile(&whitepoint);
    QString testProfileName = "TestLabProfileName";

    cmsWriteTag(lcmsProfile, cmsSigProfileDescriptionTag, testProfileName.toLatin1().constData());
    cmsWriteTag(lcmsProfile, cmsSigDeviceModelDescTag, testProfileName.toLatin1().constData());
    cmsWriteTag(lcmsProfile, cmsSigDeviceMfgDescTag, "");

}

void TestColorSpaceRegistry::testColorProfileAliasAddUnique()
{
    auto *registry = KoColorSpaceRegistry::instance();

    const QString aliasSource = "test_alias.icc";
    const QString aliasDestination = registry->p2020G10Profile()->name();

    registry->addProfileAlias(aliasSource, aliasDestination);

    {
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(profile);
        QCOMPARE(profile->name(), aliasDestination);
        for (int request = 0; request < 2; ++request) {
            const KoColorSpace *space = registry->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), aliasSource);
            QVERIFY(space);
            QCOMPARE(space->profile()->rawData(), profile->rawData());
        }
    }

    registry->removeProfileAlias(aliasSource);

    {
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(!profile);
    }

}

void TestColorSpaceRegistry::testColorProfileAliasAddWhenProfileExists()
{
    auto *registry = KoColorSpaceRegistry::instance();

    KoColorProfile *originalProfile = const_cast<KoColorProfile*>(registry->p709G10Profile());

    const QString aliasSource = originalProfile->name();
    const QString aliasDestination = registry->p2020G10Profile()->name();
    KIS_ASSERT(aliasSource != aliasDestination);

    // add alias
    registry->addProfileAlias(aliasSource, aliasDestination);

    {
        /**
         * The alias is added but it takes no effect, since the actual profile
         * with this name already exists
         */
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(profile);
        QCOMPARE(profile->name(), aliasSource);
    }

    // now remove the profile
    registry->removeProfile(originalProfile);

    {
        /**
         * The alias is active now!
         */
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(profile);
        QCOMPARE(profile->name(), aliasDestination);
    }

    // now remove the alias
    registry->removeProfileAlias(aliasSource);

    {
        /**
         * Nothing is found now. We have removed both, the profile and the alias
         */
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(!profile);
    }

    // restore the profile
    registry->addProfile(originalProfile);
    {
        // no alias is present now
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(profile);
        QCOMPARE(profile->name(), aliasSource);
    }
}

namespace {
struct AliasRemovalAdapter
{
    AliasRemovalAdapter(const QString &aliasSource)
        : m_aliasSource(aliasSource)
    {
        auto *registry = KoColorSpaceRegistry::instance();
        m_originalAliasTarget = registry->profileAlias(aliasSource);
    }

    inline void lock() {
        if (m_originalAliasTarget != m_aliasSource) {
            auto *registry = KoColorSpaceRegistry::instance();
            registry->removeProfileAlias(m_aliasSource);
        }
    }

    inline void unlock() {
        if (m_originalAliasTarget != m_aliasSource) {
            auto *registry = KoColorSpaceRegistry::instance();
            registry->addProfileAlias(m_aliasSource, m_originalAliasTarget);
        }
    }

private:
    QString m_aliasSource;
    QString m_originalAliasTarget;
};

KIS_DECLARE_ADAPTED_LOCK(AliasRemovalLock, AliasRemovalAdapter)
}

void TestColorSpaceRegistry::testColorProfileAliasOverrideByProfile()
{
    /**
     * The profile aliases are considered 'weak', i.e. when a real
     * profile with the same name is loaded, the alias gets overridden.
     *
     * When this explicit profile is unloaded, the alias is expected
     * to be re-enabled again.
     */
    auto *registry = KoColorSpaceRegistry::instance();
    const QString legacyProfileName = "High Dynamic Range UHDTV Wide Color Gamut Display (Rec. 2020) - SMPTE ST 2084 PQ EOTF";
    const QString legacyProfileFileName = "ITUR_2100_PQ_FULL.ICC";

    AliasRemovalLock lock(legacyProfileName);

    QVERIFY(!registry->profileByName(legacyProfileName));

    const QString aliasSource = legacyProfileName;
    const QString aliasDestination = registry->p2020PQProfile()->name();
    KIS_ASSERT(aliasSource != aliasDestination);

    registry->addProfileAlias(legacyProfileName, aliasDestination);

    {
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(profile);
        QCOMPARE(profile->name(), aliasDestination);
    }

    {
        const QString profileFilePath = TestUtil::fetchDataFileLazy(legacyProfileFileName);
        KIS_ASSERT(QFile::exists(profileFilePath));

        {
            KoColorSpaceEngine *iccEngine = KoColorSpaceEngineRegistry::instance()->get("icc");
            KIS_ASSERT(iccEngine);
            iccEngine->addProfile(profileFilePath);
        }
    }

    {
        // the profile has overridden the alias
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(profile);
        QCOMPARE(profile->name(), aliasSource);
    }

    registry->removeProfile(const_cast<KoColorProfile*>(registry->profileByName(aliasSource)));

    {
        // alias is active again
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(profile);
        QCOMPARE(profile->name(), aliasDestination);
    }

    registry->removeProfileAlias(aliasSource);

    {
        // no alias exists anymore, hence no profile
        auto *profile = registry->profileByName(aliasSource);
        QVERIFY(!profile);
    }
}

void TestColorSpaceRegistry::testExplicitSameNameProfiles_data()
{
    QTest::addColumn<bool>("reuseDeclaredId");
    QTest::newRow("different-content") << false;
    QTest::newRow("same-declared-id") << true;
}

void TestColorSpaceRegistry::testExplicitSameNameProfiles()
{
    QFETCH(bool, reuseDeclaredId);

    // Real Qt ICC profiles with different primaries share a display name.
    // Fix the date and compute the ID after changing the description.
    const QByteArray name = QByteArray("LibrePaint explicit ICC contract ") + QTest::currentDataTag();
    const QByteArray firstData = renamedProfileData(QColorSpace(QColorSpace::SRgb).iccProfile(), name);
    QByteArray secondData = renamedProfileData(QColorSpace(QColorSpace::DisplayP3).iccProfile(), name);
    QVERIFY(!firstData.isEmpty());
    QVERIFY(!secondData.isEmpty());
    if (reuseDeclaredId) secondData.replace(84, 16, firstData.mid(84, 16));
    QVERIFY(firstData != secondData);

    auto *registry = KoColorSpaceRegistry::instance();
    auto *engine = KoColorSpaceEngineRegistry::instance()->get("icc");
    QVERIFY(engine);
    const KoColorProfile *firstProfile = engine->addProfile(firstData);
    QVERIFY(firstProfile);
    const KoColorSpace *firstSpace = registry->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), firstProfile);
    QVERIFY(firstSpace);
    QCOMPARE(firstSpace->profile()->rawData(), firstData);

    // The existing name-based creation API deliberately reuses registered names.
    const KoColorProfile *legacyProfile = registry->createColorProfile(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), secondData);
    QVERIFY(legacyProfile);
    QCOMPARE(legacyProfile->rawData(), firstData);

    QVERIFY(!registry->createColorProfileFromData(QStringLiteral("unsupported"), Integer8BitsColorDepthID.id(), secondData));
    QVERIFY(!registry->createColorProfileFromData(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), QByteArray("invalid ICC")));
    const KoColorProfile *embedded = registry->createColorProfileFromData(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), secondData);
    QVERIFY(embedded);
    QCOMPARE(embedded->rawData(), secondData);
    QCOMPARE(registry->profileByName(firstProfile->name())->rawData(), firstData);
    QCOMPARE(registry->createColorProfileFromData(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), firstData), firstProfile);
    QCOMPARE(registry->createColorProfileFromData(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), secondData), embedded);

    const KoColorProfile *secondProfile = engine->addProfile(secondData);
    QVERIFY(secondProfile);
    QCOMPARE(secondProfile->name(), firstProfile->name());
    const KoColorSpace *secondSpace = registry->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), secondProfile);
    QVERIFY(secondSpace);
    QCOMPARE(secondSpace->profile()->rawData(), secondData);
    QCOMPARE(firstSpace->profile()->rawData(), firstData);
    QCOMPARE(registry->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), firstProfile)->profile()->rawData(), firstData);
    QCOMPARE(registry->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), secondProfile->name())->profile()->rawData(), secondData);

    cmsHPROFILE first = cmsOpenProfileFromMem(firstData.constData(), firstData.size());
    cmsHPROFILE second = cmsOpenProfileFromMem(secondData.constData(), secondData.size());
    QVERIFY(first);
    QVERIFY(second);
    auto closeProfiles = qScopeGuard([&] { cmsCloseProfile(first); cmsCloseProfile(second); });
    cmsHTRANSFORM transform = cmsCreateTransform(first, TYPE_BGRA_8, second, TYPE_BGRA_8,
                                                INTENT_RELATIVE_COLORIMETRIC,
                                                cmsFLAGS_NOOPTIMIZE | cmsFLAGS_COPY_ALPHA);
    QVERIFY(transform);
    auto deleteTransform = qScopeGuard([&] { cmsDeleteTransform(transform); });
    const std::array<quint8, 4> pixel {40, 90, 200, 255};
    std::array<quint8, 4> expected {}, actual {};
    cmsDoTransform(transform, pixel.data(), expected.data(), 1);
    QVERIFY(expected != pixel);
    QVERIFY(firstSpace->convertPixelsTo(pixel.data(), actual.data(), secondSpace, 1,
                                       KoColorConversionTransformation::IntentRelativeColorimetric,
                                       KoColorConversionTransformation::NoOptimization |
                                           KoColorConversionTransformation::CopyAlpha));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(actual.data()), actual.size()),
             QByteArray(reinterpret_cast<const char*>(expected.data()), expected.size()));

    // Re-registering identical data must leave both existing spaces usable.
    QVERIFY(engine->addProfile(firstData));
    QCOMPARE(firstSpace->profile()->rawData(), firstData);
    QCOMPARE(secondSpace->profile()->rawData(), secondData);
    actual.fill(0);
    QVERIFY(firstSpace->convertPixelsTo(pixel.data(), actual.data(), secondSpace, 1,
                                       KoColorConversionTransformation::IntentRelativeColorimetric,
                                       KoColorConversionTransformation::NoOptimization |
                                           KoColorConversionTransformation::CopyAlpha));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(actual.data()), actual.size()),
             QByteArray(reinterpret_cast<const char*>(expected.data()), expected.size()));

    // Removing an older same-name registration preserves the current named
    // profile and the spaces already retained by open documents.
    registry->removeProfile(const_cast<KoColorProfile*>(secondProfile));
    delete secondProfile;
    const KoColorProfile *current = registry->profileByName(QString::fromLatin1(name));
    QVERIFY(current);
    QCOMPARE(current->rawData(), firstData);
    QCOMPARE(secondSpace->profile()->rawData(), secondData);
    cmsHTRANSFORM reverse = cmsCreateTransform(second, TYPE_BGRA_8, first, TYPE_BGRA_8,
                                              INTENT_RELATIVE_COLORIMETRIC,
                                              cmsFLAGS_NOOPTIMIZE | cmsFLAGS_COPY_ALPHA);
    QVERIFY(reverse);
    auto deleteReverse = qScopeGuard([&] { cmsDeleteTransform(reverse); });
    cmsDoTransform(reverse, pixel.data(), expected.data(), 1);
    actual.fill(0);
    QVERIFY(secondSpace->convertPixelsTo(pixel.data(), actual.data(), firstSpace, 1,
                                        KoColorConversionTransformation::IntentRelativeColorimetric,
                                        KoColorConversionTransformation::NoOptimization |
                                            KoColorConversionTransformation::CopyAlpha));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(actual.data()), actual.size()),
             QByteArray(reinterpret_cast<const char*>(expected.data()), expected.size()));

    const KoColorProfile *retained = registry->createColorProfileFromData(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), secondData);
    QVERIFY(retained);
    std::unique_ptr<KoColorProfile> removalQuery(retained->clone());
    registry->removeProfile(removalQuery.get());
    QCOMPARE(retained->rawData(), secondData);
    QCOMPARE(registry->profileByName(current->name())->rawData(), firstData);
    const KoColorProfile *reloaded = registry->createColorProfileFromData(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), secondData);
    QVERIFY(reloaded);
    QCOMPARE(reloaded->rawData(), secondData);

    QTemporaryFile profileFile;
    QVERIFY(profileFile.open());
    QCOMPARE(profileFile.write(secondData), secondData.size());
    QVERIFY(profileFile.flush());
    const QString profilePath = profileFile.fileName();
    profileFile.close();
    engine->removeProfile(profilePath);
    QCOMPARE(registry->profileByName(current->name())->rawData(), firstData);
    QCOMPARE(secondSpace->profile()->rawData(), secondData);
    const KoColorProfile *idBefore = registry->profileByUniqueId(secondSpace->profile()->uniqueId());
    QVERIFY(!registry->colorSpace(GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), secondSpace->profile()));
    QCOMPARE(registry->profileByUniqueId(secondSpace->profile()->uniqueId()), idBefore);
    const KoColorSpace *retainedSpace = registry->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), secondSpace->profile());
    QVERIFY(retainedSpace);
    QCOMPARE(retainedSpace->profile()->rawData(), secondData);
    const KoColorSpace *retained16 = registry->colorSpace(RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), secondSpace->profile());
    QVERIFY(retained16);
    QCOMPARE(retained16->profile()->rawData(), secondData);
    QCOMPARE(registry->profileByName(current->name())->rawData(), firstData);
}

void TestColorSpaceRegistry::testEmbeddedPqWithLinearProfileName()
{
    auto *registry = KoColorSpaceRegistry::instance();
    const KoColorProfile *linear = registry->p2020G10Profile();
    const KoColorProfile *pq = registry->p2020PQProfile();
    QVERIFY(linear);
    QVERIFY(pq);
    const KoColorSpace *linearSpace = registry->colorSpace(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), linear);
    const KoColorSpace *pqSpace = registry->colorSpace(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), pq);
    QVERIFY(linearSpace);
    QVERIFY(pqSpace);
    const QByteArray embeddedData = renamedProfileData(pq->rawData(), linear->name().toLatin1());
    QVERIFY(!embeddedData.isEmpty());
    const KoColorProfile *embedded = registry->createColorProfileFromData(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), embeddedData);
    QVERIFY(embedded);
    QCOMPARE(embedded->rawData(), embeddedData);
    QCOMPARE(embedded->name(), linear->name());
    QCOMPARE(embedded->getTransferCharacteristics(), pq->getTransferCharacteristics());
    QCOMPARE(embedded->hdrReferenceWhite().value_or(203.0), pq->hdrReferenceWhite().value_or(203.0));
    QCOMPARE(registry->profileByName(linear->name())->rawData(), linear->rawData());
    const KoColorSpace *embeddedSpace = registry->colorSpace(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), embedded);
    QVERIFY(embeddedSpace);
    const std::array<float, 4> pixel {0.2f, 0.5f, 0.7f, 1.0f};
    std::array<float, 4> expected {}, actual {};
    QVERIFY(pqSpace->convertPixelsTo(reinterpret_cast<const quint8*>(pixel.data()), reinterpret_cast<quint8*>(expected.data()), linearSpace, 1,
                                   KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags()));
    QVERIFY(embeddedSpace->convertPixelsTo(reinterpret_cast<const quint8*>(pixel.data()), reinterpret_cast<quint8*>(actual.data()), linearSpace, 1,
                                         KoColorConversionTransformation::internalRenderingIntent(), KoColorConversionTransformation::internalConversionFlags()));
    QCOMPARE(QByteArray(reinterpret_cast<const char*>(actual.data()), sizeof(actual)),
             QByteArray(reinterpret_cast<const char*>(expected.data()), sizeof(expected)));
}

void TestColorSpaceRegistry::testEmbeddedProfileRejection_data()
{
    QTest::addColumn<QString>("model");
    QTest::addColumn<QByteArray>("data");
    const QByteArray rgb = QColorSpace(QColorSpace::SRgb).iccProfile();
    QTest::newRow("unsupported-model") << QStringLiteral("unsupported") << rgb;
    QTest::newRow("invalid-icc") << RGBAColorModelID.id() << QByteArray("invalid ICC");
    QTest::newRow("empty-icc") << RGBAColorModelID.id() << QByteArray();
    QTest::newRow("rgb-in-gray") << GrayAColorModelID.id() << rgb;
    QTest::newRow("ignored-raw-data") << AlphaColorModelID.id() << rgb;
}

void TestColorSpaceRegistry::testEmbeddedProfileRejection()
{
    QFETCH(QString, model);
    QFETCH(QByteArray, data);
    auto *registry = KoColorSpaceRegistry::instance();
    auto *engine = KoColorSpaceEngineRegistry::instance()->get("icc");
    QVERIFY(engine);
    const QByteArray originalData = QColorSpace(QColorSpace::SRgb).iccProfile();
    const KoColorProfile *original = engine->addProfile(originalData);
    QVERIFY(original);
    const KoColorSpace *space = registry->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), original);
    QVERIFY(space);

    QVERIFY(!registry->createColorProfileFromData(model, Integer8BitsColorDepthID.id(), data));
    QCOMPARE(registry->profileByName(original->name()), original);
    QCOMPARE(space->profile()->rawData(), originalData);
}

KISTEST_MAIN(TestColorSpaceRegistry)

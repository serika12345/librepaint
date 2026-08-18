/*
 *  SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "TestColorConversionSystem.h"

#include <simpletest.h>

#include <DebugPigment.h>
#include <KoColorConversionSystem.h>
#include <KoColorModelStandardIds.h>
#include <KoColorProfile.h>
#include <KoColorProfileQuery.h>
#include <KoColorSpaceRegistry.h>
#include <testpigment.h>

#include <kis_debug.h>
#include <qimage_test_util.h>

#include <QRandomGenerator>

TestColorConversionSystem::TestColorConversionSystem()
{
    Q_FOREACH (const KoID& modelId, KoColorSpaceRegistry::instance()->colorModelsList(KoColorSpaceRegistry::AllColorSpaces)) {
        Q_FOREACH (const KoID& depthId, KoColorSpaceRegistry::instance()->colorDepthList(modelId, KoColorSpaceRegistry::AllColorSpaces)) {
            QList< const KoColorProfile * > profiles =
                KoColorSpaceRegistry::instance()->profilesFor(
                    KoColorSpaceRegistry::instance()->colorSpaceId(modelId, depthId));
            Q_FOREACH (const KoColorProfile * profile, profiles) {
                listModels.append(ModelDepthProfile(modelId.id(), depthId.id(), profile->name()));
            }
        }
    }
    //listModels.append(ModelDepthProfile(AlphaColorModelID.id(), Integer8BitsColorDepthID.id(), ""));
}

void TestColorConversionSystem::testConnections()
{
    Q_FOREACH (const ModelDepthProfile& srcCS, listModels) {
        Q_FOREACH (const ModelDepthProfile& dstCS, listModels) {
            QVERIFY2(KoColorSpaceRegistry::instance()->colorConversionSystem()->existsPath(srcCS.model, srcCS.depth, srcCS.profile, dstCS.model, dstCS.depth, dstCS.profile) , QString("No path between %1 / %2 and %3 / %4").arg(srcCS.model).arg(srcCS.depth).arg(dstCS.model).arg(dstCS.depth).toLatin1());
        }
    }
}

void TestColorConversionSystem::testGoodConnections()
{
    int countFail = 0;
    Q_FOREACH (const ModelDepthProfile& srcCS, listModels) {
        Q_FOREACH (const ModelDepthProfile& dstCS, listModels) {
            if (!KoColorSpaceRegistry::instance()->colorConversionSystem()->existsGoodPath(srcCS.model, srcCS.depth, srcCS.profile , dstCS.model, dstCS.depth, dstCS.profile)) {
                ++countFail;
                dbgPigment << "No good path between \"" << srcCS.model << " " << srcCS.depth << " " << srcCS.profile << "\" \"" << dstCS.model << " " << dstCS.depth << " " << dstCS.profile << "\"";
            }
        }
    }
    int failed = 0;
    if (!KoColorSpaceRegistry::instance()->colorSpace( RGBAColorModelID.id(), Float32BitsColorDepthID.id(), 0) && KoColorSpaceRegistry::instance()->colorSpace( "KS6", Float32BitsColorDepthID.id(), 0) ) {
        failed = 42;
    }
    QVERIFY2(countFail == failed, QString("%1 tests have fails (it should have been %2)").arg(countFail).arg(failed).toLatin1());
}

#include <KoColor.h>

#include <KoColorConversionSystem_p.h>
#include <kis_debug.h>

namespace QTest {
inline bool qCompare(const std::vector<KoColorConversionSystem::NodeKey> &t1,
                     const std::vector<KoColorConversionSystem::NodeKey> &t2,
                     const char *actual, const char *expected,
                     const char *file, int line) {

    bool result = t1 == t2;

    if (!result) {
        QString actualStr;
        QDebug act(&actualStr);
        act.nospace() << actual << ": " << t1;

        QString expectedStr;
        QDebug exp(&expectedStr);
        exp.nospace() << expected << ": " << t2;

        QString message = QString("Compared paths are not the same:\n Expected: %1\n Actual: %2").arg(expectedStr).arg(actualStr);
        QTest::qFail(message.toLocal8Bit(), file, line);
    }

    return t1 == t2;
}
}

std::vector<KoColorConversionSystem::NodeKey> TestColorConversionSystem::calcPath(const std::vector<KoColorConversionSystem::NodeKey> &expectedPath, bool skipEngineNodes) {

    const KoColorConversionSystem *system = KoColorSpaceRegistry::instance()->colorConversionSystem();

    KoColorConversionSystem::Path path =
        system->findBestPath(expectedPath.front(), expectedPath.back());

    std::vector<KoColorConversionSystem::NodeKey> realPath;

    Q_FOREACH (const KoColorConversionSystem::Vertex *vertex, path.vertexes) {
        if (!skipEngineNodes || !vertex->srcNode->isEngine) {
            realPath.push_back(vertex->srcNode->key());
        }
    }
    realPath.push_back(path.vertexes.last()->dstNode->key());

    return realPath;
};

void TestColorConversionSystem::testAlphaConnectionPaths()
{
    const KoColorSpace *alpha8 = KoColorSpaceRegistry::instance()->alpha8();

    using NodeKey = KoColorConversionSystem::NodeKey;

    std::vector<NodeKey> expectedPath;

       // to Alpha8 conversions. Everything should go via GrayA color space,
    // we expect alpha colorspace be just a flattened of graya color space
    // with srgb tone curve.

    expectedPath =
        {{GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

return;

    expectedPath =
        {{GrayAColorModelID.id(), Integer16BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

#ifdef HAVE_OPENEXR
    expectedPath =
        {{GrayAColorModelID.id(), Float16BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);
#endif

    expectedPath =
        {{GrayAColorModelID.id(), Float32BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    expectedPath =
        {{RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()},
         {GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    expectedPath =
        {{RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()},
         {GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    expectedPath =
        {{RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()},
         {GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {AlphaColorModelID.id(), Integer16BitsColorDepthID.id(), alpha8->profile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    expectedPath =
        {{RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()},
         {GrayAColorModelID.id(), Integer16BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {AlphaColorModelID.id(), Integer16BitsColorDepthID.id(), alpha8->profile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    // from Alpha8 conversions. Everything should go via GrayA color space

    expectedPath =
        {{alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()},
         {GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    expectedPath =
        {{alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()},
         {GrayAColorModelID.id(), Integer16BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

#ifdef HAVE_OPENEXR
    expectedPath =
        {{alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()},
         {GrayAColorModelID.id(), Float16BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"}};
    QCOMPARE(calcPath(expectedPath), expectedPath);
#endif

    expectedPath =
        {{alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()},
         {GrayAColorModelID.id(), Float32BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    expectedPath =
        {{alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()},
         {GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);


    expectedPath =
        {{alpha8->colorModelId().id(), alpha8->colorDepthId().id(), alpha8->profile()->name()},
         {GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    expectedPath =
        {{AlphaColorModelID.id(), Integer16BitsColorDepthID.id(), alpha8->profile()->name()},
         {GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    expectedPath =
        {{AlphaColorModelID.id(), Integer16BitsColorDepthID.id(), alpha8->profile()->name()},
         {GrayAColorModelID.id(), Integer16BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
         {RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);
}

void TestColorConversionSystem::testAlphaConversions()
{
    const KoColorSpace *alpha8 = KoColorSpaceRegistry::instance()->alpha8();
    const KoColorSpace *rgb8 = KoColorSpaceRegistry::instance()->rgb8();
    const KoColorSpace *rgb16 = KoColorSpaceRegistry::instance()->rgb16();

    {
        KoColor c(QColor(255,255,255,255), alpha8);
        QCOMPARE(c.opacityU8(), quint8(255));
        c.convertTo(rgb8);
        QCOMPARE(c.toQColor(), QColor(255,255,255));
        c.convertTo(alpha8);
        QCOMPARE(c.opacityU8(), quint8(255));
    }

    {
        KoColor c(QColor(255,255,255,0), alpha8);
        c.convertTo(rgb8);
        QCOMPARE(c.toQColor(), QColor(0,0,0,255));
        c.convertTo(alpha8);
        QCOMPARE(c.opacityU8(), quint8(0));
    }

    {
        KoColor c(QColor(255,255,255,128), alpha8);
        c.convertTo(rgb8);
        QCOMPARE(c.toQColor(), QColor(128,128,128,255));
        c.convertTo(alpha8);
        QCOMPARE(c.opacityU8(), quint8(128));
    }

    {
        KoColor c(QColor(255,255,255,255), alpha8);
        QCOMPARE(c.opacityU8(), quint8(255));
        c.convertTo(rgb16);
        QCOMPARE(c.toQColor(), QColor(255,255,255));
        c.convertTo(alpha8);
        QCOMPARE(c.opacityU8(), quint8(255));
    }

    {
        KoColor c(QColor(255,255,255,0), alpha8);
        c.convertTo(rgb16);
        QCOMPARE(c.toQColor(), QColor(0,0,0,255));
        c.convertTo(alpha8);
        QCOMPARE(c.opacityU8(), quint8(0));
    }

    {
        KoColor c(QColor(255,255,255,128), alpha8);
        c.convertTo(rgb16);
        QCOMPARE(c.toQColor(), QColor(128,128,128,255));
        c.convertTo(alpha8);
        QCOMPARE(c.opacityU8(), quint8(128));
    }
}

void TestColorConversionSystem::testAlphaU16Conversions()
{
    KoColorSpaceRegistry::instance();
    const KoColorSpace *alpha16 = KoColorSpaceRegistry::instance()->alpha16();
    const KoColorSpace *rgb8 = KoColorSpaceRegistry::instance()->rgb8();
    const KoColorSpace *rgb16 = KoColorSpaceRegistry::instance()->rgb16();

    {
        KoColor c(QColor(255,255,255,255), alpha16);
        QCOMPARE(c.opacityU8(), quint8(255));
        c.convertTo(rgb8);
        QCOMPARE(c.toQColor(), QColor(255,255,255));
        c.convertTo(alpha16);
        QCOMPARE(c.opacityU8(), quint8(255));
    }

    {
        KoColor c(QColor(255,255,255,0), alpha16);
        c.convertTo(rgb8);
        QCOMPARE(c.toQColor(), QColor(0,0,0,255));
        c.convertTo(alpha16);
        QCOMPARE(c.opacityU8(), quint8(0));
    }

    {
        KoColor c(QColor(255,255,255,128), alpha16);
        c.convertTo(rgb8);
        QCOMPARE(c.toQColor(), QColor(128,128,128,255));
        c.convertTo(alpha16);
        QCOMPARE(c.opacityU8(), quint8(128));
    }

    {
        KoColor c(QColor(255,255,255,255), alpha16);
        QCOMPARE(c.opacityU8(), quint8(255));
        c.convertTo(rgb16);
        QCOMPARE(c.toQColor(), QColor(255,255,255));
        c.convertTo(alpha16);
        QCOMPARE(c.opacityU8(), quint8(255));
    }

    {
        KoColor c(QColor(255,255,255,0), alpha16);
        c.convertTo(rgb16);
        QCOMPARE(c.toQColor(), QColor(0,0,0,255));
        c.convertTo(alpha16);
        QCOMPARE(c.opacityU8(), quint8(0));
    }

    {
        KoColor c(QColor(255,255,255,128), alpha16);
        c.convertTo(rgb16);
        QCOMPARE(c.toQColor(), QColor(128,128,128,255));
        c.convertTo(alpha16);
        QCOMPARE(c.opacityU8(), quint8(128));
    }
}

void TestColorConversionSystem::testGrayAConnectionPaths()
{
    using NodeKey = KoColorConversionSystem::NodeKey;

    std::vector<NodeKey> expectedPath;

    expectedPath =
       {{GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
        {GrayAColorModelID.id(), Integer16BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"}};
    QCOMPARE(calcPath(expectedPath), expectedPath);


    expectedPath =
       {{GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
        {RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()}};
    QCOMPARE(calcPath(expectedPath), expectedPath);

    expectedPath =
       {{GrayAColorModelID.id(), Integer16BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"},
        {GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"}};
    QCOMPARE(calcPath(expectedPath), expectedPath);


    expectedPath =
       {{RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()},
        {GrayAColorModelID.id(), Integer8BitsColorDepthID.id(), "Gray-D50-elle-V2-srgbtrc.icc"}};
    QCOMPARE(calcPath(expectedPath), expectedPath);


}

void TestColorConversionSystem::testGrayAConversions()
{
    KoColorSpaceRegistry::instance();
    const KoColorSpace *graya8 = KoColorSpaceRegistry::instance()->graya8();
    const KoColorSpace *graya16 = KoColorSpaceRegistry::instance()->graya16();
    const KoColorSpace *rgb8 = KoColorSpaceRegistry::instance()->rgb8();

    {
        KoColor c(Qt::transparent, graya8);
        QCOMPARE(c.opacityU8(), quint8(0));
        c.convertTo(graya16);
        QCOMPARE(c.opacityU8(), quint8(0));
        QCOMPARE(c.toQColor(), QColor(Qt::transparent));
        c.convertTo(graya8);
        QCOMPARE(c.opacityU8(), quint8(0));

        c.convertTo(rgb8);
        QCOMPARE(c.opacityU8(), quint8(0));
        QCOMPARE(c.toQColor(), QColor(Qt::transparent));
    }

    {
        KoColor c(QColor(255,255,255), graya8);
        QCOMPARE(c.opacityU8(), quint8(255));
        c.convertTo(graya16);
        QCOMPARE(c.opacityU8(), quint8(255));
        QCOMPARE(c.toQColor(), QColor(Qt::white));
        c.convertTo(graya8);
        QCOMPARE(c.opacityU8(), quint8(255));

        c.convertTo(rgb8);
        QCOMPARE(c.opacityU8(), quint8(255));
        QCOMPARE(c.toQColor(), QColor(Qt::white));
    }

    {
        KoColor c(QColor(180,180,180), graya8);
        QCOMPARE(c.opacityU8(), quint8(255));
        c.convertTo(graya16);
        QCOMPARE(c.opacityU8(), quint8(255));
        QCOMPARE(c.toQColor(), QColor(180,180,180));
        c.convertTo(graya8);
        QCOMPARE(c.opacityU8(), quint8(255));

        c.convertTo(rgb8);
        QCOMPARE(c.opacityU8(), quint8(255));
        QCOMPARE(c.toQColor(), QColor(180,180,180));
    }
}

using ExpectedPathVector = std::vector<KoColorConversionSystem::NodeKey>;
Q_DECLARE_METATYPE(ExpectedPathVector)

void TestColorConversionSystem::testRec2020PQConnectionPaths_data()
{
    using NodeKey = KoColorConversionSystem::NodeKey;
    QTest::addColumn<ExpectedPathVector>("expectedPath");

    auto addPQTests = [](const char *pqTagVariant, const QString &pqProfileName) {
        auto rgbNode = [](const KoID &colorDepthId, const QString &profileName) -> NodeKey {
            return {RGBAColorModelID.id(), colorDepthId.id(), profileName};
        };

        auto iccNode = []() -> NodeKey {
            return {"icc", "icc", "icc"};
        };

#ifdef HAVE_OPENEXR
        QTest::addRow("f16-%s-f16-linear", pqTagVariant) << ExpectedPathVector{
            rgbNode(Float16BitsColorDepthID, pqProfileName),
            rgbNode(Float16BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
        };

        QTest::addRow("f16-linear-f16-%s", pqTagVariant) << ExpectedPathVector{
            rgbNode(Float16BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
            rgbNode(Float16BitsColorDepthID, pqProfileName),
        };
#endif

        QTest::addRow("f32-%s-f32-linear", pqTagVariant) << ExpectedPathVector{
            rgbNode(Float32BitsColorDepthID, pqProfileName),
            rgbNode(Float32BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
        };

        QTest::addRow("f32-linear-f32-%s", pqTagVariant) << ExpectedPathVector{
            rgbNode(Float32BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
            rgbNode(Float32BitsColorDepthID, pqProfileName),
        };

        //
        // PQ spaces have direct connections to floating point spaces only. It disables
        // the usage of the integer spaces as intermediate spaces
        //

#ifdef HAVE_OPENEXR
        QTest::addRow("f16-linear-u16-%s", pqTagVariant) << ExpectedPathVector{
            rgbNode(Float16BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
            rgbNode(Integer16BitsColorDepthID, pqProfileName),
        };

        QTest::addRow("u16-%s-f16-linear", pqTagVariant) << ExpectedPathVector{
            rgbNode(Integer16BitsColorDepthID, pqProfileName),
            rgbNode(Float16BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
        };
#endif

        //
        // It is impossible to convert any PQ space into an integer space directly,
        // it should go through a floating point space first
        //

        QTest::addRow("u16-%s-u16-linear", pqTagVariant) << ExpectedPathVector{
            rgbNode(Integer16BitsColorDepthID, pqProfileName),
            rgbNode(Float32BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
            iccNode(),
            rgbNode(Integer16BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
        };

        //
        // Conversion into sRGB happens via F32 space as well
        // TODO: perhaps we could optimize U8 case and use F16 as an intermediate space instead?
        //

        QTest::addRow("u16-%s-u8-srgb", pqTagVariant) << ExpectedPathVector{
            rgbNode(Integer16BitsColorDepthID, pqProfileName),
            rgbNode(Float32BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
            iccNode(),
            rgbNode(Integer8BitsColorDepthID, KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()),
        };

        QTest::addRow("u8-%s-u8-srgb", pqTagVariant) << ExpectedPathVector{
            rgbNode(Integer8BitsColorDepthID, pqProfileName),
            rgbNode(Float32BitsColorDepthID, KoColorSpaceRegistry::instance()->p2020G10Profile()->name()),
            iccNode(),
            rgbNode(Integer8BitsColorDepthID, KoColorSpaceRegistry::instance()->p709SRGBProfile()->name()),
        };
    };

    addPQTests("pq", KoColorSpaceRegistry::instance()->p2020PQProfile()->name());

    KoColorProfileQuery query(PRIMARIES_ITU_R_BT_2020_2_AND_2100_0,
                              TRC_ITU_R_BT_2100_0_PQ,
                              180.0);
    const KoColorProfile *additionalProfile =
        KoColorSpaceRegistry::instance()->profileForInternal(query, true);
    QVERIFY(additionalProfile);
    addPQTests("pq_180nit", additionalProfile->name());

    addPQTests("pq_legacy", "High Dynamic Range UHDTV Wide Color Gamut Display (Rec. 2020) - SMPTE ST 2084 PQ EOTF");
}

void TestColorConversionSystem::testRec2020PQConnectionPaths()
{
    using NodeKey = KoColorConversionSystem::NodeKey;
    QFETCH(std::vector<NodeKey>, expectedPath);

    auto loadExternalProfile = [](const QString &profileName, const QString &profileFileName) {
        const KoColorProfile *profile = KoColorSpaceRegistry::instance()->profileByName(profileName);
        if (!profile || profile->name() != profileName) {
            const QString profileFilePath = TestUtil::fetchDataFileLazy(profileFileName);
            KIS_ASSERT(QFile::exists(profileFilePath));

            {
                KoColorSpaceEngine *iccEngine = KoColorSpaceEngineRegistry::instance()->get("icc");
                KIS_ASSERT(iccEngine);
                const KoColorProfile *profile = iccEngine->addProfile(profileFilePath);
                KIS_ASSERT(profile);
            }
        }
    };

    const QString legacyProfileName = "High Dynamic Range UHDTV Wide Color Gamut Display (Rec. 2020) - SMPTE ST 2084 PQ EOTF";
    const QString legacyProfileFileName = "ITUR_2100_PQ_FULL.ICC";

    loadExternalProfile(legacyProfileName, legacyProfileFileName);

    QCOMPARE(calcPath(expectedPath, false), expectedPath);
}

void TestColorConversionSystem::benchmarkAlphaToRgbConversion()
{
    const KoColorSpace *alpha8 = KoColorSpaceRegistry::instance()->alpha8();
    const KoColorSpace *rgb8 = KoColorSpaceRegistry::instance()->rgb8();

    const int numPixels = 1024 * 4096;
    QByteArray srcBuf(numPixels * alpha8->pixelSize(), '\0');
    QByteArray dstBuf(numPixels * rgb8->pixelSize(), '\0');

    QRandomGenerator rng{};
    for (int i = 0; i < srcBuf.size(); i++) {
        srcBuf[i] = static_cast<char>(rng.bounded(256));
    }

    QBENCHMARK {
        alpha8->convertPixelsTo((quint8*)srcBuf.data(),
                                (quint8*)dstBuf.data(),
                                rgb8,
                                numPixels,
                                KoColorConversionTransformation::IntentPerceptual,
                                KoColorConversionTransformation::Empty);
    }
}

void TestColorConversionSystem::benchmarkRgbToAlphaConversion()
{
    const KoColorSpace *alpha8 = KoColorSpaceRegistry::instance()->alpha8();
    const KoColorSpace *rgb8 = KoColorSpaceRegistry::instance()->rgb8();

    const int numPixels = 1024 * 4096;
    QByteArray srcBuf(numPixels * rgb8->pixelSize(), '\0');
    QByteArray dstBuf(numPixels * alpha8->pixelSize(), '\0');

    QRandomGenerator rng{};
    for (int i = 0; i < srcBuf.size(); i++) {
        srcBuf[i] = static_cast<char>(rng.bounded(256));
    }

    QBENCHMARK {
        rgb8->convertPixelsTo((quint8*)srcBuf.data(),
                              (quint8*)dstBuf.data(),
                              alpha8,
                              numPixels,
                              KoColorConversionTransformation::IntentPerceptual,
                              KoColorConversionTransformation::Empty);
    }
}

void TestColorConversionSystem::testCmykBitnessConversion()
{
    const KoColorSpace *cmyk8 =
        KoColorSpaceRegistry::instance()->colorSpace(CMYKAColorModelID.id(),
                                                     Integer8BitsColorDepthID.id(),
                                                     "Chemical proof");

    const KoColorSpace *cmyk16 =
        KoColorSpaceRegistry::instance()->colorSpace(CMYKAColorModelID.id(),
                                                     Integer16BitsColorDepthID.id(),
                                                     "Chemical proof");

//    ENTER_FUNCTION() << ppVar(cmyk8);
//    ENTER_FUNCTION() << ppVar(cmyk8->profile()->name());
//    ENTER_FUNCTION() << ppVar(cmyk8->profile()->fileName());

//    ENTER_FUNCTION() << ppVar(cmyk16);
//    ENTER_FUNCTION() << ppVar(cmyk16->profile()->name());
//    ENTER_FUNCTION() << ppVar(cmyk16->profile()->fileName());


    KoColor color(QColor(177, 180, 42, 255), cmyk8);
//    qDebug() << ppVar(color);
    color.convertTo(cmyk16);
//    qDebug() << ppVar(color);
    KoColor color2 = color.convertedTo(cmyk8);
//    qDebug() << ppVar(color2);

    /**
     * For some reason out CMYK color spaces don't support round-tripping
     * to-from 16-bit representation. So the code that relies on that should
     * use KoOptimizedCmykPixelDataScalerU8ToU16Factory::create().
     */
    QVERIFY(color != color2);

}


KISTEST_MAIN(TestColorConversionSystem)

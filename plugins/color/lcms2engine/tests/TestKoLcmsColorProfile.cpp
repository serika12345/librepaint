/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "TestKoLcmsColorProfile.h"

#include <cmath>
#include <ctime>
#include <cstring>
#include <lcms2.h>
#include <lcms2_plugin.h>
#include <array>
#include <memory>
#include <QCryptographicHash>
#include <QDate>
#include <QColorSpace>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <KoColorModelStandardIds.h>
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

namespace {
struct DisplayPipelineCapture {
    cmsPipeline *pipeline = nullptr;
    cmsHPROFILE source = nullptr;
    cmsHPROFILE destination = nullptr;
    int matches = 0;
    ~DisplayPipelineCapture() { if (pipeline) cmsPipelineFree(pipeline); }
};
cmsPipeline *captureIntegerDisplayPipeline(cmsContext context, cmsUInt32Number count,
    cmsUInt32Number intents[], cmsHPROFILE profiles[], cmsBool bpc[],
    cmsFloat64Number adaptation[], cmsUInt32Number flags)
{
    auto *original = _cmsDefaultICCintents(context, count, intents, profiles, bpc, adaptation, flags);
    auto *capture = static_cast<DisplayPipelineCapture *>(cmsGetContextUserData(context));
    if (original && count >= 2 && profiles[0] == capture->source &&
        profiles[count-1] == capture->destination) {
        auto *copy = cmsPipelineDup(original);
        if (copy) {
            if (capture->pipeline) cmsPipelineFree(capture->pipeline);
            capture->pipeline = copy;
            ++capture->matches;
        }
    }
    return original;
}
}

void TestKoLcmsColorProfile::displayStagesRetainActualRgbValues_data()
{
    QTest::addColumn<QString>("profileName");
    QTest::newRow("linear") << QStringLiteral("linear");
    QTest::newRow("nonlinear") << QStringLiteral("nonlinear");
    QTest::newRow("different-primaries") << QStringLiteral("different-primaries");
}

void TestKoLcmsColorProfile::displayStagesRetainActualRgbValues()
{
    QFETCH(QString, profileName);
    auto *registry = KoColorSpaceRegistry::instance();
    const KoColorSpace *source = registry->rgb16();
    if (profileName != QLatin1String("linear")) {
        const auto raw = QColorSpace(profileName == QLatin1String("nonlinear")
            ? QColorSpace::SRgb : QColorSpace::DisplayP3).iccProfile();
        const auto *profile = registry->createColorProfileFromData(
            RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), raw);
        QVERIFY(profile);
        source = registry->colorSpace(RGBAColorModelID.id(), Integer16BitsColorDepthID.id(), profile);
    }
    QVERIFY(source);
    // KisConfig::monitorProfile's default and the multi-surface manager's
    // null-profile fallback both resolve to this registered Elle profile.
    const auto *monitor = registry->p709SRGBProfile();
    QVERIFY(monitor);
    const auto *destination = registry->colorSpace(RGBAColorModelID.id(), Integer8BitsColorDepthID.id(), monitor);
    QVERIFY(destination);
    const auto intent = KoColorConversionTransformation::IntentPerceptual;
    const auto flags = KoColorConversionTransformation::HighQuality |
                       KoColorConversionTransformation::BlackpointCompensation;
    std::unique_ptr<KoColorConversionTransformation> copy(source->createColorConverter(source, intent, flags));
    QVERIFY(copy);
    const auto copied = copy->displayStages();
    QCOMPARE(copied.status, KoColorConversionStagesResult::Success);
    QVERIFY(copied.value.stages.isEmpty());
    QCOMPARE(copied.value.input.iccProfile, source->profile()->rawData());
    std::unique_ptr<KoColorConversionTransformation> conversion(
        source->createColorConverter(destination, intent, flags));
    QVERIFY(conversion);
    const auto result = conversion->displayStages();
    QVERIFY2(result.status == KoColorConversionStagesResult::Success, qPrintable(result.diagnostic));
    QCOMPARE(result.value.input.iccProfile, source->profile()->rawData());
    QCOMPARE(result.value.output.iccProfile, destination->profile()->rawData());
    QCOMPARE(result.value.input.depth, KoColorConversionStages::Encoding::U16);
    QCOMPARE(result.value.output.depth, KoColorConversionStages::Encoding::U8);
    using Value = KoColorConversionStages;
    std::unique_ptr<cmsPipeline, decltype(&cmsPipelineFree)> pipeline(
        cmsPipelineAlloc(nullptr, 3, 3), cmsPipelineFree);
    QVERIFY(pipeline);
    QJsonArray stages;
    for (const auto &stage : result.value.stages) {
        cmsStage *native = nullptr;
        QJsonObject json;
        if (const auto *matrix = std::get_if<Value::Matrix>(&stage)) {
            native = cmsStageAllocMatrix(nullptr, 3, 3, matrix->coefficients.data(), matrix->offset.data());
            QJsonArray coefficients, offset;
            for (double v : matrix->coefficients) coefficients.append(v);
            for (double v : matrix->offset) offset.append(v);
            json = {{"matrix", coefficients}, {"offset", offset}};
        } else if (const auto *clut = std::get_if<Value::Clut>(&stage)) {
            native = cmsStageAllocCLut16bitGranular(nullptr, clut->grid.data(), 3, 3,
                                                    clut->table.constData());
            QJsonArray grid;
            for (quint32 v : clut->grid) grid.append(int(v));
            QByteArray tableBytes;
            tableBytes.reserve(clut->table.size() * 2);
            for (quint16 v : clut->table) {
                const quint16 little = qToLittleEndian(v);
                tableBytes.append(reinterpret_cast<const char *>(&little), 2);
            }
            json = {{"clut", QJsonObject{{"grid", grid},
                {"table16Base64", QString::fromLatin1(tableBytes.toBase64())}}}};
        } else {
            const auto &curves = std::get<Value::Curves>(stage);
            cmsToneCurve *nativeCurves[3]{};
            const auto freeCurves = qScopeGuard([&] {
                for (auto *curve : nativeCurves) if (curve) cmsFreeToneCurve(curve);
            });
            QJsonArray channels;
            for (int c = 0; c < 3; ++c) {
                QJsonObject curve;
                if (const auto *parametric = std::get_if<Value::ParametricCurve>(&curves.rgb[c])) {
                    nativeCurves[c] = cmsBuildParametricToneCurve(nullptr, parametric->functionType,
                                                                parametric->parameters.constData());
                    QJsonArray parameters;
                    for (double v : parametric->parameters) parameters.append(v);
                    curve = {{"functionType", parametric->functionType}, {"parameters", parameters}};
                } else {
                    const auto &table = std::get<Value::Table16Curve>(curves.rgb[c]).values;
                    nativeCurves[c] = cmsBuildTabulatedToneCurve16(nullptr, table.size(), table.constData());
                    QJsonArray values;
                    for (quint16 v : table) values.append(int(v));
                    curve = {{"table16", values}};
                }
                QVERIFY(nativeCurves[c]);
                channels.append(curve);
            }
            native = cmsStageAllocToneCurves(nullptr, 3, nativeCurves);
            json = {{"curves", channels}};
        }
        QVERIFY(native);
        QVERIFY(cmsPipelineInsertStage(pipeline.get(), cmsAT_END, native));
        stages.append(json);
    }
    const auto inputRaw = source->profile()->rawData(), outputRaw = destination->profile()->rawData();
    DisplayPipelineCapture capture;
    const auto context = cmsCreateContext(nullptr, &capture);
    QVERIFY(context);
    const auto deleteContext = qScopeGuard([&] {
        if (capture.pipeline) cmsPipelineFree(capture.pipeline);
        capture.pipeline = nullptr;
        cmsDeleteContext(context);
    });
    cmsPluginRenderingIntent hook{};
    hook.base.Magic = cmsPluginMagicNumber;
    hook.base.ExpectedVersion = LCMS_VERSION;
    hook.base.Type = cmsPluginRenderingIntentSig;
    hook.Intent = intent;
    hook.Link = captureIntegerDisplayPipeline;
    QVERIFY(cmsPluginTHR(context, &hook));
    const auto input = cmsOpenProfileFromMemTHR(context, inputRaw.constData(), inputRaw.size());
    const auto output = cmsOpenProfileFromMemTHR(context, outputRaw.constData(), outputRaw.size());
    QVERIFY(input && output);
    capture.source = input;
    capture.destination = output;
    const auto close = qScopeGuard([&] { cmsCloseProfile(input); cmsCloseProfile(output); });
    cmsUInt32Number displayFlags = flags | cmsFLAGS_COPY_ALPHA;
    if (source->profile()->isLinear() || destination->profile()->isLinear())
        displayFlags |= cmsFLAGS_NOOPTIMIZE;
    const auto integerTransform = cmsCreateTransformTHR(context, input, TYPE_BGRA_16,
        output, TYPE_BGRA_8, intent, displayFlags);
    QVERIFY(integerTransform);
    const auto deleteTransform = qScopeGuard([&] { cmsDeleteTransform(integerTransform); });
    QVERIFY(capture.pipeline);
    QVERIFY(capture.matches > 0);
    const cmsPipeline *actualPipeline = cmsGetTransformPipeline(integerTransform);
    QVERIFY(actualPipeline);
    const auto toDisplayByte = [](quint16 word) {
        return quint8((quint32(word) * 65281u + 8388608u) >> 24u);
    };
    QJsonArray samples;
    int maximumCpuDisplayDifference = 0;
    int ownedStageDifference = 0;
    const quint16 levels[] = {0, 1, 16, 128, 256, 1024, 8192, 32768, 60000, 65535};
    for (quint16 r : levels) for (quint16 g : levels) for (quint16 b : levels) {
        // The owned values must reproduce the actual integer display pipeline,
        // including optimized CLUT stages, at the same 16-bit inputs.
        const quint16 inputWords[] = {r, g, b};
        quint16 expectedWords[3]{}, ownedWords[3]{};
        cmsPipelineEval16(inputWords, expectedWords, actualPipeline);
        cmsPipelineEval16(inputWords, ownedWords, pipeline.get());
        ownedStageDifference += expectedWords[0] != ownedWords[0] ||
                                expectedWords[1] != ownedWords[1] ||
                                expectedWords[2] != ownedWords[2];
        const float normalized[] = {r / 65535.f, g / 65535.f, b / 65535.f};
        float original[3]{};
        cmsPipelineEvalFloat(normalized, original, capture.pipeline);
        const quint16 bgra[] = {b, g, r, 65535};
        quint8 cpuBgra[4]{}, capturedBgra[4]{};
        conversion->transform(reinterpret_cast<const quint8 *>(bgra), cpuBgra, 1);
        cmsDoTransform(integerTransform, bgra, capturedBgra, 1);
        QCOMPARE(QByteArray(reinterpret_cast<const char *>(capturedBgra), 4),
                 QByteArray(reinterpret_cast<const char *>(cpuBgra), 4));
        QJsonArray originalRgb, cpu;
        for (int c = 0; c < 3; ++c) {
            const int staged = qBound(0, int(std::floor(original[c] * 255.0 + 0.5)), 255);
            const int displayed = toDisplayByte(expectedWords[c]);
            QCOMPARE(displayed, int(cpuBgra[2-c]));
            maximumCpuDisplayDifference = qMax(maximumCpuDisplayDifference, std::abs(staged-displayed));
            originalRgb.append(staged); cpu.append(displayed);
        }
        samples.append(QJsonObject{{"rgb16", QJsonArray{int(r), int(g), int(b)}},
                                    {"originalStageRgb8", originalRgb}, {"cpuDisplayRgb8", cpu}});
    }
    QCOMPARE(ownedStageDifference, 0);
    qInfo() << "original-stage versus CPU optimized display maximum U8 difference:"
            << maximumCpuDisplayDifference << "(measurement, not an accepted tolerance)";
    const auto directory = qEnvironmentVariable("LIBREPAINT_DISPLAY_STAGE_FIXTURE");
    if (!directory.isEmpty()) {
        QJsonObject fixture{
            {"provenance", "LCMS 2.19.1 actual optimized display pipeline via cmsGetTransformPipeline; KisConfig monitorProfile default and multi-surface null fallback sRGB-elle-V2-srgbtrc.icc; Perceptual, HighQuality+BPC; owned values reproduce cmsPipelineEval16 exactly"},
            {"inputIccBase64", QString::fromLatin1(inputRaw.toBase64())},
            {"outputIccBase64", QString::fromLatin1(outputRaw.toBase64())},
            {"stages", stages}, {"samples", samples},
            {"maximumCpuDisplayDifference", maximumCpuDisplayDifference}};
        QFile file(QDir(directory).filePath(profileName + QStringLiteral(".json")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        const auto bytes = QJsonDocument(fixture).toJson(QJsonDocument::Compact);
        QCOMPARE(file.write(bytes), bytes.size());
    }
}

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

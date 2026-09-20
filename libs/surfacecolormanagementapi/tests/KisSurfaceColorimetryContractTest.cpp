/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <surfacecolormanagement/KisSurfaceColorimetry.h>

#include <QDebug>
#include <QTest>

#include <array>

class KisSurfaceColorimetryContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void protocolIdentifiersRemainStable();
    void luminancePreservesDisplayRange();
    void masteringLuminancePreservesContentRange();
    void colorSpaceDetectsHdrLuminance();
    void masteringAndSurfaceDescriptionsPreserveOptionalMetadata();
    void debugAndTextReportsDescribeAllMetadata();
};

using namespace KisSurfaceColorimetry;

void KisSurfaceColorimetryContractTest::protocolIdentifiersRemainStable()
{
    const std::array<std::pair<NamedPrimaries, int>, 6> primaries {{
        {NamedPrimaries::primaries_unknown, 0},
        {NamedPrimaries::primaries_srgb, 1},
        {NamedPrimaries::primaries_bt2020, 6},
        {NamedPrimaries::primaries_dci_p3, 8},
        {NamedPrimaries::primaries_display_p3, 9},
        {NamedPrimaries::primaries_adobe_rgb, 10},
    }};
    const std::array<std::pair<NamedTransferFunction, int>, 9> transferFunctions {{
        {NamedTransferFunction::transfer_function_unknown, 0},
        {NamedTransferFunction::transfer_function_bt1886, 1},
        {NamedTransferFunction::transfer_function_gamma22, 2},
        {NamedTransferFunction::transfer_function_gamma28, 3},
        {NamedTransferFunction::transfer_function_ext_linear, 5},
        {NamedTransferFunction::transfer_function_srgb, 9},
        {NamedTransferFunction::transfer_function_ext_srgb, 10},
        {NamedTransferFunction::transfer_function_st2084_pq, 11},
        {NamedTransferFunction::transfer_function_st428, 12},
    }};
    const std::array<RenderIntent, 5> renderIntents {{
        RenderIntent::render_intent_perceptual,
        RenderIntent::render_intent_relative,
        RenderIntent::render_intent_saturation,
        RenderIntent::render_intent_absolute,
        RenderIntent::render_intent_relative_bpc,
    }};

    for (const auto &entry : primaries) {
        QCOMPARE(int(entry.first), entry.second);
    }
    for (const auto &entry : transferFunctions) {
        QCOMPARE(int(entry.first), entry.second);
    }
    for (std::size_t index = 0; index < renderIntents.size(); ++index) {
        QCOMPARE(int(renderIntents[index]), int(index));
    }
}

void KisSurfaceColorimetryContractTest::luminancePreservesDisplayRange()
{
    const Luminance defaults;
    QCOMPARE(defaults.minLuminance, 2000U);
    QCOMPARE(defaults.maxLuminance, 80U);
    QCOMPARE(defaults.referenceLuminance, 80U);

    const Luminance hdr(5, 1000, 203);
    QCOMPARE(hdr.minLuminance, 5U);
    QCOMPARE(hdr.maxLuminance, 1000U);
    QCOMPARE(hdr.referenceLuminance, 203U);
    QVERIFY(hdr == Luminance(5, 1000, 203));
    QVERIFY(!(hdr == defaults));
    QCOMPARE(hdr.clipToSdr(), Luminance(5, 203, 203));
}

void KisSurfaceColorimetryContractTest::masteringLuminancePreservesContentRange()
{
    const MasteringLuminance defaults;
    QCOMPARE(defaults.minLuminance, 2000U);
    QCOMPARE(defaults.maxLuminance, 80U);

    const MasteringLuminance hdr(5, 1000);
    QCOMPARE(hdr.minLuminance, 5U);
    QCOMPARE(hdr.maxLuminance, 1000U);
    QVERIFY(hdr == MasteringLuminance(5, 1000));
    QVERIFY(!(hdr == defaults));
    QCOMPARE(MasteringLuminance::fromLuminance(Luminance(5, 1000, 203)), hdr);
}

void KisSurfaceColorimetryContractTest::colorSpaceDetectsHdrLuminance()
{
    ColorSpace colorSpace;
    QVERIFY(std::holds_alternative<NamedPrimaries>(colorSpace.primaries));
    QCOMPARE(std::get<NamedPrimaries>(colorSpace.primaries), NamedPrimaries::primaries_unknown);
    QVERIFY(std::holds_alternative<NamedTransferFunction>(colorSpace.transferFunction));
    QCOMPARE(std::get<NamedTransferFunction>(colorSpace.transferFunction),
             NamedTransferFunction::transfer_function_unknown);
    QVERIFY(!colorSpace.luminance);
    QVERIFY(!colorSpace.isHDR());

    colorSpace.luminance = Luminance(5, 1000, 203);
    QVERIFY(colorSpace.isHDR());
    const ColorSpace same = colorSpace;
    QVERIFY(colorSpace == same);
    colorSpace.transferFunction = uint32_t(22000);
    QVERIFY(!(colorSpace == same));
}

void KisSurfaceColorimetryContractTest::masteringAndSurfaceDescriptionsPreserveOptionalMetadata()
{
    MasteringInfo mastering;
    QCOMPARE(mastering.primaries, KoColorimetryUtils::Colorimetry::BT709);
    QCOMPARE(mastering.luminance, MasteringLuminance());
    QVERIFY(!mastering.maxCll);
    QVERIFY(!mastering.maxFall);

    const MasteringInfo defaultMastering = mastering;
    QVERIFY(mastering == defaultMastering);
    mastering.maxCll = 1000;
    mastering.maxFall = 400;
    QVERIFY(!(mastering == defaultMastering));

    SurfaceDescription description;
    QVERIFY(description.colorSpace == ColorSpace());
    QVERIFY(!description.masteringInfo);
    const SurfaceDescription defaults = description;
    QVERIFY(description == defaults);
    description.masteringInfo = mastering;
    QVERIFY(!(description == defaults));
}

void KisSurfaceColorimetryContractTest::debugAndTextReportsDescribeAllMetadata()
{
    QString output;
    QDebug(&output).nospace() << NamedPrimaries::primaries_bt2020;
    QCOMPARE(output, QStringLiteral("NamedPrimaries(primaries_bt2020)"));

    output.clear();
    QDebug(&output).nospace() << NamedTransferFunction::transfer_function_st2084_pq;
    QCOMPARE(output, QStringLiteral("NamedTransferFunction(transfer_function_st2084_pq)"));

    output.clear();
    QDebug(&output).nospace() << Luminance(5, 1000, 203);
    QCOMPARE(output, QStringLiteral("Luminance(minLuminance: 5, maxLuminance: 1000, referenceLuminance: 203)"));

    output.clear();
    QDebug(&output).nospace() << MasteringLuminance(5, 1000);
    QCOMPARE(output, QStringLiteral("MasteringLuminance(minLuminance: 5, maxLuminance: 1000)"));

    ColorSpace colorSpace;
    colorSpace.primaries = NamedPrimaries::primaries_bt2020;
    colorSpace.transferFunction = NamedTransferFunction::transfer_function_st2084_pq;
    colorSpace.luminance = Luminance(5, 1000, 203);
    output.clear();
    QDebug(&output).nospace() << colorSpace;
    QVERIFY(output.contains(QStringLiteral("ColorSpace(primaries: NamedPrimaries(primaries_bt2020)")));
    QVERIFY(output.contains(QStringLiteral("transfer_function_st2084_pq")));

    MasteringInfo mastering;
    mastering.luminance = MasteringLuminance(5, 1000);
    mastering.maxCll = 1000;
    mastering.maxFall = 400;
    output.clear();
    QDebug(&output).nospace() << mastering;
    QVERIFY(output.contains(QStringLiteral("MasteringInfo(primaries: Colorimetry(")));
    QVERIFY(output.contains(QStringLiteral("maxCll: 1000, maxFall: 400")));

    SurfaceDescription description;
    description.colorSpace = colorSpace;
    description.masteringInfo = mastering;
    output.clear();
    QDebug(&output).nospace() << description;
    QVERIFY(output.startsWith(QStringLiteral("SurfaceDescription(colorSpace: ColorSpace(")));
    QVERIFY(output.contains(QStringLiteral("masteringInfo: MasteringInfo(")));

    output.clear();
    QDebug(&output).nospace() << RenderIntent::render_intent_relative_bpc;
    QCOMPARE(output, QStringLiteral("RenderIntent(render_intent_relative_bpc)"));

    const QString report = description.makeTextReport();
    QString normalizedReport = report;
    normalizedReport.remove(QLatin1Char('"'));
    normalizedReport = normalizedReport.simplified();
    QVERIFY2(normalizedReport.contains(QStringLiteral("Primaries: NamedPrimaries(primaries_bt2020)")),
             qPrintable(report));
    QVERIFY2(normalizedReport.contains(QStringLiteral("Transfer Function: NamedTransferFunction(transfer_function_st2084_pq)")),
             qPrintable(report));
    QVERIFY2(normalizedReport.contains(QStringLiteral("Max CLL: 1000")), qPrintable(report));
    QVERIFY2(normalizedReport.contains(QStringLiteral("Max FALL: 400")), qPrintable(report));
}

QTEST_GUILESS_MAIN(KisSurfaceColorimetryContractTest)

#include "KisSurfaceColorimetryContractTest.moc"

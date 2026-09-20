/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorimetryUtils.h>

#include <QDebug>
#include <QTest>

#include <array>

namespace
{
constexpr double tolerance = 0.00001;

void compareDouble(double actual, double expected)
{
    QVERIFY2(qAbs(actual - expected) <= tolerance,
             qPrintable(QStringLiteral("actual %1, expected %2")
                            .arg(actual, 0, 'g', 12)
                            .arg(expected, 0, 'g', 12)));
}

void compareXYZ(const KoColorimetryUtils::XYZ &actual,
                const KoColorimetryUtils::XYZ &expected)
{
    compareDouble(actual.X, expected.X);
    compareDouble(actual.Y, expected.Y);
    compareDouble(actual.Z, expected.Z);
}

void compareIdentity(const QMatrix4x4 &matrix)
{
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            compareDouble(matrix(row, column), row == column ? 1.0 : 0.0);
        }
    }
}
}

class KoColorimetryUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void chromaticityValuesConvertToTristimulusValues();
    void tristimulusValuesPreserveVectorArithmetic();
    void matricesAndConstructorsPreserveColorCoordinates();
    void namedColorimetriesPreservePublishedPrimaries();
    void validationDistinguishesUsableAndPhysicalGamuts();
    void colorTransformsPreserveWhitepointRules();
    void debugOutputIdentifiesValueTypes();
};

using namespace KoColorimetryUtils;

void KoColorimetryUtilsContractTest::chromaticityValuesConvertToTristimulusValues()
{
    const xy chromaticity {0.25, 0.5};
    QCOMPARE(chromaticity.x, 0.25);
    QCOMPARE(chromaticity.y, 0.5);
    compareXYZ(chromaticity.toXYZ(), XYZ {0.5, 1.0, 0.5});
    QCOMPARE(chromaticity.asVector(), QVector2D(0.25f, 0.5f));
    QVERIFY((chromaticity == xy {0.25, 0.5}));
    QVERIFY(!(chromaticity == xy {0.2, 0.5}));
    compareXYZ(xy {0.25, 0.0}.toXYZ(), XYZ {0.0, 0.0, 0.0});

    const xyY luminanceChromaticity {0.25, 0.5, 0.4};
    QCOMPARE(luminanceChromaticity.x, 0.25);
    QCOMPARE(luminanceChromaticity.y, 0.5);
    QCOMPARE(luminanceChromaticity.Y, 0.4);
    compareXYZ(luminanceChromaticity.toXYZ(), XYZ {0.2, 0.4, 0.2});
    QVERIFY((luminanceChromaticity == xyY {0.25, 0.5, 0.4}));
    QVERIFY(!(luminanceChromaticity == xyY {0.25, 0.5, 0.5}));
    compareXYZ(xyY {0.25, 0.0, 0.4}.toXYZ(), XYZ {0.0, 0.0, 0.0});
}

void KoColorimetryUtilsContractTest::tristimulusValuesPreserveVectorArithmetic()
{
    const XYZ value {0.2, 0.4, 0.2};
    QCOMPARE(value.X, 0.2);
    QCOMPARE(value.Y, 0.4);
    QCOMPARE(value.Z, 0.2);

    const xyY xyYValue = value.toxyY();
    compareDouble(xyYValue.x, 0.25);
    compareDouble(xyYValue.y, 0.5);
    compareDouble(xyYValue.Y, 0.4);
    const xy xyValue = value.toxy();
    compareDouble(xyValue.x, 0.25);
    compareDouble(xyValue.y, 0.5);

    QCOMPARE(value.asVector(), QVector3D(0.2f, 0.4f, 0.2f));
    compareXYZ(XYZ::fromVector(QVector3D(0.2f, 0.4f, 0.2f)), value);
    compareXYZ(value * 2.0, XYZ {0.4, 0.8, 0.4});
    compareXYZ(value / 2.0, XYZ {0.1, 0.2, 0.1});
    compareXYZ(value + XYZ {0.1, 0.2, 0.3}, XYZ {0.3, 0.6, 0.5});
    QVERIFY((value == XYZ {0.2, 0.4, 0.2}));
    QVERIFY(!(value == XYZ {0.2, 0.4, 0.3}));

    QCOMPARE((XYZ {0.0, 0.0, 0.0}.toxy()), (xy {0.0, 0.0}));
    QCOMPARE((XYZ {0.0, 0.0, 0.0}.toxyY()), (xyY {0.0, 0.0, 1.0}));
}

void KoColorimetryUtilsContractTest::matricesAndConstructorsPreserveColorCoordinates()
{
    const QMatrix4x4 columns = matrixFromColumns(QVector3D(1.0f, 2.0f, 3.0f),
                                                 QVector3D(4.0f, 5.0f, 6.0f),
                                                 QVector3D(7.0f, 8.0f, 9.0f));
    QCOMPARE(columns.map(QVector3D(1.0f, 0.0f, 0.0f)), QVector3D(1.0f, 2.0f, 3.0f));
    QCOMPARE(columns.map(QVector3D(0.0f, 1.0f, 0.0f)), QVector3D(4.0f, 5.0f, 6.0f));
    QCOMPARE(columns.map(QVector3D(0.0f, 0.0f, 1.0f)), QVector3D(7.0f, 8.0f, 9.0f));

    const xy red {0.64, 0.33};
    const xy green {0.30, 0.60};
    const xy blue {0.15, 0.06};
    const xy white {0.3127, 0.3290};
    const Colorimetry fromXy(red, green, blue, white);
    const Colorimetry fromXyY(fromXy.red().toxyY(),
                              fromXy.green().toxyY(),
                              fromXy.blue().toxyY(),
                              fromXy.white().toxyY());
    const Colorimetry fromXYZ(fromXy.red(), fromXy.green(), fromXy.blue(), fromXy.white());

    QVERIFY(fromXy == fromXyY);
    QVERIFY(fromXy == fromXYZ);
    QCOMPARE(fromXy.red().toxy(), red);
    QCOMPARE(fromXy.green().toxy(), green);
    QCOMPARE(fromXy.blue().toxy(), blue);
    QCOMPARE(fromXy.white().toxy(), white);
    compareIdentity(fromXy.fromXYZ() * fromXy.toXYZ());

    const QMatrix4x4 calculated = Colorimetry::calculateToXYZMatrix(
        fromXy.red(), fromXy.green(), fromXy.blue(), fromXy.white());
    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            compareDouble(calculated(row, column), fromXy.toXYZ()(row, column));
        }
    }
}

void KoColorimetryUtilsContractTest::namedColorimetriesPreservePublishedPrimaries()
{
    struct ExpectedColorimetry {
        const Colorimetry *colorimetry;
        xy red;
        xy white;
    };
    const std::array<ExpectedColorimetry, 10> colorimetries {{
        {&Colorimetry::BT709, {0.64, 0.33}, {0.3127, 0.3290}},
        {&Colorimetry::PAL_M, {0.67, 0.33}, {0.310, 0.316}},
        {&Colorimetry::PAL, {0.640, 0.330}, {0.3127, 0.3290}},
        {&Colorimetry::NTSC, {0.630, 0.340}, {0.3127, 0.3290}},
        {&Colorimetry::GenericFilm, {0.681, 0.319}, {0.310, 0.316}},
        {&Colorimetry::BT2020, {0.708, 0.292}, {0.3127, 0.3290}},
        {&Colorimetry::CIEXYZ, {1.0, 0.0}, {1.0 / 3.0, 1.0 / 3.0}},
        {&Colorimetry::DCIP3, {0.680, 0.320}, {0.314, 0.351}},
        {&Colorimetry::DisplayP3, {0.680, 0.320}, {0.3127, 0.3290}},
        {&Colorimetry::AdobeRGB, {0.6400, 0.3300}, {0.3127, 0.3290}},
    }};

    for (const ExpectedColorimetry &expected : colorimetries) {
        QCOMPARE(expected.colorimetry->red().toxy(), expected.red);
        QCOMPARE(expected.colorimetry->white().toxy(), expected.white);
    }
}

void KoColorimetryUtilsContractTest::validationDistinguishesUsableAndPhysicalGamuts()
{
    const xy red {0.64, 0.33};
    const xy green {0.30, 0.60};
    const xy blue {0.15, 0.06};
    const xy white {0.3127, 0.3290};
    QVERIFY(Colorimetry::isValid(red, green, blue, white));
    QVERIFY(Colorimetry::isReal(red, green, blue, white));

    QVERIFY(!Colorimetry::isValid(red, red, red, white));

    const xy outsideRed {-0.2, 0.0};
    const xy outsideGreen {1.0, 0.0};
    const xy outsideBlue {0.0, 1.0};
    const xy insideWhite {0.3, 0.3};
    QVERIFY(Colorimetry::isValid(outsideRed, outsideGreen, outsideBlue, insideWhite));
    QVERIFY(!Colorimetry::isReal(outsideRed, outsideGreen, outsideBlue, insideWhite));
}

void KoColorimetryUtilsContractTest::colorTransformsPreserveWhitepointRules()
{
    const Colorimetry &source = Colorimetry::BT709;
    compareIdentity(Colorimetry::chromaticAdaptationMatrix(source.white(), source.white()));

    const xyY d50 {0.34567, 0.35850, 1.0};
    const QMatrix4x4 adaptation = Colorimetry::chromaticAdaptationMatrix(source.white(), d50.toXYZ());
    compareXYZ(XYZ::fromVector(adaptation.map(source.white().asVector())), d50.toXYZ());

    const Colorimetry adapted = source.adaptedTo(d50);
    QCOMPARE(adapted.white().toxy(), d50.toXYZ().toxy());
    const Colorimetry changedWhite = source.withWhitepoint(d50);
    QCOMPARE(changedWhite.red(), source.red());
    QCOMPARE(changedWhite.white().toxy(), d50.toXYZ().toxy());

    const Colorimetry midpoint = source.interpolateGamutTo(Colorimetry::BT2020, 0.5);
    compareXYZ(midpoint.red(), (source.red() + Colorimetry::BT2020.red()) / 2.0);
    QCOMPARE(midpoint.white(), source.white());

    compareIdentity(source.relativeColorimetricTo(source));
    compareIdentity(source.absoluteColorimetricTo(source));
    compareIdentity(source.fromLMS() * source.toLMS());
}

void KoColorimetryUtilsContractTest::debugOutputIdentifiesValueTypes()
{
    QString output;
    QDebug(&output).nospace() << xy {0.25, 0.5};
    QCOMPARE(output, QStringLiteral("xy(x: 0.25, y: 0.5)"));

    output.clear();
    QDebug(&output).nospace() << xyY {0.25, 0.5, 0.4};
    QCOMPARE(output, QStringLiteral("xyY(x: 0.25, y: 0.5, Y: 0.4)"));

    output.clear();
    QDebug(&output).nospace() << XYZ {0.2, 0.4, 0.2};
    QCOMPARE(output, QStringLiteral("XYZ(X: 0.2, Y: 0.4, Z: 0.2)"));

    output.clear();
    QDebug(&output).nospace() << Colorimetry::BT709;
    QVERIFY(output.startsWith(QStringLiteral("Colorimetry(Red: xy(x: 0.64, y: 0.33)")));
    QVERIFY(output.contains(QStringLiteral("White: xy(x: 0.3127, y: 0.329)")));
}

QTEST_GUILESS_MAIN(KoColorimetryUtilsContractTest)

#include "KoColorimetryUtilsContractTest.moc"

/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2010 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2012 Friedrich W. H. Kossebau <kossebau@kde.org>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoUnit.h>

#include <QDebug>
#include <QTest>
#include <QTransform>

#include <array>
#include <cmath>

class KoUnitContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void conversionConstantsRemainStable();
    void unitAndOptionIdentifiersRemainStable();
    void constructionAssignmentAndEqualityPreserveTypeAndFactor();
    void symbolsAndDescriptionsRemainStable();
    void uiListsRoundTripAndHidePixels();
    void userConversionsRoundTripAndRound();
    void parsersHandleUnitsAnglesAndFallbacks();
    void pixelTransformUpdatesScale();
    void debugStreamUsesUnitSymbol();
};

void KoUnitContractTest::conversionConstantsRemainStable()
{
    QCOMPARE(POINT_TO_MM(1.0), 0.352777167);
    QCOMPARE(MM_TO_POINT(1.0), 2.83465058);
    QCOMPARE(POINT_TO_CM(1.0), 0.0352777167);
    QCOMPARE(CM_TO_POINT(1.0), 28.3465058);
    QCOMPARE(POINT_TO_DM(1.0), 0.00352777167);
    QCOMPARE(DM_TO_POINT(1.0), 283.465058);
    QCOMPARE(POINT_TO_INCH(1.0), 0.01388888888889);
    QCOMPARE(INCH_TO_POINT(1.0), 72.0);
    QCOMPARE(MM_TO_INCH(1.0), 0.039370147);
    QCOMPARE(INCH_TO_MM(1.0), 25.399956);
    QCOMPARE(POINT_TO_PI(1.0), 0.083333333);
    QCOMPARE(POINT_TO_CC(1.0), 0.077880997);
    QCOMPARE(PI_TO_POINT(1.0), 12.0);
    QCOMPARE(CC_TO_POINT(1.0), 12.840103);

    QCOMPARE(PT_ROUNDING, 1000.0);
    QCOMPARE(CM_ROUNDING, 10000.0);
    QCOMPARE(DM_ROUNDING, 10000.0);
    QCOMPARE(MM_ROUNDING, 10000.0);
    QCOMPARE(IN_ROUNDING, 100000.0);
    QCOMPARE(PI_ROUNDING, 100000.0);
    QCOMPARE(CC_ROUNDING, 100000.0);
}

void KoUnitContractTest::unitAndOptionIdentifiersRemainStable()
{
    QCOMPARE(int(KoUnit::Millimeter), 0);
    QCOMPARE(int(KoUnit::Point), 1);
    QCOMPARE(int(KoUnit::Inch), 2);
    QCOMPARE(int(KoUnit::Centimeter), 3);
    QCOMPARE(int(KoUnit::Decimeter), 4);
    QCOMPARE(int(KoUnit::Pica), 5);
    QCOMPARE(int(KoUnit::Cicero), 6);
    QCOMPARE(int(KoUnit::Pixel), 7);
    QCOMPARE(int(KoUnit::TypeCount), 8);

    QCOMPARE(int(KoUnit::ListAll), 0);
    QCOMPARE(int(KoUnit::HidePixel), 1);
    QCOMPARE(int(KoUnit::HideMask), int(KoUnit::HidePixel));

    const KoUnit::ListOptions hidden = KoUnit::HidePixel;
    QVERIFY(hidden.testFlag(KoUnit::HidePixel));
    QVERIFY(hidden.testFlag(KoUnit::HideMask));
}

void KoUnitContractTest::constructionAssignmentAndEqualityPreserveTypeAndFactor()
{
    const KoUnit defaults;
    QCOMPARE(defaults.type(), KoUnit::Point);
    QCOMPARE(defaults, KoUnit(KoUnit::Point));

    KoUnit pixel(KoUnit::Pixel, 0.5);
    QCOMPARE(pixel.toUserValuePrecise(10.0), 5.0);
    pixel.setFactor(2.0);
    QCOMPARE(pixel.toUserValuePrecise(10.0), 20.0);

    pixel = KoUnit::Inch;
    QCOMPARE(pixel.type(), KoUnit::Inch);
    QCOMPARE(pixel, KoUnit(KoUnit::Inch));
    QVERIFY(!(pixel == defaults));
}

void KoUnitContractTest::symbolsAndDescriptionsRemainStable()
{
    struct UnitSymbol {
        KoUnit::Type type;
        const char *symbol;
    };
    const std::array<UnitSymbol, KoUnit::TypeCount> units {{
        {KoUnit::Millimeter, "mm"},
        {KoUnit::Point, "pt"},
        {KoUnit::Inch, "in"},
        {KoUnit::Centimeter, "cm"},
        {KoUnit::Decimeter, "dm"},
        {KoUnit::Pica, "pi"},
        {KoUnit::Cicero, "cc"},
        {KoUnit::Pixel, "px"},
    }};

    for (const UnitSymbol &entry : units) {
        const KoUnit unit(entry.type);
        QCOMPARE(unit.symbol(), QLatin1String(entry.symbol));
        QCOMPARE(unit.toString(), unit.symbol());
        QVERIFY(!KoUnit::unitDescription(entry.type).isEmpty());

        bool ok = false;
        QCOMPARE(KoUnit::fromSymbol(QLatin1String(entry.symbol), &ok), unit);
        QVERIFY(ok);
    }

    bool ok = false;
    QCOMPARE(KoUnit::fromSymbol(QStringLiteral("inch"), &ok), KoUnit(KoUnit::Inch));
    QVERIFY(ok);
    QCOMPARE(KoUnit::fromSymbol(QStringLiteral("unknown"), &ok), KoUnit(KoUnit::Point));
    QVERIFY(!ok);
    QVERIFY(!KoUnit::unitDescription(KoUnit::TypeCount).isEmpty());
}

void KoUnitContractTest::uiListsRoundTripAndHidePixels()
{
    QCOMPARE(KoUnit::listOfUnitNameForUi(KoUnit::ListAll).size(), int(KoUnit::TypeCount));
    QCOMPARE(KoUnit::listOfUnitNameForUi(KoUnit::HidePixel).size(), int(KoUnit::TypeCount) - 1);

    for (int index = 0; index < KoUnit::TypeCount; ++index) {
        const KoUnit unit = KoUnit::fromListForUi(index, KoUnit::ListAll);
        QCOMPARE(unit.indexInListForUi(KoUnit::ListAll), index);
    }

    QCOMPARE(KoUnit(KoUnit::Pixel).indexInListForUi(KoUnit::HidePixel), -1);
    const KoUnit scaledPixel = KoUnit::fromListForUi(KoUnit::TypeCount - 1,
                                                     KoUnit::ListAll,
                                                     2.0);
    QCOMPARE(scaledPixel.type(), KoUnit::Pixel);
    QCOMPARE(scaledPixel.toUserValuePrecise(3.0), 6.0);
}

void KoUnitContractTest::userConversionsRoundTripAndRound()
{
    const KoUnit point(KoUnit::Point);
    const KoUnit inch(KoUnit::Inch);
    QVERIFY(qAbs(KoUnit::convertFromUnitToUnit(72.0, point, inch) - 1.0) < 1e-12);
    QVERIFY(qAbs(KoUnit::convertFromUnitToUnit(1.0, inch, point) - 72.0) < 1e-12);
    QCOMPARE(KoUnit::convertFromUnitToUnit(6.0,
                                           KoUnit(KoUnit::Pixel),
                                           point,
                                           2.0),
             3.0);

    const KoUnit millimeter(KoUnit::Millimeter);
    const qreal precise = millimeter.toUserValuePrecise(1000.0);
    QCOMPARE(millimeter.toUserValue(1000.0, false), precise);
    QCOMPARE(millimeter.toUserValue(1000.0, true),
             millimeter.toUserValueRounded(1000.0));
    QVERIFY(qAbs(millimeter.fromUserValue(precise) - 1000.0) < 1e-3);

    bool ok = false;
    const QString userValue = point.toUserStringValue(12.5);
    QCOMPARE(point.fromUserValue(userValue, &ok), 12.5);
    QVERIFY(ok);
}

void KoUnitContractTest::parsersHandleUnitsAnglesAndFallbacks()
{
    QCOMPARE(KoUnit::parseValue(QStringLiteral("10mm")), MM_TO_POINT(10.0));
    QCOMPARE(KoUnit::parseValue(QStringLiteral("2in")), INCH_TO_POINT(2.0));
    QCOMPARE(KoUnit::parseValue(QStringLiteral("2m")), DM_TO_POINT(20.0));
    QCOMPARE(KoUnit::parseValue(QStringLiteral("unknown"), 7.0), 7.0);

    QCOMPARE(KoUnit::parseAngle(QStringLiteral("180deg")), 180.0);
    QVERIFY(qAbs(KoUnit::parseAngle(QString::number(M_PI, 'g', 17) + QStringLiteral("rad")) -
                 180.0) < 1e-4);
    QCOMPARE(KoUnit::parseAngle(QStringLiteral("100grad")), 90.0);
    QCOMPARE(KoUnit::parseAngle(QStringLiteral("unknown"), 7.0), 7.0);
}

void KoUnitContractTest::pixelTransformUpdatesScale()
{
    QTransform transform;
    transform.scale(3.0, 3.0);
    QCOMPARE(KoUnit::approxTransformScale(transform), 3.0);

    KoUnit pixel(KoUnit::Pixel, 2.0);
    pixel.adjustByPixelTransform(transform);
    QCOMPARE(pixel.toUserValuePrecise(4.0), 24.0);
}

void KoUnitContractTest::debugStreamUsesUnitSymbol()
{
    QString output;
    QDebug(&output) << KoUnit(KoUnit::Inch);
#ifndef NDEBUG
    QVERIFY(output.contains(QStringLiteral("in")));
#else
    QVERIFY(output.trimmed().isEmpty());
#endif
}

QTEST_GUILESS_MAIN(KoUnitContractTest)

#include "KoUnitContractTest.moc"

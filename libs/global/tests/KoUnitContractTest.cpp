/* This file is part of the KDE project
 * SPDX-FileCopyrightText: 2010 Thomas Zander <zander@kde.org>
 * SPDX-FileCopyrightText: 2012 Friedrich W. H. Kossebau <kossebau@kde.org>
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoUnit.h>

#include <QTest>
#include <QTransform>

class KoUnitContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void unitFieldSymbolsConvertValues_data();
    void unitFieldSymbolsConvertValues();
    void unitPickerMapsDisplayedChoicesToUnits();
    void userConversionsPreservePhysicalLengths();
    void pixelTransformUpdatesScale();
};

void KoUnitContractTest::unitFieldSymbolsConvertValues_data()
{
    QTest::addColumn<QString>("symbol");
    QTest::addColumn<qreal>("pointsPerUnit");

    QTest::newRow("millimeter") << QStringLiteral("mm") << qreal(2.83465058);
    QTest::newRow("point") << QStringLiteral("pt") << qreal(1.0);
    QTest::newRow("inch") << QStringLiteral("in") << qreal(72.0);
    QTest::newRow("centimeter") << QStringLiteral("cm") << qreal(28.3465058);
    QTest::newRow("decimeter") << QStringLiteral("dm") << qreal(283.465058);
    QTest::newRow("pica") << QStringLiteral("pi") << qreal(12.0);
    QTest::newRow("cicero") << QStringLiteral("cc") << qreal(12.840103);
    QTest::newRow("pixel") << QStringLiteral("px") << qreal(1.0);
}

void KoUnitContractTest::unitFieldSymbolsConvertValues()
{
    // Consumer: Unit-entry fields in painting and shape dialogs.
    // Operation: A user supplies a supported unit symbol with a numeric value.
    // Observable result: The symbol is recognized and one unit converts to its internal point length.
    // Failure impact: User-entered lengths are interpreted with the wrong size.
    QFETCH(QString, symbol);
    QFETCH(qreal, pointsPerUnit);

    bool ok = false;
    const KoUnit unit = KoUnit::fromSymbol(symbol, &ok);
    QVERIFY(ok);
    QCOMPARE(unit.symbol(), symbol);
    QVERIFY(!KoUnit::unitDescription(unit.type()).isEmpty());
    QVERIFY(qAbs(unit.fromUserValue(1.0) - pointsPerUnit) < 1e-6);

    const KoUnit inch = KoUnit::fromSymbol(QStringLiteral("inch"), &ok);
    QVERIFY(ok);
    QCOMPARE(inch.symbol(), QStringLiteral("in"));
    QVERIFY(qAbs(inch.fromUserValue(1.0) - 72.0) < 1e-12);
}

void KoUnitContractTest::unitPickerMapsDisplayedChoicesToUnits()
{
    // Consumer: document-unit menus and selection grow, shrink, feather, and border dialogs.
    // Operation: A user chooses an entry from the displayed unit list.
    // Observable result: Every displayed entry resolves back to its selected unit, and hiding pixels removes only that choice.
    // Failure impact: A dialog applies the wrong unit to a document or selection operation.
    const QStringList displayedUnits = KoUnit::listOfUnitNameForUi();
    QVERIFY(!displayedUnits.isEmpty());

    for (int index = 0; index < displayedUnits.size(); ++index) {
        const KoUnit unit = KoUnit::fromListForUi(index, KoUnit::ListAll);
        QVERIFY(!displayedUnits.at(index).isEmpty());
        QCOMPARE(displayedUnits.at(index), KoUnit::unitDescription(unit.type()));
        QCOMPARE(unit.indexInListForUi(KoUnit::ListAll), index);
    }

    const QStringList unitsWithoutPixels = KoUnit::listOfUnitNameForUi(KoUnit::HidePixel);
    const int pixelIndex = KoUnit(KoUnit::Pixel).indexInListForUi(KoUnit::ListAll);
    QVERIFY(pixelIndex >= 0);
    QCOMPARE(KoUnit::fromListForUi(pixelIndex).symbol(), QStringLiteral("px"));
    QCOMPARE(KoUnit(KoUnit::Pixel).indexInListForUi(KoUnit::HidePixel), -1);

    for (int index = 0; index < unitsWithoutPixels.size(); ++index) {
        const KoUnit unit = KoUnit::fromListForUi(index, KoUnit::HidePixel);
        QCOMPARE(unitsWithoutPixels.at(index), KoUnit::unitDescription(unit.type()));
        QCOMPARE(unit.indexInListForUi(KoUnit::HidePixel), index);
        QVERIFY(unit.symbol() != QStringLiteral("px"));
    }
}

void KoUnitContractTest::userConversionsPreservePhysicalLengths()
{
    // Consumer: canvas, shape, and screentone controls converting user lengths and resolutions.
    // Operation: A user changes between millimeters, inches, points, and resolution-scaled pixels.
    // Observable result: Equivalent physical lengths retain their size after conversion and text entry.
    // Failure impact: A canvas, shape, or generated screentone changes size when its displayed unit changes.
    bool ok = false;
    const KoUnit millimeter = KoUnit::fromSymbol(QStringLiteral("mm"), &ok);
    QVERIFY(ok);
    const KoUnit inch = KoUnit::fromSymbol(QStringLiteral("in"), &ok);
    QVERIFY(ok);
    const KoUnit point = KoUnit::fromSymbol(QStringLiteral("pt"), &ok);
    QVERIFY(ok);

    QVERIFY(qAbs(millimeter.toUserValuePrecise(72.0) - 25.4) < 1e-3);
    QVERIFY(qAbs(millimeter.fromUserValue(25.4) - 72.0) < 1e-3);
    QVERIFY(qAbs(KoUnit::convertFromUnitToUnit(25.4, millimeter, inch) - 1.0) < 1e-4);
    QVERIFY(qAbs(KoUnit::convertFromUnitToUnit(1.0, inch, point) - 72.0) < 1e-12);

    const KoUnit pixel = KoUnit::fromListForUi(
        KoUnit(KoUnit::Pixel).indexInListForUi(), KoUnit::ListAll, 2.0);
    QCOMPARE(pixel.symbol(), QStringLiteral("px"));
    QCOMPARE(pixel.toUserValuePrecise(3.0), 6.0);
    QCOMPARE(pixel.fromUserValue(6.0), 3.0);

    const QString userValue = millimeter.toUserStringValue(72.0);
    QVERIFY(qAbs(millimeter.fromUserValue(userValue, &ok) - 72.0) < 1e-3);
    QVERIFY(ok);
}

void KoUnitContractTest::pixelTransformUpdatesScale()
{
    // Consumer: transformed shape and pixel-based canvas controls.
    // Operation: A pixel unit follows a uniform transformation applied to its shape.
    // Observable result: The displayed pixel value scales with the transformed geometry.
    // Failure impact: Pixel-based dimensions no longer match the transformed shape on canvas.
    QTransform transform;
    transform.scale(3.0, 3.0);
    QCOMPARE(KoUnit::approxTransformScale(transform), 3.0);

    KoUnit pixel(KoUnit::Pixel, 2.0);
    pixel.adjustByPixelTransform(transform);
    QCOMPARE(pixel.toUserValuePrecise(4.0), 24.0);
}

QTEST_GUILESS_MAIN(KoUnitContractTest)

#include "KoUnitContractTest.moc"

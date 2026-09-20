/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisHandleStyle.h"

#include <QTest>

namespace
{
void verifyDashedStyle(const KisHandleStyle &style,
                       const QColor &baseColor,
                       const QColor &handleFill,
                       const QColor &outlineColor)
{
    QCOMPARE(style.lineIterations.size(), 2);
    QCOMPARE(style.handleIterations.size(), 1);

    const KisHandleStyle::IterationStyle &outline = style.lineIterations[0];
    QVERIFY(outline.isValid);
    QCOMPARE(outline.stylePair.first.style(), Qt::SolidLine);
    QCOMPARE(outline.stylePair.first.color(), outlineColor);
    QCOMPARE(outline.stylePair.second.style(), Qt::NoBrush);

    const KisHandleStyle::IterationStyle &ants = style.lineIterations[1];
    QVERIFY(ants.isValid);
    QCOMPARE(ants.stylePair.first.style(), Qt::CustomDashLine);
    QCOMPARE(ants.stylePair.first.color(), baseColor);
    QCOMPARE(ants.stylePair.second.style(), Qt::NoBrush);

    const KisHandleStyle::IterationStyle &handle = style.handleIterations[0];
    QVERIFY(handle.isValid);
    QCOMPARE(handle.stylePair.first.color(), baseColor);
    QCOMPARE(handle.stylePair.first.width(), 2);
    QVERIFY(handle.stylePair.first.isCosmetic());
    QCOMPARE(handle.stylePair.first.joinStyle(), Qt::RoundJoin);
    QCOMPARE(handle.stylePair.second.color(), handleFill);
}

KisHandlePalette distinctivePalette()
{
    KisHandlePalette palette;
    palette.primaryColor = QColor(1, 2, 3);
    palette.secondaryColor = QColor(4, 5, 6);
    palette.gradientFillColor = QColor(7, 8, 9);
    palette.highlightColor = QColor(10, 11, 12);
    palette.highlightOutlineColor = QColor(13, 14, 15);
    palette.selectionColor = QColor(16, 17, 18);
    palette.white = QColor(19, 20, 21);
    palette.black = QColor(22, 23, 24);
    return palette;
}
}

class KisHandleStyleContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paletteProvidesStableDefaultColors();
    void iterationStylesDistinguishInheritanceFromPainting();
    void inheritStyleUsesInvalidIterationsAndStableStorage();
    void standardStylesUsePaletteRolesAndSharedGradientArrows();
    void highlightedStylesUseHighlightAndSelectionRoles();
};

void KisHandleStyleContractTest::paletteProvidesStableDefaultColors()
{
    const KisHandlePalette palette;
    QCOMPARE(palette.primaryColor, QColor(0, 0, 90, 180));
    QCOMPARE(palette.secondaryColor, QColor(0, 0, 255, 127));
    QCOMPARE(palette.gradientFillColor, QColor(255, 197, 39));
    QCOMPARE(palette.highlightColor, QColor(255, 100, 100));
    QCOMPARE(palette.highlightOutlineColor, QColor(155, 0, 0));
    QCOMPARE(palette.selectionColor, QColor(164, 227, 243));
    QCOMPARE(palette.white, QColor(Qt::white));
    QCOMPARE(palette.black, QColor(Qt::black));
}

void KisHandleStyleContractTest::iterationStylesDistinguishInheritanceFromPainting()
{
    const KisHandleStyle::IterationStyle inherited;
    QVERIFY(!inherited.isValid);

    const QPen pen(QColor(Qt::red), 3.0);
    const QBrush brush {QColor(Qt::green)};
    const KisHandleStyle::IterationStyle painted(pen, brush);
    QVERIFY(painted.isValid);
    QCOMPARE(painted.stylePair.first, pen);
    QCOMPARE(painted.stylePair.second, brush);
}

void KisHandleStyleContractTest::inheritStyleUsesInvalidIterationsAndStableStorage()
{
    KisHandleStyle &style = KisHandleStyle::inheritStyle();
    QCOMPARE(&KisHandleStyle::inheritStyle(), &style);
    QCOMPARE(style.handleIterations.size(), 1);
    QCOMPARE(style.lineIterations.size(), 1);
    QVERIFY(!style.handleIterations.first().isValid);
    QVERIFY(!style.lineIterations.first().isValid);
}

void KisHandleStyleContractTest::standardStylesUsePaletteRolesAndSharedGradientArrows()
{
    const KisHandlePalette palette = distinctivePalette();
    KisHandleStyle &primary = KisHandleStyle::primarySelection(palette);
    verifyDashedStyle(primary, palette.primaryColor, palette.white, palette.white);
    verifyDashedStyle(KisHandleStyle::secondarySelection(palette),
                      palette.secondaryColor,
                      palette.white,
                      palette.white);
    verifyDashedStyle(KisHandleStyle::gradientHandles(palette),
                      palette.primaryColor,
                      palette.gradientFillColor,
                      palette.white);
    QCOMPARE(&KisHandleStyle::gradientArrows(palette), &primary);
}

void KisHandleStyleContractTest::highlightedStylesUseHighlightAndSelectionRoles()
{
    const KisHandlePalette palette = distinctivePalette();
    verifyDashedStyle(KisHandleStyle::highlightedPrimaryHandles(palette),
                      palette.highlightOutlineColor,
                      palette.highlightColor,
                      palette.white);
    verifyDashedStyle(KisHandleStyle::partiallyHighlightedPrimaryHandles(palette),
                      palette.highlightOutlineColor,
                      palette.selectionColor,
                      palette.white);
    verifyDashedStyle(KisHandleStyle::selectedPrimaryHandles(palette),
                      palette.primaryColor,
                      palette.selectionColor,
                      palette.white);

    const KisHandleStyle &solid =
        KisHandleStyle::highlightedPrimaryHandlesWithSolidOutline(palette);
    QCOMPARE(solid.handleIterations.size(), 1);
    QCOMPARE(solid.lineIterations.size(), 1);
    QVERIFY(solid.handleIterations.first().isValid);
    QVERIFY(solid.lineIterations.first().isValid);
    QCOMPARE(solid.handleIterations.first().stylePair.first.color(),
             palette.highlightOutlineColor);
    QCOMPARE(solid.handleIterations.first().stylePair.second.color(),
             palette.highlightColor);
    QCOMPARE(solid.lineIterations.first().stylePair.first.style(), Qt::SolidLine);
    QCOMPARE(solid.lineIterations.first().stylePair.first.color(),
             palette.highlightOutlineColor);
}

QTEST_GUILESS_MAIN(KisHandleStyleContractTest)

#include "KisHandleStyleContractTest.moc"

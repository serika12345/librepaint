/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <canvas/kis_grid_config.h>

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_GRID_CONFIG_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisGridConfig::method)), signature>)
} // namespace

class KisGridConfigValueContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typesEnumsAndTrigoCacheValuesRemainStable();
    void rectangularGridStateSignaturesRemainStable();
    void isometricGridStateSignaturesRemainStable();
    void gridPresentationSignaturesRemainStable();
    void gridEqualitySignatureRemainsStable();
};

void KisGridConfigValueContractTest::typesEnumsAndTrigoCacheValuesRemainStable()
{
    static_assert(std::is_class_v<KisGridConfig>);
    static_assert(std::is_enum_v<KisGridConfig::GridType>);
    static_assert(std::is_enum_v<KisGridConfig::LineTypeInternal>);

    QCOMPARE(int(KisGridConfig::GRID_RECTANGULAR), 0);
    QCOMPARE(int(KisGridConfig::GRID_ISOMETRIC_LEGACY), 1);
    QCOMPARE(int(KisGridConfig::GRID_ISOMETRIC), 2);

    QCOMPARE(int(KisGridConfig::LINE_SOLID), 0);
    QCOMPARE(int(KisGridConfig::LINE_DASHED), 1);
    QCOMPARE(int(KisGridConfig::LINE_DOTTED), 2);
    QCOMPARE(int(KisGridConfig::LINE_NONE), 3);

    KisGridConfig::TrigoCache cache{
        1.25,
        2.5,
        3.75,
        4.25,
        5.5,
        6.75,
        8.0,
    };

    static_assert(std::is_same_v<decltype(cache.correctedAngleRightCellSize), qreal>);
    static_assert(std::is_same_v<decltype(cache.correctedAngleRightOffsetX), qreal>);
    static_assert(std::is_same_v<decltype(cache.tanAngleRight), qreal>);
    static_assert(std::is_same_v<decltype(cache.correctedAngleLeftCellSize), qreal>);
    static_assert(std::is_same_v<decltype(cache.correctedAngleLeftOffsetX), qreal>);
    static_assert(std::is_same_v<decltype(cache.tanAngleLeft), qreal>);
    static_assert(std::is_same_v<decltype(cache.verticalSpace), qreal>);

    QCOMPARE(cache.correctedAngleRightCellSize, 1.25);
    QCOMPARE(cache.correctedAngleRightOffsetX, 2.5);
    QCOMPARE(cache.tanAngleRight, 3.75);
    QCOMPARE(cache.correctedAngleLeftCellSize, 4.25);
    QCOMPARE(cache.correctedAngleLeftOffsetX, 5.5);
    QCOMPARE(cache.tanAngleLeft, 6.75);
    QCOMPARE(cache.verticalSpace, 8.0);

    KisGridConfig::TrigoCache copy = cache;
    copy.correctedAngleRightCellSize = 9.0;
    copy.correctedAngleLeftOffsetX = 10.0;
    copy.verticalSpace = 11.0;

    QCOMPARE(cache.correctedAngleRightCellSize, 1.25);
    QCOMPARE(cache.correctedAngleLeftOffsetX, 5.5);
    QCOMPARE(cache.verticalSpace, 8.0);
    QCOMPARE(copy.correctedAngleRightCellSize, 9.0);
    QCOMPARE(copy.correctedAngleLeftOffsetX, 10.0);
    QCOMPARE(copy.verticalSpace, 11.0);
}

void KisGridConfigValueContractTest::rectangularGridStateSignaturesRemainStable()
{
    ASSERT_GRID_CONFIG_SIGNATURE(showGrid, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setShowGrid, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(snapToGrid, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSnapToGrid, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(spacing, QPoint (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSpacing, void (KisGridConfig::*)(const QPoint &));
    ASSERT_GRID_CONFIG_SIGNATURE(xSpacingActive, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setXSpacingActive, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(ySpacingActive, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setYSpacingActive, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(offsetActive, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setOffsetActive, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(offset, QPoint (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setOffset, void (KisGridConfig::*)(const QPoint &));
    ASSERT_GRID_CONFIG_SIGNATURE(offsetAspectLocked, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setOffsetAspectLocked, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(spacingAspectLocked, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSpacingAspectLocked, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(subdivision, int (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSubdivision, void (KisGridConfig::*)(int));

    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().showGrid()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().snapToGrid()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().spacing()), QPoint>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().xSpacingActive()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().ySpacingActive()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().offsetActive()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().offset()), QPoint>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().offsetAspectLocked()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().spacingAspectLocked()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().subdivision()), int>);
}

void KisGridConfigValueContractTest::isometricGridStateSignaturesRemainStable()
{
    ASSERT_GRID_CONFIG_SIGNATURE(angleLeft, qreal (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleLeft, void (KisGridConfig::*)(qreal));
    ASSERT_GRID_CONFIG_SIGNATURE(angleRight, qreal (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleRight, void (KisGridConfig::*)(qreal));
    ASSERT_GRID_CONFIG_SIGNATURE(angleLeftActive, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleLeftActive, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(angleRightActive, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleRightActive, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(angleAspectLocked, bool (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleAspectLocked, void (KisGridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(cellSpacing, int (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setCellSpacing, void (KisGridConfig::*)(int));
    ASSERT_GRID_CONFIG_SIGNATURE(cellSize, int (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setCellSize, void (KisGridConfig::*)(int));
    ASSERT_GRID_CONFIG_SIGNATURE(gridType, KisGridConfig::GridType (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setGridType, void (KisGridConfig::*)(KisGridConfig::GridType));

    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().angleLeft()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().angleRight()), qreal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().angleLeftActive()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().angleRightActive()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().angleAspectLocked()), bool>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().cellSpacing()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().cellSize()), int>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().gridType()), KisGridConfig::GridType>);
}

void KisGridConfigValueContractTest::gridPresentationSignaturesRemainStable()
{
    ASSERT_GRID_CONFIG_SIGNATURE(lineTypeMain, KisGridConfig::LineTypeInternal (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setLineTypeMain, void (KisGridConfig::*)(KisGridConfig::LineTypeInternal));
    ASSERT_GRID_CONFIG_SIGNATURE(lineTypeSubdivision, KisGridConfig::LineTypeInternal (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setLineTypeSubdivision, void (KisGridConfig::*)(KisGridConfig::LineTypeInternal));
    ASSERT_GRID_CONFIG_SIGNATURE(lineTypeVertical, KisGridConfig::LineTypeInternal (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setLineTypeVertical, void (KisGridConfig::*)(KisGridConfig::LineTypeInternal));
    ASSERT_GRID_CONFIG_SIGNATURE(colorMain, QColor (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setColorMain, void (KisGridConfig::*)(const QColor &));
    ASSERT_GRID_CONFIG_SIGNATURE(colorSubdivision, QColor (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setColorSubdivision, void (KisGridConfig::*)(const QColor &));
    ASSERT_GRID_CONFIG_SIGNATURE(colorVertical, QColor (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setColorVertical, void (KisGridConfig::*)(const QColor &));
    ASSERT_GRID_CONFIG_SIGNATURE(penMain, QPen (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(penSubdivision, QPen (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(penVertical, QPen (KisGridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(trigoCache, KisGridConfig::TrigoCache (KisGridConfig::*)() const);

    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().lineTypeMain()),
                                 KisGridConfig::LineTypeInternal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().lineTypeSubdivision()),
                                 KisGridConfig::LineTypeInternal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().lineTypeVertical()),
                                 KisGridConfig::LineTypeInternal>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().colorMain()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().colorSubdivision()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().colorVertical()), QColor>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().penMain()), QPen>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().penSubdivision()), QPen>);
    static_assert(std::is_same_v<decltype(std::declval<const KisGridConfig &>().penVertical()), QPen>);
    static_assert(
        std::is_same_v<decltype(std::declval<const KisGridConfig &>().trigoCache()), KisGridConfig::TrigoCache>);
}

void KisGridConfigValueContractTest::gridEqualitySignatureRemainsStable()
{
    ASSERT_GRID_CONFIG_SIGNATURE(operator==, bool (KisGridConfig::*)(const KisGridConfig &) const);
}

#undef ASSERT_GRID_CONFIG_SIGNATURE

QTEST_GUILESS_MAIN(KisGridConfigValueContractTest)

#include "KisGridConfigValueContractTest.moc"

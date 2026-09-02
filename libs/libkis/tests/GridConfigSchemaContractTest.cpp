/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "GridConfig.h"
#include "GuidesConfig.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_GRID_CONFIG_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&GridConfig::method)), signature>)
#define ASSERT_GUIDES_CONFIG_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&GuidesConfig::method)), signature>)
} // namespace

class GridConfigSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void gridIdentityAndVisibilityPolicySignaturesRemainStable();
    void rectangularGridSpacingAndOffsetSignaturesRemainStable();
    void isometricGridGeometrySignaturesRemainStable();
    void gridLinePresentationSignaturesRemainStable();
    void gridColorPresentationSignaturesRemainStable();
    void guidesConfigIdentityAndConstructionSchemaRemainsStable();
    void guidesConfigComparisonAndSerializationSignaturesRemainStable();
    void guidesConfigPresentationSignaturesRemainStable();
    void guidesConfigPositionSignaturesRemainStable();
    void guidesConfigInteractionStateSignaturesRemainStable();
};

void GridConfigSchemaContractTest::gridIdentityAndVisibilityPolicySignaturesRemainStable()
{
    static_assert(std::is_class_v<GridConfig>);
    static_assert(std::is_base_of_v<QObject, GridConfig>);

    ASSERT_GRID_CONFIG_SIGNATURE(operator==, bool (GridConfig::*)(const GridConfig &) const);
    ASSERT_GRID_CONFIG_SIGNATURE(operator!=, bool (GridConfig::*)(const GridConfig &) const);
    ASSERT_GRID_CONFIG_SIGNATURE(visible, bool (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setVisible, void (GridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(snap, bool (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSnap, void (GridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(type, QString (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setType, void (GridConfig::*)(const QString &));

    QVERIFY(true);
}

void GridConfigSchemaContractTest::rectangularGridSpacingAndOffsetSignaturesRemainStable()
{
    ASSERT_GRID_CONFIG_SIGNATURE(offset, QPoint (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setOffset, void (GridConfig::*)(QPoint));
    ASSERT_GRID_CONFIG_SIGNATURE(offsetAspectLocked, bool (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setOffsetAspectLocked, void (GridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(spacing, QPoint (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSpacing, void (GridConfig::*)(QPoint));
    ASSERT_GRID_CONFIG_SIGNATURE(spacingActiveHorizontal, bool (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSpacingActiveHorizontal, void (GridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(spacingActiveVertical, bool (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSpacingActiveVertical, void (GridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(spacingAspectLocked, bool (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSpacingAspectLocked, void (GridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(subdivision, int (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setSubdivision, void (GridConfig::*)(int));

    QVERIFY(true);
}

void GridConfigSchemaContractTest::isometricGridGeometrySignaturesRemainStable()
{
    ASSERT_GRID_CONFIG_SIGNATURE(angleLeft, qreal (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleLeft, void (GridConfig::*)(qreal));
    ASSERT_GRID_CONFIG_SIGNATURE(angleRight, qreal (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleRight, void (GridConfig::*)(qreal));
    ASSERT_GRID_CONFIG_SIGNATURE(angleLeftActive, bool (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleLeftActive, void (GridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(angleRightActive, bool (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleRightActive, void (GridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(angleAspectLocked, bool (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setAngleAspectLocked, void (GridConfig::*)(bool));
    ASSERT_GRID_CONFIG_SIGNATURE(cellSpacing, int (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setCellSpacing, void (GridConfig::*)(int));
    ASSERT_GRID_CONFIG_SIGNATURE(cellSize, int (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setCellSize, void (GridConfig::*)(int));

    QVERIFY(true);
}

void GridConfigSchemaContractTest::gridLinePresentationSignaturesRemainStable()
{
    ASSERT_GRID_CONFIG_SIGNATURE(lineTypeMain, QString (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setLineTypeMain, void (GridConfig::*)(const QString &));
    ASSERT_GRID_CONFIG_SIGNATURE(lineTypeSubdivision, QString (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setLineTypeSubdivision, void (GridConfig::*)(const QString &));
    ASSERT_GRID_CONFIG_SIGNATURE(lineTypeVertical, QString (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setLineTypeVertical, void (GridConfig::*)(const QString &));

    QVERIFY(true);
}

void GridConfigSchemaContractTest::gridColorPresentationSignaturesRemainStable()
{
    ASSERT_GRID_CONFIG_SIGNATURE(colorMain, QColor (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setColorMain, void (GridConfig::*)(QColor));
    ASSERT_GRID_CONFIG_SIGNATURE(colorSubdivision, QColor (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setColorSubdivision, void (GridConfig::*)(QColor));
    ASSERT_GRID_CONFIG_SIGNATURE(colorVertical, QColor (GridConfig::*)() const);
    ASSERT_GRID_CONFIG_SIGNATURE(setColorVertical, void (GridConfig::*)(QColor));

    QVERIFY(true);
}

void GridConfigSchemaContractTest::guidesConfigIdentityAndConstructionSchemaRemainsStable()
{
    static_assert(std::is_class_v<GuidesConfig>);
    static_assert(std::is_base_of_v<QObject, GuidesConfig>);
    static_assert(std::is_constructible_v<GuidesConfig, KisGuidesConfig *>);
    static_assert(std::is_default_constructible_v<GuidesConfig>);
    static_assert(std::is_destructible_v<GuidesConfig>);
    static_assert(std::has_virtual_destructor_v<GuidesConfig>);
}

void GridConfigSchemaContractTest::guidesConfigComparisonAndSerializationSignaturesRemainStable()
{
    ASSERT_GUIDES_CONFIG_SIGNATURE(operator==, bool (GuidesConfig::*)(const GuidesConfig &) const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(operator!=, bool (GuidesConfig::*)(const GuidesConfig &) const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(fromXml, bool (GuidesConfig::*)(const QString &) const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(toXml, QString (GuidesConfig::*)() const);
}

void GridConfigSchemaContractTest::guidesConfigPresentationSignaturesRemainStable()
{
    ASSERT_GUIDES_CONFIG_SIGNATURE(color, QColor (GuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setColor, void (GuidesConfig::*)(const QColor &) const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(lineType, QString (GuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setLineType, void (GuidesConfig::*)(const QString &));
}

void GridConfigSchemaContractTest::guidesConfigPositionSignaturesRemainStable()
{
    ASSERT_GUIDES_CONFIG_SIGNATURE(hasGuides, bool (GuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(hasSamePositionAs, bool (GuidesConfig::*)(const GuidesConfig &) const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(horizontalGuides, QList<qreal> (GuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setHorizontalGuides, void (GuidesConfig::*)(const QList<qreal> &));
    ASSERT_GUIDES_CONFIG_SIGNATURE(verticalGuides, QList<qreal> (GuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setVerticalGuides, void (GuidesConfig::*)(const QList<qreal> &));
}

void GridConfigSchemaContractTest::guidesConfigInteractionStateSignaturesRemainStable()
{
    ASSERT_GUIDES_CONFIG_SIGNATURE(removeAllGuides, void (GuidesConfig::*)());
    ASSERT_GUIDES_CONFIG_SIGNATURE(visible, bool (GuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setVisible, void (GuidesConfig::*)(bool));
    ASSERT_GUIDES_CONFIG_SIGNATURE(locked, bool (GuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setLocked, void (GuidesConfig::*)(bool));
    ASSERT_GUIDES_CONFIG_SIGNATURE(snap, bool (GuidesConfig::*)() const);
    ASSERT_GUIDES_CONFIG_SIGNATURE(setSnap, void (GuidesConfig::*)(bool));
}

#undef ASSERT_GRID_CONFIG_SIGNATURE
#undef ASSERT_GUIDES_CONFIG_SIGNATURE

QTEST_GUILESS_MAIN(GridConfigSchemaContractTest)

#include "GridConfigSchemaContractTest.moc"

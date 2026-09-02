/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <nodes/kis_node_view_color_scheme.h>

#include <QRect>
#include <QTest>

#include <type_traits>

class KisNodeViewColorSchemeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeViewColorSchemeIdentitySchemaRemainsStable();
    void nodeViewColorSchemePrimitiveMetricsSchemaRemainsStable();
    void nodeViewColorSchemeLayoutSchemaRemainsStable();
    void nodeViewColorSchemeRelativeGeometrySchemaRemainsStable();
    void nodeViewColorSchemeColorSchemaRemainsStable();
};

void KisNodeViewColorSchemeSchemaContractTest::nodeViewColorSchemeIdentitySchemaRemainsStable()
{
    using Scheme = KisNodeViewColorScheme;

    static_assert(std::is_class_v<Scheme>);
    static_assert(std::is_default_constructible_v<Scheme>);
    static_assert(std::is_destructible_v<Scheme>);
    static_assert(std::is_same_v<decltype(&Scheme::instance), Scheme *(*)()>);
}

void KisNodeViewColorSchemeSchemaContractTest::nodeViewColorSchemePrimitiveMetricsSchemaRemainsStable()
{
    using Scheme = KisNodeViewColorScheme;
    using Metric = int (Scheme::*)() const;

    static_assert(std::is_same_v<decltype(&Scheme::visibilitySize), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::visibilityMargin), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::thumbnailSize), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::thumbnailMargin), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::decorationSize), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::decorationMargin), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::textMargin), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::iconSize), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::iconMargin), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::border), Metric>);
}

void KisNodeViewColorSchemeSchemaContractTest::nodeViewColorSchemeLayoutSchemaRemainsStable()
{
    using Scheme = KisNodeViewColorScheme;
    using Metric = int (Scheme::*)() const;

    static_assert(std::is_same_v<decltype(&Scheme::rowHeight), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::visibilityColumnWidth), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::indentation), Metric>);
    static_assert(std::is_same_v<decltype(&Scheme::selectedButtonColumnWidth), Metric>);
}

void KisNodeViewColorSchemeSchemaContractTest::nodeViewColorSchemeRelativeGeometrySchemaRemainsStable()
{
    using Scheme = KisNodeViewColorScheme;
    using Geometry = QRect (Scheme::*)() const;

    static_assert(std::is_same_v<decltype(&Scheme::relVisibilityRect), Geometry>);
    static_assert(std::is_same_v<decltype(&Scheme::relThumbnailRect), Geometry>);
    static_assert(std::is_same_v<decltype(&Scheme::relDecorationRect), Geometry>);
    static_assert(std::is_same_v<decltype(&Scheme::relExpandButtonRect), Geometry>);
}

void KisNodeViewColorSchemeSchemaContractTest::nodeViewColorSchemeColorSchemaRemainsStable()
{
    using Scheme = KisNodeViewColorScheme;

    static_assert(std::is_same_v<decltype(&Scheme::gridColor),
                                 QColor (Scheme::*)(const QStyleOptionViewItem &, QTreeView *) const>);
    static_assert(std::is_same_v<decltype(&Scheme::colorFromLabelIndex), QColor (Scheme::*)(int) const>);
    static_assert(std::is_same_v<decltype(&Scheme::allColorLabels), QVector<QColor> (Scheme::*)() const>);
}

QTEST_GUILESS_MAIN(KisNodeViewColorSchemeSchemaContractTest)

#include "KisNodeViewColorSchemeSchemaContractTest.moc"

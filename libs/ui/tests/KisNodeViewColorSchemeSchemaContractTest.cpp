/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <NodeView.h>
#include <nodes/kis_node_view_color_scheme.h>

#include <QRect>
#include <QTest>

#include <type_traits>

#define ASSERT_NODE_VIEW_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&NodeView::method)), signature>)

class KisNodeViewColorSchemeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nodeViewColorSchemeIdentitySchemaRemainsStable();
    void nodeViewColorSchemePrimitiveMetricsSchemaRemainsStable();
    void nodeViewColorSchemeLayoutSchemaRemainsStable();
    void nodeViewColorSchemeRelativeGeometrySchemaRemainsStable();
    void nodeViewColorSchemeColorSchemaRemainsStable();
    void nodeViewTypeColumnAndLifetimeSchemaRemainStable();
    void nodeViewModelAndPaintingSignaturesRemainStable();
    void nodeViewDragAndMouseEventSignaturesRemainStable();
    void nodeViewNodeOperationSignaturesRemainStable();
    void nodeViewNotificationAndUpdateSignaturesRemainStable();
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

void KisNodeViewColorSchemeSchemaContractTest::nodeViewTypeColumnAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<NodeView>);
    static_assert(std::is_enum_v<NodeView::ColumnIndex>);
    static_assert(NodeView::DEFAULT_COL == 0);
    static_assert(NodeView::VISIBILITY_COL == 1);
    static_assert(NodeView::SELECTED_COL == 2);
    static_assert(std::is_constructible_v<NodeView, QWidget *>);
    static_assert(std::has_virtual_destructor_v<NodeView>);
}

void KisNodeViewColorSchemeSchemaContractTest::nodeViewModelAndPaintingSignaturesRemainStable()
{
    ASSERT_NODE_VIEW_SIGNATURE(setModel, void (NodeView::*)(QAbstractItemModel *));
    ASSERT_NODE_VIEW_SIGNATURE(resizeEvent, void (NodeView::*)(QResizeEvent *));
    ASSERT_NODE_VIEW_SIGNATURE(paintEvent, void (NodeView::*)(QPaintEvent *));
    ASSERT_NODE_VIEW_SIGNATURE(drawBranches, void (NodeView::*)(QPainter *, const QRect &, const QModelIndex &) const);
}

void KisNodeViewColorSchemeSchemaContractTest::nodeViewDragAndMouseEventSignaturesRemainStable()
{
    ASSERT_NODE_VIEW_SIGNATURE(dragEnterEvent, void (NodeView::*)(QDragEnterEvent *));
    ASSERT_NODE_VIEW_SIGNATURE(dragLeaveEvent, void (NodeView::*)(QDragLeaveEvent *));
    ASSERT_NODE_VIEW_SIGNATURE(dragMoveEvent, void (NodeView::*)(QDragMoveEvent *));
    ASSERT_NODE_VIEW_SIGNATURE(dropEvent, void (NodeView::*)(QDropEvent *));
    ASSERT_NODE_VIEW_SIGNATURE(mousePressEvent, void (NodeView::*)(QMouseEvent *));
}

void KisNodeViewColorSchemeSchemaContractTest::nodeViewNodeOperationSignaturesRemainStable()
{
    ASSERT_NODE_VIEW_SIGNATURE(addPropertyActions, void (NodeView::*)(QMenu *, const QModelIndex &));
    ASSERT_NODE_VIEW_SIGNATURE(toggleSolo, void (NodeView::*)(const QModelIndex &));
    ASSERT_NODE_VIEW_SIGNATURE(updateNode, void (NodeView::*)(const QModelIndex &));
}

void KisNodeViewColorSchemeSchemaContractTest::nodeViewNotificationAndUpdateSignaturesRemainStable()
{
    ASSERT_NODE_VIEW_SIGNATURE(contextMenuRequested, void (NodeView::*)(const QPoint &, const QModelIndex &));
    ASSERT_NODE_VIEW_SIGNATURE(selectionChanged, void (NodeView::*)(const QModelIndexList &));
    ASSERT_NODE_VIEW_SIGNATURE(slotConfigurationChanged, void (NodeView::*)());
    ASSERT_NODE_VIEW_SIGNATURE(slotScrollerStateChanged, void (NodeView::*)(QScroller::State));
    ASSERT_NODE_VIEW_SIGNATURE(slotUpdateIcons, void (NodeView::*)());
}

QTEST_GUILESS_MAIN(KisNodeViewColorSchemeSchemaContractTest)

#include "KisNodeViewColorSchemeSchemaContractTest.moc"

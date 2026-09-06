/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "KisAnimTimelineFramesView.h"

#define ASSERT_VIEW_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(&KisAnimTimelineFramesView::method), signature>)

class KisAnimTimelineFramesViewSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeModelLifetimeAndDisplaySchemaRemainStable();
    void selectionAndRangeUpdateSchemaRemainStable();
    void layerAndKeyframeInsertionSchemaRemainStable();
    void frameRemovalTransferAndCacheSchemaRemainStable();
    void audioAndScrollSchemaRemainStable();
};

void KisAnimTimelineFramesViewSchemaContractTest::typeModelLifetimeAndDisplaySchemaRemainStable()
{
    using View = KisAnimTimelineFramesView;

    static_assert(std::is_class_v<View>);
    static_assert(std::is_base_of_v<QTableView, View>);
    static_assert(std::is_enum_v<TimelineDirection>);
    static_assert(std::is_same_v<std::underlying_type_t<TimelineDirection>, short>);
    static_assert(LEFT == -1);
    static_assert(RIGHT == 1);
    static_assert(BEFORE == LEFT);
    static_assert(AFTER == RIGHT);
    static_assert(std::is_constructible_v<View, QWidget *>);
    static_assert(std::has_virtual_destructor_v<View>);
    ASSERT_VIEW_SIGNATURE(calculateActiveLayerSelectedTimes, void (View::*)(const QModelIndexList &));
    ASSERT_VIEW_SIGNATURE(setActionManager, void (View::*)(KisActionManager *));
    ASSERT_VIEW_SIGNATURE(setModel, void (View::*)(QAbstractItemModel *));
    ASSERT_VIEW_SIGNATURE(slotCanvasUpdate, void (View::*)(KoCanvasBase *));
    ASSERT_VIEW_SIGNATURE(slotDataChanged, void (View::*)(const QModelIndex &, const QModelIndex &));
    ASSERT_VIEW_SIGNATURE(slotHeaderDataChanged, void (View::*)(Qt::Orientation, int, int));
    ASSERT_VIEW_SIGNATURE(updateGeometries, void (View::*)());

    QVERIFY(true);
}

void KisAnimTimelineFramesViewSchemaContractTest::selectionAndRangeUpdateSchemaRemainStable()
{
    using View = KisAnimTimelineFramesView;
    using NoArgument = void (View::*)();

    ASSERT_VIEW_SIGNATURE(slotColorLabelChanged, void (View::*)(int));
    ASSERT_VIEW_SIGNATURE(slotReselectCurrentIndex, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotSelectionChanged, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotSetEndTimeToCurrentPosition, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotSetStartTimeToCurrentPosition, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotTryTransferSelectionBetweenRows, void (View::*)(int, int));
    ASSERT_VIEW_SIGNATURE(slotUpdateFrameActions, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotUpdateIcons, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotUpdateInfiniteFramesCount, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotUpdateLayersMenu, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotUpdatePlaybackRange, NoArgument);

    QVERIFY(true);
}

void KisAnimTimelineFramesViewSchemaContractTest::layerAndKeyframeInsertionSchemaRemainStable()
{
    using View = KisAnimTimelineFramesView;
    using NoArgument = void (View::*)();

    ASSERT_VIEW_SIGNATURE(slotAddBlankFrame, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotAddDuplicateFrame, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotAddExistingLayer, void (View::*)(QAction *));
    ASSERT_VIEW_SIGNATURE(slotAddNewLayer, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertHoldFrame, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertHoldFrameColumn, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertKeyframeColumnLeft, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertKeyframeColumnRight, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertKeyframeLeft, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertKeyframeRight, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertMultipleHoldFrameColumns, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertMultipleHoldFrames, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertMultipleKeyframeColumns, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotInsertMultipleKeyframes, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotLayerContextMenuRequested, void (View::*)(const QPoint &));
    ASSERT_VIEW_SIGNATURE(slotRemoveLayer, NoArgument);

    QVERIFY(true);
}

void KisAnimTimelineFramesViewSchemaContractTest::frameRemovalTransferAndCacheSchemaRemainStable()
{
    using View = KisAnimTimelineFramesView;
    using NoArgument = void (View::*)();

    ASSERT_VIEW_SIGNATURE(slotClearCache, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotCopyColumns, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotCopyFrames, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotCutColumns, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotCutFrames, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotMakeClonesUnique, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotMirrorColumns, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotMirrorFrames, void (View::*)(bool));
    ASSERT_VIEW_SIGNATURE(slotPasteColumns, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotPasteFrames, void (View::*)(bool));
    ASSERT_VIEW_SIGNATURE(slotRemoveHoldFrame, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotRemoveHoldFrameColumn, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotRemoveMultipleHoldFrameColumns, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotRemoveMultipleHoldFrames, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotRemoveSelectedColumns, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotRemoveSelectedColumnsAndShift, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotRemoveSelectedFrames, void (View::*)(bool, bool));
    ASSERT_VIEW_SIGNATURE(slotRemoveSelectedFramesAndShift, NoArgument);

    QVERIFY(true);
}

void KisAnimTimelineFramesViewSchemaContractTest::audioAndScrollSchemaRemainStable()
{
    using View = KisAnimTimelineFramesView;
    using NoArgument = void (View::*)();

    ASSERT_VIEW_SIGNATURE(slotAudioChannelMute, void (View::*)(bool));
    ASSERT_VIEW_SIGNATURE(slotAudioChannelRemove, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotAudioVolumeChanged, void (View::*)(int));
    ASSERT_VIEW_SIGNATURE(slotEnsureRowVisible, void (View::*)(int));
    ASSERT_VIEW_SIGNATURE(slotFitViewToFrameRange, void (View::*)(int, int));
    ASSERT_VIEW_SIGNATURE(slotRealignScrollBars, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotScrollerStateChanged, void (View::*)(QScroller::State));
    ASSERT_VIEW_SIGNATURE(slotSelectAudioChannelFile, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotUpdateDragInfiniteFramesCount, NoArgument);
    ASSERT_VIEW_SIGNATURE(slotZoom, void (View::*)(qreal));

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisAnimTimelineFramesViewSchemaContractTest)

#include "KisAnimTimelineFramesViewSchemaContractTest.moc"

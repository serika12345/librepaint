/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "CommentModel.h"
#include "StoryboardModel.h"

#define ASSERT_MODEL_SIGNATURE(method, signature)                                                                      \
    static_assert(std::is_same_v<decltype(&StoryboardModel::method), signature>)
#define ASSERT_COMMENT_MODEL_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(&StoryboardCommentModel::method), signature>)

class StoryboardModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeRoleLifetimeAndLockSchemaRemainStable();
    void tableMimeAndRowSchemaRemainStable();
    void commentAndStoryboardDataSchemaRemainStable();
    void frameTimelineAndUndoSchemaRemainStable();
    void imageViewThumbnailAndActiveNodeSchemaRemainStable();
    void commentModelTypeConstructionAndNotificationSchemaRemainStable();
    void commentModelRowDataSignaturesRemainStable();
    void commentModelRowMutationSignaturesRemainStable();
    void commentModelMimeSignaturesRemainStable();
    void commentModelCollectionSignaturesRemainStable();
};

void StoryboardModelSchemaContractTest::typeRoleLifetimeAndLockSchemaRemainStable()
{
    using Model = StoryboardModel;
    using Lock = Model::KeyframeReorderLock;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_base_of_v<QAbstractItemModel, Model>);
    static_assert(std::is_class_v<Lock>);
    static_assert(std::is_enum_v<Model::AdditionalRoles>);
    static_assert(Model::TotalSceneDurationInFrames == Qt::UserRole + 1);
    static_assert(Model::TotalSceneDurationInSeconds == Qt::UserRole + 2);
    static_assert(std::is_constructible_v<Lock, Model *>);
    static_assert(std::is_destructible_v<Lock>);
    static_assert(std::is_constructible_v<Model, QObject *>);
    static_assert(std::has_virtual_destructor_v<Model>);
    ASSERT_MODEL_SIGNATURE(isFrozen, bool (Model::*)() const);
    ASSERT_MODEL_SIGNATURE(isLocked, bool (Model::*)() const);
    ASSERT_MODEL_SIGNATURE(setFreeze, void (Model::*)(bool));
    ASSERT_MODEL_SIGNATURE(setLocked, void (Model::*)(bool));

    QVERIFY(true);
}

void StoryboardModelSchemaContractTest::tableMimeAndRowSchemaRemainStable()
{
    using Model = StoryboardModel;
    using Count = int (Model::*)(const QModelIndex &) const;
    using RowOperation = bool (Model::*)(int, int, const QModelIndex &);
    using SupportedActions = Qt::DropActions (Model::*)() const;

    ASSERT_MODEL_SIGNATURE(columnCount, Count);
    ASSERT_MODEL_SIGNATURE(data, QVariant (Model::*)(const QModelIndex &, int) const);
    ASSERT_MODEL_SIGNATURE(dropMimeData,
                           bool (Model::*)(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &));
    ASSERT_MODEL_SIGNATURE(flags, Qt::ItemFlags (Model::*)(const QModelIndex &) const);
    ASSERT_MODEL_SIGNATURE(index, QModelIndex (Model::*)(int, int, const QModelIndex &) const);
    ASSERT_MODEL_SIGNATURE(insertRows, RowOperation);
    ASSERT_MODEL_SIGNATURE(mimeData, QMimeData * (Model::*)(const QModelIndexList &) const);
    ASSERT_MODEL_SIGNATURE(mimeTypes, QStringList (Model::*)() const);
    ASSERT_MODEL_SIGNATURE(moveRows, bool (Model::*)(const QModelIndex &, int, int, const QModelIndex &, int));
    ASSERT_MODEL_SIGNATURE(parent, QModelIndex (Model::*)(const QModelIndex &) const);
    ASSERT_MODEL_SIGNATURE(removeRows, RowOperation);
    ASSERT_MODEL_SIGNATURE(rowCount, Count);
    ASSERT_MODEL_SIGNATURE(setData, bool (Model::*)(const QModelIndex &, const QVariant &, int));
    ASSERT_MODEL_SIGNATURE(supportedDragActions, SupportedActions);
    ASSERT_MODEL_SIGNATURE(supportedDropActions, SupportedActions);

    QVERIFY(true);
}

void StoryboardModelSchemaContractTest::commentAndStoryboardDataSchemaRemainStable()
{
    using Model = StoryboardModel;
    using InsertCommand = void (Model::*)(int, KUndo2Command *);
    using InsertItem = void (Model::*)(int, StoryboardItemSP);

    ASSERT_MODEL_SIGNATURE(getComment, StoryboardComment (Model::*)(int) const);
    ASSERT_MODEL_SIGNATURE(getData, StoryboardItemList (Model::*)());
    static_assert(std::is_same_v<decltype(static_cast<InsertCommand>(&Model::insertChildRows)), InsertCommand>);
    static_assert(std::is_same_v<decltype(static_cast<InsertItem>(&Model::insertChildRows)), InsertItem>);
    ASSERT_MODEL_SIGNATURE(insertItem, bool (Model::*)(QModelIndex, bool));
    ASSERT_MODEL_SIGNATURE(resetData, void (Model::*)(StoryboardItemList));
    ASSERT_MODEL_SIGNATURE(setCommentModel, void (Model::*)(StoryboardCommentModel *));
    ASSERT_MODEL_SIGNATURE(setCommentScrollData, bool (Model::*)(const QModelIndex &, const QVariant &));
    ASSERT_MODEL_SIGNATURE(sigStoryboardItemListChanged, void (Model::*)());
    ASSERT_MODEL_SIGNATURE(totalCommentCount, int (Model::*)());
    ASSERT_MODEL_SIGNATURE(visibleCommentCount, int (Model::*)() const);
    ASSERT_MODEL_SIGNATURE(visibleCommentsUpto, int (Model::*)(QModelIndex) const);

    QVERIFY(true);
}

void StoryboardModelSchemaContractTest::frameTimelineAndUndoSchemaRemainStable()
{
    using Model = StoryboardModel;
    using CreateKeyframes = void (Model::*)(const QModelIndex &, KUndo2Command *);

    ASSERT_MODEL_SIGNATURE(affectedIndexes, QModelIndexList (Model::*)(KisTimeSpan) const);
    ASSERT_MODEL_SIGNATURE(changeSceneHoldLength, bool (Model::*)(int, QModelIndex));
    ASSERT_MODEL_SIGNATURE(createBlankKeyframes, CreateKeyframes);
    ASSERT_MODEL_SIGNATURE(createDuplicateKeyframes, CreateKeyframes);
    ASSERT_MODEL_SIGNATURE(getFramesPerSecond, int (Model::*)() const);
    ASSERT_MODEL_SIGNATURE(indexFromFrame, QModelIndex (Model::*)(int, bool) const);
    ASSERT_MODEL_SIGNATURE(lastIndexBeforeFrame, QModelIndex (Model::*)(int) const);
    ASSERT_MODEL_SIGNATURE(lastKeyframeGlobal, int (Model::*)() const);
    ASSERT_MODEL_SIGNATURE(lastKeyframeWithin, int (Model::*)(QModelIndex));
    ASSERT_MODEL_SIGNATURE(nextKeyframeGlobal, int (Model::*)(int) const);
    ASSERT_MODEL_SIGNATURE(pushUndoCommand, void (Model::*)(KUndo2Command *));
    ASSERT_MODEL_SIGNATURE(removeItem, bool (Model::*)(QModelIndex, KUndo2Command *));
    ASSERT_MODEL_SIGNATURE(reorderKeyframes, void (Model::*)());
    ASSERT_MODEL_SIGNATURE(shiftKeyframes, void (Model::*)(KisTimeSpan, int, KUndo2Command *));
    ASSERT_MODEL_SIGNATURE(updateDurationData, bool (Model::*)(const QModelIndex &));
    ASSERT_MODEL_SIGNATURE(visualizeScene, void (Model::*)(const QModelIndex &, bool));

    QVERIFY(true);
}

void StoryboardModelSchemaContractTest::imageViewThumbnailAndActiveNodeSchemaRemainStable()
{
    using Model = StoryboardModel;

    ASSERT_MODEL_SIGNATURE(setImage, void (Model::*)(KisImageWSP));
    ASSERT_MODEL_SIGNATURE(setThumbnailPixmapData, bool (Model::*)(const QModelIndex &, const KisPaintDeviceSP &));
    ASSERT_MODEL_SIGNATURE(setView, void (Model::*)(StoryboardView *));
    ASSERT_MODEL_SIGNATURE(slotSetActiveNode, void (Model::*)(KisNodeSP));
    ASSERT_MODEL_SIGNATURE(slotUpdateThumbnailsForItems, void (Model::*)(QModelIndexList));

    QVERIFY(true);
}

void StoryboardModelSchemaContractTest::commentModelTypeConstructionAndNotificationSchemaRemainStable()
{
    using Model = StoryboardCommentModel;

    static_assert(std::is_class_v<Model>);
    static_assert(std::is_base_of_v<QAbstractListModel, Model>);
    static_assert(std::is_constructible_v<Model, QObject *>);
    static_assert(std::has_virtual_destructor_v<Model>);
    ASSERT_COMMENT_MODEL_SIGNATURE(sigCommentListChanged, void (Model::*)());
}

void StoryboardModelSchemaContractTest::commentModelRowDataSignaturesRemainStable()
{
    using Model = StoryboardCommentModel;

    ASSERT_COMMENT_MODEL_SIGNATURE(rowCount, int (Model::*)(const QModelIndex &) const);
    ASSERT_COMMENT_MODEL_SIGNATURE(data, QVariant (Model::*)(const QModelIndex &, int) const);
    ASSERT_COMMENT_MODEL_SIGNATURE(flags, Qt::ItemFlags (Model::*)(const QModelIndex &) const);
    ASSERT_COMMENT_MODEL_SIGNATURE(setData, bool (Model::*)(const QModelIndex &, const QVariant &, int));
}

void StoryboardModelSchemaContractTest::commentModelRowMutationSignaturesRemainStable()
{
    using Model = StoryboardCommentModel;
    using RowOperation = bool (Model::*)(int, int, const QModelIndex &);

    ASSERT_COMMENT_MODEL_SIGNATURE(insertRows, RowOperation);
    ASSERT_COMMENT_MODEL_SIGNATURE(removeRows, RowOperation);
    ASSERT_COMMENT_MODEL_SIGNATURE(moveRows, bool (Model::*)(const QModelIndex &, int, int, const QModelIndex &, int));
}

void StoryboardModelSchemaContractTest::commentModelMimeSignaturesRemainStable()
{
    using Model = StoryboardCommentModel;
    using SupportedActions = Qt::DropActions (Model::*)() const;

    ASSERT_COMMENT_MODEL_SIGNATURE(mimeTypes, QStringList (Model::*)() const);
    ASSERT_COMMENT_MODEL_SIGNATURE(mimeData, QMimeData * (Model::*)(const QModelIndexList &) const);
    ASSERT_COMMENT_MODEL_SIGNATURE(dropMimeData,
                                   bool (Model::*)(const QMimeData *, Qt::DropAction, int, int, const QModelIndex &));
    ASSERT_COMMENT_MODEL_SIGNATURE(supportedDropActions, SupportedActions);
    ASSERT_COMMENT_MODEL_SIGNATURE(supportedDragActions, SupportedActions);
}

void StoryboardModelSchemaContractTest::commentModelCollectionSignaturesRemainStable()
{
    using Model = StoryboardCommentModel;

    ASSERT_COMMENT_MODEL_SIGNATURE(resetData, void (Model::*)(QVector<StoryboardComment>));
    ASSERT_COMMENT_MODEL_SIGNATURE(getData, QVector<StoryboardComment> (Model::*)());
}

QTEST_GUILESS_MAIN(StoryboardModelSchemaContractTest)

#include "StoryboardModelSchemaContractTest.moc"

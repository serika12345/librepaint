/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/KoShapeReorderCommand.h>

#include <QDebug>
#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
using IndexedShape = KoShapeReorderCommand::IndexedShape;
using MoveShapeType = KoShapeReorderCommand::MoveShapeType;
using ShapeList = QList<KoShape *>;

#define ASSERT_REORDER_SIGNATURE(method, signature)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeReorderCommand::method)), signature>)
} // namespace

class KoShapeReorderCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeReorderIdentityAndLifecycleSignaturesRemainStable();
    void indexedShapeValueSchemaRemainsStable();
    void shapeReorderMoveTypeValuesRemainStable();
    void shapeReorderCreationAndMergeSignaturesRemainStable();
    void shapeReorderNormalizationAndExecutionSignaturesRemainStable();
};

void KoShapeReorderCommandSchemaContractTest::shapeReorderIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoShapeReorderCommand>);
    static_assert(std::is_base_of_v<KUndo2Command, KoShapeReorderCommand>);
    static_assert(std::has_virtual_destructor_v<KoShapeReorderCommand>);
    static_assert(std::is_constructible_v<KoShapeReorderCommand, const ShapeList &, QList<int> &>);
    static_assert(std::is_constructible_v<KoShapeReorderCommand, const ShapeList &, QList<int> &, KUndo2Command *>);
    static_assert(std::is_constructible_v<KoShapeReorderCommand, const QList<IndexedShape> &>);
    static_assert(std::is_constructible_v<KoShapeReorderCommand, const QList<IndexedShape> &, KUndo2Command *>);
}

void KoShapeReorderCommandSchemaContractTest::indexedShapeValueSchemaRemainsStable()
{
    static_assert(std::is_class_v<IndexedShape>);
    static_assert(std::is_same_v<decltype(IndexedShape::zIndex), int>);
    static_assert(std::is_same_v<decltype(IndexedShape::shape), KoShape *>);
    static_assert(std::is_constructible_v<IndexedShape>);
    static_assert(std::is_constructible_v<IndexedShape, KoShape *>);
    static_assert(std::is_same_v<decltype(static_cast<bool (IndexedShape::*)(const IndexedShape &) const>(
                                     &IndexedShape::operator<)),
                                 bool (IndexedShape::*)(const IndexedShape &) const>);
    static_assert(std::is_same_v<decltype(static_cast<QDebug (*)(QDebug, const IndexedShape &)>(&operator<<)),
                                 QDebug (*)(QDebug, const IndexedShape &)>);
}

void KoShapeReorderCommandSchemaContractTest::shapeReorderMoveTypeValuesRemainStable()
{
    static_assert(std::is_enum_v<MoveShapeType>);
    QCOMPARE(int(KoShapeReorderCommand::RaiseShape), 0);
    QCOMPARE(int(KoShapeReorderCommand::LowerShape), 1);
    QCOMPARE(int(KoShapeReorderCommand::BringToFront), 2);
    QCOMPARE(int(KoShapeReorderCommand::SendToBack), 3);
}

void KoShapeReorderCommandSchemaContractTest::shapeReorderCreationAndMergeSignaturesRemainStable()
{
    ASSERT_REORDER_SIGNATURE(createCommand,
                             KoShapeReorderCommand
                                 * (*)(const ShapeList &, KoShapeManager *, MoveShapeType, KUndo2Command *));
    ASSERT_REORDER_SIGNATURE(mergeInShape, KoShapeReorderCommand * (*)(ShapeList, KoShape *, KUndo2Command *));
    ASSERT_REORDER_SIGNATURE(mergeDownShapes, QList<IndexedShape> (*)(ShapeList, ShapeList));

    static_assert(std::is_same_v<decltype(KoShapeReorderCommand::createCommand(std::declval<const ShapeList &>(),
                                                                               std::declval<KoShapeManager *>(),
                                                                               std::declval<MoveShapeType>())),
                                 KoShapeReorderCommand *>);
    static_assert(std::is_same_v<decltype(KoShapeReorderCommand::mergeInShape(std::declval<ShapeList>(),
                                                                              std::declval<KoShape *>())),
                                 KoShapeReorderCommand *>);
}

void KoShapeReorderCommandSchemaContractTest::shapeReorderNormalizationAndExecutionSignaturesRemainStable()
{
    ASSERT_REORDER_SIGNATURE(homogenizeZIndexes, QList<IndexedShape> (*)(QList<IndexedShape>));
    ASSERT_REORDER_SIGNATURE(homogenizeZIndexesLazy, QList<IndexedShape> (*)(QList<IndexedShape>));
    ASSERT_REORDER_SIGNATURE(redo, void (KoShapeReorderCommand::*)());
    ASSERT_REORDER_SIGNATURE(undo, void (KoShapeReorderCommand::*)());
}

QTEST_APPLESS_MAIN(KoShapeReorderCommandSchemaContractTest)

#include "KoShapeReorderCommandSchemaContractTest.moc"

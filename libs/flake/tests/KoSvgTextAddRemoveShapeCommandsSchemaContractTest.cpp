/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/KoSvgTextAddRemoveShapeCommands.h>

#include <QTest>

#include <type_traits>

class KoSvgTextAddRemoveShapeCommandsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgTextContourCommandTypeHierarchyRemainsStable();
    void svgTextContourTypeValuesRemainStable();
    void svgTextContourCommandPhaseSignaturesRemainStable();
    void svgTextContourSpecializedLifecycleSignaturesRemainStable();
    void svgTextContourRemovalOperationSignatureRemainsStable();
};

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::svgTextContourCommandTypeHierarchyRemainsStable()
{
    static_assert(std::is_class_v<KoSvgTextAddRemoveShapeCommandImpl>);
    static_assert(std::is_class_v<KoSvgTextAddShapeCommand>);
    static_assert(std::is_class_v<KoSvgTextRemoveShapeCommand>);
    static_assert(std::is_class_v<KoSvgTextSetTextPathOnRangeCommand>);
    static_assert(std::is_base_of_v<KoSvgTextAddRemoveShapeCommandImpl, KoSvgTextAddShapeCommand>);
    static_assert(std::is_base_of_v<KoSvgTextAddRemoveShapeCommandImpl, KoSvgTextRemoveShapeCommand>);
    static_assert(std::is_base_of_v<KoSvgTextAddRemoveShapeCommandImpl, KoSvgTextSetTextPathOnRangeCommand>);
}

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::svgTextContourTypeValuesRemainStable()
{
    using ContourType = KoSvgTextAddRemoveShapeCommandImpl::ContourType;
    static_assert(std::is_enum_v<ContourType>);
    QCOMPARE(int(ContourType::Unknown), 0);
    QCOMPARE(int(ContourType::Inside), 1);
    QCOMPARE(int(ContourType::Subtract), 2);
    QCOMPARE(int(ContourType::TextPath), 3);
}

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::svgTextContourCommandPhaseSignaturesRemainStable()
{
    using Command = KoSvgTextAddRemoveShapeCommandImpl;
    using ContourType = Command::ContourType;
    using State = KisCommandUtils::FlipFlopCommand::State;

    static_assert(std::is_constructible_v<Command, KoSvgTextShape *, KoShape *, ContourType, State, int, int>);
    static_assert(
        std::is_constructible_v<Command, KoSvgTextShape *, KoShape *, ContourType, State, int, int, KUndo2Command *>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_same_v<decltype(static_cast<void (Command::*)()>(&Command::partA)), void (Command::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<void (Command::*)()>(&Command::partB)), void (Command::*)()>);
}

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::svgTextContourSpecializedLifecycleSignaturesRemainStable()
{
    static_assert(std::is_constructible_v<KoSvgTextAddShapeCommand, KoSvgTextShape *, KoShape *, bool>);
    static_assert(
        std::is_constructible_v<KoSvgTextAddShapeCommand, KoSvgTextShape *, KoShape *, bool, KUndo2Command *>);
    static_assert(std::has_virtual_destructor_v<KoSvgTextAddShapeCommand>);

    static_assert(std::is_constructible_v<KoSvgTextRemoveShapeCommand, KoSvgTextShape *, KoShape *>);
    static_assert(std::is_constructible_v<KoSvgTextRemoveShapeCommand, KoSvgTextShape *, KoShape *, KUndo2Command *>);
    static_assert(std::has_virtual_destructor_v<KoSvgTextRemoveShapeCommand>);

    static_assert(std::is_constructible_v<KoSvgTextSetTextPathOnRangeCommand, KoSvgTextShape *, KoShape *, int, int>);
    static_assert(std::is_constructible_v<KoSvgTextSetTextPathOnRangeCommand,
                                          KoSvgTextShape *,
                                          KoShape *,
                                          int,
                                          int,
                                          KUndo2Command *>);
    static_assert(std::has_virtual_destructor_v<KoSvgTextSetTextPathOnRangeCommand>);
}

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::svgTextContourRemovalOperationSignatureRemainsStable()
{
    using RemoveFunction = void (*)(KoSvgTextShape *, KUndo2Command *, bool, bool);
    static_assert(std::is_same_v<decltype(&KoSvgTextRemoveShapeCommand::removeContourShapesFromFlow), RemoveFunction>);
}

QTEST_APPLESS_MAIN(KoSvgTextAddRemoveShapeCommandsSchemaContractTest)

#include "KoSvgTextAddRemoveShapeCommandsSchemaContractTest.moc"

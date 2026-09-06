/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/KoShapeMergeTextPropertiesCommand.h>
#include <commands/KoSvgConvertTextTypeCommand.h>
#include <commands/KoSvgTextAddRemoveShapeCommands.h>
#include <commands/KoSvgTextPathInfoChangeCommand.h>

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
    void mergeTextPropertiesTypeSchema();
    void mergeTextPropertiesHistorySchema();
    void textPathInfoTypeSchema();
    void textPathInfoHistorySchema();
    void convertTextTypeSchema();
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

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::mergeTextPropertiesTypeSchema()
{
    using Command = KoShapeMergeTextPropertiesCommand;
    using PropertyIds = QSet<KoSvgTextProperties::PropertyId>;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command,
                                          const QList<KoShape *> &,
                                          const KoSvgTextProperties &,
                                          PropertyIds,
                                          KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, const QList<KoShape *> &, const KoSvgTextProperties &>);
}

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::mergeTextPropertiesHistorySchema()
{
    using Command = KoShapeMergeTextPropertiesCommand;

    static_assert(
        std::is_same_v<decltype(static_cast<int (Command::*)() const>(&Command::id)), int (Command::*)() const>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Command::*)(const KUndo2Command *)>(&Command::mergeWith)),
                                 bool (Command::*)(const KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Command::*)()>(&Command::redo)), void (Command::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<void (Command::*)()>(&Command::undo)), void (Command::*)()>);
}

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::textPathInfoTypeSchema()
{
    using Command = KoSvgTextPathInfoChangeCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::is_constructible_v<Command, KoSvgTextShape *, int, KoSvgText::TextOnPathInfo, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KoSvgTextShape *, int, KoSvgText::TextOnPathInfo>);
}

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::textPathInfoHistorySchema()
{
    using Command = KoSvgTextPathInfoChangeCommand;

    static_assert(
        std::is_same_v<decltype(static_cast<int (Command::*)() const>(&Command::id)), int (Command::*)() const>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Command::*)(const KUndo2Command *)>(&Command::mergeWith)),
                                 bool (Command::*)(const KUndo2Command *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (Command::*)()>(&Command::redo)), void (Command::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<void (Command::*)()>(&Command::undo)), void (Command::*)()>);
}

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::convertTextTypeSchema()
{
    using Command = KoSvgConvertTextTypeCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, KoSvgTextShape *, KoSvgTextShape::TextType, int, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KoSvgTextShape *, KoSvgTextShape::TextType, int>);
    static_assert(std::is_same_v<decltype(static_cast<void (Command::*)()>(&Command::redo)), void (Command::*)()>);
    static_assert(std::is_same_v<decltype(static_cast<void (Command::*)()>(&Command::undo)), void (Command::*)()>);
}

QTEST_APPLESS_MAIN(KoSvgTextAddRemoveShapeCommandsSchemaContractTest)

#include "KoSvgTextAddRemoveShapeCommandsSchemaContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisImageResolutionProxy.h>
#include <commands_new/kis_change_projection_color_command.h>
#include <commands_new/kis_image_set_resolution_command.h>

#include <QTest>

#include <type_traits>

#define ASSERT_COMMAND_SIGNATURE(command, method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&command::method)), signature>)

class KisImagePropertyCommandsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void projectionColorCommandTypeConstructionAndLifetimeSchemaRemainStable();
    void projectionColorCommandHistoryAndExecutionSchemaRemainStable();
    void imageResolutionCommandSchemaRemainStable();
    void resetShapesCommandSchemaRemainStable();
    void resolutionProxyTypeAndConstructionSchemaRemainStable();
    void resolutionProxyQueryAndCloneSchemaRemainStable();
    void resolutionProxyCreationSchemaRemainStable();
};

void KisImagePropertyCommandsSchemaContractTest::projectionColorCommandTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Command = KisChangeProjectionColorCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::is_base_of_v<KisAsynchronouslyMergeableCommandInterface, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, KisImageSP, const KoColor &, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KisImageSP, const KoColor &>);

    QVERIFY(true);
}

void KisImagePropertyCommandsSchemaContractTest::projectionColorCommandHistoryAndExecutionSchemaRemainStable()
{
    using Command = KisChangeProjectionColorCommand;

    ASSERT_COMMAND_SIGNATURE(Command, canMergeWith, bool (Command::*)(const KUndo2Command *) const);
    ASSERT_COMMAND_SIGNATURE(Command, id, int (Command::*)() const);
    ASSERT_COMMAND_SIGNATURE(Command, mergeWith, bool (Command::*)(const KUndo2Command *));
    ASSERT_COMMAND_SIGNATURE(Command, redo, void (Command::*)());
    ASSERT_COMMAND_SIGNATURE(Command, undo, void (Command::*)());

    QVERIFY(true);
}

void KisImagePropertyCommandsSchemaContractTest::imageResolutionCommandSchemaRemainStable()
{
    using Command = KisImageSetResolutionCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, qreal, qreal, KUndo2Command *>);
    static_assert(std::is_constructible_v<Command, KisImageWSP, qreal, qreal>);
    ASSERT_COMMAND_SIGNATURE(Command, redo, void (Command::*)());
    ASSERT_COMMAND_SIGNATURE(Command, undo, void (Command::*)());

    QVERIFY(true);
}

void KisImagePropertyCommandsSchemaContractTest::resetShapesCommandSchemaRemainStable()
{
    using Command = KisResetShapesCommand;

    static_assert(std::is_class_v<Command>);
    static_assert(std::is_base_of_v<KUndo2Command, Command>);
    static_assert(std::has_virtual_destructor_v<Command>);
    static_assert(std::is_constructible_v<Command, KisNodeSP>);
    ASSERT_COMMAND_SIGNATURE(Command, redo, void (Command::*)());
    ASSERT_COMMAND_SIGNATURE(Command, undo, void (Command::*)());

    QVERIFY(true);
}

void KisImagePropertyCommandsSchemaContractTest::resolutionProxyTypeAndConstructionSchemaRemainStable()
{
    using Proxy = KisImageResolutionProxy;

    static_assert(std::is_same_v<KisImageResolutionProxySP, QSharedPointer<Proxy>>);
    static_assert(std::is_class_v<Proxy>);
    static_assert(std::is_base_of_v<QObject, Proxy>);
    static_assert(std::has_virtual_destructor_v<Proxy>);
    static_assert(std::is_default_constructible_v<Proxy>);
    static_assert(std::is_constructible_v<Proxy, KisImageWSP>);
    static_assert(std::is_constructible_v<Proxy, const Proxy &>);

    QVERIFY(true);
}

void KisImagePropertyCommandsSchemaContractTest::resolutionProxyQueryAndCloneSchemaRemainStable()
{
    using Proxy = KisImageResolutionProxy;

    ASSERT_COMMAND_SIGNATURE(Proxy, xRes, qreal (Proxy::*)() const);
    ASSERT_COMMAND_SIGNATURE(Proxy, yRes, qreal (Proxy::*)() const);
    ASSERT_COMMAND_SIGNATURE(Proxy, compareResolution, bool (Proxy::*)(const Proxy &) const);
    ASSERT_COMMAND_SIGNATURE(Proxy, cloneDetached, KisImageResolutionProxySP (Proxy::*)() const);

    QVERIFY(true);
}

void KisImagePropertyCommandsSchemaContractTest::resolutionProxyCreationSchemaRemainStable()
{
    using Proxy = KisImageResolutionProxy;

    ASSERT_COMMAND_SIGNATURE(Proxy, createOrCloneDetached, KisImageResolutionProxySP (Proxy::*)(KisImageWSP) const);
    ASSERT_COMMAND_SIGNATURE(Proxy, identity, KisImageResolutionProxySP (*)());

    QVERIFY(true);
}

#undef ASSERT_COMMAND_SIGNATURE

QTEST_APPLESS_MAIN(KisImagePropertyCommandsSchemaContractTest)

#include "KisImageProjectionAndResolutionCommandsSchemaContractTest.moc"

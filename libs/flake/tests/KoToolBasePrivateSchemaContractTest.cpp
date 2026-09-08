/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoToolBase.h>
#include <KoToolBase_p.h>

#include <QTest>

#include <type_traits>

class KoToolBasePrivateSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void privateTypeConstructionAndLifetimeSchemaRemainStable();
    void toolCanvasResourcesSchemaRemainsStable();
    void presentationAndOwnershipStateSchemaRemainsStable();
    void modeAndActivationStateSchemaRemainsStable();
    void canvasResourceAndSignalConnectionSchemaRemainStable();
};

void KoToolBasePrivateSchemaContractTest::privateTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Private = KoToolBasePrivate;

    static_assert(std::is_class_v<Private>);
    static_assert(std::is_constructible_v<Private, KoToolBase *, KoCanvasBase *>);
    static_assert(std::has_virtual_destructor_v<Private>);
}

void KoToolBasePrivateSchemaContractTest::toolCanvasResourcesSchemaRemainsStable()
{
    using Resources = KoToolBasePrivate::ToolCanvasResources;

    static_assert(std::is_class_v<Resources>);
    static_assert(std::is_same_v<decltype(&Resources::abstractResources),
                                 QHash<int, KoAbstractCanvasResourceInterfaceSP> Resources::*>);
    static_assert(
        std::is_same_v<decltype(&Resources::converters), QHash<int, KoDerivedResourceConverterSP> Resources::*>);
}

void KoToolBasePrivateSchemaContractTest::presentationAndOwnershipStateSchemaRemainsStable()
{
    using Private = KoToolBasePrivate;

    static_assert(std::is_same_v<decltype(&Private::optionWidgets), QList<QPointer<QWidget>> Private::*>);
    static_assert(std::is_same_v<decltype(&Private::currentCursor), QCursor Private::*>);
    static_assert(std::is_same_v<decltype(&Private::q), KoToolBase * Private::*>);
    static_assert(std::is_same_v<decltype(&Private::factory), KoToolFactoryBase * Private::*>);
}

void KoToolBasePrivateSchemaContractTest::modeAndActivationStateSchemaRemainsStable()
{
    using Private = KoToolBasePrivate;

    static_assert(std::is_same_v<decltype(&Private::optionWidgetsCreated), bool Private::*>);
    static_assert(std::is_same_v<decltype(&Private::isInTextMode), bool Private::*>);
    static_assert(std::is_same_v<decltype(&Private::maskSyntheticEvents), bool Private::*>);
    static_assert(std::is_same_v<decltype(&Private::isActivated), bool Private::*>);
    static_assert(std::is_same_v<decltype(&Private::isOpacityPresetMode), bool Private::*>);
}

void KoToolBasePrivateSchemaContractTest::canvasResourceAndSignalConnectionSchemaRemainStable()
{
    using Private = KoToolBasePrivate;
    using ConnectSignalsSignature = void (Private::*)();

    static_assert(std::is_same_v<decltype(&Private::canvas), KoCanvasBase * Private::*>);
    static_assert(std::is_same_v<decltype(&Private::lastDecorationsRect), QRectF Private::*>);
    static_assert(std::is_same_v<decltype(&Private::toolCanvasResources), Private::ToolCanvasResources Private::*>);
    static_assert(std::is_same_v<decltype(static_cast<ConnectSignalsSignature>(&Private::connectSignals)),
                                 ConnectSignalsSignature>);
}

QTEST_APPLESS_MAIN(KoToolBasePrivateSchemaContractTest)

#include "KoToolBasePrivateSchemaContractTest.moc"

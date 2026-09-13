/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "actions/KisNoParameterActionFactory.h"
#include "actions/KisPasteActionFactories.h"
#include "actions/kis_selection_action_factories.h"
#include "operations/kis_filter_selection_operation.h"
#include "operations/kis_operation.h"
#include "operations/kis_operation_configuration.h"
#include "operations/kis_operation_registry.h"
#include "operations/kis_operation_ui_factory.h"
#include "selection/KisSelectionActionsAdapter.h"

#define ASSERT_NO_PARAMETER_ACTION_SCHEMA(Type)                                                                        \
    static_assert(std::is_class_v<Type>);                                                                              \
    static_assert(std::is_base_of_v<KisNoParameterActionFactory, Type>);                                               \
    static_assert(std::is_default_constructible_v<Type>);                                                              \
    static_assert(std::is_same_v<decltype(&Type::run), void (Type::*)(KisViewManager *)>)

#define ASSERT_STROKE_SELECTION_ACTION_SCHEMA(Type)                                                                    \
    static_assert(std::is_class_v<Type>);                                                                              \
    static_assert(std::is_base_of_v<KisOperation, Type>);                                                              \
    static_assert(std::is_default_constructible_v<Type>);                                                              \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(&Type::run), void (Type::*)(KisViewManager *, const StrokeSelectionOptions &)>)

class KisSelectionActionFactoriesSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void noParameterActionFactoryBaseSchemaRemainsStable();
    void selectionActionsAdapterTypeAndSelectionEntrySchemaRemainStable();
    void filterSelectionOperationTypeAndFilterEntrySchemaRemainStable();
    void operationRegistryTypeLifetimeAndInstanceSchemaRemainStable();
    void operationBaseTypeConstructionAndDispatchSchemaRemainStable();
    void operationConfigurationTypeConstructionAndIdentitySchemaRemainStable();
    void operationUiFactoryTypeConstructionAndConfigurationSchemaRemainStable();
    void basicSelectionStateActionSchemaRemainsStable();
    void fillAndInvertActionSchemaRemainsStable();
    void cutAndCopyActionSchemaRemainsStable();
    void selectionRepresentationActionSchemaRemainsStable();
    void strokeSelectionActionSchemaRemainsStable();
    void pasteActionTypeAndFlagSchemaRemainStable();
    void pasteActionDispatchSignaturesRemainStable();
    void pasteIntoActionSchemaRemainsStable();
    void pasteNewAndReferenceActionSchemaRemainStable();
    void pasteShapeStyleActionSchemaRemainsStable();
};

void KisSelectionActionFactoriesSchemaContractTest::noParameterActionFactoryBaseSchemaRemainsStable()
{
    using Factory = KisNoParameterActionFactory;
    using Run = void (Factory::*)(KisViewManager *);
    using RunFromXml = void (Factory::*)(KisViewManager *, const KisOperationConfiguration &);
    struct FactoryProbe final : Factory {
        using Factory::Factory;

        void run(KisViewManager *) override
        {
        }
    };

    static_assert(std::is_base_of_v<KisOperation, Factory>);
    static_assert(std::is_abstract_v<Factory>);
    static_assert(std::is_constructible_v<FactoryProbe, const QString &>);
    static_assert(std::is_same_v<decltype(&Factory::run), Run>);
    static_assert(std::is_same_v<decltype(&Factory::runFromXML), RunFromXml>);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::selectionActionsAdapterTypeAndSelectionEntrySchemaRemainStable()
{
    using Adapter = KisSelectionActionsAdapter;
    using SelectOpaqueOnNode = void (Adapter::*)(KisNodeSP, SelectionAction);

    static_assert(std::is_constructible_v<Adapter, KisSelectionManager *>);
    static_assert(std::is_same_v<decltype(&Adapter::selectOpaqueOnNode), SelectOpaqueOnNode>);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::filterSelectionOperationTypeAndFilterEntrySchemaRemainStable()
{
    using Operation = KisFilterSelectionOperation;
    using RunFilter = void (Operation::*)(KisSelectionFilter *, KisViewManager *, const KisOperationConfiguration &);

    static_assert(std::is_base_of_v<KisOperation, Operation>);
    static_assert(std::is_constructible_v<Operation, const QString &>);
    static_assert(std::is_same_v<decltype(&Operation::runFilter), RunFilter>);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::operationRegistryTypeLifetimeAndInstanceSchemaRemainStable()
{
    using Registry = KisOperationRegistry;

    static_assert(std::is_base_of_v<KoGenericRegistry<KisOperation *>, Registry>);
    static_assert(std::is_constructible_v<Registry>);
    static_assert(std::has_virtual_destructor_v<Registry>);
    static_assert(std::is_same_v<decltype(&Registry::instance), Registry *(*)()>);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::operationBaseTypeConstructionAndDispatchSchemaRemainStable()
{
    using Operation = KisOperation;
    using Id = QString (Operation::*)() const;
    using RunFromXml = void (Operation::*)(KisViewManager *, const KisOperationConfiguration &);

    static_assert(std::is_constructible_v<Operation, const QString &>);
    static_assert(std::has_virtual_destructor_v<Operation>);
    static_assert(std::is_same_v<decltype(&Operation::id), Id>);
    static_assert(std::is_same_v<decltype(&Operation::runFromXML), RunFromXml>);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::
    operationConfigurationTypeConstructionAndIdentitySchemaRemainStable()
{
    using Configuration = KisOperationConfiguration;
    using Id = QString (Configuration::*)() const;

    static_assert(std::is_base_of_v<KisPropertiesConfiguration, Configuration>);
    static_assert(std::is_default_constructible_v<Configuration>);
    static_assert(std::is_constructible_v<Configuration, const QString &>);
    static_assert(std::has_virtual_destructor_v<Configuration>);
    static_assert(std::is_same_v<decltype(&Configuration::id), Id>);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::
    operationUiFactoryTypeConstructionAndConfigurationSchemaRemainStable()
{
    using Factory = KisOperationUIFactory;
    using FetchConfiguration = bool (Factory::*)(KisViewManager *, KisOperationConfigurationSP);
    using Id = QString (Factory::*)() const;
    struct FactoryProbe final : Factory {
        using Factory::Factory;

        bool fetchConfiguration(KisViewManager *, KisOperationConfigurationSP) override
        {
            return false;
        }
    };

    static_assert(std::is_abstract_v<Factory>);
    static_assert(std::is_constructible_v<FactoryProbe, const QString &>);
    static_assert(std::has_virtual_destructor_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::id), Id>);
    static_assert(std::is_same_v<decltype(&Factory::fetchConfiguration), FetchConfiguration>);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::basicSelectionStateActionSchemaRemainsStable()
{
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisSelectAllActionFactory);
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisDeselectActionFactory);
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisReselectActionFactory);
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisClearActionFactory);
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisImageResizeToSelectionActionFactory);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::fillAndInvertActionSchemaRemainsStable()
{
    using FillFromXml = void (KisFillActionFactory::*)(KisViewManager *, const KisOperationConfiguration &);
    using Fill = void (KisFillActionFactory::*)(const QString &, KisViewManager *);
    using InvertFromXml = void (KisInvertSelectionOperation::*)(KisViewManager *, const KisOperationConfiguration &);

    static_assert(std::is_class_v<KisFillActionFactory>);
    static_assert(std::is_base_of_v<KisOperation, KisFillActionFactory>);
    static_assert(std::is_default_constructible_v<KisFillActionFactory>);
    static_assert(std::is_same_v<decltype(&KisFillActionFactory::runFromXML), FillFromXml>);
    static_assert(std::is_same_v<decltype(&KisFillActionFactory::run), Fill>);
    static_assert(std::is_class_v<KisInvertSelectionOperation>);
    static_assert(std::is_base_of_v<KisFilterSelectionOperation, KisInvertSelectionOperation>);
    static_assert(std::is_default_constructible_v<KisInvertSelectionOperation>);
    static_assert(std::is_same_v<decltype(&KisInvertSelectionOperation::runFromXML), InvertFromXml>);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::cutAndCopyActionSchemaRemainsStable()
{
    using CutCopyFromXml = void (KisCutCopyActionFactory::*)(KisViewManager *, const KisOperationConfiguration &);
    using CutCopy = void (KisCutCopyActionFactory::*)(KisCutCopyActionFactory::Flags, KisViewManager *);

    static_assert(std::is_class_v<KisCutCopyActionFactory>);
    static_assert(std::is_base_of_v<KisOperation, KisCutCopyActionFactory>);
    static_assert(std::is_default_constructible_v<KisCutCopyActionFactory>);
    static_assert(std::is_enum_v<KisCutCopyActionFactory::Flag>);
    static_assert(std::is_same_v<KisCutCopyActionFactory::Flags, QFlags<KisCutCopyActionFactory::Flag>>);
    static_assert(KisCutCopyActionFactory::None == 0x0);
    static_assert(KisCutCopyActionFactory::CutClip == 0x1);
    static_assert(KisCutCopyActionFactory::SharpClip == 0x2);
    static_assert(std::is_same_v<decltype(&KisCutCopyActionFactory::runFromXML), CutCopyFromXml>);
    static_assert(std::is_same_v<decltype(&KisCutCopyActionFactory::run), CutCopy>);
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisCopyMergedActionFactory);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::selectionRepresentationActionSchemaRemainsStable()
{
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisSelectionToVectorActionFactory);
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisSelectionToRasterActionFactory);
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisShapesToVectorSelectionActionFactory);
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisSelectionToShapeActionFactory);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::strokeSelectionActionSchemaRemainsStable()
{
    ASSERT_STROKE_SELECTION_ACTION_SCHEMA(KisStrokeSelectionActionFactory);
    ASSERT_STROKE_SELECTION_ACTION_SCHEMA(KisStrokeBrushSelectionActionFactory);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::pasteActionTypeAndFlagSchemaRemainStable()
{
    using Paste = KisPasteActionFactory;
    static_assert(std::is_class_v<Paste> && std::is_base_of_v<KisOperation, Paste>);
    static_assert(std::is_enum_v<Paste::Flag> && std::is_same_v<Paste::Flags, QFlags<Paste::Flag>>);
    static_assert(Paste::None == 0x0);
    static_assert(Paste::PasteAtCursor == 0x1);
    static_assert(Paste::ForceNewLayer == 0x2);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::pasteActionDispatchSignaturesRemainStable()
{
    using Paste = KisPasteActionFactory;
    using FromXml = void (Paste::*)(KisViewManager *, const KisOperationConfiguration &);
    using Run = void (Paste::*)(Paste::Flags, KisViewManager *);
    static_assert(std::is_default_constructible_v<Paste>);
    static_assert(std::is_same_v<decltype(&Paste::runFromXML), FromXml>);
    static_assert(std::is_same_v<decltype(&Paste::run), Run>);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::pasteIntoActionSchemaRemainsStable()
{
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisPasteIntoActionFactory);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::pasteNewAndReferenceActionSchemaRemainStable()
{
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisPasteNewActionFactory);
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisPasteReferenceActionFactory);

    QVERIFY(true);
}

void KisSelectionActionFactoriesSchemaContractTest::pasteShapeStyleActionSchemaRemainsStable()
{
    ASSERT_NO_PARAMETER_ACTION_SCHEMA(KisPasteShapeStyleActionFactory);

    QVERIFY(true);
}

#undef ASSERT_NO_PARAMETER_ACTION_SCHEMA
#undef ASSERT_STROKE_SELECTION_ACTION_SCHEMA

QTEST_MAIN(KisSelectionActionFactoriesSchemaContractTest)

#include "KisSelectionActionFactoriesSchemaContractTest.moc"

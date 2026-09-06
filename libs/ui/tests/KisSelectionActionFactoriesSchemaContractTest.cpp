/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "actions/kis_selection_action_factories.h"

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
    void basicSelectionStateActionSchemaRemainsStable();
    void fillAndInvertActionSchemaRemainsStable();
    void cutAndCopyActionSchemaRemainsStable();
    void selectionRepresentationActionSchemaRemainsStable();
    void strokeSelectionActionSchemaRemainsStable();
};

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

#undef ASSERT_NO_PARAMETER_ACTION_SCHEMA
#undef ASSERT_STROKE_SELECTION_ACTION_SCHEMA

QTEST_MAIN(KisSelectionActionFactoriesSchemaContractTest)

#include "KisSelectionActionFactoriesSchemaContractTest.moc"

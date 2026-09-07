/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <ui/orchestration/kis_action.h>

#include <QTest>

#include <array>
#include <type_traits>
#include <utility>

#define ASSERT_ACTION_SIGNATURE(method, signature)                                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAction::method)), signature>)

class KisActionEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void activationFlagsRemainIndependentBits();
    void activationConditionsRemainIndependentBits();
    void actionTypeConstructionAndLifetimeSchemaRemainStable();
    void actionDefaultShortcutSignaturesRemainStable();
    void actionActivationFlagSignaturesRemainStable();
    void actionActivationConditionSignaturesRemainStable();
    void actionNodeOperationAndNotificationSignaturesRemainStable();
};

void KisActionEnumContractTest::activationFlagsRemainIndependentBits()
{
    const std::array<std::pair<KisAction::ActivationFlag, int>, 21> flags{{
        {KisAction::NONE, 0x00000},
        {KisAction::ACTIVE_IMAGE, 0x00001},
        {KisAction::MULTIPLE_IMAGES, 0x00002},
        {KisAction::CURRENT_IMAGE_MODIFIED, 0x00004},
        {KisAction::ACTIVE_NODE, 0x00008},
        {KisAction::ACTIVE_DEVICE, 0x00010},
        {KisAction::ACTIVE_LAYER, 0x00020},
        {KisAction::ACTIVE_TRANSPARENCY_MASK, 0x00040},
        {KisAction::ACTIVE_SHAPE_LAYER, 0x00080},
        {KisAction::PIXELS_SELECTED, 0x00100},
        {KisAction::SHAPES_SELECTED, 0x00200},
        {KisAction::ANY_SELECTION_WITH_PIXELS, 0x00400},
        {KisAction::PIXELS_IN_CLIPBOARD, 0x00800},
        {KisAction::SHAPES_IN_CLIPBOARD, 0x01000},
        {KisAction::NEVER_ACTIVATE, 0x02000},
        {KisAction::LAYERS_IN_CLIPBOARD, 0x04000},
        {KisAction::IMAGE_HAS_ANIMATION, 0x08000},
        {KisAction::SHAPE_SELECTION_WITH_SHAPES, 0x10000},
        {KisAction::PIXEL_SELECTION_WITH_PIXELS, 0x20000},
        {KisAction::IMAGE_CAN_RESELECT, 0x40000},
        {KisAction::IMAGE_IS_WRITABLE, 0x80000},
    }};

    for (const auto &flag : flags) {
        QCOMPARE(int(flag.first), flag.second);
    }

    const KisAction::ActivationFlags selection = KisAction::PIXELS_SELECTED | KisAction::SHAPES_SELECTED;
    QVERIFY(selection.testFlag(KisAction::PIXELS_SELECTED));
    QVERIFY(selection.testFlag(KisAction::SHAPES_SELECTED));
    QVERIFY(!selection.testFlag(KisAction::ACTIVE_IMAGE));
}

void KisActionEnumContractTest::activationConditionsRemainIndependentBits()
{
    const std::array<std::pair<KisAction::ActivationCondition, int>, 5> conditions{{
        {KisAction::NO_CONDITION, 0x0},
        {KisAction::ACTIVE_NODE_EDITABLE, 0x1},
        {KisAction::ACTIVE_NODE_EDITABLE_PAINT_DEVICE, 0x2},
        {KisAction::SELECTION_EDITABLE, 0x4},
        {KisAction::OPENGL_ENABLED, 0x8},
    }};

    for (const auto &condition : conditions) {
        QCOMPARE(int(condition.first), condition.second);
    }

    const KisAction::ActivationConditions editable = KisAction::ACTIVE_NODE_EDITABLE | KisAction::SELECTION_EDITABLE;
    QVERIFY(editable.testFlag(KisAction::ACTIVE_NODE_EDITABLE));
    QVERIFY(editable.testFlag(KisAction::SELECTION_EDITABLE));
    QVERIFY(!editable.testFlag(KisAction::OPENGL_ENABLED));
}

void KisActionEnumContractTest::actionTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAction>);
    static_assert(std::is_base_of_v<QWidgetAction, KisAction>);
    static_assert(std::is_constructible_v<KisAction, QObject *>);
    static_assert(std::is_constructible_v<KisAction, const QString &, QObject *>);
    static_assert(std::is_constructible_v<KisAction, const QIcon &, const QString &, QObject *>);
    static_assert(std::has_virtual_destructor_v<KisAction>);
}

void KisActionEnumContractTest::actionDefaultShortcutSignaturesRemainStable()
{
    ASSERT_ACTION_SIGNATURE(defaultShortcut, QKeySequence (KisAction::*)() const);
    ASSERT_ACTION_SIGNATURE(setDefaultShortcut, void (KisAction::*)(const QKeySequence &));
}

void KisActionEnumContractTest::actionActivationFlagSignaturesRemainStable()
{
    ASSERT_ACTION_SIGNATURE(activationFlags, KisAction::ActivationFlags (KisAction::*)());
    ASSERT_ACTION_SIGNATURE(setActivationFlags, void (KisAction::*)(KisAction::ActivationFlags));
}

void KisActionEnumContractTest::actionActivationConditionSignaturesRemainStable()
{
    ASSERT_ACTION_SIGNATURE(activationConditions, KisAction::ActivationConditions (KisAction::*)());
    ASSERT_ACTION_SIGNATURE(setActivationConditions, void (KisAction::*)(KisAction::ActivationConditions));
}

void KisActionEnumContractTest::actionNodeOperationAndNotificationSignaturesRemainStable()
{
    ASSERT_ACTION_SIGNATURE(excludedNodeTypes, const QStringList &(KisAction::*)() const);
    ASSERT_ACTION_SIGNATURE(setExcludedNodeTypes, void (KisAction::*)(const QStringList &));
    ASSERT_ACTION_SIGNATURE(setActionEnabled, void (KisAction::*)(bool));
    ASSERT_ACTION_SIGNATURE(setOperationID, void (KisAction::*)(const QString &));
    ASSERT_ACTION_SIGNATURE(sigEnableSlaves, void (KisAction::*)(bool));
}

QTEST_GUILESS_MAIN(KisActionEnumContractTest)

#include "KisActionEnumContractTest.moc"

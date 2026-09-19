/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_alternate_invocation_action.h"

#include "kis_abstract_input_action.h"

#include "kis_tool_invocation_action.h"

#include <QTest>


class KisToolInvocationActionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void toolInvocationPrimaryShortcutOrdinalsRemainStable();
    void toolInvocationTemporaryToolShortcutOrdinalsRemainStable();
    void alternateInvocationModeShortcutOrdinalsRemainStable();
    void alternateInvocationSamplingShortcutOrdinalsRemainStable();
};

void KisToolInvocationActionSchemaContractTest::toolInvocationPrimaryShortcutOrdinalsRemainStable()
{
    using Shortcut = KisToolInvocationAction::Shortcut;

    QCOMPARE(static_cast<int>(Shortcut::ActivateShortcut), 0);
    QCOMPARE(static_cast<int>(Shortcut::ConfirmShortcut), 1);
    QCOMPARE(static_cast<int>(Shortcut::CancelShortcut), 2);
    QCOMPARE(static_cast<int>(Shortcut::ActivateWithOtherColorShortcut), 14);
}

void KisToolInvocationActionSchemaContractTest::toolInvocationTemporaryToolShortcutOrdinalsRemainStable()
{
    using Shortcut = KisToolInvocationAction::Shortcut;

    QCOMPARE(static_cast<int>(Shortcut::LineToolShortcut), 3);
    QCOMPARE(static_cast<int>(Shortcut::EllipseToolShortcut), 4);
    QCOMPARE(static_cast<int>(Shortcut::RectToolShortcut), 5);
    QCOMPARE(static_cast<int>(Shortcut::MoveToolShortcut), 6);
    QCOMPARE(static_cast<int>(Shortcut::FillToolShortcut), 7);
    QCOMPARE(static_cast<int>(Shortcut::GradientToolShortcut), 8);
    QCOMPARE(static_cast<int>(Shortcut::MeasureToolShortcut), 9);
    QCOMPARE(static_cast<int>(Shortcut::EllipseSelToolShortcut), 10);
    QCOMPARE(static_cast<int>(Shortcut::RectSelToolShortcut), 11);
    QCOMPARE(static_cast<int>(Shortcut::ContigSelToolShortcut), 12);
    QCOMPARE(static_cast<int>(Shortcut::FreehandSelToolShortcut), 13);
}

void KisToolInvocationActionSchemaContractTest::alternateInvocationModeShortcutOrdinalsRemainStable()
{
    using Shortcut = KisAlternateInvocationAction::Shortcut;

    QCOMPARE(static_cast<int>(Shortcut::PrimaryAlternateModeShortcut), 0);
    QCOMPARE(static_cast<int>(Shortcut::SecondaryAlternateModeShortcut), 1);
    QCOMPARE(static_cast<int>(Shortcut::TertiaryAlternateModeShortcut), 6);
}

void KisToolInvocationActionSchemaContractTest::alternateInvocationSamplingShortcutOrdinalsRemainStable()
{
    using Shortcut = KisAlternateInvocationAction::Shortcut;

    QCOMPARE(static_cast<int>(Shortcut::SampleColorFgLayerModeShortcut), 2);
    QCOMPARE(static_cast<int>(Shortcut::SampleColorBgLayerModeShortcut), 3);
    QCOMPARE(static_cast<int>(Shortcut::SampleColorFgImageModeShortcut), 4);
    QCOMPARE(static_cast<int>(Shortcut::SampleColorBgImageModeShortcut), 5);
}

QTEST_APPLESS_MAIN(KisToolInvocationActionSchemaContractTest)

#include "KisToolInvocationActionSchemaContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_tool.h>

#include <QTest>

#include <array>
#include <utility>

void kis_assert_recoverable(const char *, const char *, int)
{
}

class KisToolActionContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void customSettingFlagsRemainIndependentBits();
    void toolActionValuesRemainStable();
    void alternateActionsRemainToolActionAliases();
    void nodePaintAbilityValuesRemainStable();
    void activationIdentifierRemainsStable();
};

void KisToolActionContractTest::customSettingFlagsRemainIndependentBits()
{
    QCOMPARE(int(KisTool::FLAG_USES_CUSTOM_PRESET), 0x01);
    QCOMPARE(int(KisTool::FLAG_USES_CUSTOM_COMPOSITEOP), 0x02);
    QCOMPARE(int(KisTool::FLAG_USES_CUSTOM_SIZE), 0x04);
}

void KisToolActionContractTest::toolActionValuesRemainStable()
{
    const std::array<KisTool::ToolAction, 11> actions {{
        KisTool::Primary,
        KisTool::AlternateChangeSize,
        KisTool::AlternateChangeSizeSnap,
        KisTool::AlternateSampleFgNode,
        KisTool::AlternateSampleBgNode,
        KisTool::AlternateSampleFgImage,
        KisTool::AlternateSampleBgImage,
        KisTool::AlternateSecondary,
        KisTool::AlternateThird,
        KisTool::AlternateFourth,
        KisTool::AlternateFifth,
    }};

    for (std::size_t index = 0; index < actions.size(); ++index) {
        QCOMPARE(int(actions[index]), int(index));
    }
    QCOMPARE(int(KisTool::Alternate_NONE), 10000);
}

void KisToolActionContractTest::alternateActionsRemainToolActionAliases()
{
    const std::array<std::pair<KisTool::ToolAction, KisTool::AlternateAction>, 11> actions {{
        {KisTool::AlternateChangeSize, KisTool::ChangeSize},
        {KisTool::AlternateChangeSizeSnap, KisTool::ChangeSizeSnap},
        {KisTool::AlternateSampleFgNode, KisTool::SampleFgNode},
        {KisTool::AlternateSampleBgNode, KisTool::SampleBgNode},
        {KisTool::AlternateSampleFgImage, KisTool::SampleFgImage},
        {KisTool::AlternateSampleBgImage, KisTool::SampleBgImage},
        {KisTool::AlternateSecondary, KisTool::Secondary},
        {KisTool::AlternateThird, KisTool::Third},
        {KisTool::AlternateFourth, KisTool::Fourth},
        {KisTool::AlternateFifth, KisTool::Fifth},
        {KisTool::Alternate_NONE, KisTool::NONE},
    }};

    for (const auto &action : actions) {
        QCOMPARE(int(action.second), int(action.first));
        QCOMPARE(KisTool::actionToAlternateAction(action.first), action.second);
    }
}

void KisToolActionContractTest::nodePaintAbilityValuesRemainStable()
{
    const std::array<KisTool::NodePaintAbility, 5> abilities {{
        KisTool::VECTOR,
        KisTool::CLONE,
        KisTool::PAINT,
        KisTool::UNPAINTABLE,
        KisTool::MYPAINTBRUSH_UNPAINTABLE,
    }};

    for (std::size_t index = 0; index < abilities.size(); ++index) {
        QCOMPARE(int(abilities[index]), int(index));
    }
}

void KisToolActionContractTest::activationIdentifierRemainsStable()
{
    QCOMPARE(KRITA_TOOL_ACTIVATION_ID, QStringLiteral("flake/always"));
}

QTEST_GUILESS_MAIN(KisToolActionContractTest)

#include "KisToolActionContractTest.moc"

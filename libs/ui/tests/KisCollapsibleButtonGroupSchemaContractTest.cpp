/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "events/KisLongPressEventFilter.h"
#include "utils/KisRecentFileIconCache.h"
#include "widgets/KisColorSamplerPreviewPreview.h"
#include "widgets/KisDockerHud.h"
#include "widgets/KisLodAvailabilityData.h"
#include "widgets/KisMemoryReportButton.h"
#include "widgets/kis_collapsible_button_group.h"
#include "widgets/kis_tone_curve_widget.h"
#include "widgets/kis_tool_button.h"
#include "widgets/kis_utility_title_bar.h"

#include <QTest>


class KisCollapsibleButtonGroupSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void lodAvailabilityDefaultsMembersAndEqualityRemainStable();
    void longPressEventFilterTypePropertyAndEntrySchemaRemainStable();
};

void KisCollapsibleButtonGroupSchemaContractTest::lodAvailabilityDefaultsMembersAndEqualityRemainStable()
{
    using Data = KisLodAvailabilityData;


    Data first;
    Data second;
    QVERIFY(first.isLodUserAllowed);
    QVERIFY(first.isLodSizeThresholdSupported);
    QCOMPARE(first.lodSizeThreshold, 100.0);
    QVERIFY(first == second);

    second.isLodUserAllowed = false;
    QVERIFY(first != second);
    second = first;
    second.isLodSizeThresholdSupported = false;
    QVERIFY(first != second);
    second = first;
    second.lodSizeThreshold = 50.0;
    QVERIFY(first != second);
}

void KisCollapsibleButtonGroupSchemaContractTest::longPressEventFilterTypePropertyAndEntrySchemaRemainStable()
{
    using Filter = KisLongPressEventFilter;

    QCOMPARE(Filter::ENABLED_PROPERTY, "KRITA_LONG_PRESS");
}

QTEST_GUILESS_MAIN(KisCollapsibleButtonGroupSchemaContractTest)

#include "KisCollapsibleButtonGroupSchemaContractTest.moc"

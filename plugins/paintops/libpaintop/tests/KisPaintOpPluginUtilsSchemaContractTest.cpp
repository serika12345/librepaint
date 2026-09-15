/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_paintop_plugin_utils.h>

#include <QTest>

#include <type_traits>

class KisPaintOpPluginUtilsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pluginSpacingAndTimingUtilitySchemaRemainStable();
};

void KisPaintOpPluginUtilsSchemaContractTest::pluginSpacingAndTimingUtilitySchemaRemainStable()
{
    using SpacingSignature = KisSpacingInformation (*)(qreal,
                                                       qreal,
                                                       bool,
                                                       qreal,
                                                       bool,
                                                       qreal,
                                                       bool,
                                                       qreal,
                                                       qreal,
                                                       const KisAirbrushOptionData *,
                                                       const KisSpacingOption *,
                                                       const KisPaintInformation &);
    using TimingSignature =
        KisTimingInformation (*)(const KisAirbrushOptionData *, const KisRateOption *, const KisPaintInformation &);

    static_assert(std::is_same_v<decltype(&KisPaintOpPluginUtils::effectiveSpacing), SpacingSignature>);
    static_assert(std::is_same_v<decltype(&KisPaintOpPluginUtils::effectiveTiming), TimingSignature>);
}

QTEST_GUILESS_MAIN(KisPaintOpPluginUtilsSchemaContractTest)

#include "KisPaintOpPluginUtilsSchemaContractTest.moc"

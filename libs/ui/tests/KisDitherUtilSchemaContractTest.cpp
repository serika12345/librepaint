/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "utils/KisDitherUtil.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{

#define ASSERT_SIGNATURE(method, ...)                                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisDitherUtil::method)), __VA_ARGS__>)

} // namespace

class KisDitherUtilSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndConstructionSchemaRemainStable();
    void thresholdModeSchemaRemainsStable();
    void patternValueModeSchemaRemainsStable();
    void configurationAndThresholdSignaturesRemainStable();
};

void KisDitherUtilSchemaContractTest::typeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<KisDitherUtil>);
    static_assert(std::is_default_constructible_v<KisDitherUtil>);
}

void KisDitherUtilSchemaContractTest::thresholdModeSchemaRemainsStable()
{
    using Mode = KisDitherUtil::ThresholdMode;

    static_assert(std::is_enum_v<Mode>);
    static_assert(static_cast<int>(KisDitherUtil::Pattern) == 0);
    static_assert(static_cast<int>(KisDitherUtil::Noise) == 1);
}

void KisDitherUtilSchemaContractTest::patternValueModeSchemaRemainsStable()
{
    using Mode = KisDitherUtil::PatternValueMode;

    static_assert(std::is_enum_v<Mode>);
    static_assert(static_cast<int>(KisDitherUtil::Auto) == 0);
    static_assert(static_cast<int>(KisDitherUtil::Lightness) == 1);
    static_assert(static_cast<int>(KisDitherUtil::Alpha) == 2);
}

void KisDitherUtilSchemaContractTest::configurationAndThresholdSignaturesRemainStable()
{
    ASSERT_SIGNATURE(setConfiguration, void (KisDitherUtil::*)(const KisFilterConfiguration &, const QString &));
    ASSERT_SIGNATURE(threshold, qreal (KisDitherUtil::*)(const QPoint &));
    static_assert(std::is_same_v<decltype(std::declval<KisDitherUtil &>().setConfiguration(
                                     std::declval<const KisFilterConfiguration &>())),
                                 void>);
}

#undef ASSERT_SIGNATURE

QTEST_GUILESS_MAIN(KisDitherUtilSchemaContractTest)

#include "KisDitherUtilSchemaContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "timeline_frames_index_converter.h"

#include <QTest>

#include <type_traits>

namespace
{

#define ASSERT_SIGNATURE(method, ...)                                                                                  \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<__VA_ARGS__>(&TimelineFramesIndexConverter::method)), __VA_ARGS__>)

} // namespace

class TimelineFramesIndexConverterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void rowMappingSignaturesRemainStable();
    void activeDummySynchronizationSignaturesRemainStable();
    void timelineFilteringSignaturesRemainStable();
};

void TimelineFramesIndexConverterSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<TimelineFramesIndexConverter>);
    static_assert(std::is_constructible_v<TimelineFramesIndexConverter, KisDummiesFacadeBase *>);
    static_assert(std::is_destructible_v<TimelineFramesIndexConverter>);
}

void TimelineFramesIndexConverterSchemaContractTest::rowMappingSignaturesRemainStable()
{
    ASSERT_SIGNATURE(dummyFromRow, KisNodeDummy * (TimelineFramesIndexConverter::*)(int));
    ASSERT_SIGNATURE(rowForDummy, int (TimelineFramesIndexConverter::*)(KisNodeDummy *));
    ASSERT_SIGNATURE(rowCount, int (TimelineFramesIndexConverter::*)());
}

void TimelineFramesIndexConverterSchemaContractTest::activeDummySynchronizationSignaturesRemainStable()
{
    ASSERT_SIGNATURE(activeDummy, KisNodeDummy * (TimelineFramesIndexConverter::*)() const);
    ASSERT_SIGNATURE(updateActiveDummy, void (TimelineFramesIndexConverter::*)(KisNodeDummy *, bool *, bool *));
    ASSERT_SIGNATURE(notifyDummyRemoved, void (TimelineFramesIndexConverter::*)(KisNodeDummy *));
}

void TimelineFramesIndexConverterSchemaContractTest::timelineFilteringSignaturesRemainStable()
{
    ASSERT_SIGNATURE(setShowGlobalSelectionMask, void (TimelineFramesIndexConverter::*)(bool));
    ASSERT_SIGNATURE(isDummyAvailableForTimeline, bool (TimelineFramesIndexConverter::*)(KisNodeDummy *) const);
    ASSERT_SIGNATURE(isDummyVisible, bool (TimelineFramesIndexConverter::*)(KisNodeDummy *) const);
}

#undef ASSERT_SIGNATURE

QTEST_GUILESS_MAIN(TimelineFramesIndexConverterSchemaContractTest)

#include "TimelineFramesIndexConverterSchemaContractTest.moc"

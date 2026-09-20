/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include "brushengine/kis_stroke_random_source.h"

class KisStrokeRandomSourceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultSourcesAreStableAtLevelZero();
    void seededSourcesMatchAcrossWrappersAndLevels();
    void levelSwitchSelectsMatchingSourcePairs();
    void copyAndAssignmentShareSourcesAndLevel();
    void returnedSourcesOutliveWrapper();
};

void KisStrokeRandomSourceContractTest::defaultSourcesAreStableAtLevelZero()
{
    KisStrokeRandomSource wrapper;

    QCOMPARE(wrapper.levelOfDetail(), 0);
    const KisRandomSourceSP initialSource = wrapper.source();
    const KisPerStrokeRandomSourceSP initialPerStrokeSource = wrapper.perStrokeSource();
    QVERIFY(initialSource);
    QVERIFY(initialPerStrokeSource);
    QCOMPARE(wrapper.source().data(), initialSource.data());
    QCOMPARE(wrapper.perStrokeSource().data(), initialPerStrokeSource.data());
}

void KisStrokeRandomSourceContractTest::seededSourcesMatchAcrossWrappersAndLevels()
{
    KisStrokeRandomSource first(173);
    KisStrokeRandomSource second(173);

    for (const int level : {0, 4}) {
        first.setLevelOfDetail(level);
        second.setLevelOfDetail(level);

        for (int i = 0; i < 16; ++i) {
            QCOMPARE(first.source()->generate(), second.source()->generate());
        }
    }
}

void KisStrokeRandomSourceContractTest::levelSwitchSelectsMatchingSourcePairs()
{
    KisStrokeRandomSource wrapper(179);
    const KisRandomSourceSP levelZeroSource = wrapper.source();
    const KisPerStrokeRandomSourceSP levelZeroPerStrokeSource = wrapper.perStrokeSource();
    const QString key = QStringLiteral("shared-between-levels");
    const int levelZeroValue = levelZeroPerStrokeSource->generate(key, -50, 75);

    wrapper.setLevelOfDetail(-3);
    QCOMPARE(wrapper.levelOfDetail(), -3);
    const KisRandomSourceSP nonZeroSource = wrapper.source();
    const KisPerStrokeRandomSourceSP nonZeroPerStrokeSource = wrapper.perStrokeSource();
    QVERIFY(nonZeroSource.data() != levelZeroSource.data());
    QVERIFY(nonZeroPerStrokeSource.data() != levelZeroPerStrokeSource.data());
    QCOMPARE(nonZeroPerStrokeSource->generate(key, -50, 75), levelZeroValue);

    wrapper.setLevelOfDetail(5);
    QCOMPARE(wrapper.levelOfDetail(), 5);
    QCOMPARE(wrapper.source().data(), nonZeroSource.data());
    QCOMPARE(wrapper.perStrokeSource().data(), nonZeroPerStrokeSource.data());

    wrapper.setLevelOfDetail(0);
    QCOMPARE(wrapper.source().data(), levelZeroSource.data());
    QCOMPARE(wrapper.perStrokeSource().data(), levelZeroPerStrokeSource.data());
}

void KisStrokeRandomSourceContractTest::copyAndAssignmentShareSourcesAndLevel()
{
    KisStrokeRandomSource original(181);
    original.setLevelOfDetail(2);

    KisStrokeRandomSource copy(original);
    QCOMPARE(copy.levelOfDetail(), original.levelOfDetail());
    QCOMPARE(copy.source().data(), original.source().data());
    QCOMPARE(copy.perStrokeSource().data(), original.perStrokeSource().data());

    KisStrokeRandomSource assigned(191);
    KisStrokeRandomSource &assignmentResult = (assigned = original);
    QCOMPARE(&assignmentResult, &assigned);
    QCOMPARE(assigned.levelOfDetail(), original.levelOfDetail());
    QCOMPARE(assigned.source().data(), original.source().data());
    QCOMPARE(assigned.perStrokeSource().data(), original.perStrokeSource().data());

    const KisRandomSource *const sourceBeforeSelfAssignment = assigned.source().data();
    const KisPerStrokeRandomSource *const perStrokeBeforeSelfAssignment = assigned.perStrokeSource().data();
    KisStrokeRandomSource &selfAlias = assigned;
    KisStrokeRandomSource &selfAssignmentResult = (assigned = selfAlias);
    QCOMPARE(&selfAssignmentResult, &assigned);
    QCOMPARE(assigned.levelOfDetail(), 2);
    QCOMPARE(assigned.source().data(), sourceBeforeSelfAssignment);
    QCOMPARE(assigned.perStrokeSource().data(), perStrokeBeforeSelfAssignment);
}

void KisStrokeRandomSourceContractTest::returnedSourcesOutliveWrapper()
{
    KisRandomSourceSP randomSource;
    KisPerStrokeRandomSourceSP perStrokeSource;
    const QString key = QStringLiteral("survives-wrapper");
    int expectedPerStrokeValue = 0;

    {
        KisStrokeRandomSource wrapper(193);
        randomSource = wrapper.source();
        perStrokeSource = wrapper.perStrokeSource();
        expectedPerStrokeValue = perStrokeSource->generate(key, 10, 100);
    }

    const qreal normalized = randomSource->generateNormalized();
    QVERIFY(normalized >= 0.0);
    QVERIFY(normalized <= 1.0);
    QCOMPARE(perStrokeSource->generate(key, 10, 100), expectedPerStrokeValue);
}

QTEST_GUILESS_MAIN(KisStrokeRandomSourceContractTest)

#include "KisStrokeRandomSourceContractTest.moc"

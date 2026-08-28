/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_random_source.h"

#include <QTest>

#include <cmath>
#include <type_traits>

class KisRandomSourceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void seededSourcesReproduceAllGenerationPaths();
    void copiesAndAssignmentsForkTheCurrentSequence();
    void defaultSourceAndPointerAliasesPreserveLifetime();
};

void KisRandomSourceContractTest::seededSourcesReproduceAllGenerationPaths()
{
    KisRandomSource first(41);
    KisRandomSource second(41);
    for (int i = 0; i < 16; ++i) {
        QCOMPARE(first.generate(), second.generate());
    }

    KisRandomSource bounded(43);
    for (int i = 0; i < 64; ++i) {
        const int value = bounded.generate(-3, 7);
        QVERIFY(value >= -3);
        QVERIFY(value <= 7);
    }

    KisRandomSource normalized(47);
    for (int i = 0; i < 64; ++i) {
        const qreal value = normalized.generateNormalized();
        QVERIFY(value >= 0.0);
        QVERIFY(value <= 1.0);
    }

    KisRandomSource gaussianFirst(53);
    KisRandomSource gaussianSecond(53);
    for (int i = 0; i < 16; ++i) {
        const qreal firstValue = gaussianFirst.generateGaussian(12.5, 2.0);
        const qreal secondValue = gaussianSecond.generateGaussian(12.5, 2.0);
        QVERIFY(std::isfinite(firstValue));
        QCOMPARE(firstValue, secondValue);
    }
}

void KisRandomSourceContractTest::copiesAndAssignmentsForkTheCurrentSequence()
{
    KisRandomSource source(59);
    (void)source.generate();
    KisRandomSource copy(source);
    for (int i = 0; i < 16; ++i) {
        QCOMPARE(source.generate(), copy.generate());
    }

    KisRandomSource assignmentSource(61);
    (void)assignmentSource.generate();
    KisRandomSource assigned(67);
    assigned = assignmentSource;
    for (int i = 0; i < 16; ++i) {
        QCOMPARE(assignmentSource.generate(), assigned.generate());
    }

    KisRandomSource selfAssigned(71);
    KisRandomSource selfReference(71);
    KisRandomSource &selfAlias = selfAssigned;
    selfAssigned = selfAlias;
    QCOMPARE(selfAssigned.generate(), selfReference.generate());
}

void KisRandomSourceContractTest::defaultSourceAndPointerAliasesPreserveLifetime()
{
    static_assert(std::is_same_v<KisRandomSourceSP, KisSharedPtr<KisRandomSource>>);
    static_assert(std::is_same_v<KisRandomSourceWSP, KisWeakSharedPtr<KisRandomSource>>);

    KisRandomSource defaultSource;
    const qreal value = defaultSource.generateNormalized();
    QVERIFY(value >= 0.0);
    QVERIFY(value <= 1.0);

    KisRandomSourceSP strong(new KisRandomSource(73));
    KisRandomSourceWSP weak(strong);
    QVERIFY(weak.isValid());

    strong.clear();
    QVERIFY(!weak.isValid());
}

QTEST_GUILESS_MAIN(KisRandomSourceContractTest)

#include "KisRandomSourceContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisRandomGenerator2D.h"

#include <QTest>

#include <limits>

namespace
{

struct RandomSample {
    quint64 seed;
    qint64 x;
    qint64 y;
    quint64 integerValue;
    double normalizedValue;
};

constexpr RandomSample samples[] = {
    {0ULL, 0, 0, 0xb3834ca0e94a261fULL, 0x1.67069941d2945p-1},
    {0ULL, -1, -1, 0x8ddf3d45b974a7c1ULL, 0x1.1bbe7a8b72e95p-1},
    {1ULL, -1, -1, 0x89e28aa0a7148fb0ULL, 0x1.13c515414e292p-1},
    {0x0123456789abcdefULL, -17, 23, 0x51873fe6ab1bbdd0ULL, 0x1.461cff9aac6efp-2},
    {std::numeric_limits<quint64>::max(), -4096, 8192, 0x9eb75dccbf352c0fULL, 0x1.3d6ebb997e6a6p-1},
    {std::numeric_limits<quint64>::max(), 0, 0, 0xda8dde466631ee7fULL, 0x1.b51bbc8ccc63ep-1},
};

} // namespace

class KisRandomGenerator2DContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void fixedSeedsAndCoordinatesPreserveReferenceValues();
    void normalizedValuesPreserveIntegerMapping();
};

void KisRandomGenerator2DContractTest::fixedSeedsAndCoordinatesPreserveReferenceValues()
{
    for (const RandomSample &sample : samples) {
        quint64 firstLifetimeValue = 0;
        {
            KisRandomGenerator2D generator(sample.seed);
            firstLifetimeValue = generator.randomAt(sample.x, sample.y);
            QCOMPARE(firstLifetimeValue, sample.integerValue);
        }

        KisRandomGenerator2D nextLifetimeGenerator(sample.seed);
        QCOMPARE(nextLifetimeGenerator.randomAt(sample.x, sample.y), sample.integerValue);
        QCOMPARE(nextLifetimeGenerator.randomAt(sample.x, sample.y), firstLifetimeValue);
    }
}

void KisRandomGenerator2DContractTest::normalizedValuesPreserveIntegerMapping()
{
    for (const RandomSample &sample : samples) {
        KisRandomGenerator2D generator(sample.seed);
        const double normalizedValue = generator.doubleRandomAt(sample.x, sample.y);
        const double normalizedIntegerValue =
            static_cast<double>(sample.integerValue) / static_cast<double>(std::numeric_limits<quint64>::max());

        QCOMPARE(normalizedValue, sample.normalizedValue);
        QCOMPARE(normalizedValue, normalizedIntegerValue);
        QVERIFY(normalizedValue >= 0.0);
        QVERIFY(normalizedValue <= 1.0);
    }
}

QTEST_GUILESS_MAIN(KisRandomGenerator2DContractTest)

#include "KisRandomGenerator2DContractTest.moc"

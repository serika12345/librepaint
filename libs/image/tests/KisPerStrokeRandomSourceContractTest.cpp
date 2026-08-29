/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QtTest>

#include <type_traits>

#include "brushengine/KisPerStrokeRandomSource.h"

class KisPerStrokeRandomSourceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void sameKeyIsStableAndValuesStayInRequestedRanges();
    void copyPreservesCachedAndFutureKeyValues();
    void strongAndWeakAliasesShareAndReleaseLifetime();
};

void KisPerStrokeRandomSourceContractTest::sameKeyIsStableAndValuesStayInRequestedRanges()
{
    KisPerStrokeRandomSource source;
    const QString key = QStringLiteral("stable-key");

    const int first = source.generate(key, -37, 42);
    const int second = source.generate(key, -37, 42);

    QCOMPARE(second, first);
    QVERIFY(first >= -37);
    QVERIFY(first < 42);

    const qreal firstNormalized = source.generateNormalized(key);
    const qreal secondNormalized = source.generateNormalized(key);

    QCOMPARE(secondNormalized, firstNormalized);
    QVERIFY(firstNormalized >= 0.0);
    QVERIFY(firstNormalized <= 1.0);

    for (int i = 0; i < 16; ++i) {
        const int value = source.generate(QStringLiteral("range-%1").arg(i), 11, 19);
        QVERIFY(value >= 11);
        QVERIFY(value < 19);
    }
}

void KisPerStrokeRandomSourceContractTest::copyPreservesCachedAndFutureKeyValues()
{
    KisPerStrokeRandomSource source;
    const QString cachedKey = QStringLiteral("cached-before-copy");
    const QString futureKey = QStringLiteral("queried-after-copy");

    const int cachedValue = source.generate(cachedKey, 100, 1000);
    const qreal cachedNormalized = source.generateNormalized(cachedKey);
    KisPerStrokeRandomSource copy(source);

    QCOMPARE(copy.generate(cachedKey, 100, 1000), cachedValue);
    QCOMPARE(copy.generateNormalized(cachedKey), cachedNormalized);
    QCOMPARE(copy.generate(futureKey, -1000, 1000), source.generate(futureKey, -1000, 1000));
    QCOMPARE(copy.generateNormalized(futureKey), source.generateNormalized(futureKey));
}

void KisPerStrokeRandomSourceContractTest::strongAndWeakAliasesShareAndReleaseLifetime()
{
    static_assert(std::is_same_v<KisPerStrokeRandomSourceSP, KisSharedPtr<KisPerStrokeRandomSource>>);
    static_assert(std::is_same_v<KisPerStrokeRandomSourceWSP, KisWeakSharedPtr<KisPerStrokeRandomSource>>);

    KisPerStrokeRandomSourceWSP weak;

    {
        KisPerStrokeRandomSourceSP owner(new KisPerStrokeRandomSource);
        weak = owner;

        QVERIFY(weak.isValid());
        QCOMPARE(owner->refCount(), 1);

        {
            KisPerStrokeRandomSourceSP promoted = weak.toStrongRef();
            QVERIFY(promoted);
            QCOMPARE(promoted.data(), owner.data());
            QCOMPARE(owner->refCount(), 2);
            QCOMPARE(promoted->generate(QStringLiteral("owned"), 3, 9), owner->generate(QStringLiteral("owned"), 3, 9));
        }

        QCOMPARE(owner->refCount(), 1);
    }

    QVERIFY(!weak.isValid());
    QVERIFY(!weak.toStrongRef());
}

QTEST_GUILESS_MAIN(KisPerStrokeRandomSourceContractTest)

#include "KisPerStrokeRandomSourceContractTest.moc"

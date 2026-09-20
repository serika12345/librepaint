/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResourceCacheStorage.h>

#include <QTest>

namespace
{
int safeAssertCount = 0;
}

void kis_safe_assert_recoverable(const char *assertion, const char *, int)
{
    ++safeAssertCount;
    Q_UNUSED(assertion);
}

class KoResourceCacheStorageContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void unavailableEntriesStayInvalid();
    void storesDistinctValues();
    void duplicateKeysReportCacheAliasing();
};

void KoResourceCacheStorageContractTest::init()
{
    safeAssertCount = 0;
}

void KoResourceCacheStorageContractTest::unavailableEntriesStayInvalid()
{
    // Consumer: Paint presets deciding whether a prepared brush cache is available.
    // Operation: A preset asks for a cache entry that preparation did not create.
    // Observable result: The lookup returns an invalid QVariant.
    // Failure impact: A brush can treat an absent outline or image pyramid as usable and render incorrectly.
    KoResourceCacheStorage cache;

    QVERIFY(!cache.fetch(QStringLiteral("missing")).isValid());
}

void KoResourceCacheStorageContractTest::storesDistinctValues()
{
    // Consumer: Paint presets reusing separately prepared brush outlines and image pyramids.
    // Operation: Preparation stores values under distinct cache keys and the preset reads both later.
    // Observable result: Each key returns its own original value.
    // Failure impact: A stroke can receive a mismatched prepared resource or regenerate work that should be reused.
    KoResourceCacheStorage cache;

    cache.put(QStringLiteral("integer"), 17);
    cache.put(QStringLiteral("text"), QStringLiteral("payload"));

    QCOMPARE(cache.fetch(QStringLiteral("integer")), QVariant(17));
    QCOMPARE(cache.fetch(QStringLiteral("text")), QVariant(QStringLiteral("payload")));
    QCOMPARE(safeAssertCount, 0);
}

void KoResourceCacheStorageContractTest::duplicateKeysReportCacheAliasing()
{
    // Consumer: Paint preset cache preparation that assigns each generated resource one key.
    // Operation: Preparation attempts to store two values under the same cache key.
    // Observable result: The cache reports a recoverable cache-aliasing error.
    // Failure impact: A key collision can silently substitute one prepared brush resource for another.
    KoResourceCacheStorage cache;
    cache.put(QStringLiteral("shared-key"), QStringLiteral("first"));
    cache.put(QStringLiteral("shared-key"), QStringLiteral("second"));

    QCOMPARE(safeAssertCount, 1);
}

QTEST_GUILESS_MAIN(KoResourceCacheStorageContractTest)

#include "KoResourceCacheStorageContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResourceCacheStorage.h>

#include <QByteArray>
#include <QTest>

namespace
{
int safeAssertCount = 0;
QByteArray safeAssertExpression;

class DestructionTrackedStorage final : public KoResourceCacheStorage
{
public:
    explicit DestructionTrackedStorage(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~DestructionTrackedStorage() override
    {
        *m_destroyed = true;
    }

private:
    bool *m_destroyed;
};
}

void kis_safe_assert_recoverable(const char *assertion, const char *, int)
{
    ++safeAssertCount;
    safeAssertExpression = assertion;
}

class KoResourceCacheStorageContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init();
    void startsEmptyAndDestroysPolymorphically();
    void storesDistinctValues();
    void replacesValueAfterDuplicateKeyDiagnostic();
};

void KoResourceCacheStorageContractTest::init()
{
    safeAssertCount = 0;
    safeAssertExpression.clear();
}

void KoResourceCacheStorageContractTest::startsEmptyAndDestroysPolymorphically()
{
    bool destroyed = false;
    KoResourceCacheInterface *cache = new DestructionTrackedStorage(&destroyed);

    QVERIFY(!cache->fetch(QStringLiteral("missing")).isValid());
    delete cache;
    QVERIFY(destroyed);
}

void KoResourceCacheStorageContractTest::storesDistinctValues()
{
    KoResourceCacheStorage cache;

    cache.put(QStringLiteral("integer"), 17);
    cache.put(QStringLiteral("text"), QStringLiteral("payload"));

    QCOMPARE(cache.fetch(QStringLiteral("integer")), QVariant(17));
    QCOMPARE(cache.fetch(QStringLiteral("text")), QVariant(QStringLiteral("payload")));
    QCOMPARE(safeAssertCount, 0);
}

void KoResourceCacheStorageContractTest::replacesValueAfterDuplicateKeyDiagnostic()
{
    KoResourceCacheStorage cache;
    cache.put(QStringLiteral("shared-key"), QStringLiteral("first"));
    cache.put(QStringLiteral("shared-key"), QStringLiteral("second"));

    QCOMPARE(safeAssertCount, 1);
    QCOMPARE(safeAssertExpression, QByteArray("!m_d->map.contains(key)"));
    QCOMPARE(cache.fetch(QStringLiteral("shared-key")), QVariant(QStringLiteral("second")));
}

QTEST_GUILESS_MAIN(KoResourceCacheStorageContractTest)

#include "KoResourceCacheStorageContractTest.moc"

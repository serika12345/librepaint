/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResourceCacheInterface.h>

#include <QMetaType>
#include <QString>
#include <QTest>
#include <QVariant>

namespace
{
class RecordingResourceCache final : public KoResourceCacheInterface
{
public:
    explicit RecordingResourceCache(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~RecordingResourceCache() override
    {
        *m_destroyed = true;
    }

    QVariant fetch(const QString &key) const override
    {
        fetchedKey = key;
        return QStringLiteral("cached-value");
    }

    void put(const QString &key, const QVariant &value) override
    {
        putKey = key;
        putValue = value;
    }

    mutable QString fetchedKey;
    QString putKey;
    QVariant putValue;

private:
    bool *m_destroyed;
};
}

class KoResourceCacheInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dispatchesCacheOperationsAndSupportsPolymorphicLifetime();
    void preservesRelatedResourceCookie();
    void registersSharedPointerMetaType();
};

void KoResourceCacheInterfaceContractTest::dispatchesCacheOperationsAndSupportsPolymorphicLifetime()
{
    bool destroyed = false;
    auto *recordingCache = new RecordingResourceCache(&destroyed);
    KoResourceCacheInterface *cache = recordingCache;

    cache->put(QStringLiteral("outline"), 27);
    QCOMPARE(recordingCache->putKey, QStringLiteral("outline"));
    QCOMPARE(recordingCache->putValue, QVariant(27));

    QCOMPARE(cache->fetch(QStringLiteral("pyramid")), QVariant(QStringLiteral("cached-value")));
    QCOMPARE(recordingCache->fetchedKey, QStringLiteral("pyramid"));

    delete cache;
    QVERIFY(destroyed);
}

void KoResourceCacheInterfaceContractTest::preservesRelatedResourceCookie()
{
    bool destroyed = false;
    RecordingResourceCache cache(&destroyed);

    QCOMPARE(cache.relatedResourceCookie(), KoResourceCacheInterface::RelatedResourceCookie());
    const KoResourceCacheInterface::RelatedResourceCookie cookie = 0x173u;
    cache.setRelatedResourceCookie(cookie);
    QCOMPARE(cache.relatedResourceCookie(), cookie);
}

void KoResourceCacheInterfaceContractTest::registersSharedPointerMetaType()
{
    const QMetaType type = QMetaType::fromName("KoResourceCacheInterfaceSP");

    QVERIFY(type.isValid());
    QCOMPARE(type, QMetaType::fromType<KoResourceCacheInterfaceSP>());
}

QTEST_GUILESS_MAIN(KoResourceCacheInterfaceContractTest)

#include "KoResourceCacheInterfaceContractTest.moc"

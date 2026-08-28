/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResourceCachePrefixedStorageWrapper.h>

#include <QHash>
#include <QTest>

namespace
{
class RecordingResourceCache final : public KoResourceCacheInterface
{
public:
    explicit RecordingResourceCache(bool *destroyed = nullptr)
        : m_destroyed(destroyed)
    {
    }

    ~RecordingResourceCache() override
    {
        if (m_destroyed) {
            *m_destroyed = true;
        }
    }

    QVariant fetch(const QString &key) const override
    {
        fetchedKey = key;
        return values.value(key);
    }

    void put(const QString &key, const QVariant &value) override
    {
        putKey = key;
        putValue = value;
        values.insert(key, value);
    }

    mutable QHash<QString, QVariant> values;
    mutable QString fetchedKey;
    QString putKey;
    QVariant putValue;

private:
    bool *m_destroyed;
};
}

class KoResourceCachePrefixedStorageWrapperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void prefixesAndForwardsCacheOperations();
    void retainsBaseInterfaceForItsLifetime();
};

void KoResourceCachePrefixedStorageWrapperContractTest::prefixesAndForwardsCacheOperations()
{
    QSharedPointer<RecordingResourceCache> base(new RecordingResourceCache);
    base->values.insert(QStringLiteral("MaskingBrush/Preset/existing"), 17);
    KoResourceCachePrefixedStorageWrapper cache(
        QStringLiteral("MaskingBrush/Preset/"), base);

    QCOMPARE(cache.fetch(QStringLiteral("existing")), QVariant(17));
    QCOMPARE(base->fetchedKey, QStringLiteral("MaskingBrush/Preset/existing"));

    cache.put(QStringLiteral("generated"), QStringLiteral("payload"));
    QCOMPARE(base->putKey, QStringLiteral("MaskingBrush/Preset/generated"));
    QCOMPARE(base->putValue, QVariant(QStringLiteral("payload")));
    QCOMPARE(base->values.value(QStringLiteral("MaskingBrush/Preset/generated")),
             QVariant(QStringLiteral("payload")));

    KoResourceCachePrefixedStorageWrapper unprefixed(QString(), base);
    unprefixed.put(QStringLiteral("plain"), 23);
    QCOMPARE(base->putKey, QStringLiteral("plain"));
}

void KoResourceCachePrefixedStorageWrapperContractTest::retainsBaseInterfaceForItsLifetime()
{
    bool destroyed = false;
    KoResourceCacheInterfaceSP base(new RecordingResourceCache(&destroyed));
    {
        KoResourceCachePrefixedStorageWrapper cache(QStringLiteral("scope/"), base);
        base.clear();
        QVERIFY(!destroyed);
        cache.put(QStringLiteral("key"), QStringLiteral("value"));
        QCOMPARE(cache.fetch(QStringLiteral("key")), QVariant(QStringLiteral("value")));
    }
    QVERIFY(destroyed);
}

QTEST_GUILESS_MAIN(KoResourceCachePrefixedStorageWrapperContractTest)

#include "KoResourceCachePrefixedStorageWrapperContractTest.moc"

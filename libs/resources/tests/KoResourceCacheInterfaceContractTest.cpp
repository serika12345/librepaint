/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResourceCacheInterface.h>

#include <QMetaType>
#include <QTest>
#include <QVariant>

namespace
{
class CookieResourceCache final : public KoResourceCacheInterface
{
public:
    QVariant fetch(const QString &) const override
    {
        return {};
    }

    void put(const QString &, const QVariant &) override
    {
    }
};
}

class KoResourceCacheInterfaceContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cacheOwnershipCookieSurvivesPreparation();
    void cacheResultsSupportQueuedPresetUpdates();
};

void KoResourceCacheInterfaceContractTest::cacheOwnershipCookieSurvivesPreparation()
{
    // Consumer: Painting snapshots that reject a cache created for a previous brush or canvas-resource state.
    // Operation: Cache preparation assigns its resource ownership cookie and the snapshot later reads it.
    // Observable result: The assigned cookie remains available for cache-validity comparison.
    // Failure impact: A stroke can reuse a stale brush outline or resource cache after the selected resource changes.
    CookieResourceCache cache;

    QCOMPARE(cache.relatedResourceCookie(), KoResourceCacheInterface::RelatedResourceCookie());
    const KoResourceCacheInterface::RelatedResourceCookie cookie = 0x173u;
    cache.setRelatedResourceCookie(cookie);
    QCOMPARE(cache.relatedResourceCookie(), cookie);
}

void KoResourceCacheInterfaceContractTest::cacheResultsSupportQueuedPresetUpdates()
{
    // Consumer: The preset shadow updater transferring prepared caches from a background job to the UI thread.
    // Operation: The updater emits a queued result containing a shared resource-cache interface.
    // Observable result: Qt resolves the registered cache-result type by its signal name.
    // Failure impact: A selected brush can miss its prepared cache or fail when its background update reaches the UI.
    const QMetaType type = QMetaType::fromName("KoResourceCacheInterfaceSP");

    QVERIFY(type.isValid());
}

QTEST_GUILESS_MAIN(KoResourceCacheInterfaceContractTest)

#include "KoResourceCacheInterfaceContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoResourceCachePrefixedStorageWrapper.h>
#include <KoResourceCacheStorage.h>

#include <QTest>

void kis_safe_assert_recoverable(const char *assertion, const char *file, int line)
{
    qFatal("unexpected safe assertion: %s at %s:%d", assertion, file, line);
}

class KoResourceCachePrefixedStorageWrapperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void maskingBrushCachesUseSeparateNamespaces();
};

void KoResourceCachePrefixedStorageWrapperContractTest::maskingBrushCachesUseSeparateNamespaces()
{
    // Consumer: Paint presets that prepare caches for a main brush and its masking brush.
    // Operation: Both brushes store an outline under the same logical cache key.
    // Observable result: The masking brush reads its prefixed outline while the main brush keeps its own outline.
    // Failure impact: A brush stroke can use the masking brush's cache and render the wrong outline or dab.
    KoResourceCacheInterfaceSP base(new KoResourceCacheStorage);
    base->put(QStringLiteral("outline"), QStringLiteral("main-outline"));
    KoResourceCachePrefixedStorageWrapper cache(
        QStringLiteral("MaskingBrush/Preset/"), base);

    cache.put(QStringLiteral("outline"), QStringLiteral("masking-outline"));

    QCOMPARE(base->fetch(QStringLiteral("outline")), QVariant(QStringLiteral("main-outline")));
    QCOMPARE(cache.fetch(QStringLiteral("outline")), QVariant(QStringLiteral("masking-outline")));
    QCOMPARE(base->fetch(QStringLiteral("MaskingBrush/Preset/outline")),
             QVariant(QStringLiteral("masking-outline")));
}

QTEST_GUILESS_MAIN(KoResourceCachePrefixedStorageWrapperContractTest)

#include "KoResourceCachePrefixedStorageWrapperContractTest.moc"

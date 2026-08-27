/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "animation/kis_animation_frame_cache_fwd.h"

#include "kis_shared_ptr.h"

#include <QAtomicInt>
#include <QTest>

#include <type_traits>

class KisAnimationFrameCache
{
public:
    explicit KisAnimationFrameCache(int *destructionCount)
        : m_destructionCount(destructionCount)
    {
    }

    ~KisAnimationFrameCache()
    {
        ++*m_destructionCount;
        Q_ASSERT(m_references == 0);
        if (m_weakReference && !m_weakReference->deref()) {
            delete m_weakReference;
        }
    }

    bool ref()
    {
        return m_references.ref();
    }

    bool deref()
    {
        return m_references.deref();
    }

    QAtomicInt *sharedWeakReference()
    {
        if (!m_weakReference) {
            m_weakReference = new QAtomicInt;
            m_weakReference->ref();
        }
        return m_weakReference;
    }

private:
    int *m_destructionCount;
    QAtomicInt m_references = 0;
    QAtomicInt *m_weakReference = nullptr;
};

class KisAnimationFrameCacheFwdContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void strongAndWeakAliasesShareAndTrackCacheLifetime();
};

void KisAnimationFrameCacheFwdContractTest::strongAndWeakAliasesShareAndTrackCacheLifetime()
{
    static_assert(std::is_same_v<KisAnimationFrameCacheSP, KisSharedPtr<KisAnimationFrameCache>>);
    static_assert(std::is_same_v<KisAnimationFrameCacheWSP, KisWeakSharedPtr<KisAnimationFrameCache>>);

    int destructionCount = 0;
    KisAnimationFrameCacheSP strong = new KisAnimationFrameCache(&destructionCount);
    KisAnimationFrameCacheWSP weak = strong;
    QVERIFY(weak.isValid());

    KisAnimationFrameCacheSP promoted = weak.toStrongRef();
    QCOMPARE(promoted.data(), strong.data());
    strong.clear();
    QCOMPARE(destructionCount, 0);
    QVERIFY(weak.isValid());

    promoted.clear();
    QCOMPARE(destructionCount, 1);
    QVERIFY(!weak.isValid());
    QVERIFY(weak.toStrongRef().isNull());
}

QTEST_GUILESS_MAIN(KisAnimationFrameCacheFwdContractTest)

#include "KisAnimationFrameCacheFwdContractTest.moc"

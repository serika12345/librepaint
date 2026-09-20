/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_pinned_shared_ptr.h>
#include <kis_shared.h>

#include <QDebug>
#include <QTest>

#include <type_traits>

namespace
{

class TrackedShared : public KisShared
{
public:
    TrackedShared(bool *destroyed, int value)
        : m_destroyed(destroyed)
        , value(value)
    {
    }

    ~TrackedShared()
    {
        *m_destroyed = true;
    }

    bool *m_destroyed;
    int value;
};

static_assert(!std::is_convertible_v<KisPinnedSharedPtr<TrackedShared>, const TrackedShared *>);

} // namespace

class KisPinnedSharedPtrTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nullStateConversions();
    void constructorsShareOwnership();
    void debugOutputMatchesContainedPointer();
};

void KisPinnedSharedPtrTest::nullStateConversions()
{
    KisPinnedSharedPtr<TrackedShared> pointer;

    QVERIFY(!pointer);
    QVERIFY(pointer ? false : true);
}

void KisPinnedSharedPtrTest::constructorsShareOwnership()
{
    bool rawDestroyed = false;
    {
        KisPinnedSharedPtr<TrackedShared> pointer(new TrackedShared(&rawDestroyed, 7));
        QVERIFY(pointer);
        QCOMPARE(pointer.data()->value, 7);
    }
    QVERIFY(rawDestroyed);

    bool sharedDestroyed = false;
    {
        KisSharedPtr<TrackedShared> shared(new TrackedShared(&sharedDestroyed, 11));
        KisWeakSharedPtr<TrackedShared> weak(shared);
        KisPinnedSharedPtr<TrackedShared> fromShared(shared);
        KisPinnedSharedPtr<TrackedShared> fromWeak(weak);

        QCOMPARE(shared->refCount(), 3);
        QCOMPARE(fromShared.data(), shared.data());
        QCOMPARE(fromWeak.data(), shared.data());
    }
    QVERIFY(sharedDestroyed);
}

void KisPinnedSharedPtrTest::debugOutputMatchesContainedPointer()
{
    bool destroyed = false;
    KisPinnedSharedPtr<TrackedShared> pointer(new TrackedShared(&destroyed, 13));

    QString actual;
    {
        QDebug debug(&actual);
        debug << pointer;
    }

    QString expected;
    {
        QDebug debug(&expected);
        debug.nospace() << pointer.data();
    }

    QCOMPARE(actual, expected);
}

QTEST_GUILESS_MAIN(KisPinnedSharedPtrTest)

#include "KisPinnedSharedPtrTest.moc"

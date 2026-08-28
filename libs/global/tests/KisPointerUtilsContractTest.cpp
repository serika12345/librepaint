/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_pointer_utils.h"

#include "kis_pinned_shared_ptr.h"
#include "kis_shared_ptr.h"

#include <QList>
#include <QTest>

#include <type_traits>

namespace
{

class IntrusiveBase
{
public:
    explicit IntrusiveBase(int value)
        : value(value)
    {
    }

    virtual ~IntrusiveBase() = default;

    void ref()
    {
        ++m_referenceCount;
    }

    bool deref()
    {
        return --m_referenceCount != 0;
    }

    int value;

private:
    int m_referenceCount = 0;
};

class IntrusiveDerived : public IntrusiveBase
{
public:
    using IntrusiveBase::IntrusiveBase;
};

class IntrusiveSibling : public IntrusiveBase
{
public:
    using IntrusiveBase::IntrusiveBase;
};

} // namespace

class KisPointerUtilsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void rawPointerConversionsTransferOwnershipAndPreserveOrder();
    void weakListConversionsPreserveLiveEntriesAndAllOrNothingRule();
    void implicitListConversionPreservesValues();
    void sharedPointerRemovalReturnsContainedPointer();
    void sharedPointerTraitsPreserveTypesAndDynamicCasts();
};

void KisPointerUtilsContractTest::rawPointerConversionsTransferOwnershipAndPreserveOrder()
{
    auto *single = new int(7);
    const QSharedPointer<int> shared = toQShared(single);
    QCOMPARE(shared.data(), single);
    QCOMPARE(*shared, 7);

    auto *first = new int(11);
    auto *second = new int(13);
    const QList<QSharedPointer<int>> list = listToQShared(QList<int *>{first, second});

    QCOMPARE(list.size(), 2);
    QCOMPARE(list.at(0).data(), first);
    QCOMPARE(list.at(1).data(), second);
    QCOMPARE(*list.at(0), 11);
    QCOMPARE(*list.at(1), 13);
}

void KisPointerUtilsContractTest::weakListConversionsPreserveLiveEntriesAndAllOrNothingRule()
{
    QSharedPointer<int> first(new int(17));
    QSharedPointer<int> second(new int(19));
    const QList<QSharedPointer<int>> strong{first, second};
    const QList<QWeakPointer<int>> weak = listStrongToWeak(strong);

    const QList<QSharedPointer<int>> restored = listWeakToStrong(weak);
    QCOMPARE(restored.size(), 2);
    QCOMPARE(restored.at(0).data(), first.data());
    QCOMPARE(restored.at(1).data(), second.data());

    QSharedPointer<int> live(new int(23));
    QSharedPointer<int> expired(new int(29));
    const QList<QWeakPointer<int>> partlyExpired{QWeakPointer<int>(live), QWeakPointer<int>(expired)};
    expired.clear();

    QVERIFY(listWeakToStrong(partlyExpired).isEmpty());
    const QList<QSharedPointer<int>> available = listWeakToStrong(partlyExpired, false);
    QCOMPARE(available.size(), 1);
    QCOMPARE(available.constFirst().data(), live.data());
}

void KisPointerUtilsContractTest::implicitListConversionPreservesValues()
{
    const QList<int> source{2, 3, 5};
    const QList<qint64> converted = implicitCastList<qint64>(source);

    QCOMPARE(converted, QList<qint64>({2, 3, 5}));
}

void KisPointerUtilsContractTest::sharedPointerRemovalReturnsContainedPointer()
{
    auto *raw = new IntrusiveDerived(31);
    QCOMPARE(removeSharedPointer(raw), raw);
    delete raw;

    QSharedPointer<IntrusiveDerived> qtShared(new IntrusiveDerived(37));
    QCOMPARE(removeSharedPointer(qtShared), qtShared.data());

    KisSharedPtr<IntrusiveDerived> shared(new IntrusiveDerived(41));
    QCOMPARE(removeSharedPointer(shared), shared.data());

    KisPinnedSharedPtr<IntrusiveDerived> pinned(new IntrusiveDerived(43));
    QCOMPARE(removeSharedPointer(pinned), pinned.data());
}

void KisPointerUtilsContractTest::sharedPointerTraitsPreserveTypesAndDynamicCasts()
{
    using QtTraits = KisSharedPointerTraits<QSharedPointer<IntrusiveBase>>;
    using SharedTraits = KisSharedPointerTraits<KisSharedPtr<IntrusiveBase>>;
    using PinnedTraits = KisSharedPointerTraits<KisPinnedSharedPtr<IntrusiveBase>>;

    static_assert(std::is_same_v<QtTraits::ValueType, IntrusiveBase>);
    static_assert(std::is_same_v<QtTraits::SharedPointerType<IntrusiveDerived>, QSharedPointer<IntrusiveDerived>>);
    static_assert(std::is_same_v<SharedTraits::ValueType, IntrusiveBase>);
    static_assert(std::is_same_v<SharedTraits::SharedPointerType<IntrusiveDerived>, KisSharedPtr<IntrusiveDerived>>);
    static_assert(std::is_same_v<PinnedTraits::ValueType, IntrusiveBase>);
    static_assert(std::is_same_v<PinnedTraits::SharedPointerType<IntrusiveDerived>, KisPinnedSharedPtr<IntrusiveDerived>>);

    QSharedPointer<IntrusiveBase> qtBase(new IntrusiveDerived(47));
    const auto qtDerived = QtTraits::dynamicCastSP<IntrusiveDerived>(qtBase);
    const auto qtSibling = QtTraits::dynamicCastSP<IntrusiveSibling>(qtBase);
    QCOMPARE(qtDerived.data(), qtBase.data());
    QVERIFY(qtSibling.isNull());

    KisSharedPtr<IntrusiveBase> sharedBase(new IntrusiveDerived(53));
    const auto sharedDerived = SharedTraits::dynamicCastSP<IntrusiveDerived>(sharedBase);
    const auto sharedSibling = SharedTraits::dynamicCastSP<IntrusiveSibling>(sharedBase);
    QCOMPARE(sharedDerived.data(), sharedBase.data());
    QVERIFY(sharedSibling.isNull());

    KisPinnedSharedPtr<IntrusiveBase> pinnedBase(new IntrusiveDerived(59));
    const auto pinnedDerived = PinnedTraits::dynamicCastSP<IntrusiveDerived>(pinnedBase);
    const auto pinnedSibling = PinnedTraits::dynamicCastSP<IntrusiveSibling>(pinnedBase);
    QCOMPARE(pinnedDerived.data(), pinnedBase.data());
    QVERIFY(pinnedSibling.isNull());
}

QTEST_GUILESS_MAIN(KisPointerUtilsContractTest)

#include "KisPointerUtilsContractTest.moc"

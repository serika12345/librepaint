/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_shared.h>
#include <kis_shared_ptr.h>

#include <QTest>

#include <type_traits>

namespace
{

struct LifetimeState {
    int destroyed = 0;
};

class TrackedBase : public KisShared
{
public:
    TrackedBase(LifetimeState *state, int value)
        : state(state)
        , value(value)
    {
    }

    virtual ~TrackedBase()
    {
        ++state->destroyed;
    }

    LifetimeState *state;
    int value;
};

class TrackedDerived : public TrackedBase
{
public:
    using TrackedBase::TrackedBase;
};

static_assert(std::is_same_v<KisSharedPtr<TrackedBase>::element_type, TrackedBase>);
static_assert(std::is_same_v<KisSharedPtr<TrackedBase>::weak_type, KisWeakSharedPtr<TrackedBase>>);
static_assert(std::is_same_v<KisWeakSharedPtr<TrackedBase>::element_type, TrackedBase>);
static_assert(std::is_convertible_v<TrackedBase *, KisSharedPtr<TrackedBase>>);

} // namespace

class KisSharedPtrContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAliasesAndNullState();
    void referenceHooksControlLifetime();
    void strongPointersShareAccessAndAssignment();
    void strongAndWeakConversionsPreserveIdentity();
    void weakPointersObserveLiveAndExpiredObjects();
};

void KisSharedPtrContractTest::typeAliasesAndNullState()
{
    KisSharedPtr<TrackedBase> strong;
    QVERIFY(strong.isNull());
    QCOMPARE(static_cast<const TrackedBase *>(strong), nullptr);

    KisWeakSharedPtr<TrackedBase> weak;
    QVERIFY(weak.isNull());
    QVERIFY(!weak.isValid());
    QCOMPARE(static_cast<const TrackedBase *>(weak), nullptr);
}

void KisSharedPtrContractTest::referenceHooksControlLifetime()
{
    LifetimeState freeFunctionState;
    auto *freeFunctionObject = new TrackedBase(&freeFunctionState, 3);

    kisSharedPtrAddReference(freeFunctionObject);
    kisSharedPtrAddReference(freeFunctionObject);
    QCOMPARE(freeFunctionObject->refCount(), 2);
    QVERIFY(kisSharedPtrRelease(freeFunctionObject));
    QCOMPARE(freeFunctionObject->refCount(), 1);
    QVERIFY(!kisSharedPtrRelease(freeFunctionObject));
    QCOMPARE(freeFunctionState.destroyed, 1);

    LifetimeState staticFunctionState;
    auto *staticFunctionObject = new TrackedBase(&staticFunctionState, 5);
    KisSharedPtr<TrackedBase> tracker;

    KisSharedPtr<TrackedBase>::ref(&tracker, staticFunctionObject);
    QCOMPARE(staticFunctionObject->refCount(), 1);
    QVERIFY(!KisSharedPtr<TrackedBase>::deref(&tracker, staticFunctionObject));
    QCOMPARE(staticFunctionState.destroyed, 1);

    KisSharedPtr<TrackedBase>::ref(&tracker, nullptr);
    QVERIFY(KisSharedPtr<TrackedBase>::deref(&tracker, nullptr));
}

void KisSharedPtrContractTest::strongPointersShareAccessAndAssignment()
{
    LifetimeState firstState;
    LifetimeState replacementState;

    {
        KisSharedPtr<TrackedBase> first(new TrackedBase(&firstState, 7));
        TrackedBase *firstRaw = first.data();
        QCOMPARE(firstRaw->refCount(), 1);

        KisSharedPtr<TrackedBase> copy(first);
        KisSharedPtr<TrackedBase> assigned;
        assigned = first;
        KisSharedPtr<TrackedBase> rawAssigned;
        rawAssigned = firstRaw;
        QCOMPARE(firstRaw->refCount(), 4);

        QCOMPARE(first.data(), firstRaw);
        QCOMPARE(first.constData(), firstRaw);
        QCOMPARE((*first).value, 7);
        QCOMPARE(first->value, 7);

        const KisSharedPtr<TrackedBase> &constFirst = first;
        QCOMPARE(constFirst.data(), firstRaw);
        QCOMPARE(constFirst.constData(), firstRaw);
        QCOMPARE((*constFirst).value, 7);
        QCOMPARE(constFirst->value, 7);

        QVERIFY(first == firstRaw);
        QVERIFY(first != nullptr);
        QVERIFY(first == copy);

        KisSharedPtr<TrackedBase> replacement(new TrackedBase(&replacementState, 11));
        QVERIFY(first != replacement.data());
        QVERIFY(first != replacement);

        assigned.attach(replacement.data());
        QCOMPARE(assigned.data(), replacement.data());
        QCOMPARE(firstRaw->refCount(), 3);
        QCOMPARE(replacement->refCount(), 2);

        assigned.clear();
        QVERIFY(assigned.isNull());
        QCOMPARE(replacement->refCount(), 1);
    }

    QCOMPARE(firstState.destroyed, 1);
    QCOMPARE(replacementState.destroyed, 1);
}

void KisSharedPtrContractTest::strongAndWeakConversionsPreserveIdentity()
{
    LifetimeState state;
    KisSharedPtr<TrackedDerived> derived(new TrackedDerived(&state, 13));

    KisWeakSharedPtr<TrackedDerived> weakFromRaw(derived.data());
    KisWeakSharedPtr<TrackedDerived> weakFromStrong(derived);
    KisWeakSharedPtr<TrackedDerived> weakCopy(weakFromStrong);

    KisSharedPtr<TrackedDerived> promoted(weakFromStrong);
    KisSharedPtr<TrackedBase> base = static_cast<KisSharedPtr<TrackedBase>>(derived);
    KisWeakSharedPtr<TrackedBase> weakBase = static_cast<KisWeakSharedPtr<TrackedBase>>(weakFromStrong);

    QCOMPARE(weakFromRaw.data(), derived.data());
    QCOMPARE(weakCopy.data(), derived.data());
    QCOMPARE(promoted.data(), derived.data());
    QCOMPARE(base.data(), static_cast<TrackedBase *>(derived.data()));
    QCOMPARE(weakBase.data(), static_cast<TrackedBase *>(derived.data()));
    QCOMPARE(derived->refCount(), 3);
}

void KisSharedPtrContractTest::weakPointersObserveLiveAndExpiredObjects()
{
    LifetimeState observedState;
    LifetimeState otherState;
    KisWeakSharedPtr<TrackedBase> observed;

    {
        KisSharedPtr<TrackedBase> owner(new TrackedBase(&observedState, 17));
        KisSharedPtr<TrackedBase> otherOwner(new TrackedBase(&otherState, 19));
        TrackedBase *ownerRaw = owner.data();

        observed = ownerRaw;
        KisWeakSharedPtr<TrackedBase> copy;
        copy = observed;
        KisWeakSharedPtr<TrackedBase> other(otherOwner);

        QVERIFY(observed.isValid());
        QVERIFY(!observed.isNull());
        QCOMPARE(observed.data(), ownerRaw);
        QCOMPARE(observed.constData(), ownerRaw);
        QCOMPARE((*observed).value, 17);
        QCOMPARE(observed->value, 17);

        const KisWeakSharedPtr<TrackedBase> &constObserved = observed;
        QCOMPARE(constObserved.data(), ownerRaw);
        QCOMPARE(constObserved.constData(), ownerRaw);
        QCOMPARE((*constObserved).value, 17);
        QCOMPARE(constObserved->value, 17);

        QVERIFY(observed == ownerRaw);
        QVERIFY(observed != otherOwner.data());
        QVERIFY(observed == copy);
        QVERIFY(observed != other);
        QCOMPARE(static_cast<const TrackedBase *>(observed), ownerRaw);

        {
            KisSharedPtr<TrackedBase> promoted = observed.toStrongRef();
            QCOMPARE(promoted.data(), ownerRaw);
            QCOMPARE(ownerRaw->refCount(), 2);
        }
        QCOMPARE(ownerRaw->refCount(), 1);
    }

    QCOMPARE(observedState.destroyed, 1);
    QCOMPARE(otherState.destroyed, 1);
    QVERIFY(!observed.isValid());
    QCOMPARE(static_cast<const TrackedBase *>(observed), nullptr);
    KisSharedPtr<TrackedBase> constructedFromExpired(observed);
    QVERIFY(constructedFromExpired.isNull());
    QVERIFY(observed.toStrongRef().isNull());
}

QTEST_GUILESS_MAIN(KisSharedPtrContractTest)

#include "KisSharedPtrContractTest.moc"

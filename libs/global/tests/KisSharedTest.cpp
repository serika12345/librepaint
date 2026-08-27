/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_shared.h>

#include <QTest>

namespace
{

class TestShared : public KisShared
{
public:
    TestShared() = default;
    ~TestShared() = default;
};

} // namespace

class KisSharedTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void referenceCountTransitions();
    void weakReferenceMarkerIsStable();
};

void KisSharedTest::referenceCountTransitions()
{
    TestShared shared;
    QCOMPARE(shared.refCount(), 0);

    QVERIFY(shared.ref());
    QCOMPARE(shared.refCount(), 1);

    QVERIFY(shared.ref());
    QCOMPARE(shared.refCount(), 2);

    QVERIFY(shared.deref());
    QCOMPARE(shared.refCount(), 1);

    QVERIFY(!shared.deref());
    QCOMPARE(shared.refCount(), 0);
}

void KisSharedTest::weakReferenceMarkerIsStable()
{
    TestShared shared;

    QAtomicInt *marker = shared.sharedWeakReference();

    QVERIFY(marker);
    QCOMPARE(marker->loadRelaxed(), 1);
    QCOMPARE(shared.sharedWeakReference(), marker);
    QCOMPARE(marker->loadRelaxed(), 1);
}

QTEST_GUILESS_MAIN(KisSharedTest)

#include "KisSharedTest.moc"

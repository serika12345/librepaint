/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KisResourceItemChooserSync.h>

#include <QPointer>
#include <QSignalSpy>
#include <QTest>

class KisResourceItemChooserSyncContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsAtDefaultBaseLength();
    void clampsAndEmitsEffectiveBaseLength();
    void instanceReturnsStableSharedObject();
    void destructionInvalidatesGuardedPointer();
};

void KisResourceItemChooserSyncContractTest::startsAtDefaultBaseLength()
{
    KisResourceItemChooserSync sync;

    QCOMPARE(sync.baseLength(), 50);
}

void KisResourceItemChooserSyncContractTest::clampsAndEmitsEffectiveBaseLength()
{
    KisResourceItemChooserSync sync;
    QSignalSpy changedSpy(&sync, &KisResourceItemChooserSync::baseLengthChanged);
    const QList<int> requestedLengths {10, 70, 120, 120};
    const QList<int> effectiveLengths {25, 70, 100, 100};

    for (int i = 0; i < requestedLengths.size(); ++i) {
        sync.setBaseLength(requestedLengths.at(i));

        QCOMPARE(sync.baseLength(), effectiveLengths.at(i));
        QCOMPARE(changedSpy.size(), i + 1);
        QCOMPARE(changedSpy.at(i).at(0).toInt(), effectiveLengths.at(i));
    }
}

void KisResourceItemChooserSyncContractTest::instanceReturnsStableSharedObject()
{
    KisResourceItemChooserSync *first = KisResourceItemChooserSync::instance();

    QVERIFY(first);
    QCOMPARE(KisResourceItemChooserSync::instance(), first);
}

void KisResourceItemChooserSyncContractTest::destructionInvalidatesGuardedPointer()
{
    QPointer<KisResourceItemChooserSync> sync = new KisResourceItemChooserSync;

    QVERIFY(sync);
    delete sync.data();
    QVERIFY(sync.isNull());
}

QTEST_GUILESS_MAIN(KisResourceItemChooserSyncContractTest)

#include "KisResourceItemChooserSyncContractTest.moc"

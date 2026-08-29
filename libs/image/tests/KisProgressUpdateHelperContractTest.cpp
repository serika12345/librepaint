/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_progress_update_helper.h"

#include <KoUpdater.h>

#include <QSignalSpy>
#include <QTest>

class KisProgressUpdateHelperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void reportsDistinctStepsAndFinalEndpoint();
    void acceptsNullUpdater();
};

void KisProgressUpdateHelperContractTest::reportsDistinctStepsAndFinalEndpoint()
{
    KoDummyUpdaterHolder updaterHolder;
    KoUpdaterPtr updater = updaterHolder.updater();
    updater->setProgress(37);
    QSignalSpy progressSpy(updater, &KoUpdater::sigProgress);

    {
        KisProgressUpdateHelper helper(updater, 5, 8);
        for (int step = 0; step < 5; ++step) {
            helper.step();
        }

        QCOMPARE(progressSpy.count(), 3);
        QCOMPARE(progressSpy.at(0).at(0).toInt(), 38);
        QCOMPARE(progressSpy.at(1).at(0).toInt(), 39);
        QCOMPARE(progressSpy.at(2).at(0).toInt(), 40);
    }

    QCOMPARE(progressSpy.count(), 4);
    QCOMPARE(progressSpy.at(3).at(0).toInt(), 42);
    QCOMPARE(updater->progress(), 42);
}

void KisProgressUpdateHelperContractTest::acceptsNullUpdater()
{
    KisProgressUpdateHelper helper(KoUpdaterPtr(), 10, 3);

    helper.step();
    helper.step();
}

QTEST_GUILESS_MAIN(KisProgressUpdateHelperContractTest)

#include "KisProgressUpdateHelperContractTest.moc"

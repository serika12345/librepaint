/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisCumulativeUndoData.h>

#include <KConfig>
#include <KConfigGroup>
#include <QDebug>
#include <QTemporaryDir>
#include <QTest>

class KisCumulativeUndoDataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void defaultsAndEquality();
    void configurationRoundTrip();
    void debugOutput();
};

void KisCumulativeUndoDataTest::defaultsAndEquality()
{
    QCOMPARE(KisCumulativeUndoData::defaultValue.excludeFromMerge, 10);
    QCOMPARE(KisCumulativeUndoData::defaultValue.mergeTimeout, 5000);
    QCOMPARE(KisCumulativeUndoData::defaultValue.maxGroupSeparation, 1000);
    QCOMPARE(KisCumulativeUndoData::defaultValue.maxGroupDuration, 5000);

    KisCumulativeUndoData equalToDefault;
    QVERIFY(equalToDefault == KisCumulativeUndoData::defaultValue);

    equalToDefault.maxGroupDuration = 5001;
    QVERIFY(!(equalToDefault == KisCumulativeUndoData::defaultValue));
}

void KisCumulativeUndoDataTest::configurationRoundTrip()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    KConfig config(directory.filePath("configuration"), KConfig::SimpleConfig);
    KConfigGroup group(&config, QString());

    KisCumulativeUndoData written;
    written.excludeFromMerge = 17;
    written.mergeTimeout = 2300;
    written.maxGroupSeparation = 410;
    written.maxGroupDuration = 7200;
    written.write(&group);

    QCOMPARE(group.readEntry("cumulativeUndoExcludeFromMerge", -1), 17);
    QCOMPARE(group.readEntry("cumulativeUndoMergeTimeout", -1), 2300);
    QCOMPARE(group.readEntry("cumulativeUndoMaxGroupSeparation", -1), 410);
    QCOMPARE(group.readEntry("cumulativeUndoMaxGroupDuration", -1), 7200);

    KisCumulativeUndoData read;
    QVERIFY(read.read(&group));
    QVERIFY(read == written);
}

void KisCumulativeUndoDataTest::debugOutput()
{
    KisCumulativeUndoData data;
    data.excludeFromMerge = 17;
    data.mergeTimeout = 2300;
    data.maxGroupSeparation = 410;
    data.maxGroupDuration = 7200;

    QString diagnostic;
    {
        QDebug debug(&diagnostic);
        debug << data;
    }

    QVERIFY(diagnostic.contains("KisCumulativeUndoData("));
    QVERIFY(diagnostic.contains("data.excludeFromMerge = 17"));
    QVERIFY(diagnostic.contains("data.mergeTimeout = 2300"));
    QVERIFY(diagnostic.contains("data.maxGroupSeparation = 410"));
    QVERIFY(diagnostic.contains("data.maxGroupDuration = 7200"));
}

QTEST_GUILESS_MAIN(KisCumulativeUndoDataTest)

#include "KisCumulativeUndoDataTest.moc"

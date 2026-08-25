/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KConfig>
#include <KConfigGroup>
#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <KisCumulativeUndoData.h>
#include <KisTemporaryFileConfiguration.h>

class KisConfigurationValueTypesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cumulativeUndoDefaultsAndKeysRemainStable();
    void configuredTemporaryLocationRemainsStable();
    void defaultAndFallbackTemporaryLocationsRemainStable();
};

void KisConfigurationValueTypesTest::cumulativeUndoDefaultsAndKeysRemainStable()
{
    QCOMPARE(KisCumulativeUndoData::defaultValue.excludeFromMerge, 10);
    QCOMPARE(KisCumulativeUndoData::defaultValue.mergeTimeout, 5000);
    QCOMPARE(KisCumulativeUndoData::defaultValue.maxGroupSeparation, 1000);
    QCOMPARE(KisCumulativeUndoData::defaultValue.maxGroupDuration, 5000);

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
    QCOMPARE(read, written);
}

void KisConfigurationValueTypesTest::configuredTemporaryLocationRemainsStable()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    KConfig config(directory.filePath("configuration"), KConfig::SimpleConfig);
    KConfigGroup group(&config, QString());
    group.writeEntry("swaplocation", directory.path());
    group.writeEntry("animationCacheDir", directory.path());

    QCOMPARE(
        KritaUtils::writableSwapFileLocation(group, false),
        directory.path());
    QCOMPARE(
        KritaUtils::writableTemporaryFileLocation(
            group, "animation_cache", "animationCacheDir", false),
        directory.path());
}

void KisConfigurationValueTypesTest::defaultAndFallbackTemporaryLocationsRemainStable()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    KConfig config(directory.filePath("configuration"), KConfig::SimpleConfig);
    KConfigGroup group(&config, QString());
    group.writeEntry("swaplocation", directory.path());

    const QString defaultLocation = KritaUtils::writableSwapFileLocation(group, true);
    QVERIFY(!defaultLocation.isEmpty());
    QVERIFY(defaultLocation != directory.path());

    const QString missingLocation = directory.filePath("missing");
    group.writeEntry("swaplocation", missingLocation);
    QCOMPARE(KritaUtils::writableSwapFileLocation(group, false), QDir::tempPath());
}

QTEST_MAIN(KisConfigurationValueTypesTest)

#include "KisConfigurationValueTypesTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KConfig>
#include <KConfigGroup>
#include <QDir>
#include <QTemporaryDir>
#include <QTest>

#include <KisTemporaryFileConfiguration.h>

class KisConfigurationValueTypesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void configuredTemporaryLocationRemainsStable();
    void defaultAndFallbackTemporaryLocationsRemainStable();
};

void KisConfigurationValueTypesTest::configuredTemporaryLocationRemainsStable()
{
    QTemporaryDir directory(QDir::home().filePath(".KisConfigurationValueTypesTest-XXXXXX"));
    QVERIFY(directory.isValid());
    KConfig config(directory.filePath("configuration"), KConfig::SimpleConfig);
    KConfigGroup group(&config, QString());
    group.writeEntry("swaplocation", directory.path());
    group.writeEntry("animationCacheDir", directory.path());

    QCOMPARE(KritaUtils::writableSwapFileLocation(group, false), directory.path());
    QCOMPARE(KritaUtils::writableTemporaryFileLocation(group, "animation_cache", "animationCacheDir", false),
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

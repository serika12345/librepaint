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

class KisTemporaryFileConfigurationTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void configuredLocationRemainsStable();
    void defaultAndFallbackLocationsRemainStable();
    void temporarySwapLocationNormalization();
};

void KisTemporaryFileConfigurationTest::configuredLocationRemainsStable()
{
    QTemporaryDir directory(QDir::home().filePath(".KisTemporaryFileConfigurationTest-XXXXXX"));
    QVERIFY(directory.isValid());
    KConfig config(directory.filePath("configuration"), KConfig::SimpleConfig);
    KConfigGroup group(&config, QString());
    group.writeEntry("swaplocation", directory.path());
    group.writeEntry("animationCacheDir", directory.path());

    QCOMPARE(KritaUtils::writableSwapFileLocation(group, false), directory.path());
    QCOMPARE(KritaUtils::writableTemporaryFileLocation(group, "animation_cache", "animationCacheDir", false),
             directory.path());
}

void KisTemporaryFileConfigurationTest::defaultAndFallbackLocationsRemainStable()
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

void KisTemporaryFileConfigurationTest::temporarySwapLocationNormalization()
{
    QTemporaryDir directory;
    QVERIFY(directory.isValid());
    KConfig config(directory.filePath("configuration"), KConfig::SimpleConfig);
    KConfigGroup group(&config, QString());
    const QString temporaryLocation = QStringLiteral("/var/folders/librepaint-contract");
    group.writeEntry("swaplocation", temporaryLocation);

    KritaUtils::normalizeSwapFileLocation(group);

#ifdef Q_OS_MACOS
    QVERIFY(!group.hasKey("swaplocation"));
#else
    QCOMPARE(group.readEntry("swaplocation", QString()), temporaryLocation);
#endif
}

QTEST_GUILESS_MAIN(KisTemporaryFileConfigurationTest)

#include "KisTemporaryFileConfigurationTest.moc"

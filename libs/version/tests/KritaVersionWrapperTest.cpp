/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KritaVersionWrapper.h>
#include <kritagitversion.h>
#include <kritaversion.h>

class KritaVersionWrapperTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void versionStringUsesConfiguredMetadata();
    void developerBuildClassification();
};

void KritaVersionWrapperTest::versionStringUsesConfiguredMetadata()
{
    const QString configuredVersion = QStringLiteral(KRITA_VERSION_STRING);

    QCOMPARE(KritaVersionWrapper::versionString(), configuredVersion);
    QCOMPARE(KritaVersionWrapper::versionString(false), configuredVersion);
#ifdef KRITA_GIT_SHA1_STRING
    QCOMPARE(KritaVersionWrapper::versionString(true),
             QStringLiteral("%1 (git %2)").arg(configuredVersion, QStringLiteral(KRITA_GIT_SHA1_STRING)));
#else
    QCOMPARE(KritaVersionWrapper::versionString(true), configuredVersion);
#endif
}

void KritaVersionWrapperTest::developerBuildClassification()
{
    QVERIFY(KritaVersionWrapper::isDevelopersBuild());
}

QTEST_GUILESS_MAIN(KritaVersionWrapperTest)

#include "KritaVersionWrapperTest.moc"

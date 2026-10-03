/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoResourcePaths.h"

#include <QDir>
#include <QStandardPaths>
#include <QTest>

class KoResourcePathsAndroidContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void packagedAssetsResolveFromPrivateAppData();
};

void KoResourcePathsAndroidContractTest::packagedAssetsResolveFromPrivateAppData()
{
    // Consumer: Android startup resource and ICC profile discovery.
    // Operation: Resolve the installation root after Java extracts packaged assets.
    // Observable result: C++ searches the same private files directory used by ConfigsManager.
    // Failure impact: Packaged profiles are absent from the color conversion graph and startup crashes.
    const QString privateAppData =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    QVERIFY(!privateAppData.isEmpty());
    QCOMPARE(QDir::cleanPath(KoResourcePaths::getApplicationRoot()),
             QDir::cleanPath(privateAppData));
}

QTEST_GUILESS_MAIN(KoResourcePathsAndroidContractTest)

#include "KoResourcePathsAndroidContractTest.moc"

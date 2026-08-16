/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoStore.h>
#include <kritastore_export.h>

#include <QTest>

class TestLegacyStoreCompatibility : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void legacyTargetAndHeadersExposeStorageApi();
};

void TestLegacyStoreCompatibility::legacyTargetAndHeadersExposeStorageApi()
{
    QCOMPARE(int(KoStore::Read), 0);
    QCOMPARE(int(KoStore::Write), 1);
    QCOMPARE(int(KoStore::Auto), 0);
    QCOMPARE(int(KoStore::Zip), 1);
    QCOMPARE(int(KoStore::Directory), 2);
}

QTEST_GUILESS_MAIN(TestLegacyStoreCompatibility)

#include "TestLegacyStoreCompatibility.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisRemoteFileFetcher.h>

#include <QTest>

#include <type_traits>

class KisRemoteFileFetcherSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void virtualDestructorSchemaRemainsStable();
};

void KisRemoteFileFetcherSchemaContractTest::virtualDestructorSchemaRemainsStable()
{
    static_assert(std::is_base_of_v<QObject, KisRemoteFileFetcher>);
    static_assert(std::has_virtual_destructor_v<KisRemoteFileFetcher>);
}

QTEST_GUILESS_MAIN(KisRemoteFileFetcherSchemaContractTest)

#include "KisRemoteFileFetcherSchemaContractTest.moc"

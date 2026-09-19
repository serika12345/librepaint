/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KoStore.h"

#include <QTest>

#include <utility>

class KoStoreSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void storeTypeAndLifetimeSchemaRemainStable();
};

void KoStoreSchemaContractTest::storeTypeAndLifetimeSchemaRemainStable()
{
    using Store = KoStore;


    QCOMPARE(static_cast<int>(Store::Read), 0);
    QCOMPARE(static_cast<int>(Store::Write), 1);
    QCOMPARE(static_cast<int>(Store::Auto), 0);
    QCOMPARE(static_cast<int>(Store::Zip), 1);
    QCOMPARE(static_cast<int>(Store::Directory), 2);
}

QTEST_APPLESS_MAIN(KoStoreSchemaContractTest)

#include "KoStoreSchemaContractTest.moc"

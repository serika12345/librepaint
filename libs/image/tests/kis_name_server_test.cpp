/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_name_server_test.h"

#include "kis_name_server.h"

#include <QTest>

void KisNameServerTest::defaultSequenceBeginsAtOne()
{
    KisNameServer server;

    QCOMPARE(server.currentSeed(), 1);
    QCOMPARE(server.number(), 1);
    QCOMPARE(server.currentSeed(), 2);
}

void KisNameServerTest::customSeedAdvancesAndRollsBack()
{
    KisNameServer server(41);

    QCOMPARE(server.currentSeed(), 41);
    QCOMPARE(server.number(), 41);
    QCOMPARE(server.number(), 42);
    QCOMPARE(server.currentSeed(), 43);

    server.rollback();

    QCOMPARE(server.currentSeed(), 42);
    QCOMPARE(server.number(), 42);
    QCOMPARE(server.currentSeed(), 43);
}

QTEST_GUILESS_MAIN(KisNameServerTest)

/*
 *  SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisPredictionTestSwitches.h"

#include <QTest>

// The colour switch tells the prediction line apart in captures. Unset, empty
// or invalid values must keep the normal drawing, and the accessor must keep
// the first value of the process (KisPredictionTestSwitches.h).
class KisPredictionTestSwitchesContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void init()
    {
        qunsetenv("LIBREPAINT_PREDICTION_TEST_COLOR");
    }

    void testColorRgbKeepsTheFirstValueForTheProcess()
    {
        // This slot runs before the others. The variable is unset here, so
        // the first read caches "not set" for the whole process.
        QCOMPARE(KisPredictionTestSwitches::testColorRgb(), -1);

        qputenv("LIBREPAINT_PREDICTION_TEST_COLOR", "FF0000");
        QCOMPARE(KisPredictionTestSwitches::testColorRgb(), -1);
    }

    void parseTestColorAcceptsSixHexadecimalDigits()
    {
        QCOMPARE(KisPredictionTestSwitches::Detail::parseTestColor("FF0000"), 0xFF0000);
        QCOMPARE(KisPredictionTestSwitches::Detail::parseTestColor("ff0000"), 0xFF0000);
        QCOMPARE(KisPredictionTestSwitches::Detail::parseTestColor("00FF7f"), 0x00FF7F);
        QCOMPARE(KisPredictionTestSwitches::Detail::parseTestColor("000000"), 0);
        QCOMPARE(KisPredictionTestSwitches::Detail::parseTestColor("FFFFFF"), 0xFFFFFF);

        for (const char *value : {"", "#FF0000", "FF00", "FF000000", "GG0000", " FF0000"}) {
            QCOMPARE(KisPredictionTestSwitches::Detail::parseTestColor(value), -1);
        }
    }

    void readTestColorFollowsTheEnvironment()
    {
        QCOMPARE(KisPredictionTestSwitches::Detail::readTestColor(), -1);

        qputenv("LIBREPAINT_PREDICTION_TEST_COLOR", "00FF7f");
        QCOMPARE(KisPredictionTestSwitches::Detail::readTestColor(), 0x00FF7F);

        qputenv("LIBREPAINT_PREDICTION_TEST_COLOR", "FF0000");
        QCOMPARE(KisPredictionTestSwitches::Detail::readTestColor(), 0xFF0000);

        for (const char *value : {"", "notacolor", "#FF0000", "FF00"}) {
            qputenv("LIBREPAINT_PREDICTION_TEST_COLOR", value);
            QCOMPARE(KisPredictionTestSwitches::Detail::readTestColor(), -1);
        }

        qunsetenv("LIBREPAINT_PREDICTION_TEST_COLOR");
        QCOMPARE(KisPredictionTestSwitches::Detail::readTestColor(), -1);
    }
};

QTEST_GUILESS_MAIN(KisPredictionTestSwitchesContractTest)

#include "KisPredictionTestSwitchesContractTest.moc"

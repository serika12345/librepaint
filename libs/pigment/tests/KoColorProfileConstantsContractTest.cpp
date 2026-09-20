/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoColorProfileConstants.h"

#include <QTest>

class KoColorProfileConstantsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorPrimariesPreserveStandardValues_data();
    void colorPrimariesPreserveStandardValues();
    void transferCharacteristicsPreserveStandardValues_data();
    void transferCharacteristicsPreserveStandardValues();
};

void KoColorProfileConstantsContractTest::colorPrimariesPreserveStandardValues_data()
{
    QTest::addColumn<int>("actual");
    QTest::addColumn<int>("expected");

    QTest::newRow("PRIMARIES_ITU_R_BT_709_5") << static_cast<int>(PRIMARIES_ITU_R_BT_709_5) << 1;
    QTest::newRow("PRIMARIES_UNSPECIFIED") << static_cast<int>(PRIMARIES_UNSPECIFIED) << 2;
    QTest::newRow("PRIMARIES_ITU_R_BT_470_6_SYSTEM_M") << static_cast<int>(PRIMARIES_ITU_R_BT_470_6_SYSTEM_M) << 4;
    QTest::newRow("PRIMARIES_ITU_R_BT_470_6_SYSTEM_B_G") << static_cast<int>(PRIMARIES_ITU_R_BT_470_6_SYSTEM_B_G) << 5;
    QTest::newRow("PRIMARIES_ITU_R_BT_601_6") << static_cast<int>(PRIMARIES_ITU_R_BT_601_6) << 6;
    QTest::newRow("PRIMARIES_SMPTE_240M") << static_cast<int>(PRIMARIES_SMPTE_240M) << 7;
    QTest::newRow("PRIMARIES_GENERIC_FILM") << static_cast<int>(PRIMARIES_GENERIC_FILM) << 8;
    QTest::newRow("PRIMARIES_ITU_R_BT_2020_2_AND_2100_0") << static_cast<int>(PRIMARIES_ITU_R_BT_2020_2_AND_2100_0) << 9;
    QTest::newRow("PRIMARIES_SMPTE_ST_428_1") << static_cast<int>(PRIMARIES_SMPTE_ST_428_1) << 10;
    QTest::newRow("PRIMARIES_SMPTE_RP_431_2") << static_cast<int>(PRIMARIES_SMPTE_RP_431_2) << 11;
    QTest::newRow("PRIMARIES_SMPTE_EG_432_1") << static_cast<int>(PRIMARIES_SMPTE_EG_432_1) << 12;
    QTest::newRow("PRIMARIES_EBU_Tech_3213_E") << static_cast<int>(PRIMARIES_EBU_Tech_3213_E) << 22;
    QTest::newRow("PRIMARIES_ADOBE_RGB_1998") << static_cast<int>(PRIMARIES_ADOBE_RGB_1998) << 256;
    QTest::newRow("PRIMARIES_PROPHOTO") << static_cast<int>(PRIMARIES_PROPHOTO) << 257;
}

void KoColorProfileConstantsContractTest::colorPrimariesPreserveStandardValues()
{
    QFETCH(int, actual);
    QFETCH(int, expected);

    QCOMPARE(actual, expected);
}

void KoColorProfileConstantsContractTest::transferCharacteristicsPreserveStandardValues_data()
{
    QTest::addColumn<int>("actual");
    QTest::addColumn<int>("expected");

    QTest::newRow("TRC_ITU_R_BT_709_5") << static_cast<int>(TRC_ITU_R_BT_709_5) << 1;
    QTest::newRow("TRC_UNSPECIFIED") << static_cast<int>(TRC_UNSPECIFIED) << 2;
    QTest::newRow("TRC_ITU_R_BT_470_6_SYSTEM_M") << static_cast<int>(TRC_ITU_R_BT_470_6_SYSTEM_M) << 4;
    QTest::newRow("TRC_ITU_R_BT_470_6_SYSTEM_B_G") << static_cast<int>(TRC_ITU_R_BT_470_6_SYSTEM_B_G) << 5;
    QTest::newRow("TRC_ITU_R_BT_601_6") << static_cast<int>(TRC_ITU_R_BT_601_6) << 6;
    QTest::newRow("TRC_SMPTE_240M") << static_cast<int>(TRC_SMPTE_240M) << 7;
    QTest::newRow("TRC_LINEAR") << static_cast<int>(TRC_LINEAR) << 8;
    QTest::newRow("TRC_LOGARITHMIC_100") << static_cast<int>(TRC_LOGARITHMIC_100) << 9;
    QTest::newRow("TRC_LOGARITHMIC_100_sqrt10") << static_cast<int>(TRC_LOGARITHMIC_100_sqrt10) << 10;
    QTest::newRow("TRC_IEC_61966_2_4") << static_cast<int>(TRC_IEC_61966_2_4) << 11;
    QTest::newRow("TRC_ITU_R_BT_1361") << static_cast<int>(TRC_ITU_R_BT_1361) << 12;
    QTest::newRow("TRC_IEC_61966_2_1") << static_cast<int>(TRC_IEC_61966_2_1) << 13;
    QTest::newRow("TRC_ITU_R_BT_2020_2_10bit") << static_cast<int>(TRC_ITU_R_BT_2020_2_10bit) << 14;
    QTest::newRow("TRC_ITU_R_BT_2020_2_12bit") << static_cast<int>(TRC_ITU_R_BT_2020_2_12bit) << 15;
    QTest::newRow("TRC_ITU_R_BT_2100_0_PQ") << static_cast<int>(TRC_ITU_R_BT_2100_0_PQ) << 16;
    QTest::newRow("TRC_SMPTE_ST_428_1") << static_cast<int>(TRC_SMPTE_ST_428_1) << 17;
    QTest::newRow("TRC_ITU_R_BT_2100_0_HLG") << static_cast<int>(TRC_ITU_R_BT_2100_0_HLG) << 18;
    QTest::newRow("TRC_GAMMA_1_8") << static_cast<int>(TRC_GAMMA_1_8) << 256;
    QTest::newRow("TRC_GAMMA_2_4") << static_cast<int>(TRC_GAMMA_2_4) << 257;
    QTest::newRow("TRC_PROPHOTO") << static_cast<int>(TRC_PROPHOTO) << 258;
    QTest::newRow("TRC_A98") << static_cast<int>(TRC_A98) << 259;
    QTest::newRow("TRC_LAB_L") << static_cast<int>(TRC_LAB_L) << 260;
}

void KoColorProfileConstantsContractTest::transferCharacteristicsPreserveStandardValues()
{
    QFETCH(int, actual);
    QFETCH(int, expected);

    QCOMPARE(actual, expected);
}

QTEST_GUILESS_MAIN(KoColorProfileConstantsContractTest)

#include "KoColorProfileConstantsContractTest.moc"

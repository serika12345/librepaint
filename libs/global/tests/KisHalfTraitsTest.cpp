/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisHalfTraits.h>

#include <QTest>

#include <limits>

class KisHalfTraitsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void classifiesHalfValues();
};

void KisHalfTraitsTest::classifiesHalfValues()
{
    const half normal(1.5f);
    QVERIFY(std::isfinite(normal));
    QVERIFY(!std::isinf(normal));
    QVERIFY(!std::isnan(normal));
    QVERIFY(std::isnormal(normal));
    QVERIFY(!std::signbit(normal));

    const half zero(0.0f);
    QVERIFY(std::isfinite(zero));
    QVERIFY(!std::isnormal(zero));

    const half negative(-1.0f);
    QVERIFY(std::signbit(negative));

    const half infinity = std::numeric_limits<half>::infinity();
    QVERIFY(!std::isfinite(infinity));
    QVERIFY(std::isinf(infinity));

    const half nan = std::numeric_limits<half>::quiet_NaN();
    QVERIFY(!std::isfinite(nan));
    QVERIFY(std::isnan(nan));
}

QTEST_GUILESS_MAIN(KisHalfTraitsTest)

#include "KisHalfTraitsTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoCmykColorSpaceMaths.h"

#include <QTest>

class KoCmykColorSpaceMathsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void integerTraitsUseNativeRange();
    void floatingTraitsUseCmykPercentageRange();
};

void KoCmykColorSpaceMathsContractTest::integerTraitsUseNativeRange()
{
    QCOMPARE(KoCmykColorSpaceMathsTraits<quint8>::zeroValue, quint8(0));
    QCOMPARE(KoCmykColorSpaceMathsTraits<quint8>::unitValue, quint8(255));
}

void KoCmykColorSpaceMathsContractTest::floatingTraitsUseCmykPercentageRange()
{
    QCOMPARE(KoCmykColorSpaceMathsTraits<float>::zeroValueCMYK, 0.0f);
    QCOMPARE(KoCmykColorSpaceMathsTraits<float>::halfValueCMYK, 50.0f);
    QCOMPARE(KoCmykColorSpaceMathsTraits<float>::unitValueCMYK, 100.0f);

    QCOMPARE(KoCmykColorSpaceMathsTraits<double>::zeroValueCMYK, 0.0);
    QCOMPARE(KoCmykColorSpaceMathsTraits<double>::halfValueCMYK, 50.0);
    QCOMPARE(KoCmykColorSpaceMathsTraits<double>::unitValueCMYK, 100.0);
}

QTEST_GUILESS_MAIN(KoCmykColorSpaceMathsContractTest)

#include "KoCmykColorSpaceMathsContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KoLabColorSpaceMaths.h"

#include <QTest>

#include <type_traits>

class KoLabColorSpaceMathsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void integerTraitsUseEncodedLabRanges();
    void floatingTraitsUseLabCoordinateRanges();
};

void KoLabColorSpaceMathsContractTest::integerTraitsUseEncodedLabRanges()
{
    static_assert(std::is_base_of_v<KoColorSpaceMathsTraits<quint8>,
                                    KoLabColorSpaceMathsTraits<quint8>>);
    static_assert(std::is_base_of_v<KoColorSpaceMathsTraits<quint16>,
                                    KoLabColorSpaceMathsTraits<quint16>>);
    static_assert(std::is_base_of_v<KoColorSpaceMathsTraits<qint16>,
                                    KoLabColorSpaceMathsTraits<qint16>>);
    static_assert(std::is_base_of_v<KoColorSpaceMathsTraits<quint32>,
                                    KoLabColorSpaceMathsTraits<quint32>>);

    QCOMPARE(quint8(KoLabColorSpaceMathsTraits<quint8>::zeroValueL), quint8(0));
    QCOMPARE(quint8(KoLabColorSpaceMathsTraits<quint8>::halfValueL), quint8(127));
    QCOMPARE(quint8(KoLabColorSpaceMathsTraits<quint8>::unitValueL), quint8(255));
    QCOMPARE(quint8(KoLabColorSpaceMathsTraits<quint8>::zeroValueAB), quint8(0));
    QCOMPARE(quint8(KoLabColorSpaceMathsTraits<quint8>::halfValueAB), quint8(128));
    QCOMPARE(quint8(KoLabColorSpaceMathsTraits<quint8>::unitValueAB), quint8(255));

    QCOMPARE(qint16(KoLabColorSpaceMathsTraits<qint16>::halfValueAB), qint16(19549));
    QCOMPARE(quint16(KoLabColorSpaceMathsTraits<quint16>::halfValueAB), quint16(0x8080));
    QCOMPARE(quint32(KoLabColorSpaceMathsTraits<quint32>::halfValueAB), quint32(0x80808080));
}

void KoLabColorSpaceMathsContractTest::floatingTraitsUseLabCoordinateRanges()
{
    QCOMPARE(KoLabColorSpaceMathsTraits<float>::zeroValueL, 0.0f);
    QCOMPARE(KoLabColorSpaceMathsTraits<float>::halfValueL, 50.0f);
    QCOMPARE(KoLabColorSpaceMathsTraits<float>::unitValueL, 100.0f);
    QCOMPARE(KoLabColorSpaceMathsTraits<float>::zeroValueAB, -128.0f);
    QCOMPARE(KoLabColorSpaceMathsTraits<float>::halfValueAB, 0.0f);
    QCOMPARE(KoLabColorSpaceMathsTraits<float>::unitValueAB, 127.0f);

    QCOMPARE(KoLabColorSpaceMathsTraits<double>::zeroValueL, 0.0);
    QCOMPARE(KoLabColorSpaceMathsTraits<double>::halfValueL, 50.0);
    QCOMPARE(KoLabColorSpaceMathsTraits<double>::unitValueL, 100.0);
    QCOMPARE(KoLabColorSpaceMathsTraits<double>::zeroValueAB, -128.0);
    QCOMPARE(KoLabColorSpaceMathsTraits<double>::halfValueAB, 0.0);
    QCOMPARE(KoLabColorSpaceMathsTraits<double>::unitValueAB, 127.0);
}

QTEST_GUILESS_MAIN(KoLabColorSpaceMathsContractTest)

#include "KoLabColorSpaceMathsContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisSurfaceColorSpaceWrapperTest.h"

#include <color/KisSurfaceColorSpaceWrapper.h>
#include <simpletest.h>

void KisSurfaceColorSpaceWrapperTest::testValueSemantics()
{
    KisSurfaceColorSpaceWrapper defaultConstructed;
    KisSurfaceColorSpaceWrapper defaultValue(
        KisSurfaceColorSpaceWrapper::DefaultColorSpace);
    KisSurfaceColorSpaceWrapper srgb(
        KisSurfaceColorSpaceWrapper::sRGBColorSpace);
    KisSurfaceColorSpaceWrapper linear(
        KisSurfaceColorSpaceWrapper::scRGBColorSpace);

    QVERIFY(defaultConstructed == defaultValue);
    QVERIFY(defaultConstructed != srgb);
    QVERIFY(srgb == KisSurfaceColorSpaceWrapper::sRGBColorSpace);
    QVERIFY(srgb != KisSurfaceColorSpaceWrapper::scRGBColorSpace);

    defaultConstructed = std::move(linear);
    QVERIFY(defaultConstructed ==
            KisSurfaceColorSpaceWrapper::scRGBColorSpace);
}

void KisSurfaceColorSpaceWrapperTest::testQtColorSpaceConversion()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto defaultValue = KisSurfaceColorSpaceWrapper::fromQtColorSpace(
        QSurfaceFormat::DefaultColorSpace);
    const auto srgb = KisSurfaceColorSpaceWrapper::fromQtColorSpace(
        QSurfaceFormat::sRGBColorSpace);
    const auto linear = KisSurfaceColorSpaceWrapper::fromQtColorSpace(
        QSurfaceFormat::scRGBColorSpace);
    const auto pq = KisSurfaceColorSpaceWrapper::fromQtColorSpace(
        QSurfaceFormat::bt2020PQColorSpace);

    QVERIFY(defaultValue == QSurfaceFormat::DefaultColorSpace);
    QVERIFY(srgb == QSurfaceFormat::sRGBColorSpace);
    QVERIFY(linear == QSurfaceFormat::scRGBColorSpace);
    QVERIFY(pq == QSurfaceFormat::bt2020PQColorSpace);
#else
    const auto defaultValue = KisSurfaceColorSpaceWrapper::fromQtColorSpace(
        QColorSpace());
    const auto srgb = KisSurfaceColorSpaceWrapper::fromQtColorSpace(
        QColorSpace::SRgb);
    const auto linear = KisSurfaceColorSpaceWrapper::fromQtColorSpace(
        QColorSpace::SRgbLinear);
    const auto pq = KisSurfaceColorSpaceWrapper::fromQtColorSpace(
        QColorSpace::Bt2100Pq);

    QVERIFY(defaultValue == QColorSpace());
    QVERIFY(srgb == QColorSpace::SRgb);
    QVERIFY(linear == QColorSpace::SRgbLinear);
    QVERIFY(pq == QColorSpace::Bt2100Pq);
#endif
}

SIMPLE_TEST_MAIN(KisSurfaceColorSpaceWrapperTest)

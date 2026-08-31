/*
 * SPDX-FileCopyrightText: 2016 Dmitry Kazakov <dimula73@gmail.com>
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisSurfaceColorSpaceWrapperTest.h"

#include <QTest>
#include <color/KisSurfaceColorSpaceWrapper.h>

void KisSurfaceColorSpaceWrapperTest::enumValuesDefaultsAndFactories()
{
    using Wrapper = KisSurfaceColorSpaceWrapper;

    QCOMPARE(int(Wrapper::DefaultColorSpace), 0);
    QCOMPARE(int(Wrapper::sRGBColorSpace), 1);
    QCOMPARE(int(Wrapper::scRGBColorSpace), 2);
    QCOMPARE(int(Wrapper::bt2020PQColorSpace), 3);

    const Wrapper defaultConstructed;
    const Wrapper defaultValue(Wrapper::DefaultColorSpace);
    const Wrapper srgb = Wrapper::makeSRGBColorSpace();
    const Wrapper linear = Wrapper::makeSCRGBColorSpace();
    const Wrapper pq = Wrapper::makeBt2020PQColorSpace();

    QVERIFY(defaultConstructed == defaultValue);
    QVERIFY(defaultConstructed != srgb);
    QVERIFY(srgb == Wrapper::sRGBColorSpace);
    QVERIFY(srgb != Wrapper::scRGBColorSpace);
    QVERIFY(linear == Wrapper::scRGBColorSpace);
    QVERIFY(pq == Wrapper::bt2020PQColorSpace);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QVERIFY(Wrapper::sRGBColorSpace == srgb);
    QVERIFY(Wrapper::scRGBColorSpace != srgb);
#endif
}

void KisSurfaceColorSpaceWrapperTest::copyAndMovePreserveIndependentValues()
{
    using Wrapper = KisSurfaceColorSpaceWrapper;

    Wrapper source = Wrapper::makeSRGBColorSpace();
    Wrapper copied(source);
    Wrapper moved(std::move(source));
    QCOMPARE(copied, Wrapper::makeSRGBColorSpace());
    QCOMPARE(moved, Wrapper::makeSRGBColorSpace());

    source = Wrapper::makeSCRGBColorSpace();
    copied = source;
    source = Wrapper::makeBt2020PQColorSpace();
    QCOMPARE(copied, Wrapper::makeSCRGBColorSpace());

    moved = std::move(source);
    QCOMPARE(moved, Wrapper::makeBt2020PQColorSpace());
    QCOMPARE(copied, Wrapper::makeSCRGBColorSpace());
}

void KisSurfaceColorSpaceWrapperTest::qtColorSpacesRoundTrip()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    const auto defaultValue = KisSurfaceColorSpaceWrapper::fromQtColorSpace(QSurfaceFormat::DefaultColorSpace);
    const auto srgb = KisSurfaceColorSpaceWrapper::fromQtColorSpace(QSurfaceFormat::sRGBColorSpace);
    const auto linear = KisSurfaceColorSpaceWrapper::fromQtColorSpace(QSurfaceFormat::scRGBColorSpace);
    const auto pq = KisSurfaceColorSpaceWrapper::fromQtColorSpace(QSurfaceFormat::bt2020PQColorSpace);

    QVERIFY(defaultValue == QSurfaceFormat::DefaultColorSpace);
    QVERIFY(srgb == QSurfaceFormat::sRGBColorSpace);
    QVERIFY(linear == QSurfaceFormat::scRGBColorSpace);
    QVERIFY(pq == QSurfaceFormat::bt2020PQColorSpace);
#else
    const auto defaultValue = KisSurfaceColorSpaceWrapper::fromQtColorSpace(QColorSpace());
    const auto srgb = KisSurfaceColorSpaceWrapper::fromQtColorSpace(QColorSpace::SRgb);
    const auto linear = KisSurfaceColorSpaceWrapper::fromQtColorSpace(QColorSpace::SRgbLinear);
    const auto pq = KisSurfaceColorSpaceWrapper::fromQtColorSpace(QColorSpace::Bt2100Pq);

    QVERIFY(defaultValue == QColorSpace());
    QVERIFY(srgb == QColorSpace::SRgb);
    QVERIFY(linear == QColorSpace::SRgbLinear);
    QVERIFY(pq == QColorSpace::Bt2100Pq);
#endif
}

void KisSurfaceColorSpaceWrapperTest::unsupportedQtColorSpaceFallsBackToDefault()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QSKIP("QSurfaceFormat::ColorSpace has no additional named color space");
#else
    QTest::ignoreMessage(
        QtWarningMsg,
        QRegularExpression("^WARNING: KisSurfaceColorSpaceWrapper: unsupported surface color space.*"));
    const auto unsupported = KisSurfaceColorSpaceWrapper::fromQtColorSpace(QColorSpace::DisplayP3);
    QCOMPARE(QColorSpace(unsupported), QColorSpace());
#endif
}

QTEST_GUILESS_MAIN(KisSurfaceColorSpaceWrapperTest)

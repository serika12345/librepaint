/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "TestColorConversion.h"
#include "KoColorConversions.h"

#include <QTest>

namespace
{

bool isClose(qreal actual, qreal expected, qreal tolerance = 1e-6)
{
    return qAbs(actual - expected) <= tolerance;
}

} // namespace

void TestColorConversion::testIntegerRGBHSV()
{
    int h = 0;
    int s = 0;
    int v = 0;
    rgb_to_hsv(255, 0, 0, &h, &s, &v);
    QCOMPARE(h, 0);
    QCOMPARE(s, 255);
    QCOMPARE(v, 255);

    int r = 0;
    int g = 0;
    int b = 0;
    hsv_to_rgb(h, s, v, &r, &g, &b);
    QCOMPARE(r, 255);
    QCOMPARE(g, 0);
    QCOMPARE(b, 0);
}

void TestColorConversion::testRGBHSV()
{
    float r, g, b, h, s, v;

    RGBToHSV(1, 0, 0, &h, &s, &v);
    QCOMPARE(h, 0.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(v, 1.0f);

    RGBToHSV(1, 1, 0, &h, &s, &v);
    QCOMPARE(h, 60.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(v, 1.0f);

    RGBToHSV(0, 1, 0, &h, &s, &v);
    QCOMPARE(h, 120.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(v, 1.0f);

    RGBToHSV(0, 1, 1, &h, &s, &v);
    QCOMPARE(h, 180.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(v, 1.0f);

    RGBToHSV(0, 0, 1, &h, &s, &v);
    QCOMPARE(h, 240.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(v, 1.0f);

    RGBToHSV(1, 0, 1, &h, &s, &v);
    QCOMPARE(h, 300.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(v, 1.0f);

    RGBToHSV(0, 0, 0, &h, &s, &v);
    QCOMPARE(h, -1.0f);
    QCOMPARE(s, 0.0f);
    QCOMPARE(v, 0.0f);

    RGBToHSV(1, 1, 1, &h, &s, &v);
    QCOMPARE(h, -1.0f);
    QCOMPARE(s, 0.0f);
    QCOMPARE(v, 1.0f);

    RGBToHSV(0.5, 0.25, 0.75, &h, &s, &v);
    QCOMPARE(h, 270.0f);
    QCOMPARE(s, 0.666667f);
    QCOMPARE(v, 0.75f);

    HSVToRGB(0, 1, 1, &r, &g, &b);
    QCOMPARE(r, 1.0f);
    QCOMPARE(g, 0.0f);
    QCOMPARE(b, 0.0f);

    HSVToRGB(60, 1, 1, &r, &g, &b);
    QCOMPARE(r, 1.0f);
    QCOMPARE(g, 1.0f);
    QCOMPARE(b, 0.0f);

    HSVToRGB(120, 1, 1, &r, &g, &b);
    QCOMPARE(r, 0.0f);
    QCOMPARE(g, 1.0f);
    QCOMPARE(b, 0.0f);

    HSVToRGB(180, 1, 1, &r, &g, &b);
    QCOMPARE(r, 0.0f);
    QCOMPARE(g, 1.0f);
    QCOMPARE(b, 1.0f);

    HSVToRGB(240, 1, 1, &r, &g, &b);
    QCOMPARE(r, 0.0f);
    QCOMPARE(g, 0.0f);
    QCOMPARE(b, 1.0f);

    HSVToRGB(300, 1, 1, &r, &g, &b);
    QCOMPARE(r, 1.0f);
    QCOMPARE(g, 0.0f);
    QCOMPARE(b, 1.0f);

    HSVToRGB(-1, 0, 0, &r, &g, &b);
    QCOMPARE(r, 0.0f);
    QCOMPARE(g, 0.0f);
    QCOMPARE(b, 0.0f);

    HSVToRGB(-1, 0, 1, &r, &g, &b);
    QCOMPARE(r, 1.0f);
    QCOMPARE(g, 1.0f);
    QCOMPARE(b, 1.0f);

    HSVToRGB(270, 0.666667f, 0.75f, &r, &g, &b);
    QCOMPARE(r, 0.5f);
    QCOMPARE(g, 0.25f);
    QCOMPARE(b, 0.75f);
}

void TestColorConversion::testRGBHSL()
{
    float r, g, b, h, s, l;

    RGBToHSL(1, 0, 0, &h, &s, &l);
    QCOMPARE(h, 0.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(l, 0.5f);

    RGBToHSL(1, 1, 0, &h, &s, &l);
    QCOMPARE(h, 60.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(l, 0.5f);

    RGBToHSL(0, 1, 0, &h, &s, &l);
    QCOMPARE(h, 120.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(l, 0.5f);

    RGBToHSL(0, 1, 1, &h, &s, &l);
    QCOMPARE(h, 180.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(l, 0.5f);

    RGBToHSL(0, 0, 1, &h, &s, &l);
    QCOMPARE(h, 240.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(l, 0.5f);

    RGBToHSL(1, 0, 1, &h, &s, &l);
    QCOMPARE(h, 300.0f);
    QCOMPARE(s, 1.0f);
    QCOMPARE(l, 0.5f);

    RGBToHSL(0, 0, 0, &h, &s, &l);
    QCOMPARE(h, -1.0f);
    QCOMPARE(s, 0.0f);
    QCOMPARE(l, 0.0f);

    RGBToHSL(1, 1, 1, &h, &s, &l);
    QCOMPARE(h, -1.0f);
    QCOMPARE(s, 0.0f);
    QCOMPARE(l, 1.0f);

    RGBToHSL(0.5, 0.25, 0.75, &h, &s, &l);
    QCOMPARE(h, 270.0f);
    QCOMPARE(s, 0.5f);
    QCOMPARE(l, 0.5f);

    HSLToRGB(0, 1, 0.5, &r, &g, &b);
    QCOMPARE(r, 1.0f);
    QCOMPARE(g, 0.0f);
    QCOMPARE(b, 0.0f);

    HSLToRGB(60, 1, 0.5, &r, &g, &b);
    QCOMPARE(r, 1.0f);
    QCOMPARE(g, 1.0f);
    QCOMPARE(b, 0.0f);

    HSLToRGB(120, 1, 0.5, &r, &g, &b);
    QCOMPARE(r, 0.0f);
    QCOMPARE(g, 1.0f);
    QCOMPARE(b, 0.0f);

    HSLToRGB(180, 1, 0.5, &r, &g, &b);
    QCOMPARE(r, 0.0f);
    QCOMPARE(g, 1.0f);
    QCOMPARE(b, 1.0f);

    HSLToRGB(240, 1, 0.5, &r, &g, &b);
    QCOMPARE(r, 0.0f);
    QCOMPARE(g, 0.0f);
    QCOMPARE(b, 1.0f);

    HSLToRGB(300, 1, 0.5, &r, &g, &b);
    QCOMPARE(r, 1.0f);
    QCOMPARE(g, 0.0f);
    QCOMPARE(b, 1.0f);

    HSLToRGB(-1, 0, 0, &r, &g, &b);
    QCOMPARE(r, 0.0f);
    QCOMPARE(g, 0.0f);
    QCOMPARE(b, 0.0f);

    HSLToRGB(-1, 0, 1, &r, &g, &b);
    QCOMPARE(r, 1.0f);
    QCOMPARE(g, 1.0f);
    QCOMPARE(b, 1.0f);

    HSLToRGB(270, 0.5, 0.5, &r, &g, &b);
    QCOMPARE(r, 0.5f);
    QCOMPARE(g, 0.25f);
    QCOMPARE(b, 0.75f);
}

void TestColorConversion::testFloatingHLS()
{
    float h = 0.0f;
    float l = 0.0f;
    float s = 0.0f;
    rgb_to_hls(255, 0, 0, &h, &l, &s);
    QCOMPARE(h, 0.0f);
    QCOMPARE(l, 0.5f);
    QCOMPARE(s, 1.0f);

    QCOMPARE(hue_value(0.2f, 0.8f, 420.0f), 0.8f);

    quint8 r = 0;
    quint8 g = 0;
    quint8 b = 0;
    hls_to_rgb(h, l, s, &r, &g, &b);
    QCOMPARE(r, quint8(255));
    QCOMPARE(g, quint8(0));
    QCOMPARE(b, quint8(0));
}

void TestColorConversion::testIntegerHLS()
{
    int h = 0;
    int l = 0;
    int s = 0;
    rgb_to_hls(128, 128, 128, &h, &l, &s);
    QCOMPARE(h, 0);
    QCOMPARE(l, 128);
    QCOMPARE(s, 0);

    quint8 r = 0;
    quint8 g = 0;
    quint8 b = 0;
    hls_to_rgb(h, l, s, &r, &g, &b);
    QCOMPARE(r, quint8(128));
    QCOMPARE(g, quint8(128));
    QCOMPARE(b, quint8(128));
}

void TestColorConversion::testHSI()
{
    qreal h = 0.0;
    qreal s = 0.0;
    qreal i = 0.0;
    RGBToHSI(0.2, 0.4, 0.6, &h, &s, &i);

    qreal r = 0.0;
    qreal g = 0.0;
    qreal b = 0.0;
    HSIToRGB(h, s, i, &r, &g, &b);
    QVERIFY(isClose(r, 0.2));
    QVERIFY(isClose(g, 0.4));
    QVERIFY(isClose(b, 0.6));
}

void TestColorConversion::testHSY()
{
    qreal h = 0.0;
    qreal s = 0.0;
    qreal y = 0.0;
    RGBToHSY(0.2, 0.4, 0.6, &h, &s, &y);

    qreal r = 0.0;
    qreal g = 0.0;
    qreal b = 0.0;
    HSYToRGB(h, s, y, &r, &g, &b);
    QVERIFY(isClose(r, 0.2));
    QVERIFY(isClose(g, 0.4));
    QVERIFY(isClose(b, 0.6));
}

void TestColorConversion::testHCI()
{
    qreal h = 0.0;
    qreal c = 0.0;
    qreal i = 0.0;
    RGBToHCI(1.0, 0.0, 0.0, &h, &c, &i);
    QCOMPARE(h, 0.0);
    QCOMPARE(c, 1.0);
    QVERIFY(isClose(i, 1.0 / 3.0));

    qreal r = 0.0;
    qreal g = 0.0;
    qreal b = 0.0;
    HCIToRGB(h, c, i, &r, &g, &b);
    QVERIFY(isClose(r, 1.0));
    QVERIFY(isClose(g, 0.0));
    QVERIFY(isClose(b, 0.0));
}

void TestColorConversion::testHCY()
{
    qreal h = 0.0;
    qreal c = 0.0;
    qreal y = 0.0;
    RGBToHCY(0.2, 0.4, 0.6, &h, &c, &y);

    qreal r = 0.0;
    qreal g = 0.0;
    qreal b = 0.0;
    HCYToRGB(h, c, y, &r, &g, &b);
    QVERIFY(isClose(r, 0.2));
    QVERIFY(isClose(g, 0.4));
    QVERIFY(isClose(b, 0.6));
}

void TestColorConversion::testYUV()
{
    qreal y = 0.0;
    qreal u = 0.0;
    qreal v = 0.0;
    RGBToYUV(0.2, 0.4, 0.6, &y, &u, &v);

    qreal r = 0.0;
    qreal g = 0.0;
    qreal b = 0.0;
    YUVToRGB(y, u, v, &r, &g, &b);
    QVERIFY(isClose(r, 0.2));
    QVERIFY(isClose(g, 0.4));
    QVERIFY(isClose(b, 0.6));
}

void TestColorConversion::testLabLCH()
{
    qreal L = 0.0;
    qreal C = 0.0;
    qreal H = 0.0;
    LabToLCH(0.4, 0.3, 0.6, &L, &C, &H);

    qreal l = 0.0;
    qreal a = 0.0;
    qreal b = 0.0;
    LCHToLab(L, C, H, &l, &a, &b);
    QVERIFY(isClose(l, 0.4));
    QVERIFY(isClose(a, 0.3));
    QVERIFY(isClose(b, 0.6));
}

void TestColorConversion::testXYZxyY()
{
    qreal x = 0.0;
    qreal y = 0.0;
    qreal yY = 0.0;
    XYZToxyY(0.2, 0.3, 0.5, &x, &y, &yY);

    qreal X = 0.0;
    qreal Y = 0.0;
    qreal Z = 0.0;
    xyYToXYZ(x, y, yY, &X, &Y, &Z);
    QVERIFY(isClose(X, 0.2));
    QVERIFY(isClose(Y, 0.3));
    QVERIFY(isClose(Z, 0.5));
}

void TestColorConversion::testCMYCMYK()
{
    qreal c = 0.2;
    qreal m = 0.4;
    qreal y = 0.6;
    qreal k = 0.0;
    CMYToCMYK(&c, &m, &y, &k);
    QVERIFY(isClose(c, 0.0));
    QVERIFY(isClose(m, 0.25));
    QVERIFY(isClose(y, 0.5));
    QVERIFY(isClose(k, 0.2));

    CMYKToCMY(&c, &m, &y, &k);
    QVERIFY(isClose(c, 0.2));
    QVERIFY(isClose(m, 0.4));
    QVERIFY(isClose(y, 0.6));
}

QTEST_GUILESS_MAIN(TestColorConversion)

/*
 *  SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TestColorConversion_H
#define TestColorConversion_H

#include <QObject>

class TestColorConversion : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void testIntegerRGBHSV();
    void testRGBHSV();
    void testRGBHSL();
    void testFloatingHLS();
    void testIntegerHLS();
    void testHSI();
    void testHSY();
    void testHCI();
    void testHCY();
    void testYUV();
    void testLabLCH();
    void testXYZxyY();
    void testCMYCMYK();
};

#endif

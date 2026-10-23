/*
 *  SPDX-FileCopyrightText: 2005 Adrian Page <adrian@pagenet.plus.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */


#ifndef KORGBCOLORSPACETESTER_H
#define KORGBCOLORSPACETESTER_H

#include <QObject>

class KoRgbU8ColorSpaceTester : public QObject
{
    Q_OBJECT
    void testCompositeOps();
private Q_SLOTS:
    void testBasics();
    void testMixColors();
    void testMixColorsAverage();
    void testCompositeOpsWithChannelFlags();
    void testCompositeCopyDivisionByZero();
    void testSourceOverPreservesDivisionRounding_data();
    void testSourceOverPreservesDivisionRounding();
    void testSourceOverPreservesPixelsAcrossBatchBoundaries_data();
    void testSourceOverPreservesPixelsAcrossBatchBoundaries();
    void testInverseFloatMaskPreservesPixelsAcrossBatches_data();
    void testInverseFloatMaskPreservesPixelsAcrossBatches();
};

#endif


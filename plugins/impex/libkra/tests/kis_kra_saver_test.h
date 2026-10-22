/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_KRA_SAVER_TEST_H
#define KIS_KRA_SAVER_TEST_H

#include <simpletest.h>

class KisKraSaverTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:

    void initTestCase();

    void testCrashyShapeLayer();

    // XXX: Also test roundtripping of metadata
    void testRoundTrip();

    void testSaveEmpty();
    void testChannelProtectionSurvivesResaving_data();
    void testChannelProtectionSurvivesResaving();
    void testSavedPreviewPreservesImagePixels_data();
    void testSavedPreviewPreservesImagePixels();
    void testRoundTripFillLayerColor();
    void testRoundTripFillLayerPattern();

    void testRoundTripLayerStyles();

    void testRoundTripAnimation();

    void testRoundTripColorizeMask();

    void testRoundTripShapeLayer();
    void testRoundTripShapeSelection();
    void testRoundTripStoryboard();

    void testExportToReadonly();

    void testRoundTripCicp_data();
    void testRoundTripCicp();

};

#endif

/*
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _COMPRESSION_TEST_H_
#define _COMPRESSION_TEST_H_

#include <QObject>

class CompressionTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void emptyInputProducesEmptyOutput();
    void uncompressedModePreservesBinaryInput();
    void rleUsesPackBitsAndRoundTripsBinaryInput();
    void rleRejectsTruncatedInputAndPadsShortOutput();
    void zipRoundTripsBinaryInputAndRejectsMalformedData();
    void zipPredictionPreservesCurrentEightBitTransform();
    void zipPredictionPreservesCurrentSixteenBitTransform();
    void zipPredictionRejectsUnsupportedDepth();
    void unknownCompressionTypeIsFatal_data();
    void unknownCompressionTypeIsFatal();
};

#endif

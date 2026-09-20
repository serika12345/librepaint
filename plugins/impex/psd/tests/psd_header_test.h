/*
 * SPDX-FileCopyrightText: 2009 Boudewijn Rempt <boud@valdyas.org>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef _PSD_HEADER_TEST_H_
#define _PSD_HEADER_TEST_H_

#include <QObject>

class PSDHeaderTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void defaultsExposeInvalidEmptyHeader();
    void validationEnforcesFormatSpecificRanges();
    void readDecodesBigEndianHeaderAndRejectsTruncation();
    void writeProducesCanonicalHeaderBytes();
    void debugOutputDescribesValidatedHeader();
};

#endif

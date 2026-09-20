/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_DISPLAY_COLOR_TRANSFORM_TEST_H
#define KIS_DISPLAY_COLOR_TRANSFORM_TEST_H

#include <QObject>

class KisDisplayColorTransformTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testStandardDisplayConversionWithoutUi();
    void testDisplayFilterParticipatesInConversion();
};

#endif

/*
 *  SPDX-FileCopyrightText: 2021 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#ifndef TESTKOSTOPGRADIENT_H
#define TESTKOSTOPGRADIENT_H

#include <QObject>

class TestKoStopGradient: public QObject
{
    Q_OBJECT
public:
private Q_SLOTS:
    void TestSVGStopGradientLoading();
    void testInterpolatesBetweenStops();

};

#endif // TESTKOSTOPGRADIENT_H

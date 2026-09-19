/*
 *  SPDX-FileCopyrightText: 2021 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "TestKoStopGradient.h"

#include <simpletest.h>

#include <QDomElement>

#include "KoColorModelStandardIds.h"

#include "KoStopGradient.h"

#include "KoColor.h"
#include "KoColorSpace.h"
#include "KoColorProfile.h"
#include "KoColorSpaceRegistry.h"

#include <testpigment.h>

void TestKoStopGradient::TestSVGStopGradientLoading()
{
    QHash <QString, const KoColorProfile *> profileList;
    KoStopGradient gradient;

    const KoColorSpace *cmyk = KoColorSpaceRegistry::instance()->colorSpace(CMYKAColorModelID.id(), Integer8BitsColorDepthID.id());
    QString cmykName = "sillyCMYKName";
    profileList.insert(cmykName, cmyk->profile());

    QList<KoGradientStop> stops;

    stops << KoGradientStop(0.0, KoColor::fromSVG11("#ff00ff icc-color(sillyCMYKName, 1.0, 0, 0, 0)", profileList));
    stops << KoGradientStop(0.5, KoColor::fromSVG11("#777777 icc-color(sillyCMYKName, 0, .5, 1, 0)", profileList));
    stops << KoGradientStop(0.5, KoColor::fromSVG11("#00ff00 icc-color(sillyCMYKName, 1.0, 0, 1, 0)", profileList));

    gradient.setStops(stops);

    // We need a better way to check if this worked.

    QString svgSerialization = gradient.saveSvgGradient();
    QVERIFY2(svgSerialization.contains("icc-color"), QString("icc-color not found in serialization of cmyk gradient.").toLatin1());
    QVERIFY2(svgSerialization.contains("color-profile"), QString("color-profile not found in serialization of cmyk gradient.").toLatin1());

}

void TestKoStopGradient::testInterpolatesBetweenStops()
{
    const KoColorSpace *space = KoColorSpaceRegistry::instance()->rgb8();
    KoStopGradient gradient;
    gradient.setStops({KoGradientStop(0.0, KoColor(Qt::black, space)),
                       KoGradientStop(1.0, KoColor(Qt::white, space))});

    KoColor sample(space);
    gradient.colorAt(sample, 0.0);
    QCOMPARE(sample.toQColor(), QColor(Qt::black));
    gradient.colorAt(sample, 1.0);
    QCOMPARE(sample.toQColor(), QColor(Qt::white));
    gradient.colorAt(sample, 0.5);
    const QColor middle = sample.toQColor();
    QVERIFY(qAbs(middle.red() - 128) <= 1);
    QCOMPARE(middle.green(), middle.red());
    QCOMPARE(middle.blue(), middle.red());
    QCOMPARE(middle.alpha(), 255);
}

KISTEST_MAIN(TestKoStopGradient)

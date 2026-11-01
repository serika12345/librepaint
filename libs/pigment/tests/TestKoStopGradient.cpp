/*
 *  SPDX-FileCopyrightText: 2021 Wolthera van Hövell tot Westerflier <griffinvalley@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#include "TestKoStopGradient.h"

#include <simpletest.h>

#include <QDomElement>
#include <cmath>

#include "KoColorModelStandardIds.h"

#include "KoStopGradient.h"
#include "KoCachedGradient.h"

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

void TestKoStopGradient::singleSampleCacheKeepsFirstColor()
{
    // A gradient on a one-pixel canvas/selection uses a one-entry palette.
    // The native painter and GPU resource capture must retain its first color
    // at every cache position rather than sampling an undefined 0/0 position.
    class RecordingGradient final : public KoStopGradient {
    public:
        mutable bool finiteSamples = true;
        void colorAt(KoColor &color, qreal position) const override {
            if (!std::isfinite(position)) { finiteSamples = false; return; }
            KoStopGradient::colorAt(color, position);
        }
    };
    const auto gradient = QSharedPointer<RecordingGradient>::create();
    const auto *rgb = KoColorSpaceRegistry::instance()->rgb8();
    gradient->setStops({{0, KoColor(QColor(213, 49, 107, 177), rgb)},
                        {1, KoColor(QColor(17, 129, 241, 93), rgb)}});
    for (const auto &model : {RGBAColorModelID, GrayAColorModelID}) {
        const auto *space = KoColorSpaceRegistry::instance()->colorSpace(model.id(), Integer16BitsColorDepthID.id(), QString());
        QVERIFY(space);
        KoColor expected(space); gradient->colorAt(expected, 0);
        const KoCachedGradient cached(gradient, 1, space);
        QVERIFY2(gradient->finiteSamples, "The gradient resource received a nonfinite cache position");
        for (const qreal position : {qreal(0), qreal(.5), qreal(1)})
            QCOMPARE(QByteArray(reinterpret_cast<const char *>(cached.cachedAt(position)), space->pixelSize()),
                     QByteArray(reinterpret_cast<const char *>(expected.data()), space->pixelSize()));
        const KoCachedGradient ordinary(gradient, 5, space);
        for (const qreal position : {qreal(0), qreal(.5), qreal(1)}) {
            gradient->colorAt(expected, position);
            QCOMPARE(QByteArray(reinterpret_cast<const char *>(ordinary.cachedAt(position)), space->pixelSize()),
                     QByteArray(reinterpret_cast<const char *>(expected.data()), space->pixelSize()));
        }
    }
}

KISTEST_MAIN(TestKoStopGradient)

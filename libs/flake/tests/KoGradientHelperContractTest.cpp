/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoGradientHelper.h>

#include <QScopedPointer>
#include <QTest>

class KoGradientHelperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createsRequestedDefaultGradient();
    void convertsGradientGeometryAndAppearance();
    void interpolatesAndPreservesCurrentExactStopSelection();
};

void KoGradientHelperContractTest::createsRequestedDefaultGradient()
{
    const QGradientStops stops {
        {0.0, QColor(Qt::red)},
        {1.0, QColor(Qt::blue)}};

    for (const QGradient::Type type : {
             QGradient::LinearGradient,
             QGradient::RadialGradient,
             QGradient::ConicalGradient}) {
        QScopedPointer<QGradient> gradient(
            KoGradientHelper::defaultGradient(type, QGradient::RepeatSpread, stops));

        QVERIFY(gradient);
        QCOMPARE(gradient->type(), type);
        QCOMPARE(gradient->coordinateMode(), QGradient::ObjectBoundingMode);
        QCOMPARE(gradient->spread(), QGradient::RepeatSpread);
        QCOMPARE(gradient->stops(), stops);
    }

    QVERIFY(!KoGradientHelper::defaultGradient(
        QGradient::NoGradient,
        QGradient::PadSpread,
        stops));
}

void KoGradientHelperContractTest::convertsGradientGeometryAndAppearance()
{
    const QGradientStops stops {
        {0.0, QColor(Qt::yellow)},
        {1.0, QColor(Qt::green)}};
    QLinearGradient source(QPointF(1.0, 2.0), QPointF(4.0, 6.0));
    source.setCoordinateMode(QGradient::StretchToDeviceMode);
    source.setSpread(QGradient::ReflectSpread);
    source.setStops(stops);

    QScopedPointer<QGradient> converted(
        KoGradientHelper::convertGradient(&source, QGradient::RadialGradient));

    QVERIFY(converted);
    QCOMPARE(converted->type(), QGradient::RadialGradient);
    QCOMPARE(converted->coordinateMode(), QGradient::ObjectBoundingMode);
    QCOMPARE(converted->spread(), source.spread());
    QCOMPARE(converted->stops(), source.stops());
    const auto *radial = static_cast<const QRadialGradient *>(converted.data());
    QCOMPARE(radial->center(), source.start());
    QCOMPARE(radial->focalPoint(), source.start());
    QCOMPARE(radial->radius(), 5.0);

    QVERIFY(!KoGradientHelper::convertGradient(&source, QGradient::NoGradient));
}

void KoGradientHelperContractTest::interpolatesAndPreservesCurrentExactStopSelection()
{
    const QColor red(Qt::red);
    const QColor blue(Qt::blue);
    const QGradientStops stops {{0.0, red}, {1.0, blue}};

    QVERIFY(!KoGradientHelper::colorAt(0.5, {}).isValid());
    QCOMPARE(KoGradientHelper::colorAt(0.5, {{0.25, red}}), red);
    QCOMPARE(KoGradientHelper::colorAt(-0.5, stops), red);
    QCOMPARE(KoGradientHelper::colorAt(1.5, stops), blue);

    const QColor middle = KoGradientHelper::colorAt(0.5, stops);
    QCOMPARE(middle, QColor::fromRgbF(0.5, 0.0, 0.5, 1.0));

    QCOMPARE(KoGradientHelper::colorAt(0.0, stops), blue);
    QCOMPARE(KoGradientHelper::colorAt(1.0, stops), red);
}

QTEST_GUILESS_MAIN(KoGradientHelperContractTest)

#include "KoGradientHelperContractTest.moc"

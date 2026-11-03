/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt boud @valdyas.org
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_paintop_test.h"

#include <simpletest.h>
#include "KoID.h"
#include "brushengine/KisPaintOpPresetUpdateProxy.h"
#include "brushengine/kis_no_size_paintop_settings.h"
#include "brushengine/kis_slider_based_paintop_property.h"
#include "brushengine/kis_standard_uniform_properties_factory.h"
#include "brushengine/kis_uniform_paintop_property.h"
#include "kis_paintop.h"
#include "kis_painter.h"
#include "kis_spacing_information.h"
#include "brushengine/kis_paintop_utils.h"
#include "brushengine/kis_paint_information.h"
#include "kis_distance_information.h"
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

class TestPaintOp : public KisPaintOp
{
public:

    TestPaintOp(KisPainter * gc)
            : KisPaintOp(gc) {
    }

protected:

    KisSpacingInformation paintAt(const KisPaintInformation&) override {
        return KisSpacingInformation(0.0);
    }

    KisSpacingInformation updateSpacingImpl(const KisPaintInformation&) const override
    {
        return KisSpacingInformation(0.0);
    }

};

class TestPaintOpSettings : public KisPaintOpSettings
{
public:
    TestPaintOpSettings()
        : KisPaintOpSettings(KisResourcesInterfaceSP())
    {
    }

    void setPaintOpSize(qreal value) override
    {
        m_size = value;
    }

    qreal paintOpSize() const override
    {
        return m_size;
    }

    void setPaintOpAngle(qreal value) override
    {
        m_angle = value;
    }

    qreal paintOpAngle() const override
    {
        return m_angle;
    }

private:
    qreal m_size = 25.0;
    qreal m_angle = 0.0;
};

class TestPaintOpSettingsUpdateListener : public KisPaintOpSettings::UpdateListener
{
public:
    void setDirty(bool value) override
    {
        m_dirty = value;
    }

    bool isDirty() const override
    {
        return m_dirty;
    }

    void notifySettingsChanged() override
    {
    }

private:
    bool m_dirty = false;
};

void KisPaintopTest::testCreation()
{
    KisPainter p;
    TestPaintOp test(&p);
}

void KisPaintopTest::testUniformPropertyConfiguration()
{
    KisPaintOpSettingsSP settings = new KisNoSizePaintOpSettings(KisResourcesInterfaceSP());
    KisUniformPaintOpProperty property(KoID("test"), settings, nullptr);

    QCOMPARE(property.configuration(), settings.data());

    settings.clear();
    QVERIFY(property.configuration());

    KisUniformPaintOpProperty emptyProperty(KoID("empty"), KisPaintOpSettingsSP(), nullptr);
    QVERIFY(!emptyProperty.configuration());
}

void KisPaintopTest::testStandardUniformPropertiesSynchronizeSettings()
{
    using namespace KisStandardUniformPropertiesFactory;

    KisPaintOpSettingsSP settings = new TestPaintOpSettings;
    const auto settingsListener = QSharedPointer<TestPaintOpSettingsUpdateListener>::create();
    settings->setUpdateListener(settingsListener);
    KisPaintOpPresetUpdateProxy updateProxy;

    const auto sizeProperty = createProperty(size, settings, &updateProxy);
    QVERIFY(sizeProperty);
    QCOMPARE(sizeProperty->id(), QStringLiteral("size"));
    QCOMPARE(sizeProperty->value().toReal(), 25.0);

    auto *sizeSlider = dynamic_cast<KisDoubleSliderBasedPaintOpProperty *>(sizeProperty.data());
    QVERIFY(sizeSlider);
    QCOMPARE(sizeSlider->min(), 0.0);
    QCOMPARE(sizeSlider->singleStep(), 1.0);
    QCOMPARE(sizeSlider->exponentRatio(), 3.0);
    QCOMPARE(sizeSlider->decimals(), 2);
    QCOMPARE(sizeSlider->suffix(), QStringLiteral(" px"));

    sizeProperty->setValue(48.5);
    QCOMPARE(settings->paintOpSize(), 48.5);

    settings->setPaintOpSize(12.25);
    updateProxy.postponeSettingsChanges();
    updateProxy.notifySettingsChanged();
    updateProxy.unpostponeSettingsChanges();
    QCOMPARE(sizeProperty->value().toReal(), 12.25);

    const auto opacityProperty = createProperty(QStringLiteral("opacity"), settings, &updateProxy);
    QVERIFY(opacityProperty);
    QCOMPARE(opacityProperty->value().toReal(), 1.0);
    opacityProperty->setValue(0.35);
    QCOMPARE(settings->paintOpOpacity(), 0.35);

    auto *opacitySlider = dynamic_cast<KisDoubleSliderBasedPaintOpProperty *>(opacityProperty.data());
    QVERIFY(opacitySlider);
    QCOMPARE(opacitySlider->min(), 0.0);
    QCOMPARE(opacitySlider->max(), 1.0);
    QCOMPARE(opacitySlider->singleStep(), 0.01);
    QCOMPARE(opacitySlider->suffix(), QString());

    const auto flowProperty = createProperty(flow, settings, &updateProxy);
    QVERIFY(flowProperty);
    flowProperty->setValue(0.6);
    QCOMPARE(settings->paintOpFlow(), 0.6);
}

void KisPaintopTest::testFreehandOutlineUsesPreviousDistinctCursorPosition()
{
    // Consumer: Freehand and liquify canvas helpers that derive the brush-outline direction.
    // Operation: Move the cursor beyond the outline-history threshold.
    // Observable result: The next outline direction uses the previous distinct cursor position.
    // Failure impact: The brush outline flickers or points in the wrong direction while drawing.
    KisPaintOpUtils::PositionHistory history;
    const QPointF initialPoint(10.0, 20.0);
    const QPointF movedPoint(20.0, 30.0);
    const QPointF nextPoint(30.0, 40.0);

    history.reset(initialPoint);

    QCOMPARE(history.pushThroughHistory(movedPoint, 1.0), initialPoint);
    QCOMPARE(history.pushThroughHistory(nextPoint, 1.0), movedPoint);
}

void KisPaintopTest::testAutomaticBrushSpacingPreservesConfiguredStrokeGeometry()
{
    // Consumer: Brush paint operations that place dabs along a user-drawn stroke.
    // Operation: Enable automatic spacing for an anisotropic or isotropic brush shape.
    // Observable result: Dab distance follows the configured brush geometry, scale, and spacing mode.
    // Failure impact: Strokes gain gaps, excess dabs, or a direction-dependent density.
    const KisSpacingInformation anisotropicSpacing = KisPaintOpUtils::effectiveSpacing(
        4.0, 9.0, 1.0, true, false, 0.0, false, 0.0, true, 0.5, 1.0);
    QVERIFY(anisotropicSpacing.isDistanceSpacingEnabled());
    QCOMPARE(anisotropicSpacing.distanceSpacing(), QPointF(1.0, 1.5));
    QVERIFY(!anisotropicSpacing.isIsotropic());

    const KisSpacingInformation isotropicSpacing = KisPaintOpUtils::effectiveSpacing(
        4.0, 9.0, 2.0, false, true, 0.75, true, 0.0, true, 1.0, 1.0);
    QVERIFY(!isotropicSpacing.isDistanceSpacingEnabled());
    QCOMPARE(isotropicSpacing.distanceSpacing(), QPointF(6.0, 6.0));
    QCOMPARE(isotropicSpacing.rotation(), 0.0);
    QVERIFY(!isotropicSpacing.coordinateSystemFlipped());
}

void KisPaintopTest::testScaledBrushSkipsSubpixelDabs()
{
    // Consumer: Brush paint operations running at a small scale or low-detail level.
    // Operation: Evaluate a dab whose scaled width or height falls below one hundredth of a pixel.
    // Observable result: The operation skips an unrenderable dab and keeps a renderable dab.
    // Failure impact: Strokes spend work on invisible marks or unexpectedly lose visible marks.
    QVERIFY(KisPaintOpUtils::checkSizeTooSmall(1.0, 0.009, 10.0));
    QVERIFY(KisPaintOpUtils::checkSizeTooSmall(1.0, 10.0, 0.009));
    QVERIFY(!KisPaintOpUtils::checkSizeTooSmall(1.0, 0.01, 0.01));
}

void KisPaintopTest::testCubicTraversalKeepsGeometryAndInput_data()
{
    QTest::addColumn<int>("kind");
    QTest::newRow("open") << 0;
    QTest::newRow("closed") << 1;
    QTest::newRow("closed-collinear") << 2;
    QTest::newRow("point") << 3;
}

void KisPaintopTest::testCubicTraversalKeepsGeometryAndInput()
{
    QFETCH(int, kind);
    // Brush plugins consume these virtual line calls. A closed cubic must
    // retain its traversal and sensors when supplied as two equal halves.
    class LineRecordingPaintOp : public TestPaintOp {
    public:
        using TestPaintOp::TestPaintOp;
        QList<QPair<KisPaintInformation, KisPaintInformation>> lines;
        void paintLine(const KisPaintInformation &first, const KisPaintInformation &last,
                       KisDistanceInformation *) override
        {
            lines.append(qMakePair(first, last));
        }
    };
    const QPointF first(12.25, 13.75);
    const QPointF last = kind == 0 ? QPointF(105.5, 91.25) : first;
    const QPointF control1 = kind == 2 ? first + QPointF(96, 0) :
                            kind == 3 ? first : QPointF(114.125, -15.75);
    const QPointF control2 = kind == 2 ? first - QPointF(96, 0) :
                            kind == 3 ? first : QPointF(-26.375, 115.25);
    KisPaintInformation start(first, .2), end(last, .8);
    start.setCurrentTime(16);
    end.setCurrentTime(48);
    KisPainter painter;
    LineRecordingPaintOp actual(&painter), split(&painter);
    KisDistanceInformation distance;
    actual.paintBezierCurve(start, control1, control2, end, &distance);
    QVERIFY(!actual.lines.isEmpty());
    QCOMPARE(actual.lines.first().first.pos(), first);
    QCOMPARE(actual.lines.last().second.pos(), last);
    QCOMPARE(actual.lines.first().first.pressure(), start.pressure());
    QCOMPARE(actual.lines.last().second.pressure(), end.pressure());
    QCOMPARE(actual.lines.first().first.currentTime(), start.currentTime());
    QCOMPARE(actual.lines.last().second.currentTime(), end.currentTime());
    bool moved = false;
    QJsonArray recorded;
    for (qsizetype i = 0; i < actual.lines.size(); ++i) {
        const auto &line = actual.lines[i];
        moved |= line.first.pos() != line.second.pos();
        QVERIFY(line.first.pressure() <= line.second.pressure());
        QVERIFY(line.first.currentTime() <= line.second.currentTime());
        if (i) {
            QCOMPARE(actual.lines[i - 1].second.pos(), line.first.pos());
            QCOMPARE(actual.lines[i - 1].second.pressure(), line.first.pressure());
            QCOMPARE(actual.lines[i - 1].second.currentTime(), line.first.currentTime());
        }
        recorded.append(QJsonArray{line.first.pos().x(), line.first.pos().y(), line.second.pos().x(), line.second.pos().y(),
                                   line.first.pressure(), line.second.pressure(), line.first.currentTime(), line.second.currentTime()});
    }
    QCOMPARE(moved, kind != 3);
    vQPointF points;
    painter.getBezierCurvePoints(first, control1, control2, last, points);
    QVERIFY(!points.isEmpty());
    QCOMPARE(points.first(), first);
    bool hasExcursion = false;
    QJsonArray flattened;
    for (const auto &point : points) {
        hasExcursion |= point != first;
        flattened.append(QJsonArray{point.x(), point.y()});
    }
    QCOMPARE(hasExcursion, kind != 3);
    if (kind != 3) {
        const auto left = (first + control1) / 2;
        const auto middleControl = (control1 + control2) / 2;
        const auto right = (control2 + last) / 2;
        const auto leftInner = (left + middleControl) / 2;
        const auto rightInner = (middleControl + right) / 2;
        const auto midpoint = (leftInner + rightInner) / 2;
        const auto middle = KisPaintInformation::mix(midpoint, .5, start, end);
        split.paintBezierCurve(start, left, leftInner, middle, &distance);
        split.paintBezierCurve(middle, rightInner, right, end, &distance);
        QCOMPARE(actual.lines.size(), split.lines.size());
        for (qsizetype i = 0; i < actual.lines.size(); ++i) {
            QCOMPARE(actual.lines[i].first.pos(), split.lines[i].first.pos());
            QCOMPARE(actual.lines[i].second.pos(), split.lines[i].second.pos());
            QCOMPARE(actual.lines[i].first.pressure(), split.lines[i].first.pressure());
            QCOMPARE(actual.lines[i].second.pressure(), split.lines[i].second.pressure());
            QCOMPARE(actual.lines[i].first.currentTime(), split.lines[i].first.currentTime());
            QCOMPARE(actual.lines[i].second.currentTime(), split.lines[i].second.currentTime());
        }
    }
    const auto directory = qEnvironmentVariable("LIBREPAINT_GPU_RESULT_DIR");
    if (!directory.isEmpty()) {
        QVERIFY(QDir().mkpath(directory));
        QFile file(QDir(directory).filePath(QStringLiteral("cubic-traversal-") +
                   QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".json")));
        QVERIFY(file.open(QIODevice::WriteOnly));
        const auto bytes = QJsonDocument(QJsonObject{{QStringLiteral("lines"), recorded},
                                                     {QStringLiteral("points"), flattened}}).toJson(QJsonDocument::Compact);
        QCOMPARE(file.write(bytes), bytes.size());
    }
}


SIMPLE_TEST_MAIN(KisPaintopTest)

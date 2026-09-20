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


SIMPLE_TEST_MAIN(KisPaintopTest)

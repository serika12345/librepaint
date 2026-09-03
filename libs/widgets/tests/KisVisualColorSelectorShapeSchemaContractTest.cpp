/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <functional>

#include <KisDlgInternalColorSelector.h>
#include <KisVisualColorSelectorShape.h>
#include <KisVisualEllipticalSelectorShape.h>
#include <KisVisualRectangleSelectorShape.h>
#include <QTest>

#include <type_traits>
#include <utility>

#define ASSERT_SELECTOR_SHAPE_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisVisualColorSelectorShape::method)), signature>)

#define ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(type, method, signature)                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

#define ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(method, signature)                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDlgInternalColorSelector::method)), signature>)

class SelectorShapeConstructorProbe : public KisVisualColorSelectorShape
{
public:
    using KisVisualColorSelectorShape::KisVisualColorSelectorShape;

    QRect getSpaceForSquare(QRect geometry) override;
    QRect getSpaceForCircle(QRect geometry) override;
    QRect getSpaceForTriangle(QRect geometry) override;
    void setBorderWidth(int width) override;

protected:
    QPointF convertShapeCoordinateToWidgetCoordinate(QPointF position) const override;
    QPointF convertWidgetCoordinateToShapeCoordinate(QPointF position) const override;
    QRegion getMaskMap() override;
    void drawCursor(QPainter &painter) override;
};

class KisVisualColorSelectorShapeSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void visualColorSelectorShapeTypeAndDimensionSchemaRemainsStable();
    void visualColorSelectorShapeCursorAndChannelSchemaRemainsStable();
    void visualColorSelectorShapeColorAndImageSchemaRemainsStable();
    void visualColorSelectorShapeGeometrySchemaRemainsStable();
    void visualColorSelectorShapePolicyAndNotificationSchemaRemainsStable();
    void visualConcreteSelectorShapeConstructionSchemaRemainsStable();
    void visualConcreteSelectorShapeOneDimensionalTypeValuesRemainStable();
    void visualConcreteSelectorShapeGeometrySignaturesRemainStable();
    void visualRectangleSelectorShapeModeSignatureRemainsStable();
    void visualEllipticalSelectorShapeGamutSignaturesRemainStable();
    void internalColorSelectorTypeLifetimeAndFactorySchemaRemainStable();
    void internalColorSelectorConfigSchemaRemainStable();
    void internalColorSelectorColorSpaceAndRendererSignaturesRemainStable();
    void internalColorSelectorColorEditingSignaturesRemainStable();
    void internalColorSelectorNotificationAndCloseSignaturesRemainStable();
};

void KisVisualColorSelectorShapeSchemaContractTest::internalColorSelectorTypeLifetimeAndFactorySchemaRemainStable()
{
    using Dialog = KisDlgInternalColorSelector;
    using Config = Dialog::Config;
    using ScreenColorSamplerFactory = std::function<KisScreenColorSamplerBase *(QWidget *)>;
    using FactorySetter = void (*)(ScreenColorSamplerFactory);

    static_assert(std::is_class_v<Dialog>);
    static_assert(std::is_base_of_v<QDialog, Dialog>);
    static_assert(std::is_constructible_v<Dialog, QWidget *, KoColor, Config, const QString &>);
    static_assert(std::is_constructible_v<Dialog,
                                          QWidget *,
                                          KoColor,
                                          Config,
                                          const QString &,
                                          const KoColorDisplayRendererInterface *>);
    static_assert(std::has_virtual_destructor_v<Dialog>);
    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(setScreenColorSamplerFactory, FactorySetter);
}

void KisVisualColorSelectorShapeSchemaContractTest::internalColorSelectorConfigSchemaRemainStable()
{
    using Config = KisDlgInternalColorSelector::Config;

    static_assert(std::is_class_v<Config>);
    static_assert(std::is_default_constructible_v<Config>);
    static_assert(std::is_same_v<decltype(&Config::hexInput), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::modal), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::paletteBox), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::prevNextButtons), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::screenColorSampler), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::useAlpha), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::visualColorSelector), bool Config::*>);
}

void KisVisualColorSelectorShapeSchemaContractTest::internalColorSelectorColorSpaceAndRendererSignaturesRemainStable()
{
    using Dialog = KisDlgInternalColorSelector;
    using ColorSpaceSetter = void (Dialog::*)(const KoColorSpace *);
    using DisplayRendererSetter = void (Dialog::*)(const KoColorDisplayRendererInterface *);

    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(colorSpaceChanged, ColorSpaceSetter);
    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(lockUsedColorSpace, ColorSpaceSetter);
    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(setDisplayRenderer, DisplayRendererSetter);
}

void KisVisualColorSelectorShapeSchemaContractTest::internalColorSelectorColorEditingSignaturesRemainStable()
{
    using Dialog = KisDlgInternalColorSelector;
    using ColorGetter = KoColor (Dialog::*)();
    using AlphaChooser = void (Dialog::*)(bool);
    using ColorSetter = void (Dialog::*)(KoColor);
    using ColorPatchSetter = void (Dialog::*)(KoColorPatch *);

    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(getCurrentColor, ColorGetter);
    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(chooseAlpha, AlphaChooser);
    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(setPreviousColor, ColorSetter);
    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(slotColorUpdated, ColorSetter);
    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(slotSetColorFromPatch, ColorPatchSetter);
}

void KisVisualColorSelectorShapeSchemaContractTest::internalColorSelectorNotificationAndCloseSignaturesRemainStable()
{
    using Dialog = KisDlgInternalColorSelector;
    using ColorNotification = void (Dialog::*)(KoColor);
    using CloseRequest = void (Dialog::*)();

    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(signalForegroundColorChosen, ColorNotification);
    ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE(reject, CloseRequest);
}

void KisVisualColorSelectorShapeSchemaContractTest::visualConcreteSelectorShapeConstructionSchemaRemainsStable()
{
    using Dimensions = KisVisualColorSelectorShape::Dimensions;
    using RectangleShape = KisVisualRectangleSelectorShape;
    using EllipticalShape = KisVisualEllipticalSelectorShape;

    static_assert(std::is_class_v<RectangleShape>);
    static_assert(std::is_base_of_v<KisVisualColorSelectorShape, RectangleShape>);
    static_assert(std::is_constructible_v<RectangleShape, KisVisualColorSelector *, Dimensions, int, int>);
    static_assert(std::is_constructible_v<RectangleShape,
                                          KisVisualColorSelector *,
                                          Dimensions,
                                          int,
                                          int,
                                          int,
                                          RectangleShape::singelDTypes>);
    static_assert(std::has_virtual_destructor_v<RectangleShape>);

    static_assert(std::is_class_v<EllipticalShape>);
    static_assert(std::is_base_of_v<KisVisualColorSelectorShape, EllipticalShape>);
    static_assert(std::is_constructible_v<EllipticalShape, KisVisualColorSelector *, Dimensions, int, int>);
    static_assert(std::is_constructible_v<EllipticalShape,
                                          KisVisualColorSelector *,
                                          Dimensions,
                                          int,
                                          int,
                                          int,
                                          EllipticalShape::singelDTypes>);
    static_assert(std::has_virtual_destructor_v<EllipticalShape>);
}

void KisVisualColorSelectorShapeSchemaContractTest::visualConcreteSelectorShapeOneDimensionalTypeValuesRemainStable()
{
    using RectangleShape = KisVisualRectangleSelectorShape;
    using EllipticalShape = KisVisualEllipticalSelectorShape;

    static_assert(std::is_enum_v<RectangleShape::singelDTypes>);
    static_assert(RectangleShape::vertical == 0);
    static_assert(RectangleShape::horizontal == 1);
    static_assert(RectangleShape::border == 2);
    static_assert(RectangleShape::borderMirrored == 3);

    static_assert(std::is_enum_v<EllipticalShape::singelDTypes>);
    static_assert(EllipticalShape::border == 0);
    static_assert(EllipticalShape::borderMirrored == 1);
}

void KisVisualColorSelectorShapeSchemaContractTest::visualConcreteSelectorShapeGeometrySignaturesRemainStable()
{
    using RectangleShape = KisVisualRectangleSelectorShape;
    using RectangleGeometryQuery = QRect (RectangleShape::*)(QRect);
    using RectangleBorderSetter = void (RectangleShape::*)(int);
    using EllipticalShape = KisVisualEllipticalSelectorShape;
    using EllipticalGeometryQuery = QRect (EllipticalShape::*)(QRect);
    using EllipticalBorderSetter = void (EllipticalShape::*)(int);

    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(RectangleShape, getSpaceForSquare, RectangleGeometryQuery);
    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(RectangleShape, getSpaceForCircle, RectangleGeometryQuery);
    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(RectangleShape, getSpaceForTriangle, RectangleGeometryQuery);
    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(RectangleShape, setBorderWidth, RectangleBorderSetter);

    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(EllipticalShape, getSpaceForSquare, EllipticalGeometryQuery);
    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(EllipticalShape, getSpaceForCircle, EllipticalGeometryQuery);
    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(EllipticalShape, getSpaceForTriangle, EllipticalGeometryQuery);
    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(EllipticalShape, setBorderWidth, EllipticalBorderSetter);
}

void KisVisualColorSelectorShapeSchemaContractTest::visualRectangleSelectorShapeModeSignatureRemainsStable()
{
    using RectangleShape = KisVisualRectangleSelectorShape;
    using ModeSetter = void (RectangleShape::*)(RectangleShape::singelDTypes);

    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(RectangleShape, setOneDimensionalType, ModeSetter);
}

void KisVisualColorSelectorShapeSchemaContractTest::visualEllipticalSelectorShapeGamutSignaturesRemainStable()
{
    using EllipticalShape = KisVisualEllipticalSelectorShape;
    using GamutSupportQuery = bool (EllipticalShape::*)() const;
    using GamutUpdater = void (EllipticalShape::*)();

    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(EllipticalShape, supportsGamutMask, GamutSupportQuery);
    ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE(EllipticalShape, updateGamutMask, GamutUpdater);
}

void KisVisualColorSelectorShapeSchemaContractTest::visualColorSelectorShapeTypeAndDimensionSchemaRemainsStable()
{
    static_assert(std::is_class_v<KisVisualColorSelectorShape>);
    static_assert(std::is_abstract_v<KisVisualColorSelectorShape>);
    static_assert(std::is_enum_v<KisVisualColorSelectorShape::Dimensions>);
    static_assert(KisVisualColorSelectorShape::onedimensional == 0);
    static_assert(KisVisualColorSelectorShape::twodimensional == 1);
    static_assert(std::is_constructible_v<SelectorShapeConstructorProbe,
                                          KisVisualColorSelector *,
                                          KisVisualColorSelectorShape::Dimensions,
                                          int,
                                          int>);
    static_assert(std::has_virtual_destructor_v<KisVisualColorSelectorShape>);
    ASSERT_SELECTOR_SHAPE_SIGNATURE(getDimensions,
                                    KisVisualColorSelectorShape::Dimensions (KisVisualColorSelectorShape::*)() const);
}

void KisVisualColorSelectorShapeSchemaContractTest::visualColorSelectorShapeCursorAndChannelSchemaRemainsStable()
{
    ASSERT_SELECTOR_SHAPE_SIGNATURE(getCursorPosition, QPointF (KisVisualColorSelectorShape::*)() const);
    ASSERT_SELECTOR_SHAPE_SIGNATURE(setCursorPosition, void (KisVisualColorSelectorShape::*)(QPointF, bool));
    static_assert(
        std::is_same_v<decltype(std::declval<KisVisualColorSelectorShape &>().setCursorPosition(QPointF{})), void>);
    ASSERT_SELECTOR_SHAPE_SIGNATURE(channel, int (KisVisualColorSelectorShape::*)(int) const);
    ASSERT_SELECTOR_SHAPE_SIGNATURE(channelMask, quint32 (KisVisualColorSelectorShape::*)() const);
    ASSERT_SELECTOR_SHAPE_SIGNATURE(setChannelValues, void (KisVisualColorSelectorShape::*)(QVector4D, quint32));
}

void KisVisualColorSelectorShapeSchemaContractTest::visualColorSelectorShapeColorAndImageSchemaRemainsStable()
{
    ASSERT_SELECTOR_SHAPE_SIGNATURE(getImageMap, const QImage &(KisVisualColorSelectorShape::*)());
    ASSERT_SELECTOR_SHAPE_SIGNATURE(getCurrentColor, KoColor (KisVisualColorSelectorShape::*)());
    ASSERT_SELECTOR_SHAPE_SIGNATURE(getColorFromConverter, QColor (KisVisualColorSelectorShape::*)(KoColor));
    ASSERT_SELECTOR_SHAPE_SIGNATURE(forceImageUpdate, void (KisVisualColorSelectorShape::*)());
}

void KisVisualColorSelectorShapeSchemaContractTest::visualColorSelectorShapeGeometrySchemaRemainsStable()
{
    ASSERT_SELECTOR_SHAPE_SIGNATURE(getSpaceForSquare, QRect (KisVisualColorSelectorShape::*)(QRect));
    ASSERT_SELECTOR_SHAPE_SIGNATURE(getSpaceForCircle, QRect (KisVisualColorSelectorShape::*)(QRect));
    ASSERT_SELECTOR_SHAPE_SIGNATURE(getSpaceForTriangle, QRect (KisVisualColorSelectorShape::*)(QRect));
    ASSERT_SELECTOR_SHAPE_SIGNATURE(setBorderWidth, void (KisVisualColorSelectorShape::*)(int));
}

void KisVisualColorSelectorShapeSchemaContractTest::visualColorSelectorShapePolicyAndNotificationSchemaRemainsStable()
{
    ASSERT_SELECTOR_SHAPE_SIGNATURE(isHueControl, bool (KisVisualColorSelectorShape::*)() const);
    ASSERT_SELECTOR_SHAPE_SIGNATURE(supportsGamutMask, bool (KisVisualColorSelectorShape::*)() const);
    ASSERT_SELECTOR_SHAPE_SIGNATURE(updateGamutMask, void (KisVisualColorSelectorShape::*)());
    ASSERT_SELECTOR_SHAPE_SIGNATURE(setAcceptTabletEvents, void (KisVisualColorSelectorShape::*)(bool));
    ASSERT_SELECTOR_SHAPE_SIGNATURE(sigCursorMoved, void (KisVisualColorSelectorShape::*)(QPointF));
}

#undef ASSERT_CONCRETE_SELECTOR_SHAPE_SIGNATURE
#undef ASSERT_INTERNAL_COLOR_SELECTOR_SIGNATURE
#undef ASSERT_SELECTOR_SHAPE_SIGNATURE

QTEST_GUILESS_MAIN(KisVisualColorSelectorShapeSchemaContractTest)
#include "KisVisualColorSelectorShapeSchemaContractTest.moc"

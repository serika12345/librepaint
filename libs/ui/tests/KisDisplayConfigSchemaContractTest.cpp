/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <application/ui/workspace/kis_splash_screen.h>
#include <canvas/KisCanvasSurfaceColorSpaceManager.h>
#include <canvas/KisDisplayConfig.h>
#include <canvas/KisRootSurfaceInfoProxy.h>
#include <canvas/KisSRGBSurfaceColorSpaceManager.h>
#include <canvas/kis_display_color_converter.h>
#include <dialogs/kis_dlg_import_image_sequence.h>
#include <theme/thememanager.h>
#include <widgets/KisGamutMaskToolbar.h>
#include <widgets/KisProofingOptionsWidget.h>
#include <widgets/gradient/KisSegmentGradientEditor.h>
#include <widgets/kis_advanced_color_space_selector.h>
#include <widgets/kis_custom_image_widget.h>
#include <widgets/kis_paintop_presets_chooser_popup.h>
#include <widgets/kis_seexpr_script_chooser.h>

#include <surfacecolormanagement/KisSurfaceColorimetry.h>

#include <QDebug>
#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(method, signature)                                                    \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDisplayColorConverter::method)), signature>)

#define ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(method, signature)                                                      \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<signature>(&KisCanvasSurfaceColorSpaceManager::method)), signature>)
} // namespace

class KisDisplayConfigSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayConfigTypeAndConstructionSchemaRemainsStable();
    void displayConfigValueSchemaRemainsStable();
    void displayConfigExternalPolicySignaturesRemainStable();
    void multiSurfaceDisplayConfigValueSchemaRemainsStable();
    void multiSurfaceDisplayConfigProjectionSchemaRemainsStable();
    void displayColorConverterTypeConstructionAndRendererSchemaRemainStable();
    void displayColorConverterImageAndConfigurationSignaturesRemainStable();
    void displayColorConverterColorAndPaletteSignaturesRemainStable();
    void displayColorConverterDeviceImageAndNotificationSignaturesRemainStable();
    void displayColorConverterComponentSignaturesRemainStable();
    void surfaceColorManagerTypeConstructionAndLifetimeSchemaRemainStable();
    void surfaceColorManagerConfigurationSchemaRemainStable();
    void surfaceColorManagerReportSchemaRemainStable();
    void surfaceColorManagerNotificationSchemaRemainStable();
    void sRgbSurfaceColorManagerTypeConstructionAndPlatformFactorySchemaRemainStable();
    void rootSurfaceInfoProxySchemaRemainStable();
    void proofingOptionsWidgetSchemaRemainsStable();
    void themeManagerSchemaRemainsStable();
    void seExprScriptChooserSchemaRemainsStable();
    void paintOpPresetsChooserPopupSchemaRemainsStable();
    void advancedColorSpaceSelectorSchemaRemainsStable();
    void gamutMaskToolbarSchemaRemainsStable();
    void importImageSequenceDialogSchemaRemainsStable();
    void segmentGradientEditorSchemaRemainsStable();
    void customImageWidgetSchemaRemainsStable();
    void splashScreenSchemaRemainsStable();
};

void KisDisplayConfigSchemaContractTest::displayConfigTypeAndConstructionSchemaRemainsStable()
{
    using Intent = KoColorConversionTransformation::Intent;
    using ConversionFlags = KoColorConversionTransformation::ConversionFlags;
    using Options = std::pair<Intent, ConversionFlags>;

    static_assert(std::is_class_v<KisDisplayConfig>);
    static_assert(std::is_same_v<KisDisplayConfig::Options, Options>);
    static_assert(std::is_default_constructible_v<KisDisplayConfig>);
    static_assert(std::is_constructible_v<KisDisplayConfig, const KoColorProfile *, Intent, ConversionFlags, bool>);
    static_assert(std::is_same_v<decltype(KisDisplayConfig(nullptr, Intent::IntentPerceptual, ConversionFlags())),
                                 KisDisplayConfig>);

    static_assert(std::is_class_v<KisMultiSurfaceDisplayConfig>);
    static_assert(std::is_same_v<KisMultiSurfaceDisplayConfig::Options, Options>);
    static_assert(std::is_default_constructible_v<KisMultiSurfaceDisplayConfig>);
}

void KisDisplayConfigSchemaContractTest::displayConfigValueSchemaRemainsStable()
{
    using Config = KisDisplayConfig;

    static_assert(std::is_same_v<decltype(&Config::profile), const KoColorProfile * Config::*>);
    static_assert(std::is_same_v<decltype(&Config::intent), KoColorConversionTransformation::Intent Config::*>);
    static_assert(
        std::is_same_v<decltype(&Config::conversionFlags), KoColorConversionTransformation::ConversionFlags Config::*>);
    static_assert(std::is_same_v<decltype(&Config::isHDR), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::operator==), bool (Config::*)(const Config &) const>);
    static_assert(std::is_same_v<decltype(&Config::options), Config::Options (Config::*)() const>);
    static_assert(std::is_same_v<decltype(&Config::setOptions), void (Config::*)(const Config::Options &)>);
}

void KisDisplayConfigSchemaContractTest::displayConfigExternalPolicySignaturesRemainStable()
{
    using Config = KisDisplayConfig;

    static_assert(std::is_same_v<decltype(&Config::optionsFromKisConfig), Config::Options (*)(const KisConfig &)>);
    static_assert(std::is_same_v<decltype(&Config::initializeSystemColorManager), void (*)()>);
    static_assert(std::is_same_v<decltype(&Config::profileForScreen), const KoColorProfile *(*)(int)>);
    static_assert(std::is_same_v<decltype(static_cast<QDebug (*)(QDebug, const Config &)>(&operator<<)),
                                 QDebug (*)(QDebug, const Config &)>);
}

void KisDisplayConfigSchemaContractTest::multiSurfaceDisplayConfigValueSchemaRemainsStable()
{
    using Config = KisMultiSurfaceDisplayConfig;

    static_assert(std::is_same_v<decltype(&Config::uiProfile), const KoColorProfile * Config::*>);
    static_assert(std::is_same_v<decltype(&Config::canvasProfile), const KoColorProfile * Config::*>);
    static_assert(std::is_same_v<decltype(&Config::intent), KoColorConversionTransformation::Intent Config::*>);
    static_assert(
        std::is_same_v<decltype(&Config::conversionFlags), KoColorConversionTransformation::ConversionFlags Config::*>);
    static_assert(std::is_same_v<decltype(&Config::isCanvasHDR), bool Config::*>);
    static_assert(std::is_same_v<decltype(&Config::operator==), bool (Config::*)(const Config &) const>);
}

void KisDisplayConfigSchemaContractTest::multiSurfaceDisplayConfigProjectionSchemaRemainsStable()
{
    using Config = KisMultiSurfaceDisplayConfig;

    static_assert(std::is_same_v<decltype(&Config::uiDisplayConfig), KisDisplayConfig (Config::*)() const>);
    static_assert(std::is_same_v<decltype(&Config::canvasDisplayConfig), KisDisplayConfig (Config::*)() const>);
    static_assert(std::is_same_v<decltype(&Config::options), Config::Options (Config::*)() const>);
    static_assert(std::is_same_v<decltype(&Config::setOptions), void (Config::*)(const Config::Options &)>);
}

void KisDisplayConfigSchemaContractTest::displayColorConverterTypeConstructionAndRendererSchemaRemainStable()
{
    using Converter = KisDisplayColorConverter;
    using Options =
        std::pair<KoColorConversionTransformation::Intent, KoColorConversionTransformation::ConversionFlags>;

    static_assert(std::is_class_v<Converter>);
    static_assert(std::is_base_of_v<QObject, Converter>);
    static_assert(std::is_same_v<Converter::ConversionOptions, Options>);
    static_assert(std::is_default_constructible_v<Converter>);
    static_assert(std::is_constructible_v<Converter, KoCanvasResourceProvider *, QObject *>);
    static_assert(std::has_virtual_destructor_v<Converter>);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(dumbConverterInstance, Converter * (*)());
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(displayRendererInterface,
                                             KoColorDisplayRendererInterface * (Converter::*)() const);
}

void KisDisplayConfigSchemaContractTest::displayColorConverterImageAndConfigurationSignaturesRemainStable()
{
    using Converter = KisDisplayColorConverter;

    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(setImage, void (Converter::*)(KisImageSP));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(setImageColorSpace, void (Converter::*)(const KoColorSpace *));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(paintingColorSpace, const KoColorSpace *(Converter::*)() const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(nodeColorSpace, const KoColorSpace *(Converter::*)() const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(setMultiSurfaceDisplayConfig,
                                             void (Converter::*)(const KisMultiSurfaceDisplayConfig &));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(setDisplayFilter, void (Converter::*)(QSharedPointer<KisDisplayFilter>));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(displayConfig, KisDisplayConfig (Converter::*)() const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(displayFilter, QSharedPointer<KisDisplayFilter> (Converter::*)() const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(multiSurfaceDisplayConfig,
                                             KisMultiSurfaceDisplayConfig (Converter::*)() const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(conversionOptions, Converter::ConversionOptions (Converter::*)() const);
}

void KisDisplayConfigSchemaContractTest::displayColorConverterColorAndPaletteSignaturesRemainStable()
{
    using Converter = KisDisplayColorConverter;

    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(toQColor, QColor (Converter::*)(const KoColor &, bool) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(approximateFromRenderedQColor,
                                             KoColor (Converter::*)(const QColor &) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(canSkipDisplayConversion, bool (Converter::*)(const KoColorSpace *) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(applyDisplayFiltering,
                                             KoColor (Converter::*)(const KoColor &, const KoID &) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(convertColorToDisplayColorSpace,
                                             QColor (Converter::*)(const KoColor, bool) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(handlePaletteForDisplayColorSpace,
                                             KisHandlePalette (Converter::*)() const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(systemPaletteForDisplayColorSpace, QPalette (Converter::*)() const);
}

void KisDisplayConfigSchemaContractTest::displayColorConverterDeviceImageAndNotificationSignaturesRemainStable()
{
    using Converter = KisDisplayColorConverter;

    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(applyDisplayFilteringF32,
                                             void (Converter::*)(KisFixedPaintDeviceSP, const KoColorSpace *) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(convertImageToDisplayColorSpace,
                                             QImage (Converter::*)(KisPaintDeviceSP, QRect, bool) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(toQImage, QImage (Converter::*)(KisPaintDeviceSP, bool) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(toQImage,
                                             QImage (Converter::*)(const KoColorSpace *, const quint8 *, QSize, bool)
                                                 const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(displayConfigurationChanged, void (Converter::*)());
}

void KisDisplayConfigSchemaContractTest::displayColorConverterComponentSignaturesRemainStable()
{
    using Converter = KisDisplayColorConverter;

    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(fromHsv, KoColor (Converter::*)(int, int, int, int) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(fromHsvF, KoColor (Converter::*)(qreal, qreal, qreal, qreal));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(fromHslF, KoColor (Converter::*)(qreal, qreal, qreal, qreal));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(fromHsiF, KoColor (Converter::*)(qreal, qreal, qreal));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(fromHsyF,
                                             KoColor (Converter::*)(qreal, qreal, qreal, qreal, qreal, qreal, qreal));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(getHsv,
                                             void (Converter::*)(const KoColor &, int *, int *, int *, int *) const);
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(getHsvF,
                                             void (Converter::*)(const KoColor &, qreal *, qreal *, qreal *, qreal *));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(getHslF,
                                             void (Converter::*)(const KoColor &, qreal *, qreal *, qreal *, qreal *));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(getHsiF, void (Converter::*)(const KoColor &, qreal *, qreal *, qreal *));
    ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE(
        getHsyF,
        void (Converter::*)(const KoColor &, qreal *, qreal *, qreal *, qreal, qreal, qreal, qreal));
}

void KisDisplayConfigSchemaContractTest::surfaceColorManagerTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Manager = KisCanvasSurfaceColorSpaceManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_constructible_v<Manager,
                                          KisSurfaceColorManagerInterface *,
                                          KisConfig::CanvasSurfaceMode,
                                          const KisDisplayConfig::Options &,
                                          QObject *>);
    static_assert(std::has_virtual_destructor_v<Manager>);
}

void KisDisplayConfigSchemaContractTest::surfaceColorManagerConfigurationSchemaRemainStable()
{
    using Manager = KisCanvasSurfaceColorSpaceManager;

    ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(
        setDisplayConfigOptions,
        void (Manager::*)(KisConfig::CanvasSurfaceMode, const KisDisplayConfig::Options &));
    ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(setDisplayConfigOptions,
                                           void (Manager::*)(const KisDisplayConfig::Options &));
    ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(isReady, bool (Manager::*)() const);
    ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(displayConfig, KisDisplayConfig (Manager::*)() const);
}

void KisDisplayConfigSchemaContractTest::surfaceColorManagerReportSchemaRemainStable()
{
    using Manager = KisCanvasSurfaceColorSpaceManager;
    using SurfaceDescription = KisSurfaceColorimetry::SurfaceDescription;

    ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(colorManagementReport, QString (Manager::*)() const);
    ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(osPreferredColorSpaceReport, QString (Manager::*)() const);
    ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(currentSurfaceDescription,
                                           std::optional<SurfaceDescription> (Manager::*)() const);
    ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(lastErrorString, QString (Manager::*)() const);
}

void KisDisplayConfigSchemaContractTest::surfaceColorManagerNotificationSchemaRemainStable()
{
    using Manager = KisCanvasSurfaceColorSpaceManager;

    ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE(sigDisplayConfigChanged, void (Manager::*)(const KisDisplayConfig &));
}

void KisDisplayConfigSchemaContractTest::sRgbSurfaceColorManagerTypeConstructionAndPlatformFactorySchemaRemainStable()
{
    using Manager = KisSRGBSurfaceColorSpaceManager;

    static_assert(std::is_class_v<Manager>);
    static_assert(std::is_base_of_v<KisCanvasSurfaceColorSpaceManager, Manager>);
    static_assert(std::is_constructible_v<Manager, KisSurfaceColorManagerInterface *, QObject *>);
    static_assert(std::has_virtual_destructor_v<Manager>);
    static_assert(std::is_same_v<decltype(&Manager::tryCreateForCurrentPlatform), Manager *(*)(QWidget *)>);
}

void KisDisplayConfigSchemaContractTest::rootSurfaceInfoProxySchemaRemainStable()
{
    using Proxy = KisRootSurfaceInfoProxy;
    using SurfaceDescription = KisSurfaceColorimetry::SurfaceDescription;

    static_assert(std::is_class_v<Proxy>);
    static_assert(std::is_base_of_v<KisRootSurfaceTrackerBase, Proxy>);
    static_assert(std::is_constructible_v<Proxy, QWidget *, QObject *>);
    static_assert(std::has_virtual_destructor_v<Proxy>);
    static_assert(std::is_same_v<decltype(&Proxy::rootSurfaceProfile), const KoColorProfile *(Proxy::*)() const>);
    static_assert(std::is_same_v<decltype(&Proxy::isReady), bool (Proxy::*)() const>);
    static_assert(std::is_same_v<decltype(&Proxy::colorManagementReport), QString (Proxy::*)() const>);
    static_assert(std::is_same_v<decltype(&Proxy::osPreferredColorSpaceReport), QString (Proxy::*)() const>);
    static_assert(std::is_same_v<decltype(&Proxy::currentSurfaceDescription),
                                 std::optional<SurfaceDescription> (Proxy::*)() const>);
    static_assert(
        std::is_same_v<decltype(&Proxy::sigRootSurfaceProfileChanged), void (Proxy::*)(const KoColorProfile *) const>);

    QVERIFY(true);
}

void KisDisplayConfigSchemaContractTest::proofingOptionsWidgetSchemaRemainsStable()
{
    using Widget = KisProofingOptionsWidget;

    static_assert(std::is_base_of_v<QWidget, Widget>);
    static_assert(std::is_constructible_v<Widget, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Widget>);
    static_assert(
        std::is_same_v<decltype(&Widget::currentProofingConfig), KisProofingConfigurationSP (Widget::*)() const>);
    static_assert(std::is_same_v<decltype(&Widget::setProofingConfig), void (Widget::*)(KisProofingConfigurationSP)>);
    static_assert(std::is_same_v<decltype(&Widget::setDisplayConfigOptions),
                                 void (Widget::*)(const KisDisplayConfig::Options &)>);
    static_assert(std::is_same_v<decltype(&Widget::stopPendingUpdates), void (Widget::*)()>);
    static_assert(
        std::is_same_v<decltype(&Widget::sigProofingConfigChanged), void (Widget::*)(KisProofingConfigurationSP)>);
}

void KisDisplayConfigSchemaContractTest::themeManagerSchemaRemainsStable()
{
    using Manager = Digikam::ThemeManager;

    static_assert(std::is_base_of_v<QObject, Manager>);
    static_assert(std::is_constructible_v<Manager, const QString &, QObject *>);
    static_assert(std::has_virtual_destructor_v<Manager>);
    static_assert(std::is_same_v<decltype(&Manager::currentThemeName), QString (Manager::*)() const>);
    static_assert(std::is_same_v<decltype(&Manager::setCurrentTheme), void (Manager::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Manager::setThemeMenuAction), void (Manager::*)(KActionMenu *const)>);
    static_assert(std::is_same_v<decltype(&Manager::registerThemeActions), void (Manager::*)(KisKActionCollection *)>);
    static_assert(std::is_same_v<decltype(&Manager::signalThemeChanged), void (Manager::*)()>);
}

void KisDisplayConfigSchemaContractTest::seExprScriptChooserSchemaRemainsStable()
{
    using Chooser = KisSeExprScriptChooser;

    static_assert(std::is_base_of_v<QFrame, Chooser>);
    static_assert(std::is_constructible_v<Chooser, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Chooser>);
    static_assert(std::is_same_v<decltype(&Chooser::currentResource), KoResourceSP (Chooser::*)()>);
    static_assert(std::is_same_v<decltype(&Chooser::setCurrentScript), void (Chooser::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Chooser::setCurrentItem), void (Chooser::*)(int)>);
    static_assert(std::is_same_v<decltype(&Chooser::setPreviewOrientation), void (Chooser::*)(Qt::Orientation)>);
    static_assert(std::is_same_v<decltype(&Chooser::resourceSelected), void (Chooser::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Chooser::updateItemSize), void (Chooser::*)()>);
}

void KisDisplayConfigSchemaContractTest::paintOpPresetsChooserPopupSchemaRemainsStable()
{
    using Popup = KisPaintOpPresetsChooserPopup;

    static_assert(std::is_base_of_v<QWidget, Popup>);
    static_assert(std::is_constructible_v<Popup, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Popup>);
    static_assert(std::is_same_v<decltype(&Popup::updateViewSettings), void (Popup::*)()>);
    static_assert(std::is_same_v<decltype(&Popup::setResponsiveness), void (Popup::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Popup::canvasResourceChanged), void (Popup::*)(KisPaintOpPresetSP)>);
    static_assert(std::is_same_v<decltype(&Popup::slotThemeChanged), void (Popup::*)()>);
    static_assert(std::is_same_v<decltype(&Popup::resourceSelected), void (Popup::*)(KoResourceSP)>);
    static_assert(std::is_same_v<decltype(&Popup::resourceClicked), void (Popup::*)(KoResourceSP)>);
}

void KisDisplayConfigSchemaContractTest::advancedColorSpaceSelectorSchemaRemainsStable()
{
    using Selector = KisAdvancedColorSpaceSelector;

    static_assert(std::is_base_of_v<QDialog, Selector>);
    static_assert(std::is_constructible_v<Selector, QWidget *, const QString &>);
    static_assert(std::has_virtual_destructor_v<Selector>);
    static_assert(std::is_same_v<decltype(&Selector::currentColorSpace), const KoColorSpace *(Selector::*)()>);
    static_assert(std::is_same_v<decltype(&Selector::setCurrentColorModel), void (Selector::*)(const KoID &)>);
    static_assert(std::is_same_v<decltype(&Selector::setCurrentColorDepth), void (Selector::*)(const KoID &)>);
    static_assert(std::is_same_v<decltype(&Selector::setCurrentProfile), void (Selector::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Selector::setCurrentColorSpace), void (Selector::*)(const KoColorSpace *)>);
    static_assert(std::is_same_v<decltype(&Selector::selectionChanged), void (Selector::*)(bool)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Selector::*)(const KoColorSpace *)>(&Selector::colorSpaceChanged)),
                       void (Selector::*)(const KoColorSpace *)>);
}

void KisDisplayConfigSchemaContractTest::gamutMaskToolbarSchemaRemainsStable()
{
    using Toolbar = KisGamutMaskToolbar;

    static_assert(std::is_base_of_v<QWidget, Toolbar>);
    static_assert(std::is_constructible_v<Toolbar, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Toolbar>);
    static_assert(
        std::is_same_v<decltype(&Toolbar::connectMaskSignals), void (Toolbar::*)(KisCanvasResourceProvider *)>);
    static_assert(std::is_same_v<decltype(&Toolbar::sigGamutMaskToggle), void (Toolbar::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Toolbar::sigGamutMaskChanged), void (Toolbar::*)(KoGamutMaskSP)>);
    static_assert(std::is_same_v<decltype(&Toolbar::sigGamutMaskDeactivated), void (Toolbar::*)()>);
    static_assert(std::is_same_v<decltype(&Toolbar::slotGamutMaskSet), void (Toolbar::*)(KoGamutMaskSP)>);
    static_assert(std::is_same_v<decltype(&Toolbar::slotGamutMaskUnset), void (Toolbar::*)()>);
    static_assert(std::is_same_v<decltype(&Toolbar::slotGamutMaskDeactivate), void (Toolbar::*)()>);
}

void KisDisplayConfigSchemaContractTest::importImageSequenceDialogSchemaRemainsStable()
{
    using Dialog = KisDlgImportImageSequence;

    static_assert(std::is_base_of_v<KoDialog, Dialog>);
    static_assert(std::is_constructible_v<Dialog, KisMainWindow *, KisDocument *>);
    static_assert(std::is_same_v<decltype(&Dialog::sortOrder), int Dialog::*>);
    static_assert(std::is_same_v<decltype(&Dialog::showOpenFileDialog), QStringList (Dialog::*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::files), QStringList (Dialog::*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::firstFrame), int (Dialog::*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::step), int (Dialog::*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::autoAddHoldframes), bool (Dialog::*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::startFrom1), bool (Dialog::*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::isAscending), int (Dialog::*)()>);
}

void KisDisplayConfigSchemaContractTest::segmentGradientEditorSchemaRemainsStable()
{
    using Editor = KisSegmentGradientEditor;

    static_assert(std::is_base_of_v<QWidget, Editor>);
    static_assert(std::is_constructible_v<Editor, QWidget *>);
    static_assert(std::is_constructible_v<Editor,
                                          KoSegmentGradientSP,
                                          QWidget *,
                                          const char *,
                                          const QString &,
                                          KoCanvasResourcesInterfaceSP>);
    static_assert(std::has_virtual_destructor_v<Editor>);
    static_assert(std::is_same_v<decltype(&Editor::gradient), void (Editor::*)() const>);
    static_assert(std::is_same_v<decltype(&Editor::setGradient), void (Editor::*)(KoSegmentGradientSP)>);
    static_assert(
        std::is_same_v<decltype(&Editor::canvasResourcesInterface), KoCanvasResourcesInterfaceSP (Editor::*)() const>);
    static_assert(
        std::is_same_v<decltype(&Editor::setCanvasResourcesInterface), void (Editor::*)(KoCanvasResourcesInterfaceSP)>);
    static_assert(std::is_same_v<decltype(&Editor::setCompactMode), void (Editor::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Editor::sigGradientChanged), void (Editor::*)()>);
}

void KisDisplayConfigSchemaContractTest::customImageWidgetSchemaRemainsStable()
{
    using Widget = KisCustomImageWidget;

    static_assert(std::is_enum_v<CustomImageWidgetType>);
    static_assert(CUSTOM_DOCUMENT != NEW_IMG_FROM_CB);
    static_assert(std::is_base_of_v<QWidget, WdgNewImage>);
    static_assert(std::is_constructible_v<WdgNewImage, QWidget *>);
    static_assert(std::is_base_of_v<WdgNewImage, Widget>);
    static_assert(std::is_constructible_v<Widget,
                                          QWidget *,
                                          qint32,
                                          qint32,
                                          double,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QString &>);
    static_assert(std::has_virtual_destructor_v<Widget>);
}

void KisDisplayConfigSchemaContractTest::splashScreenSchemaRemainsStable()
{
    using Splash = KisSplashScreen;

    static_assert(std::is_base_of_v<QWidget, Splash>);
    static_assert(std::is_constructible_v<Splash, QWidget *, Qt::WindowFlags>);
    static_assert(std::is_same_v<decltype(&Splash::repaint), void (Splash::*)()>);
    static_assert(std::is_same_v<decltype(&Splash::show), void (Splash::*)()>);
    static_assert(std::is_same_v<decltype(&Splash::displayLinks), void (Splash::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Splash::displayRecentFiles), void (Splash::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Splash::setLoadingText), void (Splash::*)(QString)>);
    static_assert(std::is_same_v<decltype(&Splash::imageResourcePath), QString (*)()>);
}

#undef ASSERT_DISPLAY_COLOR_CONVERTER_SIGNATURE
#undef ASSERT_SURFACE_COLOR_MANAGER_SIGNATURE

QTEST_GUILESS_MAIN(KisDisplayConfigSchemaContractTest)

#include "KisDisplayConfigSchemaContractTest.moc"

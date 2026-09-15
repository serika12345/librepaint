/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "brushengine/kis_no_size_paintop_settings.h"
#include "brushengine/kis_paintop_config_widget.h"
#include "ui/kis_paintop_settings_widget.h"

#include <QTest>

#include <type_traits>

namespace
{

class PaintOpConfigWidgetProbe : public KisPaintOpConfigWidget
{
public:
    PaintOpConfigWidgetProbe(QWidget *parent, Qt::WindowFlags flags);

    KisPropertiesConfigurationSP configuration() const override;

protected:
    void setConfiguration(const KisPropertiesConfigurationSP config) override;
    void writeConfiguration(KisPropertiesConfigurationSP config) const override;
    KisPaintopLodLimitations lodLimitations() const override;
    lager::reader<KisPaintopLodLimitations> lodLimitationsReader() const override;
    lager::reader<qreal> effectiveBrushSize() const override;
};

class PaintOpSettingsWidgetProbe : public KisPaintOpSettingsWidget
{
public:
    explicit PaintOpSettingsWidgetProbe(QWidget *parent);

    KisPropertiesConfigurationSP configuration() const override;
};

#define ASSERT_CONFIG_WIDGET_SIGNATURE(method, ...)                                                                    \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisConfigWidget::method)), __VA_ARGS__>)
#define ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(method, ...)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisPaintOpConfigWidget::method)), __VA_ARGS__>)
#define ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(method, ...)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<__VA_ARGS__>(&KisPaintOpSettingsWidget::method)), __VA_ARGS__>)

} // namespace

class KisPaintOpConfigWidgetSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void configWidgetTypeAndLifetimeSchemaRemainStable();
    void configWidgetConfigurationSignaturesRemainStable();
    void configWidgetViewAndCanvasResourceSignaturesRemainStable();
    void configWidgetNotificationSignaturesRemainStable();
    void paintOpConfigWidgetTypeConstructionAndLifetimeSchemaRemainStable();
    void paintOpConfigWidgetSafeConfigurationSignaturesRemainStable();
    void paintOpConfigWidgetLodSignaturesRemainStable();
    void paintOpConfigWidgetContextAndScratchBoxSignaturesRemainStable();
    void paintOpSettingsWidgetTypeLifetimeAndOptionSignaturesRemainStable();
    void paintOpSettingsWidgetConfigurationLodAndContextSignaturesRemainStable();
    void noSizePaintOpSettingsSchemaRemainStable();
};

void KisPaintOpConfigWidgetSchemaContractTest::configWidgetTypeAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisConfigWidget>);
    static_assert(std::is_base_of_v<QWidget, KisConfigWidget>);
    static_assert(std::is_abstract_v<KisConfigWidget>);
    static_assert(std::has_virtual_destructor_v<KisConfigWidget>);

    QVERIFY(true);
}

void KisPaintOpConfigWidgetSchemaContractTest::configWidgetConfigurationSignaturesRemainStable()
{
    ASSERT_CONFIG_WIDGET_SIGNATURE(setConfiguration, void (KisConfigWidget::*)(KisPropertiesConfigurationSP));
    ASSERT_CONFIG_WIDGET_SIGNATURE(configuration, KisPropertiesConfigurationSP (KisConfigWidget::*)() const);
}

void KisPaintOpConfigWidgetSchemaContractTest::configWidgetViewAndCanvasResourceSignaturesRemainStable()
{
    ASSERT_CONFIG_WIDGET_SIGNATURE(setView, void (KisConfigWidget::*)(KisViewManager *));
    ASSERT_CONFIG_WIDGET_SIGNATURE(setCanvasResourcesInterface,
                                   void (KisConfigWidget::*)(KoCanvasResourcesInterfaceSP));
    ASSERT_CONFIG_WIDGET_SIGNATURE(canvasResourcesInterface, KoCanvasResourcesInterfaceSP (KisConfigWidget::*)() const);
}

void KisPaintOpConfigWidgetSchemaContractTest::configWidgetNotificationSignaturesRemainStable()
{
    ASSERT_CONFIG_WIDGET_SIGNATURE(sigConfigurationUpdated, void (KisConfigWidget::*)());
    ASSERT_CONFIG_WIDGET_SIGNATURE(sigConfigurationItemChanged, void (KisConfigWidget::*)());
    ASSERT_CONFIG_WIDGET_SIGNATURE(sigSaveLockedConfig, void (KisConfigWidget::*)(KisPropertiesConfigurationSP));
    ASSERT_CONFIG_WIDGET_SIGNATURE(sigDropLockedConfig, void (KisConfigWidget::*)(KisPropertiesConfigurationSP));
}

// clang-format off
void KisPaintOpConfigWidgetSchemaContractTest::paintOpConfigWidgetTypeConstructionAndLifetimeSchemaRemainStable()
// clang-format on
{
    static_assert(std::is_class_v<KisPaintOpConfigWidget>);
    static_assert(std::is_base_of_v<KisConfigWidget, KisPaintOpConfigWidget>);
    static_assert(std::is_abstract_v<KisPaintOpConfigWidget>);
    static_assert(std::is_constructible_v<PaintOpConfigWidgetProbe, QWidget *, Qt::WindowFlags>);
    static_assert(std::has_virtual_destructor_v<KisPaintOpConfigWidget>);

    QVERIFY(true);
}

// clang-format off
void KisPaintOpConfigWidgetSchemaContractTest::paintOpConfigWidgetSafeConfigurationSignaturesRemainStable()
// clang-format on
{
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(setConfigurationSafe,
                                           void (KisPaintOpConfigWidget::*)(KisPropertiesConfigurationSP));
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(writeConfigurationSafe,
                                           void (KisPaintOpConfigWidget::*)(KisPropertiesConfigurationSP) const);
}

void KisPaintOpConfigWidgetSchemaContractTest::paintOpConfigWidgetLodSignaturesRemainStable()
{
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(lodLimitations,
                                           KisPaintopLodLimitations (KisPaintOpConfigWidget::*)() const);
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(lodLimitationsReader,
                                           lager::reader<KisPaintopLodLimitations> (KisPaintOpConfigWidget::*)() const);
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(effectiveBrushSize,
                                           lager::reader<qreal> (KisPaintOpConfigWidget::*)() const);
}

// clang-format off
void KisPaintOpConfigWidgetSchemaContractTest::paintOpConfigWidgetContextAndScratchBoxSignaturesRemainStable()
// clang-format on
{
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(setImage, void (KisPaintOpConfigWidget::*)(KisImageWSP));
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(setNode, void (KisPaintOpConfigWidget::*)(KisNodeWSP));
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(setResourcesInterface,
                                           void (KisPaintOpConfigWidget::*)(KisResourcesInterfaceSP));
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(resourcesInterface,
                                           KisResourcesInterfaceSP (KisPaintOpConfigWidget::*)() const);
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(setView, void (KisPaintOpConfigWidget::*)(KisViewManager *));
    ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE(supportScratchBox, bool (KisPaintOpConfigWidget::*)());
}

void KisPaintOpConfigWidgetSchemaContractTest::paintOpSettingsWidgetTypeLifetimeAndOptionSignaturesRemainStable()
{
    static_assert(std::is_class_v<KisPaintOpSettingsWidget>);
    static_assert(std::is_base_of_v<KisPaintOpConfigWidget, KisPaintOpSettingsWidget>);
    static_assert(std::is_abstract_v<KisPaintOpSettingsWidget>);
    static_assert(std::is_constructible_v<PaintOpSettingsWidgetProbe, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KisPaintOpSettingsWidget>);
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(addPaintOpOption, void (KisPaintOpSettingsWidget::*)(KisPaintOpOption *));
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(
        addPaintOpOption,
        void (KisPaintOpSettingsWidget::*)(KisPaintOpOption *, KisPaintOpOption::PaintopCategory));
}

void KisPaintOpConfigWidgetSchemaContractTest::paintOpSettingsWidgetConfigurationLodAndContextSignaturesRemainStable()
{
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(setConfiguration,
                                             void (KisPaintOpSettingsWidget::*)(KisPropertiesConfigurationSP));
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(writeConfiguration,
                                             void (KisPaintOpSettingsWidget::*)(KisPropertiesConfigurationSP) const);
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(lodLimitations,
                                             KisPaintopLodLimitations (KisPaintOpSettingsWidget::*)() const);
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(lodLimitationsReader,
                                             lager::reader<KisPaintopLodLimitations> (KisPaintOpSettingsWidget::*)()
                                                 const);
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(effectiveBrushSize,
                                             lager::reader<qreal> (KisPaintOpSettingsWidget::*)() const);
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(setImage, void (KisPaintOpSettingsWidget::*)(KisImageWSP));
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(setNode, void (KisPaintOpSettingsWidget::*)(KisNodeWSP));
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(setResourcesInterface,
                                             void (KisPaintOpSettingsWidget::*)(KisResourcesInterfaceSP));
    ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE(setCanvasResourcesInterface,
                                             void (KisPaintOpSettingsWidget::*)(KoCanvasResourcesInterfaceSP));
}

void KisPaintOpConfigWidgetSchemaContractTest::noSizePaintOpSettingsSchemaRemainStable()
{
    using Settings = KisNoSizePaintOpSettings;

    static_assert(std::is_class_v<Settings>);
    static_assert(std::is_base_of_v<KisPaintOpSettings, Settings>);
    static_assert(std::is_constructible_v<Settings, KisResourcesInterfaceSP>);
    static_assert(std::is_same_v<decltype(&Settings::setPaintOpSize), void (Settings::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&Settings::paintOpSize), qreal (Settings::*)() const>);
    static_assert(std::is_same_v<decltype(&Settings::setPaintOpAngle), void (Settings::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&Settings::paintOpAngle), qreal (Settings::*)() const>);
}

#undef ASSERT_PAINTOP_SETTINGS_WIDGET_SIGNATURE
#undef ASSERT_PAINTOP_CONFIG_WIDGET_SIGNATURE
#undef ASSERT_CONFIG_WIDGET_SIGNATURE

QTEST_APPLESS_MAIN(KisPaintOpConfigWidgetSchemaContractTest)

#include "KisPaintOpConfigWidgetSchemaContractTest.moc"

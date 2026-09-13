/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisDynamicSensorFactoryDistance.h>
#include <KisDynamicSensorFactoryDrawingAngle.h>
#include <KisDynamicSensorFactoryFade.h>
#include <KisDynamicSensorFactoryRegistry.h>
#include <KisDynamicSensorFactoryTime.h>
#include <KisMirrorOption.h>
#include <KisScatterOption.h>

#include <QTest>

#include <type_traits>

namespace
{

template<typename Factory>
void verifyLengthAwareFactorySchema()
{
    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_base_of_v<KisSimpleDynamicSensorFactory, Factory>);
    static_assert(std::is_default_constructible_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::createConfigWidget),
                                 QWidget *(Factory::*)(lager::cursor<KisCurveOptionDataCommon>, QWidget *)>);
    static_assert(std::is_same_v<decltype(&Factory::maximumLabel), QString (Factory::*)(int)>);
    static_assert(std::is_same_v<decltype(&Factory::maximumValue), int (Factory::*)(int)>);
}

} // namespace

class KisDynamicSensorFactoryDrawingAngleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void drawingAngleFactorySchemaRemainStable();
    void distanceFactorySchemaRemainStable();
    void fadeFactorySchemaRemainStable();
    void factoryRegistrySchemaRemainStable();
    void mirrorOptionSchemaRemainStable();
    void scatterOptionSchemaRemainStable();
    void timeFactorySchemaRemainStable();
};

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::drawingAngleFactorySchemaRemainStable()
{
    using Factory = KisDynamicSensorFactoryDrawingAngle;

    static_assert(std::is_class_v<Factory>);
    static_assert(std::is_base_of_v<KisSimpleDynamicSensorFactory, Factory>);
    static_assert(std::is_default_constructible_v<Factory>);
    static_assert(std::is_same_v<decltype(&Factory::createConfigWidget),
                                 QWidget *(Factory::*)(lager::cursor<KisCurveOptionDataCommon>, QWidget *)>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::distanceFactorySchemaRemainStable()
{
    verifyLengthAwareFactorySchema<KisDynamicSensorFactoryDistance>();
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::fadeFactorySchemaRemainStable()
{
    verifyLengthAwareFactorySchema<KisDynamicSensorFactoryFade>();
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::factoryRegistrySchemaRemainStable()
{
    using Registry = KisDynamicSensorFactoryRegistry;

    static_assert(std::is_class_v<Registry>);
    static_assert(std::is_base_of_v<KoGenericRegistry<KisDynamicSensorFactory *>, Registry>);
    static_assert(std::is_default_constructible_v<Registry>);
    static_assert(std::has_virtual_destructor_v<Registry>);
    static_assert(std::is_same_v<decltype(&Registry::instance), Registry *(*)()>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::mirrorOptionSchemaRemainStable()
{
    using Option = KisMirrorOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisCurveOption, Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *>);
    static_assert(
        std::is_same_v<decltype(&Option::apply), MirrorProperties (Option::*)(const KisPaintInformation &) const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::scatterOptionSchemaRemainStable()
{
    using Option = KisScatterOption;

    static_assert(std::is_class_v<Option>);
    static_assert(std::is_base_of_v<KisCurveOption, Option>);
    static_assert(std::is_constructible_v<Option, const KisPropertiesConfiguration *>);
    static_assert(
        std::is_same_v<decltype(&Option::apply), QPointF (Option::*)(const KisPaintInformation &, qreal, qreal) const>);
}

void KisDynamicSensorFactoryDrawingAngleSchemaContractTest::timeFactorySchemaRemainStable()
{
    verifyLengthAwareFactorySchema<KisDynamicSensorFactoryTime>();
}

QTEST_GUILESS_MAIN(KisDynamicSensorFactoryDrawingAngleSchemaContractTest)

#include "KisDynamicSensorFactoryDrawingAngleSchemaContractTest.moc"

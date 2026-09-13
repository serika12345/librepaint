/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisDynamicSensorFactoryDrawingAngle.h>

#include <QTest>

#include <type_traits>

class KisDynamicSensorFactoryDrawingAngleSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void drawingAngleFactorySchemaRemainStable();
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

QTEST_GUILESS_MAIN(KisDynamicSensorFactoryDrawingAngleSchemaContractTest)

#include "KisDynamicSensorFactoryDrawingAngleSchemaContractTest.moc"

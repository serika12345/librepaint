/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <QSignalSpy>

#include <KoColor.h>
#include <KoColorSpaceRegistry.h>

#include <KisDisplayConfig.h>
#include <kis_display_color_converter.h>
#include <simpletest.h>

class KisDisplayColorConverterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testDisplayConfigurationIsAppliedOnce();
};

void KisDisplayColorConverterContractTest::testDisplayConfigurationIsAppliedOnce()
{
    KisDisplayColorConverter converter;
    QSignalSpy changedSpy(
        &converter,
        &KisDisplayColorConverter::displayConfigurationChanged);

    KisMultiSurfaceDisplayConfig configuration;
    configuration.uiProfile =
        KoColorSpaceRegistry::instance()->p709SRGBProfile();
    configuration.canvasProfile = configuration.uiProfile;
    configuration.intent =
        KoColorConversionTransformation::internalRenderingIntent();
    configuration.conversionFlags =
        KoColorConversionTransformation::internalConversionFlags();

    converter.setMultiSurfaceDisplayConfig(configuration);
    QCOMPARE(changedSpy.count(), 1);

    converter.setMultiSurfaceDisplayConfig(configuration);
    QCOMPARE(changedSpy.count(), 1);

    const QColor expected(18, 52, 86, 120);
    const KoColor source(
        expected, KoColorSpaceRegistry::instance()->rgb8());
    QCOMPARE(converter.toQColor(source), expected);
}

SIMPLE_TEST_MAIN(KisDisplayColorConverterContractTest)

#include "kis_display_color_converter_contract_test.moc"

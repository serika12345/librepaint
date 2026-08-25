/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceLoaderRegistry.h>
#include <KisResourceTypes.h>
#include <canvas/kis_canvas_resource_provider.h>
#include <simpletest.h>

class TestApplicationPaintingResourceRegistration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testCanvasOwnerRegistersPaintingResources();
};

void TestApplicationPaintingResourceRegistration::testCanvasOwnerRegistersPaintingResources()
{
    KisResourceLoaderRegistry *registry = KisResourceLoaderRegistry::instance();

    KisCanvasResourceProvider::registerPaintOpAndBrushResourceLoaders(*registry);
    KisCanvasResourceProvider::registerLayerStyleResourceLoader(*registry);
    KisCanvasResourceProvider::registerBrushResourceCacheFixup(*registry);

    QCOMPARE(registry->loader(ResourceType::PaintOpPresets,
                              QStringLiteral("application/x-krita-paintoppreset"))
                 ->resourceSubType(),
             ResourceSubType::KritaPaintOpPresets);
    QCOMPARE(registry->loader(ResourceType::Brushes, QStringLiteral("image/x-gimp-brush"))
                 ->resourceSubType(),
             ResourceSubType::GbrBrushes);
    QCOMPARE(registry->loader(ResourceType::Brushes,
                              QStringLiteral("image/x-gimp-brush-animated"))
                 ->resourceSubType(),
             ResourceSubType::GihBrushes);
    QCOMPARE(registry->loader(ResourceType::Brushes, QStringLiteral("image/svg+xml"))
                 ->resourceSubType(),
             ResourceSubType::SvgBrushes);
    QCOMPARE(registry->loader(ResourceType::Brushes, QStringLiteral("image/png"))
                 ->resourceSubType(),
             ResourceSubType::PngBrushes);
    QCOMPARE(registry->loader(ResourceType::LayerStyles,
                              QStringLiteral("application/x-photoshop-style"))
                 ->resourceSubType(),
             ResourceType::LayerStyles);
}

SIMPLE_TEST_MAIN(TestApplicationPaintingResourceRegistration)

#include "TestApplicationPaintingResourceRegistration.moc"

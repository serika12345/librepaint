/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisResourceLoaderRegistry.h>
#include <KisResourceTypes.h>
#include <kis_brush_registry.h>
#include <brushengine/kis_paintop_registry.h>
#include <kis_psd_layer_style.h>
#include <simpletest.h>

class TestBuiltInResourceLoaderRegistration : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testPaintingResourceOwnersRegisterTheirLoaders();
};

void TestBuiltInResourceLoaderRegistration::testPaintingResourceOwnersRegisterTheirLoaders()
{
    KisResourceLoaderRegistry *registry = KisResourceLoaderRegistry::instance();

    KisPaintOpRegistry::registerResourceLoader(*registry);
    KisBrushRegistry::registerResourceLoaders(*registry);
    KisPSDLayerStyle::registerResourceLoader(*registry);
    KisBrushRegistry::registerResourceCacheFixup(*registry);

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

SIMPLE_TEST_MAIN(TestBuiltInResourceLoaderRegistration)

#include "TestBuiltInResourceLoaderRegistration.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_acs_pixel_cache_renderer.h>

#include <QTest>

#include <type_traits>

namespace
{
class SamplerProbe;
} // namespace

class KisAcsPixelCacheRendererSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pixelCacheRendererTypeSchemaRemainsStable();
    void pixelCacheRenderEntrypointSchemaRemainsStable();
};

void KisAcsPixelCacheRendererSchemaContractTest::pixelCacheRendererTypeSchemaRemainsStable()
{
    static_assert(std::is_class_v<Acs::PixelCacheRenderer>);
    static_assert(std::is_default_constructible_v<Acs::PixelCacheRenderer>);

    QVERIFY(true);
}

void KisAcsPixelCacheRendererSchemaContractTest::pixelCacheRenderEntrypointSchemaRemainsStable()
{
    using RenderSignature = void (*)(SamplerProbe *,
                                     const KisDisplayColorConverter *,
                                     const QRect &,
                                     KisPaintDeviceSP &,
                                     QImage &,
                                     QPoint &,
                                     qreal);

    static_assert(std::is_same_v<decltype(static_cast<RenderSignature>(&Acs::PixelCacheRenderer::render<SamplerProbe>)),
                                 RenderSignature>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisAcsPixelCacheRendererSchemaContractTest)

#include "KisAcsPixelCacheRendererSchemaContractTest.moc"

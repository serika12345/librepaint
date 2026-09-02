/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoColorDisplayRendererInterface.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_INTERFACE_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoColorDisplayRendererInterface::method)), signature>)
#define ASSERT_DUMB_RENDERER_SIGNATURE(method, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoDumbColorDisplayRenderer::method)), signature>)

class DisplayRendererConstructorProbe : public KoColorDisplayRendererInterface
{
protected:
    DisplayRendererConstructorProbe()
        : KoColorDisplayRendererInterface()
    {
    }
};
} // namespace

class KoColorDisplayRendererSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void displayRendererInterfaceIdentityAndLifecycleSignaturesRemainStable();
    void displayRendererInterfaceColorConversionSignaturesRemainStable();
    void displayRendererInterfaceSpaceAndPaletteSignaturesRemainStable();
    void dumbDisplayRendererColorConversionSignaturesRemainStable();
    void dumbDisplayRendererIdentitySpaceAndPaletteSignaturesRemainStable();
};

void KoColorDisplayRendererSchemaContractTest::displayRendererInterfaceIdentityAndLifecycleSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoColorDisplayRendererInterface>);
    static_assert(std::is_abstract_v<KoColorDisplayRendererInterface>);
    static_assert(std::is_base_of_v<QObject, KoColorDisplayRendererInterface>);
    static_assert(std::has_virtual_destructor_v<KoColorDisplayRendererInterface>);
    static_assert(std::is_abstract_v<DisplayRendererConstructorProbe>);

    ASSERT_INTERFACE_SIGNATURE(displayConfigurationChanged, void (KoColorDisplayRendererInterface::*)());
}

void KoColorDisplayRendererSchemaContractTest::displayRendererInterfaceColorConversionSignaturesRemainStable()
{
    ASSERT_INTERFACE_SIGNATURE(approximateFromRenderedQColor,
                               KoColor (KoColorDisplayRendererInterface::*)(const QColor &) const);
    ASSERT_INTERFACE_SIGNATURE(convertColorToDisplayColorSpace,
                               QColor (KoColorDisplayRendererInterface::*)(const KoColor) const);
    ASSERT_INTERFACE_SIGNATURE(convertImageToDisplayColorSpace,
                               QImage (KoColorDisplayRendererInterface::*)(const QImage) const);
    ASSERT_INTERFACE_SIGNATURE(fromHsv, KoColor (KoColorDisplayRendererInterface::*)(int, int, int, int) const);
    ASSERT_INTERFACE_SIGNATURE(getHsv,
                               void (KoColorDisplayRendererInterface::*)(const KoColor &, int *, int *, int *, int *)
                                   const);
    ASSERT_INTERFACE_SIGNATURE(toQColor, QColor (KoColorDisplayRendererInterface::*)(const KoColor &, bool) const);
    ASSERT_INTERFACE_SIGNATURE(
        toQImage,
        QImage (KoColorDisplayRendererInterface::*)(const KoColorSpace *, const quint8 *, QSize, bool) const);
}

void KoColorDisplayRendererSchemaContractTest::displayRendererInterfaceSpaceAndPaletteSignaturesRemainStable()
{
    ASSERT_INTERFACE_SIGNATURE(getPaintingColorSpace, const KoColorSpace *(KoColorDisplayRendererInterface::*)() const);
    ASSERT_INTERFACE_SIGNATURE(handlePaletteForDisplayColorSpace,
                               KisHandlePalette (KoColorDisplayRendererInterface::*)() const);
    ASSERT_INTERFACE_SIGNATURE(maxVisibleFloatValue,
                               qreal (KoColorDisplayRendererInterface::*)(const KoChannelInfo *) const);
    ASSERT_INTERFACE_SIGNATURE(minVisibleFloatValue,
                               qreal (KoColorDisplayRendererInterface::*)(const KoChannelInfo *) const);
    ASSERT_INTERFACE_SIGNATURE(systemPaletteForDisplayColorSpace,
                               QPalette (KoColorDisplayRendererInterface::*)() const);
}

void KoColorDisplayRendererSchemaContractTest::dumbDisplayRendererColorConversionSignaturesRemainStable()
{
    ASSERT_DUMB_RENDERER_SIGNATURE(approximateFromRenderedQColor,
                                   KoColor (KoDumbColorDisplayRenderer::*)(const QColor &) const);
    ASSERT_DUMB_RENDERER_SIGNATURE(convertColorToDisplayColorSpace,
                                   QColor (KoDumbColorDisplayRenderer::*)(KoColor) const);
    ASSERT_DUMB_RENDERER_SIGNATURE(convertImageToDisplayColorSpace,
                                   QImage (KoDumbColorDisplayRenderer::*)(const QImage) const);
    ASSERT_DUMB_RENDERER_SIGNATURE(fromHsv, KoColor (KoDumbColorDisplayRenderer::*)(int, int, int, int) const);
    ASSERT_DUMB_RENDERER_SIGNATURE(getHsv,
                                   void (KoDumbColorDisplayRenderer::*)(const KoColor &, int *, int *, int *, int *)
                                       const);
    ASSERT_DUMB_RENDERER_SIGNATURE(toQColor, QColor (KoDumbColorDisplayRenderer::*)(const KoColor &, bool) const);
    ASSERT_DUMB_RENDERER_SIGNATURE(
        toQImage,
        QImage (KoDumbColorDisplayRenderer::*)(const KoColorSpace *, const quint8 *, QSize, bool) const);
}

void KoColorDisplayRendererSchemaContractTest::dumbDisplayRendererIdentitySpaceAndPaletteSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoDumbColorDisplayRenderer>);
    static_assert(std::is_base_of_v<KoColorDisplayRendererInterface, KoDumbColorDisplayRenderer>);
    static_assert(!std::is_abstract_v<KoDumbColorDisplayRenderer>);

    ASSERT_DUMB_RENDERER_SIGNATURE(getPaintingColorSpace, const KoColorSpace *(KoDumbColorDisplayRenderer::*)() const);
    ASSERT_DUMB_RENDERER_SIGNATURE(handlePaletteForDisplayColorSpace,
                                   KisHandlePalette (KoDumbColorDisplayRenderer::*)() const);
    ASSERT_DUMB_RENDERER_SIGNATURE(instance, KoColorDisplayRendererInterface * (*)());
    ASSERT_DUMB_RENDERER_SIGNATURE(maxVisibleFloatValue,
                                   qreal (KoDumbColorDisplayRenderer::*)(const KoChannelInfo *) const);
    ASSERT_DUMB_RENDERER_SIGNATURE(minVisibleFloatValue,
                                   qreal (KoDumbColorDisplayRenderer::*)(const KoChannelInfo *) const);
    ASSERT_DUMB_RENDERER_SIGNATURE(systemPaletteForDisplayColorSpace, QPalette (KoDumbColorDisplayRenderer::*)() const);
}

QTEST_GUILESS_MAIN(KoColorDisplayRendererSchemaContractTest)

#include "KoColorDisplayRendererSchemaContractTest.moc"

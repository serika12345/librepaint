/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_png_brush.h>
#include <kis_svg_brush.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PNG_BRUSH_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisPngBrush::method)), signature>)
#define ASSERT_SVG_BRUSH_SIGNATURE(method, signature)                                                                  \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisSvgBrush::method)), signature>)
} // namespace

class KisSingleImageBrushSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void pngConstructionAndLifetimeSchemaRemainStable();
    void pngResourceIoSchemaRemainStable();
    void svgConstructionAndLifetimeSchemaRemainStable();
    void svgResourceIoSchemaRemainStable();
};

void KisSingleImageBrushSchemaContractTest::pngConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisPngBrush>);
    static_assert(std::is_constructible_v<KisPngBrush, const QString &>);
    static_assert(std::is_copy_constructible_v<KisPngBrush>);
    static_assert(!std::is_copy_assignable_v<KisPngBrush>);
}

void KisSingleImageBrushSchemaContractTest::pngResourceIoSchemaRemainStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KisPngBrush::*)() const;

    ASSERT_PNG_BRUSH_SIGNATURE(clone, KoResourceSP (KisPngBrush::*)() const);
    ASSERT_PNG_BRUSH_SIGNATURE(defaultFileExtension, QString (KisPngBrush::*)() const);
    ASSERT_PNG_BRUSH_SIGNATURE(loadFromDevice, bool (KisPngBrush::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_PNG_BRUSH_SIGNATURE(resourceType, ResourceTypeSignature);
    ASSERT_PNG_BRUSH_SIGNATURE(saveToDevice, bool (KisPngBrush::*)(QIODevice *) const);
    ASSERT_PNG_BRUSH_SIGNATURE(toXML, void (KisPngBrush::*)(QDomDocument &, QDomElement &) const);
}

void KisSingleImageBrushSchemaContractTest::svgConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisSvgBrush>);
    static_assert(std::is_constructible_v<KisSvgBrush, const QString &>);
    static_assert(std::is_copy_constructible_v<KisSvgBrush>);
    static_assert(!std::is_copy_assignable_v<KisSvgBrush>);
}

void KisSingleImageBrushSchemaContractTest::svgResourceIoSchemaRemainStable()
{
    using ResourceTypeSignature = QPair<QString, QString> (KisSvgBrush::*)() const;

    ASSERT_SVG_BRUSH_SIGNATURE(clone, KoResourceSP (KisSvgBrush::*)() const);
    ASSERT_SVG_BRUSH_SIGNATURE(defaultFileExtension, QString (KisSvgBrush::*)() const);
    ASSERT_SVG_BRUSH_SIGNATURE(loadFromDevice, bool (KisSvgBrush::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_SVG_BRUSH_SIGNATURE(resourceType, ResourceTypeSignature);
    ASSERT_SVG_BRUSH_SIGNATURE(saveToDevice, bool (KisSvgBrush::*)(QIODevice *) const);
    ASSERT_SVG_BRUSH_SIGNATURE(toXML, void (KisSvgBrush::*)(QDomDocument &, QDomElement &) const);
}

QTEST_GUILESS_MAIN(KisSingleImageBrushSchemaContractTest)

#include "KisSingleImageBrushSchemaContractTest.moc"

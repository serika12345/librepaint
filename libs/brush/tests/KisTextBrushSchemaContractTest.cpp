/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_text_brush.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_TEXT_BRUSH_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisTextBrush::method)), signature>)
} // namespace

class KisTextBrushSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndResourceIoSchemaRemainStable();
    void textFontAndPipeSchemaRemainStable();
    void strokeSelectionSchemaRemainStable();
    void geometryAndConfigurationSchemaRemainStable();
    void dabRenderingAndSerializationSchemaRemainStable();
};

void KisTextBrushSchemaContractTest::typeConstructionAndResourceIoSchemaRemainStable()
{
    static_assert(std::is_class_v<KisTextBrush>);
    static_assert(std::is_same_v<KisTextBrushSP, QSharedPointer<KisTextBrush>>);
    static_assert(std::is_default_constructible_v<KisTextBrush>);
    static_assert(std::is_copy_constructible_v<KisTextBrush>);
    static_assert(std::has_virtual_destructor_v<KisTextBrush>);
    static_assert(!std::is_copy_assignable_v<KisTextBrush>);
    ASSERT_TEXT_BRUSH_SIGNATURE(clone, KoResourceSP (KisTextBrush::*)() const);
    ASSERT_TEXT_BRUSH_SIGNATURE(isEphemeral, bool (KisTextBrush::*)() const);
    ASSERT_TEXT_BRUSH_SIGNATURE(loadFromDevice, bool (KisTextBrush::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_TEXT_BRUSH_SIGNATURE(saveToDevice, bool (KisTextBrush::*)(QIODevice *) const);
}

void KisTextBrushSchemaContractTest::textFontAndPipeSchemaRemainStable()
{
    ASSERT_TEXT_BRUSH_SIGNATURE(setText, void (KisTextBrush::*)(const QString &));
    ASSERT_TEXT_BRUSH_SIGNATURE(text, QString (KisTextBrush::*)() const);
    ASSERT_TEXT_BRUSH_SIGNATURE(font, QFont (KisTextBrush::*)());
    ASSERT_TEXT_BRUSH_SIGNATURE(setFont, void (KisTextBrush::*)(const QFont &));
    ASSERT_TEXT_BRUSH_SIGNATURE(setPipeMode, void (KisTextBrush::*)(bool));
    ASSERT_TEXT_BRUSH_SIGNATURE(pipeMode, bool (KisTextBrush::*)() const);
    ASSERT_TEXT_BRUSH_SIGNATURE(updateBrush, void (KisTextBrush::*)());
}

void KisTextBrushSchemaContractTest::strokeSelectionSchemaRemainStable()
{
    ASSERT_TEXT_BRUSH_SIGNATURE(notifyStrokeStarted, void (KisTextBrush::*)());
    ASSERT_TEXT_BRUSH_SIGNATURE(prepareForSeqNo, void (KisTextBrush::*)(const KisPaintInformation &, int));
    ASSERT_TEXT_BRUSH_SIGNATURE(brushIndex, quint32 (KisTextBrush::*)() const);
}

void KisTextBrushSchemaContractTest::geometryAndConfigurationSchemaRemainStable()
{
    using MaskSizeSignature =
        qint32 (KisTextBrush::*)(const KisDabShape &, double, double, const KisPaintInformation &) const;

    ASSERT_TEXT_BRUSH_SIGNATURE(maskWidth, MaskSizeSignature);
    ASSERT_TEXT_BRUSH_SIGNATURE(maskHeight, MaskSizeSignature);
    ASSERT_TEXT_BRUSH_SIGNATURE(setAngle, void (KisTextBrush::*)(qreal));
    ASSERT_TEXT_BRUSH_SIGNATURE(setScale, void (KisTextBrush::*)(qreal));
    ASSERT_TEXT_BRUSH_SIGNATURE(setSpacing, void (KisTextBrush::*)(double));
}

void KisTextBrushSchemaContractTest::dabRenderingAndSerializationSchemaRemainStable()
{
    using PaintDeviceSignature = KisFixedPaintDeviceSP (
        KisTextBrush::*)(const KoColorSpace *, const KisDabShape &, const KisPaintInformation &, double, double) const;
    using GenerateDabSignature = void (KisTextBrush::*)(KisFixedPaintDeviceSP,
                                                        KisBrush::ColoringInformation *,
                                                        const KisDabShape &,
                                                        const KisPaintInformation &,
                                                        double,
                                                        double,
                                                        qreal,
                                                        qreal) const;

    ASSERT_TEXT_BRUSH_SIGNATURE(paintDevice, PaintDeviceSignature);
    ASSERT_TEXT_BRUSH_SIGNATURE(generateMaskAndApplyMaskOrCreateDab, GenerateDabSignature);
    ASSERT_TEXT_BRUSH_SIGNATURE(toXML, void (KisTextBrush::*)(QDomDocument &, QDomElement &) const);
}

QTEST_GUILESS_MAIN(KisTextBrushSchemaContractTest)

#include "KisTextBrushSchemaContractTest.moc"

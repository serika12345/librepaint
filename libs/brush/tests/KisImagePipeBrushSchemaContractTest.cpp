/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_imagepipe_brush.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PIPE_SIGNATURE(method, signature)                                                                       \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisImagePipeBrush::method)), signature>)
} // namespace

class KisImagePipeBrushSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndSelectionModeSchemaRemainStable();
    void constructionAndResourceIoSchemaRemainStable();
    void dabRenderingSchemaRemainStable();
    void brushConfigurationSchemaRemainStable();
    void pipeSelectionAndContentSchemaRemainStable();
};

void KisImagePipeBrushSchemaContractTest::typeAndSelectionModeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisImagePipeBrush>);
    static_assert(std::is_same_v<KisImagePipeBrushSP, QSharedPointer<KisImagePipeBrush>>);
    static_assert(std::is_enum_v<KisParasite::SelectionMode>);
    static_assert(KisParasite::Constant == 0);
    static_assert(KisParasite::Incremental == 1);
    static_assert(KisParasite::Angular == 2);
    static_assert(KisParasite::Velocity == 3);
    static_assert(KisParasite::Random == 4);
    static_assert(KisParasite::Pressure == 5);
    static_assert(KisParasite::TiltX == 6);
    static_assert(KisParasite::TiltY == 7);
}

void KisImagePipeBrushSchemaContractTest::constructionAndResourceIoSchemaRemainStable()
{
    using Devices = QVector<QVector<KisPaintDevice *>>;
    using Modes = QVector<KisParasite::SelectionMode>;

    static_assert(std::is_constructible_v<KisImagePipeBrush, const QString &>);
    static_assert(std::is_constructible_v<KisImagePipeBrush, const QString &, int, int, Devices, Modes>);
    static_assert(std::is_copy_constructible_v<KisImagePipeBrush>);
    static_assert(std::has_virtual_destructor_v<KisImagePipeBrush>);
    static_assert(!std::is_copy_assignable_v<KisImagePipeBrush>);
    ASSERT_PIPE_SIGNATURE(clone, KoResourceSP (KisImagePipeBrush::*)() const);
    ASSERT_PIPE_SIGNATURE(loadFromDevice, bool (KisImagePipeBrush::*)(QIODevice *, KisResourcesInterfaceSP));
    ASSERT_PIPE_SIGNATURE(saveToDevice, bool (KisImagePipeBrush::*)(QIODevice *) const);
    ASSERT_PIPE_SIGNATURE(defaultFileExtension, QString (KisImagePipeBrush::*)() const);
}

void KisImagePipeBrushSchemaContractTest::dabRenderingSchemaRemainStable()
{
    using PaintDeviceSignature = KisFixedPaintDeviceSP (
        KisImagePipeBrush::*)(const KoColorSpace *, const KisDabShape &, const KisPaintInformation &, double, double)
        const;
    using MaskSizeSignature =
        qint32 (KisImagePipeBrush::*)(const KisDabShape &, double, double, const KisPaintInformation &) const;
    using GenerateDabSignature = void (KisImagePipeBrush::*)(KisFixedPaintDeviceSP,
                                                             KisBrush::ColoringInformation *,
                                                             const KisDabShape &,
                                                             const KisPaintInformation &,
                                                             double,
                                                             double,
                                                             qreal,
                                                             qreal) const;

    ASSERT_PIPE_SIGNATURE(paintDevice, PaintDeviceSignature);
    ASSERT_PIPE_SIGNATURE(maskWidth, MaskSizeSignature);
    ASSERT_PIPE_SIGNATURE(maskHeight, MaskSizeSignature);
    ASSERT_PIPE_SIGNATURE(generateMaskAndApplyMaskOrCreateDab, GenerateDabSignature);
    ASSERT_PIPE_SIGNATURE(outline, KisOptimizedBrushOutline (KisImagePipeBrush::*)(bool) const);
    ASSERT_PIPE_SIGNATURE(makeMaskImage, void (KisImagePipeBrush::*)(bool));
    ASSERT_PIPE_SIGNATURE(canPaintFor, bool (KisImagePipeBrush::*)(const KisPaintInformation &));
}

void KisImagePipeBrushSchemaContractTest::brushConfigurationSchemaRemainStable()
{
    ASSERT_PIPE_SIGNATURE(setAdjustmentMidPoint, void (KisImagePipeBrush::*)(quint8));
    ASSERT_PIPE_SIGNATURE(setBrightnessAdjustment, void (KisImagePipeBrush::*)(qreal));
    ASSERT_PIPE_SIGNATURE(setContrastAdjustment, void (KisImagePipeBrush::*)(qreal));
    ASSERT_PIPE_SIGNATURE(setAutoAdjustMidPoint, void (KisImagePipeBrush::*)(bool));
    ASSERT_PIPE_SIGNATURE(setAngle, void (KisImagePipeBrush::*)(qreal));
    ASSERT_PIPE_SIGNATURE(setScale, void (KisImagePipeBrush::*)(qreal));
    ASSERT_PIPE_SIGNATURE(setSpacing, void (KisImagePipeBrush::*)(double));
    ASSERT_PIPE_SIGNATURE(brushIndex, quint32 (KisImagePipeBrush::*)() const);
    ASSERT_PIPE_SIGNATURE(coldInitBrush, void (KisImagePipeBrush::*)());
}

void KisImagePipeBrushSchemaContractTest::pipeSelectionAndContentSchemaRemainStable()
{
    ASSERT_PIPE_SIGNATURE(parasiteSelection, QString (KisImagePipeBrush::*)());
    ASSERT_PIPE_SIGNATURE(notifyStrokeStarted, void (KisImagePipeBrush::*)());
    ASSERT_PIPE_SIGNATURE(prepareForSeqNo, void (KisImagePipeBrush::*)(const KisPaintInformation &, int));
    ASSERT_PIPE_SIGNATURE(notifyBrushIsGoingToBeClonedForStroke, void (KisImagePipeBrush::*)());
    ASSERT_PIPE_SIGNATURE(brushes, QVector<KisGbrBrushSP> (KisImagePipeBrush::*)() const);
    ASSERT_PIPE_SIGNATURE(parasite, const KisPipeBrushParasite &(KisImagePipeBrush::*)() const);
    ASSERT_PIPE_SIGNATURE(setParasite, void (KisImagePipeBrush::*)(const KisPipeBrushParasite &));
    ASSERT_PIPE_SIGNATURE(setDevices, void (KisImagePipeBrush::*)(QVector<QVector<KisPaintDevice *>>, int, int));
}

QTEST_GUILESS_MAIN(KisImagePipeBrushSchemaContractTest)

#include "KisImagePipeBrushSchemaContractTest.moc"

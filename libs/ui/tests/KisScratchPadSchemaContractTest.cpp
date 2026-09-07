/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <tool/kis_scratch_pad.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_SCRATCH_PAD_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisScratchPad::method)), signature>)
} // namespace

class KisScratchPadSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeLifetimeAndModeSchemaRemainStable();
    void zoomAndPanSignaturesRemainStable();
    void boundsAndInputSignaturesRemainStable();
    void basicFillSignaturesRemainStable();
    void gradientAndDocumentFillSignaturesRemainStable();
    void imageTransferSignaturesRemainStable();
    void notificationSignaturesRemainStable();
};

void KisScratchPadSchemaContractTest::typeLifetimeAndModeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisScratchPad>);
    static_assert(std::is_base_of_v<QWidget, KisScratchPad>);
    static_assert(std::is_constructible_v<KisScratchPad, QWidget *>);
    static_assert(std::has_virtual_destructor_v<KisScratchPad>);
    ASSERT_SCRATCH_PAD_SIGNATURE(setupScratchPad, void (KisScratchPad::*)(KisCanvasResourceProvider *, const QColor &));
    ASSERT_SCRATCH_PAD_SIGNATURE(setModeManually, void (KisScratchPad::*)(bool));
    ASSERT_SCRATCH_PAD_SIGNATURE(setModeType, void (KisScratchPad::*)(QString));
    ASSERT_SCRATCH_PAD_SIGNATURE(resetState, void (KisScratchPad::*)());
}

void KisScratchPadSchemaContractTest::zoomAndPanSignaturesRemainStable()
{
    ASSERT_SCRATCH_PAD_SIGNATURE(canvasZoomLink, bool (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(setCanvasZoomLink, void (KisScratchPad::*)(bool));
    ASSERT_SCRATCH_PAD_SIGNATURE(setScale, bool (KisScratchPad::*)(qreal, qreal));
    ASSERT_SCRATCH_PAD_SIGNATURE(scaleX, qreal (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(scaleY, qreal (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(scaleToFit, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(scaleReset, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(panTo, void (KisScratchPad::*)(qint32, qint32));
    ASSERT_SCRATCH_PAD_SIGNATURE(panCenter, void (KisScratchPad::*)());
}

void KisScratchPadSchemaContractTest::boundsAndInputSignaturesRemainStable()
{
    ASSERT_SCRATCH_PAD_SIGNATURE(setCutoutOverlayRect, void (KisScratchPad::*)(const QRect &));
    ASSERT_SCRATCH_PAD_SIGNATURE(cutoutOverlay, QImage (KisScratchPad::*)() const);
    ASSERT_SCRATCH_PAD_SIGNATURE(imageUpdated, void (KisScratchPad::*)(const QRect &));
    ASSERT_SCRATCH_PAD_SIGNATURE(imageBounds, QRect (KisScratchPad::*)() const);
    ASSERT_SCRATCH_PAD_SIGNATURE(viewportBounds, QRect (KisScratchPad::*)() const);
    ASSERT_SCRATCH_PAD_SIGNATURE(contentBounds, QRect (KisScratchPad::*)() const);
    ASSERT_SCRATCH_PAD_SIGNATURE(wheelDelta, void (KisScratchPad::*)(QWheelEvent *));
    ASSERT_SCRATCH_PAD_SIGNATURE(pointerPress, void (KisScratchPad::*)(KoPointerEvent *));
    ASSERT_SCRATCH_PAD_SIGNATURE(pointerRelease, void (KisScratchPad::*)(KoPointerEvent *));
    ASSERT_SCRATCH_PAD_SIGNATURE(pointerMove, void (KisScratchPad::*)(KoPointerEvent *));
}

void KisScratchPadSchemaContractTest::basicFillSignaturesRemainStable()
{
    ASSERT_SCRATCH_PAD_SIGNATURE(fillDefault, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(fillPattern, void (KisScratchPad::*)(QTransform));
    ASSERT_SCRATCH_PAD_SIGNATURE(fillBackground, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(fillForeground, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(fillTransparent, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(setFillColor, void (KisScratchPad::*)(QColor));
    ASSERT_SCRATCH_PAD_SIGNATURE(fillLayer, void (KisScratchPad::*)(bool));
}

void KisScratchPadSchemaContractTest::gradientAndDocumentFillSignaturesRemainStable()
{
    using GradientSignature = void (KisScratchPad::*)(const QPoint &,
                                                      const QPoint &,
                                                      KisGradientPainter::enumGradientShape,
                                                      KisGradientPainter::enumGradientRepeat,
                                                      bool,
                                                      bool);
    ASSERT_SCRATCH_PAD_SIGNATURE(fillGradient, GradientSignature);
    ASSERT_SCRATCH_PAD_SIGNATURE(fillGradient, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(fillDocument, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(fillDocument, void (KisScratchPad::*)(bool));
}

void KisScratchPadSchemaContractTest::imageTransferSignaturesRemainStable()
{
    ASSERT_SCRATCH_PAD_SIGNATURE(setPresetImage, void (KisScratchPad::*)(const QImage &));
    ASSERT_SCRATCH_PAD_SIGNATURE(paintPresetImage, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(paintCustomImage, void (KisScratchPad::*)(const QImage &));
    ASSERT_SCRATCH_PAD_SIGNATURE(loadScratchpadImage, void (KisScratchPad::*)(QImage));
    ASSERT_SCRATCH_PAD_SIGNATURE(copyScratchpadImageData, QImage (KisScratchPad::*)());
}

void KisScratchPadSchemaContractTest::notificationSignaturesRemainStable()
{
    ASSERT_SCRATCH_PAD_SIGNATURE(scaleChanged, void (KisScratchPad::*)(qreal));
    ASSERT_SCRATCH_PAD_SIGNATURE(contentChanged, void (KisScratchPad::*)());
    ASSERT_SCRATCH_PAD_SIGNATURE(viewportChanged, void (KisScratchPad::*)(QRect));
    ASSERT_SCRATCH_PAD_SIGNATURE(colorSelected, void (KisScratchPad::*)(const KoColor &));
    ASSERT_SCRATCH_PAD_SIGNATURE(sigUpdateCanvas, void (KisScratchPad::*)(const QRect &));
}

#undef ASSERT_SCRATCH_PAD_SIGNATURE

QTEST_GUILESS_MAIN(KisScratchPadSchemaContractTest)

#include "KisScratchPadSchemaContractTest.moc"

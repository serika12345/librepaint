/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <lazybrush/kis_colorize_mask.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisColorizeMaskSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorizeMaskTypeLifetimeAndKeyStrokeColorsSchemaRemainStable();
    void colorizeMaskHierarchyAndPresentationSignaturesRemainStable();
    void colorizeMaskProjectionAndProcessingSignaturesRemainStable();
    void colorizeMaskOptionQuerySignaturesRemainStable();
    void colorizeMaskMutationAndNotificationSignaturesRemainStable();
};

void KisColorizeMaskSchemaContractTest::colorizeMaskTypeLifetimeAndKeyStrokeColorsSchemaRemainStable()
{
    using Mask = KisColorizeMask;
    using KeyStrokeColors = Mask::KeyStrokeColors;

    static_assert(std::is_class_v<Mask>);
    static_assert(std::is_class_v<KeyStrokeColors>);
    static_assert(std::is_same_v<decltype(KeyStrokeColors::colors), QVector<KoColor>>);
    static_assert(std::is_same_v<decltype(KeyStrokeColors::transparentIndex), int>);
    static_assert(std::is_constructible_v<Mask, KisImageWSP, const QString &>);
    static_assert(std::is_copy_constructible_v<Mask>);
    static_assert(std::is_destructible_v<Mask>);
}

void KisColorizeMaskSchemaContractTest::colorizeMaskHierarchyAndPresentationSignaturesRemainStable()
{
    using Mask = KisColorizeMask;
    using PropertyList = KisBaseNode::PropertyList;

    static_assert(std::is_same_v<decltype(static_cast<bool (Mask::*)(KisNodeVisitor &)>(&Mask::accept)),
                                 bool (Mask::*)(KisNodeVisitor &)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (Mask::*)(KisProcessingVisitor &, KisUndoAdapter *)>(&Mask::accept)),
                       void (Mask::*)(KisProcessingVisitor &, KisUndoAdapter *)>);
    static_assert(std::is_same_v<decltype(&Mask::clone), KisNodeSP (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::icon), QIcon (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::sectionModelProperties), PropertyList (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::setSectionModelProperties), void (Mask::*)(const PropertyList &)>);
    static_assert(std::is_same_v<decltype(&Mask::supportsNonIndirectPainting), bool (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::allPaintDevices), QVector<KisPaintDeviceSP> (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::getLodCapableDevices), KisPaintDeviceList (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::paintDevice), KisPaintDeviceSP (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::coloringProjection), KisPaintDeviceSP (Mask::*)() const>);
}

void KisColorizeMaskSchemaContractTest::colorizeMaskProjectionAndProcessingSignaturesRemainStable()
{
    using Mask = KisColorizeMask;
    using Intent = KoColorConversionTransformation::Intent;
    using ConversionFlags = KoColorConversionTransformation::ConversionFlags;

    static_assert(std::is_same_v<decltype(&Mask::colorSampleSourceDevice), KisPaintDeviceSP (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::colorSpace), const KoColorSpace *(Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::decorateRect),
                                 QRect (Mask::*)(KisPaintDeviceSP &,
                                                 KisPaintDeviceSP &,
                                                 const QRect &,
                                                 KisNode::PositionToFilthy,
                                                 KisRenderPassFlags) const>);
    static_assert(std::is_same_v<decltype(&Mask::exactBounds), QRect (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::extent), QRect (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::nonDependentExtent), QRect (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::initializeCompositeOp), void (Mask::*)()>);
    static_assert(std::is_same_v<decltype(&Mask::regeneratePrefilteredDeviceIfNeeded), void (Mask::*)()>);
    static_assert(std::is_same_v<decltype(&Mask::forceRegenerateMask), void (Mask::*)()>);
    static_assert(std::is_same_v<decltype(&Mask::resetCache), void (Mask::*)()>);
    static_assert(std::is_same_v<decltype(&Mask::mergeToLayerThreaded),
                                 void (Mask::*)(KisNodeSP,
                                                KUndo2Command *,
                                                const KUndo2MagicString &,
                                                int,
                                                QVector<KisRunnableStrokeJobData *> *)>);
    static_assert(
        std::is_same_v<decltype(&Mask::writeMergeData), void (Mask::*)(KisPainter *, KisPaintDeviceSP, const QRect &)>);
    static_assert(std::is_same_v<decltype(&Mask::testingFilteredSource), KisPaintDeviceSP (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::setColorSpace),
                                 KUndo2Command *(Mask::*)(const KoColorSpace *, Intent, ConversionFlags, KoUpdater *)>);
    static_assert(std::is_same_v<decltype(std::declval<Mask &>().setColorSpace(std::declval<const KoColorSpace *>())),
                                 KUndo2Command *>);
    static_assert(std::is_same_v<decltype(&Mask::setProfile), void (Mask::*)(const KoColorProfile *, KUndo2Command *)>);
}

void KisColorizeMaskSchemaContractTest::colorizeMaskOptionQuerySignaturesRemainStable()
{
    using Mask = KisColorizeMask;

    static_assert(std::is_same_v<decltype(&Mask::cleanUpAmount), qreal (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::edgeDetectionSize), qreal (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::fuzzyRadius), qreal (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::limitToDeviceBounds), bool (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::useEdgeDetection), bool (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::x), qint32 (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::y), qint32 (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::keyStrokesColors), Mask::KeyStrokeColors (Mask::*)() const>);
}

void KisColorizeMaskSchemaContractTest::colorizeMaskMutationAndNotificationSignaturesRemainStable()
{
    using Mask = KisColorizeMask;
    using KeyStrokes = QList<KisLazyFillTools::KeyStroke>;

    static_assert(std::is_same_v<decltype(&Mask::fetchKeyStrokesDirect), KeyStrokes (Mask::*)() const>);
    static_assert(std::is_same_v<decltype(&Mask::setCleanUpAmount), void (Mask::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&Mask::setCurrentColor), void (Mask::*)(const KoColor &)>);
    static_assert(std::is_same_v<decltype(&Mask::setEdgeDetectionSize), void (Mask::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&Mask::setFuzzyRadius), void (Mask::*)(qreal)>);
    static_assert(std::is_same_v<decltype(&Mask::setImage), void (Mask::*)(KisImageWSP)>);
    static_assert(std::is_same_v<decltype(&Mask::setKeyStrokesColors), void (Mask::*)(Mask::KeyStrokeColors)>);
    static_assert(std::is_same_v<decltype(&Mask::setKeyStrokesDirect), void (Mask::*)(const KeyStrokes &)>);
    static_assert(std::is_same_v<decltype(&Mask::setLimitToDeviceBounds), void (Mask::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Mask::setUseEdgeDetection), void (Mask::*)(bool)>);
    static_assert(std::is_same_v<decltype(&Mask::setX), void (Mask::*)(qint32)>);
    static_assert(std::is_same_v<decltype(&Mask::setY), void (Mask::*)(qint32)>);
    static_assert(std::is_same_v<decltype(&Mask::removeKeyStroke), void (Mask::*)(const KoColor &)>);
    static_assert(
        std::is_same_v<decltype(&Mask::testingAddKeyStroke), void (Mask::*)(KisPaintDeviceSP, const KoColor &, bool)>);
    static_assert(std::is_same_v<decltype(std::declval<Mask &>().testingAddKeyStroke(std::declval<KisPaintDeviceSP>(),
                                                                                     std::declval<const KoColor &>())),
                                 void>);
    static_assert(std::is_same_v<decltype(&Mask::sigKeyStrokesListChanged), void (Mask::*)()>);
    static_assert(std::is_same_v<decltype(&Mask::sigUpdateOnDirtyParent), void (Mask::*)() const>);
}

QTEST_GUILESS_MAIN(KisColorizeMaskSchemaContractTest)

#include "KisColorizeMaskSchemaContractTest.moc"

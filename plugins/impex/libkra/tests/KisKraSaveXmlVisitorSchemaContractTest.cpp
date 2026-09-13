/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_colorize_dom_utils.h"
#include "kis_kra_load_visitor.h"
#include "kis_kra_loader.h"
#include "kis_kra_save_visitor.h"
#include "kis_kra_savexml_visitor.h"

#include <QTest>

#include <type_traits>

class KisKraSaveXmlVisitorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void saveXmlVisitorTypeConstructionAndSelectionSchemaRemainStable();
    void saveXmlVisitorNodeAndLayerVisitSignaturesRemainStable();
    void saveXmlVisitorMaskVisitSignaturesRemainStable();
    void saveXmlVisitorFileNameAndErrorSignaturesRemainStable();
    void saveXmlVisitorPaintLayerAttributeSignaturesRemainStable();
    void kraBinaryVisitorTypeConstructionAndLifetimeSchemaRemainStable();
    void kraLoadVisitorNodeAndLayerVisitSignaturesRemainStable();
    void kraLoadVisitorMaskStateAndProfileSignaturesRemainStable();
    void kraSaveVisitorNodeAndLayerVisitSignaturesRemainStable();
    void kraSaveVisitorMaskAndStateSignaturesRemainStable();
    void kraLoaderTypeConstructionAndLifetimeSchemaRemainStable();
    void kraLoaderImageDataSignaturesRemainStable();
    void kraLoaderDocumentAssetSignaturesRemainStable();
    void kraLoaderSelectionAssistantAndStoryboardSignaturesRemainStable();
    void kraLoaderDiagnosticSignaturesRemainStable();
    void colorizeDomSerializationSignaturesRemainStable();
};

void KisKraSaveXmlVisitorSchemaContractTest::saveXmlVisitorTypeConstructionAndSelectionSchemaRemainStable()
{
    using SetSelectedNodesSignature = void (KisSaveXmlVisitor::*)(vKisNodeSP);

    QVERIFY(std::is_class_v<KisSaveXmlVisitor>);
    QVERIFY((std::is_constructible_v<KisSaveXmlVisitor,
                                     QDomDocument,
                                     const QDomElement &,
                                     quint32 &,
                                     const QString &,
                                     bool>));
    QVERIFY((std::is_same_v<decltype(static_cast<SetSelectedNodesSignature>(&KisSaveXmlVisitor::setSelectedNodes)),
                            SetSelectedNodesSignature>));
}

void KisKraSaveXmlVisitorSchemaContractTest::saveXmlVisitorNodeAndLayerVisitSignaturesRemainStable()
{
    using AdjustmentLayerVisitSignature = bool (KisSaveXmlVisitor::*)(KisAdjustmentLayer *);
    using CloneLayerVisitSignature = bool (KisSaveXmlVisitor::*)(KisCloneLayer *);
    using ExternalLayerVisitSignature = bool (KisSaveXmlVisitor::*)(KisExternalLayer *);
    using GeneratorLayerVisitSignature = bool (KisSaveXmlVisitor::*)(KisGeneratorLayer *);
    using GroupLayerVisitSignature = bool (KisSaveXmlVisitor::*)(KisGroupLayer *);
    using NodeVisitSignature = bool (KisSaveXmlVisitor::*)(KisNode *);
    using PaintLayerVisitSignature = bool (KisSaveXmlVisitor::*)(KisPaintLayer *);

    QVERIFY((std::is_same_v<decltype(static_cast<AdjustmentLayerVisitSignature>(&KisSaveXmlVisitor::visit)),
                            AdjustmentLayerVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<CloneLayerVisitSignature>(&KisSaveXmlVisitor::visit)),
                            CloneLayerVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<ExternalLayerVisitSignature>(&KisSaveXmlVisitor::visit)),
                            ExternalLayerVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<GeneratorLayerVisitSignature>(&KisSaveXmlVisitor::visit)),
                            GeneratorLayerVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<GroupLayerVisitSignature>(&KisSaveXmlVisitor::visit)),
                            GroupLayerVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<NodeVisitSignature>(&KisSaveXmlVisitor::visit)), NodeVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<PaintLayerVisitSignature>(&KisSaveXmlVisitor::visit)),
                            PaintLayerVisitSignature>));
}

void KisKraSaveXmlVisitorSchemaContractTest::saveXmlVisitorMaskVisitSignaturesRemainStable()
{
    using ColorizeMaskVisitSignature = bool (KisSaveXmlVisitor::*)(KisColorizeMask *);
    using FilterMaskVisitSignature = bool (KisSaveXmlVisitor::*)(KisFilterMask *);
    using SelectionMaskVisitSignature = bool (KisSaveXmlVisitor::*)(KisSelectionMask *);
    using TransformMaskVisitSignature = bool (KisSaveXmlVisitor::*)(KisTransformMask *);
    using TransparencyMaskVisitSignature = bool (KisSaveXmlVisitor::*)(KisTransparencyMask *);

    QVERIFY((std::is_same_v<decltype(static_cast<ColorizeMaskVisitSignature>(&KisSaveXmlVisitor::visit)),
                            ColorizeMaskVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<FilterMaskVisitSignature>(&KisSaveXmlVisitor::visit)),
                            FilterMaskVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<SelectionMaskVisitSignature>(&KisSaveXmlVisitor::visit)),
                            SelectionMaskVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<TransformMaskVisitSignature>(&KisSaveXmlVisitor::visit)),
                            TransformMaskVisitSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<TransparencyMaskVisitSignature>(&KisSaveXmlVisitor::visit)),
                            TransparencyMaskVisitSignature>));
}

void KisKraSaveXmlVisitorSchemaContractTest::saveXmlVisitorFileNameAndErrorSignaturesRemainStable()
{
    using ErrorMessagesSignature = QStringList (KisSaveXmlVisitor::*)() const;
    using FileNamesSignature = QMap<const KisNode *, QString> (KisSaveXmlVisitor::*)();

    QVERIFY((std::is_same_v<decltype(static_cast<ErrorMessagesSignature>(&KisSaveXmlVisitor::errorMessages)),
                            ErrorMessagesSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<FileNamesSignature>(&KisSaveXmlVisitor::keyframeFileNames)),
                            FileNamesSignature>));
    QVERIFY((std::is_same_v<decltype(static_cast<FileNamesSignature>(&KisSaveXmlVisitor::nodeFileNames)),
                            FileNamesSignature>));
}

void KisKraSaveXmlVisitorSchemaContractTest::saveXmlVisitorPaintLayerAttributeSignaturesRemainStable()
{
    using LoadAttributesSignature = void (*)(const QDomElement &, KisPaintLayer *, bool);
    using SaveAttributesSignature = QDomElement (KisSaveXmlVisitor::*)(KisPaintLayer *, QDomDocument &, bool);

    QVERIFY(
        (std::is_same_v<decltype(static_cast<LoadAttributesSignature>(&KisSaveXmlVisitor::loadPaintLayerAttributes)),
                        LoadAttributesSignature>));
    QVERIFY(
        (std::is_same_v<decltype(static_cast<SaveAttributesSignature>(&KisSaveXmlVisitor::savePaintLayerAttributes)),
                        SaveAttributesSignature>));
}

void KisKraSaveXmlVisitorSchemaContractTest::kraBinaryVisitorTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisKraLoadVisitor>);
    static_assert(std::is_base_of_v<KisNodeVisitor, KisKraLoadVisitor>);
    static_assert(std::is_constructible_v<KisKraLoadVisitor,
                                          KisImageSP,
                                          KoStore *,
                                          KoShapeControllerBase *,
                                          QMap<KisNode *, QString> &,
                                          QMap<KisNode *, QString> &,
                                          const QString &,
                                          int>);
    static_assert(std::is_class_v<KisKraSaveVisitor>);
    static_assert(std::is_base_of_v<KisNodeVisitor, KisKraSaveVisitor>);
    static_assert(
        std::is_constructible_v<KisKraSaveVisitor, KoStore *, const QString &, QMap<const KisNode *, QString>>);
    static_assert(std::has_virtual_destructor_v<KisKraSaveVisitor>);
}

void KisKraSaveXmlVisitorSchemaContractTest::kraLoadVisitorNodeAndLayerVisitSignaturesRemainStable()
{
    using Visitor = KisKraLoadVisitor;

    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisNode *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisNode *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisExternalLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisExternalLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisPaintLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisPaintLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisGroupLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisGroupLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisAdjustmentLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisAdjustmentLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisGeneratorLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisGeneratorLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisCloneLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisCloneLayer *)>);
}

void KisKraSaveXmlVisitorSchemaContractTest::kraLoadVisitorMaskStateAndProfileSignaturesRemainStable()
{
    using Visitor = KisKraLoadVisitor;

    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisFilterMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisFilterMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisTransformMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisTransformMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisTransparencyMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisTransparencyMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisSelectionMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisSelectionMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisColorizeMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisColorizeMask *)>);
    static_assert(std::is_same_v<decltype(&Visitor::setExternalUri), void (Visitor::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Visitor::errorMessages), QStringList (Visitor::*)() const>);
    static_assert(std::is_same_v<decltype(&Visitor::warningMessages), QStringList (Visitor::*)() const>);
    static_assert(std::is_same_v<decltype(&Visitor::customProfileNameAliasForKra), QHash<QString, QString> (*)()>);
}

void KisKraSaveXmlVisitorSchemaContractTest::kraSaveVisitorNodeAndLayerVisitSignaturesRemainStable()
{
    using Visitor = KisKraSaveVisitor;

    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisNode *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisNode *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisExternalLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisExternalLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisPaintLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisPaintLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisGroupLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisGroupLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisAdjustmentLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisAdjustmentLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisGeneratorLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisGeneratorLayer *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisCloneLayer *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisCloneLayer *)>);
}

void KisKraSaveXmlVisitorSchemaContractTest::kraSaveVisitorMaskAndStateSignaturesRemainStable()
{
    using Visitor = KisKraSaveVisitor;

    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisFilterMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisFilterMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisTransformMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisTransformMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisTransparencyMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisTransparencyMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisSelectionMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisSelectionMask *)>);
    static_assert(std::is_same_v<decltype(static_cast<bool (Visitor::*)(KisColorizeMask *)>(&Visitor::visit)),
                                 bool (Visitor::*)(KisColorizeMask *)>);
    static_assert(std::is_same_v<decltype(&Visitor::setExternalUri), void (Visitor::*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&Visitor::errorMessages), QStringList (Visitor::*)() const>);
}

void KisKraSaveXmlVisitorSchemaContractTest::kraLoaderTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<KisKraLoader>);
    static_assert(std::is_constructible_v<KisKraLoader, KisDocument *, int, const QVersionNumber &>);
    static_assert(std::is_destructible_v<KisKraLoader>);
}

void KisKraSaveXmlVisitorSchemaContractTest::kraLoaderImageDataSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisKraLoader::loadXML), KisImageSP (KisKraLoader::*)(const QDomElement &)>);
    static_assert(std::is_same_v<decltype(&KisKraLoader::loadBinaryData),
                                 void (KisKraLoader::*)(KoStore *, KisImageSP, const QString &, bool)>);
}

void KisKraSaveXmlVisitorSchemaContractTest::kraLoaderDocumentAssetSignaturesRemainStable()
{
    static_assert(
        std::is_same_v<decltype(&KisKraLoader::loadResources), void (KisKraLoader::*)(KoStore *, KisDocument *)>);
    static_assert(
        std::is_same_v<decltype(&KisKraLoader::loadStoryboards), void (KisKraLoader::*)(KoStore *, KisDocument *)>);
    static_assert(
        std::is_same_v<decltype(&KisKraLoader::loadAnimationMetadata), void (KisKraLoader::*)(KoStore *, KisImageSP)>);
    static_assert(std::is_same_v<decltype(&KisKraLoader::loadAudio), void (KisKraLoader::*)(KoStore *, KisDocument *)>);

    // The deprecated entry point remains a public compatibility surface and is intentionally referenced here.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
    static_assert(std::is_same_v<decltype(&KisKraLoader::backCompat_loadAudio),
                                 void (KisKraLoader::*)(const QDomElement &, KisDocument *)>);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}

void KisKraSaveXmlVisitorSchemaContractTest::kraLoaderSelectionAssistantAndStoryboardSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisKraLoader::selectedNodes), vKisNodeSP (KisKraLoader::*)() const>);
    static_assert(
        std::is_same_v<decltype(&KisKraLoader::assistants), QList<KisPaintingAssistantSP> (KisKraLoader::*)() const>);
    static_assert(
        std::is_same_v<decltype(&KisKraLoader::storyboardItemList), StoryboardItemList (KisKraLoader::*)() const>);
    static_assert(std::is_same_v<decltype(&KisKraLoader::storyboardCommentList),
                                 StoryboardCommentList (KisKraLoader::*)() const>);
}

void KisKraSaveXmlVisitorSchemaContractTest::kraLoaderDiagnosticSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisKraLoader::errorMessages), QStringList (KisKraLoader::*)() const>);
    static_assert(std::is_same_v<decltype(&KisKraLoader::warningMessages), QStringList (KisKraLoader::*)() const>);
    static_assert(std::is_same_v<decltype(&KisKraLoader::imageName), QString (KisKraLoader::*)() const>);
}

void KisKraSaveXmlVisitorSchemaContractTest::colorizeDomSerializationSignaturesRemainStable()
{
    using SaveValue = void (*)(QDomElement *, const QString &, const KisLazyFillTools::KeyStroke &);
    using LoadValue =
        bool (*)(const QDomElement &, KisLazyFillTools::KeyStroke *, const KoColorSpace *, const QPoint &);

    static_assert(std::is_same_v<decltype(&KisDomUtils::saveValue), SaveValue>);
    static_assert(std::is_same_v<decltype(&KisDomUtils::loadValue), LoadValue>);

    QVERIFY(true);
}

QTEST_GUILESS_MAIN(KisKraSaveXmlVisitorSchemaContractTest)

#include "KisKraSaveXmlVisitorSchemaContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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

QTEST_GUILESS_MAIN(KisKraSaveXmlVisitorSchemaContractTest)

#include "KisKraSaveXmlVisitorSchemaContractTest.moc"

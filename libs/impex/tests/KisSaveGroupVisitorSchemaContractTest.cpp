/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../ui/KisSaveGroupVisitor.h"

#include <type_traits>

#include <QTest>

namespace
{

#define ASSERT_VISIT_SIGNATURE(nodeType)                                                                               \
    static_assert(                                                                                                     \
        std::is_same_v<decltype(static_cast<bool (KisSaveGroupVisitor::*)(nodeType *)>(&KisSaveGroupVisitor::visit)),  \
                       bool (KisSaveGroupVisitor::*)(nodeType *)>)

} // namespace

class KisSaveGroupVisitorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndLifetimeSchemaRemainStable();
    void baseAndConcreteLayerVisitSignaturesRemainStable();
    void generatedLayerVisitSignaturesRemainStable();
    void maskVisitSignaturesRemainStable();
};

void KisSaveGroupVisitorSchemaContractTest::typeConstructionAndLifetimeSchemaRemainStable()
{
    using Visitor = KisSaveGroupVisitor;

    static_assert(std::is_class_v<Visitor>);
    static_assert(std::is_base_of_v<KisNodeVisitor, Visitor>);
    static_assert(std::is_constructible_v<Visitor,
                                          KisImageWSP,
                                          bool,
                                          bool,
                                          const QString &,
                                          const QString &,
                                          const QString &,
                                          const QString &>);
    static_assert(std::has_virtual_destructor_v<Visitor>);

    QVERIFY(true);
}

void KisSaveGroupVisitorSchemaContractTest::baseAndConcreteLayerVisitSignaturesRemainStable()
{
    ASSERT_VISIT_SIGNATURE(KisNode);
    ASSERT_VISIT_SIGNATURE(KisPaintLayer);
    ASSERT_VISIT_SIGNATURE(KisGroupLayer);
    ASSERT_VISIT_SIGNATURE(KisExternalLayer);
}

void KisSaveGroupVisitorSchemaContractTest::generatedLayerVisitSignaturesRemainStable()
{
    ASSERT_VISIT_SIGNATURE(KisAdjustmentLayer);
    ASSERT_VISIT_SIGNATURE(KisGeneratorLayer);
    ASSERT_VISIT_SIGNATURE(KisCloneLayer);
}

void KisSaveGroupVisitorSchemaContractTest::maskVisitSignaturesRemainStable()
{
    ASSERT_VISIT_SIGNATURE(KisFilterMask);
    ASSERT_VISIT_SIGNATURE(KisTransformMask);
    ASSERT_VISIT_SIGNATURE(KisTransparencyMask);
    ASSERT_VISIT_SIGNATURE(KisSelectionMask);
    ASSERT_VISIT_SIGNATURE(KisColorizeMask);
}

#undef ASSERT_VISIT_SIGNATURE

QTEST_APPLESS_MAIN(KisSaveGroupVisitorSchemaContractTest)

#include "KisSaveGroupVisitorSchemaContractTest.moc"

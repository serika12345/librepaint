/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_count_visitor.h"

#include <QTest>

#include <type_traits>

#define ASSERT_COUNT_VISITOR_SIGNATURE(member, signature)                                                              \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisCountVisitor::member)), signature>)

class KisCountVisitorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstructionAndCountSchemaRemainStable();
    void nodeAndStandardLayerVisitSignaturesRemainStable();
    void generatedExternalAndCloneLayerVisitSignaturesRemainStable();
    void filterTransformAndTransparencyMaskVisitSignaturesRemainStable();
    void selectionAndColorizeMaskVisitSignaturesRemainStable();
};

void KisCountVisitorSchemaContractTest::typeConstructionAndCountSchemaRemainStable()
{
    static_assert(std::is_class_v<KisCountVisitor>);
    static_assert(std::is_base_of_v<KisNodeVisitor, KisCountVisitor>);
    static_assert(std::is_constructible_v<KisCountVisitor, const QStringList &, const KoProperties &>);
    ASSERT_COUNT_VISITOR_SIGNATURE(count, quint32 (KisCountVisitor::*)());
}

void KisCountVisitorSchemaContractTest::nodeAndStandardLayerVisitSignaturesRemainStable()
{
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisNode *));
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisPaintLayer *));
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisGroupLayer *));
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisAdjustmentLayer *));
}

void KisCountVisitorSchemaContractTest::generatedExternalAndCloneLayerVisitSignaturesRemainStable()
{
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisGeneratorLayer *));
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisExternalLayer *));
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisCloneLayer *));
}

void KisCountVisitorSchemaContractTest::filterTransformAndTransparencyMaskVisitSignaturesRemainStable()
{
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisFilterMask *));
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisTransformMask *));
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisTransparencyMask *));
}

void KisCountVisitorSchemaContractTest::selectionAndColorizeMaskVisitSignaturesRemainStable()
{
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisSelectionMask *));
    ASSERT_COUNT_VISITOR_SIGNATURE(visit, bool (KisCountVisitor::*)(KisColorizeMask *));
}

QTEST_APPLESS_MAIN(KisCountVisitorSchemaContractTest)

#include "KisCountVisitorSchemaContractTest.moc"

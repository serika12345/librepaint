/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_exif_info_visitor.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_EXIF_VISITOR_SIGNATURE(method, signature)                                                               \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisExifInfoVisitor::method)), signature>)
} // namespace

class KisExifInfoVisitorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeAndMetadataAccessSchemaRemainStable();
    void nodeAndStandardLayerVisitSignaturesRemainStable();
    void specializedLayerVisitSignaturesRemainStable();
    void maskVisitSignaturesRemainStable();
};

void KisExifInfoVisitorSchemaContractTest::typeAndMetadataAccessSchemaRemainStable()
{
    using Visitor = KisExifInfoVisitor;

    static_assert(std::is_class_v<Visitor>);
    static_assert(std::is_base_of_v<KisNodeVisitor, Visitor>);
    static_assert(std::is_default_constructible_v<Visitor>);
    ASSERT_EXIF_VISITOR_SIGNATURE(metaDataCount, uint (Visitor::*)());
    ASSERT_EXIF_VISITOR_SIGNATURE(exifInfo, KisMetaData::Store * (Visitor::*)());

    QVERIFY(true);
}

void KisExifInfoVisitorSchemaContractTest::nodeAndStandardLayerVisitSignaturesRemainStable()
{
    using Visitor = KisExifInfoVisitor;

    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisNode *));
    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisPaintLayer *));
    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisGroupLayer *));
}

void KisExifInfoVisitorSchemaContractTest::specializedLayerVisitSignaturesRemainStable()
{
    using Visitor = KisExifInfoVisitor;

    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisAdjustmentLayer *));
    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisCloneLayer *));
    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisExternalLayer *));
    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisGeneratorLayer *));
}

void KisExifInfoVisitorSchemaContractTest::maskVisitSignaturesRemainStable()
{
    using Visitor = KisExifInfoVisitor;

    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisColorizeMask *));
    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisFilterMask *));
    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisSelectionMask *));
    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisTransformMask *));
    ASSERT_EXIF_VISITOR_SIGNATURE(visit, bool (Visitor::*)(KisTransparencyMask *));
}

#undef ASSERT_EXIF_VISITOR_SIGNATURE

QTEST_GUILESS_MAIN(KisExifInfoVisitorSchemaContractTest)

#include "KisExifInfoVisitorSchemaContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <psd_layer_record.h>
#include <psd_layer_section.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PSD_LAYER_RECORD_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&PSDLayerRecord::method)), signature>)
#define ASSERT_PSD_LAYER_SECTION_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&PSDLayerMaskSection::method)), signature>)
} // namespace

class PSDLayerRecordSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void layerRecordTypeLifetimeAndValiditySchemaRemainStable();
    void layerRecordSerializedValueMemberSchemaRemainsStable();
    void layerRecordReadSignaturesRemainStable();
    void layerRecordPathAndWriteSignaturesRemainStable();
    void layerRecordDiagnosticOutputSignaturesRemainStable();
    void layerMaskSectionTypeLifetimeSchemaRemainsStable();
    void layerMaskSectionGlobalMaskValueSchemaRemainsStable();
    void layerMaskSectionStateMemberSchemaRemainsStable();
    void layerMaskSectionIoSignaturesRemainStable();
};

void PSDLayerRecordSchemaContractTest::layerRecordTypeLifetimeAndValiditySchemaRemainStable()
{
    static_assert(std::is_class_v<PSDLayerRecord>);
    static_assert(std::is_constructible_v<PSDLayerRecord, const PSDHeader &>);
    static_assert(!std::is_default_constructible_v<PSDLayerRecord>);
    static_assert(std::is_destructible_v<PSDLayerRecord>);
    ASSERT_PSD_LAYER_RECORD_SIGNATURE(valid, bool (PSDLayerRecord::*)());
}

void PSDLayerRecordSchemaContractTest::layerRecordSerializedValueMemberSchemaRemainsStable()
{
    static_assert(
        std::is_same_v<decltype(&PSDLayerRecord::channelInfoRecords), QVector<ChannelInfo *> PSDLayerRecord::*>);
    static_assert(std::is_same_v<decltype(&PSDLayerRecord::error), QString PSDLayerRecord::*>);
    static_assert(std::is_same_v<decltype(&PSDLayerRecord::fillConfig), QDomDocument PSDLayerRecord::*>);
    static_assert(std::is_same_v<decltype(&PSDLayerRecord::infoBlocks), PsdAdditionalLayerInfoBlock PSDLayerRecord::*>);
    static_assert(std::is_same_v<decltype(&PSDLayerRecord::textShape), psd_layer_type_shape PSDLayerRecord::*>);
    static_assert(std::is_same_v<decltype(&PSDLayerRecord::vectorMask), psd_vector_mask PSDLayerRecord::*>);
    static_assert(std::is_same_v<decltype(&PSDLayerRecord::vectorOriginationData), QDomDocument PSDLayerRecord::*>);
    static_assert(std::is_same_v<decltype(&PSDLayerRecord::vectorStroke), QDomDocument PSDLayerRecord::*>);
}

void PSDLayerRecordSchemaContractTest::layerRecordReadSignaturesRemainStable()
{
    ASSERT_PSD_LAYER_RECORD_SIGNATURE(channelRect, QRect (PSDLayerRecord::*)(ChannelInfo *) const);
    ASSERT_PSD_LAYER_RECORD_SIGNATURE(read, bool (PSDLayerRecord::*)(QIODevice &));
    ASSERT_PSD_LAYER_RECORD_SIGNATURE(readMask, bool (PSDLayerRecord::*)(QIODevice &, KisPaintDeviceSP, ChannelInfo *));
    ASSERT_PSD_LAYER_RECORD_SIGNATURE(readPixelData, bool (PSDLayerRecord::*)(QIODevice &, KisPaintDeviceSP));
}

void PSDLayerRecordSchemaContractTest::layerRecordPathAndWriteSignaturesRemainStable()
{
    ASSERT_PSD_LAYER_RECORD_SIGNATURE(addPathShapeToPSDPath,
                                      void (PSDLayerRecord::*)(psd_path &, KoPathShape *, double, double));
    ASSERT_PSD_LAYER_RECORD_SIGNATURE(constructPathShape, KoPathShape * (PSDLayerRecord::*)(psd_path, double, double));
    ASSERT_PSD_LAYER_RECORD_SIGNATURE(write,
                                      void (PSDLayerRecord::*)(QIODevice &,
                                                               KisPaintDeviceSP,
                                                               KisNodeSP,
                                                               const QRect &,
                                                               psd_section_type,
                                                               const QDomDocument &,
                                                               bool));
    ASSERT_PSD_LAYER_RECORD_SIGNATURE(writePixelData, void (PSDLayerRecord::*)(QIODevice &, psd_compression_type));
}

void PSDLayerRecordSchemaContractTest::layerRecordDiagnosticOutputSignaturesRemainStable()
{
    using ChannelInfoDebug = QDebug (*)(QDebug, const ChannelInfo &);
    using LayerBlendingRange = PSDLayerRecord::LayerBlendingRanges::LayerBlendingRange;
    using LayerBlendingRangeDebug = QDebug (*)(QDebug, const LayerBlendingRange &);
    using LayerRecordDebug = QDebug (*)(QDebug, const PSDLayerRecord &);

    static_assert(std::is_same_v<decltype(static_cast<ChannelInfoDebug>(&operator<<)), ChannelInfoDebug>);
    static_assert(std::is_same_v<decltype(static_cast<LayerRecordDebug>(&operator<<)), LayerRecordDebug>);
    static_assert(std::is_same_v<decltype(static_cast<LayerBlendingRangeDebug>(&operator<<)), LayerBlendingRangeDebug>);
}

void PSDLayerRecordSchemaContractTest::layerMaskSectionTypeLifetimeSchemaRemainsStable()
{
    static_assert(std::is_class_v<PSDLayerMaskSection>);
    static_assert(std::is_class_v<PSDLayerMaskSection::GlobalLayerMaskInfo>);
    static_assert(std::is_constructible_v<PSDLayerMaskSection, const PSDHeader &>);
    static_assert(std::is_destructible_v<PSDLayerMaskSection>);
}

void PSDLayerRecordSchemaContractTest::layerMaskSectionGlobalMaskValueSchemaRemainsStable()
{
    using MaskInfo = PSDLayerMaskSection::GlobalLayerMaskInfo;

    static_assert(std::is_same_v<decltype(&MaskInfo::colorComponents), quint16(MaskInfo::*)[4]>);
    static_assert(std::is_same_v<decltype(&MaskInfo::kind), quint8 MaskInfo::*>);
    static_assert(std::is_same_v<decltype(&MaskInfo::opacity), quint16 MaskInfo::*>);
    static_assert(std::is_same_v<decltype(&MaskInfo::overlayColorSpace), quint16 MaskInfo::*>);
}

void PSDLayerRecordSchemaContractTest::layerMaskSectionStateMemberSchemaRemainsStable()
{
    using Section = PSDLayerMaskSection;

    static_assert(std::is_same_v<decltype(&Section::error), QString Section::*>);
    static_assert(std::is_same_v<decltype(&Section::globalInfoSection), PsdAdditionalLayerInfoBlock Section::*>);
    static_assert(std::is_same_v<decltype(&Section::globalLayerMaskInfo), Section::GlobalLayerMaskInfo Section::*>);
    static_assert(std::is_same_v<decltype(&Section::hasTransparency), bool Section::*>);
    static_assert(std::is_same_v<decltype(&Section::layers), QVector<PSDLayerRecord *> Section::*>);
    static_assert(std::is_same_v<decltype(&Section::nLayers), qint16 Section::*>);
}

void PSDLayerRecordSchemaContractTest::layerMaskSectionIoSignaturesRemainStable()
{
    ASSERT_PSD_LAYER_SECTION_SIGNATURE(read, bool (PSDLayerMaskSection::*)(QIODevice &));
    ASSERT_PSD_LAYER_SECTION_SIGNATURE(write,
                                       bool (PSDLayerMaskSection::*)(QIODevice &, KisNodeSP, psd_compression_type));
}

#undef ASSERT_PSD_LAYER_SECTION_SIGNATURE
#undef ASSERT_PSD_LAYER_RECORD_SIGNATURE

QTEST_APPLESS_MAIN(PSDLayerRecordSchemaContractTest)

#include "PSDLayerRecordSchemaContractTest.moc"

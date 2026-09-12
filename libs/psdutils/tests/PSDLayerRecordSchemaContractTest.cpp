/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <psd_additional_layer_info_block.h>
#include <psd_layer_record.h>
#include <psd_layer_section.h>
#include <psd_pixel_utils.h>
#include <psd_resource_block.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_PSD_LAYER_RECORD_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&PSDLayerRecord::method)), signature>)
#define ASSERT_PSD_LAYER_SECTION_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&PSDLayerMaskSection::method)), signature>)
#define ASSERT_PSD_RESOURCE_BLOCK_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&PSDResourceBlock::method)), signature>)
#define ASSERT_PSD_TEXT_SHAPE_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&psd_layer_type_shape::method)), signature>)
#define ASSERT_PSD_VECTOR_ORIGINATION_SIGNATURE(method, signature)                                                     \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&psd_vector_origination_data::method)), signature>)
#define ASSERT_PSD_VECTOR_STROKE_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&psd_vector_stroke_data::method)), signature>)
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
    void resourceBlockTypeConstructionAndLifetimeSchemaRemainStable();
    void resourceBlockSerializedValueMemberSchemaRemainsStable();
    void resourceBlockOwnershipAndCloneSchemaRemainStable();
    void resourceBlockDisplayAndValiditySignaturesRemainStable();
    void resourceBlockIoSignaturesRemainStable();
    void additionalLayerTextToolMemberSchemaRemainsStable();
    void additionalLayerTextShapeConfigurationSignaturesRemainStable();
    void additionalLayerTextShapeXmlSignaturesRemainStable();
    void additionalLayerVectorOriginationSignaturesRemainStable();
    void additionalLayerVectorStrokeSignaturesRemainStable();
    void pixelChannelWritingInfoAndIoSignaturesRemainStable();
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

void PSDLayerRecordSchemaContractTest::resourceBlockTypeConstructionAndLifetimeSchemaRemainStable()
{
    static_assert(std::is_class_v<PSDResourceBlock>);
    static_assert(std::is_default_constructible_v<PSDResourceBlock>);
    static_assert(std::is_destructible_v<PSDResourceBlock>);
    static_assert(std::has_virtual_destructor_v<PSDResourceBlock>);
}

void PSDLayerRecordSchemaContractTest::resourceBlockSerializedValueMemberSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&PSDResourceBlock::data), QByteArray PSDResourceBlock::*>);
    static_assert(std::is_same_v<decltype(&PSDResourceBlock::dataSize), quint32 PSDResourceBlock::*>);
    static_assert(std::is_same_v<decltype(&PSDResourceBlock::error), QString PSDResourceBlock::*>);
    static_assert(std::is_same_v<decltype(&PSDResourceBlock::identifier), quint16 PSDResourceBlock::*>);
    static_assert(std::is_same_v<decltype(&PSDResourceBlock::name), QString PSDResourceBlock::*>);
}

void PSDLayerRecordSchemaContractTest::resourceBlockOwnershipAndCloneSchemaRemainStable()
{
    static_assert(std::is_same_v<decltype(&PSDResourceBlock::resource), PSDInterpretedResource * PSDResourceBlock::*>);
    ASSERT_PSD_RESOURCE_BLOCK_SIGNATURE(clone, KisAnnotation * (PSDResourceBlock::*)() const);
}

void PSDLayerRecordSchemaContractTest::resourceBlockDisplayAndValiditySignaturesRemainStable()
{
    ASSERT_PSD_RESOURCE_BLOCK_SIGNATURE(displayText, QString (PSDResourceBlock::*)() const);
    ASSERT_PSD_RESOURCE_BLOCK_SIGNATURE(valid, bool (PSDResourceBlock::*)());
}

void PSDLayerRecordSchemaContractTest::resourceBlockIoSignaturesRemainStable()
{
    ASSERT_PSD_RESOURCE_BLOCK_SIGNATURE(read, bool (PSDResourceBlock::*)(QIODevice &));
    ASSERT_PSD_RESOURCE_BLOCK_SIGNATURE(write, bool (PSDResourceBlock::*)(QIODevice &) const);
}

void PSDLayerRecordSchemaContractTest::additionalLayerTextToolMemberSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(&psd_layer_type_face::vector), qint32 * psd_layer_type_face::*>);
    static_assert(std::is_same_v<decltype(&psd_layer_type_tool::face), psd_layer_type_face * psd_layer_type_tool::*>);
    static_assert(std::is_same_v<decltype(&psd_layer_type_tool::line), psd_layer_type_line * psd_layer_type_tool::*>);
    static_assert(std::is_same_v<decltype(&psd_layer_type_tool::style), psd_layer_type_style * psd_layer_type_tool::*>);
}

void PSDLayerRecordSchemaContractTest::additionalLayerTextShapeConfigurationSignaturesRemainStable()
{
    using SetupCatcher = void (*)(QString, KisAslCallbackObjectCatcher &, psd_layer_type_shape *);

    ASSERT_PSD_TEXT_SHAPE_SIGNATURE(setEngineData, void (psd_layer_type_shape::*)(QByteArray));
    ASSERT_PSD_TEXT_SHAPE_SIGNATURE(setupCatcher, SetupCatcher);
}

void PSDLayerRecordSchemaContractTest::additionalLayerTextShapeXmlSignaturesRemainStable()
{
    ASSERT_PSD_TEXT_SHAPE_SIGNATURE(textDataASLXML, QDomDocument (psd_layer_type_shape::*)());
    ASSERT_PSD_TEXT_SHAPE_SIGNATURE(textWarpXML, QDomDocument (psd_layer_type_shape::*)());
}

void PSDLayerRecordSchemaContractTest::additionalLayerVectorOriginationSignaturesRemainStable()
{
    using SetupCatcher = void (*)(QString, KisAslCallbackObjectCatcher &, psd_vector_origination_data *);

    ASSERT_PSD_VECTOR_ORIGINATION_SIGNATURE(getASL, QDomDocument (psd_vector_origination_data::*)());
    ASSERT_PSD_VECTOR_ORIGINATION_SIGNATURE(setupCatcher, SetupCatcher);
}

void PSDLayerRecordSchemaContractTest::additionalLayerVectorStrokeSignaturesRemainStable()
{
    using SetupCatcher = void (*)(QString, KisAslCallbackObjectCatcher &, psd_vector_stroke_data *);

    ASSERT_PSD_VECTOR_STROKE_SIGNATURE(getASLXML, QDomDocument (psd_vector_stroke_data::*)());
    ASSERT_PSD_VECTOR_STROKE_SIGNATURE(loadFromShapeStroke, void (psd_vector_stroke_data::*)(KoShapeStrokeSP));
    ASSERT_PSD_VECTOR_STROKE_SIGNATURE(setupCatcher, SetupCatcher);
    ASSERT_PSD_VECTOR_STROKE_SIGNATURE(setupShapeStroke, void (psd_vector_stroke_data::*)(KoShapeStrokeSP));
}

void PSDLayerRecordSchemaContractTest::pixelChannelWritingInfoAndIoSignaturesRemainStable()
{
    using Info = PsdPixelUtils::ChannelWritingInfo;
    using ReadChannels = void (*)(QIODevice &,
                                  KisPaintDeviceSP,
                                  psd_color_mode,
                                  int,
                                  const QRect &,
                                  QVector<ChannelInfo *>,
                                  psd_byte_order);
    using ReadAlphaMaskChannels =
        void (*)(QIODevice &, KisPaintDeviceSP, int, const QRect &, QVector<ChannelInfo *>, psd_byte_order);
    using WriteChannelDataRle =
        void (*)(QIODevice &, const quint8 *, int, const QRect &, qint64, qint64, bool, psd_byte_order);
    using WritePixelData = void (*)(QIODevice &,
                                    KisPaintDeviceSP,
                                    const QRect &,
                                    psd_color_mode,
                                    int,
                                    bool,
                                    bool,
                                    QVector<Info> &,
                                    psd_compression_type,
                                    psd_byte_order);

    static_assert(std::is_class_v<Info>);
    static_assert(std::is_same_v<decltype(&Info::channelId), qint16 Info::*>);
    static_assert(std::is_same_v<decltype(&Info::sizeFieldOffset), int Info::*>);
    static_assert(std::is_same_v<decltype(&Info::rleBlockOffset), int Info::*>);
    static_assert(std::is_same_v<decltype(&PsdPixelUtils::readChannels), ReadChannels>);
    static_assert(std::is_same_v<decltype(&PsdPixelUtils::readAlphaMaskChannels), ReadAlphaMaskChannels>);
    static_assert(std::is_same_v<decltype(&PsdPixelUtils::writeChannelDataRLE), WriteChannelDataRle>);
    static_assert(std::is_same_v<decltype(&PsdPixelUtils::writePixelDataCommon), WritePixelData>);

    const Info defaults;
    QCOMPARE(defaults.channelId, qint16(0));
    QCOMPARE(defaults.sizeFieldOffset, -1);
    QCOMPARE(defaults.rleBlockOffset, -1);

    const Info externalSizeTag(-3, 17);
    QCOMPARE(externalSizeTag.channelId, qint16(-3));
    QCOMPARE(externalSizeTag.sizeFieldOffset, 17);
    QCOMPARE(externalSizeTag.rleBlockOffset, -1);

    const Info externalOffsets(7, 23, 31);
    QCOMPARE(externalOffsets.channelId, qint16(7));
    QCOMPARE(externalOffsets.sizeFieldOffset, 23);
    QCOMPARE(externalOffsets.rleBlockOffset, 31);
}

#undef ASSERT_PSD_VECTOR_STROKE_SIGNATURE
#undef ASSERT_PSD_VECTOR_ORIGINATION_SIGNATURE
#undef ASSERT_PSD_TEXT_SHAPE_SIGNATURE
#undef ASSERT_PSD_RESOURCE_BLOCK_SIGNATURE
#undef ASSERT_PSD_LAYER_SECTION_SIGNATURE
#undef ASSERT_PSD_LAYER_RECORD_SIGNATURE

QTEST_APPLESS_MAIN(PSDLayerRecordSchemaContractTest)

#include "PSDLayerRecordSchemaContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeSavingContext.h>
#include <svg/SvgSavingContext.h>
#include <svg/SvgWriter.h>

#include <QTest>

#include <type_traits>

namespace
{
using SavedImageMap = QMap<qint64, QString>;

#define ASSERT_SAVING_CONTEXT_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoShapeSavingContext::method)), signature>)

#define ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(method, signature)                                                         \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SvgSavingContext::method)), signature>)

#define ASSERT_SVG_WRITER_SIGNATURE(method, signature)                                                                 \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&SvgWriter::method)), signature>)
} // namespace

class KoShapeSavingContextSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeSavingOptionValueSchemaRemainsStable();
    void shapeSavingContextIdentityAndWriterSignaturesRemainStable();
    void shapeSavingContextOptionSignaturesRemainStable();
    void shapeSavingContextLayerImageAndMarkerSignaturesRemainStable();
    void shapeSavingContextSharedDataAndOffsetSignaturesRemainStable();
    void svgSavingTypeLifetimeAndConstructionSchemaRemainStable();
    void svgSavingWriterTransformAndModeSignaturesRemainStable();
    void svgSavingIdentityAndExternalAssetSignaturesRemainStable();
    void svgWriterOutputSignaturesRemainStable();
    void svgWriterDocumentMetadataSignaturesRemainStable();
};

void KoShapeSavingContextSchemaContractTest::shapeSavingOptionValueSchemaRemainsStable()
{
    using Option = KoShapeSavingContext::ShapeSavingOption;
    using Options = KoShapeSavingContext::ShapeSavingOptions;

    static_assert(std::is_enum_v<Option>);
    static_assert(std::is_same_v<Options, QFlags<Option>>);

    QCOMPARE(int(KoShapeSavingContext::PresentationShape), 1);
    QCOMPARE(int(KoShapeSavingContext::DrawId), 2);
    QCOMPARE(int(KoShapeSavingContext::AutoStyleInStyleXml), 4);
    QCOMPARE(int(KoShapeSavingContext::UniqueMasterPages), 8);
    QCOMPARE(int(KoShapeSavingContext::ZIndex), 16);

    const Options combined = KoShapeSavingContext::PresentationShape | KoShapeSavingContext::AutoStyleInStyleXml
        | KoShapeSavingContext::ZIndex;
    QVERIFY(combined.testFlag(KoShapeSavingContext::PresentationShape));
    QVERIFY(combined.testFlag(KoShapeSavingContext::AutoStyleInStyleXml));
    QVERIFY(combined.testFlag(KoShapeSavingContext::ZIndex));
    QVERIFY(!combined.testFlag(KoShapeSavingContext::DrawId));
    QVERIFY(!combined.testFlag(KoShapeSavingContext::UniqueMasterPages));
}

void KoShapeSavingContextSchemaContractTest::shapeSavingContextIdentityAndWriterSignaturesRemainStable()
{
    static_assert(std::is_class_v<KoShapeSavingContext>);
    static_assert(std::is_constructible_v<KoShapeSavingContext, KoXmlWriter &>);
    static_assert(std::has_virtual_destructor_v<KoShapeSavingContext>);

    ASSERT_SAVING_CONTEXT_SIGNATURE(setXmlWriter, void (KoShapeSavingContext::*)(KoXmlWriter &));
    ASSERT_SAVING_CONTEXT_SIGNATURE(xmlWriter, KoXmlWriter & (KoShapeSavingContext::*)());
}

void KoShapeSavingContextSchemaContractTest::shapeSavingContextOptionSignaturesRemainStable()
{
    ASSERT_SAVING_CONTEXT_SIGNATURE(addOption, void (KoShapeSavingContext::*)(KoShapeSavingContext::ShapeSavingOption));
    ASSERT_SAVING_CONTEXT_SIGNATURE(isSet,
                                    bool (KoShapeSavingContext::*)(KoShapeSavingContext::ShapeSavingOption) const);
    ASSERT_SAVING_CONTEXT_SIGNATURE(options,
                                    KoShapeSavingContext::ShapeSavingOptions (KoShapeSavingContext::*)() const);
    ASSERT_SAVING_CONTEXT_SIGNATURE(removeOption,
                                    void (KoShapeSavingContext::*)(KoShapeSavingContext::ShapeSavingOption));
    ASSERT_SAVING_CONTEXT_SIGNATURE(setOptions,
                                    void (KoShapeSavingContext::*)(KoShapeSavingContext::ShapeSavingOptions));
}

void KoShapeSavingContextSchemaContractTest::shapeSavingContextLayerImageAndMarkerSignaturesRemainStable()
{
    ASSERT_SAVING_CONTEXT_SIGNATURE(addLayerForSaving, void (KoShapeSavingContext::*)(const KoShapeLayer *));
    ASSERT_SAVING_CONTEXT_SIGNATURE(clearLayers, void (KoShapeSavingContext::*)());
    ASSERT_SAVING_CONTEXT_SIGNATURE(imagesToSave, SavedImageMap (KoShapeSavingContext::*)());
    ASSERT_SAVING_CONTEXT_SIGNATURE(markerRef, QString (KoShapeSavingContext::*)(const KoMarker *));
    ASSERT_SAVING_CONTEXT_SIGNATURE(saveLayerSet, void (KoShapeSavingContext::*)(KoXmlWriter &) const);
}

void KoShapeSavingContextSchemaContractTest::shapeSavingContextSharedDataAndOffsetSignaturesRemainStable()
{
    ASSERT_SAVING_CONTEXT_SIGNATURE(addSharedData,
                                    void (KoShapeSavingContext::*)(const QString &, KoSharedSavingData *));
    ASSERT_SAVING_CONTEXT_SIGNATURE(sharedData, KoSharedSavingData * (KoShapeSavingContext::*)(const QString &) const);
    ASSERT_SAVING_CONTEXT_SIGNATURE(addShapeOffset,
                                    void (KoShapeSavingContext::*)(const KoShape *, const QTransform &));
    ASSERT_SAVING_CONTEXT_SIGNATURE(removeShapeOffset, void (KoShapeSavingContext::*)(const KoShape *));
    ASSERT_SAVING_CONTEXT_SIGNATURE(shapeOffset, QTransform (KoShapeSavingContext::*)(const KoShape *) const);
}

void KoShapeSavingContextSchemaContractTest::svgSavingTypeLifetimeAndConstructionSchemaRemainStable()
{
    static_assert(std::is_class_v<SvgSavingContext>);
    static_assert(std::is_constructible_v<SvgSavingContext, QIODevice &>);
    static_assert(std::is_constructible_v<SvgSavingContext, QIODevice &, bool>);
    static_assert(std::is_constructible_v<SvgSavingContext, QIODevice &, QIODevice &>);
    static_assert(std::is_constructible_v<SvgSavingContext, QIODevice &, QIODevice &, bool>);
    static_assert(std::has_virtual_destructor_v<SvgSavingContext>);

    static_assert(std::is_class_v<SvgWriter>);
    static_assert(std::is_constructible_v<SvgWriter, const QList<KoShape *> &>);
    static_assert(std::is_constructible_v<SvgWriter, const QList<KoShapeLayer *> &>);
    static_assert(std::has_virtual_destructor_v<SvgWriter>);
}

void KoShapeSavingContextSchemaContractTest::svgSavingWriterTransformAndModeSignaturesRemainStable()
{
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(styleWriter, KoXmlWriter & (SvgSavingContext::*)());
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(shapeWriter, KoXmlWriter & (SvgSavingContext::*)());
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(userSpaceTransform, QTransform (SvgSavingContext::*)() const);
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(isSavingInlineImages, bool (SvgSavingContext::*)() const);
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(setStrippedTextMode, void (SvgSavingContext::*)(bool));
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(strippedTextMode, bool (SvgSavingContext::*)() const);
}

void KoShapeSavingContextSchemaContractTest::svgSavingIdentityAndExternalAssetSignaturesRemainStable()
{
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(createUID, QString (SvgSavingContext::*)(const QString &));
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(getID, QString (SvgSavingContext::*)(const KoShape *));
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(createFileName, QString (SvgSavingContext::*)(const QString &));
    ASSERT_SVG_SAVING_CONTEXT_SIGNATURE(saveImage, QString (SvgSavingContext::*)(const QImage &));
}

void KoShapeSavingContextSchemaContractTest::svgWriterOutputSignaturesRemainStable()
{
    ASSERT_SVG_WRITER_SIGNATURE(save, bool (SvgWriter::*)(QIODevice &, const QSizeF &));
    ASSERT_SVG_WRITER_SIGNATURE(save, bool (SvgWriter::*)(const QString &, const QSizeF &, bool));
    ASSERT_SVG_WRITER_SIGNATURE(saveDetached, bool (SvgWriter::*)(QIODevice &));
    ASSERT_SVG_WRITER_SIGNATURE(saveDetached, bool (SvgWriter::*)(SvgSavingContext &));
}

void KoShapeSavingContextSchemaContractTest::svgWriterDocumentMetadataSignaturesRemainStable()
{
    ASSERT_SVG_WRITER_SIGNATURE(setDocumentTitle, void (SvgWriter::*)(QString));
    ASSERT_SVG_WRITER_SIGNATURE(setDocumentDescription, void (SvgWriter::*)(QString));
}

QTEST_APPLESS_MAIN(KoShapeSavingContextSchemaContractTest)

#include "KoShapeSavingContextSchemaContractTest.moc"

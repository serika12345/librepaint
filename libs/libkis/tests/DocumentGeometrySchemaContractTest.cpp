/* SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later */

#include <Document.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_DOCUMENT_SIGNATURE(method, signature)                                                                   \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&Document::method)), signature>)
} // namespace

class DocumentGeometrySchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void documentBoundsAndDimensionSignaturesRemainStable();
    void documentOffsetSignaturesRemainStable();
    void documentResolutionSignaturesRemainStable();
    void documentCanvasExtentMutationSignaturesRemainStable();
    void documentGeometricTransformSignaturesRemainStable();
    void documentAnnotationSignaturesRemainStable();
    void documentGridAndGuidesConfigSignaturesRemainStable();
    void documentLegacyGuideStateSignaturesRemainStable();
    void documentLegacyGuideLineSignaturesRemainStable();
    void documentRefreshProjectionSignatureRemainsStable();
};

void DocumentGeometrySchemaContractTest::documentBoundsAndDimensionSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(bounds, QRect (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(width, int (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(height, int (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setWidth, void (Document::*)(int));
    ASSERT_DOCUMENT_SIGNATURE(setHeight, void (Document::*)(int));
}

void DocumentGeometrySchemaContractTest::documentOffsetSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(xOffset, int (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(yOffset, int (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setXOffset, void (Document::*)(int));
    ASSERT_DOCUMENT_SIGNATURE(setYOffset, void (Document::*)(int));
}

void DocumentGeometrySchemaContractTest::documentResolutionSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(resolution, int (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(xRes, double (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(yRes, double (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setResolution, void (Document::*)(int));
    ASSERT_DOCUMENT_SIGNATURE(setXRes, void (Document::*)(double) const);
    ASSERT_DOCUMENT_SIGNATURE(setYRes, void (Document::*)(double) const);
}

void DocumentGeometrySchemaContractTest::documentCanvasExtentMutationSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(crop, void (Document::*)(int, int, int, int));
    ASSERT_DOCUMENT_SIGNATURE(resizeImage, void (Document::*)(int, int, int, int));
    ASSERT_DOCUMENT_SIGNATURE(scaleImage, void (Document::*)(int, int, int, int, QString));
}

void DocumentGeometrySchemaContractTest::documentGeometricTransformSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(rotateImage, void (Document::*)(double));
    ASSERT_DOCUMENT_SIGNATURE(shearImage, void (Document::*)(double, double));
}

void DocumentGeometrySchemaContractTest::documentAnnotationSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(annotation, QByteArray (Document::*)(const QString &));
    ASSERT_DOCUMENT_SIGNATURE(annotationDescription, QString (Document::*)(const QString &) const);
    ASSERT_DOCUMENT_SIGNATURE(annotationTypes, QStringList (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(removeAnnotation, void (Document::*)(const QString &));
    ASSERT_DOCUMENT_SIGNATURE(setAnnotation, void (Document::*)(const QString &, const QString &, const QByteArray &));
}

void DocumentGeometrySchemaContractTest::documentGridAndGuidesConfigSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(gridConfig, GridConfig * (Document::*)());
    ASSERT_DOCUMENT_SIGNATURE(guidesConfig, GuidesConfig * (Document::*)());
    ASSERT_DOCUMENT_SIGNATURE(setGridConfig, void (Document::*)(GridConfig *));
    ASSERT_DOCUMENT_SIGNATURE(setGuidesConfig, void (Document::*)(GuidesConfig *));
}

// Deprecated scripting entry points remain a public compatibility surface and are intentionally referenced here.
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif
void DocumentGeometrySchemaContractTest::documentLegacyGuideStateSignaturesRemainStable()
{
    ASSERT_DOCUMENT_SIGNATURE(guidesLocked, bool (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(guidesVisible, bool (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setGuidesLocked, void (Document::*)(bool));
    ASSERT_DOCUMENT_SIGNATURE(setGuidesVisible, void (Document::*)(bool));
}

void DocumentGeometrySchemaContractTest::documentLegacyGuideLineSignaturesRemainStable()
{
    using GuideLines = QList<qreal>;

    ASSERT_DOCUMENT_SIGNATURE(horizontalGuides, GuideLines (Document::*)() const);
    ASSERT_DOCUMENT_SIGNATURE(setHorizontalGuides, void (Document::*)(const GuideLines &));
    ASSERT_DOCUMENT_SIGNATURE(setVerticalGuides, void (Document::*)(const GuideLines &));
    ASSERT_DOCUMENT_SIGNATURE(verticalGuides, GuideLines (Document::*)() const);
}
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

void DocumentGeometrySchemaContractTest::documentRefreshProjectionSignatureRemainsStable()
{
    ASSERT_DOCUMENT_SIGNATURE(refreshProjection, void (Document::*)());
}

QTEST_APPLESS_MAIN(DocumentGeometrySchemaContractTest)

#include "DocumentGeometrySchemaContractTest.moc"

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

QTEST_APPLESS_MAIN(DocumentGeometrySchemaContractTest)

#include "DocumentGeometrySchemaContractTest.moc"

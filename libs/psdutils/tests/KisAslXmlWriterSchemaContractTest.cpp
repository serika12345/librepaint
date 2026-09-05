/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <asl/kis_asl_xml_writer.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_ASL_XML_WRITER_SIGNATURE(method, signature)                                                             \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisAslXmlWriter::method)), signature>)
} // namespace

class KisAslXmlWriterSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void aslXmlWriterTypeLifetimeAndDocumentSchemaRemainStable();
    void aslXmlWriterDescriptorAndListContainerSignaturesRemainStable();
    void aslXmlWriterScalarValueSignaturesRemainStable();
    void aslXmlWriterGeometrySignaturesRemainStable();
    void aslXmlWriterColorPatternAndGradientSignaturesRemainStable();
};

void KisAslXmlWriterSchemaContractTest::aslXmlWriterTypeLifetimeAndDocumentSchemaRemainStable()
{
    static_assert(std::is_class_v<KisAslXmlWriter>);
    static_assert(std::is_default_constructible_v<KisAslXmlWriter>);
    static_assert(std::is_destructible_v<KisAslXmlWriter>);

    ASSERT_ASL_XML_WRITER_SIGNATURE(document, QDomDocument (KisAslXmlWriter::*)() const);
}

void KisAslXmlWriterSchemaContractTest::aslXmlWriterDescriptorAndListContainerSignaturesRemainStable()
{
    ASSERT_ASL_XML_WRITER_SIGNATURE(enterDescriptor,
                                    void (KisAslXmlWriter::*)(const QString &, const QString &, const QString &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(enterList, void (KisAslXmlWriter::*)(const QString &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(leaveDescriptor, void (KisAslXmlWriter::*)());
    ASSERT_ASL_XML_WRITER_SIGNATURE(leaveList, void (KisAslXmlWriter::*)());
}

void KisAslXmlWriterSchemaContractTest::aslXmlWriterScalarValueSignaturesRemainStable()
{
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeBoolean, void (KisAslXmlWriter::*)(const QString &, bool));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeDouble, void (KisAslXmlWriter::*)(const QString &, double));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeEnum,
                                    void (KisAslXmlWriter::*)(const QString &, const QString &, const QString &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeInteger, void (KisAslXmlWriter::*)(const QString &, int));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeRawData, void (KisAslXmlWriter::*)(const QString, const QByteArray *));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeText, void (KisAslXmlWriter::*)(const QString &, const QString &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeUnitFloat,
                                    void (KisAslXmlWriter::*)(const QString &, const QString &, double));
}

void KisAslXmlWriterSchemaContractTest::aslXmlWriterGeometrySignaturesRemainStable()
{
    ASSERT_ASL_XML_WRITER_SIGNATURE(
        writeCurve,
        void (KisAslXmlWriter::*)(const QString &, const QString &, const QVector<QPointF> &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeFloatRect, void (KisAslXmlWriter::*)(const QString &, const QRectF &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeOffsetPoint, void (KisAslXmlWriter::*)(const QString &, const QPointF &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writePhasePoint, void (KisAslXmlWriter::*)(const QString &, const QPointF &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writePoint, void (KisAslXmlWriter::*)(const QString &, const QPointF &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writePointRect, void (KisAslXmlWriter::*)(const QString &, const QPolygonF &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeTransform, void (KisAslXmlWriter::*)(const QString &, const QTransform &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeUnitRect,
                                    void (KisAslXmlWriter::*)(const QString &, const QString &, const QRectF &));
}

void KisAslXmlWriterSchemaContractTest::aslXmlWriterColorPatternAndGradientSignaturesRemainStable()
{
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeColor, void (KisAslXmlWriter::*)(const QString &, const KoColor &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writePattern, QString (KisAslXmlWriter::*)(const QString &, const KoPatternSP));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writePatternRef,
                                    void (KisAslXmlWriter::*)(const QString &, const KoPatternSP, const QString &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeSegmentGradient,
                                    void (KisAslXmlWriter::*)(const QString &, const KoSegmentGradient &));
    ASSERT_ASL_XML_WRITER_SIGNATURE(writeStopGradient,
                                    void (KisAslXmlWriter::*)(const QString &, const KoStopGradient &));
}

QTEST_APPLESS_MAIN(KisAslXmlWriterSchemaContractTest)

#include "KisAslXmlWriterSchemaContractTest.moc"

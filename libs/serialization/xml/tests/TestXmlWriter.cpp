/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoXmlWriter.h>

#include <QBuffer>
#include <QTest>

class TestXmlWriter : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void numericAttributesRemainStable();
    void contentIsEscaped();
    void nestedDocumentIsIndented();
};

void TestXmlWriter::numericAttributesRemainStable()
{
    QByteArray xml;
    QBuffer output(&xml);
    KoXmlWriter writer(&output);

    writer.startDocument("numbers");
    writer.startElement("numbers");
    writer.addAttribute("double", 1234.56789012345);
    writer.addAttribute("float", 1.2345678f);
    writer.endElement();
    writer.endDocument();

    QCOMPARE(
        xml,
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<numbers double=\"1234.56789012345\" float=\"1.23457\"/>\n"));
}

void TestXmlWriter::contentIsEscaped()
{
    QByteArray xml;
    QBuffer output(&xml);
    KoXmlWriter writer(&output);

    writer.startDocument("root");
    writer.startElement("root");
    writer.addAttribute("attribute", QByteArrayLiteral("<&\""));
    writer.addTextNode(QByteArrayLiteral("<&>"));
    writer.endElement();
    writer.endDocument();

    QCOMPARE(
        xml,
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<root attribute=\"&lt;&amp;&quot;\">&lt;&amp;&gt;</root>\n"));
}

void TestXmlWriter::nestedDocumentIsIndented()
{
    QByteArray xml;
    QBuffer output(&xml);
    KoXmlWriter writer(&output);

    writer.startDocument("root");
    writer.startElement("root");
    writer.startElement("child");
    writer.endElement();
    writer.endElement();
    writer.endDocument();

    QCOMPARE(
        xml,
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<root>\n <child/>\n</root>\n"));
}

QTEST_GUILESS_MAIN(TestXmlWriter)

#include "TestXmlWriter.moc"

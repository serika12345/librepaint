/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <KoXmlNS.h>
#include <KoXmlWriter.h>

#include <QBuffer>
#include <QTest>

class TestXmlWriter : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void namespaceUrisAndPrefixes();
    void documentHeaderAndDeviceLifetime();
    void numericAttributesRemainStable();
    void scalarAttributesRemainStable();
    void contentIsEscaped();
    void textOverloadsAndControlsRemainStable();
    void nestedDocumentIsIndented();
    void indentationCanBeSuppressed();
    void completeElementIsInserted();
    void manifestEntryIsExpanded();
};

void TestXmlWriter::namespaceUrisAndPrefixes()
{
    struct NamespaceExpectation {
        const QString *value;
        const char *uri;
        const char *prefix;
    };
    const NamespaceExpectation mappedNamespaces[] = {
        {&KoXmlNS::office, "urn:oasis:names:tc:opendocument:xmlns:office:1.0", "office"},
        {&KoXmlNS::meta, "urn:oasis:names:tc:opendocument:xmlns:meta:1.0", "meta"},
        {&KoXmlNS::config, "urn:oasis:names:tc:opendocument:xmlns:config:1.0", "config"},
        {&KoXmlNS::text, "urn:oasis:names:tc:opendocument:xmlns:text:1.0", "text"},
        {&KoXmlNS::table, "urn:oasis:names:tc:opendocument:xmlns:table:1.0", "table"},
        {&KoXmlNS::draw, "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0", "draw"},
        {&KoXmlNS::presentation, "urn:oasis:names:tc:opendocument:xmlns:presentation:1.0", "presentation"},
        {&KoXmlNS::dr3d, "urn:oasis:names:tc:opendocument:xmlns:dr3d:1.0", "dr3d"},
        {&KoXmlNS::chart, "urn:oasis:names:tc:opendocument:xmlns:chart:1.0", "chart"},
        {&KoXmlNS::form, "urn:oasis:names:tc:opendocument:xmlns:form:1.0", "form"},
        {&KoXmlNS::script, "urn:oasis:names:tc:opendocument:xmlns:script:1.0", "script"},
        {&KoXmlNS::style, "urn:oasis:names:tc:opendocument:xmlns:style:1.0", "style"},
        {&KoXmlNS::number, "urn:oasis:names:tc:opendocument:xmlns:datastyle:1.0", "number"},
        {&KoXmlNS::manifest, "urn:oasis:names:tc:opendocument:xmlns:manifest:1.0", "manifest"},
        {&KoXmlNS::anim, "urn:oasis:names:tc:opendocument:xmlns:animation:1.0", "anim"},
        {&KoXmlNS::math, "http://www.w3.org/1998/Math/MathML", "math"},
        {&KoXmlNS::svg, "urn:oasis:names:tc:opendocument:xmlns:svg-compatible:1.0", "svg"},
        {&KoXmlNS::fo, "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0", "fo"},
        {&KoXmlNS::dc, "http://purl.org/dc/elements/1.1/", "dc"},
        {&KoXmlNS::xlink, "http://www.w3.org/1999/xlink", "xlink"},
        {&KoXmlNS::VL, "http://openoffice.org/2001/versions-list", "VL"},
        {&KoXmlNS::smil, "urn:oasis:names:tc:opendocument:xmlns:smil-compatible:1.0", "smil"},
        {&KoXmlNS::xhtml, "http://www.w3.org/1999/xhtml", "xhtml"},
        {&KoXmlNS::xml, "http://www.w3.org/XML/1998/namespace", "xml"},
        {&KoXmlNS::calligra, "http://www.calligra.org/2005/", "calligra"},
        {&KoXmlNS::officeooo, "http://openoffice.org/2009/office", "officeooo"},
    };

    for (const NamespaceExpectation &entry : mappedNamespaces) {
        QCOMPARE(*entry.value, QString::fromLatin1(entry.uri));
        QCOMPARE(QByteArray(KoXmlNS::nsURI2NS(*entry.value)), QByteArray(entry.prefix));
    }

    const NamespaceExpectation unmappedNamespaces[] = {
        {&KoXmlNS::sodipodi, "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd", ""},
        {&KoXmlNS::krita, "http://krita.org/namespaces/svg/krita", ""},
        {&KoXmlNS::ooo, "http://openoffice.org/2004/office", ""},
        {&KoXmlNS::delta, "http://www.deltaxml.com/ns/track-changes/delta-namespace", ""},
        {&KoXmlNS::split, "http://www.deltaxml.com/ns/track-changes/split-namespace", ""},
        {&KoXmlNS::ac, "http://www.deltaxml.com/ns/track-changes/attribute-change-namespace", ""},
    };

    for (const NamespaceExpectation &entry : unmappedNamespaces) {
        QCOMPARE(*entry.value, QString::fromLatin1(entry.uri));
        QCOMPARE(QByteArray(KoXmlNS::nsURI2NS(*entry.value)), QByteArray(entry.prefix));
    }
    QCOMPARE(QByteArray(KoXmlNS::nsURI2NS(QStringLiteral("urn:unknown"))), QByteArray());
}

void TestXmlWriter::documentHeaderAndDeviceLifetime()
{
    QByteArray xml;
    QBuffer output(&xml);

    {
        KoXmlWriter writer(&output);
        QVERIFY(output.isOpen());
        writer.startDocument("root", "-//LibrePaint//DTD Contract 1.0//EN", "contract.dtd");
        writer.startElement("root");
        writer.endElement();
        writer.endDocument();
    }

    QVERIFY(output.isOpen());
    QCOMPARE(
        xml,
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE root PUBLIC \"-//LibrePaint//DTD Contract 1.0//EN\" \"contract.dtd\">\n"
            "<root/>\n"));
}

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

void TestXmlWriter::scalarAttributesRemainStable()
{
    QByteArray xml;
    QBuffer output(&xml);
    KoXmlWriter writer(&output);

    writer.startDocument("values");
    writer.startElement("values");
    writer.addAttribute("string", QStringLiteral("文字<&\""));
    writer.addAttribute("bytes", QByteArrayLiteral("bytes<&\""));
    writer.addAttribute("chars", "chars<&\"");
    writer.addAttribute("integer", -42);
    writer.addAttribute("unsigned", uint(42));
    writer.addAttribute("enabled", true);
    writer.addAttribute("disabled", false);
    writer.endElement();
    writer.endDocument();

    QCOMPARE(
        xml,
        QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<values string=\"文字&lt;&amp;&quot;\" bytes=\"bytes&lt;&amp;&quot;\" "
            "chars=\"chars&lt;&amp;&quot;\" integer=\"-42\" unsigned=\"42\" "
            "enabled=\"true\" disabled=\"false\"/>\n")
            .toUtf8());
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

void TestXmlWriter::textOverloadsAndControlsRemainStable()
{
    QByteArray xml;
    QBuffer output(&xml);
    KoXmlWriter writer(&output);

    writer.startDocument("root");
    writer.startElement("root");
    writer.addTextNode(QStringLiteral("文字<&>"));
    writer.addTextNode(QByteArray("bytes\x01\t<&>"));
    writer.addTextNode("chars\x02\r<&>");
    writer.endElement();
    writer.endDocument();

    QCOMPARE(
        xml,
        QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<root>文字&lt;&amp;&gt;bytes\t&lt;&amp;&gt;chars\r&lt;&amp;&gt;</root>\n")
            .toUtf8());
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

void TestXmlWriter::indentationCanBeSuppressed()
{
    QByteArray indentedXml;
    QBuffer indentedOutput(&indentedXml);
    KoXmlWriter indentedWriter(&indentedOutput, 2);
    indentedWriter.startDocument("root");
    indentedWriter.startElement("root");
    indentedWriter.startElement("child");
    indentedWriter.endElement();
    indentedWriter.endElement();
    indentedWriter.endDocument();

    QCOMPARE(
        indentedXml,
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<root>\n   <child/>\n  </root>\n"));

    QByteArray compactXml;
    QBuffer compactOutput(&compactXml);
    KoXmlWriter compactWriter(&compactOutput);
    compactWriter.startDocument("root");
    compactWriter.startElement("root", false);
    compactWriter.startElement("child");
    compactWriter.endElement();
    compactWriter.endElement();
    compactWriter.endDocument();

    QCOMPARE(
        compactXml,
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<root><child/></root>\n"));
}

void TestXmlWriter::completeElementIsInserted()
{
    QByteArray fragment = QByteArrayLiteral("<child/>");
    QBuffer input(&fragment);
    QByteArray xml;
    QBuffer output(&xml);
    KoXmlWriter writer(&output);

    writer.startDocument("root");
    writer.startElement("root");
    writer.addCompleteElement(&input);
    writer.endElement();
    writer.endDocument();

    QVERIFY(!input.isOpen());
    QCOMPARE(
        xml,
        QByteArrayLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<root>\n  <child/>\n</root>\n"));
}

void TestXmlWriter::manifestEntryIsExpanded()
{
    QByteArray xml;
    QBuffer output(&xml);
    KoXmlWriter writer(&output);

    writer.startDocument("manifest:manifest");
    writer.startElement("manifest:manifest");
    writer.addManifestEntry(QStringLiteral("Pictures/表紙.png"), QStringLiteral("image/png"));
    writer.endElement();
    writer.endDocument();

    QCOMPARE(
        xml,
        QStringLiteral(
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<manifest:manifest>\n"
            " <manifest:file-entry manifest:media-type=\"image/png\" "
            "manifest:full-path=\"Pictures/表紙.png\"/>\n"
            "</manifest:manifest>\n")
            .toUtf8());
}

QTEST_GUILESS_MAIN(TestXmlWriter)

#include "TestXmlWriter.moc"

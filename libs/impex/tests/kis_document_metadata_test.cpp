/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <metadata/KoDocumentInfo.h>

#include <QDateTime>
#include <QDomDocument>
#include <QSignalSpy>

class KisImportExportDocumentMetadataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testAboutAndGeneratorValues();
    void testSaveUsesExplicitDocumentState();
    void testParameterUpdateUsesExplicitModificationState();
    void testLoadAndSaveRoundTrip();
    void testCopyPreservesDocumentValues();
    void testResetMetadataStartsANewEditingHistory();
};

void KisImportExportDocumentMetadataTest::testAboutAndGeneratorValues()
{
    QObject parent;
    KoDocumentInfo info(&parent);
    QCOMPARE(info.parent(), &parent);
    QCOMPARE(info.aboutInfo("editing-cycles"), QStringLiteral("0"));
    QVERIFY(QDateTime::fromString(info.aboutInfo("creation-date"), Qt::ISODate).isValid());

    QSignalSpy updates(&info, &KoDocumentInfo::infoUpdated);
    info.setAboutInfo("title", "Contract title");
    QCOMPARE(info.aboutInfo("title"), QStringLiteral("Contract title"));
    QCOMPARE(updates.count(), 1);

    info.setAboutInfo("unsupported-field", "ignored");
    QCOMPARE(info.aboutInfo("unsupported-field"), QString());
    QCOMPARE(updates.count(), 1);

    QCOMPARE(info.originalGenerator(), QString());
    info.setOriginalGenerator("LibrePaint contract generator");
    QCOMPARE(info.originalGenerator(), QStringLiteral("LibrePaint contract generator"));
}

void KisImportExportDocumentMetadataTest::testSaveUsesExplicitDocumentState()
{
    KoDocumentInfo info;
    info.setAboutInfo("editing-cycles", "4");

    QDomDocument autosaveDocument;
    autosaveDocument.appendChild(autosaveDocument.createElement("document"));
    info.save(autosaveDocument, true, true);
    QCOMPARE(info.aboutInfo("editing-cycles"), QString("4"));

    QDomDocument unchangedDocument;
    unchangedDocument.appendChild(unchangedDocument.createElement("document"));
    info.save(unchangedDocument, false, false);
    QCOMPARE(info.aboutInfo("editing-cycles"), QString("5"));
}

void KisImportExportDocumentMetadataTest::testParameterUpdateUsesExplicitModificationState()
{
    KoDocumentInfo info;
    info.setAuthorInfo("creator", "Explicit Author");

    info.updateParameters(false);
    QCOMPARE(info.authorInfo("creator"), QString());

    info.updateParameters(true);
    QCOMPARE(info.authorInfo("creator"), QString("Explicit Author"));
}

void KisImportExportDocumentMetadataTest::testLoadAndSaveRoundTrip()
{
    QDomDocument source;
    QDomElement root = source.createElement("document");
    source.appendChild(root);

    QDomElement about = source.createElement("about");
    root.appendChild(about);
    QDomElement title = source.createElement("title");
    title.appendChild(source.createTextNode("Loaded title"));
    about.appendChild(title);
    QDomElement abstract = source.createElement("abstract");
    abstract.appendChild(source.createCDATASection("Loaded <abstract>"));
    about.appendChild(abstract);

    QDomElement author = source.createElement("author");
    root.appendChild(author);
    QDomElement fullName = source.createElement("full-name");
    fullName.appendChild(source.createTextNode("Contract Author"));
    author.appendChild(fullName);
    QDomElement contact = source.createElement("contact");
    contact.setAttribute("type", "email");
    contact.appendChild(source.createTextNode("author@example.test"));
    author.appendChild(contact);

    KoDocumentInfo info;
    QVERIFY(info.load(source));
    QCOMPARE(info.aboutInfo("title"), QStringLiteral("Loaded title"));
    QCOMPARE(info.aboutInfo("abstract"), QStringLiteral("Loaded <abstract>"));
    QCOMPARE(info.authorInfo("creator"), QStringLiteral("Contract Author"));
    QCOMPARE(info.authorContactInfo(), QStringList({QStringLiteral("author@example.test")}));

    QDomDocument destination;
    destination.appendChild(destination.createElement("document"));
    const QDomDocument saved = info.save(destination, true, false);
    QCOMPARE(saved.documentElement().firstChildElement("about").firstChildElement("title").text(),
             QStringLiteral("Loaded title"));
    QCOMPARE(saved.documentElement().firstChildElement("about").firstChildElement("abstract").text(),
             QStringLiteral("Loaded <abstract>"));
    QCOMPARE(saved.documentElement().firstChildElement("author").firstChildElement("full-name").text(),
             QStringLiteral("Contract Author"));
    const QDomElement savedContact = saved.documentElement().firstChildElement("author").firstChildElement("contact");
    QCOMPARE(savedContact.attribute("type"), QStringLiteral("email"));
    QCOMPARE(savedContact.text(), QStringLiteral("author@example.test"));

    QDomDocument missingSections;
    missingSections.appendChild(missingSections.createElement("document"));
    QVERIFY(!info.load(missingSections));
}

void KisImportExportDocumentMetadataTest::testCopyPreservesDocumentValues()
{
    QObject parent;
    KoDocumentInfo original;
    original.setAboutInfo("title", "Original title");
    original.setOriginalGenerator("Original generator");
    original.setAuthorInfo("creator", "Original author");
    original.updateParameters(true);

    KoDocumentInfo copy(original, &parent);
    QCOMPARE(copy.parent(), &parent);
    QCOMPARE(copy.aboutInfo("title"), QStringLiteral("Original title"));
    QCOMPARE(copy.authorInfo("creator"), QStringLiteral("Original author"));
    QCOMPARE(copy.originalGenerator(), QStringLiteral("Original generator"));

    original.setAboutInfo("title", "Changed original");
    QCOMPARE(copy.aboutInfo("title"), QStringLiteral("Original title"));
}

void KisImportExportDocumentMetadataTest::testResetMetadataStartsANewEditingHistory()
{
    KoDocumentInfo info;
    info.setAuthorInfo("creator", "Reset Author");
    info.updateParameters(true);
    info.setAboutInfo("editing-cycles", "12");
    info.setAboutInfo("editing-time", "345");
    info.setAboutInfo("creation-date", "2001-02-03T04:05:06");

    info.resetMetaData();

    QCOMPARE(info.aboutInfo("editing-cycles"), QStringLiteral("0"));
    QCOMPARE(info.aboutInfo("editing-time"), QStringLiteral("0"));
    QCOMPARE(info.aboutInfo("initial-creator"), QStringLiteral("Reset Author"));
    QVERIFY(QDateTime::fromString(info.aboutInfo("creation-date"), Qt::ISODate).isValid());
    QVERIFY(info.aboutInfo("creation-date") != QStringLiteral("2001-02-03T04:05:06"));
}

QTEST_GUILESS_MAIN(KisImportExportDocumentMetadataTest)

#include "kis_document_metadata_test.moc"

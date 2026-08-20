/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <metadata/KoDocumentInfo.h>

#include <QDomDocument>

class KisDocumentMetadataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSaveUsesExplicitDocumentState();
    void testParameterUpdateUsesExplicitModificationState();
};

void KisDocumentMetadataTest::testSaveUsesExplicitDocumentState()
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

void KisDocumentMetadataTest::testParameterUpdateUsesExplicitModificationState()
{
    KoDocumentInfo info;
    info.setAuthorInfo("creator", "Explicit Author");

    info.updateParameters(false);
    QCOMPARE(info.authorInfo("creator"), QString());

    info.updateParameters(true);
    QCOMPARE(info.authorInfo("creator"), QString("Explicit Author"));
}

QTEST_GUILESS_MAIN(KisDocumentMetadataTest)

#include "kis_document_metadata_test.moc"

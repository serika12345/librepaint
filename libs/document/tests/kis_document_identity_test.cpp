/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <session/kis_document_identity.h>

using Krita::Document::Identity;

class KisDocumentIdentityTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void startsWithoutAFileOrFormat();
    void tracksDisplayAndLocalPathsIndependently();
    void preservesMimeSelectionAndDetectionStateWhenCopied();
};

void KisDocumentIdentityTest::startsWithoutAFileOrFormat()
{
    const Identity identity;

    QVERIFY(identity.path().isEmpty());
    QVERIFY(identity.localFilePath().isEmpty());
    QVERIFY(identity.mimeType().isEmpty());
    QVERIFY(!identity.mimeTypeWasAutoDetected());
}

void KisDocumentIdentityTest::tracksDisplayAndLocalPathsIndependently()
{
    Identity identity;

    QVERIFY(identity.setPath(QStringLiteral("content://provider/document/42")));
    QVERIFY(!identity.setPath(QStringLiteral("content://provider/document/42")));
    identity.setLocalFilePath(QStringLiteral("/tmp/librepaint-open-42.kra"));

    QCOMPARE(identity.path(), QStringLiteral("content://provider/document/42"));
    QCOMPARE(identity.localFilePath(), QStringLiteral("/tmp/librepaint-open-42.kra"));

    identity.resetPaths();
    QVERIFY(identity.path().isEmpty());
    QVERIFY(identity.localFilePath().isEmpty());
}

void KisDocumentIdentityTest::preservesMimeSelectionAndDetectionStateWhenCopied()
{
    Identity identity;
    identity.setMimeType("image/openraster");
    identity.setMimeTypeWasAutoDetected(true);

    const Identity copy(identity);

    QCOMPARE(copy.mimeType(), QByteArray("image/openraster"));
    QVERIFY(copy.mimeTypeWasAutoDetected());

    identity.clearMimeType();
    QVERIFY(identity.mimeType().isEmpty());
    QVERIFY(identity.mimeTypeWasAutoDetected());
}

QTEST_GUILESS_MAIN(KisDocumentIdentityTest)

#include "kis_document_identity_test.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisMimeDatabase.h>

class KisMimeDatabaseTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nativeDocumentSuffix();
    void suffixLookupNormalizesInput();
    void contentAndMetadataLookup();
};

void KisMimeDatabaseTest::nativeDocumentSuffix()
{
    QCOMPARE(KisMimeDatabase::mimeTypeForFile(QStringLiteral("document.kra"), false),
             QStringLiteral("application/x-krita"));
    QCOMPARE(KisMimeDatabase::mimeTypeForFile(QStringLiteral("DOCUMENT.KRA"), false),
             QStringLiteral("application/x-krita"));
}

void KisMimeDatabaseTest::suffixLookupNormalizesInput()
{
    QCOMPARE(KisMimeDatabase::mimeTypeForSuffix(QStringLiteral("kra")), QStringLiteral("application/x-krita"));
    QCOMPARE(KisMimeDatabase::mimeTypeForSuffix(QStringLiteral("*.KRA")), QStringLiteral("application/x-krita"));
}

void KisMimeDatabaseTest::contentAndMetadataLookup()
{
    QCOMPARE(KisMimeDatabase::mimeTypeForData(QByteArrayLiteral("%PDF-1.7\n")), QStringLiteral("application/pdf"));

    const QString documentMimeType = QStringLiteral("application/x-krita");
    const QString description = KisMimeDatabase::descriptionForMimeType(documentMimeType);
    QVERIFY(!description.isEmpty());
    QVERIFY(description != documentMimeType);
    QCOMPARE(KisMimeDatabase::suffixesForMimeType(documentMimeType), QStringList{QStringLiteral("kra")});
    QVERIFY(!KisMimeDatabase::iconNameForMimeType(QStringLiteral("image/png")).isEmpty());
}

QTEST_GUILESS_MAIN(KisMimeDatabaseTest)

#include "KisMimeDatabaseTest.moc"

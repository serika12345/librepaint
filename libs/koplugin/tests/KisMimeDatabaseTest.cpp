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
};

void KisMimeDatabaseTest::nativeDocumentSuffix()
{
    QCOMPARE(KisMimeDatabase::mimeTypeForFile(QStringLiteral("document.kra"), false),
             QStringLiteral("application/x-krita"));
    QCOMPARE(KisMimeDatabase::mimeTypeForFile(QStringLiteral("DOCUMENT.KRA"), false),
             QStringLiteral("application/x-krita"));
}

QTEST_GUILESS_MAIN(KisMimeDatabaseTest)

#include "KisMimeDatabaseTest.moc"

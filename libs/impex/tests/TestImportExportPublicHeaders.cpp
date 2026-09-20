/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KisImportExportMimeType.h>
#include <kis_store_paintdevice_writer.h>

class TestImportExportPublicHeaders : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void headersAreSelfContained();
};

void TestImportExportPublicHeaders::headersAreSelfContained()
{
    QVERIFY(true);
}

QTEST_GUILESS_MAIN(TestImportExportPublicHeaders)

#include "TestImportExportPublicHeaders.moc"

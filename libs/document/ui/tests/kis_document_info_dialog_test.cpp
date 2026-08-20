/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <info/KoDocumentInfoDlg.h>
#include <metadata/KoDocumentInfo.h>

#include <QLabel>
#include <QLineEdit>

class KisDocumentInfoDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testDocumentValuesArePassedWithoutKisDocumentParent();
    void testAcceptUpdatesMetadata();
};

void KisDocumentInfoDialogTest::testDocumentValuesArePassedWithoutKisDocumentParent()
{
    KoDocumentInfo info;
    KoDocumentInfoDlg dialog(nullptr,
                             &info,
                             QStringLiteral("/tmp/example.kra"),
                             QByteArrayLiteral("application/x-krita"));

    QLabel *filePath = dialog.findChild<QLabel *>("filePathLabel");
    QLabel *type = dialog.findChild<QLabel *>("lblType");
    QVERIFY(filePath);
    QVERIFY(type);
    QCOMPARE(filePath->text(), QStringLiteral("/tmp/example.kra"));
    QVERIFY(!type->text().isEmpty());
}

void KisDocumentInfoDialogTest::testAcceptUpdatesMetadata()
{
    KoDocumentInfo info;
    KoDocumentInfoDlg dialog(nullptr,
                             &info,
                             QStringLiteral("/tmp/example.kra"),
                             QByteArrayLiteral("application/x-krita"));

    QLineEdit *title = dialog.findChild<QLineEdit *>("leTitle");
    QVERIFY(title);
    title->setText(QStringLiteral("Document title"));

    dialog.accept();

    QCOMPARE(info.aboutInfo("title"), QStringLiteral("Document title"));
}

QTEST_MAIN(KisDocumentInfoDialogTest)

#include "kis_document_info_dialog_test.moc"

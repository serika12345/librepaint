/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QTest>

#include <KoPageWidgetItem.h>
#include <info/KoDocumentInfoDlg.h>
#include <metadata/KoDocumentInfo.h>

#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalSpy>

class TrackingPageItem final : public KoPageWidgetItem
{
public:
    TrackingPageItem(bool *veto, bool *applied)
        : m_widget(new QWidget)
        , m_veto(veto)
        , m_applied(applied)
    {
    }

    QWidget *widget() override
    {
        return m_widget;
    }

    QString name() const override
    {
        return QStringLiteral("Extension");
    }

    QString iconName() const override
    {
        return QString();
    }

    bool shouldDialogCloseBeVetoed() override
    {
        return *m_veto;
    }

    void apply() override
    {
        *m_applied = true;
    }

private:
    QWidget *m_widget;
    bool *m_veto;
    bool *m_applied;
};

class KisDocumentInfoDialogTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testDocumentValuesArePassedWithoutKisDocumentParent();
    void testAcceptUpdatesMetadata();
    void testAdditionalPagesCanVetoAndApplyAcceptance();
    void testReadOnlyStateAppliesToDocumentFields();
    void testDocumentSavedFlagRemainsFalseAfterAccept();
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

void KisDocumentInfoDialogTest::testAdditionalPagesCanVetoAndApplyAcceptance()
{
    KoDocumentInfo info;
    KoDocumentInfoDlg dialog(nullptr, &info, QString(), QByteArray());
    bool veto = true;
    bool applied = false;
    QSignalSpy acceptedSpy(&dialog, &QDialog::accepted);

    QCOMPARE(dialog.pages().size(), 2);
    dialog.addPageItem(new TrackingPageItem(&veto, &applied));
    QCOMPARE(dialog.pages().size(), 3);

    dialog.accept();
    QCOMPARE(acceptedSpy.count(), 0);
    QVERIFY(!applied);

    veto = false;
    dialog.accept();
    QCOMPARE(acceptedSpy.count(), 1);
    QVERIFY(applied);
}

void KisDocumentInfoDialogTest::testReadOnlyStateAppliesToDocumentFields()
{
    KoDocumentInfo info;
    KoDocumentInfoDlg dialog(nullptr, &info, QString(), QByteArray());

    dialog.setReadOnly(true);
    for (KPageWidgetItem *page : dialog.pages()) {
        const QList<QLineEdit *> lineEdits = page->widget()->findChildren<QLineEdit *>();
        for (QLineEdit *lineEdit : lineEdits) {
            QVERIFY(lineEdit->isReadOnly());
        }
        const QList<QPushButton *> pushButtons = page->widget()->findChildren<QPushButton *>();
        for (QPushButton *pushButton : pushButtons) {
            QVERIFY(!pushButton->isEnabled());
        }
    }

    dialog.setReadOnly(false);
    for (KPageWidgetItem *page : dialog.pages()) {
        const QList<QLineEdit *> lineEdits = page->widget()->findChildren<QLineEdit *>();
        for (QLineEdit *lineEdit : lineEdits) {
            QVERIFY(!lineEdit->isReadOnly());
        }
        const QList<QPushButton *> pushButtons = page->widget()->findChildren<QPushButton *>();
        for (QPushButton *pushButton : pushButtons) {
            QVERIFY(pushButton->isEnabled());
        }
    }
}

void KisDocumentInfoDialogTest::testDocumentSavedFlagRemainsFalseAfterAccept()
{
    KoDocumentInfo info;
    KoDocumentInfoDlg dialog(nullptr, &info, QString(), QByteArray());

    QVERIFY(!dialog.isDocumentSaved());
    dialog.accept();
    QVERIFY(!dialog.isDocumentSaved());
}

QTEST_MAIN(KisDocumentInfoDialogTest)

#include "kis_document_info_dialog_test.moc"

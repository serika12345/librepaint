/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDialog.h>

#include <QPushButton>
#include <QSignalSpy>
#include <QTest>

#include <KGuiItem>

class KoDialogContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dialogDisplaysContentAndModifiedCaption();
    void availableChoicesReflectDialogState();
    void buttonClicksDispatchSelections();
    void detailsButtonTogglesExtraContent();
};

void KoDialogContractTest::dialogDisplaysContentAndModifiedCaption()
{
    /*
     * Consumer: Import, export, and configuration dialogs derived from KoDialog.
     * Operation: Supplies dialog content and marks a document caption as modified.
     * Observable result: The content is displayed and the title changes to indicate the modified document.
     * Failure impact: Users cannot identify the dialog's content or whether its document has unsaved changes.
     */
    KoDialog dialog;
    auto *content = new QWidget;
    dialog.setMainWidget(content);
    dialog.setCaption(QStringLiteral("Document"));
    const QString savedCaption = dialog.windowTitle();
    dialog.setCaption(QStringLiteral("Document"), true);

    dialog.show();
    QCoreApplication::processEvents();

    QVERIFY(content->isVisible());
    QVERIFY(dialog.windowTitle().contains(QStringLiteral("Document")));
    QVERIFY(dialog.windowTitle() != savedCaption);
}

void KoDialogContractTest::availableChoicesReflectDialogState()
{
    /*
     * Consumer: Dialogs that enable, disable, label, and select standard actions while validating user input.
     * Operation: Configures confirmation, apply, and cancellation choices.
     * Observable result: The displayed controls expose the configured default state, availability, visibility, and text.
     * Failure impact: Users can be offered unavailable actions or cannot recognize the action they are about to take.
     */
    KoDialog dialog;
    dialog.setButtons(KoDialog::Ok | KoDialog::Apply | KoDialog::Cancel);
    dialog.show();
    QCoreApplication::processEvents();

    QVERIFY(dialog.button(KoDialog::Ok));
    QVERIFY(dialog.button(KoDialog::Apply));
    QVERIFY(dialog.button(KoDialog::Cancel));

    dialog.setDefaultButton(KoDialog::Apply);
    QCOMPARE(dialog.defaultButton(), KoDialog::Apply);

    dialog.enableButton(KoDialog::Ok, false);
    dialog.enableButtonApply(false);
    dialog.enableButtonCancel(true);
    QVERIFY(!dialog.isButtonEnabled(KoDialog::Ok));
    QVERIFY(!dialog.isButtonEnabled(KoDialog::Apply));
    QVERIFY(dialog.isButtonEnabled(KoDialog::Cancel));

    dialog.showButton(KoDialog::Apply, false);
    QVERIFY(!dialog.button(KoDialog::Apply)->isVisible());

    dialog.setButtonText(KoDialog::Ok, QStringLiteral("Proceed"));
    dialog.setButtonToolTip(KoDialog::Ok, QStringLiteral("Continue with the import"));
    dialog.setButtonWhatsThis(KoDialog::Ok, QStringLiteral("Confirms the import settings"));
    QCOMPARE(dialog.buttonText(KoDialog::Ok), QStringLiteral("Proceed"));
    QCOMPARE(dialog.buttonToolTip(KoDialog::Ok), QStringLiteral("Continue with the import"));
    QCOMPARE(dialog.buttonWhatsThis(KoDialog::Ok), QStringLiteral("Confirms the import settings"));

    dialog.setButtonGuiItem(KoDialog::Ok, KGuiItem(QStringLiteral("Import")));
    QCOMPARE(dialog.buttonText(KoDialog::Ok), QStringLiteral("Import"));
}

void KoDialogContractTest::buttonClicksDispatchSelections()
{
    /*
     * Consumer: Dialog implementations connected to KoDialog's standard button signals.
     * Operation: Clicks each available standard action.
     * Observable result: The matching signal identifies the selected action, and closing actions return their documented result.
     * Failure impact: Dialog workflows can save, discard, retry, or close in response to the wrong user choice.
     */
    struct ButtonCase {
        KoDialog::ButtonCode code;
        const char *signal;
        bool closesDialog;
        int result;
    };
    const ButtonCase cases[] = {
        {KoDialog::Help, SIGNAL(helpClicked()), false, QDialog::Rejected},
        {KoDialog::Default, SIGNAL(defaultClicked()), false, QDialog::Rejected},
        {KoDialog::Apply, SIGNAL(applyClicked()), false, QDialog::Rejected},
        {KoDialog::Try, SIGNAL(tryClicked()), false, QDialog::Rejected},
        {KoDialog::Reset, SIGNAL(resetClicked()), false, QDialog::Rejected},
        {KoDialog::User1, SIGNAL(user1Clicked()), false, QDialog::Rejected},
        {KoDialog::User2, SIGNAL(user2Clicked()), false, QDialog::Rejected},
        {KoDialog::User3, SIGNAL(user3Clicked()), false, QDialog::Rejected},
        {KoDialog::Ok, SIGNAL(okClicked()), true, QDialog::Accepted},
        {KoDialog::Yes, SIGNAL(yesClicked()), true, KoDialog::Yes},
        {KoDialog::No, SIGNAL(noClicked()), true, KoDialog::No},
        {KoDialog::Cancel, SIGNAL(cancelClicked()), true, QDialog::Rejected},
        {KoDialog::Close, SIGNAL(closeClicked()), true, KoDialog::Close},
    };

    for (const ButtonCase &entry : cases) {
        KoDialog dialog;
        dialog.setButtons(entry.code);
        QPushButton *button = dialog.button(entry.code);
        QVERIFY(button);

        QSignalSpy specific(&dialog, entry.signal);
        QSignalSpy general(&dialog, &KoDialog::buttonClicked);
        QVERIFY(specific.isValid());
        QVERIFY(general.isValid());

        button->click();

        QCOMPARE(specific.count(), 1);
        QCOMPARE(general.count(), 1);
        QCOMPARE(general.first().first().toInt(), int(entry.code));
        if (entry.closesDialog) {
            QCOMPARE(dialog.result(), entry.result);
        }
    }
}

void KoDialogContractTest::detailsButtonTogglesExtraContent()
{
    /*
     * Consumer: Dialogs that reveal optional diagnostic or advanced content on demand.
     * Operation: Opens and closes the Details control.
     * Observable result: The details widget becomes visible and hidden with the control, and the pre-display signal is emitted.
     * Failure impact: Users cannot inspect optional information needed to complete or diagnose a dialog workflow.
     */
    KoDialog dialog;
    dialog.setButtons(KoDialog::Details);
    auto *details = new QWidget;
    dialog.setDetailsWidget(details);
    dialog.show();
    QCoreApplication::processEvents();

    QSignalSpy detailsSpy(&dialog, &KoDialog::aboutToShowDetails);
    QPushButton *button = dialog.button(KoDialog::Details);
    QVERIFY(button);

    button->click();
    QVERIFY(dialog.isDetailsWidgetVisible());
    QVERIFY(details->isVisible());
    QCOMPARE(detailsSpy.count(), 1);

    button->click();
    QVERIFY(!dialog.isDetailsWidgetVisible());
    QVERIFY(!details->isVisible());
}

QTEST_MAIN(KoDialogContractTest)

#include "KoDialogContractTest.moc"

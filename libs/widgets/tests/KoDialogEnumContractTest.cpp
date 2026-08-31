/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDialog.h>

#include <QPointer>
#include <QPushButton>
#include <QSignalSpy>
#include <QStyle>
#include <QTest>
#include <QVBoxLayout>

#include <KGuiItem>
#include <khelpclient.h>

void KHelpClient::invokeHelp(const QString &, const QString &)
{
    qFatal("The KoDialog contract must not launch external help");
}

#include <array>
#include <utility>

class KoDialogEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buttonCodesRemainStableFlags();
    void popupModesRemainStableFlags();
    void captionFlagsRemainStableAliases();
    void constructionLayoutCaptionAndLifetimeRemainStable();
    void buttonStateAndPresentationRoundTrip();
    void buttonActivationDispatchesStableSignals();
    void mainDetailsAndHelpStateRemainIndependent();
    void visibilityLayoutAndDeferredLifetimeTransitions();
};

void KoDialogEnumContractTest::buttonCodesRemainStableFlags()
{
    const std::array<std::pair<KoDialog::ButtonCode, int>, 16> codes{{
        {KoDialog::None, 0x00000000},
        {KoDialog::Help, 0x00000001},
        {KoDialog::Default, 0x00000002},
        {KoDialog::Ok, 0x00000004},
        {KoDialog::Apply, 0x00000008},
        {KoDialog::Try, 0x00000010},
        {KoDialog::Cancel, 0x00000020},
        {KoDialog::Close, 0x00000040},
        {KoDialog::No, 0x00000080},
        {KoDialog::Yes, 0x00000100},
        {KoDialog::Reset, 0x00000200},
        {KoDialog::Details, 0x00000400},
        {KoDialog::User1, 0x00001000},
        {KoDialog::User2, 0x00002000},
        {KoDialog::User3, 0x00004000},
        {KoDialog::NoDefault, 0x00008000},
    }};

    for (const auto &code : codes) {
        QCOMPARE(int(code.first), code.second);
    }

    const KoDialog::ButtonCodes buttons = KoDialog::Ok | KoDialog::Cancel | KoDialog::Apply;
    QVERIFY(buttons.testFlag(KoDialog::Ok));
    QVERIFY(buttons.testFlag(KoDialog::Cancel));
    QVERIFY(buttons.testFlag(KoDialog::Apply));
    QVERIFY(!buttons.testFlag(KoDialog::Help));
}

void KoDialogEnumContractTest::popupModesRemainStableFlags()
{
    QCOMPARE(int(KoDialog::InstantPopup), 0);
    QCOMPARE(int(KoDialog::DelayedPopup), 1);

    const KoDialog::ButtonPopupModes modes = KoDialog::DelayedPopup;
    QVERIFY(modes.testFlag(KoDialog::DelayedPopup));
}

void KoDialogEnumContractTest::captionFlagsRemainStableAliases()
{
    QCOMPARE(int(KoDialog::NoCaptionFlags), 0);
    QCOMPARE(int(KoDialog::AppNameCaption), 1);
    QCOMPARE(int(KoDialog::ModifiedCaption), 2);
    QCOMPARE(int(KoDialog::HIGCompliantCaption), int(KoDialog::AppNameCaption));

    const KoDialog::CaptionFlags flags = KoDialog::HIGCompliantCaption | KoDialog::ModifiedCaption;
    QVERIFY(flags.testFlag(KoDialog::AppNameCaption));
    QVERIFY(flags.testFlag(KoDialog::ModifiedCaption));
}

void KoDialogEnumContractTest::constructionLayoutCaptionAndLifetimeRemainStable()
{
    QWidget parent;
    QPointer<KoDialog> dialog = new KoDialog(&parent);
    QCOMPARE(dialog->parentWidget(), &parent);
    QVERIFY(dialog->mainWidget());
    QCOMPARE(dialog->mainWidget()->parentWidget(), dialog.data());
    QVERIFY(!dialog->isDetailsWidgetVisible());

    dialog->setPlainCaption(QStringLiteral("Plain"));
    QCOMPARE(dialog->windowTitle(), QStringLiteral("Plain"));
    dialog->setCaption(QStringLiteral("Document"));
    QVERIFY(dialog->windowTitle().contains(QStringLiteral("Document")));
    dialog->setCaption(QStringLiteral("Changed"), true);
    QVERIFY(dialog->windowTitle().contains(QStringLiteral("Changed")));
    QVERIFY(!KoDialog::makeStandardCaption(QStringLiteral("Caption"), dialog).isEmpty());
    const int margin = KoDialog::marginHint();
    const int spacing = KoDialog::spacingHint();
    const int groupSpacing = KoDialog::groupSpacingHint();
    QCOMPARE(KoDialog::marginHint(), margin);
    QCOMPARE(KoDialog::spacingHint(), spacing);
    QCOMPARE(KoDialog::groupSpacingHint(), groupSpacing);

    dialog->setInitialSize(QSize(120, 80));
    dialog->incrementInitialSize(QSize(20, 10));
    QVERIFY(dialog->minimumSizeHint().width() >= 0);
    QVERIFY(dialog->sizeHint().height() >= 0);
    delete dialog;
    QVERIFY(dialog.isNull());
}

void KoDialogEnumContractTest::buttonStateAndPresentationRoundTrip()
{
    KoDialog dialog;
    dialog.setButtons(KoDialog::Ok | KoDialog::Apply | KoDialog::Cancel | KoDialog::Help);
    QVERIFY(dialog.button(KoDialog::Ok));
    QVERIFY(dialog.button(KoDialog::Apply));
    QVERIFY(dialog.button(KoDialog::Cancel));
    dialog.setDefaultButton(KoDialog::Apply);
    QCOMPARE(dialog.defaultButton(), KoDialog::Apply);
    dialog.setEscapeButton(KoDialog::Cancel);
    dialog.setButtonFocus(KoDialog::Ok);

    dialog.enableButton(KoDialog::Ok, false);
    QVERIFY(!dialog.isButtonEnabled(KoDialog::Ok));
    dialog.enableButtonOk(true);
    dialog.enableButtonApply(false);
    dialog.enableButtonCancel(false);
    QVERIFY(dialog.isButtonEnabled(KoDialog::Ok));
    QVERIFY(!dialog.isButtonEnabled(KoDialog::Apply));
    QVERIFY(!dialog.isButtonEnabled(KoDialog::Cancel));
    dialog.showButton(KoDialog::Apply, false);
    QVERIFY(dialog.button(KoDialog::Apply)->isHidden());

    const QIcon icon = dialog.style()->standardIcon(QStyle::SP_DialogApplyButton);
    dialog.setButtonText(KoDialog::Ok, QStringLiteral("Proceed"));
    dialog.setButtonIcon(KoDialog::Ok, icon);
    dialog.setButtonToolTip(KoDialog::Ok, QStringLiteral("tip"));
    dialog.setButtonWhatsThis(KoDialog::Ok, QStringLiteral("what"));
    QCOMPARE(dialog.buttonText(KoDialog::Ok), QStringLiteral("Proceed"));
    QCOMPARE(dialog.buttonIcon(KoDialog::Ok).cacheKey(), icon.cacheKey());
    QCOMPARE(dialog.buttonToolTip(KoDialog::Ok), QStringLiteral("tip"));
    QCOMPARE(dialog.buttonWhatsThis(KoDialog::Ok), QStringLiteral("what"));
    dialog.setButtonGuiItem(KoDialog::Ok, KGuiItem(QStringLiteral("Item")));
    QCOMPARE(dialog.buttonText(KoDialog::Ok), QStringLiteral("Item"));
    dialog.setButtonsOrientation(Qt::Vertical);
    dialog.showButtonSeparator(true);
}

void KoDialogEnumContractTest::buttonActivationDispatchesStableSignals()
{
    struct SignalCase {
        KoDialog::ButtonCode code;
        const char *signal;
    };
    const SignalCase cases[] = {
        {KoDialog::Apply, SIGNAL(applyClicked())},
        {KoDialog::Try, SIGNAL(tryClicked())},
        {KoDialog::Default, SIGNAL(defaultClicked())},
        {KoDialog::Reset, SIGNAL(resetClicked())},
        {KoDialog::User1, SIGNAL(user1Clicked())},
        {KoDialog::User2, SIGNAL(user2Clicked())},
        {KoDialog::User3, SIGNAL(user3Clicked())},
        {KoDialog::Help, SIGNAL(helpClicked())},
        {KoDialog::Ok, SIGNAL(okClicked())},
        {KoDialog::Yes, SIGNAL(yesClicked())},
        {KoDialog::No, SIGNAL(noClicked())},
        {KoDialog::Cancel, SIGNAL(cancelClicked())},
        {KoDialog::Close, SIGNAL(closeClicked())},
    };
    for (const SignalCase &entry : cases) {
        KoDialog dialog;
        QSignalSpy specific(&dialog, entry.signal);
        QSignalSpy general(&dialog, &KoDialog::buttonClicked);
        QVERIFY(
            QMetaObject::invokeMethod(&dialog, "slotButtonClicked", Qt::DirectConnection, Q_ARG(int, int(entry.code))));
        QCOMPARE(specific.count(), 1);
        QCOMPARE(general.count(), 1);
        QCOMPARE(general.first().first().toInt(), int(entry.code));
    }
}

void KoDialogEnumContractTest::mainDetailsAndHelpStateRemainIndependent()
{
    KoDialog dialog;
    QWidget *main = new QWidget;
    dialog.setMainWidget(main);
    QCoreApplication::processEvents();
    QCOMPARE(dialog.mainWidget(), main);
    QCOMPARE(main->parentWidget(), &dialog);

    QWidget *details = new QWidget;
    QSignalSpy detailsSpy(&dialog, &KoDialog::aboutToShowDetails);
    dialog.setDetailsWidget(details);
    dialog.setDetailsWidgetVisible(true);
    QVERIFY(dialog.isDetailsWidgetVisible());
    QCOMPARE(detailsSpy.count(), 1);
    dialog.setDetailsWidgetVisible(false);
    QVERIFY(!dialog.isDetailsWidgetVisible());

    dialog.setHelpLinkText(QStringLiteral("Local help"));
    QCOMPARE(dialog.helpLinkText(), QStringLiteral("Local help"));
    dialog.enableLinkedHelp(true);
    dialog.enableLinkedHelp(false);
    dialog.setHelp(QString(), QString());
}

void KoDialogEnumContractTest::visibilityLayoutAndDeferredLifetimeTransitions()
{
    QPointer<KoDialog> dialog = new KoDialog;
    QWidget *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    layout->addWidget(new QWidget(content));
    KoDialog::resizeLayout(layout, 3, 4);
    QCOMPARE(layout->contentsMargins().left(), 3);
    QCOMPARE(layout->spacing(), 4);
    KoDialog::resizeLayout(content, 5, 6);
    QCOMPARE(layout->contentsMargins().left(), 5);
    QCOMPARE(layout->spacing(), 6);
    dialog->setMainWidget(content);

    QSignalSpy hiddenSpy(dialog, &KoDialog::hidden);
    QSignalSpy finishedSpy(dialog, &KoDialog::finished);
    QSignalSpy layoutSpy(dialog, &KoDialog::layoutHintChanged);
    dialog->show();
    QCoreApplication::processEvents();
    dialog->hide();
    QCoreApplication::processEvents();
    QCOMPARE(hiddenSpy.count(), 1);
    QVERIFY(finishedSpy.count() >= 1);
    QCOMPARE(layoutSpy.count(), 0);
    dialog->delayedDestruct();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QVERIFY(dialog.isNull());
}

QTEST_MAIN(KoDialogEnumContractTest)

#include "KoDialogEnumContractTest.moc"

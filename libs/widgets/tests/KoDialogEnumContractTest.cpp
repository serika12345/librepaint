/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoDialog.h>

#include <QTest>

#include <array>
#include <utility>

class KoDialogEnumContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void buttonCodesRemainStableFlags();
    void popupModesRemainStableFlags();
    void captionFlagsRemainStableAliases();
};

void KoDialogEnumContractTest::buttonCodesRemainStableFlags()
{
    const std::array<std::pair<KoDialog::ButtonCode, int>, 16> codes {{
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

QTEST_GUILESS_MAIN(KoDialogEnumContractTest)

#include "KoDialogEnumContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisMediaEncoderFormatPreferencesDialog.h"

#include <QTest>

#include <type_traits>

class KisMediaEncoderPreferencesDialogSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void dialogTypeConstructionAndPreferencesSchemaRemainStable();
};

void KisMediaEncoderPreferencesDialogSchemaContractTest::dialogTypeConstructionAndPreferencesSchemaRemainStable()
{
    using Dialog = KisMediaEncoderPreferencesDialog;

    static_assert(std::is_base_of_v<QDialog, Dialog>);
    static_assert(std::is_constructible_v<Dialog, KisMediaEncoderFormat *, const QVariantMap &, QWidget *>);
    static_assert(std::is_same_v<decltype(&Dialog::preferences), QVariant (Dialog::*)() const>);
}

QTEST_GUILESS_MAIN(KisMediaEncoderPreferencesDialogSchemaContractTest)

#include "KisMediaEncoderPreferencesDialogSchemaContractTest.moc"

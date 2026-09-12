/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dialogs/KisDlgCustomTabletResolution.h"

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_TABLET_RESOLUTION_SIGNATURE(method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisDlgCustomTabletResolution::method)), signature>)
} // namespace

class KisDlgCustomTabletResolutionSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void tabletResolutionDialogSchemaRemainStable();
    void tabletResolutionModeSchemaRemainStable();
    void tabletResolutionConfigurationSchemaRemainStable();
};

void KisDlgCustomTabletResolutionSchemaContractTest::tabletResolutionDialogSchemaRemainStable()
{
    using Dialog = KisDlgCustomTabletResolution;

    static_assert(std::is_class_v<Dialog>);
    static_assert(std::is_base_of_v<QDialog, Dialog>);
    static_assert(std::is_constructible_v<Dialog, QWidget *>);
    static_assert(std::has_virtual_destructor_v<Dialog>);

    QVERIFY(true);
}

void KisDlgCustomTabletResolutionSchemaContractTest::tabletResolutionModeSchemaRemainStable()
{
    using Mode = KisDlgCustomTabletResolution::Mode;

    static_assert(std::is_enum_v<Mode>);
    static_assert(Mode::USE_WINTAB == 0);
    static_assert(Mode::USE_VIRTUAL_SCREEN == 1);
    static_assert(Mode::USE_CUSTOM == 2);

    QVERIFY(true);
}

void KisDlgCustomTabletResolutionSchemaContractTest::tabletResolutionConfigurationSchemaRemainStable()
{
    using Dialog = KisDlgCustomTabletResolution;
    using Mode = Dialog::Mode;

    ASSERT_TABLET_RESOLUTION_SIGNATURE(accept, void (Dialog::*)());
    static_assert(std::is_same_v<decltype(&Dialog::calcNativeScreenRect), QRect (*)()>);
    static_assert(std::is_same_v<decltype(&Dialog::getTabletMode), Mode (*)(QRect *)>);
    static_assert(std::is_same_v<decltype(&Dialog::applyConfiguration), void (*)(Mode, const QRect &)>);

    QVERIFY(true);
}

#undef ASSERT_TABLET_RESOLUTION_SIGNATURE

QTEST_APPLESS_MAIN(KisDlgCustomTabletResolutionSchemaContractTest)

#include "KisDlgCustomTabletResolutionSchemaContractTest.moc"

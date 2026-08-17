/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QSignalSpy>
#include <QTest>

#include <kis_paintop_option.h>

class TestToolSettingsUiContract : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void optionStateAndNotifications();
};

void TestToolSettingsUiContract::optionStateAndNotifications()
{
    KisPaintOpOption option(QStringLiteral("Opacity"), KisPaintOpOption::GENERAL, true);
    QSignalSpy checkedSpy(&option, &KisPaintOpOption::sigCheckedChanged);
    QSignalSpy settingSpy(&option, &KisPaintOpOption::sigSettingChanged);

    QCOMPARE(option.label(), QStringLiteral("Opacity"));
    QCOMPARE(option.category(), KisPaintOpOption::GENERAL);
    QVERIFY(option.isCheckable());
    QVERIFY(option.isChecked());

    option.setChecked(false);

    QVERIFY(!option.isChecked());
    QCOMPARE(checkedSpy.count(), 1);
    QCOMPARE(settingSpy.count(), 1);
}

QTEST_MAIN(TestToolSettingsUiContract)

#include "TestToolSettingsUiContract.moc"

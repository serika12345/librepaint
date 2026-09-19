/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_acs_types.h>

#include <QTest>

class KisAcsTypesTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void mouseButtonsSelectTheActiveColor();
};

void KisAcsTypesTest::mouseButtonsSelectTheActiveColor()
{
    QCOMPARE(Acs::buttonToRole(Qt::LeftButton), Acs::Foreground);
    QCOMPARE(Acs::buttonToRole(Qt::RightButton), Acs::Background);
    QCOMPARE(Acs::buttonsToRole(Qt::RightButton, Qt::LeftButton), Acs::Foreground);
    QCOMPARE(Acs::buttonsToRole(Qt::MiddleButton, Qt::RightButton), Acs::Background);
}

QTEST_GUILESS_MAIN(KisAcsTypesTest)

#include "KisAcsTypesTest.moc"

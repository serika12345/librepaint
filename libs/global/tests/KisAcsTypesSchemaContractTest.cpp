/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <kis_acs_types.h>

#include <QTest>


namespace
{
class ResourceProviderProbe;
class PaintDeviceHandleProbe;
class IteratorProbe;
} // namespace

class KisAcsTypesSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void colorRoleAndMouseButtonMappingRemainStable();
};

void KisAcsTypesSchemaContractTest::colorRoleAndMouseButtonMappingRemainStable()
{


    QCOMPARE(Acs::buttonToRole(Qt::LeftButton), Acs::Foreground);
    QCOMPARE(Acs::buttonToRole(Qt::RightButton), Acs::Background);
    QCOMPARE(Acs::buttonsToRole(Qt::RightButton, Qt::LeftButton), Acs::Foreground);
    QCOMPARE(Acs::buttonsToRole(Qt::MiddleButton, Qt::RightButton), Acs::Background);
}

QTEST_GUILESS_MAIN(KisAcsTypesSchemaContractTest)

#include "KisAcsTypesSchemaContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisPaletteChooser.h>
#include <KisPaletteModel.h>
#include <KoColorPatch.h>
#include <KoColorSetWidget.h>

#include <QTest>

#include <utility>

namespace
{
} // namespace

class KisPaletteModelSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void paletteModelIdentityAndRoleSchemaRemainsStable();
};

void KisPaletteModelSchemaContractTest::paletteModelIdentityAndRoleSchemaRemainsStable()
{

    QCOMPARE(int(KisPaletteModel::IsGroupNameRole), int(Qt::UserRole) + 1);
    QCOMPARE(int(KisPaletteModel::CheckSlotRole), int(Qt::UserRole) + 2);
    QCOMPARE(int(KisPaletteModel::GroupNameRole), int(Qt::UserRole) + 3);
    QCOMPARE(int(KisPaletteModel::RowInGroupRole), int(Qt::UserRole) + 4);
}

QTEST_APPLESS_MAIN(KisPaletteModelSchemaContractTest)

#include "KisPaletteModelSchemaContractTest.moc"

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAngleSelector.h"
#include "kis_aspect_ratio_locker.h"

#include <QTest>

#include <utility>

class AngleBoxProbe;
class SpinBoxProbe;

class KisAngleSelectorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void angleSelectorFlipPresentationSchemaRemainsStable();
};

void KisAngleSelectorSchemaContractTest::angleSelectorFlipPresentationSchemaRemainsStable()
{
    using Selector = KisAngleSelector;
    using Mode = Selector::FlipOptionsMode;


    QCOMPARE(static_cast<int>(Mode::FlipOptionsMode_NoFlipOptions), 0);
    QCOMPARE(static_cast<int>(Mode::FlipOptionsMode_MenuButton), 1);
    QCOMPARE(static_cast<int>(Mode::FlipOptionsMode_Buttons), 2);
    QCOMPARE(static_cast<int>(Mode::FlipOptionsMode_ContextMenu), 3);
}

QTEST_APPLESS_MAIN(KisAngleSelectorSchemaContractTest)

#include "KisAngleSelectorSchemaContractTest.moc"

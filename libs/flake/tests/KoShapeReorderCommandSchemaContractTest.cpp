/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoShapeGroup.h>
#include <commands/KoPathPointTypeCommand.h>
#include <commands/KoPathSegmentTypeCommand.h>
#include <commands/KoShapeGroupCommand.h>
#include <commands/KoShapeReorderCommand.h>
#include <commands/KoShapeUngroupCommand.h>

#include <QDebug>
#include <QTest>

#include <utility>

namespace
{
using IndexedShape = KoShapeReorderCommand::IndexedShape;
using MoveShapeType = KoShapeReorderCommand::MoveShapeType;
using ShapeList = QList<KoShape *>;

} // namespace

class KoShapeReorderCommandSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void shapeReorderMoveTypeValuesRemainStable();
};

void KoShapeReorderCommandSchemaContractTest::shapeReorderMoveTypeValuesRemainStable()
{
    QCOMPARE(int(KoShapeReorderCommand::RaiseShape), 0);
    QCOMPARE(int(KoShapeReorderCommand::LowerShape), 1);
    QCOMPARE(int(KoShapeReorderCommand::BringToFront), 2);
    QCOMPARE(int(KoShapeReorderCommand::SendToBack), 3);
}

QTEST_APPLESS_MAIN(KoShapeReorderCommandSchemaContractTest)

#include "KoShapeReorderCommandSchemaContractTest.moc"

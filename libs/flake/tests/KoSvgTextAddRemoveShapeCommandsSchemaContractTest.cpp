/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <commands/KoShapeMergeTextPropertiesCommand.h>
#include <commands/KoSvgConvertTextTypeCommand.h>
#include <commands/KoSvgTextAddRemoveShapeCommands.h>
#include <commands/KoSvgTextPathInfoChangeCommand.h>

#include <QTest>


class KoSvgTextAddRemoveShapeCommandsSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void svgTextContourTypeValuesRemainStable();
};

void KoSvgTextAddRemoveShapeCommandsSchemaContractTest::svgTextContourTypeValuesRemainStable()
{
    using ContourType = KoSvgTextAddRemoveShapeCommandImpl::ContourType;
    QCOMPARE(int(ContourType::Unknown), 0);
    QCOMPARE(int(ContourType::Inside), 1);
    QCOMPARE(int(ContourType::Subtract), 2);
    QCOMPARE(int(ContourType::TextPath), 3);
}

QTEST_APPLESS_MAIN(KoSvgTextAddRemoveShapeCommandsSchemaContractTest)

#include "KoSvgTextAddRemoveShapeCommandsSchemaContractTest.moc"

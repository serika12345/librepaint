/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_command_ids.h"

#include <QTest>

class KisCommandIdsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void commandIdsPreserveValues_data();
    void commandIdsPreserveValues();
};

void KisCommandIdsContractTest::commandIdsPreserveValues_data()
{
    QTest::addColumn<int>("actual");
    QTest::addColumn<int>("expected");

    using namespace KisCommandUtils;

    QTest::newRow("MoveShapeId") << static_cast<int>(MoveShapeId) << 9999;
    QTest::newRow("ResizeShapeId") << static_cast<int>(ResizeShapeId) << 10000;
    QTest::newRow("TransformShapeId") << static_cast<int>(TransformShapeId) << 10001;
    QTest::newRow("ChangeShapeTransparencyId") << static_cast<int>(ChangeShapeTransparencyId) << 10002;
    QTest::newRow("ChangeShapeBackgroundId") << static_cast<int>(ChangeShapeBackgroundId) << 10003;
    QTest::newRow("ChangeShapeStrokeId") << static_cast<int>(ChangeShapeStrokeId) << 10004;
    QTest::newRow("ChangeShapeMarkersId") << static_cast<int>(ChangeShapeMarkersId) << 10005;
    QTest::newRow("ChangeShapeParameterId") << static_cast<int>(ChangeShapeParameterId) << 10006;
    QTest::newRow("ChangeEllipseShapeId") << static_cast<int>(ChangeEllipseShapeId) << 10007;
    QTest::newRow("ChangeRectangleShapeId") << static_cast<int>(ChangeRectangleShapeId) << 10008;
    QTest::newRow("ChangePathShapePointId") << static_cast<int>(ChangePathShapePointId) << 10009;
    QTest::newRow("ChangePathShapeControlPointId") << static_cast<int>(ChangePathShapeControlPointId) << 10010;
    QTest::newRow("ChangePaletteId") << static_cast<int>(ChangePaletteId) << 10011;
    QTest::newRow("TransformToolId") << static_cast<int>(TransformToolId) << 10012;
    QTest::newRow("ChangeNodeOpacityId") << static_cast<int>(ChangeNodeOpacityId) << 10013;
    QTest::newRow("ChangeNodeNameId") << static_cast<int>(ChangeNodeNameId) << 10014;
    QTest::newRow("ChangeNodeCompositeOpId") << static_cast<int>(ChangeNodeCompositeOpId) << 10015;
    QTest::newRow("ChangeCurrentTimeId") << static_cast<int>(ChangeCurrentTimeId) << 10016;
    QTest::newRow("ChangeCurrentTimeToKeyId") << static_cast<int>(ChangeCurrentTimeToKeyId) << 10017;
    QTest::newRow("DisableUIUpdatesCommandId") << static_cast<int>(DisableUIUpdatesCommandId) << 10018;
    QTest::newRow("UpdateCommandId") << static_cast<int>(UpdateCommandId) << 10019;
    QTest::newRow("EmitImageSignalsCommandId") << static_cast<int>(EmitImageSignalsCommandId) << 10020;
    QTest::newRow("NodePropertyListCommandId") << static_cast<int>(NodePropertyListCommandId) << 10021;
    QTest::newRow("ChangeStoryboardChild") << static_cast<int>(ChangeStoryboardChild) << 10022;
    QTest::newRow("ChangeTransformMaskCommand") << static_cast<int>(ChangeTransformMaskCommand) << 10023;
    QTest::newRow("ChangeProjectionColorCommand") << static_cast<int>(ChangeProjectionColorCommand) << 10024;
    QTest::newRow("SvgInlineSizeChangeCommand") << static_cast<int>(SvgInlineSizeChangeCommand) << 10025;
    QTest::newRow("SvgMoveTextCommand") << static_cast<int>(SvgMoveTextCommand) << 10026;
    QTest::newRow("SvgInsertTextCommand") << static_cast<int>(SvgInsertTextCommand) << 10027;
    QTest::newRow("SvgRemoveTextCommand") << static_cast<int>(SvgRemoveTextCommand) << 10028;
    QTest::newRow("ChangePaintOrderCommand") << static_cast<int>(ChangePaintOrderCommand) << 10029;
    QTest::newRow("SvgTextMergePropertiesRangeCommand") << static_cast<int>(SvgTextMergePropertiesRangeCommand) << 10030;
    QTest::newRow("KoShapeMergeTextPropertiesCommandId") << static_cast<int>(KoShapeMergeTextPropertiesCommandId) << 10031;
    QTest::newRow("ImageAnimSettingCommandId") << static_cast<int>(ImageAnimSettingCommandId) << 10032;
    QTest::newRow("SvgTextChangeTransformsOnRangeCommandId") << static_cast<int>(SvgTextChangeTransformsOnRangeCommandId) << 10033;
    QTest::newRow("SvgTextPathInfoChangeCommandId") << static_cast<int>(SvgTextPathInfoChangeCommandId) << 10034;
}

void KisCommandIdsContractTest::commandIdsPreserveValues()
{
    QFETCH(int, actual);
    QFETCH(int, expected);

    QCOMPARE(actual, expected);
}

QTEST_GUILESS_MAIN(KisCommandIdsContractTest)

#include "KisCommandIdsContractTest.moc"

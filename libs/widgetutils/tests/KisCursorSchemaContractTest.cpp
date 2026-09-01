/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_cursor.h"

#include <QTest>

#include <type_traits>
#include <utility>

namespace
{
#define ASSERT_CURSOR_FACTORY_SIGNATURE(method)                                                                        \
    static_assert(std::is_same_v<decltype(static_cast<QCursor (*)()>(&KisCursor::method)), QCursor (*)()>)
} // namespace

class KisCursorSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void cursorTypeAndStandardFactorySchemaRemainsStable();
    void cursorNavigationAndTransformFactorySchemaRemainsStable();
    void cursorSamplingAndAdjustmentFactorySchemaRemainsStable();
    void cursorMeshFactorySchemaRemainsStable();
    void cursorLoadingSignatureSchemaRemainsStable();
};

void KisCursorSchemaContractTest::cursorTypeAndStandardFactorySchemaRemainsStable()
{
    static_assert(std::is_class_v<KisCursor>);
    static_assert(std::is_default_constructible_v<KisCursor>);

    ASSERT_CURSOR_FACTORY_SIGNATURE(arrowCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(upArrowCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(crossCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(roundCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(pixelBlackCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(pixelWhiteCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(waitCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(ibeamCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(sizeVerCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(sizeHorCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(sizeBDiagCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(sizeFDiagCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(sizeAllCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(blankCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(splitVCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(splitHCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(pointingHandCursor);
}

void KisCursorSchemaContractTest::cursorNavigationAndTransformFactorySchemaRemainsStable()
{
    ASSERT_CURSOR_FACTORY_SIGNATURE(zoomSmoothCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(zoomDiscreteCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(rotateCanvasSmoothCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(rotateCanvasDiscreteCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(triangleLeftHandedCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(triangleRightHandedCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(moveCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(moveSelectionCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(handCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(openHandCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(closedHandCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(rotateCursor);
}

void KisCursorSchemaContractTest::cursorSamplingAndAdjustmentFactorySchemaRemainsStable()
{
    ASSERT_CURSOR_FACTORY_SIGNATURE(samplerImageForegroundCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(samplerImageBackgroundCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(samplerLayerForegroundCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(samplerLayerBackgroundCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(samplerCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(pickLayerCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(changeExposureCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(changeGammaCursor);
    ASSERT_CURSOR_FACTORY_SIGNATURE(eraserCursor);
}

void KisCursorSchemaContractTest::cursorMeshFactorySchemaRemainsStable()
{
    ASSERT_CURSOR_FACTORY_SIGNATURE(meshCursorFree);
    ASSERT_CURSOR_FACTORY_SIGNATURE(meshCursorLocked);
}

void KisCursorSchemaContractTest::cursorLoadingSignatureSchemaRemainsStable()
{
    using Load = QCursor (*)(const QString &, int, int);
    using LoadWithSize = QCursor (*)(const QString &, int, int, int, int);

    static_assert(std::is_same_v<decltype(static_cast<Load>(&KisCursor::load)), Load>);
    static_assert(std::is_same_v<decltype(static_cast<LoadWithSize>(&KisCursor::loadWithSize)), LoadWithSize>);
    static_assert(std::is_same_v<decltype(KisCursor::load(std::declval<const QString &>())), QCursor>);
    static_assert(std::is_same_v<decltype(KisCursor::loadWithSize(std::declval<const QString &>(), 0, 0)), QCursor>);
}

#undef ASSERT_CURSOR_FACTORY_SIGNATURE

QTEST_GUILESS_MAIN(KisCursorSchemaContractTest)

#include "KisCursorSchemaContractTest.moc"

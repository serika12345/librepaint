/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_cursor.h"
#include "kis_icon_utils.h"

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
    void iconStandardSizeSchemaRemainsStable();
    void iconGroupSchemaRemainsStable();
    void iconLoadingThemeAndCacheSignaturesRemainStable();
    void iconObjectAndActionUpdateSignaturesRemainStable();
    void iconContainerUpdateSignaturesRemainStable();
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

void KisCursorSchemaContractTest::iconStandardSizeSchemaRemainsStable()
{
    static_assert(std::is_enum_v<KisIconUtils::StdSizes>);
    static_assert(KisIconUtils::SizeSmall == 16);
    static_assert(KisIconUtils::SizeSmallMedium == 22);
    static_assert(KisIconUtils::SizeMedium == 32);
    static_assert(KisIconUtils::SizeLarge == 48);
    static_assert(KisIconUtils::SizeHuge == 64);
    static_assert(KisIconUtils::SizeEnormous == 128);
    static_assert(std::is_enum_v<KisIconUtils::Group>);
}

void KisCursorSchemaContractTest::iconGroupSchemaRemainsStable()
{
    using Group = KisIconUtils::Group;

    static_assert(int(Group::NoGroup) == -1);
    static_assert(int(Group::Desktop) == 0);
    static_assert(Group::FirstGroup == Group::Desktop);
    static_assert(int(Group::Toolbar) == 1);
    static_assert(int(Group::MainToolbar) == 2);
    static_assert(int(Group::Small) == 3);
    static_assert(int(Group::Panel) == 4);
    static_assert(int(Group::Dialog) == 5);
    static_assert(int(Group::LastGroup) == 6);
    static_assert(int(Group::User) == 7);
}

void KisCursorSchemaContractTest::iconLoadingThemeAndCacheSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisIconUtils::loadIcon), QIcon (*)(const QString &)>);
    static_assert(std::is_same_v<decltype(&KisIconUtils::useDarkIcons), bool (*)()>);
    static_assert(std::is_same_v<decltype(&KisIconUtils::clearIconCache), void (*)()>);
    static_assert(std::is_same_v<decltype(&KisIconUtils::allUniqueLoadedIconNames), QStringList (*)()>);
}

void KisCursorSchemaContractTest::iconObjectAndActionUpdateSignaturesRemainStable()
{
    static_assert(std::is_same_v<decltype(&KisIconUtils::updateIconCommon), void (*)(QObject *)>);
    static_assert(std::is_same_v<decltype(static_cast<void (*)(QAbstractButton *)>(&KisIconUtils::updateIcon)),
                                 void (*)(QAbstractButton *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (*)(QAction *)>(&KisIconUtils::updateIcon)), void (*)(QAction *)>);
}

void KisCursorSchemaContractTest::iconContainerUpdateSignaturesRemainStable()
{
    static_assert(
        std::is_same_v<decltype(static_cast<void (*)(QComboBox *)>(&KisIconUtils::updateIcon)), void (*)(QComboBox *)>);
    static_assert(
        std::is_same_v<decltype(static_cast<void (*)(QTabBar *)>(&KisIconUtils::updateIcon)), void (*)(QTabBar *)>);
}

#undef ASSERT_CURSOR_FACTORY_SIGNATURE

QTEST_GUILESS_MAIN(KisCursorSchemaContractTest)

#include "KisCursorSchemaContractTest.moc"

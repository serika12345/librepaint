/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KoCreatePathTool.h>
#include <KoPencilTool.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_CREATE_PATH_TOOL_SIGNATURE(method, signature)                                                           \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoCreatePathTool::method)), signature>)
#define ASSERT_PENCIL_TOOL_SIGNATURE(method, signature)                                                                \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KoPencilTool::method)), signature>)
} // namespace

class KoPathCreationToolSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void createPathToolTypeLifetimeAndPathStateSchemaRemainStable();
    void createPathToolInputAndRenderingSignaturesRemainStable();
    void pathCreationToolActivationResourceAndNotificationSignaturesRemainStable();
    void pencilToolTypeLifetimeAndRenderingSchemaRemainStable();
    void pencilToolInputSignaturesRemainStable();
};

void KoPathCreationToolSchemaContractTest::createPathToolTypeLifetimeAndPathStateSchemaRemainStable()
{
    static_assert(std::is_class_v<KoCreatePathTool>);
    static_assert(std::is_constructible_v<KoCreatePathTool, KoCanvasBase *>);
    static_assert(std::has_virtual_destructor_v<KoCreatePathTool>);
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(pathStarted, bool (KoCreatePathTool::*)() const);
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(setEnableClosePathShortcut, void (KoCreatePathTool::*)(bool));
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(tryMergeInPathShape, bool (KoCreatePathTool::*)(KoPathShape *));
}

void KoPathCreationToolSchemaContractTest::createPathToolInputAndRenderingSignaturesRemainStable()
{
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(decorationsRect, QRectF (KoCreatePathTool::*)() const);
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(mouseDoubleClickEvent, void (KoCreatePathTool::*)(KoPointerEvent *));
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(mouseMoveEvent, void (KoCreatePathTool::*)(KoPointerEvent *));
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(mousePressEvent, void (KoCreatePathTool::*)(KoPointerEvent *));
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(mouseReleaseEvent, void (KoCreatePathTool::*)(KoPointerEvent *));
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(paint, void (KoCreatePathTool::*)(QPainter &, const KoViewConverter &));
}

void KoPathCreationToolSchemaContractTest::pathCreationToolActivationResourceAndNotificationSignaturesRemainStable()
{
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(activate, void (KoCreatePathTool::*)(const QSet<KoShape *> &));
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(canvasResourceChanged, void (KoCreatePathTool::*)(int, const QVariant &));
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(deactivate, void (KoCreatePathTool::*)());
    ASSERT_CREATE_PATH_TOOL_SIGNATURE(sigUpdateAutoSmoothCurvesGUI, void (KoCreatePathTool::*)(bool));
    ASSERT_PENCIL_TOOL_SIGNATURE(activate, void (KoPencilTool::*)(const QSet<KoShape *> &));
    ASSERT_PENCIL_TOOL_SIGNATURE(deactivate, void (KoPencilTool::*)());
}

void KoPathCreationToolSchemaContractTest::pencilToolTypeLifetimeAndRenderingSchemaRemainStable()
{
    static_assert(std::is_class_v<KoPencilTool>);
    static_assert(std::is_constructible_v<KoPencilTool, KoCanvasBase *>);
    static_assert(std::has_virtual_destructor_v<KoPencilTool>);
    ASSERT_PENCIL_TOOL_SIGNATURE(paint, void (KoPencilTool::*)(QPainter &, const KoViewConverter &));
}

void KoPathCreationToolSchemaContractTest::pencilToolInputSignaturesRemainStable()
{
    ASSERT_PENCIL_TOOL_SIGNATURE(keyPressEvent, void (KoPencilTool::*)(QKeyEvent *));
    ASSERT_PENCIL_TOOL_SIGNATURE(mouseMoveEvent, void (KoPencilTool::*)(KoPointerEvent *));
    ASSERT_PENCIL_TOOL_SIGNATURE(mousePressEvent, void (KoPencilTool::*)(KoPointerEvent *));
    ASSERT_PENCIL_TOOL_SIGNATURE(mouseReleaseEvent, void (KoPencilTool::*)(KoPointerEvent *));
}

QTEST_GUILESS_MAIN(KoPathCreationToolSchemaContractTest)

#include "KoPathCreationToolSchemaContractTest.moc"

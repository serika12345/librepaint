/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <type_traits>

#include <QObject>
#include <QTest>

#include <application/ui/orchestration/KisActionPlugin.h>
#include <application/ui/orchestration/KisApplication.h>
#include <application/ui/orchestration/KisApplicationArguments.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/KisPlatformPluginInterfaceFactory.h>
#include <application/ui/orchestration/KisResourceServerProvider.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/kis_config.h>
#include <tool/kis_bookmarked_configurations_editor.h>
#include <tool/kis_bookmarked_configurations_model.h>
#include <tool/kis_paintop_box.h>
#include <tool/kis_scratch_pad.h>
#include <application/ui/workspace/KisAndroidSplash.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisSessionResource.h>
#include <application/ui/workspace/KisTemplateCreateDia.h>
#include <application/ui/workspace/KisTemplateGroup.h>
#include <application/ui/workspace/KisTemplateTree.h>
#include <application/ui/workspace/KisView.h>
#include <application/ui/workspace/KisViewManager.h>
#include <application/ui/workspace/KisWelcomePageWidget.h>
#include <application/ui/workspace/KisWindowLayoutResource.h>
#include <application/ui/workspace/kis_mainwindow_observer.h>
#include <application/ui/workspace/kis_preference_set_registry.h>
#include <application/ui/workspace/kis_splash_screen.h>
#include <workspace/kis_workspace_resource.h>

static_assert(std::is_base_of_v<QObject, KisApplication>);
static_assert(std::is_base_of_v<QObject, KisMainWindow>);
static_assert(std::is_base_of_v<QObject, KisPaintopBox>);
static_assert(std::is_base_of_v<QWidget, KisScratchPad>);
static_assert(std::is_same_v<decltype(&KisViewManager::activateTransformToolWithExternalSource),
                             void (KisViewManager::*)(KisPaintDeviceSP)>);

class TestApplicationWorkspaceToolUiPublicHeaders : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void headersCompileAsOnePublicSurface();
};

void TestApplicationWorkspaceToolUiPublicHeaders::headersCompileAsOnePublicSurface()
{
    QVERIFY(true);
}

QTEST_MAIN(TestApplicationWorkspaceToolUiPublicHeaders)

#include "TestApplicationWorkspaceToolUiPublicHeaders.moc"

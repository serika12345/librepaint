/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <type_traits>

#include <QObject>
#include <QTest>

#include <application/KisActionPlugin.h>
#include <application/KisApplication.h>
#include <application/KisApplicationArguments.h>
#include <application/KisPart.h>
#include <application/KisPlatformPluginInterfaceFactory.h>
#include <application/KisResourceServerProvider.h>
#include <application/kis_action.h>
#include <application/kis_action_manager.h>
#include <application/kis_config.h>
#include <tool/kis_bookmarked_configurations_editor.h>
#include <tool/kis_bookmarked_configurations_model.h>
#include <tool/kis_paintop_box.h>
#include <tool/kis_scratch_pad.h>
#include <workspace/KisAndroidSplash.h>
#include <workspace/KisMainWindow.h>
#include <workspace/KisSessionResource.h>
#include <workspace/KisTemplateCreateDia.h>
#include <workspace/KisTemplateGroup.h>
#include <workspace/KisTemplateTree.h>
#include <workspace/KisView.h>
#include <workspace/KisViewManager.h>
#include <workspace/KisWelcomePageWidget.h>
#include <workspace/KisWindowLayoutResource.h>
#include <workspace/kis_mainwindow_observer.h>
#include <workspace/kis_preference_set_registry.h>
#include <workspace/kis_splash_screen.h>
#include <workspace/kis_workspace_resource.h>

static_assert(std::is_base_of_v<QObject, KisApplication>);
static_assert(std::is_base_of_v<QObject, KisMainWindow>);
static_assert(std::is_base_of_v<QObject, KisPaintopBox>);
static_assert(std::is_base_of_v<QWidget, KisScratchPad>);

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

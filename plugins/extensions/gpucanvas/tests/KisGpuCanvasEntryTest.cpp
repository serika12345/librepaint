/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDocumentWindow.h"
#include <application/ui/orchestration/KisApplication.h>
#include <application/ui/orchestration/KisPart.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/orchestration/kis_action_manager.h>
#include <application/ui/workspace/KisMainWindow.h>
#include <application/ui/workspace/KisViewManager.h>
#include <opengl/kis_opengl.h>
#include <testui.h>
#include <KLocalizedString>
#include <QFile>
#include <QMenu>
#include <QWindow>

/** The installed view plugin creates a GPU document without a legacy image. */
class KisGpuCanvasEntryTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void newCanvasUsesTheApplicationOwnersAndClosesWithItsParent() {
        auto *application = qobject_cast<KisApplication *>(qApp);
        QVERIFY(application);
        auto *documents = application->gpuDocuments();
        auto *state = application->gpuApplicationState();
        QVERIFY(documents && state);
        QCOMPARE(qobject_cast<KisGpuDocumentDevice *>(static_cast<QObject *>(documents)), documents);
        QCOMPARE(qobject_cast<KisGpuApplicationState *>(static_cast<QObject *>(state)), state);
        QVERIFY(!documents->hasDocuments());
        QVERIFY(!documents->device());
        std::unique_ptr<KisMainWindow> mainWindow(KisPart::instance()->createMainWindow());
        auto *manager = mainWindow->viewManager()->actionManager();
        auto *create = manager->actionByName(QStringLiteral("gpu_canvas_new"));
        auto *open = manager->actionByName(QStringLiteral("gpu_canvas_open"));
        QVERIFY(create && open);
        auto *menu = mainWindow->findChild<QMenu *>(QStringLiteral("gpu_canvas"));
        QVERIFY(menu && menu->actions().contains(create) && menu->actions().contains(open));
        manager->updateGUI();
        QVERIFY(create->isEnabled() && open->isEnabled());
        create->trigger();
        QVERIFY(documents->hasDocuments());
        QPointer<KisGpuDocumentWindow> editor;
        for (auto *widget : QApplication::topLevelWidgets()) {
            if (auto *candidate = qobject_cast<KisGpuDocumentWindow *>(widget)) editor = candidate;
        }
        QVERIFY(editor);
        QVERIFY(editor->findChild<KisGpuCanvasSession *>());
        const auto id = editor->documentId();
        QWindow *native = nullptr;
        for (auto *window : QGuiApplication::allWindows())
            if (window->objectName() == QStringLiteral("gpuDocumentCanvas-%1").arg(id)) native = window;
        QVERIFY(native && QTest::qWaitForWindowExposed(native));
        state->applicationStateChanged(Qt::ApplicationActive);
        const auto model = documents->document(id);
        QVERIFY(model);
        QCOMPARE(model->layers().size(), qsizetype(1));
        QCOMPARE(documents->store(id)->statistics().pixelUploadBytes, quint64(0));
        QCOMPARE(documents->store(id)->statistics().pixelReadbackBytes, quint64(0));
        state->memoryPressure(255);
        QVERIFY(documents->isSuspended());
        QVERIFY(!documents->device());
        QVERIFY(state->resumeForInput());
        QTRY_COMPARE(model->state(), KisGpuLayerDocument::State::Idle);
        QCOMPARE(documents->document(id), model);
        QVERIFY(editor);
        mainWindow.reset();
        QTRY_VERIFY(!editor);
        QVERIFY(!documents->hasDocuments());
        QVERIFY(!documents->device());
    }
};

int main(int argc, char **argv)
{
    qputenv("LANGUAGE", "en");
    QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
    KLocalizedString::setApplicationDomain("krita");
    QStandardPaths::setTestModeEnabled(true);
    qputenv("EXTRA_RESOURCE_DIRS", QByteArray(KRITA_RESOURCE_DIRS_FOR_TESTS));
    qputenv("KRITA_PLUGIN_PATH", QByteArray(KRITA_PLUGINS_DIR_FOR_TESTS));
    KisApplication application(QStringLiteral("gpu-canvas-entry-test"), argc, argv);
    application.setAttribute(Qt::AA_Use96Dpi, true);
    Q_INIT_RESOURCE(krita);
    KisOpenGL::setDefaultSurfaceConfig(KisOpenGL::RendererConfig());
    registerResources();
    auto *resources = KisResourceLocator::instance();
    const auto bundlePath = resources->resourceLocationBase() + QStringLiteral("gpu-menu-default.bundle");
    if (!QFile::link(QStringLiteral(GPU_DEFAULT_RESOURCE_BUNDLE), bundlePath)
        || resources->initialize(resources->resourceLocationBase()) != KisResourceLocator::LocatorError::Ok
        || !resources->hasStorage(bundlePath))
        qFatal("Cannot register the default brush bundle for the product menu test");
    KoResourcePaths::addAssetDir(QStringLiteral("kis_actions"), QStringLiteral(GPU_ACTION_DIRECTORY));
    KisGpuCanvasEntryTest test;
    const auto result = QTest::qExec(&test, argc, argv);
    resources->removeStorage(bundlePath);
    QFile::remove(bundlePath);
    return result;
}
#include "KisGpuCanvasEntryTest.moc"

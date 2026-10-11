/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <application/ui/orchestration/KisApplication.h>
#include <application/ui/orchestration/KisActionPlugin.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/workspace/KisViewManager.h>
#include "KisGpuCanvasWorkspace.h"
#include "KisGpuDisplayColor.h"
#include <canvas/KisDisplayConfig.h>
#include <application/kis_config.h>
#include <KoColorSpaceRegistry.h>
#include <kis_config_notifier.h>
#include <QGuiApplication>
#include <QScreen>
#include <QStatusBar>
#include <QWindow>
#include <kpluginfactory.h>
#include <QFileDialog>
#include <QMessageBox>
#include <QPointer>

class KisGpuCanvasExtension : public KisActionPlugin {
    Q_OBJECT
public:
    KisGpuCanvasExtension(QObject *parent, const QVariantList &)
        : KisActionPlugin(parent)
    {
        auto *application = qobject_cast<KisApplication *>(qApp);
        if (!application || !application->gpuDocuments() || !application->gpuApplicationState() || !viewManager()) return;
        m_workspace = std::make_unique<KisGpuCanvasWorkspace>(*application->gpuDocuments(),
            *application->gpuApplicationState(), viewManager()->mainWindowAsQWidget(), this);
        auto *create = createAction(QStringLiteral("gpu_canvas_new"));
        connect(create, &QAction::triggered, this, [this] {
            if (auto *window = m_workspace->createCanvas({1024, 1024})) attachDisplayColor(*window);
            else reportError();
        });
        auto *open = createAction(QStringLiteral("gpu_canvas_open"));
        connect(open, &QAction::triggered, this, [this] {
            const QPointer<KisGpuCanvasExtension> extension(this);
            const auto path = QFileDialog::getOpenFileName(viewManager()->mainWindowAsQWidget(),
                tr("Open GPU Canvas"), {}, tr("KRA documents (*.kra)"));
            if (!extension || path.isEmpty()) return;
            auto *window = m_workspace->openCanvas(path);
            if (extension && window) attachDisplayColor(*window);
            else if (extension) reportError();
        });
    }
private:
    static void attachDisplayColor(KisGpuDocumentWindow &window)
    {
        auto *registry = KoColorSpaceRegistry::instance();
        const auto *source = registry->rgb8(registry->p709SRGBProfile());
        auto *color = new KisGpuDisplayColor(window, source);
        auto *editingWindow = &window;
        const auto update = [color, editingWindow, source] {
            const KisConfig config(true);
            const auto options = KisDisplayConfig::optionsFromKisConfig(config);
            const auto *destination = source;
#ifndef Q_OS_MACOS
            KisDisplayConfig::initializeSystemColorManager();
            const int screen = QGuiApplication::screens().indexOf(editingWindow->screen());
            if (const auto *profile = KisDisplayConfig::profileForScreen(screen))
                destination = KoColorSpaceRegistry::instance()->rgb8(profile);
#endif
            if (!color->request(destination, options.first, options.second))
                editingWindow->statusBar()->showMessage(tr("The display color profile could not be used."));
        };
        connect(color, &KisGpuDisplayColor::failed, color, [editingWindow] {
            editingWindow->statusBar()->showMessage(tr("The display color conversion could not be prepared."));
        });
        connect(KisConfigNotifier::instance(), &KisConfigNotifier::configChanged, color, update);
        if (auto *handle = window.windowHandle())
            connect(handle, &QWindow::screenChanged, color, [update](QScreen *) { update(); });
        update();
    }
    void reportError()
    {
        const auto message = m_workspace->lastError();
        if (!message.isEmpty()) QMessageBox::warning(viewManager()->mainWindowAsQWidget(), tr("GPU Canvas"), message);
    }
    std::unique_ptr<KisGpuCanvasWorkspace> m_workspace;
};

K_PLUGIN_FACTORY_WITH_JSON(KisGpuCanvasFactory, "kritagpucanvas.json", registerPlugin<KisGpuCanvasExtension>();)
#include "KisGpuCanvasExtension.moc"

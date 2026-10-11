/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include <application/ui/orchestration/KisApplication.h>
#include <application/ui/orchestration/KisActionPlugin.h>
#include <application/ui/orchestration/kis_action.h>
#include <application/ui/workspace/KisViewManager.h>
#include "KisGpuCanvasWorkspace.h"
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
            if (!m_workspace->createCanvas({1024, 1024})) reportError();
        });
        auto *open = createAction(QStringLiteral("gpu_canvas_open"));
        connect(open, &QAction::triggered, this, [this] {
            const QPointer<KisGpuCanvasExtension> extension(this);
            const auto path = QFileDialog::getOpenFileName(viewManager()->mainWindowAsQWidget(),
                tr("Open GPU Canvas"), {}, tr("KRA documents (*.kra)"));
            if (!extension || path.isEmpty()) return;
            const auto *window = m_workspace->openCanvas(path);
            if (extension && !window) reportError();
        });
    }
private:
    void reportError()
    {
        const auto message = m_workspace->lastError();
        if (!message.isEmpty()) QMessageBox::warning(viewManager()->mainWindowAsQWidget(), tr("GPU Canvas"), message);
    }
    std::unique_ptr<KisGpuCanvasWorkspace> m_workspace;
};

K_PLUGIN_FACTORY_WITH_JSON(KisGpuCanvasFactory, "kritagpucanvas.json", registerPlugin<KisGpuCanvasExtension>();)
#include "KisGpuCanvasExtension.moc"

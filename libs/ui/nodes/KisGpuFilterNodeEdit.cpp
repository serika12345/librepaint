/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuFilterNodeEdit.h"
#include "dialogs/kis_dlg_adjustment_layer.h"
#include "application/ui/workspace/KisViewManager.h"
#include "nodes/kis_node_manager.h"
#include <KisGpuFilterConfiguration.h>
#include <gpu/KisGpuPaintImage.h>
#include <kis_image.h>
#include <kis_group_layer.h>
#include <kis_node.h>
#include <kis_node_filter_interface.h>
#include <kis_paint_device.h>
#include <filter/kis_filter_configuration.h>
#include <klocalizedstring.h>
#include <QApplication>
#include <QPointer>

namespace {
KisNodeSP runFilterNodeDialog(KisViewManager *view, KisNodeSP existing, KisNodeSP parent,
    qsizetype index, LibrePaint::Image::Gpu::LayerKind kind, const QString &name,
    const KisFilterConfigurationSP &initial, bool quiet)
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!view) return {};
    KisImageSP image = view->image();
    const auto owner = image ? image->gpuImage() : Gpu::PaintImageSP{};
    if (!owner || owner->operations().hasActiveStroke() ||
        !Gpu::isFilterNode(kind) || (!existing && (!parent || parent->image().data() != image.data()))) return {};
    const QPointer<KisViewManager> retainedView(view);
    Gpu::RasterLayer properties;
    properties.uuid = QUuid::createUuid();
    properties.kind = kind;
    properties.name = name;
    properties.blendMode = kind == Gpu::LayerKind::Adjustment ? Gpu::LayerBlendMode::Copy : Gpu::LayerBlendMode::Normal;
    const bool mask = kind == Gpu::LayerKind::FilterMask;
    KisPaintDeviceSP preview = new KisPaintDevice(*(mask ? parent->original() : image->projection()));
    KisDlgAdjustmentLayer dialog({}, nullptr, preview, name,
        existing ? (mask ? i18n("Filter Mask Properties") : i18n("Filter Layer Properties")) :
                   (mask ? i18n("New Filter Mask") : i18n("New Filter Layer")),
        view, qApp->activeWindow());
    preview.clear();
    Gpu::OperationId operation;
    Gpu::LayerId target = 0;
    bool previewAccepted = false;
    const auto stillAttached = [&] {
        return retainedView && retainedView->image().data() == image.data() && image->gpuImage() == owner;
    };
    const auto cancel = [&] {
        if (!operation.isNull()) owner->cancelFilterNodeEdit(operation);
        operation = {};
        target = 0;
        previewAccepted = false;
    };
    QObject::connect(&dialog, &KisDlgAdjustmentLayer::configurationPreviewRequested, &dialog,
        [&](const KisFilterConfigurationSP &configuration, const QString &updatedName) {
        if (!stillAttached()) { cancel(); dialog.setPreviewAccepted(false); return; }
        const auto frozen = freezeGpuFilterConfiguration(configuration, image->colorSpace());
        if (!frozen) {
            cancel();
            dialog.setPreviewAccepted(false, i18n("This filter or its settings are not yet available for this GPU document."));
            return;
        }
        if (operation.isNull()) {
            properties.filter = *frozen;
            properties.name = updatedName;
            operation = image->beginGpuFilterNodeEdit(existing, properties, parent, index);
            target = owner->operations().filterNodeEditTarget();
        }
        const auto result = operation.isNull() ? Gpu::Result::LayerUnavailable :
            owner->updateFilterNodeEdit(operation, *frozen, updatedName);
        previewAccepted = result == Gpu::Result::Success;
        dialog.setPreviewAccepted(previewAccepted,
            result == Gpu::Result::Success ? QString{} : i18n("The filter preview could not be updated. The document is unchanged."));
    });
    if (initial) dialog.setInitialConfiguration(initial, bool(existing));
    else dialog.requestPreview();
    dialog.resize(dialog.minimumSizeHint());
    const bool accepted = quiet ? previewAccepted : dialog.exec() == QDialog::Accepted;
    if (!accepted || !previewAccepted || !stillAttached() || operation.isNull()) { cancel(); return {}; }
    if (owner->commitFilterNodeEdit(operation) != Gpu::Result::Success) { cancel(); return {}; }
    operation = {};
    QUuid uuid;
    for (const auto &layer : owner->operations().snapshot()->layers)
        if (layer.id == target) { uuid = layer.uuid; break; }
    QList<KisNodeSP> nodes{image->root()};
    for (qsizetype i = 0; i < nodes.size(); ++i) {
        const auto node = nodes[i];
        if (node->uuid() == uuid) {
            if (stillAttached()) retainedView->nodeManager()->slotNonUiActivatedNode(node);
            return node;
        }
        for (auto child = node->firstChild(); child; child = child->nextSibling()) nodes.append(child);
    }
    return {};
}
}

KisNodeSP createGpuFilterNode(KisViewManager *view, KisNodeSP parent, qsizetype index,
    LibrePaint::Image::Gpu::LayerKind kind, const QString &name,
    const KisFilterConfigurationSP &initial, bool quiet)
{
    return runFilterNodeDialog(view, {}, parent, index, kind, name, initial, quiet);
}

void editGpuFilterNode(KisViewManager *view, KisNodeSP node)
{
    if (!node || !node->parent()) return;
    auto *filter = dynamic_cast<KisNodeFilterInterface *>(node.data());
    if (!filter || !filter->filter()) return;
    const auto kind = node->inherits("KisFilterMask") ? LibrePaint::Image::Gpu::LayerKind::FilterMask :
                                                     LibrePaint::Image::Gpu::LayerKind::Adjustment;
    runFilterNodeDialog(view, node, node->parent(), 0, kind, node->name(), filter->filter(), false);
}

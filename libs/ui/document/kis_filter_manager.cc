/*
 *  SPDX-FileCopyrightText: 2007 Boudewijn Rempt <boud@valdyas.org>
 *  SPDX-FileCopyrightText: 2007 Cyrille Berger <cberger@cberger.net>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "document/kis_filter_manager.h"
#include <klocalizedstring.h>


#include <array>
#include <cstring>
#include <stdexcept>
#include <KoColor.h>
#include <KoColorModelStandardIds.h>
#include <KoColorSpaceRegistry.h>
#include <KisGpuBlendMode.h>
#include <KisGpuFilterConfiguration.h>
#include <KisGpuCpuFilter.h>
#include <QHash>
#include <QPointer>
#include <KisSignalMapper.h>

#include <QMessageBox>
#include <kactionmenu.h>
#include <kactioncollection.h>
#include <KConfigGroup>
#include <KSharedConfig>

#include <KoID.h>
#include <application/ui/workspace/KisMainWindow.h>

// krita/image
#include <filter/kis_filter.h>
#include <filter/kis_filter_registry.h>
#include <filter/kis_filter_configuration.h>
#include <kis_config_widget.h>
#include <gpu/KisGpuOperationDocument.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuOperationCapabilities.h>
#include <kis_paint_device.h>
#include <kis_image_animation_interface.h>
#include <kis_time_span.h>
#include <kis_image_config.h>
#include <KisAnimAutoKey.h>

// krita/ui
#include "KisQStringListFwd.h"
#include "KoColorSpace.h"
#include "application/ui/workspace/KisViewManager.h"
#include "kis_canvas2.h"
#include <kis_bookmarked_configuration_manager.h>

#include "application/ui/orchestration/kis_action.h"
#include "application/ui/orchestration/kis_action_manager.h"
#include "canvas/kis_canvas_resource_provider.h"
#include "dialogs/kis_dlg_filter.h"
#include "kis_debug.h"
#include "kis_resources_snapshot.h"
#include "kis_types.h"
#include <kis_node.h>
#include "kundo2magicstring.h"
#include "strokes/kis_filter_stroke_strategy.h"
#include "kis_icon_utils.h"
#include "kis_layer_utils.h"
#include <KisGlobalResourcesInterface.h>
#include <kis_image.h>
#include <kis_floating_message.h>
#include <QtGlobal>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <qpointer.h>
#include <qset.h>

namespace {
/** Product HSV/HSL Adjustment keys translated to the GPU operation values.
 *
 * The product filter's `defaultConfiguration()` supplies h = s = v = 0,
 * type = 1 (HSL) and compatibilityMode = false. All five product modes and
 * the Colorize option are representable: the GPU rule freezes the effective
 * document luma coefficients once, at apply time, instead of reading a live
 * color-space object while filtering tiles. Invalid modes and values outside
 * the product slider range for the selected mode are diagnosed without
 * starting the CPU filter.
 */
struct GpuHsvConfiguration
{
    LibrePaint::Image::Gpu::HsvAdjustment settings;
    bool valid = false;
    QString diagnostic;
};

GpuHsvConfiguration gpuHsvConfiguration(KisFilterConfigurationSP config,
                                        const KoColorSpace *colorSpace)
{
    GpuHsvConfiguration result;
    if (!config) return result;
    const int type = config->getInt("type", 1);
    const bool colorize = config->getBool("colorize", false);
    // The product transformation treats a missing key as legacy mode; its
    // default configuration writes the key as false, so the dialog path stays
    // in the modern rule.
    const bool compatibilityMode = config->getBool("compatibilityMode", true);
    const int hue = config->getInt("h", 0);
    const int saturation = config->getInt("s", 0);
    const int value = config->getInt("v", 0);
    if (type < 0 || type > 4) {
        result.diagnostic = i18n("This HSV Adjustment mode is not available for GPU documents.");
        return result;
    }
    const bool yuv = type == 4;
    if (colorize) {
        // Colorize shows the 0..360 hue slider for types 0 through 3 and the
        // 0..100 Yellow-Blue slider for YUV.
        if (hue < 0 || hue > (yuv ? 100 : 360) || saturation < 0 || saturation > 100 ||
            value < -100 || value > 100) {
            result.diagnostic = i18n("HSV Adjustment values are outside the product slider ranges.");
            return result;
        }
    } else if (hue < (yuv ? -100 : -180) || hue > (yuv ? 100 : 180) ||
               saturation < -100 || saturation > 100 || value < -100 || value > 100) {
        result.diagnostic = i18n("HSV Adjustment values are outside the product slider ranges.");
        return result;
    }
    using LibrePaint::Image::Gpu::HsvAdjustmentType;
    result.settings = {hue, saturation, value, HsvAdjustmentType(type), compatibilityMode};
    result.settings.colorize = colorize;
    if (result.settings.needsLuma()) {
        const QVector<qreal> coefficients = colorSpace ? colorSpace->lumaCoefficients()
                                                       : QVector<qreal>();
        std::array<qreal, 3> source{};
        if (coefficients.size() < 3) {
            result.diagnostic = i18n("The document color space does not provide HSV Adjustment luma coefficients.");
            return result;
        }
        source = {coefficients[0], coefficients[1], coefficients[2]};
        if (!LibrePaint::Image::Gpu::freezeLumaCoefficients(source, &result.settings.luma)) {
            result.diagnostic = i18n("The document color space does not provide usable HSV Adjustment luma coefficients.");
            return result;
        }
        result.settings.lumaFrozen = true;
    }
    result.valid = true;
    return result;
}

void showGpuDiagnostic(KisViewManager *view, const QString &message)
{
    view->showFloatingMessage(message, QIcon(), 2000, KisFloatingMessage::Low);
}
}

struct KisFilterManager::Private {
    KisAction* reapplyAction = nullptr;
    KisAction* reapplyActionReprompt = nullptr;
    QHash<QString, KActionMenu*> filterActionMenus;
    QHash<KisFilter*, QAction *> filters2Action;
    KisKActionCollection *actionCollection = nullptr;
    KisActionManager *actionManager = nullptr;
    KisViewManager *view = nullptr;

    KisFilterConfigurationSP lastConfiguration;
    KisFilterConfigurationSP currentlyAppliedConfiguration;
    KisStrokeId currentStrokeId;
    KisFilterStrokeStrategy::ExternalCancelUpdatesStorageSP externalCancelUpdatesStorage;
    KisFilterStrokeStrategy::IdleBarrierData::IdleBarrierCookie idleBarrierCookie;

    bool filterAllSelectedFrames = false;

    KisSignalMapper actionsMapper;

    /*!
     * \brief The filter dialog shown to the user
     * \note parent QWidget is set to mainwindow, so we delegate deletion of this widget to Qt (we don't `delete` it ourselves)
     */
    KisDlgFilter *filterDialog = nullptr;
    KisImageSP gpuFilterImage;
    KisNodeSP gpuFilterNode;
    KisPaintDeviceSP gpuFilterSource;
    LibrePaint::Image::Gpu::PaintImageSP gpuFilterOwner;
    LibrePaint::Image::Gpu::Snapshot gpuFilterBefore;
    LibrePaint::Image::Gpu::OperationId gpuFilterOperation;
};

KisFilterManager::KisFilterManager(KisViewManager * view)
    : d(new Private)
{
    d->view = view;
}

KisFilterManager::~KisFilterManager()
{
    if (d->gpuFilterOwner && !d->gpuFilterOperation.isNull())
        d->gpuFilterOwner->cancelPixelEdit(d->gpuFilterOperation);
}

bool KisFilterManager::applyGpuFilter(const QString &filterId, KisFilterConfigurationSP config, bool interactive)
{
    if (!d->view) return false;
    KisImageWSP image = d->view->image();
    if (!image || !image->gpuImage()) return false;
    auto filter = KisFilterRegistry::instance()->value(filterId);
    if (!filter) {
        showGpuDiagnostic(d->view, i18n("The requested filter could not be found."));
        return true;
    }
    if (!d->view->activeNode() || !d->view->activeNode()->isEditable()) {
        showGpuDiagnostic(d->view, i18n("Cannot apply filter to locked layer."));
        return true;
    }
    namespace Gpu = LibrePaint::Image::Gpu;
    const auto device = d->view->activeNode()->paintDevice();
    const auto target = device && device->pixelSize() == 1 ? Gpu::FilterTarget::Alpha : Gpu::FilterTarget::Rgba;
    const bool cpu = Gpu::filterExecutionLocation(filterId, target) == Gpu::ExecutionLocation::Cpu;
    if (interactive && ((cpu && filter->showConfigurationWidget()) ||
                        filterId == QLatin1String("hsvadjustment") ||
                        filterId == QLatin1String("fastcoloroverlay"))) showGpuFilterDialog(filterId, config);
    else applyGpuConfiguration(config ? config : filter->defaultConfiguration(KisGlobalResourcesInterface::instance()));
    return true;
}

bool KisFilterManager::applyGpuConfiguration(KisFilterConfigurationSP config, bool remember)
{
    if (!d->view || !config) return false;
    KisImageWSP image = d->view->image();
    if (!image || !image->gpuImage()) return false;
    namespace Gpu = LibrePaint::Image::Gpu;
    const QString id = config->name();
    bool applied = false;
    const bool convolution = id == QLatin1String("sharpen") || id == QLatin1String("mean removal") ||
        id == QLatin1String("emboss laplascian") || id == QLatin1String("emboss all directions") ||
        id == QLatin1String("emboss horizontal and vertical") || id == QLatin1String("emboss vertical only") ||
        id == QLatin1String("emboss horizontal only");
    const auto device = d->view->activeNode() ? d->view->activeNode()->paintDevice() : KisPaintDeviceSP{};
    const auto target = device && device->pixelSize() == 1 ? Gpu::FilterTarget::Alpha : Gpu::FilterTarget::Rgba;
    if (Gpu::filterExecutionLocation(id, target) == Gpu::ExecutionLocation::Cpu) {
        applied = kisGpuApplyCpuFilter(image.data(), d->view->activeNode(), config,
            remember ? KisPaintDeviceSP{} : d->gpuFilterSource);
    } else if (convolution) {
        const auto frozen = freezeGpuFilterConfiguration(config, image->colorSpace());
        if (!frozen) return false;
        applied = image->convolutionFilterGpuLayerColors(d->view->activeNode(), frozen->settings);
    } else if (id == QLatin1String("hsvadjustment")) {
        const auto parsed = gpuHsvConfiguration(config, image->colorSpace());
        if (!parsed.valid) {
            showGpuDiagnostic(d->view, parsed.diagnostic);
            return false;
        }
        applied = image->hsvAdjustmentGpuLayerColors(d->view->activeNode(), parsed.settings);
    } else if (id == QLatin1String("invert")) {
        applied = image->invertGpuLayerColors(d->view->activeNode());
    } else if (id == QLatin1String("desaturate")) {
        applied = image->grayscaleGpuLayerColors(d->view->activeNode());
    } else {
        Gpu::FilterKind kind;
        Gpu::ColorOverlay overlay;
        if (id == QLatin1String("maximize")) kind = Gpu::FilterKind::MaximizeChannel;
        else if (id == QLatin1String("minimize")) kind = Gpu::FilterKind::MinimizeChannel;
        else if (id == QLatin1String("resettransparent")) kind = Gpu::FilterKind::ResetTransparent;
        else if (id == QLatin1String("fastcoloroverlay")) {
            kind = Gpu::FilterKind::ColorOverlay;
            const auto mode = Gpu::layerBlendModeFromCompositeOp(config->getString("compositeop", COMPOSITE_OVER));
            overlay.opacity = config->getInt("opacity", 100);
            if (!mode || overlay.opacity < 0 || overlay.opacity > 100) {
                showGpuDiagnostic(d->view, i18n("This color overlay composition or opacity is not available for GPU documents."));
                return false;
            }
            overlay.blendMode = *mode;
            KoColor color = config->getColor("color", KoColor(QColor(62, 140, 236), KoColorSpaceRegistry::instance()->rgb8()));
            color.convertTo(image->colorSpace());
            if (image->colorSpace()->colorDepthId() == Integer16BitsColorDepthID) {
                quint16 bgra[4];
                std::memcpy(bgra, color.data(), sizeof(bgra));
                overlay.color = {bgra[2], bgra[1], bgra[0], bgra[3]};
            } else {
                const auto *bgra = color.data();
                overlay.color = {bgra[2], bgra[1], bgra[0], bgra[3]};
            }
        } else return false;
        applied = image->colorFilterGpuLayerColors(d->view->activeNode(), kind, overlay);
    }
    if (!applied) {
        showGpuDiagnostic(d->view, i18n("This filter requires an editable paint layer or mask and supported document precision."));
        return false;
    }
    if (remember) rememberGpuConfiguration(config);
    return true;
}

void KisFilterManager::rememberGpuConfiguration(KisFilterConfigurationSP config)
{
    if (KisFilterSP filter = KisFilterRegistry::instance()->value(config->name())) {
        if (filter->bookmarkManager())
            filter->bookmarkManager()->save(KisBookmarkedConfigurationManager::ConfigLastUsed, config.data());
    }
    d->lastConfiguration = config->clone();
    if (d->reapplyAction) {
        d->reapplyAction->setEnabled(true);
        if (d->reapplyActionReprompt) d->reapplyActionReprompt->setEnabled(true);
        if (KisFilterSP filter = KisFilterRegistry::instance()->value(config->name()))
            d->reapplyAction->setText(i18n("Apply Filter Again: %1", filter->name()));
    }
}

void KisFilterManager::showGpuFilterDialog(const QString &filterId, KisFilterConfigurationSP config)
{
    if (!d->view || !d->view->activeNode()) return;
    if (d->filterDialog) {
        d->filterDialog->setFilter(KisFilterRegistry::instance()->value(filterId), config);
        d->filterDialog->raise();
        d->filterDialog->activateWindow();
        return;
    }
    if (!d->view->blockUntilOperationsFinished(d->view->image())) return;
    KisPaintDeviceSP configurationDevice;
    try {
        configurationDevice = kisGpuFilterConfigurationDevice(d->view->image().data(), d->view->activeNode());
    } catch (const std::exception &) {
        showGpuDiagnostic(d->view, i18n("The layer pixels could not be read for the filter settings."));
        return;
    }
    if (!configurationDevice) return;
    d->gpuFilterImage = d->view->image();
    d->gpuFilterNode = d->view->activeNode();
    d->gpuFilterSource = configurationDevice;
    d->gpuFilterOwner = d->gpuFilterImage->gpuImage();
    d->gpuFilterBefore = d->gpuFilterOwner->operations().snapshot();
    d->filterDialog = new KisDlgFilter(d->view, d->gpuFilterNode, this, d->view->mainWindow(), configurationDevice);
    d->filterDialog->setObjectName(filterId == QLatin1String("hsvadjustment") ? QStringLiteral("gpuHsvAdjustmentDialog") :
        filterId == QLatin1String("fastcoloroverlay") ? QStringLiteral("gpuColorOverlayDialog") : QStringLiteral("gpuFilterDialog"));
    d->filterDialog->setAttribute(Qt::WA_DeleteOnClose);
    connect(d->filterDialog, &QDialog::finished, this, &KisFilterManager::filterDialogHasFinished);
    connect(d->gpuFilterImage, &KisImage::sigStrokeCancellationRequested,
        this, &KisFilterManager::slotStrokeCancelRequested, Qt::UniqueConnection);
    connect(d->gpuFilterImage, &KisImage::sigStrokeEndRequested,
        this, &KisFilterManager::slotStrokeEndRequested, Qt::UniqueConnection);
    d->filterDialog->setFilter(KisFilterRegistry::instance()->value(filterId), config);
    d->filterDialog->show();
}

void KisFilterManager::setView(QPointer<KisView>imageView)
{
    if (d->gpuFilterOwner && d->filterDialog && (!imageView || imageView->image().data() != d->gpuFilterImage.data()))
        d->filterDialog->reject();
}


void KisFilterManager::setup(KisKActionCollection * ac, KisActionManager *actionManager)
{
    d->actionCollection = ac;
    d->actionManager = actionManager;

    // Setup reapply action
    d->reapplyAction = d->actionManager->createAction("filter_apply_again");
    d->reapplyAction->setActivationFlags(KisAction::ACTIVE_DEVICE);
    d->reapplyAction->setEnabled(false);

    d->reapplyActionReprompt = d->actionManager->createAction("filter_apply_reprompt");
    d->reapplyActionReprompt->setActivationFlags(KisAction::ACTIVE_DEVICE);
    d->reapplyActionReprompt->setEnabled(false);

    connect(d->reapplyAction, SIGNAL(triggered()), SLOT(reapplyLastFilter()));
    connect(d->reapplyActionReprompt, SIGNAL(triggered()), SLOT(reapplyLastFilterReprompt()));

    connect(&d->actionsMapper, SIGNAL(mapped(QString)), SLOT(showFilterDialog(QString)));

    // Setup list of filters
    QStringList keys = KisFilterRegistry::instance()->keys();
    keys.sort();
    Q_FOREACH (const QString &filterName, keys) {
        insertFilter(filterName);
    }

    connect(KisFilterRegistry::instance(), SIGNAL(filterAdded(QString)), SLOT(insertFilter(QString)));
}

void KisFilterManager::insertFilter(const QString & filterName)
{
    Q_ASSERT(d->actionCollection);

    KisFilterSP filter = KisFilterRegistry::instance()->value(filterName);
    Q_ASSERT(filter);

    if (d->filters2Action.contains(filter.data())) {
        warnKrita << "Filter" << filterName << " has already been inserted";
        return;
    }

    KoID category = filter->menuCategory();
    KActionMenu* actionMenu = d->filterActionMenus[ category.id()];
    if (!actionMenu) {
        actionMenu = new KActionMenu(category.name(), this);
        d->actionCollection->addAction(category.id(), actionMenu);
        d->filterActionMenus[category.id()] = actionMenu;
    }

    KisAction *action = new KisAction(filter->menuEntry(), this);
    action->setDefaultShortcut(filter->shortcut());
    action->setActivationFlags(KisAction::ACTIVE_DEVICE);

    d->actionManager->addAction(QString("krita_filter_%1").arg(filterName), action);
    d->filters2Action[filter.data()] = action;

    actionMenu->addAction(action);

    d->actionsMapper.setMapping(action, filterName);
    connect(action, SIGNAL(triggered()), &d->actionsMapper, SLOT(map()));
}

void KisFilterManager::updateGUI()
{
    if (!d->view) return;

    bool enable = false;

    KisNodeSP activeNode = d->view->activeNode();
    enable = activeNode && activeNode->hasEditablePaintDevice();

    d->reapplyAction->setEnabled(enable);

    for (QHash<KisFilter*, QAction *>::iterator it = d->filters2Action.begin();
            it != d->filters2Action.end(); ++it) {

        bool localEnable = enable;

        it.value()->setEnabled(localEnable);
    }
}

void KisFilterManager::reapplyLastFilter()
{
    if (!d->lastConfiguration) return;

    // A GPU document owns its pixels in the operation document. Reapply must
    // take the same intercepted path as the action instead of starting a CPU
    // stroke and finishing a filter that never ran.
    if (applyGpuFilter(d->lastConfiguration->name(), d->lastConfiguration, false)) return;
    apply(d->lastConfiguration);
    finish();
}

void KisFilterManager::reapplyLastFilterReprompt()
{
    if (!d->lastConfiguration) return;

    showFilterDialog(d->lastConfiguration->name(), d->lastConfiguration);
}

void KisFilterManager::showFilterDialog(const QString &filterId, KisFilterConfigurationSP overrideDefaultConfig)
{
    if (applyGpuFilter(filterId, overrideDefaultConfig, true)) return;

    if (!d->view->activeNode()->isEditable()) {
        d->view->showFloatingMessage(i18n("Cannot apply filter to locked layer."),
                                      KisIconUtils::loadIcon("object-locked"));
        return;
    }

    if (d->filterDialog && d->filterDialog->isVisible()) {
        KisFilterSP filter = KisFilterRegistry::instance()->value(filterId);
        d->filterDialog->setFilter(filter, overrideDefaultConfig);
        return;
    }

    connect(d->view->image(),
            SIGNAL(sigStrokeCancellationRequested()),
            SLOT(slotStrokeCancelRequested()),
            Qt::UniqueConnection);

    connect(d->view->image(),
            SIGNAL(sigStrokeEndRequested()),
            SLOT(slotStrokeEndRequested()),
            Qt::UniqueConnection);

    /**
     * The UI should show only after every running stroke is finished,
     * so a virtual barrier is added here.
     */
    if (!d->view->blockUntilOperationsFinished(d->view->image())) {
        return;
    }

    Q_ASSERT(d->view);
    Q_ASSERT(d->view->activeNode());

    KisPaintDeviceSP dev = d->view->activeNode()->paintDevice();
    if (!dev) {
        warnKrita << "KisFilterManager::showFilterDialog(): Filtering was requested for illegal active layer!" << d->view->activeNode();
        return;
    }

    KisFilterSP filter = KisFilterRegistry::instance()->value(filterId);

    if (dev->colorSpace()->willDegrade(filter->colorSpaceIndependence())) {
        // Warning bells!
        if (filter->colorSpaceIndependence() == TO_LAB16) {
            if (QMessageBox::warning(d->view->mainWindow(),
                                     i18nc("@title:window", "LibrePaint"),
                                     i18n("The %1 filter will convert your %2 data to 16-bit L*a*b* and vice versa. ",
                                          filter->name(),
                                          dev->colorSpace()->name()),
                                     QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
                    != QMessageBox::Ok) return;

        } else if (filter->colorSpaceIndependence() == TO_RGBA16) {
            if (QMessageBox::warning(d->view->mainWindow(),
                                     i18nc("@title:window", "LibrePaint"),
                                     i18n("The %1 filter will convert your %2 data to 16-bit RGBA and vice versa. ",
                                          filter->name() , dev->colorSpace()->name()),
                                     QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok)
                    != QMessageBox::Ok) return;
        }
    }

    if (filter->showConfigurationWidget()) {
        if (!d->filterDialog) {
            d->filterDialog = new KisDlgFilter(d->view , d->view->activeNode(), this, d->view->mainWindow());
            d->filterDialog->setAttribute(Qt::WA_DeleteOnClose); // make sure that the dialog is deleted when calling `done()`
            connect(d->filterDialog, SIGNAL(finished(int)),
                    this, SLOT(filterDialogHasFinished(int)));
        }

        d->filterDialog->setFilter(filter, overrideDefaultConfig);
        d->filterDialog->setVisible(true);
    } else {
        KisFilterConfigurationSP defaultConfiguration =
            overrideDefaultConfig ? overrideDefaultConfig : filter->defaultConfiguration(KisGlobalResourcesInterface::instance());
        apply(defaultConfiguration);
        finish();
    }
}

void KisFilterManager::apply(KisFilterConfigurationSP _filterConfig)
{
    if (_filterConfig && d->gpuFilterOwner && d->filterDialog) {
        if (d->view->image().data() != d->gpuFilterImage.data() || d->view->activeNode() != d->gpuFilterNode ||
            d->gpuFilterImage->gpuImage() != d->gpuFilterOwner || !d->gpuFilterNode->isEditable() ||
            d->gpuFilterOwner->operations().snapshot() != d->gpuFilterBefore) {
            d->filterDialog->reject();
            return;
        }
        if (d->gpuFilterOperation.isNull()) d->gpuFilterOperation = d->gpuFilterOwner->beginPixelEdit();
        if (d->gpuFilterOperation.isNull()) return;
        const auto config = _filterConfig->cloneWithResourcesSnapshot();
        const auto result = d->gpuFilterOwner->updatePixelEdit(d->gpuFilterOperation,
            [this, config] { return applyGpuConfiguration(config, false); });
        d->currentlyAppliedConfiguration = result == LibrePaint::Image::Gpu::Result::Success ? config : KisFilterConfigurationSP{};
        return;
    }
    if (_filterConfig && applyGpuFilter(_filterConfig->name(), _filterConfig, false)) return;

    KisFilterConfigurationSP filterConfig = _filterConfig->cloneWithResourcesSnapshot();

    KisFilterSP filter = KisFilterRegistry::instance()->value(filterConfig->name());
    KisImageWSP image = d->view->image();

    if (d->currentStrokeId) {
        image->cancelStroke(d->currentStrokeId);

        d->currentStrokeId.clear();
        d->idleBarrierCookie.clear();
    } else {
        image->waitForDone();
    }

    if (!d->externalCancelUpdatesStorage) {
        // Lazily initialize the cancel updates storage, just in case
        // if the stroke has been cancelled in the meantime.

        d->externalCancelUpdatesStorage.reset(new KisFilterStrokeStrategy::ExternalCancelUpdatesStorage());
    }

    KoCanvasResourceProvider *resourceManager =
        d->view->canvasResourceProvider()->resourceManager();

    KisResourcesSnapshotSP resources =
        new KisResourcesSnapshot(image,
                                 d->view->activeNode(),
                                 resourceManager->canvasResourcesInterface());

    KisFilterStrokeStrategy *strategy = new KisFilterStrokeStrategy(filter,
                                                                    KisFilterConfigurationSP(filterConfig),
                                                                    resources,
                                                                    d->externalCancelUpdatesStorage.toWeakRef());
    {
        KConfigGroup group( KSharedConfig::openConfig(), "filterdialog");
        strategy->setForceLodModeIfPossible(group.readEntry("forceLodMode", true));
    }

    d->currentStrokeId =
        image->startStroke(strategy);

    // Apply filter preview to active, visible frame only.
    KisImageConfig imgConf(true);
    image->addJob(d->currentStrokeId, new KisFilterStrokeStrategy::FilterJobData());

    {
        KisFilterStrokeStrategy::IdleBarrierData *data =
            new KisFilterStrokeStrategy::IdleBarrierData();
        d->idleBarrierCookie = data->idleBarrierCookie();
        image->addJob(d->currentStrokeId, data);
    }

    d->currentlyAppliedConfiguration = filterConfig;
}

void KisFilterManager::finish()
{
    if (d->gpuFilterOwner) {
        if (!d->gpuFilterOperation.isNull()) {
            if (d->currentlyAppliedConfiguration &&
                d->gpuFilterOwner->commitPixelEdit(d->gpuFilterOperation) == LibrePaint::Image::Gpu::Result::Success) {
                rememberGpuConfiguration(d->currentlyAppliedConfiguration);
                d->gpuFilterOperation = {};
                d->currentlyAppliedConfiguration.clear();
            } else cancelRunningStroke();
        }
        return;
    }
    Q_ASSERT(d->currentStrokeId);

    if (d->filterAllSelectedFrames) {   // Apply filter to the other selected frames...
        KisImageSP image = d->view->image();
        KisPaintDeviceSP paintDevice = d->view->activeNode()->paintDevice();
        KisNodeSP node = d->view->activeNode();

        // Filter selected times to only those with keyframes...
        QSet<int> selectedTimes = image->animationInterface()->activeLayerSelectedTimes();
        selectedTimes = KisLayerUtils::filterTimesForOnlyRasterKeyedTimes(node, selectedTimes);
        QSet<int> uniqueFrames = KisLayerUtils::fetchUniqueFrameTimes(node, selectedTimes, true);

        Q_FOREACH(const int& frameTime, uniqueFrames) {
            image->addJob(d->currentStrokeId, new KisFilterStrokeStrategy::FilterJobData(frameTime));
        }
    }

    d->view->image()->endStroke(d->currentStrokeId);

    KisFilterSP filter = KisFilterRegistry::instance()->value(d->currentlyAppliedConfiguration->name());
    if (filter->bookmarkManager()) {
        filter->bookmarkManager()->save(KisBookmarkedConfigurationManager::ConfigLastUsed,
                                       d->currentlyAppliedConfiguration.data());
    }

    d->lastConfiguration = d->currentlyAppliedConfiguration;
    d->reapplyAction->setEnabled(true);
    d->reapplyAction->setText(i18n("Apply Filter Again: %1", filter->name()));

    d->idleBarrierCookie.clear();
    d->currentlyAppliedConfiguration.clear();
}

void KisFilterManager::cancelRunningStroke()
{
    if (d->gpuFilterOwner) {
        if (!d->gpuFilterOperation.isNull()) d->gpuFilterOwner->cancelPixelEdit(d->gpuFilterOperation);
        d->gpuFilterOperation = {};
        d->currentlyAppliedConfiguration.clear();
        return;
    }
    Q_ASSERT(d->currentStrokeId);

    // we should to notify the stroke that it should do the updates itself.
    d->externalCancelUpdatesStorage->shouldIssueCancellationUpdates.ref();
    d->view->image()->cancelStroke(d->currentStrokeId);

    d->currentStrokeId.clear();
    d->idleBarrierCookie.clear();
    d->currentlyAppliedConfiguration.clear();
    d->externalCancelUpdatesStorage.clear();
}

void KisFilterManager::cancelDialog()
{
    cancelRunningStroke();

    d->filterDialog->reject();
}

bool KisFilterManager::isStrokeRunning() const
{
    return bool(d->currentStrokeId) || !d->gpuFilterOperation.isNull();
}

bool KisFilterManager::isIdle() const
{
    return !d->idleBarrierCookie;
}

void KisFilterManager::setFilterAllSelectedFrames(bool filterAllSelectedFrames)
{
    d->filterAllSelectedFrames = filterAllSelectedFrames;
}

bool KisFilterManager::filterAllSelectedFrames()
{
    return d->filterAllSelectedFrames;
}

void KisFilterManager::slotStrokeEndRequested()
{
    if (isStrokeRunning() && d->filterDialog) {
        d->filterDialog->accept();
    }
}

void KisFilterManager::slotStrokeCancelRequested()
{
    if (isStrokeRunning() && d->filterDialog) {
        d->filterDialog->reject();
    }
}
void KisFilterManager::filterDialogHasFinished(int)
{
    // as far as we are concerned, filterDialog has been deleted
    if (!d->gpuFilterOperation.isNull()) cancelRunningStroke();
    if (d->gpuFilterImage) {
        disconnect(d->gpuFilterImage, &KisImage::sigStrokeCancellationRequested, this, &KisFilterManager::slotStrokeCancelRequested);
        disconnect(d->gpuFilterImage, &KisImage::sigStrokeEndRequested, this, &KisFilterManager::slotStrokeEndRequested);
    }
    d->filterDialog = nullptr;
    d->gpuFilterImage.clear();
    d->gpuFilterNode.clear();
    d->gpuFilterSource.clear();
    d->gpuFilterOwner.clear();
    d->gpuFilterBefore.clear();
}

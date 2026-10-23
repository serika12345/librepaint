/* This file is part of the Krita project
 *
 * SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "KisImportExportUtils.h"
#include "KisDocumentUndoStack_p.h"
#include "KisQStringListFwd.h"
#include "KisResourceModel.h"
#include "KoColorSet.h"
#include "KoColorSpaceConstants.h"
#include "KoResourceLoadResult.h"
#include "KoResourceSignature.h"
#include "application/ui/workspace/KisMainWindow.h" // XXX: remove

#include <KisMimeDatabase.h>

#include <KoCanvasBase.h>
#include <KritaVersionWrapper.h>
#include <KoColor.h>
#include <kis_dom_utils.h>
#include <KoColorProfile.h>
#include <KoColorSpaceEngine.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <info/KoDocumentInfoDlg.h>
#include <memory>
#include <functional>
#include <stdexcept>
#include <limits>
#include <optional>
#include <metadata/KoDocumentInfo.h>
#include <metadata/kis_meta_data_store.h>
#include <metadata/kis_meta_data_backend_registry.h>
#include <metadata/kis_meta_data_io_backend.h>
#include <KoUnit.h>
#include <KoID.h>
#include <KoProgressProxy.h>
#include <KoProgressUpdater.h>
#include <KoSelection.h>
#include <KoShape.h>
#include <KoShapeController.h>
#include <KoStore.h>
#include <KoUpdater.h>
#include <KisImportExportErrorCode.h>
#include <KisImportExportFilter.h>
#include <KoDocumentResourceManager.h>
#include <KoMD5Generator.h>
#include <KisResourceStorage.h>
#include <KisResourceLocator.h>
#include <KisResourceTypes.h>
#include <KisGlobalResourcesInterface.h>
#include <QtGlobal>
#include <qdatetime.h>
#include <qhashfunctions.h>
#include <QDebug>
#include <qlogging.h>
#include <qnamespace.h>
#include <qobjectdefs.h>
#include <qpointer.h>
#include <qsharedpointer.h>
#include <session/kis_document_autosave_state.h>
#include <session/kis_document_identity.h>
#include <session/kis_document_modification_state.h>
#include <session/kis_document_recovery_autosave_state.h>
#include <session/kis_document_recovery_status.h>
#include <files/kis_document_autosave_files.h>
#include <files/kis_document_backup_file.h>
#include <files/kis_document_save_target.h>
#include <KisResourceLoaderRegistry.h>
#include <KisResourceCacheDb.h>
#include <KoEmbeddedResource.h>
#include <KisUsageLogger.h>
#include <klocalizedstring.h>
#include <kis_debug.h>
#include <kis_generator_layer.h>
#include <kis_generator_registry.h>
#include <io/kis_document_io_presentation.h>
#include <application/ui/workspace/KisView.h>

#include <QTextBrowser>
#include <QApplication>
#include <QBuffer>
#include <QDir>
#include <QDomDocument>
#include <QDomElement>
#include <QFileInfo>
#include <QImage>
#include <QList>
#include <QMutex>
#include <QQueue>
#include <QPainter>
#include <QRect>
#include <QScopedPointer>
#include <QSize>
#include <QSet>
#include <QSignalBlocker>
#include <QStringList>
#include <QTimer>
#include <QThread>
#include <QWidget>
#include <QFuture>
#include <QFutureWatcher>
#include <QUuid>

// Krita Image
#include <kis_image_animation_interface.h>
#include <kis_time_span.h>
#include <application/kis_config.h>
#include <flake/kis_shape_layer.h>
#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_annotation.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuPaintDocument.h>
#include <kra/KisKraRasterCodec.h>
#include <QPointer>
#include <kis_layer.h>
#include <kis_name_server.h>
#include <kis_paint_layer.h>
#include <kis_painter.h>
#include <kis_fill_painter.h>
#include <undo/kis_document_undo_store.h>
#include <kis_idle_watcher.h>
#include <kis_signal_auto_connection.h>
#include <kis_canvas_widget_base.h>
#include "KisUniqueColorSet.h"
#include "document/StoryboardItem.h"
#include "kis_assert.h"
#include "kis_filter_configuration.h"
#include "kis_layer_utils.h"

// Local
#include "application/ui/workspace/KisViewManager.h"
#include "kis_clipboard.h"
#include "kis_node_filter_interface.h"
#include "kis_painting_assistant.h"
#include "kis_stroke_job_strategy.h"
#include "kis_types.h"
#include "kis_undo_stores.h"
#include "kundo2magicstring.h"
#include "widgets/kis_custom_image_widget.h"
#include "canvas/kis_canvas2.h"
#include "flake/kis_shape_controller.h"
#include "canvas/kis_canvas_resource_provider.h"
#include "application/ui/orchestration/KisResourceServerProvider.h"
#include "nodes/kis_node_manager.h"
#include "application/ui/orchestration/KisPart.h"
#include "application/ui/orchestration/KisApplication.h"
#include "document/KisDocument.h"
#include "KisImportExportManager.h"
#include "application/ui/workspace/KisView.h"
#include "kis_grid_config.h"
#include "kis_guides_config.h"
#include "KisImageBarrierLock.h"
#include "KisReferenceImagesLayer.h"
#include <recovery/KisRecoverNamedAutosaveDialog.h>
#include "utils/KisFileIconCreator.h"

#include <mutex>
#include "kis_config_notifier.h"
#include "events/kis_async_action_feedback.h"
#include "document/KisCloneDocumentStroke.h"

#include <kis_algebra_2d.h>
#include <KisMirrorAxisConfig.h>
#include <canvas/KisDecorationsWrapperLayer.h>
#include "kis_simple_stroke_strategy.h"
#include <KisCursorOverrideLock.h>
#include <utility>

// Define the protocol used here for embedded documents' URL
// This used to "store" but QUrl didn't like it,
// so let's simply make it "tar" !
#define STORE_PROTOCOL "tar"
// The internal path is a hack to make QUrl happy and for document children
#define INTERNAL_PROTOCOL "intern"
#define INTERNAL_PREFIX "intern:/"
// Warning, keep it sync in koStore.cc


#ifdef Q_OS_MACOS
#include "KisMacosSecurityBookmarkManager.h"
#endif

using namespace std;

namespace
{

QString autoSaveFilePath(const KisDocument *document, const QString &path)
{
    return Krita::Document::KisDocumentAutoSaveFiles::filePath(
        path,
        document->objectName(),
        KisConfig(true).readEntry<bool>("autosavefileshidden"));
}

KisImportExportErrorCode gpuFileStatus(LibrePaint::Document::Gpu::FileCode code)
{
    using LibrePaint::Document::Gpu::FileCode;
    switch (code) {
    case FileCode::Success: return ImportExportCodes::OK;
    case FileCode::CapacityExceeded: return ImportExportCodes::InsufficientMemory;
    case FileCode::Busy: return ImportExportCodes::Busy;
    case FileCode::UnsupportedData: return ImportExportCodes::FormatFeaturesUnsupported;
    case FileCode::IoFailure: return ImportExportCodes::ErrorWhileWriting;
    default: return ImportExportCodes::Failure;
    }
}

struct StaticKraSettings
{
    std::optional<int> framerate, time, initialFrame;
    std::optional<KisTimeSpan> range;
    std::optional<QString> sequencePath, sequenceName;
};

KisImportExportErrorCode readStaticKraSettings(const QByteArray &bytes, StaticKraSettings &settings, QString &diagnostic)
{
    if (bytes.isEmpty()) return ImportExportCodes::OK;
    QDomDocument xml;
    if (!xml.setContent(bytes)) {
        diagnostic = i18n("Invalid KRA animation settings.");
        return ImportExportCodes::FileFormatIncorrect;
    }
    const auto root = xml.documentElement();
    const auto unsupported = [&](const QString &feature) -> KisImportExportErrorCode {
        diagnostic = i18n("The new painting engine cannot edit the KRA feature '%1'.", root.tagName() + QLatin1Char('/') + feature);
        return ImportExportCodes::FormatFeaturesUnsupported;
    };
    const auto invalid = [&](const QString &field) -> KisImportExportErrorCode {
        diagnostic = i18n("Invalid KRA timing value '%1'.", field);
        return ImportExportCodes::FileFormatIncorrect;
    };
    for (int i = 0; i < root.attributes().size(); ++i)
        if (root.attributes().item(i).nodeName() != QLatin1String("xmlns"))
            return unsupported(root.attributes().item(i).nodeName());
    QSet<QString> seen;
    std::optional<QString> exportPath, exportName;
    for (auto value = root.firstChildElement(); !value.isNull(); value = value.nextSiblingElement()) {
        const auto tag = value.tagName();
        if (seen.contains(tag)) return invalid(tag);
        seen.insert(tag);
        const bool exports = tag == QLatin1String("export-settings");
        const bool range = tag == QLatin1String("range");
        if (!exports && !range && tag != QLatin1String("framerate") && tag != QLatin1String("currentTime"))
            return unsupported(tag);
        const QSet<QString> attributes = exports ? QSet<QString>{} : range ?
            QSet<QString>{QStringLiteral("type"), QStringLiteral("from"), QStringLiteral("to")} :
            QSet<QString>{QStringLiteral("type"), QStringLiteral("value")};
        for (int i = 0; i < value.attributes().size(); ++i)
            if (!attributes.contains(value.attributes().item(i).nodeName()))
                return unsupported(tag + QLatin1Char('/') + value.attributes().item(i).nodeName());
        if (exports) {
            QSet<QString> exportFields;
            for (auto item = value.firstChildElement(); !item.isNull(); item = item.nextSiblingElement()) {
                const auto name = item.tagName();
                if (exportFields.contains(name)) return invalid(name);
                exportFields.insert(name);
                if (name != QLatin1String("sequenceFilePath") && name != QLatin1String("sequenceBaseName") &&
                    name != QLatin1String("sequenceInitialFrameNumber")) return unsupported(name);
                for (int i = 0; i < item.attributes().size(); ++i) {
                    const auto attribute = item.attributes().item(i).nodeName();
                    if (attribute != QLatin1String("type") && attribute != QLatin1String("value")) return unsupported(name + QLatin1Char('/') + attribute);
                }
                if (!item.firstChildElement().isNull()) return unsupported(name);
                if (item.attribute(QStringLiteral("type")) != QLatin1String("value") || !item.hasAttribute(QStringLiteral("value"))) return invalid(name);
                if (name == QLatin1String("sequenceInitialFrameNumber")) {
                    bool ok = false;
                    const int number = item.attribute(QStringLiteral("value")).toInt(&ok);
                    if (!ok) return invalid(name);
                    settings.initialFrame = number;
                } else if (name == QLatin1String("sequenceFilePath")) exportPath = item.attribute(QStringLiteral("value"));
                else exportName = item.attribute(QStringLiteral("value"));
            }
        } else {
            if (!value.firstChildElement().isNull()) return unsupported(tag);
            if (range) {
                bool firstOk = false, lastOk = false;
                value.attribute(QStringLiteral("from")).toInt(&firstOk);
                value.attribute(QStringLiteral("to")).toInt(&lastOk);
                KisTimeSpan span;
                if (!firstOk || !lastOk || !KisDomUtils::loadValue(root, tag, &span) || !span.isValid() || span.isInfinite()) return invalid(tag);
                settings.range = span;
            } else if (tag == QLatin1String("framerate")) {
                float fps = 0;
                if (!KisDomUtils::loadValue(value, &fps) || !qIsFinite(fps) || fps < 1 ||
                    double(fps) > std::numeric_limits<int>::max()) return invalid(tag);
                settings.framerate = int(fps);
            } else {
                bool ok = false;
                const int time = value.attribute(QStringLiteral("value")).toInt(&ok);
                if (!ok || value.attribute(QStringLiteral("type")) != QLatin1String("value")) return invalid(tag);
                settings.time = time;
            }
        }
    }
    if (exportPath) settings.sequencePath = *exportPath;
    // The native loader uses the last provided export string as the base-name fallback.
    settings.sequenceName = exportName.value_or(exportPath.value_or(QString()));
    return ImportExportCodes::OK;
}

LibrePaint::Document::Gpu::FileOutcome applyGpuKraMetadata(KisDocument &document,
                                                        LibrePaint::Document::Gpu::PaintDocument &fileDocument,
                                                        const QDateTime &saveTime = QDateTime::currentDateTime(),
                                                        std::optional<bool> compressRaster = {})
{
    using LibrePaint::Document::Gpu::FileCode;
    if (QThread::currentThread() != document.thread())
        return {FileCode::InvalidData, {}, i18n("Image metadata requires the document thread.")};
    LibrePaint::ImportExport::Kra::ImageMetadata metadata;
    KisImageSP image = document.image();
    metadata.resolution = {image->xRes() * 72, image->yRes() * 72};
    metadata.producerVersion = KritaVersionWrapper::versionString(false);
    metadata.compressRaster = compressRaster.value_or(KisConfig(true).compressKra());
    auto info = KisDocument::createDomDocument(QStringLiteral("document-info"), QStringLiteral("document-info"), QStringLiteral("1.1"));
    metadata.documentInfo = document.documentInfo()->save(info, document.isAutosaving(), document.isModified(), saveTime).toByteArray();
    metadata.iccProfile = image->profile()->rawData();
    metadata.profileName = image->profile()->name();
    QDomDocument colors;
    auto colorHistory = colors.createElement(QStringLiteral("ColorHistory"));
    colors.appendChild(colorHistory);
    for (const auto &color : document.colorHistoryColors()) color.toXML(colors, colorHistory);
    metadata.colorHistory = colors.toByteArray();
    QDomDocument assistants;
    auto assistantsColor = assistants.createElement(QStringLiteral("GlobalAssistantsColor"));
    assistantsColor.setAttribute(QStringLiteral("SimpleColorData"), KisDomUtils::qColorToQString(document.assistantsGlobalColor()));
    assistants.appendChild(assistantsColor);
    metadata.assistantsGlobalColor = assistants.toByteArray();
    auto *animation = image->animationInterface();
    QDomDocument timing;
    auto timingRoot = timing.createElement(QStringLiteral("animation"));
    timing.appendChild(timingRoot);
    KisDomUtils::saveValue(&timingRoot, QStringLiteral("framerate"), animation->framerate());
    KisDomUtils::saveValue(&timingRoot, QStringLiteral("range"), animation->documentPlaybackRange());
    KisDomUtils::saveValue(&timingRoot, QStringLiteral("currentTime"), animation->currentUITime());
    metadata.animation = timing.toByteArray();
    auto animationIndex = document.createDomDocument(QStringLiteral("animation-metadata"), QStringLiteral("1.1"));
    auto indexRoot = animationIndex.documentElement();
    for (auto child = timingRoot.firstChild(); !child.isNull(); child = child.nextSibling())
        indexRoot.appendChild(animationIndex.importNode(child, true));
    auto exports = animationIndex.createElement(QStringLiteral("export-settings"));
    KisDomUtils::saveValue(&exports, QStringLiteral("sequenceFilePath"), animation->exportSequenceFilePath());
    KisDomUtils::saveValue(&exports, QStringLiteral("sequenceBaseName"), animation->exportSequenceBaseName());
    KisDomUtils::saveValue(&exports, QStringLiteral("sequenceInitialFrameNumber"), animation->exportInitialFrameNumber());
    indexRoot.appendChild(exports);
    metadata.animationMetadata = animationIndex.toByteArray();
    const auto &mirror = document.mirrorAxisConfig();
    if (!mirror.isDefault()) {
        QDomDocument axes;
        axes.appendChild(mirror.saveToXml(axes, QStringLiteral("MirrorAxis")));
        metadata.mirrorAxis = axes.toByteArray();
    }
    for (auto entry = image->beginAnnotations(); entry != image->endAnnotations(); ++entry) {
        if (*entry && !(*entry)->type().isEmpty() && (*entry)->type() != QLatin1String("icc"))
            metadata.annotations.append({(*entry)->type(), (*entry)->description(), (*entry)->annotation()});
    }
    KisMetaData::IOBackend *backend = nullptr;
    FileCode metadataStatus = FileCode::Success;
    QString diagnostic;
    const auto encode = [&](KisNodeSP node, QByteArray &packet) {
        auto *layer = qobject_cast<KisLayer *>(node.data());
        if (!layer || layer->metaData()->isEmpty()) return true;
        if (!backend) backend = KisMetadataBackendRegistry::instance()->get(QStringLiteral("xmp"));
        if (!backend || !backend->supportSaving()) {
            metadataStatus = FileCode::UnsupportedData;
            diagnostic = i18n("The XMP metadata writer is unavailable.");
            return false;
        }
        QBuffer buffer;
        if (!backend->saveTo(layer->metaData(), &buffer)) {
            metadataStatus = FileCode::InvalidData;
            diagnostic = i18n("Could not save metadata for layer %1.", layer->name());
            return false;
        }
        packet = buffer.data();
        return true;
    };
    KisNodeSP root = image->root();
    QByteArray rootPacket;
    if (!encode(root, rootPacket)) return {metadataStatus, {}, diagnostic};
    metadata.rootLayerXmp = std::move(rootPacket);
    metadata.layerXmp.emplace();
    bool captured = true;
    KisLayerUtils::recursiveApplyNodes(root, [&](KisNodeSP node) {
        if (!captured || node == root) return;
        QByteArray packet;
        captured = encode(node, packet);
        if (captured && !packet.isEmpty()) metadata.layerXmp->insert(node->uuid(), std::move(packet));
    });
    if (!captured) return {metadataStatus, {}, diagnostic};
    return fileDocument.setKraImageMetadata(metadata);
}

QString gpuKraEditingError(const QDomElement &image)
{
    const QSet<QString> imageAttributes{QStringLiteral("name"), QStringLiteral("mime"), QStringLiteral("width"),
        QStringLiteral("height"), QStringLiteral("colorspacename"), QStringLiteral("profile"),
        QStringLiteral("description"), QStringLiteral("x-res"), QStringLiteral("y-res")};
    const QSet<QString> imageChildren{QStringLiteral("layers"), QStringLiteral("ProjectionBackgroundColor"),
        QStringLiteral("MirrorAxis"), QStringLiteral("Annotations"), QStringLiteral("ColorHistory"),
        QStringLiteral("GlobalAssistantsColor"), QStringLiteral("Palettes"), QStringLiteral("resources"),
        QStringLiteral("animation")};
    const QSet<QString> layerAttributes{QStringLiteral("name"), QStringLiteral("uuid"), QStringLiteral("filename"),
        QStringLiteral("x"), QStringLiteral("y"), QStringLiteral("visible"), QStringLiteral("selected"),
        QStringLiteral("nodetype"), QStringLiteral("compositeop"), QStringLiteral("opacity"),
        QStringLiteral("colorspacename"), QStringLiteral("profile"), QStringLiteral("locked"),
        QStringLiteral("intimeline"), QStringLiteral("colorlabel"), QStringLiteral("channelflags"),
        QStringLiteral("channellockflags"), QStringLiteral("passthrough"), QStringLiteral("collapsed"),
        QStringLiteral("onionskin")};
    QList<QDomElement> pending{image};
    while (!pending.isEmpty()) {
        const auto element = pending.takeLast();
        const auto attributes = element.attributes();
        const auto &supported = element == image ? imageAttributes : layerAttributes;
        for (int i = 0; i < attributes.size(); ++i) {
            const auto name = attributes.item(i).nodeName();
            if (!supported.contains(name))
                return i18n("The new painting engine cannot edit the KRA feature '%1'.", name);
        }
        if (element != image && (element.attribute(QStringLiteral("onionskin"), QStringLiteral("0")) != QLatin1String("0") ||
                                  element.attribute(QStringLiteral("collapsed"), QStringLiteral("0")) != QLatin1String("0")))
            return i18n("The new painting engine cannot yet restore onion skins or collapsed groups.");
        for (auto child = element.firstChildElement(); !child.isNull(); child = child.nextSiblingElement()) {
            if (element == image && !imageChildren.contains(child.tagName()))
                return i18n("The new painting engine cannot edit the KRA feature '%1'.", child.tagName());
            if (element == image && (child.tagName() == QLatin1String("Palettes") || child.tagName() == QLatin1String("resources")) &&
                (child.hasAttributes() || !child.firstChildElement().isNull()))
                return i18n("The new painting engine cannot edit the KRA feature '%1'.", child.tagName());
            if (child.tagName() == QLatin1String("layers"))
                for (auto layer = child.firstChildElement(); !layer.isNull(); layer = layer.nextSiblingElement()) pending.append(layer);
        }
    }
    return {};
}

}

/**********************************************************
 *
 * KisDocument
 *
 **********************************************************/

//static
QString KisDocument::newObjectName()
{
    static int s_docIFNumber = 0;
    QString name; name.setNum(s_docIFNumber++); name.prepend("document_");
    return name;
}


class Q_DECL_HIDDEN KisDocument::Private
{
public:
    Private(KisDocument *_q)
        : q(_q)
        , docInfo(new KoDocumentInfo(_q)) // deleted by QObject
        , ioPresentation(new KisDocumentIoPresentation(_q)) // deleted by QObject
        , importExportManager(new KisImportExportManager(_q)) // deleted manually
        , autoSaveTimer(new QTimer(_q))
        , undoStack(new KisDocumentUndoStack(_q)) // deleted by QObject
        , readwrite(true)
        , autoSaveActive(true)
        , firstMod(QDateTime::currentDateTime())
        , lastMod(firstMod)
        , nserver(new KisNameServer(1))
        , imageIdleWatcher(2000 /*ms*/)
        , globalAssistantsColor(KisConfig(true).defaultAssistantsColor())
        , batchMode(false)
    {
        if (QLocale().measurementSystem() == QLocale::ImperialSystem) {
            unit = KoUnit::Inch;
        } else {
            unit = KoUnit::Centimeter;
        }
        connectPresentation();
        connect(&imageIdleWatcher, SIGNAL(startedIdleMode()), q, SLOT(slotPerformIdleRoutines()));
    }

    Private(const Private &rhs, KisDocument *_q)
        : q(_q)
        , docInfo(new KoDocumentInfo(*rhs.docInfo, _q))
        , ioPresentation(new KisDocumentIoPresentation(_q))
        , importExportManager(new KisImportExportManager(_q))
        , autoSaveTimer(new QTimer(_q))
        , undoStack(new KisDocumentUndoStack(_q))
        , nserver(new KisNameServer(*rhs.nserver))
        , preActivatedNode(0) // the node is from another hierarchy!
        , imageIdleWatcher(2000 /*ms*/)
        , colorHistoryModel(rhs.colorHistoryModel)
    {
        copyFromImpl(rhs, _q, CONSTRUCT);
        connectPresentation();
        connect(&imageIdleWatcher, SIGNAL(startedIdleMode()), q, SLOT(slotPerformIdleRoutines()));
    }

    ~Private() {
        // Don't delete m_d->shapeController because it's in a QObject hierarchy.
        delete nserver;
    }

    KisDocument *q = 0;
    KoDocumentInfo *docInfo = 0;
    KisDocumentIoPresentation *ioPresentation = 0;

    KoUnit unit;

    KisImportExportManager *importExportManager = 0; // The filter-manager to use when loading/saving [for the options]

    Krita::Document::AutoSaveState autoSaveState;
    Krita::Document::Identity identity;
    Krita::Document::ModificationState modificationState;
    Krita::Document::RecoveryAutoSaveState recoveryAutoSaveState;
    Krita::Document::RecoveryStatus recoveryStatus;

    QTimer *autoSaveTimer;
    QString lastErrorMessage; // see openFile()
    QString lastWarningMessage;

    int autoSaveDelay = 300; // in seconds, 0 to disable.

    KisDocumentUndoStack *undoStack = 0;

    KisGuidesConfig guidesConfig;
    KisMirrorAxisConfig mirrorAxisConfig;

    QMutex savingMutex;

    bool readwrite = false;
    bool autoSaveActive = true;

    QDateTime firstMod;
    QDateTime lastMod;

    KisNameServer *nserver;

    KisImageSP image;
    KisImageSP savingImage;
    std::shared_ptr<LibrePaint::Document::Gpu::PaintDocument> gpuDocument;
    bool gpuSaving = false;
    std::function<void()> gpuAutoSaveJob;

    KisNodeWSP preActivatedNode;
    KisShapeController* shapeController = 0;
    KoShapeController* koShapeController = 0;
    KisIdleWatcher imageIdleWatcher;
    QScopedPointer<KisSignalAutoConnection> imageIdleConnection;

    QList<KisPaintingAssistantSP> assistants;

    StoryboardItemList m_storyboardItemList;
    QVector<StoryboardComment> m_storyboardCommentList;

    QVector<QFileInfo> audioTracks;
    qreal audioLevel = 1.0;

    QColor globalAssistantsColor;
    KisUniqueColorSet colorHistoryModel;

    KisGridConfig gridConfig;

    std::unique_ptr<KisDocument> backgroundSaveDocument;
    QPointer<KoUpdater> savingUpdater;
    QFuture<KisImportExportErrorCode> childSavingFuture;
    KritaUtils::ExportFileJob backgroundSaveJob;
    QMetaObject::Connection completeSavingConnection;
    KisSignalAutoConnectionsStore referenceLayerConnections;

    bool batchMode { false };
    bool decorationsSyncingDisabled = false;
    bool wasStorageAdded = false;
    bool documentIsClosing = false;

    // Resources saved in the .kra document
    QString linkedResourcesStorageID;
    KisResourceStorageSP linkedResourceStorage;

    // Resources saved into other components of the kra file
    QString embeddedResourcesStorageID;
    KisResourceStorageSP embeddedResourceStorage;

    void syncDecorationsWrapperLayerState();

    void setImageAndInitIdleWatcher(KisImageSP _image) {
        image = _image;

        imageIdleWatcher.setTrackedImage(image);
    }

    void copyFrom(const Private &rhs, KisDocument *q);
    void copyFromImpl(const Private &rhs, KisDocument *q, KisDocument::CopyPolicy policy);

    void uploadLinkedResourcesFromLayersToStorage();
    KisDocument* lockAndCloneImpl(bool fetchResourcesFromLayers);

    void updateDocumentMetadataOnSaving(const QString &filePath, const QByteArray &mimeType);
    enum class GpuAutoSaveMode { Timer, Recovery, Pause };
    RecoveryAutoSaveStartResult startGpuAutoSave(const QString &path, GpuAutoSaveMode mode);
    void runPendingGpuAutoSave();

    void connectPresentation()
    {
        QObject::connect(ioPresentation,
                         &KisDocumentIoPresentation::statusBarMessage,
                         q,
                         &KisDocument::statusBarMessage);
        QObject::connect(ioPresentation,
                         &KisDocumentIoPresentation::savingCompleted,
                         q,
                         &KisDocument::completed);
        QObject::connect(ioPresentation,
                         &KisDocumentIoPresentation::savingFinished,
                         q,
                         &KisDocument::sigSavingFinished);
        QObject::connect(ioPresentation,
                         &KisDocumentIoPresentation::loadingFinished,
                         q,
                         &KisDocument::sigLoadingFinished);
    }

    /// clones the palette list oldList
    /// the ownership of the returned KoColorSet * belongs to the caller
    class StrippedSafeSavingLocker;
};


void KisDocument::Private::syncDecorationsWrapperLayerState()
{
    if (!this->image || this->decorationsSyncingDisabled) return;

    KisImageSP image = this->image;
    KisDecorationsWrapperLayerSP decorationsLayer =
            KisLayerUtils::findNodeByType<KisDecorationsWrapperLayer>(image->root());

    const bool needsDecorationsWrapper =
            gridConfig.showGrid() || (guidesConfig.showGuides() && guidesConfig.hasGuides()) || !assistants.isEmpty();
    if (!needsDecorationsWrapper && !decorationsLayer) return;
    KisDecorationsWrapperLayerSP layerToAdd = needsDecorationsWrapper && !decorationsLayer ? new KisDecorationsWrapperLayer(q) : KisDecorationsWrapperLayerSP();
    struct SyncDecorationsWrapperStroke : public KisSimpleStrokeStrategy {
        SyncDecorationsWrapperStroke(KisImageSP image, KisDecorationsWrapperLayerSP layerToAdd, bool needsDecorationsWrapper)
            : KisSimpleStrokeStrategy(QLatin1String("sync-decorations-wrapper"),
                                      kundo2_noi18n("start-isolated-mode")),
              m_image(image),
              m_layerToAdd(layerToAdd),
              m_needsDecorationsWrapper(needsDecorationsWrapper)
        {
            this->enableJob(JOB_INIT, true, KisStrokeJobData::SEQUENTIAL, KisStrokeJobData::EXCLUSIVE);
            setClearsRedoOnStart(false);
            setRequestsOtherStrokesToEnd(false);
        }

        void initStrokeCallback() override {
            KisDecorationsWrapperLayerSP decorationsLayer = KisLayerUtils::findNodeByType<KisDecorationsWrapperLayer>(m_image->root());

            if (m_needsDecorationsWrapper && !decorationsLayer) {
                m_image->addNode(m_layerToAdd);
            } else if (!m_needsDecorationsWrapper && decorationsLayer) {
                m_image->removeNode(decorationsLayer);
            }
        }
    private:
        KisImageSP m_image;
        KisDecorationsWrapperLayerSP m_layerToAdd;
        bool m_needsDecorationsWrapper = false;
    };

    KisStrokeId id = image->startStroke(new SyncDecorationsWrapperStroke(image, layerToAdd, needsDecorationsWrapper));
    image->endStroke(id);
}

void KisDocument::Private::copyFrom(const Private &rhs, KisDocument *q)
{
    copyFromImpl(rhs, q, KisDocument::REPLACE);
}

void KisDocument::Private::copyFromImpl(const Private &rhs, KisDocument *q, KisDocument::CopyPolicy policy)
{
    if (policy == REPLACE) {
        delete docInfo;
    }
    docInfo = (new KoDocumentInfo(*rhs.docInfo, q));
    unit = rhs.unit;
    identity = rhs.identity;

    if (policy == REPLACE) {
        q->setGuidesConfig(rhs.guidesConfig);
        q->setMirrorAxisConfig(rhs.mirrorAxisConfig);
        q->setModified(rhs.modificationState.isModified());
        q->setAssistants(KisPaintingAssistant::cloneAssistantList(rhs.assistants));
        q->setStoryboardItemList(StoryboardItem::cloneStoryboardItemList(rhs.m_storyboardItemList));
        q->setStoryboardCommentList(rhs.m_storyboardCommentList);
        q->setAudioTracks(rhs.audioTracks);
        q->setAudioVolume(rhs.audioLevel);
        q->setGridConfig(rhs.gridConfig);
        colorHistoryModel.setColorList(rhs.colorHistoryModel.colorList());
    } else {
        // in CONSTRUCT mode, we cannot use the functions of KisDocument
        // because KisDocument does not yet have a pointer to us.
        guidesConfig = rhs.guidesConfig;
        mirrorAxisConfig = rhs.mirrorAxisConfig;
        modificationState = rhs.modificationState.snapshotState();
        assistants = KisPaintingAssistant::cloneAssistantList(rhs.assistants);
        m_storyboardItemList = StoryboardItem::cloneStoryboardItemList(rhs.m_storyboardItemList);
        m_storyboardCommentList = rhs.m_storyboardCommentList;
        audioTracks = rhs.audioTracks;
        audioLevel = rhs.audioLevel;
        gridConfig = rhs.gridConfig;
    }
    if (rhs.modificationState.imageModifiedWithoutUndo()) {
        modificationState.markImageModifiedWithoutUndo();
    } else {
        modificationState.clearImageModifiedWithoutUndo();
    }
    readwrite = rhs.readwrite;
    autoSaveActive = rhs.autoSaveActive;
    firstMod = rhs.firstMod;
    lastMod = rhs.lastMod;
    // XXX: the display properties will be shared between different snapshots
    globalAssistantsColor = rhs.globalAssistantsColor;
    batchMode = rhs.batchMode;


    if (rhs.linkedResourceStorage) {
        linkedResourceStorage = rhs.linkedResourceStorage->clone();
    }

    if (rhs.embeddedResourceStorage) {
        embeddedResourceStorage = rhs.embeddedResourceStorage->clone();
    }

}

class KisDocument::Private::StrippedSafeSavingLocker {
public:
    StrippedSafeSavingLocker(QMutex *savingMutex, KisImageSP image)
        : m_locked(false)
        , m_image(image)
        , m_savingLock(savingMutex)
        , m_imageLock(image, std::defer_lock)

    {
        /**
         * Initial try to lock both objects. Locking the image guards
         * us from any image composition threads running in the
         * background, while savingMutex guards us from entering the
         * saving code twice by autosave and main threads.
         *
         * Since we are trying to lock multiple objects, so we should
         * do it in a safe manner.
         */
        m_locked = std::try_lock(m_imageLock, *m_savingLock) < 0;

        if (!m_locked) {
            m_image->requestStrokeEnd();
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

            // one more try...
            m_locked = std::try_lock(m_imageLock, *m_savingLock) < 0;
        }
    }

    ~StrippedSafeSavingLocker() {
        if (m_locked) {
            m_imageLock.unlock();
            m_savingLock->unlock();
        }
    }

    bool successfullyLocked() const {
        return m_locked;
    }

private:
    Q_DISABLE_COPY_MOVE(StrippedSafeSavingLocker)

    bool m_locked;
    KisImageSP m_image;
    QMutex *m_savingLock;
    KisImageReadOnlyBarrierLock m_imageLock;
};

KisDocument::KisDocument(bool addStorage)
    : d(new Private(this))
{
    connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
    connect(d->undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(slotUndoStackCleanChanged(bool)));
    connect(d->autoSaveTimer, SIGNAL(timeout()), this, SLOT(slotAutoSave()));
    setObjectName(newObjectName());

#ifdef Q_OS_MACOS
    KisMacosSecurityBookmarkManager *bookmarkmngr = KisMacosSecurityBookmarkManager::instance();
    if (bookmarkmngr->isSandboxed()) {
        connect(this, SIGNAL(sigSavingFinished(const QString&)), bookmarkmngr, SLOT(slotCreateBookmark(const QString&)));
    }
#endif


    if (addStorage) {
        d->linkedResourcesStorageID = QUuid::createUuid().toString();
        d->linkedResourceStorage.reset(new KisResourceStorage(d->linkedResourcesStorageID));
        KisResourceLocator::instance()->addStorage(d->linkedResourcesStorageID, d->linkedResourceStorage);

        d->embeddedResourcesStorageID = QUuid::createUuid().toString();
        d->embeddedResourceStorage.reset(new KisResourceStorage(d->embeddedResourcesStorageID));
        KisResourceLocator::instance()->addStorage(d->embeddedResourcesStorageID, d->embeddedResourceStorage);

        d->wasStorageAdded = true;
    }

    // preload the krita resources
    KisResourceServerProvider::instance();

    d->shapeController = new KisShapeController(d->nserver, d->undoStack, this);
    d->koShapeController = new KoShapeController(0, d->shapeController);

    slotConfigChanged();
}

KisDocument::KisDocument(const KisDocument &rhs, bool addStorage)
    : QObject(),
      d(new Private(*rhs.d, this))
{
    copyFromDocumentImpl(rhs, CONSTRUCT);

    if (addStorage) {
        KisResourceLocator::instance()->addStorage(d->linkedResourcesStorageID, d->linkedResourceStorage);
        KisResourceLocator::instance()->addStorage(d->embeddedResourcesStorageID, d->embeddedResourceStorage);
        d->wasStorageAdded = true;
    }
}

KisDocument::~KisDocument()
{
    d->documentIsClosing = true;
    d->undoStack->beginDocumentClose();

    // wait until all the pending operations are in progress
    waitForSavingToComplete();

    // A recovery request may have been waiting to join a normal/manual save.
    // Once that save has drained there is no useful follow-up snapshot to
    // start for a document being destroyed, but callers must not be left
    // waiting for a completion that can no longer arrive.
    if (d->recoveryAutoSaveState.hasPendingRequest()) {
        finishRecoveryAutoSaveRequest(QString(), false);
    }

    d->imageIdleWatcher.setTrackedImage(0);

    /**
     * Push a timebomb, which will try to release the memory after
     * the document has been deleted
     */
    KisPaintDevice::createMemoryReleaseObject()->deleteLater();

    d->autoSaveTimer->disconnect(this);
    d->autoSaveTimer->stop();

    delete d->importExportManager;

    // Despite being QObject they needs to be deleted before the image
    delete d->shapeController;

    delete d->koShapeController;

    if (d->image) {
        d->image->animationInterface()->blockBackgroundFrameGeneration();

        d->image->notifyAboutToBeDeleted();

        /**
         * WARNING: We should wait for all the internal image jobs to
         * finish before entering KisImage's destructor. The problem is,
         * while execution of KisImage::~KisImage, all the weak shared
         * pointers pointing to the image enter an inconsistent
         * state(!). The shared counter is already zero and destruction
         * has started, but the weak reference doesn't know about it,
         * because KisShared::~KisShared hasn't been executed yet. So all
         * the threads running in background and having weak pointers will
         * enter the KisImage's destructor as well.
         */

        d->image->requestStrokeCancellation();
        d->image->waitForDone();

        // clear undo commands that can still point to the image
        d->undoStack->clear();
        d->image->waitForDone();

        d->image->setUndoStore(new KisDumbUndoStore());
        d->image.clear();
    }

    if (d->wasStorageAdded) {
        if (KisResourceLocator::instance()->hasStorage(d->linkedResourcesStorageID)) {
            KisResourceLocator::instance()->removeStorage(d->linkedResourcesStorageID);
        }
        if (KisResourceLocator::instance()->hasStorage(d->embeddedResourcesStorageID)) {
            KisResourceLocator::instance()->removeStorage(d->embeddedResourcesStorageID);
        }
    }

    delete d;
}

QString KisDocument::embeddedResourcesStorageId() const
{
    return d->embeddedResourcesStorageID;
}

QString KisDocument::linkedResourcesStorageId() const
{
    return d->linkedResourcesStorageID;
}

KisDocument *KisDocument::clone(bool addStorage)
{
    return new KisDocument(*this, addStorage);
}

bool KisDocument::exportDocumentImpl(const KritaUtils::ExportFileJob &job, KisPropertiesConfigurationSP exportConfiguration, bool isAdvancedExporting)
{
    if (job.mimeType == nativeFormatMimeType()) {
        exportConfiguration = exportConfiguration ? KisPropertiesConfigurationSP(new KisPropertiesConfiguration(*exportConfiguration)) :
                                                    KisPropertiesConfigurationSP(new KisPropertiesConfiguration);
        if (!exportConfiguration->hasProperty("compressRasterData"))
            exportConfiguration->setProperty("compressRasterData", KisConfig(true).compressKra());
        exportConfiguration->setPropertyNotSaved("compressRasterData");
        if (exportConfiguration->hasProperty("documentSaveTime")) exportConfiguration->setPropertyNotSaved("documentSaveTime");
    }
    if (d->gpuDocument && job.mimeType != nativeFormatMimeType() &&
        job.mimeType != QByteArrayLiteral("image/png") && job.mimeType != QByteArrayLiteral("image/jpeg"))
        return exportGpuDocument(job, true, exportConfiguration, isAdvancedExporting);
    // ANDROID NOTES (other comments in this file reference this one!)
    //
    // The Android file system doesn't work like on a real operating system.
    // Instead of normal file paths, we get to deal with "content URIs", which
    // can have various kinds of storage providers behind them. For example,
    // there's a "normal" storage provider, a slightly less normal documents
    // provider, a Google Drive provider and various kinds of third-party
    // providers that are mostly just good at losing the data you give them.
    // For example, we have reports of compression programs that provide a
    // storage provider to write to ZIP or RAR archives or something. Except
    // that they don't seem to work at all, they just accept the data with no
    // error and throw it on the floor.
    //
    // The providers are particularly unreliable with regards to permissions,
    // so calling "isWritable" will just always return false on some of them.
    // This includes the default provider on some devices, which means checking
    // whether a file is writable will mean that the user can't save anything!
    // So, all the Android code skips over the writability check and assumes the
    // files are writable. If they're not, we'll notice later anyway, by the
    // fact that writing to them fails.
    //
    // Another issue is that it's only possible to overwrite files using File >
    // Save. Using File > Save As or File > Export can't replace existing
    // files. The reason for this is that we have to go through the operating
    // system to request access to a file and the only things you can ask for
    // is to open an existing file or to create a new file. Out of necessity,
    // Save As and Export use the latter. If the user selects an existing file,
    // the operating system "helpfully" appends a number to the path, *after*
    // the file extension of course, because it hates the living. So that's
    // another reason we can't go with the "safer" option of assuming that files
    // aren't writable in some cases, since that would prevent the user from
    // saving them normally and end up with a lot of "kiki.kra (2)".
    //
    // Also, when you request a file from the operating system, it always
    // creates an empty file. That means checking whether a file exists will
    // pretty much always succeed, so to know whether a file actually exists
    // you have to check whether it isn't empty. Of course providers may fail to
    // implement this correctly, but I'm not aware of any of them botching it
    // that hard. Well, at least none of the ones that actually save files, as
    // mentioned above some of them just seem to lose whatever you give them.

    const Krita::Document::KisDocumentSaveTarget saveTarget =
        Krita::Document::KisDocumentSaveTarget::inspect(job.filePath);
#ifndef Q_OS_ANDROID
    if (saveTarget.exists && !saveTarget.writable) {
        slotCompleteSavingDocument(job, ImportExportCodes::NoAccessToWrite,
                                   i18n("%1 cannot be written to. Please save under a different name.", job.filePath),
                                   "");
        return false;
    }
#endif

    KisConfig cfg(true);
    Krita::Document::KisDocumentBackupOptions backupOptions;
    backupOptions.enabled = cfg.backupFile() && saveTarget.exists;
    switch (cfg.readEntry<int>("backupfilelocation", 0)) {
        case 1:
            backupOptions.location = Krita::Document::KisDocumentBackupLocation::Home;
            break;
        case 2:
            backupOptions.location = Krita::Document::KisDocumentBackupLocation::Temporary;
            break;
        default:
            backupOptions.location = Krita::Document::KisDocumentBackupLocation::Adjacent;
#ifdef Q_OS_MACOS
            KisMacosSecurityBookmarkManager *bookmarkmngr = KisMacosSecurityBookmarkManager::instance();
            if (bookmarkmngr->isSandboxed()) {
                // If the user does not have directory permission force backup
                // files to be inside Container tmp
                QUrl fileUrl = QUrl::fromLocalFile(job.filePath);
                if( !bookmarkmngr->parentDirHasPermissions(fileUrl.path()) ) {
                    backupOptions.forceTemporaryDirectory = true;
                }
            }
#endif
            break;
    }
    backupOptions.numberOfBackups = cfg.readEntry<int>("numberofbackupfiles", 1);
    backupOptions.suffix = cfg.readEntry<QString>("backupfilesuffix", "~");

    const Krita::Document::KisDocumentBackupResult backupResult =
        Krita::Document::KisDocumentBackupFile::create(
            job.filePath,
            backupOptions,
            Krita::Document::KisDocumentAutoSaveFiles::directory());
    const QString backupLocation = backupResult.directory.isEmpty()
        ? QStringLiteral("the same location as the file")
        : backupResult.directory;
    if (backupResult.status == Krita::Document::KisDocumentBackupStatus::Failed) {
        const bool numbered = backupOptions.numberOfBackups > 1;
        qWarning() << "Failed to create backup file!" << job.filePath
                   << backupResult.directory << backupOptions.suffix;
        KisUsageLogger::log(QString("Failed to create a %1 backup for %2 in %3.")
                                .arg(numbered ? QStringLiteral("numbered") : QStringLiteral("simple"),
                                     job.filePath,
                                     backupLocation));
        slotCompleteSavingDocument(
            job,
            ImportExportCodes::ErrorWhileWriting,
            numbered
                ? i18nc("Saving error message", "Failed to create a numbered backup file")
                : i18nc("Saving error message", "Failed to create a backup file"),
            "");
        return false;
    } else if (backupResult.status == Krita::Document::KisDocumentBackupStatus::Created) {
        KisUsageLogger::log(QString("Created a backup for %1 in %2.")
                                .arg(job.filePath, backupLocation));
    }

    //KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(!job.mimeType.isEmpty(), false);
    if (job.mimeType.isEmpty()) {
        KisImportExportErrorCode error = ImportExportCodes::FileFormatNotSupported;
        slotCompleteSavingDocument(job, error, error.errorMessage(), "");
        return false;

    }

    if (d->gpuDocument) return exportGpuDocument(job, true, exportConfiguration, isAdvancedExporting);

    const QString actionName =
            job.flags & KritaUtils::SaveIsExporting ?
                i18n("Exporting Document...") :
                i18n("Saving Document...");

    KritaUtils::BackgroudSavingStartResult result =
            initiateSavingInBackground(actionName,
                                       this, SLOT(slotCompleteSavingDocument(KritaUtils::ExportFileJob, KisImportExportErrorCode, QString, QString)),
                                       job, exportConfiguration, isAdvancedExporting);

    if (result != KritaUtils::BackgroudSavingStartResult::Success) {
        QString errorShortLog;
        QString errorMessage;
        ImportExportCodes::ErrorCodeID errorCode = ImportExportCodes::Failure;

        switch (result) {
        case KritaUtils::BackgroudSavingStartResult::AnotherSavingInProgress:
            errorShortLog = "another save operation is in progress";
            errorMessage = i18n("Could not start saving %1. Wait until the current save operation has finished.", job.filePath);
            errorCode = ImportExportCodes::Failure;
            break;
        case KritaUtils::BackgroudSavingStartResult::ImageLockFailure:
            errorShortLog = "failed to lock and clone the image";
            errorMessage = i18n("Could not start saving %1. Image is busy", job.filePath);
            errorCode = ImportExportCodes::Busy;
            break;
        case KritaUtils::BackgroudSavingStartResult::Failure:
            errorShortLog = "failed to start background saving";
            errorMessage = i18n("Could not start saving %1. Unknown failure has happened", job.filePath);
            errorCode = ImportExportCodes::Failure;
            break;
        case KritaUtils::BackgroudSavingStartResult::Cancelled:
            errorCode = ImportExportCodes::Cancelled;
            break;
        case KritaUtils::BackgroudSavingStartResult::Success:
            // noop, not possible
            break;
        }

        KisUsageLogger::log(QString("Failed to initiate saving %1 in background: %2").arg(job.filePath).arg(errorShortLog));

        slotCompleteSavingDocument(job, errorCode,
                                   errorMessage,
                                   "");
        return false;
    }

    return (result == KritaUtils::BackgroudSavingStartResult::Success);
}

bool KisDocument::exportGpuDocument(const KritaUtils::ExportFileJob &job, bool notifyCompletion,
                                    KisPropertiesConfigurationSP exportConfiguration, bool isAdvancedExporting)
{
    namespace GpuDocument = LibrePaint::Document::Gpu;
    const auto fileDocument = d->gpuDocument;
    if (!fileDocument) return false;
    KisImportExportErrorCode status = ImportExportCodes::Failure;
    QString diagnostic;
    QString warning;
    const bool rasterExport = job.mimeType == QByteArrayLiteral("image/png") ||
        job.mimeType == QByteArrayLiteral("image/jpeg");
    const bool exporting = job.flags & KritaUtils::SaveIsExporting;
    const auto saveTime = exportConfiguration && exportConfiguration->hasProperty("documentSaveTime")
        ? exportConfiguration->getProperty("documentSaveTime").toDateTime() : QDateTime::currentDateTime();
    if (job.mimeType == nativeFormatMimeType() && !saveTime.isValid()) {
        status = ImportExportCodes::FormatFeaturesUnsupported;
        diagnostic = i18n("The document save time is invalid.");
    } else if (rasterExport && isAdvancedExporting) {
        status = ImportExportCodes::FormatFeaturesUnsupported;
        diagnostic = i18n("Resizing during export requires GPU document support.");
    } else if (job.mimeType != nativeFormatMimeType() && !rasterExport) {
        status = ImportExportCodes::FileFormatNotSupported;
        diagnostic = i18n("This document supports saving and exporting in KRA, PNG and JPEG. Other formats require GPU document support.");
    } else if (isSaving()) {
        status = ImportExportCodes::Busy;
        diagnostic = i18n("Another save operation is in progress.");
    } else if (rasterExport && !exporting && !fileDocument->image()->operations().snapshot()->isCommitted()) {
        status = ImportExportCodes::Busy;
        diagnostic = i18n("Finish the active painting operation before saving.");
    } else if (rasterExport) {
        QPointer<KisDocument> guard(this);
        const auto pinned = fileDocument->image()->operations().snapshot();
        d->gpuSaving = true;
        if (!exporting) d->modificationState.markSavingStarted();
        try {
            // Keep GPU pixels and format metadata independent of edits made in the export dialog.
            std::unique_ptr<KisDocument> snapshot(clone(false));
            snapshot->d->savingImage = snapshot->d->image;
            const auto clearSavingImage = qScopeGuard([&snapshot] { snapshot->d->savingImage.clear(); });
            status = snapshot->d->importExportManager->exportDocument(
                job.filePath, job.filePath, job.mimeType, job.flags & KritaUtils::SaveShowWarnings,
                exportConfiguration);
            diagnostic = snapshot->errorMessage();
            warning = snapshot->warningMessage();
            if (!status.isOk() && diagnostic.isEmpty()) diagnostic = status.errorMessage();
        } catch (const std::bad_alloc &) {
            status = ImportExportCodes::InsufficientMemory;
            diagnostic = status.errorMessage();
        } catch (const std::exception &error) {
            status = ImportExportCodes::Failure;
            diagnostic = QString::fromUtf8(error.what());
        }
        if (!guard) return status.isOk();
        if (d->gpuDocument != fileDocument) {
            d->gpuSaving = false;
            return status.isOk();
        }
        if (status.isOk() && !exporting) {
            const auto adopted = fileDocument->adoptSavedFile(job.filePath, pinned);
            if (!guard) return true;
            if (d->gpuDocument != fileDocument) {
                d->gpuSaving = false;
                return true;
            }
            if (adopted.code != GpuDocument::FileCode::Success) {
                status = ImportExportCodes::Failure;
                diagnostic = adopted.diagnostic;
            } else if (!d->modificationState.wasModifiedWhileSaving() &&
                       fileDocument->image()->operations().snapshot() == pinned) {
                d->modificationState.clearImageModifiedWithoutUndo();
            }
        }
        d->gpuSaving = false;
    } else {
        QPointer<KisDocument> guard(this);
        d->gpuSaving = true;
        if (!exporting) d->modificationState.markSavingStarted();
        auto outcome = applyGpuKraMetadata(*this, *fileDocument, saveTime,
            exportConfiguration ? std::optional<bool>(exportConfiguration->getBool("compressRasterData", KisConfig(true).compressKra())) : std::nullopt);
        if (outcome.code == GpuDocument::FileCode::Success)
            outcome = exporting ? fileDocument->exportKra(job.filePath) : fileDocument->saveKra(job.filePath);
        const bool success = outcome.code == GpuDocument::FileCode::Success;
        diagnostic = outcome.diagnostic;
        status = gpuFileStatus(outcome.code);
        if (!guard) return status.isOk();
        d->gpuSaving = false;
        if (d->gpuDocument != fileDocument) return status.isOk();
        if (success && !exporting && !d->modificationState.wasModifiedWhileSaving() &&
            fileDocument->image()->operations().snapshot() == outcome.version)
            d->modificationState.clearImageModifiedWithoutUndo();
    }
    d->lastErrorMessage = diagnostic;
    d->lastWarningMessage = warning;
    if (notifyCompletion) slotCompleteSavingDocument(job, status, diagnostic, warning);
    return status.isOk();
}

bool KisDocument::exportDocument(const QString &path, const QByteArray &mimeType, bool isAdvancedExporting, bool showWarnings, KisPropertiesConfigurationSP exportConfiguration)
{
    using namespace KritaUtils;

    SaveFlags flags = SaveIsExporting;
    if (showWarnings) {
        flags |= SaveShowWarnings;
    }

    KisUsageLogger::log(QString("Exporting Document: %1 as %2. %3 * %4 pixels, %5 layers, %6 frames, %7 "
                                "framerate. Export configuration: %8")
                            .arg(path, QString::fromLatin1(mimeType), QString::number(d->image->width()),
                                 QString::number(d->image->height()), QString::number(d->image->nlayers()),
                                 QString::number(d->image->animationInterface()->totalLength()),
                                 QString::number(d->image->animationInterface()->framerate()),
                                 (exportConfiguration ? exportConfiguration->toXML() : "No configuration")));

    return exportDocumentImpl(KritaUtils::ExportFileJob(path,
                                                        mimeType,
                                                        flags),
                              exportConfiguration, isAdvancedExporting);
}

bool KisDocument::saveAs(const QString &_path, const QByteArray &mimeType, bool showWarnings, KisPropertiesConfigurationSP exportConfiguration)
{
    using namespace KritaUtils;

    KisUsageLogger::log(QString("Saving Document %9 as %1 (mime: %2). %3 * %4 pixels, %5 layers.  %6 frames, "
                                "%7 framerate. Export configuration: %8")
                            .arg(_path, QString::fromLatin1(mimeType), QString::number(d->image->width()),
                                 QString::number(d->image->height()), QString::number(d->image->nlayers()),
                                 QString::number(d->image->animationInterface()->totalLength()),
                                 QString::number(d->image->animationInterface()->framerate()),
                                 (exportConfiguration ? exportConfiguration->toXML() : "No configuration"),
                                 path()));

    // Check whether it's an existing resource were are saving to
    if (resourceSavingFilter(_path, mimeType, exportConfiguration)) {
        return true;
    }

    return exportDocumentImpl(ExportFileJob(_path,
                                            mimeType,
                                            showWarnings ? SaveShowWarnings : SaveNone),
                              exportConfiguration);
}

bool KisDocument::save(bool showWarnings, KisPropertiesConfigurationSP exportConfiguration)
{
    return saveAs(path(), mimeType(), showWarnings, exportConfiguration);
}

QByteArray KisDocument::serializeToNativeByteArray()
{
    QBuffer buffer;

    if (const auto fileDocument = d->gpuDocument) {
        if (!buffer.open(QIODevice::ReadWrite)) return {};
        try {
            auto outcome = applyGpuKraMetadata(*this, *fileDocument);
            if (outcome.code == LibrePaint::Document::Gpu::FileCode::Success)
                outcome = fileDocument->exportKra(buffer);
            d->lastErrorMessage = outcome.diagnostic;
            return outcome.code == LibrePaint::Document::Gpu::FileCode::Success ? buffer.data() : QByteArray();
        } catch (const std::exception &error) {
            d->lastErrorMessage = QString::fromUtf8(error.what());
            return {};
        }
    }

    QScopedPointer<KisImportExportFilter> filter(KisImportExportManager::filterForMimeType(nativeFormatMimeType(), KisImportExportManager::Export));
    filter->setBatchMode(true);
    filter->setMimeType(nativeFormatMimeType());

    Private::StrippedSafeSavingLocker locker(&d->savingMutex, d->image);
    if (!locker.successfullyLocked()) {
        return buffer.data();
    }

    d->savingImage = d->image;

    if (!filter->convert(this, &buffer).isOk()) {
        qWarning() << "serializeToByteArray():: Could not export to our native format";
    }

    return buffer.data();
}

void KisDocument::slotCompleteSavingDocument(const KritaUtils::ExportFileJob &job, KisImportExportErrorCode status, const QString &errorMessage, const QString &warningMessage)
{
    if (status.isCancelled()) {
        return;
    }

    d->ioPresentation->presentSaveResult(job.filePath,
                                         status,
                                         errorMessage,
                                         warningMessage,
                                         fileBatchMode());

    if (!status.isOk()) {
        return;
    }

    if (!(job.flags & KritaUtils::SaveIsExporting)) {
        const QString existingAutoSaveBaseName = localFilePath();
        const bool wasRecovered = isRecovered();

        d->updateDocumentMetadataOnSaving(job.filePath, job.mimeType);

        Krita::Document::KisDocumentAutoSaveFiles::removeForDocument(
            existingAutoSaveBaseName,
            wasRecovered,
            objectName(),
            KisConfig(true).readEntry<bool>("autosavefileshidden"));
    }

    d->ioPresentation->notifySaveSucceeded(job.filePath);
}

void KisDocument::Private::updateDocumentMetadataOnSaving(const QString &filePath, const QByteArray &mimeType)
{
    q->setPath(filePath);
    q->setLocalFilePath(filePath);
    q->setMimeType(mimeType);
    q->updateEditingTime(true);

    q->setReadWrite(Krita::Document::KisDocumentSaveTarget::inspect(filePath).writable);

    if (image && image->gpuImage()) {
        if (!modificationState.wasModifiedWhileSaving()) q->setImageModified();
    } else if (!modificationState.wasModifiedWhileSaving()) {
        /**
         * If undo stack is already clean/empty, it doesn't Q_EMIT any
         * signals, so we might forget update document modified state
         * (which was set, e.g. while recovering an autosave file)
         */

        if (undoStack->isClean()) {
            q->setModified(false);
        } else {
            modificationState.clearImageModifiedWithoutUndo();
            undoStack->setClean();
        }
    }
    q->setRecovered(false);
}

QByteArray KisDocument::mimeType() const
{
    return d->identity.mimeType();
}

void KisDocument::setMimeType(const QByteArray & mimeType)
{
    d->identity.setMimeType(mimeType);
}

bool KisDocument::fileBatchMode() const
{
    return d->batchMode;
}

void KisDocument::setFileBatchMode(const bool batchMode)
{
    d->batchMode = batchMode;
}

void KisDocument::Private::uploadLinkedResourcesFromLayersToStorage()
{
    /// Fetch resources from KisAdjustmentLayer, KisFilterMask and
    /// KisGeneratorLayer and put them into the cloned storage. This must be
    /// done in the context of the GUI thread, otherwise we will not be able to
    /// access resources database

    KisDocument *doc = q;

    KisLayerUtils::recursiveApplyNodes(doc->image()->root(),
        [doc] (KisNodeSP node) {
            if (KisNodeFilterInterface *layer = dynamic_cast<KisNodeFilterInterface*>(node.data())) {
                KisFilterConfigurationSP filterConfig = layer->filter();
                if (!filterConfig) return;

                QList<KoResourceLoadResult> linkedResources = filterConfig->linkedResources(KisGlobalResourcesInterface::instance());

                Q_FOREACH (const KoResourceLoadResult &result, linkedResources) {
                    KIS_SAFE_ASSERT_RECOVER(result.type() != KoResourceLoadResult::EmbeddedResource) { continue; }

                    KoResourceSP resource = result.resource();

                    if (!resource) {
                        qWarning() << "WARNING: KisDocument::lockAndCloneForSaving failed to fetch a resource" << result.signature();
                        continue;
                    }

                    QBuffer buf;
                    buf.open(QBuffer::WriteOnly);

                    KisResourceModel model(resource->resourceType().first);
                    bool res = model.exportResource(resource, &buf);

                    buf.close();

                    if (!res) {
                        qWarning() << "WARNING: KisDocument::lockAndCloneForSaving failed to export resource" << result.signature();
                        continue;
                    }

                    buf.open(QBuffer::ReadOnly);

                    res = doc->d->linkedResourceStorage->importResource(resource->resourceType().first + "/" + resource->filename(), &buf);

                    buf.close();

                    if (!res) {
                        qWarning() << "WARNING: KisDocument::lockAndCloneForSaving failed to import resource" << result.signature();
                        continue;
                    }
                }

            }
    });
}

KisDocument *KisDocument::Private::lockAndCloneImpl(bool fetchResourcesFromLayers)
{
    // force update of all the asynchronous nodes before cloning
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    KisLayerUtils::forceAllDelayedNodesUpdate(image->root());

    KisMainWindow *window = KisPart::instance()->currentMainwindow();
    if (window) {
        if (window->viewManager()) {
            if (!window->viewManager()->blockUntilOperationsFinished(image)) {
                return 0;
            }
        }
    }

    Private::StrippedSafeSavingLocker locker(&savingMutex, image);
    if (!locker.successfullyLocked()) {
        return 0;
    }

    KisDocument *doc = new KisDocument(*this->q, false);

    if (fetchResourcesFromLayers) {
        doc->d->uploadLinkedResourcesFromLayersToStorage();
    }

    return doc;
}

KisDocument* KisDocument::lockAndCloneForSaving()
{
    return d->lockAndCloneImpl(true);
}

KisDocument *KisDocument::lockAndCreateSnapshot()
{
    return d->lockAndCloneImpl(false);
}

void KisDocument::copyFromDocument(const KisDocument &rhs)
{
    copyFromDocumentImpl(rhs, REPLACE);
}

void KisDocument::copyFromDocumentImpl(const KisDocument &rhs, CopyPolicy policy)
{
    if (policy == REPLACE && d->image && rhs.d->image &&
        bool(d->image->gpuImage()) != bool(rhs.d->image->gpuImage()))
        throw std::invalid_argument("Document replacement requires matching painting backends");
    LibrePaint::Image::Gpu::PaintImageSP preparedGpuImage;
    if (policy == REPLACE && d->image && rhs.d->image && rhs.d->image->gpuImage()) {
        preparedGpuImage = rhs.d->image->gpuImage()->cloneTo(*d->image->gpuImage());
        if (!preparedGpuImage) throw std::invalid_argument("Document replacement requires the owning thread");
    }
    if (policy == REPLACE) {
        d->decorationsSyncingDisabled = true;
        d->copyFrom(*(rhs.d), this);
        d->decorationsSyncingDisabled = false;

        d->undoStack->clear();
    } else {
        // in CONSTRUCT mode, d should be already initialized
        connect(KisConfigNotifier::instance(), SIGNAL(configChanged()), SLOT(slotConfigChanged()));
        connect(d->undoStack, SIGNAL(cleanChanged(bool)), this, SLOT(slotUndoStackCleanChanged(bool)));
        connect(d->autoSaveTimer, SIGNAL(timeout()), this, SLOT(slotAutoSave()));

        d->shapeController = new KisShapeController(d->nserver, d->undoStack, this);
        d->koShapeController = new KoShapeController(0, d->shapeController);
    }

    setObjectName(rhs.objectName());

    slotConfigChanged();

    if (rhs.d->image) {
        if (policy == REPLACE) {
            d->image->barrierLock(/* readOnly = */ false);
            rhs.d->image->barrierLock(/* readOnly = */ true);
            if (preparedGpuImage) d->image->replaceFromGpuImage(preparedGpuImage, *(rhs.d->image));
            else d->image->copyFromImage(*(rhs.d->image));
            d->image->unlock();
            rhs.d->image->unlock();

            setCurrentImage(d->image, /* forceInitialUpdate = */ true);
        } else {
            // clone the image with keeping the GUIDs of the layers intact
            // NOTE: we expect the image to be locked!
            setCurrentImage(rhs.image()->clone(/* exactCopy = */ true), /* forceInitialUpdate = */ false);
        }
    }

    if (policy == REPLACE) {
        d->syncDecorationsWrapperLayerState();
    }

    if (rhs.d->preActivatedNode) {
        QQueue<KisNodeSP> linearizedNodes;
        KisLayerUtils::recursiveApplyNodes(rhs.d->image->root(),
                                           [&linearizedNodes](KisNodeSP node) {
            linearizedNodes.enqueue(node);
        });
        KisLayerUtils::recursiveApplyNodes(d->image->root(),
                                           [&linearizedNodes, &rhs, this](KisNodeSP node) {
            KisNodeSP refNode = linearizedNodes.dequeue();
            if (rhs.d->preActivatedNode.data() == refNode.data()) {
                d->preActivatedNode = node;
            }
        });
    }

    // reinitialize references' signal connection
    KisReferenceImagesLayerSP referencesLayer = this->referenceImagesLayer();
    if (referencesLayer) {
        d->referenceLayerConnections.clear();
        d->referenceLayerConnections.addConnection(
                    referencesLayer, SIGNAL(sigUpdateCanvas(QRectF)),
                    this, SIGNAL(sigReferenceImagesChanged()));

        Q_EMIT sigReferenceImagesLayerChanged(referencesLayer);
        Q_EMIT sigReferenceImagesChanged();
    }

    KisDecorationsWrapperLayerSP decorationsLayer =
            KisLayerUtils::findNodeByType<KisDecorationsWrapperLayer>(d->image->root());
    if (decorationsLayer) {
        decorationsLayer->setDocument(this);
    }


    if (policy == REPLACE) {
        setModified(true);
    }
}

bool KisDocument::exportDocumentSync(const QString &path, const QByteArray &mimeType, KisPropertiesConfigurationSP exportConfiguration)
{
    if (mimeType == nativeFormatMimeType() && exportConfiguration)
        exportConfiguration->setPropertyNotSaved("documentSaveTime");
    if (d->gpuDocument)
        return exportGpuDocument(KritaUtils::ExportFileJob(path, mimeType, KritaUtils::SaveIsExporting), false, exportConfiguration);
    {
        /**
         * The caller guarantees that no one else uses the document (usually,
         * it is a temporary document created specifically for exporting), so
         * we don't need to copy or lock the document. Instead we should just
         * ensure the barrier lock is synced and then released.
         */
        Private::StrippedSafeSavingLocker locker(&d->savingMutex, d->image);
        if (!locker.successfullyLocked()) {
            return false;
        }
    }

    d->savingImage = d->image;

    KisImportExportErrorCode status =
            d->importExportManager->
            exportDocument(path, path, mimeType, false, exportConfiguration);

    d->savingImage = 0;

    return status.isOk();
}


KritaUtils::BackgroudSavingStartResult KisDocument::initiateSavingInBackground(const QString actionName,
                                             const QObject *receiverObject, const char *receiverMethod,
                                             const KritaUtils::ExportFileJob &job,
                                             KisPropertiesConfigurationSP exportConfiguration,bool isAdvancedExporting)
{
    return initiateSavingInBackground(actionName, receiverObject, receiverMethod,
                                      job, exportConfiguration, std::unique_ptr<KisDocument>(), isAdvancedExporting);
}

KritaUtils::BackgroudSavingStartResult KisDocument::initiateSavingInBackground(const QString actionName,
                                             const QObject *receiverObject, const char *receiverMethod,
                                             const KritaUtils::ExportFileJob &job,
                                             KisPropertiesConfigurationSP exportConfiguration,
                                             std::unique_ptr<KisDocument> &&optionalClonedDocument,bool isAdvancedExporting)
{
    KIS_ASSERT_RECOVER_RETURN_VALUE(job.isValid(), KritaUtils::BackgroudSavingStartResult::Failure);

    std::unique_ptr<KisDocument> clonedDocument;

    if (!optionalClonedDocument) {
        clonedDocument.reset(lockAndCloneForSaving());
    } else {
        clonedDocument.reset(optionalClonedDocument.release());
    }

    if (!d->savingMutex.tryLock()){
        return KritaUtils::BackgroudSavingStartResult::AnotherSavingInProgress;
    }

    /**
     * This lock will later release() when we start the background thread,
     * it means that the ownership is transferred to the background thread
     */
    std::unique_lock<QMutex> savingMutexLock(d->savingMutex, std::adopt_lock);

    if (!clonedDocument) {
        return KritaUtils::BackgroudSavingStartResult::ImageLockFailure;
    }

    auto waitForImage = [] (KisImageSP image) {
        KisMainWindow *window = KisPart::instance()->currentMainwindow();
        if (window) {
            if (window->viewManager()) {
                window->viewManager()->blockUntilOperationsFinishedForced(image);
            }
        }
    };

    {
        KisNodeSP newRoot = clonedDocument->image()->root();
        KIS_SAFE_ASSERT_RECOVER(!KisLayerUtils::hasDelayedNodeWithUpdates(newRoot)) {
            KisLayerUtils::forceAllDelayedNodesUpdate(newRoot);
            waitForImage(clonedDocument->image());
        }
    }

    if (clonedDocument->image()->hasOverlaySelectionMask()) {
        clonedDocument->image()->setOverlaySelectionMask(0);
        waitForImage(clonedDocument->image());
    }

    KisConfig cfg(true);
    if (cfg.trimKra()) {
        clonedDocument->image()->cropImage(clonedDocument->image()->bounds());
        clonedDocument->image()->purgeUnusedData(false);
        waitForImage(clonedDocument->image());
    }

    KIS_SAFE_ASSERT_RECOVER(clonedDocument->image()->isIdle()) {
        waitForImage(clonedDocument->image());
    }

    KIS_ASSERT_RECOVER_RETURN_VALUE(!d->backgroundSaveDocument, KritaUtils::BackgroudSavingStartResult::Failure);
    KIS_ASSERT_RECOVER_RETURN_VALUE(!d->backgroundSaveJob.isValid(), KritaUtils::BackgroudSavingStartResult::Failure);

    /**
     * From now on **no** return statements are allowed, even inside
     * asserts, since the ownership over the saving mutex has already
     * been passed to the background thread.
     *
     * The cancellation process should go through
     * slotChildCompletedSavingInBackground(), which will unlock the
     * mutex itself.
     */
    savingMutexLock.release();

    d->backgroundSaveDocument.reset(clonedDocument.release());
    d->backgroundSaveJob = job;
    d->modificationState.markSavingStarted();

    if (d->backgroundSaveJob.flags & KritaUtils::SaveInAutosaveMode) {
        d->backgroundSaveDocument->d->autoSaveState.markAutoSaveExportStarted();
    }

    connect(d->backgroundSaveDocument.get(),
            SIGNAL(sigBackgroundSavingFinished(KisImportExportErrorCode, QString, QString)),
            this,
            SLOT(slotChildCompletedSavingInBackground(KisImportExportErrorCode, QString, QString)));


    if (d->completeSavingConnection) {
        disconnect(d->completeSavingConnection);
    }
    d->completeSavingConnection = connect(
        this, SIGNAL(sigCompleteBackgroundSaving(KritaUtils::ExportFileJob, KisImportExportErrorCode, QString, QString)),
        receiverObject, receiverMethod);

    KisImportExportErrorCode error =
            d->backgroundSaveDocument->startExportInBackground(actionName,
                                                               job.filePath,
                                                               job.filePath,
                                                               job.mimeType,
                                                               job.flags & KritaUtils::SaveShowWarnings,
                                                               exportConfiguration, isAdvancedExporting);
    if (!error.isOk()) {
        // the state should have been deinitialized in slotChildCompletedSavingInBackground()
        KIS_SAFE_ASSERT_RECOVER (!d->backgroundSaveDocument && !d->backgroundSaveJob.isValid()) {
            d->backgroundSaveDocument.release()->deleteLater();
            d->savingMutex.unlock();
            d->backgroundSaveJob = KritaUtils::ExportFileJob();
        }
        if (error.isCancelled()) {
            return KritaUtils::BackgroudSavingStartResult::Cancelled;
        }
        return KritaUtils::BackgroudSavingStartResult::Failure;
    }

    return KritaUtils::BackgroudSavingStartResult::Success;
}


void KisDocument::slotChildCompletedSavingInBackground(KisImportExportErrorCode status, const QString &errorMessage, const QString &warningMessage)
{
    KIS_ASSERT_RECOVER_RETURN(isSaving());

    /**
     * Take back the ownership of the saving mutex and make sure it
     * well be released whatever the result of executing this function
     * will be, even if it asserts.
     */
    std::unique_lock<QMutex> savingMutexLock(d->savingMutex, std::adopt_lock);

    KIS_ASSERT_RECOVER_RETURN(d->backgroundSaveDocument);

    if (d->backgroundSaveJob.flags & KritaUtils::SaveInAutosaveMode) {
        d->backgroundSaveDocument->d->autoSaveState.markAutoSaveExportFinished();
    }

    d->backgroundSaveDocument.release()->deleteLater();

    KIS_ASSERT_RECOVER_RETURN(d->backgroundSaveJob.isValid());

    const KritaUtils::ExportFileJob job = d->backgroundSaveJob;
    d->backgroundSaveJob = KritaUtils::ExportFileJob();

    // unlock at the very end
    savingMutexLock.unlock();

    QFileInfo fi(job.filePath);
    KisUsageLogger::log(QString("Completed saving %1 (mime: %2). Result: %3. Warning: %4. Size: %5")
                            .arg(job.filePath, QString::fromLatin1(job.mimeType),
                                 (!status.isOk() ? errorMessage : "OK"), warningMessage,
                                 QString::number(fi.size())));

    Q_EMIT sigCompleteBackgroundSaving(job, status, errorMessage, warningMessage);
    // One-shot: disconnect immediately after firing so future save completions
    // don't invoke a stale or unrelated slot.
    disconnect(d->completeSavingConnection);
    d->completeSavingConnection = {};

    if (d->recoveryAutoSaveState.hasPendingRequest()) {
        d->recoveryAutoSaveState.recordJoinedSave(
            job.filePath,
            status.isOk() && bool(job.flags & KritaUtils::SaveInAutosaveMode));

        // Do not start another save from inside the completion signal. The
        // previous job has released its mutex, but its receiver may still be
        // unwinding and the one-shot completion connection has only just been
        // cleared.
        QTimer::singleShot(0, this, &KisDocument::slotContinuePendingRecoveryAutoSave);
    }
}

void KisDocument::Private::runPendingGpuAutoSave()
{
    auto job = std::move(gpuAutoSaveJob);
    gpuAutoSaveJob = {};
    if (job) job();
}

KisDocument::RecoveryAutoSaveStartResult KisDocument::Private::startGpuAutoSave(const QString &path, GpuAutoSaveMode mode)
{
    if (q->isSaving()) return RecoveryAutoSaveStartResult::AlreadySaving;
    const auto fileDocument = gpuDocument;
    if (!fileDocument->image()->operations().activeOperation().isNull()) return RecoveryAutoSaveStartResult::Failed;
    const auto pinned = fileDocument->image()->operations().snapshot();
    const KritaUtils::ExportFileJob job(path, nativeFormatMimeType(),
                                      KritaUtils::SaveIsExporting | KritaUtils::SaveInAutosaveMode);
    gpuSaving = true;
    modificationState.markSavingStarted();
    modificationState.markAutoSaveStarted();
    autoSaveState.markAutoSaveExportStarted();
    const auto prepared = applyGpuKraMetadata(*q, *fileDocument);
    gpuAutoSaveJob = [this, fileDocument, pinned, prepared, job, mode] {
        if (gpuDocument != fileDocument) {
            gpuSaving = false;
            autoSaveState.markAutoSaveExportFinished();
            q->finishRecoveryAutoSaveRequest(job.filePath, false);
            return;
        }
        const QPointer<KisDocument> guard(q);
        const auto outcome = prepared.code == LibrePaint::Document::Gpu::FileCode::Success ?
            fileDocument->exportKra(job.filePath, pinned) : prepared;
        if (!guard) return;
        const auto status = gpuFileStatus(outcome.code);
        autoSaveState.markAutoSaveExportFinished();
        gpuSaving = false;
        if (documentIsClosing) return;
        if (gpuDocument != fileDocument) {
            q->finishRecoveryAutoSaveRequest(job.filePath, false);
            return;
        }
        lastErrorMessage = outcome.diagnostic;
        if (mode == GpuAutoSaveMode::Recovery) {
            q->slotCompleteRecoveryAutoSaving(job, status, outcome.diagnostic, {});
        } else {
            q->slotCompleteAutoSaving(job, status, outcome.diagnostic, {});
            if (guard && gpuDocument != fileDocument) {
                q->finishRecoveryAutoSaveRequest(job.filePath, false);
                return;
            }
            if (guard && recoveryAutoSaveState.hasPendingRequest()) {
                recoveryAutoSaveState.recordJoinedSave(job.filePath, status.isOk());
            }
        }
        if (guard && recoveryAutoSaveState.hasPendingRequest()) {
            QTimer::singleShot(0, q, &KisDocument::slotContinuePendingRecoveryAutoSave);
        }
    };
    if (mode == GpuAutoSaveMode::Pause) {
        runPendingGpuAutoSave();
    } else {
        QTimer::singleShot(0, q, [this] { runPendingGpuAutoSave(); });
    }
    return RecoveryAutoSaveStartResult::Started;
}

void KisDocument::slotAutoSaveImpl(std::unique_ptr<KisDocument> &&optionalClonedDocument)
{
    if (!d->modificationState.isModified() ||
        !d->modificationState.hasChangesAfterAutoSave()) {
        return;
    }
    const QString autoSaveFileName = autoSaveFilePath(this, localFilePath());

    const QPointer<KisDocument> guard(this);
    const auto fileDocument = d->gpuDocument;
    d->ioPresentation->notifyAutoSaveStarted(autoSaveFileName);
    if (!guard || d->gpuDocument != fileDocument) return;

    KisUsageLogger::log(QString("Autosaving: %1").arg(autoSaveFileName));

    if (fileDocument) {
        if (isSaving() || !fileDocument->image()->operations().activeOperation().isNull()) {
            d->ioPresentation->notifyAutoSavePostponed();
            if (guard) setEmergencyAutoSaveInterval();
            return;
        }
        d->startGpuAutoSave(autoSaveFileName, Private::GpuAutoSaveMode::Timer);
        return;
    }

    const bool hadClonedDocument = bool(optionalClonedDocument);
    KritaUtils::BackgroudSavingStartResult result = KritaUtils::BackgroudSavingStartResult::Failure;

    if (d->image->isIdle() || hadClonedDocument) {
        result = initiateSavingInBackground(i18n("Autosaving..."),
                                             this, SLOT(slotCompleteAutoSaving(KritaUtils::ExportFileJob, KisImportExportErrorCode, QString, QString)),
                                             KritaUtils::ExportFileJob(autoSaveFileName, nativeFormatMimeType(), KritaUtils::SaveIsExporting | KritaUtils::SaveInAutosaveMode),
                                             0,
                                             std::move(optionalClonedDocument));
    } else {
        d->ioPresentation->notifyAutoSavePostponed();
    }

    if (result != KritaUtils::BackgroudSavingStartResult::Success &&
        !hadClonedDocument &&
        d->autoSaveState.shouldUseCloneFallback()) {
        KisCloneDocumentStroke *stroke = new KisCloneDocumentStroke(this);
        connect(stroke, SIGNAL(sigDocumentCloned(KisDocument*)),
                this, SLOT(slotInitiateAsyncAutosaving(KisDocument*)),
                Qt::BlockingQueuedConnection);
        connect(stroke, SIGNAL(sigCloningCancelled()),
                this, SLOT(slotDocumentCloningCancelled()),
                Qt::BlockingQueuedConnection);

        KisStrokeId strokeId = d->image->startStroke(stroke);
        d->image->endStroke(strokeId);

        setInfiniteAutoSaveInterval();

    } else if (result != KritaUtils::BackgroudSavingStartResult::Success) {
        setEmergencyAutoSaveInterval();
    } else {
        d->modificationState.markAutoSaveStarted();
    }
}

bool KisDocument::resourceSavingFilter(const QString &path, const QByteArray &mimeType, KisPropertiesConfigurationSP exportConfiguration)
{
    if (QFileInfo(path).absolutePath().startsWith(KisResourceLocator::instance()->resourceLocationBase())) {

        QStringList pathParts = QFileInfo(path).absolutePath().split('/');
        if (pathParts.size() > 0) {
            QString resourceType = pathParts.last();
            if (KisResourceLoaderRegistry::instance()->resourceTypes().contains(resourceType)) {

                KisResourceModel model(resourceType);
                model.setResourceFilter(KisResourceModel::ShowAllResources);

                QString tempFileName = QDir::tempPath() + "/" + QFileInfo(path).fileName();

                if (QFileInfo(path).exists()) {

                    int outResourceId;
                    KoResourceSP res;
                    if (KisResourceCacheDb::getResourceIdFromVersionedFilename(QFileInfo(path).fileName(), resourceType, "", outResourceId)) {
                        res = model.resourceForId(outResourceId);
                    }

                    if (res) {
                        d->modificationState.markSavingStarted();

                        if (!exportConfiguration) {
                            QScopedPointer<KisImportExportFilter> filter(
                                KisImportExportManager::filterForMimeType(mimeType, KisImportExportManager::Export));
                            if (filter) {
                                exportConfiguration = filter->defaultConfiguration(nativeFormatMimeType(), mimeType);
                            }
                        }

                        if (exportConfiguration) {
                            // make sure the name of the resource doesn't change
                            exportConfiguration->setProperty("name", res->name());
                        }

                        if (exportDocumentSync(tempFileName, mimeType, exportConfiguration)) {
                            QFile f2(tempFileName);
                            KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(f2.open(QFile::ReadOnly), false);

                            QByteArray ba = f2.readAll();

                            QBuffer buf(&ba);
                            buf.open(QBuffer::ReadOnly);



                            if (res->loadFromDevice(&buf, KisGlobalResourcesInterface::instance())) {
                                if (model.updateResource(res)) {
                                    const QString filePath =
                                        KisResourceLocator::instance()->filePathForResource(res);

                                    d->updateDocumentMetadataOnSaving(filePath, mimeType);

                                    return true;
                                }
                            }
                        }
                    }
                }
                else {
                    d->modificationState.markSavingStarted();
                    if (exportDocumentSync(tempFileName, mimeType, exportConfiguration)) {
                        KoResourceSP res = model.importResourceFile(tempFileName, false);
                        if (res) {
                            const QString filePath =
                                KisResourceLocator::instance()->filePathForResource(res);

                            d->updateDocumentMetadataOnSaving(filePath, mimeType);

                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

void KisDocument::slotAutoSave()
{
    slotAutoSaveImpl(std::unique_ptr<KisDocument>());
}

void KisDocument::slotInitiateAsyncAutosaving(KisDocument *clonedDocument)
{
    slotAutoSaveImpl(std::unique_ptr<KisDocument>(clonedDocument));
}

void KisDocument::slotDocumentCloningCancelled()
{
    setEmergencyAutoSaveInterval();
}

void KisDocument::slotPerformIdleRoutines()
{
    d->image->explicitRegenerateLevelOfDetail();


    /// TODO: automatic purging is disabled for now: it modifies
    ///       data managers without creating a transaction, which breaks
    ///       undo.

    // d->image->purgeUnusedData(true);
}

void KisDocument::slotCompleteAutoSaving(const KritaUtils::ExportFileJob &job, KisImportExportErrorCode status, const QString &errorMessage, const QString &warningMessage)
{
    Q_UNUSED(job);
    Q_UNUSED(warningMessage);

    const QString fileName = QFileInfo(job.filePath).fileName();

    if (!status.isOk()) {
        // slotAutoSaveImpl() clears this flag when the background job starts
        // so edits made during that job can be detected. A failed job did not
        // create a usable checkpoint, therefore the document must remain
        // eligible for another autosave even when no newer edit arrived.
        d->modificationState.restoreAutoSaveRequirement();
        setEmergencyAutoSaveInterval();
        d->ioPresentation->notifyAutoSaveFailed(fileName, status, errorMessage);
    } else {
        KisConfig cfg(true);
        d->autoSaveDelay = cfg.autoSaveInterval();

        if (!d->modificationState.wasModifiedWhileSaving()) {
            d->autoSaveTimer->stop(); // until the next change
            d->autoSaveState.clearFailures();
        } else {
            setNormalAutoSaveInterval();
        }

        d->ioPresentation->notifyAutoSaveFinished(fileName);
    }
}

void KisDocument::slotCompleteRecoveryAutoSaving(const KritaUtils::ExportFileJob &job,
                                                 KisImportExportErrorCode status,
                                                 const QString &errorMessage,
                                                 const QString &warningMessage)
{
    const QPointer<KisDocument> guard(this);
    const KisImageWSP image = d->image;
    slotCompleteAutoSaving(job, status, errorMessage, warningMessage);
    if (!guard) return;
    if (!image.isValid() || d->image.data() != image.data()) {
        finishRecoveryAutoSaveRequest(job.filePath, false);
        return;
    }

    const bool saved = status.isOk()
        && Krita::Document::KisDocumentAutoSaveFiles::isUsable(job.filePath);

    if (!saved) {
        d->modificationState.restoreAutoSaveRequirement();
        if (status.isOk()) {
            setEmergencyAutoSaveInterval();
        }
        finishRecoveryAutoSaveRequest(job.filePath, false);
        return;
    }

    if (d->modificationState.hasChangesAfterAutoSave()) {
        // The just-finished snapshot is valid, but a newer modification was
        // observed while it was being written. Keep the request pending; the
        // common completion path queues a fresh recovery autosave after the
        // current signal stack has unwound.
        return;
    }

    finishRecoveryAutoSaveRequest(job.filePath, true);
}

void KisDocument::slotContinuePendingRecoveryAutoSave()
{
    if (!d->recoveryAutoSaveState.hasPendingRequest() || d->documentIsClosing) {
        return;
    }

    if (isSaving()) {
        // Most background jobs queue another pass from their completion
        // signal. Poll as well so a save implementation that only holds the
        // saving mutex, without emitting that signal, cannot strand the
        // recovery request indefinitely.
        QTimer::singleShot(50, this, &KisDocument::slotContinuePendingRecoveryAutoSave);
        return;
    }

    if (!d->modificationState.isModified()) {
        finishRecoveryAutoSaveRequest(QString(), true);
        return;
    }

    if (!d->modificationState.hasChangesAfterAutoSave()) {
        QString recoveryPath = d->recoveryAutoSaveState.joinedSavePath();
        if (recoveryPath.isEmpty()) {
            recoveryPath = autoSaveFilePath(this, localFilePath());
        }

        if (Krita::Document::KisDocumentAutoSaveFiles::isUsable(recoveryPath)) {
            finishRecoveryAutoSaveRequest(recoveryPath, true);
            return;
        }

        // The prior job claimed the current revision, but there is no usable
        // file to join. Force a fresh recovery snapshot.
        d->modificationState.restoreAutoSaveRequirement();
    }

    d->recoveryAutoSaveState.clearJoinedSavePath();
    const QPointer<KisDocument> guard(this);
    const RecoveryAutoSaveStartResult result = startRecoveryAutoSave();
    if (!guard) return;
    if (result == RecoveryAutoSaveStartResult::Started ||
        result == RecoveryAutoSaveStartResult::AlreadySaving) {
        return;
    }

    d->modificationState.restoreAutoSaveRequirement();
    finishRecoveryAutoSaveRequest(autoSaveFilePath(this, localFilePath()), false);
}

KisImportExportErrorCode KisDocument::startExportInBackground(const QString &actionName,
                                          const QString &location,
                                          const QString &realLocation,
                                          const QByteArray &mimeType,
                                          bool showWarnings,
                                          KisPropertiesConfigurationSP exportConfiguration, bool isAdvancedExporting)
{
    d->savingImage = d->image;

    KisMainWindow *window = KisPart::instance()->currentMainwindow();
    if (window) {
        if (window->viewManager()) {
            d->savingUpdater = window->viewManager()->createThreadedUpdater(actionName);
            d->importExportManager->setUpdater(d->savingUpdater);
        }
    }

    KisImportExportErrorCode initializationStatus(ImportExportCodes::OK);
    d->childSavingFuture =
            d->importExportManager->exportDocumentAsync(location,
                                                       realLocation,
                                                       mimeType,
                                                       initializationStatus,
                                                       showWarnings,
                                                       exportConfiguration,
                                                       isAdvancedExporting);

    if (!initializationStatus.isOk()) {
        if (d->savingUpdater) {
            d->savingUpdater->cancel();
        }
        d->savingImage.clear();
        Q_EMIT sigBackgroundSavingFinished(initializationStatus, initializationStatus.errorMessage(), "");
        return initializationStatus;
    }

    typedef QFutureWatcher<KisImportExportErrorCode> StatusWatcher;
    StatusWatcher *watcher = new StatusWatcher();
    watcher->setFuture(d->childSavingFuture);

    connect(watcher, SIGNAL(finished()), SLOT(finishExportInBackground()));
    connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));

    return initializationStatus;
}

void KisDocument::finishExportInBackground()
{
    KIS_SAFE_ASSERT_RECOVER(d->childSavingFuture.isFinished()) {
        Q_EMIT sigBackgroundSavingFinished(ImportExportCodes::InternalError, "", "");
        return;
    }

    KisImportExportErrorCode status = d->childSavingFuture.result();
    QString errorMessage = status.errorMessage();
    QString warningMessage = d->lastWarningMessage;

    if (!d->lastErrorMessage.isEmpty()) {
        if (status == ImportExportCodes::InternalError || status == ImportExportCodes::Failure) {
            errorMessage = d->lastErrorMessage;
        } else {
            errorMessage += "\n" + d->lastErrorMessage;
        }
    }

    d->savingImage.clear();
    d->childSavingFuture = QFuture<KisImportExportErrorCode>();
    d->lastErrorMessage.clear();
    d->lastWarningMessage.clear();

    if (d->savingUpdater) {
        d->savingUpdater->setProgress(100);
    }

    Q_EMIT sigBackgroundSavingFinished(status, errorMessage, warningMessage);
}

void KisDocument::setReadWrite(bool readwrite)
{
    const bool changed = readwrite != d->readwrite;

    d->readwrite = readwrite;

    if (changed) {
        Q_EMIT sigReadWriteChanged(readwrite);
    }
}

void KisDocument::setAutoSaveActive(bool autoSaveActive)
{
    const bool changed = autoSaveActive != d->autoSaveActive;

    if (changed) {
        d->autoSaveActive = autoSaveActive;
        setNormalAutoSaveInterval();
    }
}

void KisDocument::setAutoSaveDelay(int delay)
{
    if (isReadWrite() && delay > 0 && d->autoSaveActive) {
        d->autoSaveTimer->start(delay * 1000);
    } else {
        d->autoSaveTimer->stop();
    }
}

void KisDocument::setNormalAutoSaveInterval()
{
    setAutoSaveDelay(d->autoSaveDelay);
    d->autoSaveState.clearFailures();
}

void KisDocument::setEmergencyAutoSaveInterval()
{
    const int emergencyAutoSaveInterval = 10; /* sec */
    setAutoSaveDelay(emergencyAutoSaveInterval);
    d->autoSaveState.recordFailure();
}

void KisDocument::setInfiniteAutoSaveInterval()
{
    setAutoSaveDelay(-1);
}

bool KisDocument::isAutoSaveActive()
{
    return d->autoSaveActive;
}

KoDocumentInfo *KisDocument::documentInfo() const
{
    return d->docInfo;
}

bool KisDocument::isModified() const
{
    return d->modificationState.isModified();
}

QPixmap KisDocument::generatePreview(const QSize& size)
{
    return QPixmap::fromImage(generatePreviewImage(size));
}

QImage KisDocument::generatePreviewImage(const QSize& size)
{
    KisImageSP image = d->image;
    if (d->savingImage) image = d->savingImage;

    if (image) {
        QRect bounds = image->bounds();
        QSize originalSize = bounds.size();
        // QSize may round down one dimension to zero on extreme aspect rations, so ensure 1px minimum
        QSize newSize = originalSize.scaled(size, Qt::KeepAspectRatio).expandedTo({1, 1});

        bool pixelArt = false;
        // determine if the image is pixel art or not
        if (originalSize.width() < size.width() && originalSize.height() < size.height()) {
            // the image must be smaller than the requested preview
            // the scale must be integer
            if (newSize.height()%originalSize.height() == 0 && newSize.width()%originalSize.width() == 0) {
                pixelArt = true;
            }
        }

        QImage preview;
        if (pixelArt) {
            // do not scale while converting (because it uses Bicubic)
            QImage original = image->convertToQImage(originalSize, 0);
            // scale using FastTransformation, which is probably Nearest neighbour, suitable for pixel art
            preview = original.scaled(newSize, Qt::KeepAspectRatio, Qt::FastTransformation);
        } else {
            preview = image->convertToQImage(newSize, 0);
        }
        if (preview.isNull()) {
            preview = QImage(newSize, QImage::Format_ARGB32);
            QPainter gc(&preview);
            QBrush checkBrush = QBrush(KisCanvasWidgetBase::createCheckersImage(newSize.width() / 5));
            gc.fillRect(preview.rect(), checkBrush);
            gc.end();
        }
        return preview;
    }
    QImage preview(size, QImage::Format_ARGB32);
    preview.fill(Qt::transparent);
    return preview;
}

bool KisDocument::importDocument(const QString &_path)
{
    bool ret;

    dbgUI << "path=" << _path;

    // open...
    ret = openPath(_path);

    // reset url & m_file (kindly? set by KisParts::openUrl()) to simulate a
    // File --> Import
    if (ret) {
        dbgUI << "success, resetting url";
        resetPath();
    }

    return ret;
}


bool KisDocument::openPath(const QString &_path, OpenFlags flags)
{
    QPointer<KisDocument> guard(this);
    dbgUI << "path=" << _path;
    d->lastErrorMessage.clear();

    // Reimplemented, to add a check for autosave files and to improve error reporting
    if (_path.isEmpty()) {
        d->lastErrorMessage = i18n("Malformed Path\n%1", _path);  // ## used anywhere ?
        return false;
    }

    QString path = _path;
    QString original  = "";
    bool autosaveOpened = false;
    if (!fileBatchMode()) {
        QString file = path;
        QString asf = autoSaveFilePath(this, file);
        if (QFile::exists(asf)) {
            KisApplication *kisApp = static_cast<KisApplication*>(qApp);
            kisApp->hideSplashScreen();
            //qDebug() <<"asf=" << asf;
            // ## TODO compare timestamps ?
            KisRecoverNamedAutosaveDialog dlg;
            KisFileIconCreator iconCreator;
            QIcon mainFileIcon;
            QIcon autosaveFileIcon;
            const QSize iconSize = dlg.filePreviewIconSize();
            if (iconCreator.createFileIcon(file, mainFileIcon, dlg.devicePixelRatioF(), iconSize)) {
                dlg.setMainFileIcon(mainFileIcon);
            }
            if (iconCreator.createFileIcon(asf, autosaveFileIcon, dlg.devicePixelRatioF(), iconSize)) {
                dlg.setAutosaveFileIcon(autosaveFileIcon);
            }
            dlg.exec();
            int res = dlg.result();

            switch (res) {
            case KisRecoverNamedAutosaveDialog::OpenAutosave :
                original = file;
                path = asf;
                autosaveOpened = true;
                break;
            case KisRecoverNamedAutosaveDialog::OpenMainFile :
                KisUsageLogger::log(QString("Removing autosave file: %1").arg(asf));
                QFile::remove(asf);
                break;
            default: // Cancel
                return false;
            }
        }
    }

    bool ret = openPathInternal(path);
    if (!guard || !ret) return ret;

    if (autosaveOpened || flags & RecoveryFile) {
        setReadWrite(true); // enable save button
        setModified(true);
        setRecovered(true);

        setPath(original); // since it was an autosave, it will be a local file
        setLocalFilePath(original);
    }
    else {
        if (ret) {

            if (!(flags & DontAddToRecent)) {
                KisPart::instance()->addRecentURLToAllMainWindows(QUrl::fromLocalFile(_path));
            }

#ifdef Q_OS_ANDROID
            // See the comment titled "ANDROID NOTES" in this file for an
            // explanation of what this is about. (This is not that comment.)
            setReadWrite(true);
#else
            QFileInfo fi(_path);
            setReadWrite(fi.isWritable());
#endif
        }

        setRecovered(false);
    }

    return ret;
}

bool KisDocument::openFile()
{
    QPointer<KisDocument> guard(this);
    //dbgUI <<"for" << localFilePath();
    if (!QFile::exists(localFilePath()) && !fileBatchMode()) {
        d->ioPresentation->presentMissingFile(localFilePath(), fileBatchMode());
        return false;
    }

    QString filename = localFilePath();
    QString typeName = mimeType();

    if (typeName.isEmpty()) {
        typeName = KisMimeDatabase::mimeTypeForFile(filename);
    }

    // Allow to open backup files, don't keep the mimeType application/x-trash.
    if (typeName == "application/x-trash") {
        QString path = filename;
        while (path.length() > 0) {
            path.chop(1);
            typeName = KisMimeDatabase::mimeTypeForFile(path);
            //qDebug() << "\t" << path << typeName;
            if (!typeName.isEmpty()) {
                break;
            }
        }
        //qDebug() << "chopped" << filename  << "to" << path << "Was trash, is" << typeName;
    }
    dbgUI << localFilePath() << "type:" << typeName;

    KisMainWindow *window = KisPart::instance()->currentMainwindow();
    KoUpdaterPtr updater;
    if (window && window->viewManager()) {
        updater = window->viewManager()->createUnthreadedUpdater(i18n("Opening document"));
        d->importExportManager->setUpdater(updater);
    }

    KisImportExportErrorCode status = d->importExportManager->importDocument(localFilePath(), typeName);
    if (!guard) return status.isOk();

    if (!status.isOk()) {
        if (window && window->viewManager()) {
            updater->cancel();
        }
        QString msg = status.errorMessage();
        KisUsageLogger::log(QString("Loading %1 failed: %2").arg(prettyPath(), msg));

        d->ioPresentation->presentLoadFailure(prettyPath(),
                                              status,
                                              errorMessage(),
                                              warningMessage(),
                                              fileBatchMode());
        return false;
    }
    else if (!warningMessage().isEmpty() && !fileBatchMode()) {
        d->ioPresentation->presentLoadWarning(prettyPath(),
                                              warningMessage(),
                                              fileBatchMode());
        setPath(QString());
    }

    setMimeTypeAfterLoading(typeName);
    d->syncDecorationsWrapperLayerState();
    d->ioPresentation->notifyLoadSucceeded();
    if (!guard) return true;

    undoStack()->clear();

    return true;
}

KisDocument::RecoveryAutoSaveStartResult KisDocument::startRecoveryAutoSave()
{
    const QString autoSaveFileName = autoSaveFilePath(this, localFilePath());

    d->recoveryAutoSaveState.beginSaveStart();
    const QPointer<KisDocument> guard(this);
    const auto fileDocument = d->gpuDocument;
    d->ioPresentation->notifyAutoSaveStarted(autoSaveFileName);
    if (!guard || d->gpuDocument != fileDocument) return RecoveryAutoSaveStartResult::Failed;
    KisUsageLogger::log(QString("Autosaving recovery checkpoint: %1").arg(autoSaveFileName));

    // Clear before clone preparation so a modification delivered by its
    // event processing is distinguishable from the revision being saved.
    // Every unsuccessful start restores the flag below.
    d->modificationState.markAutoSaveStarted();

    const KritaUtils::ExportFileJob job(autoSaveFileName, nativeFormatMimeType(),
                                      KritaUtils::SaveIsExporting | KritaUtils::SaveInAutosaveMode);
    const KritaUtils::BackgroudSavingStartResult result = fileDocument ?
        (isSaving() ? KritaUtils::BackgroudSavingStartResult::AnotherSavingInProgress :
         d->startGpuAutoSave(autoSaveFileName, Private::GpuAutoSaveMode::Recovery) == RecoveryAutoSaveStartResult::Started ?
             KritaUtils::BackgroudSavingStartResult::Success : KritaUtils::BackgroudSavingStartResult::Failure) :
        initiateSavingInBackground(i18n("Autosaving..."),
                                   this,
                                   SLOT(slotCompleteRecoveryAutoSaving(KritaUtils::ExportFileJob, KisImportExportErrorCode, QString, QString)),
                                   job,
                                   nullptr);
    if (!guard) return RecoveryAutoSaveStartResult::Failed;

    switch (result) {
    case KritaUtils::BackgroudSavingStartResult::Success: {
        const Krita::Document::RecoveryAutoSaveState::Completion completion =
            d->recoveryAutoSaveState.markSaveStartSucceeded();
        if (completion.available) {
            QTimer::singleShot(0, this, [this, completion] {
                finishRecoveryAutoSaveRequest(completion.filePath, completion.success);
            });
        }
        return RecoveryAutoSaveStartResult::Started;
    }
    case KritaUtils::BackgroudSavingStartResult::AnotherSavingInProgress:
        d->recoveryAutoSaveState.markSaveStartDidNotSucceed();
        d->modificationState.restoreAutoSaveRequirement();
        return RecoveryAutoSaveStartResult::AlreadySaving;
    case KritaUtils::BackgroudSavingStartResult::ImageLockFailure:
    case KritaUtils::BackgroudSavingStartResult::Failure:
    case KritaUtils::BackgroudSavingStartResult::Cancelled:
        d->recoveryAutoSaveState.markSaveStartDidNotSucceed();
        d->modificationState.restoreAutoSaveRequirement();
        setEmergencyAutoSaveInterval();
        return RecoveryAutoSaveStartResult::Failed;
    }

    d->recoveryAutoSaveState.markSaveStartDidNotSucceed();
    return RecoveryAutoSaveStartResult::Failed;
}

void KisDocument::finishRecoveryAutoSaveRequest(const QString &filePath, bool success)
{
    // Export initialization can report an error synchronously through the
    // normal completion signal. Defer our public completion until
    // requestRecoveryAutoSave() has returned a matching start result.
    const Krita::Document::RecoveryAutoSaveState::CompletionHandling handling =
        d->recoveryAutoSaveState.recordCompletion(filePath, success);
    if (handling != Krita::Document::RecoveryAutoSaveState::CompletionHandling::Ready) {
        return;
    }

    Q_EMIT sigRecoveryAutoSaveFinished(filePath, success);
}

KisDocument::RecoveryAutoSaveStartResult KisDocument::requestRecoveryAutoSave()
{
    if (!d->modificationState.isModified() || d->documentIsClosing) {
        return RecoveryAutoSaveStartResult::NoChanges;
    }

    if (d->recoveryAutoSaveState.hasPendingRequest()) {
        return RecoveryAutoSaveStartResult::AlreadySaving;
    }

    if (isSaving()) {
        d->recoveryAutoSaveState.beginRequest();
        QTimer::singleShot(50, this, &KisDocument::slotContinuePendingRecoveryAutoSave);
        return RecoveryAutoSaveStartResult::AlreadySaving;
    }

    if (!d->modificationState.hasChangesAfterAutoSave()) {
        return RecoveryAutoSaveStartResult::NoChanges;
    }

    d->recoveryAutoSaveState.beginRequest();

    const QPointer<KisDocument> guard(this);
    const RecoveryAutoSaveStartResult result = startRecoveryAutoSave();
    if (!guard) return result;
    if (result == RecoveryAutoSaveStartResult::Failed) {
        d->recoveryAutoSaveState.cancelRequest();
    } else if (result == RecoveryAutoSaveStartResult::AlreadySaving) {
        // initiateSavingInBackground() can observe a save that started while
        // it was cloning. Queue a safety pass in case that job completed
        // during the reentrant clone preparation.
        QTimer::singleShot(50, this, &KisDocument::slotContinuePendingRecoveryAutoSave);
    }

    return result;
}

void KisDocument::autoSaveOnPause()
{
    const QPointer<KisDocument> guard(this);
    d->runPendingGpuAutoSave();
    if (!guard) return;
    if (!d->modificationState.isModified() ||
        !d->modificationState.hasChangesAfterAutoSave()) {
        return;
    }

    const QString autoSaveFileName = autoSaveFilePath(this, localFilePath());

    if (d->gpuDocument) {
        d->startGpuAutoSave(autoSaveFileName, Private::GpuAutoSaveMode::Pause);
        return;
    }

    bool started = exportDocumentSync(autoSaveFileName, nativeFormatMimeType());

    if (started)
    {
        d->modificationState.markAutoSaveStarted();
        dbgAndroid << "autoSaveOnPause successful";
    }
    else
    {
        qWarning() << "Could not auto-save when paused";
    }
}

// shared between openFile and koMainWindow's "create new empty document" code
void KisDocument::setMimeTypeAfterLoading(const QString& mimeType)
{
    d->identity.setMimeType(mimeType.toLatin1());
}


bool KisDocument::loadNativeFormat(const QString & file_)
{
    return openPath(file_);
}

void KisDocument::setModified(bool mod)
{
    if (mod) {
        updateEditingTime(false);
    }

    /// 1) Ignore format-side modifications in the CPU autosave clone.
    /// 2) When closing a document, QUndoStack emits a lot of
    ///    modified signals, when clearing itself, so we should
    ///    ignore all of them.
    if ((d->autoSaveState.isExportingAutoSave() && !d->gpuDocument) || d->documentIsClosing)
        return;

    //dbgUI<<" url:" << url.path();
    //dbgUI<<" mod="<<mod<<" MParts mod="<<KisParts::ReadWritePart::isModified()<<" isModified="<<isModified();

    if (mod && !d->autoSaveTimer->isActive()) {
        // First change since last autosave -> start the autosave timer
        setNormalAutoSaveInterval();
    }
    if (!d->modificationState.setModified(mod)) {
        return;
    }

    if (mod) {
        documentInfo()->updateParameters(isModified());
    }

    Q_EMIT modified(mod);
}

void KisDocument::setRecovered(bool value)
{
    if (d->recoveryStatus.setRecovered(value)) {
        Q_EMIT sigRecoveredChanged(value);
    }
}

bool KisDocument::isRecovered() const
{
    return d->recoveryStatus.isRecovered();
}

void KisDocument::updateEditingTime(bool forceStoreElapsed)
{
    QDateTime now = QDateTime::currentDateTime();
    int firstModDelta = d->firstMod.secsTo(now);
    int lastModDelta = d->lastMod.secsTo(now);

    if (lastModDelta > 30) {
        d->docInfo->setAboutInfo("editing-time", QString::number(d->docInfo->aboutInfo("editing-time").toInt() + d->firstMod.secsTo(d->lastMod)));
        d->firstMod = now;
    } else if (firstModDelta > 60 || forceStoreElapsed) {
        d->docInfo->setAboutInfo("editing-time", QString::number(d->docInfo->aboutInfo("editing-time").toInt() + firstModDelta));
        d->firstMod = now;
    }

    d->lastMod = now;
}

QString KisDocument::prettyPath() const
{
    QString _url(path());
#ifdef Q_OS_WIN
    _url = QDir::toNativeSeparators(_url);
#endif
    return _url;
}

// Get caption from document info (title(), in about page)
QString KisDocument::caption() const
{
    QString c;
    const QString _url(QFileInfo(path()).fileName());

    // if URL is empty...it is probably an unsaved file
    if (_url.isEmpty()) {
        c = " [" + i18n("Not Saved") + "] ";
    } else {
        c = _url; // Fall back to document URL
    }

    return c;
}

QDomDocument KisDocument::createDomDocument(const QString& tagName, const QString& version) const
{
    return createDomDocument("krita", tagName, version);
}

//static
QDomDocument KisDocument::createDomDocument(const QString& appName, const QString& tagName, const QString& version)
{
    QDomImplementation impl;
    QString url = QString("http://www.calligra.org/DTD/%1-%2.dtd").arg(appName).arg(version);
    QDomDocumentType dtype = impl.createDocumentType(tagName,
                                                     QString("-//KDE//DTD %1 %2//EN").arg(appName).arg(version),
                                                     url);
    // The namespace URN doesn't need to include the version number.
    QString namespaceURN = QString("http://www.calligra.org/DTD/%1").arg(appName);
    QDomDocument doc = impl.createDocument(namespaceURN, tagName, dtype);
    doc.insertBefore(doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\""), doc.documentElement());
    return doc;
}

bool KisDocument::isNativeFormat(const QByteArray& mimeType) const
{
    if (mimeType == nativeFormatMimeType())
        return true;
    return extraNativeMimeTypes().contains(mimeType);
}

void KisDocument::setErrorMessage(const QString& errMsg)
{
    d->lastErrorMessage = errMsg;
}

QString KisDocument::errorMessage() const
{
    return d->lastErrorMessage;
}

void KisDocument::setWarningMessage(const QString& warningMsg)
{
    d->lastWarningMessage = warningMsg;
}

QString KisDocument::warningMessage() const
{
    return d->lastWarningMessage;
}


KoUnit KisDocument::unit() const
{
    return d->unit;
}

void KisDocument::setUnit(const KoUnit &unit)
{
    if (d->unit != unit) {
        d->unit = unit;
        Q_EMIT unitChanged(unit);
    }
}

KUndo2Stack *KisDocument::undoStack()
{
    return d->undoStack;
}

KisImportExportManager *KisDocument::importExportManager() const
{
    return d->importExportManager;
}

void KisDocument::slotUndoStackCleanChanged(bool)
{
    // GPU edits publish image changes; saving finalizes cleanliness with its fixed file version.
    if (d->gpuSaving) return;
    // Queued notifications may describe an earlier history position.
    setImageModified();
}

void KisDocument::slotConfigChanged()
{
    KisConfig cfg(true);

    if (d->undoStack->undoLimit() != cfg.undoStackLimit()) {
        if (!d->undoStack->isClean()) {
            d->undoStack->clear();
            // we set this because the document *has* changed, even though the
            // undo history was purged.
            setImageModifiedWithoutUndo();
        }
        d->undoStack->setUndoLimit(cfg.undoStackLimit());
    }
    d->undoStack->setUseCumulativeUndoRedo(cfg.useCumulativeUndoRedo());
    d->undoStack->setCumulativeUndoData(cfg.cumulativeUndoData());

    d->autoSaveDelay = cfg.autoSaveInterval();
    setNormalAutoSaveInterval();
}

void KisDocument::slotImageRootChanged()
{
    const auto owner = d->image ? d->image->gpuImage() : nullptr;
    if (owner && (!d->gpuDocument || d->gpuDocument->image() != owner)) {
        d->gpuDocument = std::make_shared<LibrePaint::Document::Gpu::PaintDocument>(owner);
        d->undoStack->setImage(d->image);
    }
    d->syncDecorationsWrapperLayerState();
}

void KisDocument::clearUndoHistory()
{
    d->undoStack->clear();
}

KisGridConfig KisDocument::gridConfig() const
{
    return d->gridConfig;
}

void KisDocument::setGridConfig(const KisGridConfig &config)
{
    if (d->gridConfig != config) {
        d->gridConfig = config;
        d->syncDecorationsWrapperLayerState();
        Q_EMIT sigGridConfigChanged(config);

        // Store last assigned value as future default...
        KisConfig cfg(false);
        cfg.setDefaultGridSpacing(config.spacing());
    }
}

QList<KoResourceLoadResult> KisDocument::linkedDocumentResources()
{
    QList<KoResourceLoadResult> result;
    if (!d->linkedResourceStorage) {
        return result;
    }

    Q_FOREACH(const QString &resourceType, KisResourceLoaderRegistry::instance()->resourceTypes()) {
        QSharedPointer<KisResourceStorage::ResourceIterator> iter = d->linkedResourceStorage->resources(resourceType);
        while (iter->hasNext()) {
            iter->next();

            QBuffer buf;
            buf.open(QBuffer::WriteOnly);
            bool exportSuccessful =
                d->linkedResourceStorage->exportResource(iter->url(), &buf);

            KoResourceSP resource = d->linkedResourceStorage->resource(iter->url());
            exportSuccessful &= bool(resource);

            const QString name = resource ? resource->name() : QString();
            const QString fileName = QFileInfo(iter->url()).fileName();
            const KoResourceSignature signature(resourceType,
                                                KoMD5Generator::generateHash(buf.data()),
                                                fileName, name);

            if (exportSuccessful) {
                result << KoEmbeddedResource(signature, buf.data());
            } else {
                result << signature;
            }
        }
    }

    return result;
}

void KisDocument::setPaletteList(const QList<KoColorSetSP > &paletteList, bool emitSignal)
{
    QList<KoColorSetSP> oldPaletteList;
    if (d->linkedResourceStorage) {
        QSharedPointer<KisResourceStorage::ResourceIterator> iter = d->linkedResourceStorage->resources(ResourceType::Palettes);
        while (iter->hasNext()) {
            iter->next();
            KoResourceSP resource = iter->resource();
            if (resource && resource->valid()) {
                oldPaletteList << resource.dynamicCast<KoColorSet>();
            }
        }
        if (oldPaletteList != paletteList) {
            KisResourceModel resourceModel(ResourceType::Palettes);
            Q_FOREACH(KoColorSetSP palette, oldPaletteList) {
                if (!paletteList.contains(palette)) {
                    resourceModel.setResourceInactive(resourceModel.indexForResource(palette));
                }
            }
            Q_FOREACH(KoColorSetSP palette, paletteList) {
                if (!oldPaletteList.contains(palette)) {
                    resourceModel.addResource(palette, d->linkedResourcesStorageID);
                }
                else {
                    palette->setStorageLocation(d->linkedResourcesStorageID);
                    resourceModel.updateResource(palette);
                }
            }
            if (emitSignal) {
                Q_EMIT sigPaletteListChanged(oldPaletteList, paletteList);
            }
        }
    }
}

StoryboardItemList KisDocument::getStoryboardItemList()
{
    return d->m_storyboardItemList;
}

void KisDocument::setStoryboardItemList(const StoryboardItemList &storyboardItemList, bool emitSignal)
{
    d->m_storyboardItemList = storyboardItemList;
    if (emitSignal) {
        Q_EMIT sigStoryboardItemListChanged();
    }
}

QVector<StoryboardComment> KisDocument::getStoryboardCommentsList()
{
    return d->m_storyboardCommentList;
}

void KisDocument::setStoryboardCommentList(const QVector<StoryboardComment> &storyboardCommentList, bool emitSignal)
{
    d->m_storyboardCommentList = storyboardCommentList;
    if (emitSignal) {
        Q_EMIT sigStoryboardCommentListChanged();
    }
}

QVector<QFileInfo> KisDocument::getAudioTracks() const {
    return d->audioTracks;
}

void KisDocument::setAudioTracks(QVector<QFileInfo> f)
{
    d->audioTracks = f;
    Q_EMIT sigAudioTracksChanged();
}

void KisDocument::setAudioVolume(qreal level)
{
    d->audioLevel = level;
    Q_EMIT sigAudioLevelChanged(level);
}

qreal KisDocument::getAudioLevel()
{
    return d->audioLevel;
}

const KisGuidesConfig& KisDocument::guidesConfig() const
{
    return d->guidesConfig;
}

void KisDocument::setGuidesConfig(const KisGuidesConfig &data)
{
    if (d->guidesConfig == data) return;

    d->guidesConfig = data;
    d->syncDecorationsWrapperLayerState();
    Q_EMIT sigGuidesConfigChanged(d->guidesConfig);
}


const KisMirrorAxisConfig& KisDocument::mirrorAxisConfig() const
{
    return d->mirrorAxisConfig;
}

void KisDocument::setMirrorAxisConfig(const KisMirrorAxisConfig &config)
{
    if (d->mirrorAxisConfig == config) {
        return;
    }

    d->mirrorAxisConfig = config;
    if (d->image) {
        d->image->setMirrorAxesCenter(KisAlgebra2D::absoluteToRelative(d->mirrorAxisConfig.axisPosition(),
                                                                       d->image->bounds()));
    }
    setModified(true);

    Q_EMIT sigMirrorAxisConfigChanged();
}

void KisDocument::resetPath() {
    setPath(QString());
    setLocalFilePath(QString());
}

KoDocumentInfoDlg *KisDocument::createDocumentInfoDialog(QWidget *parent, KoDocumentInfo *docInfo) const
{
    return new KoDocumentInfoDlg(parent, docInfo, localFilePath(), mimeType());
}

bool KisDocument::isReadWrite() const
{
    return d->readwrite;
}

QString KisDocument::path() const
{
    return d->identity.path();
}

bool KisDocument::closePath(bool promptToSave)
{
    if (promptToSave) {
        if ( isReadWrite() && isModified()) {
            Q_FOREACH (KisView *view, KisPart::instance()->views()) {
                if (view && view->document() == this) {
                    if (!view->queryClose()) {
                        return false;
                    }
                }
            }
        }
    }
    // Not modified => ok and delete temp file.
    d->identity.clearMimeType();

    // It always succeeds for a read-only part,
    // but the return value exists for reimplementations
    // (e.g. pressing cancel for a modified read-write part)
    return true;
}



void KisDocument::setPath(const QString &path)
{
    if (d->identity.setPath(path)) {
        Q_EMIT sigPathChanged(path);
    }
}

QString KisDocument::localFilePath() const
{
    return d->identity.localFilePath();
}


void KisDocument::setLocalFilePath( const QString &localFilePath )
{
    d->identity.setLocalFilePath(localFilePath);
}

bool KisDocument::openPathInternal(const QString &path)
{
    if ( path.isEmpty() ) {
        return false;
    }
    const auto previousIdentity = d->identity;
    QPointer<KisDocument> guard(this);

    if (d->identity.mimeTypeWasAutoDetected()) {
        d->identity.clearMimeType();
        d->identity.setMimeTypeWasAutoDetected(false);
    }

    QByteArray mimeType = d->identity.mimeType();

    if ( !closePath() ) {
        return false;
    }
    if (!guard) return false;

    d->identity.setMimeType(mimeType);
    setPath(path);
    if (!guard) return false;

    d->identity.setLocalFilePath(d->identity.path());

    bool ret = false;
    // set the mimeType only if it was not already set (for example, by the host application)
    if (d->identity.mimeType().isEmpty()) {
        // get the mimeType of the file
        // using findByUrl() to avoid another string -> url conversion
        QString mime = KisMimeDatabase::mimeTypeForFile(d->identity.path());
        d->identity.setMimeType(mime.toLocal8Bit());
        d->identity.setMimeTypeWasAutoDetected(true);
    }

    setPath(d->identity.path());
    ret = openFile();
    if (!guard) return ret;

    if (!ret) {
        const bool pathChanged = d->identity.path() != previousIdentity.path();
        d->identity = previousIdentity;
        if (pathChanged) Q_EMIT sigPathChanged(d->identity.path());
        if (!guard) return false;
    }

    if (ret) {
        Q_EMIT completed();
    }
    else {
        Q_EMIT canceled(QString());
    }
    return ret;
}

KisImportExportErrorCode KisDocument::importGpuKra(QIODevice &source)
{
    namespace Kra = LibrePaint::ImportExport::Kra;
    namespace Files = LibrePaint::Document::Gpu;
    const auto reject = [&](const QString &message,
                            KisImportExportErrorCode status = ImportExportCodes::FormatFeaturesUnsupported) {
        setErrorMessage(message);
        return status;
    };
    const auto formatStatus = [](Kra::Status code) {
        switch (code) {
        case Kra::Status::Success: return ImportExportCodes::OK;
        case Kra::Status::InvalidData: return ImportExportCodes::FileFormatIncorrect;
        case Kra::Status::UnsupportedData: return ImportExportCodes::FormatFeaturesUnsupported;
        case Kra::Status::CapacityExceeded: return ImportExportCodes::InsufficientMemory;
        case Kra::Status::IoFailure: return ImportExportCodes::ErrorWhileReading;
        }
        return ImportExportCodes::FileFormatIncorrect;
    };
    const auto loaded = Kra::RasterCodec::read(source);
    if (loaded.code != Kra::Status::Success) return reject(loaded.diagnostic, formatStatus(loaded.code));
    const auto metadata = Kra::RasterCodec::readImageMetadata(loaded.document.description, loaded.document.files);
    if (metadata.code != Kra::Status::Success) return reject(metadata.diagnostic, formatStatus(metadata.code));
    QDomDocument xml;
    xml.setContent(loaded.document.description);
    const auto imageElement = xml.documentElement().firstChildElement(QStringLiteral("IMAGE"));
    const auto unsupported = gpuKraEditingError(imageElement);
    if (!unsupported.isEmpty()) return reject(unsupported);
    StaticKraSettings timing;
    QString timingDiagnostic;
    for (const auto &bytes : {metadata.metadata.animation, metadata.metadata.animationMetadata}) {
        const auto status = readStaticKraSettings(bytes, timing, timingDiagnostic);
        if (!status.isOk()) return reject(timingDiagnostic, status);
    }
    QDomDocument info;
    QList<QPair<QString, QString>> infoUpdates;
    KoDocumentInfo checkedInfo;
    connect(&checkedInfo, &KoDocumentInfo::infoUpdated, &checkedInfo,
            [&](const QString &key, const QString &value) { infoUpdates.append({key, value}); });
    if (!metadata.metadata.documentInfo.isEmpty() &&
        (!info.setContent(metadata.metadata.documentInfo) || !checkedInfo.load(info)))
        return reject(i18n("Invalid KRA document information."), ImportExportCodes::FileFormatIncorrect);
    KisMirrorAxisConfig mirror = d->mirrorAxisConfig;
    if (!metadata.metadata.mirrorAxis.isEmpty()) {
        QDomDocument axes;
        if (!axes.setContent(metadata.metadata.mirrorAxis) || !mirror.loadFromXml(axes.documentElement()))
            return reject(i18n("Invalid KRA mirror axis settings."), ImportExportCodes::FileFormatIncorrect);
    }
    QList<KoColor> colors = colorHistoryColors();
    if (!metadata.metadata.colorHistory.isEmpty()) {
        colors.clear();
        QDomDocument history;
        history.setContent(metadata.metadata.colorHistory);
        for (auto color = history.documentElement().firstChildElement(); !color.isNull(); color = color.nextSiblingElement())
            colors.append(KoColor::fromXML(color, QStringLiteral("U16")));
    }
    QColor assistants = d->globalAssistantsColor;
    if (!metadata.metadata.assistantsGlobalColor.isEmpty()) {
        QDomDocument value;
        value.setContent(metadata.metadata.assistantsGlobalColor);
        assistants = KisDomUtils::qStringToQColor(value.documentElement().attribute(QStringLiteral("SimpleColorData")));
    }
    auto *registry = KoColorSpaceRegistry::instance();
    const auto profile = metadata.metadata.iccProfile.isEmpty() ? registry->rgb8()->profile() :
        registry->createColorProfileFromData(QStringLiteral("RGBA"), QStringLiteral("U8"), metadata.metadata.iccProfile);
    const auto cs = profile ? registry->colorSpace(QStringLiteral("RGBA"), QStringLiteral("U8"), profile) : nullptr;
    if (!cs) return reject(i18n("The KRA color profile cannot be loaded."), ImportExportCodes::FormatColorSpaceUnsupported);
    KisMetaData::Store rootMetadata;
    QMap<QUuid, KisMetaData::Store> layerMetadata;
    QStringList metadataWarnings;
    if (metadata.metadata.rootLayerXmp || (metadata.metadata.layerXmp && !metadata.metadata.layerXmp->isEmpty())) {
        const auto backend = KisMetadataBackendRegistry::instance()->get(QStringLiteral("xmp"));
        if (!backend || !backend->supportLoading()) return reject(i18n("The XMP metadata reader is unavailable."));
        const auto decode = [&](QByteArray packet, const QString &name, KisMetaData::Store &store) {
            QBuffer buffer(&packet);
            KisMetaData::Store decoded;
            if (backend->loadFrom(&decoded, &buffer)) store.copyFrom(&decoded);
            else metadataWarnings.append(i18n("Could not load metadata for layer %1.", name));
        };
        if (metadata.metadata.rootLayerXmp) decode(*metadata.metadata.rootLayerXmp, QStringLiteral("root"), rootMetadata);
        if (metadata.metadata.layerXmp) {
            for (const auto &layer : loaded.document.image.layers) {
                const auto it = metadata.metadata.layerXmp->constFind(layer.uuid);
                if (it != metadata.metadata.layerXmp->cend()) decode(it.value(), layer.name, layerMetadata[layer.uuid]);
            }
        }
    }
    QString diagnostic;
    const auto execution = KisPart::instance()->gpuExecution(diagnostic);
    if (!execution) return reject(diagnostic, ImportExportCodes::Failure);
    auto opened = Files::PaintDocument::openKra(execution, loaded.document);
    if (opened.code != Files::FileCode::Success) return reject(opened.diagnostic, gpuFileStatus(opened.code));
    const std::shared_ptr<Files::PaintDocument> fileDocument = std::move(opened.document);
    auto image = KisImage::fromGpuImage(fileDocument->image(), imageElement.attribute(QStringLiteral("name")), cs);
    KisNodeSP root = image->root();
    qobject_cast<KisLayer *>(root.data())->metaData()->copyFrom(&rootMetadata);
    KisLayerUtils::recursiveApplyNodes(root, [&](KisNodeSP node) {
        if (node == root) return;
        const auto it = layerMetadata.constFind(node->uuid());
        auto *layer = qobject_cast<KisLayer *>(node.data());
        if (layer && it != layerMetadata.cend()) layer->metaData()->copyFrom(&it.value());
    });
    image->setResolution(metadata.metadata.resolution.x() / 72, metadata.metadata.resolution.y() / 72);
    auto *animation = image->animationInterface();
    if (timing.framerate) animation->setFramerate(*timing.framerate);
    if (timing.range) animation->setDocumentRange(*timing.range);
    if (timing.sequencePath) animation->setExportSequenceFilePath(*timing.sequencePath);
    if (timing.sequenceName) animation->setExportSequenceBaseName(*timing.sequenceName);
    if (timing.initialFrame) animation->setExportInitialFrameNumber(*timing.initialFrame);
    if (timing.time && !animation->initializeStaticTime(*timing.time))
        return reject(i18n("The KRA requires animated frame regeneration."));
    for (const auto &annotation : metadata.metadata.annotations)
        image->addAnnotation(new KisAnnotation(annotation.type, annotation.description, annotation.bytes));
    if (!localFilePath().isEmpty()) {
        const auto adopted = fileDocument->adoptSavedFile(localFilePath(), fileDocument->image()->operations().snapshot());
        if (adopted.code != Files::FileCode::Success) return reject(adopted.diagnostic, gpuFileStatus(adopted.code));
    }
    const QPointer<KisDocument> guard(this);
    // Metadata observers receive the complete adopted image and file owner.
    if (!metadata.metadata.documentInfo.isEmpty()) {
        const QSignalBlocker metadataNotifications(d->docInfo);
        d->docInfo->load(info);
    }
    d->mirrorAxisConfig = mirror;
    d->globalAssistantsColor = assistants;
    if (!metadataWarnings.isEmpty()) setWarningMessage(metadataWarnings.join(QLatin1Char('\n')));
    setColorHistoryColors(colors);
    if (!guard) return ImportExportCodes::OK;
    setCurrentImageImpl(image, true, {}, fileDocument);
    for (const auto &update : infoUpdates) {
        if (!guard || d->image != image) break;
        Q_EMIT d->docInfo->infoUpdated(update.first, update.second);
    }
    return ImportExportCodes::OK;
}

bool KisDocument::newImage(const QString& name,
                           qint32 width, qint32 height,
                           const KoColorSpace* cs,
                           const KoColor &bgColor, KisConfig::BackgroundStyle bgStyle,
                           int numberOfLayers,
                           const QString &description, const double imageResolution)
{
    Q_ASSERT(cs);

    KisImageSP image;

    if (!cs) return false;
    if (width <= 0 || height <= 0) {
        setErrorMessage(i18n("Image dimensions must be positive."));
        return false;
    }

    KisCursorOverrideLock cursorLock(Qt::BusyCursor);

    const bool gpuSession = KisPart::instance()->gpuEngineEnabled();
    if (gpuSession) {
        namespace Gpu = LibrePaint::Image::Gpu;
        if (cs != KoColorSpaceRegistry::instance()->rgb8() ||
            (bgStyle != KisConfig::RASTER_LAYER && bgStyle != KisConfig::CANVAS_COLOR)) {
            setErrorMessage(i18n("The new painting engine currently requires the default 8-bit RGB color space and a raster or canvas background."));
            return false;
        }
        if (numberOfLayers < 1 || numberOfLayers > Gpu::Limits{}.layerCount) {
            setErrorMessage(i18n("The requested number of layers exceeds the new painting engine's capacity."));
            return false;
        }
        if (!qIsFinite(imageResolution) || imageResolution <= 0) {
            setErrorMessage(i18n("Image resolution must be positive and finite."));
            return false;
        }
        QString diagnostic;
        const auto execution = KisPart::instance()->gpuExecution(diagnostic);
        if (!execution) { setErrorMessage(diagnostic); return false; }
        Gpu::RasterImage raster;
        raster.size = {width, height};
        KoColor initialColor(bgColor);
        initialColor.convertTo(cs);
        const auto pixel = initialColor.data();
        const std::array<quint8, 4> rgba{pixel[2], pixel[1], pixel[0], pixel[3]};
        const bool backgroundLayer = bgStyle == KisConfig::RASTER_LAYER;
        if (!backgroundLayer) raster.projectionBackground = rgba;
        const bool autopin = KisConfig(true).autoPinLayersToTimeline();
        for (int i = 0; i < numberOfLayers; ++i) {
            Gpu::RasterLayer layer;
            layer.name = i == 0 ? i18nc("Name for the bottom-most layer in the layerstack", "Background") :
                                 QStringLiteral("%1 %2").arg(i18n("Paint Layer")).arg(i);
            layer.pinnedToTimeline = (i > 0 || backgroundLayer) && autopin;
            if (i == 0 && backgroundLayer) {
                layer.defaultPixel = {rgba[0], rgba[1], rgba[2], 255};
                layer.opacity = rgba[3];
                layer.userLocked = numberOfLayers > 1;
            }
            raster.layers.append(layer);
        }
        raster.activeLayer = numberOfLayers - 1;
        try {
            image = KisImage::fromGpuImage(Gpu::PaintImageSP::create(execution, raster), name, cs);
        } catch (const std::exception &error) {
            setErrorMessage(QString::fromUtf8(error.what()));
            return false;
        }
    } else {
        image = new KisImage(createUndoStore(), width, height, cs, name);
    }

    Q_CHECK_PTR(image);

    connect(image, SIGNAL(sigImageModified()), this, SLOT(setImageModified()), Qt::UniqueConnection);
    connect(image, SIGNAL(sigImageModifiedWithoutUndo()), this, SLOT(setImageModifiedWithoutUndo()), Qt::UniqueConnection);
    image->setResolution(imageResolution, imageResolution);

    if (!gpuSession) image->assignImageProfile(cs->profile());
    image->waitForDone();

    documentInfo()->setAboutInfo("title", name);
    documentInfo()->setAboutInfo("abstract", description);

    KisConfig cfg(false);
    cfg.defImageWidth(width);
    cfg.defImageHeight(height);
    cfg.defImageResolution(imageResolution);
    if (!cfg.useDefaultColorSpace())
    {
        cfg.defColorModel(image->colorSpace()->colorModelId().id());
        cfg.setDefaultColorDepth(image->colorSpace()->colorDepthId().id());
        cfg.defColorProfile(image->colorSpace()->profile()->name());
    }

    bool autopin = cfg.autoPinLayersToTimeline();

    if (!gpuSession) {
        KisLayerSP bgLayer;
        if (bgStyle == KisConfig::RASTER_LAYER || bgStyle == KisConfig::FILL_LAYER) {
            KoColor strippedAlpha = bgColor;
            strippedAlpha.setOpacity(OPACITY_OPAQUE_U8);

            if (bgStyle == KisConfig::RASTER_LAYER) {
                bgLayer = new KisPaintLayer(image.data(), i18nc("Name for the bottom-most layer in the layerstack", "Background"), OPACITY_OPAQUE_U8, cs);
                bgLayer->paintDevice()->setDefaultPixel(strippedAlpha);
                bgLayer->setPinnedToTimeline(autopin);
            } else if (bgStyle == KisConfig::FILL_LAYER) {
                KisFilterConfigurationSP filter_config = KisGeneratorRegistry::instance()->get("color")->defaultConfiguration(KisGlobalResourcesInterface::instance());
                filter_config->setProperty("color", strippedAlpha.toQColor());
                filter_config->createLocalResourcesSnapshot();
                bgLayer = new KisGeneratorLayer(image.data(), i18nc("Name of automatically created background color fill layer", "Background Fill"), filter_config, image->globalSelection());
            }

            bgLayer->setOpacity(bgColor.opacityU8());

            if (numberOfLayers > 1) {
                //Lock bg layer if others are present.
                bgLayer->setUserLocked(true);
            }
        }
        else { // KisConfig::CANVAS_COLOR (needs an unlocked starting layer).
            image->setDefaultProjectionColor(bgColor);
            bgLayer = new KisPaintLayer(image.data(), image->nextLayerName(), OPACITY_OPAQUE_U8, cs);
        }

        Q_CHECK_PTR(bgLayer);
        image->addNode(bgLayer.data(), image->rootLayer().data());
        bgLayer->setDirty(QRect(0, 0, width, height));
    }

    // reset mirror axis to default:
    d->mirrorAxisConfig.setAxisPosition(QRectF(image->bounds()).center());
    setCurrentImage(image);

    for(int i = 1; !gpuSession && i < numberOfLayers; ++i) {
        KisPaintLayerSP layer = new KisPaintLayer(image, image->nextLayerName(), OPACITY_OPAQUE_U8, cs);
        layer->setPinnedToTimeline(autopin);
        image->addNode(layer, image->root(), i);
        layer->setDirty(QRect(0, 0, width, height));
    }

    {
        KisMainWindow *window = KisPart::instance()->currentMainwindow();
        if (window) {
            /**
             * Preinitialize color history for new documents when possible
             */
            setColorHistoryColors(window->viewManager()->canvasResourceProvider()->colorHistoryColors());
        }
    }

    KisUsageLogger::log(
        QString("Created image \"%1\", %2 * %3 pixels, %4 dpi. Color model: %6 %5 (%7). Layers: %8")
            .arg(name, QString::number(width), QString::number(height),
                 QString::number(imageResolution * 72.0), image->colorSpace()->colorModelId().name(),
                 image->colorSpace()->colorDepthId().name(), image->colorSpace()->profile()->name(),
                 QString::number(numberOfLayers)));

    return true;
}

bool KisDocument::isSaving() const
{
    if (d->gpuSaving) return true;
    const bool result = d->savingMutex.tryLock();
    if (result) {
        d->savingMutex.unlock();
    }
    return !result;
}

void KisDocument::waitForSavingToComplete()
{
    const QPointer<KisDocument> guard(this);
    d->runPendingGpuAutoSave();
    if (!guard) return;
    if (isSaving()) {
        KisAsyncActionFeedback f(i18nc("progress dialog message when the user closes the document that is being saved", "Waiting for saving to complete..."), 0);
        f.waitForMutex(d->savingMutex);
    }
}

KoShapeControllerBase *KisDocument::shapeController() const
{
    return d->shapeController;
}

KoShapeLayer* KisDocument::shapeForNode(KisNodeSP layer) const
{
    return d->shapeController->shapeForNode(layer);
}

QList<KisPaintingAssistantSP> KisDocument::assistants() const
{
    return d->assistants;
}

void KisDocument::setAssistants(const QList<KisPaintingAssistantSP> &value)
{
    if (d->assistants != value) {
        d->assistants = value;
        d->syncDecorationsWrapperLayerState();
        Q_EMIT sigAssistantsChanged();
    }
}

KisReferenceImagesLayerSP KisDocument::referenceImagesLayer() const
{
    if (!d->image) return KisReferenceImagesLayerSP();

    KisReferenceImagesLayerSP referencesLayer =
            KisLayerUtils::findNodeByType<KisReferenceImagesLayer>(d->image->root());

    return referencesLayer;
}

void KisDocument::setReferenceImagesLayer(KisSharedPtr<KisReferenceImagesLayer> layer, bool updateImage)
{
    KisReferenceImagesLayerSP currentReferenceLayer = referenceImagesLayer();

    // updateImage=false inherently means we are not changing the
    // reference images layer, but just would like to update its signals.
    if (currentReferenceLayer == layer && updateImage) {
        return;
    }

    d->referenceLayerConnections.clear();

    if (updateImage) {
        if (currentReferenceLayer) {
            d->image->removeNode(currentReferenceLayer);
        }

        if (layer) {
            d->image->addNode(layer);
        }
    }

    currentReferenceLayer = layer;

    if (currentReferenceLayer) {
        d->referenceLayerConnections.addConnection(
                    currentReferenceLayer, SIGNAL(sigUpdateCanvas(QRectF)),
                    this, SIGNAL(sigReferenceImagesChanged()));
    }

    Q_EMIT sigReferenceImagesLayerChanged(layer);
    Q_EMIT sigReferenceImagesChanged();
}

void KisDocument::setPreActivatedNode(KisNodeSP activatedNode)
{
    if (activatedNode && d->image && !d->image->isSynchronizingGpuNodes() && activatedNode->image() == d->image.data()) {
        if (const auto owner = d->image->gpuImage()) {
            for (const auto &layer : owner->operations().snapshot()->layers) {
                if (layer.uuid != activatedNode->uuid()) continue;
                if (layer.id == owner->operations().activeLayer()) break;
                const auto result = owner->selectLayer(layer.id);
                KIS_SAFE_ASSERT_RECOVER_RETURN(result == LibrePaint::Image::Gpu::Result::Success);
                break;
            }
        }
    }
    d->preActivatedNode = activatedNode;
}

KisNodeSP KisDocument::preActivatedNode() const
{
    return d->preActivatedNode;
}

KisImageWSP KisDocument::image() const
{
    return d->image;
}

bool KisDocument::hasAnimation() const
{
    return d->image && d->image->animationInterface()->hasAnimation();
}

KisImageSP KisDocument::savingImage() const
{
    return d->savingImage;
}


void KisDocument::setCurrentImage(KisImageSP image, bool forceInitialUpdate, KisNodeSP preActivatedNode)
{
    const auto fileDocument = image && image->gpuImage() ?
        std::make_shared<LibrePaint::Document::Gpu::PaintDocument>(image->gpuImage()) : nullptr;
    setCurrentImageImpl(image, forceInitialUpdate, preActivatedNode, fileDocument);
}

void KisDocument::setCurrentImageImpl(KisImageSP image, bool forceInitialUpdate, KisNodeSP preActivatedNode,
                                     std::shared_ptr<LibrePaint::Document::Gpu::PaintDocument> fileDocument)
{
    const bool changedIdentity = image && image != d->image;
    const QPointer<KisDocument> guard(this);
    if (changedIdentity && d->image) {
        Q_EMIT sigCurrentImageAboutToChange();
        if (!guard) return;
    }
    d->gpuDocument = fileDocument;
    d->undoStack->setImage(nullptr);
    if (!guard) return;
    if (changedIdentity) d->undoStack->clear();
    if (!guard) return;
    if (d->image) {
        // Disconnect existing sig/slot connections
        d->image->setUndoStore(new KisDumbUndoStore());
        d->image->disconnect(this);
        d->shapeController->setImage(0);
        d->image = 0;
    }

    if (!image) return;

    if (d->linkedResourceStorage){
        d->linkedResourceStorage->setMetaData(KisResourceStorage::s_meta_name, image->objectName());
    }

    d->setImageAndInitIdleWatcher(image);
    d->image->setUndoStore(new KisDocumentUndoStore(d->undoStack));
    if (!preActivatedNode) {
        if (const auto owner = image->gpuImage()) {
            const auto selected = owner->operations().activeLayer();
            const auto layers = owner->operations().snapshot()->layers;
            QUuid identity;
            for (const auto &layer : layers) if (layer.id == selected) identity = layer.uuid;
            QList<KisNodeSP> nodes{image->root()};
            for (qsizetype i = 0; i < nodes.size() && !preActivatedNode; ++i)
                for (auto node = nodes[i]->firstChild(); node; node = node->nextSibling()) {
                    if (node->uuid() == identity) { preActivatedNode = node; break; }
                    nodes.append(node);
                }
            d->preActivatedNode = preActivatedNode;
        }
    }
    d->shapeController->setImage(image, preActivatedNode);
    d->image->setMirrorAxesCenter(KisAlgebra2D::absoluteToRelative(d->mirrorAxisConfig.axisPosition(), image->bounds()));
    setModified(false);
    d->undoStack->setImage(image);
    connect(d->image, SIGNAL(sigImageModified()), this, SLOT(setImageModified()), Qt::UniqueConnection);
    connect(d->image, SIGNAL(sigImageModifiedWithoutUndo()), this, SLOT(setImageModifiedWithoutUndo()), Qt::UniqueConnection);
    connect(d->image, SIGNAL(sigLayersChangedAsync()), this, SLOT(slotImageRootChanged()));

    if (forceInitialUpdate) {
        d->image->initialRefreshGraph();
    }
    image.clear();
    if (guard && changedIdentity) Q_EMIT sigCurrentImageChanged();
}

void KisDocument::hackPreliminarySetImage(KisImageSP image)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(!d->image);

    // we set image without connecting idle-watcher, because loading
    // hasn't been finished yet
    d->image = image;
    d->shapeController->setImage(image);
}

void KisDocument::setImageModified()
{
    if (const auto image = d->image ? d->image->gpuImage() : nullptr) {
        setModified(d->modificationState.imageModifiedWithoutUndo() || image->operations().isModified());
        return;
    }
    // we only set as modified if undo stack is not at clean state
    setModified(d->modificationState.imageModifiedWithoutUndo() || !d->undoStack->isClean());
}

void KisDocument::setImageModifiedWithoutUndo()
{
    d->modificationState.markImageModifiedWithoutUndo();
    setImageModified();
}


KisUndoStore* KisDocument::createUndoStore()
{
    return new KisDocumentUndoStore(d->undoStack);
}

bool KisDocument::isAutosaving() const
{
    return d->autoSaveState.isExportingAutoSave();
}

void KisDocument::setAssistantsGlobalColor(QColor color)
{
    d->globalAssistantsColor = color;
}

QColor KisDocument::assistantsGlobalColor()
{
    return d->globalAssistantsColor;
}

QRectF KisDocument::documentBounds() const
{
    QRectF bounds = d->image->bounds();

    KisReferenceImagesLayerSP referenceImagesLayer = this->referenceImagesLayer();

    if (referenceImagesLayer) {
        bounds |= referenceImagesLayer->boundingImageRect();
    }

    return bounds;
}

void KisDocument::setColorHistoryColors(const QList<KoColor> &colors)
{
    d->colorHistoryModel.setColorList(colors);
}

QList<KoColor> KisDocument::colorHistoryColors() const
{
    return d->colorHistoryModel.colorList();
}

KisUniqueColorSet* KisDocument::colorHistoryModel()
{
    return &d->colorHistoryModel;
}

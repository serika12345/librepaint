/*
 * SPDX-FileCopyrightText: 2014 Boudewijn Rempt <boud@valdyas.org>
 * SPDX-FileCopyrightText: 2022 L. E. Segovia <amy@amyspark.me>
 *
 * SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "document/kis_image_manager.h"

#include "canvas/KisReferenceImage.h"
#include "canvas/KisReferenceImagesDecoration.h"
#include "canvas/kis_canvas2.h"
#include "canvas/kis_canvas_resource_provider.h"
#include "document/KisDocument.h"
#include "document/kis_file_layer.h"
#include "kis_clipboard.h"
#include "kis_mimedata.h"
#include "kis_shape_controller.h"
#include "nodes/kis_node_manager.h"
#include "workspace/KisMainWindow.h"
#include "workspace/KisView.h"
#include "workspace/KisViewManager.h"
#include "widgets/kis_canvas_drop.h"

#include <KoToolManager.h>

#include <klocalizedstring.h>

#include <QCursor>
#include <QDropEvent>
#include <QFileInfo>
#include <QMessageBox>
#include <QScopedPointer>
#include <QTemporaryFile>
#include <QUrl>

#include <kis_group_layer.h>
#include <kis_image.h>
#include <kis_layer.h>
#include <kis_node.h>
#include <kis_paint_layer.h>

#include <algorithm>
#include <functional>

void KisImageManager::handleImageDrop(KisView *imageView,
                                      QDropEvent *event,
                                      const QPoint &imagePosition)
{
    KisImageWSP kisimage = imageView->image();
    Q_ASSERT(kisimage);

    const QRect imageBounds = kisimage->bounds();
    boost::optional<QPoint> forcedCenter;
    if (event->keyboardModifiers() & Qt::ShiftModifier
        && imageBounds.contains(imagePosition)) {
        forcedCenter = imagePosition;
    }

    if (event->mimeData()->hasFormat("application/x-krita-node-internal-pointer")) {
        KisShapeController *kritaShapeController =
                dynamic_cast<KisShapeController*>(imageView->document()->shapeController());

        bool copyNode = true;
        QList<KisNodeSP> nodes;

        if (forcedCenter) {
            nodes = KisMimeData::loadNodesFastAndRecenter(*forcedCenter, event->mimeData(), kisimage, kritaShapeController, copyNode);
        } else {
            nodes = KisMimeData::loadNodesFast(event->mimeData(), kisimage, kritaShapeController, copyNode);
        }

        Q_FOREACH (KisNodeSP node, nodes) {
            if (node) {
                if (!imageView->viewManager()->nodeManager()->activeLayer()) {
                    imageView->viewManager()->nodeManager()->addNodeUndoable(
                        node, kisimage->rootLayer(), 0);
                } else {
                    imageView->viewManager()->nodeManager()->addNodeUndoable(
                        node,
                        imageView->viewManager()->nodeManager()->activeLayer()->parent(),
                        imageView->viewManager()->nodeManager()->activeLayer());
                }
            }
        }
    } else if (event->mimeData()->hasImage() || event->mimeData()->hasUrls()) {
        const QMimeData *mData = event->mimeData();

        // Opening a window on wayland causes the clipboard to be cleared, so we need to cache all the data we may need beforehand
        QList<QUrl> urls =  mData->urls();
        const QImage qimage = KisClipboard::instance()->getImageWithFallback(mData, false);

        KisCanvasDrop dlgAction;

        const auto callPos = QCursor::pos();

        const KisCanvasDrop::Action action = dlgAction.dropAs(*mData, callPos);

        if (action == KisCanvasDrop::INSERT_AS_NEW_LAYER) {
            const QPair<bool, KisClipboard::PasteFormatBehaviour> source =
                KisClipboard::instance()->askUserForSourceWithData (qimage, urls);

            if (!source.first) {
                dbgUI << "Paste event cancelled";
                return;
            }

            if (source.second != KisClipboard::PASTE_FORMAT_CLIP) {
                const auto url = std::find_if(
                    urls.constBegin(),
                    urls.constEnd(),
                    [&](const QUrl &url) {
                        if (source.second
                            == KisClipboard::PASTE_FORMAT_DOWNLOAD) {
                            return !url.isLocalFile();
                        } else if (source.second
                                   == KisClipboard::PASTE_FORMAT_LOCAL) {
                            return url.isLocalFile();
                        } else {
                            return false;
                        }
                    });

                if (url != urls.constEnd()) {
                    QScopedPointer<QTemporaryFile> tmp(new QTemporaryFile());
                    tmp->setAutoRemove(true);

                    const QUrl localUrl = [&]() -> QUrl {
                        if (!url->isLocalFile()) {
                            // download the file and substitute the url
                            tmp->setFileName(url->fileName());

                            if (!KisImageManager::fetchRemoteFile(*url, tmp.data())) {
                                warnUI << "Fetching" << *url << "failed";
                                return {};
                            }
                            return QUrl::fromLocalFile(tmp->fileName());
                        }
                        return *url;
                    }();

                    if (localUrl.isLocalFile()) {
                        imageView->mainWindow()
                            ->viewManager()
                            ->imageManager()
                            ->importImage(localUrl);
                        imageView->activateWindow();
                        return;
                    }
                }
            }

            KisPaintDeviceSP clip = KisClipboard::instance()->clipFromBoardContentsWithData(qimage, urls, QRect(), true, -1, false, source);
            if (clip) {
                const auto pos = imageView->viewConverter()
                                     ->imageToDocument(imagePosition)
                                     .toPoint();

                clip->moveTo(pos.x(), pos.y());

                KisImageManager::adaptClipToImageColorSpace(clip, imageView->image());

                KisPaintLayerSP layer = new KisPaintLayer(
                    imageView->image(),
                    imageView->image()->nextLayerName() + " " + i18n("(pasted)"),
                    OPACITY_OPAQUE_U8,
                    clip);
                imageView->mainWindow()->viewManager()->nodeManager()->addNodeUndoable(
                    layer,
                    imageView->mainWindow()->viewManager()->activeNode()->parent(),
                    imageView->mainWindow()->viewManager()->activeNode());
                imageView->activateWindow();
                return;
            }
        } else if (action == KisCanvasDrop::INSERT_AS_REFERENCE_IMAGE) {

            KisPaintDeviceSP clip = KisClipboard::instance()-> clipFromBoardContentsWithData(qimage, urls, QRect(), true);
            if (clip) {
                KisImageManager::adaptClipToImageColorSpace(clip, imageView->image());

                auto *reference =
                    KisReferenceImage::fromPaintDevice(clip,
                                                       *imageView->viewConverter(),
                                                       imageView);

                if (reference) {
                    if (!urls.isEmpty()) {
                        const auto url = std::find_if(urls.constBegin(), urls.constEnd(), std::mem_fn(&QUrl::isLocalFile));
                        if (url != urls.constEnd()) {
                            reference->setFilename((*url).toLocalFile());
                        }
                    }
                    const auto pos = imageView->canvasBase()
                                         ->coordinatesConverter()
                                         ->widgetToImage(event->pos());
                    reference->setPosition(
                        (*imageView->viewConverter()).imageToDocument(pos));
                    imageView->canvasBase()
                        ->referenceImagesDecoration()
                        ->addReferenceImage(reference);
                    KoToolManager::instance()->switchToolRequested(
                        "ToolReferenceImages");
                    return;
                }
            }
        } else if (action != KisCanvasDrop::NONE) {
            // multiple URLs detected OR about to open a document

            for (QUrl url : urls) { // do copy it
                QScopedPointer<QTemporaryFile> tmp(new QTemporaryFile());
                tmp->setAutoRemove(true);

                if (!url.isLocalFile()) {
                    // download the file and substitute the url
                    tmp->setFileName(url.fileName());

                    if (!KisImageManager::fetchRemoteFile(url, tmp.data())) {
                        qWarning() << "Fetching" << url << "failed";
                        continue;
                    }
                    url = QUrl::fromLocalFile(tmp->fileName());
                }

                if (url.isLocalFile()) {
                    if (action == KisCanvasDrop::INSERT_MANY_LAYERS) {
                        imageView->mainWindow()
                            ->viewManager()
                            ->imageManager()
                            ->importImage(url);
                        imageView->activateWindow();
                    } else if (action == KisCanvasDrop::INSERT_MANY_FILE_LAYERS
                               || action
                                   == KisCanvasDrop::INSERT_AS_NEW_FILE_LAYER) {
                        QFileInfo fileInfo(url.toLocalFile());

                        const QString formatError =
                            KisImageManager::importFileFormatError(url.toLocalFile());

                        if (!formatError.isEmpty()) {
                            QMessageBox::warning(
                                imageView,
                                i18nc("@title:window", "LibrePaint"),
                                i18n("Could not open %2.\nReason: %1.",
                                     formatError,
                                     url.toDisplayString()));
                            continue;
                        }

                        KisFileLayer *fileLayer =
                            new KisFileLayer(imageView->image(),
                                             "",
                                             url.toLocalFile(),
                                             KisFileLayer::None,
                                             "Bicubic",
                                             fileInfo.fileName(),
                                             OPACITY_OPAQUE_U8);

                        KisLayerSP above =
                            imageView->mainWindow()->viewManager()->activeLayer();
                        KisNodeSP parent = above ? above->parent()
                                                 : imageView->mainWindow()
                                                       ->viewManager()
                                                       ->image()
                                                       ->root();

                        imageView->mainWindow()
                            ->viewManager()
                            ->nodeManager()
                            ->addNodeUndoable(fileLayer, parent, above);
                    } else if (action == KisCanvasDrop::OPEN_IN_NEW_DOCUMENT
                               || action
                                   == KisCanvasDrop::OPEN_MANY_DOCUMENTS) {
                        if (imageView->mainWindow()) {
                            imageView->mainWindow()->openDocument(
                                url.toLocalFile(),
                                KisMainWindow::None);
                        }
                    } else if (action
                                   == KisCanvasDrop::INSERT_AS_REFERENCE_IMAGES
                               || action
                                   == KisCanvasDrop::
                                       INSERT_AS_REFERENCE_IMAGE) {
                        auto *reference =
                            KisReferenceImage::fromFile(url.toLocalFile(),
                                                        *imageView->viewConverter(),
                                                        imageView);

                        if (reference) {
                            const auto pos = imageView->canvasBase()
                                                 ->coordinatesConverter()
                                                 ->widgetToImage(event->pos());
                            reference->setPosition(
                                (*imageView->viewConverter()).imageToDocument(pos));
                            imageView->canvasBase()
                                ->referenceImagesDecoration()
                                ->addReferenceImage(reference);

                            KoToolManager::instance()->switchToolRequested(
                                "ToolReferenceImages");
                        }
                    }
                }
            }
        }
    }
}

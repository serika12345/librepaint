/*
 *  SPDX-FileCopyrightText: 2011 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "ChannelDockerWidget.h"

#include <QGridLayout>
#include <QTableView>
#include <QHeaderView>

#include "KisKineticScroller.h"
#include "KisQStringListFwd.h"
#include "KisWidgetWithIdleTask.h"
#include "channelmodel.h"
#include <application/ui/workspace/KisViewManager.h>
#include <kis_canvas2.h>
#include <kis_image.h>
#include <kis_paint_device.h>
#include "KisChannelsThumbnailsStrokeStrategy.h"
#include <kis_display_color_converter.h>
#include <canvas/KisDisplayConfig.h>
#include <qabstractitemview.h>
#include <qobjectdefs.h>
#include <qscroller.h>
#include <qsize.h>
#include <gpu/KisGpuPaintImage.h>
#include <gpu/KisGpuExecution.h>
#include <KoChannelInfo.h>

ChannelDockerWidget::ChannelDockerWidget(QWidget *parent, const char *name)
    : KisWidgetWithIdleTask<QWidget>(parent), m_gpuUpdate(this)
{
    setObjectName(name);

    m_channelTable = new QTableView();
    m_model = new ChannelModel(this);
    m_channelTable->setModel(m_model);
    m_channelTable->setShowGrid(false);
    m_channelTable->horizontalHeader()->setStretchLastSection(true);
    m_channelTable->verticalHeader()->setVisible(false);
    m_channelTable->horizontalHeader()->setVisible(false);
    m_channelTable->setSelectionBehavior( QAbstractItemView::SelectRows );

    m_channelTable->setMinimumHeight(50);

    // Use a layout so the widget gets resized properly
    QGridLayout *layout = new QGridLayout();
    layout->addWidget(m_channelTable);
    this->setLayout(layout);

    QScroller *scroller = KisKineticScroller::createPreconfiguredScroller(m_channelTable);
    if (scroller){
        connect(scroller, SIGNAL(stateChanged(QScroller::State)),
                this, SLOT(slotScrollerStateChanged(QScroller::State)));
    }

    connect(m_channelTable,&QTableView::activated, m_model, &ChannelModel::rowActivated);
    m_gpuUpdate.setSingleShot(true);
    m_gpuUpdate.setInterval(500);
    connect(&m_gpuUpdate, &QTimer::timeout, this, &ChannelDockerWidget::updateGpuThumbnails);
}

void ChannelDockerWidget::setCanvas(KisCanvas2 * canvas)
{
    const bool gpu = canvas && canvas->image() && canvas->image()->gpuImage();
    if (canvas && !gpu && !m_gpuCanvas && m_canvas == canvas)
        return;
    m_canvasConnections.clear();
    m_gpuUpdate.stop();
    m_gpuCanvas = nullptr;
    KisWidgetWithIdleTask<QWidget>::setCanvas(gpu ? nullptr : canvas);
    m_gpuCanvas = gpu ? canvas : nullptr;
    m_model->setCanvas(canvas);

    if (canvas) {
        m_canvasConnections.addConnection(canvas->displayColorConverter(), SIGNAL(displayConfigurationChanged()), this, SLOT(startUpdateCanvasProjection()));
        m_canvasConnections.addConnection(m_model, SIGNAL(channelFlagsChanged()), canvas, SLOT(channelSelectionChanged()));
    }
    if (gpu) {
        m_model->setChannelThumbnails({}, canvas->image()->colorSpace());
        m_canvasConnections.addConnection(canvas, &QObject::destroyed, this, [this] { setCanvas(nullptr); });
        m_canvasConnections.addConnection(canvas, &KisCanvas2::sigCanvasEngineChanged, this, [this, canvas] { setCanvas(canvas); });
        m_canvasConnections.addConnection(canvas->image()->gpuImage().data(), &LibrePaint::Image::Gpu::PaintImage::changed,
                                         this, [this] { if (isVisible()) m_gpuUpdate.start(500); });
        if (isVisible()) m_gpuUpdate.start(0);
    }
}

void ChannelDockerWidget::startUpdateCanvasProjection()
{
    if (m_gpuCanvas) m_gpuUpdate.start(500);
    else triggerCacheUpdate();
}

void ChannelDockerWidget::showEvent(QShowEvent *event)
{
    KisWidgetWithIdleTask<QWidget>::showEvent(event);
    if (m_gpuCanvas) {
        m_model->setChannelThumbnails({}, m_gpuCanvas->image()->colorSpace());
        m_gpuUpdate.start(0);
    }
}

void ChannelDockerWidget::hideEvent(QHideEvent *event)
{
    m_gpuUpdate.stop();
    KisWidgetWithIdleTask<QWidget>::hideEvent(event);
}

void ChannelDockerWidget::updateGpuThumbnails()
{
    namespace Gpu = LibrePaint::Image::Gpu;
    if (!m_gpuCanvas || !isVisible()) return;
    const auto image = m_gpuCanvas->image();
    const auto owner = image ? image->gpuImage() : Gpu::PaintImageSP{};
    if (!owner) return;
    auto size = image->size();
    const auto limit = m_model->thumbnailSizeLimit();
    if (size.width() > limit.width() || size.height() > limit.height()) size.scale(limit, Qt::KeepAspectRatio);
    if (size.isEmpty()) return;
    const auto result = owner->execution().readThumbnail(owner->handle(), owner->operations().snapshot(), 0,
        image->bounds(), size, qsizetype(size.width()) * size.height() * 4);
    if (result.code != Gpu::ExecutionCode::Success) {
        qWarning() << "Could not update GPU channel thumbnails:" << result.diagnostic;
        return;
    }
    QVector<QImage> thumbnails;
    for (const auto *channel : image->colorSpace()->channels()) {
        QImage thumbnail(size, QImage::Format_Grayscale8);
        for (int y = 0; y < size.height(); ++y) {
            const auto *source = result.image.constScanLine(y);
            auto *destination = thumbnail.scanLine(y);
            for (int x = 0; x < size.width(); ++x) destination[x] = source[x * 4 + channel->displayPosition()];
        }
        thumbnails.append(thumbnail);
    }
    updateImageThumbnails(thumbnails, image->colorSpace());
}

KisIdleTasksManager::TaskGuard ChannelDockerWidget::registerIdleTask(KisCanvas2 *canvas)
{
    return
        canvas->viewManager()->idleTasksManager()->
        addIdleTaskWithGuard([this](KisImageSP image) {
            const KisDisplayConfig config = m_canvas->displayColorConverter()->displayConfig();

            const QSize previewSize = m_model->thumbnailSizeLimit();

            KisChannelsThumbnailsStrokeStrategy *strategy =
                new KisChannelsThumbnailsStrokeStrategy(image->projection(), image->bounds(),
                                                        previewSize, false, config.profile,
                                                        config.intent, config.conversionFlags);

            connect(strategy, SIGNAL(thumbnailsUpdated(QVector<QImage>, const KoColorSpace*)), this, SLOT(updateImageThumbnails(QVector<QImage>, const KoColorSpace*)));

            return strategy;
        });
}

void ChannelDockerWidget::updateImageThumbnails(const QVector<QImage> &channels, const KoColorSpace *cs)
{
    m_model->setChannelThumbnails(channels, cs);
    m_channelTable->resizeRowsToContents();
    m_channelTable->resizeColumnsToContents();
}

void ChannelDockerWidget::clearCachedState()
{
    m_model->setChannelThumbnails({}, nullptr);
}

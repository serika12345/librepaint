/*
 *  SPDX-FileCopyrightText: 2011 Sven Langkamp <sven.langkamp@gmail.com>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#ifndef CHANNELDOCKER_WIDGET_H
#define CHANNELDOCKER_WIDGET_H


#include <KisKineticScroller.h>

#include <KisChannelsThumbnailsStrokeStrategyMetatypes.h>
#include <canvas/KisWidgetWithIdleTask.h>
#include <QTableView>
#include <QTimer>
#include <kis_signal_auto_connection.h>
#include <kis_canvas2.h>

class ChannelModel;
class QTableView;

class ChannelDockerWidget : public KisWidgetWithIdleTask<QWidget> {
    Q_OBJECT
public:
    ChannelDockerWidget(QWidget *parent = 0, const char *name = 0);

    void setCanvas(KisCanvas2 *canvas) override;

public Q_SLOTS:
    void startUpdateCanvasProjection();
    void slotScrollerStateChanged(QScroller::State state){KisKineticScroller::updateCursor(this, state);}

private Q_SLOTS:
    void updateImageThumbnails(const QVector<QImage> &channels, const KoColorSpace *cs);
    void updateGpuThumbnails();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    KisIdleTasksManager::TaskGuard registerIdleTask(KisCanvas2 *canvas) override;
    void clearCachedState() override;

private:
    QTableView *m_channelTable {nullptr};
    ChannelModel *m_model {nullptr};
    QPointer<KisCanvas2> m_gpuCanvas;
    QTimer m_gpuUpdate;
    KisSignalAutoConnectionsStore m_canvasConnections;
};


#endif

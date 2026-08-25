/*
 *  SPDX-FileCopyrightText: 2015 Jouni Pentikäinen <joupent@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_ANIMATION_CACHE_POPULATOR_H
#define KIS_ANIMATION_CACHE_POPULATOR_H

#include <QObject>
#include "kis_types.h"
#include "kis_animation_frame_cache_fwd.h"

class KisPart;
class KisIdleWatcher;

class KisAnimationCachePopulator : public QObject
{
    Q_OBJECT

public:
    KisAnimationCachePopulator(KisPart *part);
    ~KisAnimationCachePopulator() override;

    /**
     * Request generation of given frame. The request will
     * be ignored if the populator is already requesting a frame.
     * @return true if generation requested, false if busy
     */
    bool regenerate(KisAnimationFrameCacheSP cache, int frame);
    void requestRegenerationWithPriorityFrame(KisImageSP image, int frameIndex);
    KisIdleWatcher *idleWatcher() const;
    void setTrackedImages(const QVector<KisImageSP> &images);
    void forceImageModified();

public Q_SLOTS:
    void slotRequestRegeneration();

private Q_SLOTS:
    void slotTimer();

    void slotRegeneratorFrameCancelled();
    void slotRegeneratorFrameReady();

    void slotConfigChanged();

private:
    struct Private;
    QScopedPointer<Private> m_d;
};

#endif

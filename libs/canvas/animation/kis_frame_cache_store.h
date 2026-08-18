/*
 *  SPDX-FileCopyrightText: 2018 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISFRAMECACHESTORE_H
#define KISFRAMECACHESTORE_H

#include <kritacanvas_export.h>
#include <QScopedPointer>
#include <QRect>

#include "kis_frame_data_serializer.h"

/**
 * Stores canvas animation frames on disk. Its responsibilities are:
 *
 * 1) Calculate differences between the frames and decide which
 *    frame will be a keyframe for other frames.
 *
 * 2) The keyframes will be used as a base for difference
 *    calculation and stored in a short in-memory cache to avoid
 *    fetching them from disk too often.
 *
 * 3) The in-memory cache of the keyframes is stored in serializable
 *    KisFrameDataSerializer::Frame format.
 */

class KRITACANVAS_EXPORT KisFrameCacheStore
{
public:
    struct Frame
    {
        Frame() = default;
        Frame(Frame &&) = default;
        Frame &operator=(Frame &&) = default;

        Frame(const Frame &) = delete;
        Frame &operator=(const Frame &) = delete;

        bool isValid() const { return data.isValid(); }

        KisFrameDataSerializer::Frame data;
        QRect dirtyImageRect;
        QRect imageBounds;
        int levelOfDetail {0};
    };

    KisFrameCacheStore();
    KisFrameCacheStore(const QString &frameCachePath);

    ~KisFrameCacheStore();

    void saveFrame(int frameId, Frame frame);
    Frame loadFrame(int frameId, KisTileDataPoolSP pool);

    void moveFrame(int srcFrameId, int dstFrameId);

    void forgetFrame(int frameId);
    bool hasFrame(int frameId) const;

    int frameLevelOfDetail(int frameId) const;
    QRect frameDirtyRect(int frameId) const;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KISFRAMECACHESTORE_H

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_ANIMATION_FRAME_CACHE_INDEX_H
#define KIS_ANIMATION_FRAME_CACHE_INDEX_H

#include <QList>
#include <QMap>
#include <QPair>
#include <QVector>

#include <kritacanvas_export.h>

class KisTimeSpan;

class KRITACANVAS_EXPORT KisAnimationFrameCacheIndex
{
public:
    struct StorageOperation
    {
        enum Type {
            Move,
            Forget,
        };

        Type type {Forget};
        int sourceFrameId {-1};
        int destinationFrameId {-1};
    };

    struct ChangeSet
    {
        bool changed {false};
        QVector<StorageOperation> storageOperations;
    };

    bool isEmpty() const;
    void clear();

    int frameIdAtTime(int time) const;
    bool contains(int time) const;
    bool shouldUploadNewFrame(int newTime, int oldTime) const;

    ChangeSet insert(const KisTimeSpan &range);
    ChangeSet invalidate(const KisTimeSpan &range);
    ChangeSet glue(const KisTimeSpan &range);

    QList<QPair<int, int>> rangesIntersecting(const KisTimeSpan &range) const;
    int frameLength(int frameId) const;
    bool removeFrame(int frameId);

private:
    QMap<int, int> m_frames;
};

#endif

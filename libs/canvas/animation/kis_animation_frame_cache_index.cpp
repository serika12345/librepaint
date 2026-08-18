/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kis_animation_frame_cache_index.h"

#include <QMap>

#include <kis_assert.h>
#include <kis_time_span.h>

namespace
{
bool rangeContainsTime(int start, int length, int time)
{
    return start <= time && (length == -1 || time < start + length);
}

bool rangeEndsBefore(int start, int length, int time)
{
    return length != -1 && start + length - 1 < time;
}
}

bool KisAnimationFrameCacheIndex::isEmpty() const
{
    return m_frames.isEmpty();
}

void KisAnimationFrameCacheIndex::clear()
{
    m_frames.clear();
}

int KisAnimationFrameCacheIndex::frameIdAtTime(int time) const
{
    if (m_frames.isEmpty()) {
        return -1;
    }

    auto it = m_frames.upperBound(time);
    if (it == m_frames.constBegin()) {
        return -1;
    }
    --it;

    return rangeContainsTime(it.key(), it.value(), time) ? it.key() : -1;
}

bool KisAnimationFrameCacheIndex::contains(int time) const
{
    return frameIdAtTime(time) >= 0;
}

bool KisAnimationFrameCacheIndex::shouldUploadNewFrame(int newTime, int oldTime) const
{
    if (oldTime < 0) {
        return true;
    }

    const int oldFrameId = frameIdAtTime(oldTime);
    return oldFrameId < 0 || !rangeContainsTime(oldFrameId, m_frames.value(oldFrameId), newTime);
}

KisAnimationFrameCacheIndex::ChangeSet KisAnimationFrameCacheIndex::insert(const KisTimeSpan &range)
{
    if (!range.isValid()) {
        return {};
    }

    ChangeSet result = invalidate(range);
    const int length = range.isInfinite() ? -1 : range.duration();
    m_frames.insert(range.start(), length);
    result.changed = true;
    return result;
}

KisAnimationFrameCacheIndex::ChangeSet KisAnimationFrameCacheIndex::invalidate(const KisTimeSpan &range)
{
    ChangeSet result;
    if (!range.isValid() || m_frames.isEmpty()) {
        return result;
    }

    auto it = m_frames.lowerBound(range.start());
    if (it != m_frames.begin() && (it == m_frames.end() || it.key() != range.start())) {
        --it;
    }

    while (it != m_frames.end()) {
        const int start = it.key();
        const int length = it.value();
        const bool isInfinite = length == -1;
        const int end = isInfinite ? -1 : start + length - 1;

        if (start >= range.start()) {
            if (!range.isInfinite() && start > range.end()) {
                break;
            }

            if (!range.isInfinite() && (isInfinite || end > range.end())) {
                const int newStart = range.end() + 1;
                const int newLength = isInfinite ? -1 : end - newStart + 1;

                m_frames.insert(newStart, newLength);
                result.storageOperations.append({StorageOperation::Move, start, newStart});
            } else {
                result.storageOperations.append({StorageOperation::Forget, start, -1});
            }

            it = m_frames.erase(it);
            result.changed = true;
            continue;
        }

        if (isInfinite || end >= range.start()) {
            it.value() = range.start() - start;
            result.changed = true;
        }

        ++it;
    }

    return result;
}

KisAnimationFrameCacheIndex::ChangeSet KisAnimationFrameCacheIndex::glue(const KisTimeSpan &range)
{
    ChangeSet result;
    if (!range.isValid() || m_frames.isEmpty()) {
        return result;
    }

    auto it = m_frames.begin();
    for (; it != m_frames.end(); ++it) {
        if (!rangeEndsBefore(it.key(), it.value(), range.start())) {
            break;
        }
    }

    if (it == m_frames.end()) {
        return result;
    }

    if (it.key() > range.start()) {
        const int oldStart = it.key();
        const int newStart = range.start();
        const int newLength = range.isInfinite() ? -1 : range.duration();

        it = m_frames.erase(it);
        it = m_frames.insert(newStart, newLength);
        result.storageOperations.append({StorageOperation::Move, oldStart, newStart});
        result.changed = true;
    }

    if (range.isInfinite()) {
        if (it.value() != -1) {
            it.value() = -1;
            result.changed = true;
        }
    } else if (it.value() != -1 && it.key() + it.value() - 1 < range.end()) {
        it.value() = range.end() - it.key() + 1;
        result.changed = true;
    }

    ++it;
    while (it != m_frames.end()) {
        const int start = it.key();
        const int length = it.value();

        if (range.isInfinite() || (length != -1 && start + length - 1 <= range.end())) {
            result.storageOperations.append({StorageOperation::Forget, start, -1});
            it = m_frames.erase(it);
            result.changed = true;
        } else if (start > range.end()) {
            break;
        } else {
            const int newStart = range.end() + 1;
            const int newLength = length == -1 ? -1 : start + length - newStart;

            m_frames.erase(it);
            m_frames.insert(newStart, newLength);
            result.storageOperations.append({StorageOperation::Move, start, newStart});
            result.changed = true;
            break;
        }
    }

    return result;
}

QList<QPair<int, int>> KisAnimationFrameCacheIndex::rangesIntersecting(const KisTimeSpan &range) const
{
    QList<QPair<int, int>> result;
    if (!range.isValid() || m_frames.isEmpty()) {
        return result;
    }

    auto it = m_frames.upperBound(range.start());
    if (it != m_frames.constBegin()) {
        --it;
    }

    while (it != m_frames.constEnd() && (range.isInfinite() || it.key() <= range.end())) {
        if (!rangeEndsBefore(it.key(), it.value(), range.start())) {
            result.append(qMakePair(it.key(), it.value()));
        }
        ++it;
    }

    return result;
}

int KisAnimationFrameCacheIndex::frameLength(int frameId) const
{
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(m_frames.contains(frameId), 0);
    return m_frames.value(frameId);
}

bool KisAnimationFrameCacheIndex::removeFrame(int frameId)
{
    return m_frames.remove(frameId) > 0;
}

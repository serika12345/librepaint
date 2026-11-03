/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KIS_GPU_BENCHMARK_FRAME_SCHEDULE_P_H
#define KIS_GPU_BENCHMARK_FRAME_SCHEDULE_P_H

#include <QtGlobal>
#include <algorithm>

namespace LibrePaint::Canvas::Gpu::Benchmark {
struct FrameAdvance
{
    qint64 nextIndex;
    qint64 elapsedDeadlines;
};

inline FrameAdvance advanceFrame(qint64 frameIndex, qint64 completedNs, int displayHz)
{
    const auto latestDue = completedNs * displayHz / 1000000000LL;
    return {std::max(frameIndex + 1, latestDue), std::max(qint64(0), latestDue - frameIndex)};
}
}

#endif

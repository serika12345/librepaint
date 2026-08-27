/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisAnimationRender.h"

namespace
{

bool looksLikeMp4(const QString &videoType)
{
#ifdef Q_OS_ANDROID
    return videoType.contains(QStringLiteral("mp4"));
#else
    return videoType == QStringLiteral("video/mp4");
#endif
}

bool looksLikeMatroska(const QString &videoType)
{
#ifdef Q_OS_ANDROID
    return videoType.contains(QStringLiteral("matroska"));
#else
    return videoType == QStringLiteral("video/x-matroska");
#endif
}

} // namespace

bool KisAnimationRender::mustHaveEvenDimensions(const QString &videoType,
                                                KisAnimationRenderingOptions::RenderMode renderMode)
{
    return renderMode != KisAnimationRenderingOptions::RENDER_FRAMES_ONLY
        && (looksLikeMp4(videoType) || looksLikeMatroska(videoType));
}

bool KisAnimationRender::hasEvenDimensions(int width, int height)
{
    return !((width & 0x1) || (height & 0x1));
}

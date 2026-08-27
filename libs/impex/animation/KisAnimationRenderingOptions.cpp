/*
 *  SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KisAnimationRenderingOptions.h"

#include <QFileInfo>
#include <QStandardPaths>

#include <KisFileUtils.h>

KisAnimationRenderingOptions::KisAnimationRenderingOptions()
{
}

#ifndef Q_OS_ANDROID
QString KisAnimationRenderingOptions::resolveAbsoluteDocumentFilePath(const QString &documentPath) const
{
    return !documentPath.isEmpty() ? documentPath : QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

QString KisAnimationRenderingOptions::resolveAbsoluteVideoFilePath(const QString &documentPath) const
{
    const QString basePath = resolveAbsoluteDocumentFilePath(documentPath);
    return KritaUtils::resolveAbsoluteFilePath(basePath, videoFileName);
}

QString KisAnimationRenderingOptions::resolveAbsoluteFramesDirectory(const QString &documentPath) const
{
    if (renderMode() == RENDER_VIDEO_ONLY) {
        return QFileInfo(resolveAbsoluteVideoFilePath()).absolutePath();
    }

    const QString basePath = resolveAbsoluteDocumentFilePath(documentPath);
    return KritaUtils::resolveAbsoluteFilePath(basePath, directory);
}

QString KisAnimationRenderingOptions::resolveAbsoluteVideoFilePath() const
{
    return resolveAbsoluteVideoFilePath(lastDocumentPath);
}

QString KisAnimationRenderingOptions::resolveAbsoluteFramesDirectory() const
{
    return resolveAbsoluteFramesDirectory(lastDocumentPath);
}
#endif

KisAnimationRenderingOptions::RenderMode KisAnimationRenderingOptions::renderMode() const
{
#ifdef Q_OS_ANDROID
    if (shouldEncodeVideo) {
        return RENDER_VIDEO_ONLY;
    } else {
        return RENDER_FRAMES_ONLY;
    }
#else
    if (shouldDeleteSequence) {
        KIS_SAFE_ASSERT_RECOVER_NOOP(shouldEncodeVideo);
        return RENDER_VIDEO_ONLY;
    } else if (!shouldEncodeVideo) {
        KIS_SAFE_ASSERT_RECOVER_NOOP(!shouldDeleteSequence);
        return RENDER_FRAMES_ONLY;
    } else {
        return RENDER_FRAMES_AND_VIDEO;
    }
#endif
}

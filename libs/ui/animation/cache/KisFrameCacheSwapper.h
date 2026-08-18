/*
 *  SPDX-FileCopyrightText: 2018 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISFRAMECACHESWAPPER_H
#define KISFRAMECACHESWAPPER_H

#include <QScopedPointer>

#include "KisAbstractFrameCacheSwapper.h"

class KisOpenGLUpdateInfoBuilder;
class QString;


/**
 * Adapts UI-owned OpenGL update data to the canvas-owned frame store.
 * Disk compression, frame differences, and stored metadata remain behind
 * the canvas boundary.
 */

class KRITAUI_EXPORT KisFrameCacheSwapper : public KisAbstractFrameCacheSwapper
{
public:
    KisFrameCacheSwapper(const KisOpenGLUpdateInfoBuilder &builder);
    KisFrameCacheSwapper(const KisOpenGLUpdateInfoBuilder &builder, const QString &frameCachePath);
    ~KisFrameCacheSwapper();

    // WARNING: after transferring \p info to saveFrame() the object becomes invalid
    void saveFrame(int frameId, KisOpenGLUpdateInfoSP info, const QRect &imageBounds) override;
    KisOpenGLUpdateInfoSP loadFrame(int frameId) override;

    void moveFrame(int srcFrameId, int dstFrameId) override;

    void forgetFrame(int frameId) override;
    bool hasFrame(int frameId) const override;

    int frameLevelOfDetail(int frameId) const override;

    QRect frameDirtyRect(int frameId) const override;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KISFRAMECACHESWAPPER_H

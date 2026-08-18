/*
 *  SPDX-FileCopyrightText: 2018 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "kis_frame_cache_store.h"

#include <QMap>
#include <QSharedPointer>

#include <kis_assert.h>
#include <kis_shared_ptr.h>

namespace {
enum FrameType {
    FrameFull,
    FrameDiff
};

struct FrameInfo;
typedef QSharedPointer<FrameInfo> FrameInfoSP;

struct FrameInfo {
    // full frame
    FrameInfo(const QRect &dirtyImageRect, const QRect &imageBounds, int levelOfDetail, KisFrameDataSerializer &serializer, const KisFrameDataSerializer::Frame &frame);
    // diff frame
    FrameInfo(const QRect &dirtyImageRect, const QRect &imageBounds, int levelOfDetail, KisFrameDataSerializer &serializer, FrameInfoSP baseFrame, const KisFrameDataSerializer::Frame &frame);
    ~FrameInfo();

    FrameType type() const {
        return m_type;
    }

    int levelOfDetail() const {
        return m_levelOfDetail;
    }

    QRect dirtyImageRect() const {
        return m_dirtyImageRect;
    }

    QRect imageBounds() const {
        return m_imageBounds;
    }

    int frameDataId() const {
        return m_savedFrameDataId;
    }

    FrameInfoSP baseFrame() const {
        return m_baseFrame;
    }

    int m_levelOfDetail = 0;
    QRect m_dirtyImageRect;
    QRect m_imageBounds;
    FrameInfoSP m_baseFrame;
    FrameType m_type = FrameFull;
    int m_savedFrameDataId = -1;
    KisFrameDataSerializer &m_serializer;
};

// full frame
FrameInfo::FrameInfo(const QRect &dirtyImageRect, const QRect &imageBounds, int levelOfDetail, KisFrameDataSerializer &serializer, const KisFrameDataSerializer::Frame &frame)
    : m_levelOfDetail(levelOfDetail),
      m_dirtyImageRect(dirtyImageRect),
      m_imageBounds(imageBounds),
      m_baseFrame(0),
      m_type(FrameFull),
      m_serializer(serializer)
{
    m_savedFrameDataId = m_serializer.saveFrame(frame);
}

// diff frame
FrameInfo::FrameInfo(const QRect &dirtyImageRect, const QRect &imageBounds, int levelOfDetail, KisFrameDataSerializer &serializer, FrameInfoSP baseFrame, const KisFrameDataSerializer::Frame &frame)
    : m_levelOfDetail(levelOfDetail),
      m_dirtyImageRect(dirtyImageRect),
      m_imageBounds(imageBounds),
      m_baseFrame(baseFrame),
      m_type(FrameDiff),
      m_serializer(serializer)
{
    m_savedFrameDataId = m_serializer.saveFrame(frame);
}

FrameInfo::~FrameInfo()
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(m_savedFrameDataId >= 0);
    m_serializer.forgetFrame(m_savedFrameDataId);
}

}


struct KRITACANVAS_NO_EXPORT KisFrameCacheStore::Private
{
    Private(const QString &frameCachePath)
        : serializer(frameCachePath)
    {
    }

    // the serializer should be killed after *all* the frame info objects
    // got destroyed, because they use it in their own destruction
    KisFrameDataSerializer serializer;

    KisFrameDataSerializer::Frame lastSavedFullFrame;
    int lastSavedFullFrameId = -1;

    KisFrameDataSerializer::Frame lastLoadedBaseFrame;
    FrameInfoSP lastLoadedBaseFrameInfo;

    QMap<int, FrameInfoSP> savedFrames;
};

KisFrameCacheStore::KisFrameCacheStore()
    : KisFrameCacheStore(QString())
{
}

KisFrameCacheStore::KisFrameCacheStore(const QString &frameCachePath)
    : m_d(new Private(frameCachePath))
{
}


KisFrameCacheStore::~KisFrameCacheStore()
{
}

void KisFrameCacheStore::saveFrame(int frameId, Frame frame)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(frame.isValid());

    FrameInfoSP frameInfo;

    if (m_d->lastSavedFullFrame.isValid()) {
        const boost::optional<qreal> uniqueness =
            KisFrameDataSerializer::estimateFrameUniqueness(
                m_d->lastSavedFullFrame, frame.data, 0.01);

        if (uniqueness && *uniqueness < 0.5) {
            FrameInfoSP baseFrameInfo = m_d->savedFrames.value(m_d->lastSavedFullFrameId);
            KIS_SAFE_ASSERT_RECOVER_RETURN(baseFrameInfo);

            KisFrameDataSerializer::subtractFrames(frame.data, m_d->lastSavedFullFrame);
            frameInfo = FrameInfoSP(new FrameInfo(frame.dirtyImageRect,
                                                  frame.imageBounds,
                                                  frame.levelOfDetail,
                                                  m_d->serializer,
                                                  baseFrameInfo,
                                                  frame.data));
        }
    }

    if (!frameInfo) {
        frameInfo = FrameInfoSP(new FrameInfo(frame.dirtyImageRect,
                                              frame.imageBounds,
                                              frame.levelOfDetail,
                                              m_d->serializer,
                                              frame.data));
    }

    m_d->savedFrames.insert(frameId, frameInfo);

    if (frameInfo->type() == FrameFull) {
        m_d->lastSavedFullFrame = std::move(frame.data);
        m_d->lastSavedFullFrameId = frameId;
    }
}

KisFrameCacheStore::Frame KisFrameCacheStore::loadFrame(int frameId, KisTileDataPoolSP pool)
{
    Frame result;
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(m_d->savedFrames.contains(frameId), result);

    FrameInfoSP frameInfo = m_d->savedFrames[frameId];
    result.dirtyImageRect = frameInfo->dirtyImageRect();
    result.imageBounds = frameInfo->imageBounds();
    result.levelOfDetail = frameInfo->levelOfDetail();

    switch (frameInfo->type()) {
    case FrameFull:
        result.data = m_d->serializer.loadFrame(frameInfo->frameDataId(), pool);
        m_d->lastLoadedBaseFrame = result.data.clone();
        m_d->lastLoadedBaseFrameInfo = frameInfo;
        break;
    case FrameDiff: {
        FrameInfoSP baseFrameInfo = frameInfo->baseFrame();
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(baseFrameInfo, Frame());

        if (baseFrameInfo != m_d->lastLoadedBaseFrameInfo) {
            m_d->lastLoadedBaseFrame = m_d->serializer.loadFrame(baseFrameInfo->frameDataId(), pool);
            m_d->lastLoadedBaseFrameInfo = baseFrameInfo;

            KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(m_d->lastLoadedBaseFrame.isValid(), Frame());
        }

        const KisFrameDataSerializer::Frame &baseFrame = m_d->lastLoadedBaseFrame;
        KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(baseFrame.isValid(), Frame());

        result.data = m_d->serializer.loadFrame(frameInfo->frameDataId(), pool);
        KisFrameDataSerializer::addFrames(result.data, baseFrame);
        break;
    }
    }

    return result;
}

void KisFrameCacheStore::moveFrame(int srcFrameId, int dstFrameId)
{
    KIS_SAFE_ASSERT_RECOVER_RETURN(srcFrameId != dstFrameId);

    KIS_SAFE_ASSERT_RECOVER_RETURN(m_d->savedFrames.contains(srcFrameId));

    KIS_SAFE_ASSERT_RECOVER(!m_d->savedFrames.contains(dstFrameId)) {
        m_d->savedFrames.remove(dstFrameId);
    }

    m_d->savedFrames.insert(dstFrameId, m_d->savedFrames[srcFrameId]);
    m_d->savedFrames.remove(srcFrameId);

    if (m_d->lastSavedFullFrameId == srcFrameId) {
        m_d->lastSavedFullFrameId = dstFrameId;
    }
}

void KisFrameCacheStore::forgetFrame(int frameId)
{
    KIS_SAFE_ASSERT_RECOVER_NOOP(m_d->savedFrames.contains(frameId));

    if (m_d->lastSavedFullFrameId == frameId) {
        m_d->lastSavedFullFrame = KisFrameDataSerializer::Frame();
        m_d->lastSavedFullFrameId = -1;
    }

    m_d->savedFrames.remove(frameId);
}

bool KisFrameCacheStore::hasFrame(int frameId) const
{
    return m_d->savedFrames.contains(frameId);
}

int KisFrameCacheStore::frameLevelOfDetail(int frameId) const
{
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(m_d->savedFrames.contains(frameId), 0);
    return m_d->savedFrames[frameId]->levelOfDetail();
}

QRect KisFrameCacheStore::frameDirtyRect(int frameId) const
{
    KIS_SAFE_ASSERT_RECOVER_RETURN_VALUE(m_d->savedFrames.contains(frameId), QRect());
    return m_d->savedFrames[frameId]->dirtyImageRect();
}

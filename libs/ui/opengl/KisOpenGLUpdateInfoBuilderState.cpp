/*
 *  SPDX-FileCopyrightText: 2018 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisOpenGLUpdateInfoBuilder.h"
#include "KisOpenGLUpdateInfoBuilderPrivate.inc"

#include <QReadLocker>
#include <QWriteLocker>

#include "kis_global.h"
#include "kis_lod_transform.h"

KisOpenGLUpdateInfoBuilder::KisOpenGLUpdateInfoBuilder()
    : m_d(new Private)
{
}

KisOpenGLUpdateInfoBuilder::~KisOpenGLUpdateInfoBuilder()
{
}

QRect KisOpenGLUpdateInfoBuilder::calculateEffectiveTileRect(int col, int row, const QRect &imageBounds) const
{
    return imageBounds
        & QRect(col * m_d->effectiveTextureSize.width(),
                row * m_d->effectiveTextureSize.height(),
                m_d->effectiveTextureSize.width(),
                m_d->effectiveTextureSize.height());
}

QRect KisOpenGLUpdateInfoBuilder::calculatePhysicalTileRect(int col,
                                                            int row,
                                                            const QRect &imageBounds,
                                                            int levelOfDetail) const
{
    const QRect tileRect = calculateEffectiveTileRect(col, row, imageBounds);
    const QRect tileTextureRect = kisGrowRect(tileRect, m_d->textureBorder);

    return levelOfDetail ? KisLodTransform::alignedRect(tileTextureRect, levelOfDetail) : tileTextureRect;
}

int KisOpenGLUpdateInfoBuilder::xToCol(int x) const
{
    return x / m_d->effectiveTextureSize.width();
}

int KisOpenGLUpdateInfoBuilder::yToRow(int y) const
{
    return y / m_d->effectiveTextureSize.height();
}

const KoColorSpace *KisOpenGLUpdateInfoBuilder::destinationColorSpace() const
{
    QReadLocker lock(&m_d->lock);

    return m_d->conversionOptions.m_destinationColorSpace;
}

void KisOpenGLUpdateInfoBuilder::setConversionOptions(const ConversionOptions &options)
{
    QWriteLocker lock(&m_d->lock);

    m_d->conversionOptions = options;
    // the proofing transform becomes invalid when the target colorspace changes
    m_d->proofingTransform.reset();
}

void KisOpenGLUpdateInfoBuilder::setChannelFlags(const QBitArray &channelFrags,
                                                 bool onlyOneChannelSelected,
                                                 int selectedChannelIndex)
{
    QWriteLocker lock(&m_d->lock);

    m_d->channelFlags = channelFrags;
    m_d->onlyOneChannelSelected = onlyOneChannelSelected;
    m_d->selectedChannelIndex = selectedChannelIndex;
}

void KisOpenGLUpdateInfoBuilder::setTextureBorder(int value)
{
    QWriteLocker lock(&m_d->lock);

    m_d->textureBorder = value;
}

void KisOpenGLUpdateInfoBuilder::setEffectiveTextureSize(const QSize &size)
{
    QWriteLocker lock(&m_d->lock);

    m_d->effectiveTextureSize = size;
}

void KisOpenGLUpdateInfoBuilder::setTileDataPool(KisTileDataPoolSP pool)
{
    QWriteLocker lock(&m_d->lock);

    m_d->pool = pool;
}

KisTileDataPoolSP KisOpenGLUpdateInfoBuilder::tileDataPool() const
{
    QReadLocker lock(&m_d->lock);
    return m_d->pool;
}

void KisOpenGLUpdateInfoBuilder::setProofingConfig(KisProofingConfigurationSP config)
{
    QWriteLocker lock(&m_d->lock);

    m_d->proofingConfig = config;
    m_d->proofingTransform.reset();
}

KisProofingConfigurationSP KisOpenGLUpdateInfoBuilder::proofingConfig() const
{
    QReadLocker lock(&m_d->lock);

    return m_d->proofingConfig;
}

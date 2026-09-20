/*
 *  SPDX-FileCopyrightText: 2018 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QReadWriteLock>

struct KRITAUI_NO_EXPORT KisOpenGLUpdateInfoBuilder::Private {
    ConversionOptions conversionOptions;

    QBitArray channelFlags;
    bool onlyOneChannelSelected = false;
    int selectedChannelIndex = -1;

    int textureBorder = 0;
    QSize effectiveTextureSize;

    KisProofingConfigurationSP proofingConfig;
    QScopedPointer<KoColorConversionTransformation> proofingTransform;

    KisTileDataPoolSP pool;
    QReadWriteLock lock;
};

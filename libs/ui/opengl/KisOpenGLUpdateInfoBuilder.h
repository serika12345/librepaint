/*
 *  SPDX-FileCopyrightText: 2018 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KISOPENGLUPDATEINFOBUILDER_H
#define KISOPENGLUPDATEINFOBUILDER_H

#include "kritaui_export.h"
#include "kis_opengl_update_info.h"
#include <QScopedPointer>
#include <QSharedPointer>

class KisProofingConfiguration;
typedef QSharedPointer<KisProofingConfiguration> KisProofingConfigurationSP;

class KisTileDataPool;
using KisTileDataPoolSP = QSharedPointer<KisTileDataPool>;

template<class T>
class KisSharedPtr;

class KisImage;
typedef KisSharedPtr<KisImage> KisImageSP;

class KisPaintDevice;
typedef KisSharedPtr<KisPaintDevice> KisPaintDeviceSP;

class KoColorSpace;


class KRITAUI_EXPORT KisOpenGLUpdateInfoBuilder
{
public:
    KisOpenGLUpdateInfoBuilder();
    ~KisOpenGLUpdateInfoBuilder();

    KisOpenGLUpdateInfoSP buildUpdateInfo(const QRect& rect, KisImageSP srcImage, bool convertColorSpace);
    KisOpenGLUpdateInfoSP buildUpdateInfo(const QRect& rect, KisPaintDeviceSP projection, const QRect &bounds, int levelOfDetail, bool convertColorSpace);

    QRect calculatePhysicalTileRect(int col, int row, const QRect &imageBounds, int levelOfDetail) const;
    QRect calculateEffectiveTileRect(int col, int row, const QRect &imageBounds) const;
    int xToCol(int x) const;
    int yToRow(int y) const;

    const KoColorSpace* destinationColorSpace() const;

    void setConversionOptions(const ConversionOptions &options);
    void setChannelFlags(const QBitArray &channelFrags, bool onlyOneChannelSelected, int selectedChannelIndex);

    void setTextureBorder(int value);
    void setEffectiveTextureSize(const QSize &size);

    void setTileDataPool(KisTileDataPoolSP pool);
    KisTileDataPoolSP tileDataPool() const;

    void setProofingConfig(KisProofingConfigurationSP config);
    KisProofingConfigurationSP proofingConfig() const;

private:
    struct Private;
    const QScopedPointer<Private> m_d;
};

#endif // KISOPENGLUPDATEINFOBUILDER_H

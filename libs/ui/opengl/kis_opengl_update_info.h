/*
 * SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_OPENGL_UPDATE_INFO_H
#define KIS_OPENGL_UPDATE_INFO_H

#include <KoColorConversionTransformation.h>

#include <kritaui_export.h>
#include <kis_update_info.h>

#include "kis_texture_tile_update_info.h"

class KoColorSpace;

struct ConversionOptions {
    ConversionOptions() = default;
    ConversionOptions(const KoColorSpace *destinationColorSpace,
                      KoColorConversionTransformation::Intent renderingIntent,
                      KoColorConversionTransformation::ConversionFlags conversionFlags)
        : m_needsConversion(true)
        , m_destinationColorSpace(destinationColorSpace)
        , m_renderingIntent(renderingIntent)
        , m_conversionFlags(conversionFlags)
    {
    }

    bool m_needsConversion {false};
    const KoColorSpace *m_destinationColorSpace {nullptr};
    KoColorConversionTransformation::Intent m_renderingIntent {KoColorConversionTransformation::IntentPerceptual};
    KoColorConversionTransformation::ConversionFlags m_conversionFlags {KoColorConversionTransformation::Empty};
};

class KisOpenGLUpdateInfo;
using KisOpenGLUpdateInfoSP = KisSharedPtr<KisOpenGLUpdateInfo>;

class KRITAUI_EXPORT KisOpenGLUpdateInfo : public KisUpdateInfo
{
public:
    KisOpenGLUpdateInfo();

    KisTextureTileUpdateInfoSPList tileList;

    QRect dirtyViewportRect() override;
    QRect dirtyImageRect() const override;
    void assignDirtyImageRect(const QRect &rect);
    void assignLevelOfDetail(int lod);
    int levelOfDetail() const override;
    bool tryMergeWith(const KisOpenGLUpdateInfo &rhs);

private:
    QRect m_dirtyImageRect;
    int m_levelOfDetail {0};
};

#endif

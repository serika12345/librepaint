/*
 *  SPDX-FileCopyrightText: 2010 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KIS_TEXTURE_TILE_H_
#define KIS_TEXTURE_TILE_H_

#include <QRect>
#include <QRectF>
#include <QHash>
#include <QSize>
// no forward-declaration, used to get GL* primitive types defined
#include <QOpenGLFunctions>

#include "kis_opengl.h"


class KisOpenGLBufferCircularStorage;
class KisTextureTileUpdateInfo;
class QOpenGLBuffer;

/**
 * Whether a full-tile update may write into the existing texture storage.
 *
 * glTexImage2D replaces the storage of a texture level even when the size and
 * format are unchanged. A full-tile update that keeps the level size can use
 * glTexSubImage2D instead. The decision is a free function so the contract
 * test can check it without an OpenGL context. The switch is for the L1
 * measurements only; KisL1Switches.h owns the removal condition.
 */
namespace KisTextureStorage
{
inline bool canReuseLevel(bool enabled, const QSize &allocated, const QSize &patch)
{
    return enabled && allocated.isValid() && allocated == patch;
}
}


struct KisGLTexturesInfo {

    // real width and height
    int width {0};
    int height {0};

    // width and height minus border padding?
    int effectiveWidth {1};
    int effectiveHeight {1};

    // size of the border padding
    int border {0};

    GLint internalFormat {0};
    GLint format {0};
    GLint type {0};
};

class KisTextureTile
{
public:
    KisTextureTile(const QRect &imageRect, const KisGLTexturesInfo *texturesInfo,
                   const QByteArray &fillData, KisOpenGL::FilterMode mode,
                   KisOpenGLBufferCircularStorage *bufferStorage, int numMipmapLevels, QOpenGLFunctions *f);
    ~KisTextureTile();

    void setBufferStorage(KisOpenGLBufferCircularStorage *bufferStorage) {
        m_bufferStorage = bufferStorage;
    }

    void setNumMipmapLevels(int num) {
        m_numMipmapLevels = num;
    }

    void update(const KisTextureTileUpdateInfo &updateInfo, bool blockMipmapRegeneration);

    inline QRect tileRectInImagePixels() {
        return m_tileRectInImagePixels;
    }

    inline QRect textureRectInImagePixels() {
        return m_textureRectInImagePixels;
    }

    inline QRectF tileRectInTexturePixels() {
        return m_tileRectInTexturePixels;
    }

    QRectF imageRectInTexturePixels(const QRect &imageRect) const;

    /**
     * Binds the tile's texture to the current GL_TEXTURE_2D binding point,
     * regenerates the mipmap if needed and returns the levelOfDetail that
     * should be used for painting
     */
    int bindToActiveTexture(bool blockMipmapRegeneration);

private:
    inline void setTextureParameters();
    inline void restoreTextureParameters();

    void setNeedsMipmapRegeneration();
    void setPreparedLodPlane(int lod);
    void regenerateMipmap();

    GLuint m_textureId;

    QRect m_tileRectInImagePixels;
    QRectF m_tileRectInTexturePixels;
    QRect m_textureRectInImagePixels;
    KisOpenGL::FilterMode m_filter;
    const KisGLTexturesInfo *m_texturesInfo;
    bool m_needsMipmapRegeneration;
    int m_preparedLodPlane;
    int m_numMipmapLevels;
    QOpenGLFunctions *f;
    KisOpenGLBufferCircularStorage *m_bufferStorage;
    bool m_mipmapHasBeenAllocated = false;
    QHash<int, QSize> m_levelSizes;
    Q_DISABLE_COPY(KisTextureTile)
};

#endif /* KIS_TEXTURE_TILE_H_ */


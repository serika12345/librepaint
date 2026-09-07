/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <opengl/kis_opengl_image_textures.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_OPENGL_TEXTURES_SIGNATURE(method, signature)                                                            \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&KisOpenGLImageTextures::method)), signature>)
} // namespace

class KisOpenGLImageTexturesSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void typeConstantsConstructionAndLifetimeSchemaRemainStable();
    void imageAndColorManagementSignaturesRemainStable();
    void openGLInitializationAndBufferSignaturesRemainStable();
    void tileGeometrySignaturesRemainStable();
    void cacheUpdateSignaturesRemainStable();
    void notificationSignatureRemainsStable();
};

void KisOpenGLImageTexturesSchemaContractTest::typeConstantsConstructionAndLifetimeSchemaRemainStable()
{
    using Textures = KisOpenGLImageTextures;

    static_assert(std::is_class_v<Textures>);
    static_assert(std::is_base_of_v<QObject, Textures>);
    static_assert(std::is_base_of_v<KisShared, Textures>);
    static_assert(std::is_default_constructible_v<Textures>);
    static_assert(std::has_virtual_destructor_v<Textures>);
    static_assert(Textures::BACKGROUND_TEXTURE_CHECK_SIZE == 32);
    static_assert(Textures::BACKGROUND_TEXTURE_SIZE == 64);
    ASSERT_OPENGL_TEXTURES_SIGNATURE(createImageTextures,
                                     KisOpenGLImageTexturesSP (*)(KisImageWSP,
                                                                  const KoColorProfile *,
                                                                  KoColorConversionTransformation::Intent,
                                                                  KoColorConversionTransformation::ConversionFlags));
}

void KisOpenGLImageTexturesSchemaContractTest::imageAndColorManagementSignaturesRemainStable()
{
    using Textures = KisOpenGLImageTextures;

    ASSERT_OPENGL_TEXTURES_SIGNATURE(image, KisImageSP (Textures::*)() const);
    ASSERT_OPENGL_TEXTURES_SIGNATURE(setMonitorProfile,
                                     void (Textures::*)(const KoColorProfile *,
                                                        KoColorConversionTransformation::Intent,
                                                        KoColorConversionTransformation::ConversionFlags));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(setImageColorSpace, bool (Textures::*)(const KoColorSpace *));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(setChannelFlags, void (Textures::*)(const QBitArray &));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(setProofingConfig, void (Textures::*)(KisProofingConfigurationSP));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(internalColorManagementActive, bool (Textures::*)() const);
    ASSERT_OPENGL_TEXTURES_SIGNATURE(setInternalColorManagementActive, bool (Textures::*)(bool, bool));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(monitorProfile, const KoColorProfile *(Textures::*)());
}

void KisOpenGLImageTexturesSchemaContractTest::openGLInitializationAndBufferSignaturesRemainStable()
{
    using Textures = KisOpenGLImageTextures;

    ASSERT_OPENGL_TEXTURES_SIGNATURE(initGL, void (Textures::*)(QOpenGLFunctions *));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(generateCheckerTexture, void (Textures::*)(const QImage &));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(checkerTexture, GLuint (Textures::*)());
    ASSERT_OPENGL_TEXTURES_SIGNATURE(updateConfig, void (Textures::*)(bool, int));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(testingForceInitialized, void (Textures::*)());
    ASSERT_OPENGL_TEXTURES_SIGNATURE(tileVertexBuffer, QOpenGLBuffer * (Textures::*)());
    ASSERT_OPENGL_TEXTURES_SIGNATURE(tileTexCoordBuffer, QOpenGLBuffer * (Textures::*)());
}

void KisOpenGLImageTexturesSchemaContractTest::tileGeometrySignaturesRemainStable()
{
    using Textures = KisOpenGLImageTextures;

    ASSERT_OPENGL_TEXTURES_SIGNATURE(storedImageBounds, QRect (Textures::*)());
    ASSERT_OPENGL_TEXTURES_SIGNATURE(xToCol, int (Textures::*)(int));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(yToRow, int (Textures::*)(int));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(getTextureTileCR, KisTextureTile * (Textures::*)(int, int));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(getTextureBufferIndexCR, int (Textures::*)(int, int));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(texelSize, qreal (Textures::*)() const);
}

void KisOpenGLImageTexturesSchemaContractTest::cacheUpdateSignaturesRemainStable()
{
    using Textures = KisOpenGLImageTextures;

    ASSERT_OPENGL_TEXTURES_SIGNATURE(updateCache, KisOpenGLUpdateInfoSP (Textures::*)(const QRect &, KisImageSP));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(updateCacheNoConversion, KisOpenGLUpdateInfoSP (Textures::*)(const QRect &));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(recalculateCache, void (Textures::*)(KisUpdateInfoSP, bool));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(slotImageSizeChanged, void (Textures::*)(qint32, qint32));
    ASSERT_OPENGL_TEXTURES_SIGNATURE(updateInfoBuilder, KisOpenGLUpdateInfoBuilder & (Textures::*)());
}

void KisOpenGLImageTexturesSchemaContractTest::notificationSignatureRemainsStable()
{
    ASSERT_OPENGL_TEXTURES_SIGNATURE(sigShowFloatingMessage,
                                     void (KisOpenGLImageTextures::*)(const QString &, int, bool));
}

#undef ASSERT_OPENGL_TEXTURES_SIGNATURE

QTEST_GUILESS_MAIN(KisOpenGLImageTexturesSchemaContractTest)

#include "KisOpenGLImageTexturesSchemaContractTest.moc"

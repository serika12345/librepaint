/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <opengl/KisOpenGLModeProber.h>

#include <color/KisSurfaceColorSpaceWrapper.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_OPENGL_MODE_SIGNATURE(type, method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)
} // namespace

class KisOpenGLModeProberSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void proberTypeConstructionAndStateSchemaRemainStable();
    void formatOperationSignaturesRemainStable();
    void resultTypeConstructionAndVersionSchemaRemainStable();
    void resultCapabilitySignaturesRemainStable();
    void resultDriverAndFormatInformationSignaturesRemainStable();
};

void KisOpenGLModeProberSchemaContractTest::proberTypeConstructionAndStateSchemaRemainStable()
{
    using Prober = KisOpenGLModeProber;

    static_assert(std::is_class_v<Prober>);
    static_assert(std::is_default_constructible_v<Prober>);
    static_assert(std::is_destructible_v<Prober>);
    ASSERT_OPENGL_MODE_SIGNATURE(Prober, instance, Prober * (*)());
    ASSERT_OPENGL_MODE_SIGNATURE(Prober, rootSurfaceColorProfile, const KoColorProfile *(Prober::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Prober, surfaceformatInUse, QSurfaceFormat (Prober::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Prober, useHDRMode, bool (Prober::*)() const);
}

void KisOpenGLModeProberSchemaContractTest::formatOperationSignaturesRemainStable()
{
    using Prober = KisOpenGLModeProber;
    using Result = Prober::Result;

    ASSERT_OPENGL_MODE_SIGNATURE(Prober, angleRendererToString, QString (*)(KisOpenGL::AngleRenderer));
    ASSERT_OPENGL_MODE_SIGNATURE(Prober,
                                 fuzzyCompareColorSpaces,
                                 bool (*)(const KisSurfaceColorSpaceWrapper &, const KisSurfaceColorSpaceWrapper &));
    ASSERT_OPENGL_MODE_SIGNATURE(Prober,
                                 initSurfaceFormatFromConfig,
                                 void (*)(std::pair<KisSurfaceColorSpaceWrapper, int>, QSurfaceFormat *));
    ASSERT_OPENGL_MODE_SIGNATURE(Prober, isFormatHDR, bool (*)(const QSurfaceFormat &));
    ASSERT_OPENGL_MODE_SIGNATURE(Prober,
                                 probeFormat,
                                 boost::optional<Result> (Prober::*)(const KisOpenGL::RendererConfig &, bool));
}

void KisOpenGLModeProberSchemaContractTest::resultTypeConstructionAndVersionSchemaRemainStable()
{
    using Result = KisOpenGLModeProber::Result;

    static_assert(std::is_class_v<Result>);
    static_assert(std::is_constructible_v<Result, QOpenGLContext &>);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, glMajorVersion, int (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, glMinorVersion, int (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, hasOpenGL3, bool (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, isOpenGLES, bool (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, isSupportedVersion, bool (Result::*)() const);
#ifdef Q_OS_WIN
    ASSERT_OPENGL_MODE_SIGNATURE(Result, isUsingAngle, bool (Result::*)() const);
#endif
}

void KisOpenGLModeProberSchemaContractTest::resultCapabilitySignaturesRemainStable()
{
    using Result = KisOpenGLModeProber::Result;

    ASSERT_OPENGL_MODE_SIGNATURE(Result, supportsBufferInvalidation, bool (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, supportsBufferMapping, bool (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, supportsDeprecatedFunctions, bool (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, supportsFBO, bool (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, supportsFenceSync, bool (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, supportsLoD, bool (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, supportsVAO, bool (Result::*)() const);
}

void KisOpenGLModeProberSchemaContractTest::resultDriverAndFormatInformationSignaturesRemainStable()
{
    using Result = KisOpenGLModeProber::Result;

    ASSERT_OPENGL_MODE_SIGNATURE(Result, driverVersionString, QString (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, extensions, QSet<QByteArray> (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, format, QSurfaceFormat (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, rendererString, QString (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, shadingLanguageString, QString (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result, vendorString, QString (Result::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Result,
                                 xcbGlProviderProtocol,
                                 std::optional<KisOpenGL::XcbGLProviderProtocol> (Result::*)());
}

#undef ASSERT_OPENGL_MODE_SIGNATURE

QTEST_GUILESS_MAIN(KisOpenGLModeProberSchemaContractTest)

#include "KisOpenGLModeProberSchemaContractTest.moc"

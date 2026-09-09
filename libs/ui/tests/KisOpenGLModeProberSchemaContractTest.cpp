/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <opengl/KisOpenGLModeProber.h>

#include <canvas/kis_display_filter.h>
#include <color/KisSurfaceColorSpaceWrapper.h>

#include <QTest>

#include <type_traits>

namespace
{
#define ASSERT_OPENGL_MODE_SIGNATURE(type, method, signature)                                                          \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

class DisplayFilterConstructionProbe final : public KisDisplayFilter
{
public:
    using KisDisplayFilter::KisDisplayFilter;

    QString program() const override;
    void setupTextures(GLFunctions *functions, QOpenGLShaderProgram *program) const override;
    void filter(quint8 *pixels, quint32 numPixels) override;
    void approximateInverseTransformation(quint8 *pixels, quint32 numPixels) override;
    void approximateForwardTransformation(quint8 *pixels, quint32 numPixels) override;
    bool useInternalColorManagement() const override;
    KisExposureGammaCorrectionInterface *correctionInterface() const override;
    bool lockCurrentColorVisualRepresentation() const override;
    bool updateShader() override;
};
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
    void displayFilterTypeAndConstructionSchemaRemainStable();
    void displayFilterShaderSignaturesRemainStable();
    void displayFilterPixelTransformationSignaturesRemainStable();
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

void KisOpenGLModeProberSchemaContractTest::displayFilterTypeAndConstructionSchemaRemainStable()
{
    using Filter = KisDisplayFilter;

    static_assert(std::is_class_v<Filter>);
    static_assert(std::is_base_of_v<QObject, Filter>);
    static_assert(std::is_base_of_v<KisDisplayColorFilter, Filter>);
    static_assert(std::is_abstract_v<Filter>);
    static_assert(std::is_default_constructible_v<DisplayFilterConstructionProbe>);
    static_assert(std::is_constructible_v<DisplayFilterConstructionProbe, QObject *>);
    static_assert(std::has_virtual_destructor_v<Filter>);
}

void KisOpenGLModeProberSchemaContractTest::displayFilterShaderSignaturesRemainStable()
{
    using Filter = KisDisplayFilter;

    ASSERT_OPENGL_MODE_SIGNATURE(Filter, program, QString (Filter::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Filter, setupTextures, void (Filter::*)(GLFunctions *, QOpenGLShaderProgram *) const);
    ASSERT_OPENGL_MODE_SIGNATURE(Filter, updateShader, bool (Filter::*)());
}

void KisOpenGLModeProberSchemaContractTest::displayFilterPixelTransformationSignaturesRemainStable()
{
    using Filter = KisDisplayFilter;

    ASSERT_OPENGL_MODE_SIGNATURE(Filter, filter, void (Filter::*)(quint8 *, quint32));
    ASSERT_OPENGL_MODE_SIGNATURE(Filter, approximateInverseTransformation, void (Filter::*)(quint8 *, quint32));
    ASSERT_OPENGL_MODE_SIGNATURE(Filter, approximateForwardTransformation, void (Filter::*)(quint8 *, quint32));
    ASSERT_OPENGL_MODE_SIGNATURE(Filter, useInternalColorManagement, bool (Filter::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Filter,
                                 correctionInterface,
                                 KisExposureGammaCorrectionInterface * (Filter::*)() const);
    ASSERT_OPENGL_MODE_SIGNATURE(Filter, lockCurrentColorVisualRepresentation, bool (Filter::*)() const);
}

#undef ASSERT_OPENGL_MODE_SIGNATURE

QTEST_GUILESS_MAIN(KisOpenGLModeProberSchemaContractTest)

#include "KisOpenGLModeProberSchemaContractTest.moc"

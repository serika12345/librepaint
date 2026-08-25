/*
 *  SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisScreenInformationAdapter.h"

#include <QOpenGLContext>

#include <QGuiApplication>
#include <QWindow>

#include <config-hdr.h>



#ifdef Q_OS_WIN
#include <qpa/qplatformnativeinterface.h>
#include <wrl/client.h>
#include <dxgi1_6.h>
#endif

namespace {
struct ScreenInformationException {
    ScreenInformationException() {}
    ScreenInformationException(const QString &what) : m_what(what) {}

    QString what() const {
        return m_what;
    }

private:
    QString m_what;
};

}


struct KisScreenInformationAdapter::Private
{
    void initialize(QOpenGLContext *context);

    QOpenGLContext *context;
    QString errorString;

#ifdef Q_OS_WIN
    Microsoft::WRL::ComPtr<IDXGIFactory1> dxgiFactory;
#endif
};

KisScreenInformationAdapter::KisScreenInformationAdapter(QOpenGLContext *context)
    : m_d(new Private)
{
    if (context) {
        m_d->initialize(context);
    }
}

KisScreenInformationAdapter::~KisScreenInformationAdapter()
{
}

void KisScreenInformationAdapter::Private::initialize(QOpenGLContext *newContext)
{
    context = newContext;
    errorString.clear();

    try {

#if defined Q_OS_WIN
        const HRESULT result = CreateDXGIFactory1(
            __uuidof(IDXGIFactory1),
            reinterpret_cast<void **>(this->dxgiFactory.GetAddressOf()));
        if (FAILED(result)) {
            throw ScreenInformationException(
                QString("couldn't create IDXGIFactory1, result = 0x%1").arg(result, 0, 16));
        }

#else
        throw ScreenInformationException("current platform doesn't support fetching display information");
#endif

    } catch (ScreenInformationException &e) {
        this->context = 0;
        this->errorString = e.what();
#ifdef Q_OS_WIN
        this->dxgiFactory.Reset();
#endif
    }
}

bool KisScreenInformationAdapter::isValid() const
{
#ifdef Q_OS_WIN
    return m_d->context && m_d->dxgiFactory;
#else
    return false;
#endif
}

QString KisScreenInformationAdapter::errorString() const
{
    return m_d->errorString;
}

KisScreenInformationAdapter::ScreenInfo KisScreenInformationAdapter::infoForScreen(QScreen *screen) const
{
    ScreenInfo info;

#if defined Q_OS_WIN

    QPlatformNativeInterface *nativeInterface = qGuiApp->platformNativeInterface();
    HMONITOR monitor = reinterpret_cast<HMONITOR>(nativeInterface->nativeResourceForScreen("handle", screen));

    UINT adapterIndex = 0;
    Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter;
    while (m_d->dxgiFactory->EnumAdapters1(adapterIndex, adapter.GetAddressOf()) == S_OK) {
        UINT outputIndex = 0;
        Microsoft::WRL::ComPtr<IDXGIOutput> currentOutput;
        while (adapter->EnumOutputs(outputIndex, currentOutput.GetAddressOf()) == S_OK) {
            HRESULT result = 0;
            Microsoft::WRL::ComPtr<IDXGIOutput6> output6;
            result = currentOutput.As(&output6);

            if (output6) {
                DXGI_OUTPUT_DESC1 desc;
                result = output6->GetDesc1(&desc);

                if (desc.Monitor == monitor) {
                    info.screen = screen;
                    info.bitsPerColor = desc.BitsPerColor;
                    info.redPrimary[0] = desc.RedPrimary[0];
                    info.redPrimary[1] = desc.RedPrimary[1];
                    info.greenPrimary[0] = desc.GreenPrimary[0];
                    info.greenPrimary[1] = desc.GreenPrimary[1];
                    info.bluePrimary[0] = desc.BluePrimary[0];
                    info.bluePrimary[1] = desc.BluePrimary[1];
                    info.whitePoint[0] = desc.WhitePoint[0];
                    info.whitePoint[1] = desc.WhitePoint[1];
                    info.minLuminance = desc.MinLuminance;
                    info.maxLuminance = desc.MaxLuminance;
                    info.maxFullFrameLuminance = desc.MaxFullFrameLuminance;

                    info.colorSpace = KisSurfaceColorSpaceWrapper::DefaultColorSpace;

                    if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709) {
                        info.colorSpace = KisSurfaceColorSpaceWrapper::sRGBColorSpace;
                    } else if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709) {
#ifdef HAVE_HDR
                        info.colorSpace = KisSurfaceColorSpaceWrapper::scRGBColorSpace;
#else
                        qWarning("WARNING: scRGB display color space is not supported by Qt's build");
#endif
                    } else if (desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020) {
#ifdef HAVE_HDR
                        info.colorSpace = KisSurfaceColorSpaceWrapper::bt2020PQColorSpace;
#else
                        qWarning("WARNING: bt2020-pq display color space is not supported by Qt's build");
#endif
                    } else {
                        qWarning("WARNING: unknown display color space! 0x%X", desc.ColorSpace);
                    }

                    return info;
                }
            }

            Q_UNUSED(result);
            currentOutput.Reset();
            outputIndex++;
        }

        adapter.Reset();
        adapterIndex++;
    }

#endif
    Q_UNUSED(screen);
    return info;
}

QDebug operator<<(QDebug dbg, const KisScreenInformationAdapter::ScreenInfo &info)
{
    QDebugStateSaver saver(dbg);

    if (info.isValid()) {
        dbg.nospace() << "ScreenInfo("
                      << "screen " << info.screen
                      << ", bitsPerColor " << info.bitsPerColor
                      << ", colorSpace " << info.colorSpace
                      << ", redPrimary " << "(" << info.redPrimary[0] << ", " << info.redPrimary[1] << ")"
                      << ", greenPrimary " << "(" << info.greenPrimary[0] << ", " << info.greenPrimary[1] << ")"
                      << ", bluePrimary " << "(" << info.bluePrimary[0] << ", " << info.bluePrimary[1] << ")"
                      << ", whitePoint " << "(" << info.whitePoint[0] << ", " << info.whitePoint[1] << ")"
                      << ", minLuminance " << info.minLuminance
                      << ", maxLuminance " << info.maxLuminance
                      << ", maxFullFrameLuminance " << info.maxFullFrameLuminance
                      << ')';
    } else {
        dbg.nospace() << "ScreenInfo(<invalid>)";
    }

    return dbg;
}

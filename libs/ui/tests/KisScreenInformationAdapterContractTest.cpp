/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "opengl/KisScreenInformationAdapter.h"


#include <QDebug>
#include <QOpenGLContext>
#include <QTest>

namespace
{


} // namespace

class KisScreenInformationAdapterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void screenInformationIdentityDefaultsRemainStable();
    void screenInformationChromaticityFieldsRemainIndependent();
    void screenInformationLuminanceAndValidityRemainStable();
    void screenInformationAdapterDiagnosticsAndFormattingRemainStable();
};

void KisScreenInformationAdapterContractTest::screenInformationIdentityDefaultsRemainStable()
{
    using Info = KisScreenInformationAdapter::ScreenInfo;


    Info info;
    QCOMPARE(info.screen, nullptr);
    QCOMPARE(info.bitsPerColor, 0);
    QVERIFY(info.colorSpace == KisSurfaceColorSpaceWrapper());
}

void KisScreenInformationAdapterContractTest::screenInformationChromaticityFieldsRemainIndependent()
{
    using Info = KisScreenInformationAdapter::ScreenInfo;


    Info info;
    QCOMPARE(info.redPrimary[0], 0.0);
    QCOMPARE(info.redPrimary[1], 0.0);
    QCOMPARE(info.greenPrimary[0], 0.0);
    QCOMPARE(info.greenPrimary[1], 0.0);
    QCOMPARE(info.bluePrimary[0], 0.0);
    QCOMPARE(info.bluePrimary[1], 0.0);
    QCOMPARE(info.whitePoint[0], 0.0);
    QCOMPARE(info.whitePoint[1], 0.0);

    info.redPrimary[0] = 0.64;
    info.redPrimary[1] = 0.33;
    info.greenPrimary[0] = 0.30;
    info.greenPrimary[1] = 0.60;
    info.bluePrimary[0] = 0.15;
    info.bluePrimary[1] = 0.06;
    info.whitePoint[0] = 0.3127;
    info.whitePoint[1] = 0.3290;

    QCOMPARE(info.redPrimary[0], 0.64);
    QCOMPARE(info.greenPrimary[1], 0.60);
    QCOMPARE(info.bluePrimary[0], 0.15);
    QCOMPARE(info.whitePoint[1], 0.3290);
}

void KisScreenInformationAdapterContractTest::screenInformationLuminanceAndValidityRemainStable()
{
    using Info = KisScreenInformationAdapter::ScreenInfo;


    Info info;
    QCOMPARE(info.minLuminance, 0.0);
    QCOMPARE(info.maxLuminance, 0.0);
    QCOMPARE(info.maxFullFrameLuminance, 0.0);
    QVERIFY(!info.isValid());

    info.minLuminance = 0.05;
    info.maxLuminance = 1000.0;
    info.maxFullFrameLuminance = 400.0;
    info.screen = reinterpret_cast<QScreen *>(quintptr(1));

    QCOMPARE(info.minLuminance, 0.05);
    QCOMPARE(info.maxLuminance, 1000.0);
    QCOMPARE(info.maxFullFrameLuminance, 400.0);
    QVERIFY(info.isValid());
}

void KisScreenInformationAdapterContractTest::screenInformationAdapterDiagnosticsAndFormattingRemainStable()
{
    using Adapter = KisScreenInformationAdapter;
    using Info = Adapter::ScreenInfo;


#ifndef Q_OS_WIN
    QOpenGLContext context;
    Adapter adapter(&context);
    QVERIFY(!adapter.isValid());
    QCOMPARE(adapter.errorString(), QStringLiteral("current platform doesn't support fetching display information"));
    QVERIFY(!adapter.infoForScreen(nullptr).isValid());
#endif

    QString debugText;
    {
        QDebug debug(&debugText);
        debug << Info();
    }
    QCOMPARE(debugText.trimmed(), QStringLiteral("ScreenInfo(<invalid>)"));
}

QTEST_APPLESS_MAIN(KisScreenInformationAdapterContractTest)

#include "KisScreenInformationAdapterContractTest.moc"

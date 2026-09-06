/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "opengl/KisScreenInformationAdapter.h"

#include <type_traits>

#include <QDebug>
#include <QOpenGLContext>
#include <QTest>

namespace
{

#define ASSERT_SCREEN_INFORMATION_MEMBER(type, method, signature)                                                      \
    static_assert(std::is_same_v<decltype(static_cast<signature>(&type::method)), signature>)

} // namespace

class KisScreenInformationAdapterContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void screenInformationAdapterTypeConstructionAndLifetimeRemainStable();
    void screenInformationIdentityDefaultsRemainStable();
    void screenInformationChromaticityFieldsRemainIndependent();
    void screenInformationLuminanceAndValidityRemainStable();
    void screenInformationAdapterDiagnosticsAndFormattingRemainStable();
};

void KisScreenInformationAdapterContractTest::screenInformationAdapterTypeConstructionAndLifetimeRemainStable()
{
    using Adapter = KisScreenInformationAdapter;

    static_assert(std::is_class_v<Adapter>);
    static_assert(std::is_constructible_v<Adapter, QOpenGLContext *>);
    static_assert(std::is_destructible_v<Adapter>);
    static_assert(!std::is_copy_constructible_v<Adapter>);

    QVERIFY(true);
}

void KisScreenInformationAdapterContractTest::screenInformationIdentityDefaultsRemainStable()
{
    using Info = KisScreenInformationAdapter::ScreenInfo;

    static_assert(std::is_class_v<Info>);
    static_assert(std::is_same_v<decltype(&Info::screen), QScreen * Info::*>);
    static_assert(std::is_same_v<decltype(&Info::bitsPerColor), int Info::*>);
    static_assert(std::is_same_v<decltype(&Info::colorSpace), KisSurfaceColorSpaceWrapper Info::*>);

    Info info;
    QCOMPARE(info.screen, nullptr);
    QCOMPARE(info.bitsPerColor, 0);
    QVERIFY(info.colorSpace == KisSurfaceColorSpaceWrapper());
}

void KisScreenInformationAdapterContractTest::screenInformationChromaticityFieldsRemainIndependent()
{
    using Info = KisScreenInformationAdapter::ScreenInfo;

    static_assert(std::is_same_v<decltype(&Info::redPrimary), qreal(Info::*)[2]>);
    static_assert(std::is_same_v<decltype(&Info::greenPrimary), qreal(Info::*)[2]>);
    static_assert(std::is_same_v<decltype(&Info::bluePrimary), qreal(Info::*)[2]>);
    static_assert(std::is_same_v<decltype(&Info::whitePoint), qreal(Info::*)[2]>);

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

    static_assert(std::is_same_v<decltype(&Info::minLuminance), qreal Info::*>);
    static_assert(std::is_same_v<decltype(&Info::maxLuminance), qreal Info::*>);
    static_assert(std::is_same_v<decltype(&Info::maxFullFrameLuminance), qreal Info::*>);
    ASSERT_SCREEN_INFORMATION_MEMBER(Info, isValid, bool (Info::*)() const);

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

    ASSERT_SCREEN_INFORMATION_MEMBER(Adapter, isValid, bool (Adapter::*)() const);
    ASSERT_SCREEN_INFORMATION_MEMBER(Adapter, errorString, QString (Adapter::*)() const);
    ASSERT_SCREEN_INFORMATION_MEMBER(Adapter, infoForScreen, Info (Adapter::*)(QScreen *) const);
    static_assert(std::is_same_v<decltype(static_cast<QDebug (*)(QDebug, const Info &)>(&operator<<)),
                                 QDebug (*)(QDebug, const Info &)>);

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

#undef ASSERT_SCREEN_INFORMATION_MEMBER

QTEST_APPLESS_MAIN(KisScreenInformationAdapterContractTest)

#include "KisScreenInformationAdapterContractTest.moc"

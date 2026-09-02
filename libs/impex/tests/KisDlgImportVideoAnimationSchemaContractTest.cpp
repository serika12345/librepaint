/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "KisDlgImportVideoAnimation.h"

#include <QTest>

#include <type_traits>

class KisDlgImportVideoAnimationSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void basicVideoStreamGeometryValueSchemaRemainsStable();
    void basicVideoEncodingAndColorValueSchemaRemainsStable();
    void renderedFramesValueSchemaRemainsStable();
    void videoImportDialogIdentityAndLifecycleSignatureRemainsStable();
    void videoImportDialogPublicWorkflowSignaturesRemainStable();
};

void KisDlgImportVideoAnimationSchemaContractTest::basicVideoStreamGeometryValueSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::file), QString>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::stream), int>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::width), int>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::height), int>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::fps), float>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::frames), int>);

    KisBasicVideoInfo original;
    QVERIFY(original.file.isEmpty());
    QCOMPARE(original.stream, -1);
    QCOMPARE(original.width, 0);
    QCOMPARE(original.height, 0);
    QCOMPARE(original.fps, 0.0F);
    QCOMPARE(original.frames, 0);

    original.file = QStringLiteral("source.mov");
    original.stream = 2;
    original.width = 1920;
    original.height = 1080;
    original.fps = 24.0F;
    original.frames = 48;
    KisBasicVideoInfo copy = original;
    copy.file = QStringLiteral("copy.mov");
    copy.width = 1280;
    QCOMPARE(original.file, QStringLiteral("source.mov"));
    QCOMPARE(original.width, 1920);
    QCOMPARE(copy.stream, 2);
    QCOMPARE(copy.height, 1080);
    QCOMPARE(copy.fps, 24.0F);
    QCOMPARE(copy.frames, 48);
}

void KisDlgImportVideoAnimationSchemaContractTest::basicVideoEncodingAndColorValueSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::duration), float>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::encoding), QString>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::pixFormat), QString>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::hasOverriddenFPS), bool>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::colorPrimaries), ColorPrimaries>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::colorTransfer), TransferCharacteristics>);
    static_assert(std::is_same_v<decltype(KisBasicVideoInfo::colorDepth), QString>);

    KisBasicVideoInfo original;
    QCOMPARE(original.duration, 0.0F);
    QVERIFY(original.encoding.isEmpty());
    QVERIFY(original.pixFormat.isEmpty());
    QVERIFY(!original.hasOverriddenFPS);
    QCOMPARE(original.colorPrimaries, PRIMARIES_UNSPECIFIED);
    QCOMPARE(original.colorTransfer, TRC_UNSPECIFIED);
    QVERIFY(original.colorDepth.isEmpty());

    original.duration = 2.0F;
    original.encoding = QStringLiteral("h264");
    original.pixFormat = QStringLiteral("yuv420p");
    original.hasOverriddenFPS = true;
    original.colorDepth = QStringLiteral("8");
    KisBasicVideoInfo copy = original;
    copy.encoding = QStringLiteral("vp9");
    copy.colorDepth = QStringLiteral("10");
    QCOMPARE(original.encoding, QStringLiteral("h264"));
    QCOMPARE(original.colorDepth, QStringLiteral("8"));
    QCOMPARE(copy.duration, 2.0F);
    QVERIFY(copy.hasOverriddenFPS);
}

void KisDlgImportVideoAnimationSchemaContractTest::renderedFramesValueSchemaRemainsStable()
{
    static_assert(std::is_same_v<decltype(RenderedFrames::renderedFrameFiles), QStringList>);
    static_assert(std::is_same_v<decltype(RenderedFrames::renderedFrameTargetTimes), QList<int>>);

    RenderedFrames original;
    QVERIFY(!original.framesNeedRelocation());
    QVERIFY(original.isEmpty());
    QCOMPARE(original.size(), size_t(0));

    original.renderedFrameFiles.append(QStringLiteral("frame0001.png"));
    original.renderedFrameTargetTimes.append(3);
    RenderedFrames copy = original;
    copy.renderedFrameFiles.append(QStringLiteral("frame0002.png"));
    copy.renderedFrameTargetTimes[0] = 7;
    QVERIFY(original.framesNeedRelocation());
    QVERIFY(!original.isEmpty());
    QCOMPARE(original.size(), size_t(1));
    QCOMPARE(original.renderedFrameTargetTimes.constFirst(), 3);
    QCOMPARE(copy.size(), size_t(2));
    QCOMPARE(copy.renderedFrameTargetTimes.constFirst(), 7);
}

void KisDlgImportVideoAnimationSchemaContractTest::videoImportDialogIdentityAndLifecycleSignatureRemainsStable()
{
    static_assert(std::is_class_v<KisDlgImportVideoAnimation>);
    static_assert(std::is_constructible_v<KisDlgImportVideoAnimation, KisMainWindow *, KisView *>);
}

void KisDlgImportVideoAnimationSchemaContractTest::videoImportDialogPublicWorkflowSignaturesRemainStable()
{
    using DocumentInfo = QStringList (KisDlgImportVideoAnimation::*)();
    using RenderFrames = RenderedFrames (KisDlgImportVideoAnimation::*)(const QDir &);
    using ShowOpenFileDialog = QStringList (KisDlgImportVideoAnimation::*)();

    static_assert(
        std::is_same_v<decltype(static_cast<DocumentInfo>(&KisDlgImportVideoAnimation::documentInfo)), DocumentInfo>);
    static_assert(
        std::is_same_v<decltype(static_cast<RenderFrames>(&KisDlgImportVideoAnimation::renderFrames)), RenderFrames>);
    static_assert(
        std::is_same_v<decltype(static_cast<ShowOpenFileDialog>(&KisDlgImportVideoAnimation::showOpenFileDialog)),
                       ShowOpenFileDialog>);
}

QTEST_APPLESS_MAIN(KisDlgImportVideoAnimationSchemaContractTest)

#include "KisDlgImportVideoAnimationSchemaContractTest.moc"

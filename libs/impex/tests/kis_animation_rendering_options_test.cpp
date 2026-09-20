/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QStandardPaths>
#include <QTest>

#include <KisAnimationRenderingOptions.h>

void kisSharedPtrAddReference(KisPropertiesConfiguration *)
{
}

bool kisSharedPtrRelease(KisPropertiesConfiguration *)
{
    return true;
}

class KisAnimationRenderingOptionsTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void defaultsDescribeFrameOnlyExport();
    void renderModesFollowEncodingAndSequencePolicy();
    void resolvesDocumentAndOutputPaths();
    void videoOnlyExplicitFramesDirectoryUsesStoredDocumentPath();
};

void KisAnimationRenderingOptionsTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KisAnimationRenderingOptionsTest::defaultsDescribeFrameOnlyExport()
{
    const KisAnimationRenderingOptions options;

    QVERIFY(options.lastDocumentPath.isEmpty());
#ifdef Q_OS_ANDROID
    QVERIFY(options.videoFormatKey.isEmpty());
    QVERIFY(options.videoFormatPreferencesJson.isEmpty());
#else
    QCOMPARE(options.videoMimeType, QStringLiteral("video/mp4"));
#endif
    QCOMPARE(options.frameMimeType, QStringLiteral("image/png"));
    QCOMPARE(options.basename, QStringLiteral("frame"));
    QVERIFY(options.directory.isEmpty());
    QCOMPARE(options.firstFrame, 0);
    QCOMPARE(options.lastFrame, 0);
    QCOMPARE(options.sequenceStart, 0);
    QVERIFY(!options.shouldEncodeVideo);
#ifndef Q_OS_ANDROID
    QVERIFY(!options.shouldDeleteSequence);
#endif
    QVERIFY(!options.includeAudio);
    QVERIFY(!options.wantsOnlyUniqueFrameSequence);
#ifndef Q_OS_ANDROID
    QVERIFY(options.ffmpegPath.isEmpty());
#endif
    QCOMPARE(options.frameRate, 25);
    QCOMPARE(options.width, 0);
    QCOMPARE(options.height, 0);
    QVERIFY(options.scaleFilter.isEmpty());
    QVERIFY(options.videoFileName.isEmpty());
#ifndef Q_OS_ANDROID
    QVERIFY(options.customFFMpegOptions.isEmpty());
#endif
    QVERIFY(!options.frameExportConfig);
}

void KisAnimationRenderingOptionsTest::renderModesFollowEncodingAndSequencePolicy()
{
    QCOMPARE(int(KisAnimationRenderingOptions::RENDER_FRAMES_ONLY), 0);
    QCOMPARE(int(KisAnimationRenderingOptions::RENDER_VIDEO_ONLY), 1);
#ifndef Q_OS_ANDROID
    QCOMPARE(int(KisAnimationRenderingOptions::RENDER_FRAMES_AND_VIDEO), 2);
#endif

    KisAnimationRenderingOptions options;
    QCOMPARE(options.renderMode(), KisAnimationRenderingOptions::RENDER_FRAMES_ONLY);

    options.shouldEncodeVideo = true;
#ifdef Q_OS_ANDROID
    QCOMPARE(options.renderMode(), KisAnimationRenderingOptions::RENDER_VIDEO_ONLY);
#else
    QCOMPARE(options.renderMode(), KisAnimationRenderingOptions::RENDER_FRAMES_AND_VIDEO);
    options.shouldDeleteSequence = true;
    QCOMPARE(options.renderMode(), KisAnimationRenderingOptions::RENDER_VIDEO_ONLY);
#endif
}

void KisAnimationRenderingOptionsTest::resolvesDocumentAndOutputPaths()
{
#ifdef Q_OS_ANDROID
    QSKIP("Desktop path APIs are not available on Android");
#else
    KisAnimationRenderingOptions options;
    const QString documentPath = QStringLiteral("/tmp/librepaint-contract/project/document.kra");
    options.lastDocumentPath = documentPath;
    options.videoFileName = QStringLiteral("exports/movie.mp4");
    options.directory = QStringLiteral("frames");

    QCOMPARE(options.resolveAbsoluteDocumentFilePath(documentPath), documentPath);
    QCOMPARE(options.resolveAbsoluteDocumentFilePath(QString()),
             QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    QCOMPARE(options.resolveAbsoluteVideoFilePath(documentPath),
             QStringLiteral("/tmp/librepaint-contract/project/exports/movie.mp4"));
    QCOMPARE(options.resolveAbsoluteVideoFilePath(),
             QStringLiteral("/tmp/librepaint-contract/project/exports/movie.mp4"));
    QCOMPARE(options.resolveAbsoluteFramesDirectory(documentPath),
             QStringLiteral("/tmp/librepaint-contract/project/frames"));
    QCOMPARE(options.resolveAbsoluteFramesDirectory(), QStringLiteral("/tmp/librepaint-contract/project/frames"));

#endif
}

void KisAnimationRenderingOptionsTest::videoOnlyExplicitFramesDirectoryUsesStoredDocumentPath()
{
#ifdef Q_OS_ANDROID
    QSKIP("Desktop path APIs are not available on Android");
#else
    KisAnimationRenderingOptions options;
    options.lastDocumentPath = QStringLiteral("/tmp/librepaint-contract/stored/document.kra");
    options.videoFileName = QStringLiteral("exports/movie.mp4");
    options.shouldEncodeVideo = true;
    options.shouldDeleteSequence = true;

    QCOMPARE(options.resolveAbsoluteFramesDirectory(QStringLiteral("/tmp/argument/other.kra")),
             QStringLiteral("/tmp/librepaint-contract/stored/exports"));
#endif
}

QTEST_GUILESS_MAIN(KisAnimationRenderingOptionsTest)

#include "kis_animation_rendering_options_test.moc"

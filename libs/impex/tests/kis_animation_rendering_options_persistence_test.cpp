/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QStandardPaths>
#include <QTest>

#include <KisAnimationRenderingOptions.h>
#include <kis_image_config.h>

namespace
{
void populateOptions(KisAnimationRenderingOptions *options)
{
    options->lastDocumentPath = QStringLiteral("/tmp/librepaint-contract/document.kra");
#ifdef Q_OS_ANDROID
    options->videoFormatKey = QStringLiteral("video/mp4");
    options->videoFormatPreferencesJson = QStringLiteral("{\"quality\":7}");
#else
    options->videoMimeType = QStringLiteral("video/x-matroska");
#endif
    options->frameMimeType = QStringLiteral("image/webp");
    options->basename = QStringLiteral("contract-frame");
    options->directory = QStringLiteral("contract-frames");
    options->firstFrame = 3;
    options->lastFrame = 17;
    options->sequenceStart = 40;
    options->shouldEncodeVideo = true;
#ifndef Q_OS_ANDROID
    options->shouldDeleteSequence = true;
#endif
    options->includeAudio = true;
    options->wantsOnlyUniqueFrameSequence = true;
#ifndef Q_OS_ANDROID
    options->ffmpegPath = QStringLiteral("/opt/contract/ffmpeg");
#endif
    options->frameRate = 30;
    options->width = 1920;
    options->height = 1080;
    options->scaleFilter = QStringLiteral("lanczos");
    options->videoFileName = QStringLiteral("contract.mp4");
#ifndef Q_OS_ANDROID
    options->customFFMpegOptions = QStringLiteral("-preset contract");
#endif
    options->frameExportConfig = new KisPropertiesConfiguration();
    options->frameExportConfig->setProperty(QStringLiteral("quality"), 83);
}

void comparePersistedOptions(const KisAnimationRenderingOptions &actual)
{
    QCOMPARE(actual.lastDocumentPath, QStringLiteral("/tmp/librepaint-contract/document.kra"));
#ifdef Q_OS_ANDROID
    QCOMPARE(actual.videoFormatKey, QStringLiteral("video/mp4"));
    QCOMPARE(actual.videoFormatPreferencesJson, QStringLiteral("{\"quality\":7}"));
#else
    QCOMPARE(actual.videoMimeType, QStringLiteral("video/x-matroska"));
#endif
    QCOMPARE(actual.frameMimeType, QStringLiteral("image/webp"));
    QCOMPARE(actual.basename, QStringLiteral("contract-frame"));
    QCOMPARE(actual.directory, QStringLiteral("contract-frames"));
    QCOMPARE(actual.firstFrame, 3);
    QCOMPARE(actual.lastFrame, 17);
    QCOMPARE(actual.sequenceStart, 40);
    QVERIFY(actual.shouldEncodeVideo);
#ifndef Q_OS_ANDROID
    QVERIFY(actual.shouldDeleteSequence);
#endif
    QVERIFY(actual.includeAudio);
    QVERIFY(actual.wantsOnlyUniqueFrameSequence);
#ifndef Q_OS_ANDROID
    QCOMPARE(actual.ffmpegPath, QStringLiteral("/opt/contract/ffmpeg"));
#endif
    QCOMPARE(actual.frameRate, 30);
    QCOMPARE(actual.width, 1920);
    QCOMPARE(actual.height, 1080);
    QVERIFY(actual.scaleFilter.isEmpty());
    QCOMPARE(actual.videoFileName, QStringLiteral("contract.mp4"));
#ifndef Q_OS_ANDROID
    QCOMPARE(actual.customFFMpegOptions, QStringLiteral("-preset contract"));
#endif
    QVERIFY(actual.frameExportConfig);
    QCOMPARE(actual.frameExportConfig->getInt(QStringLiteral("quality")), 83);
}
} // namespace

class KisAnimationRenderingOptionsPersistenceTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void propertiesRoundTripStoredValuesAndDefaults();
    void lastUsedReadsAnimationExportConfiguration();
};

void KisAnimationRenderingOptionsPersistenceTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KisAnimationRenderingOptionsPersistenceTest::propertiesRoundTripStoredValuesAndDefaults()
{
    KisAnimationRenderingOptions source;
    populateOptions(&source);

    KisAnimationRenderingOptions restored;
    restored.fromProperties(source.toProperties());
    comparePersistedOptions(restored);

    KisAnimationRenderingOptions missing;
    missing.fromProperties(new KisPropertiesConfiguration());
#ifdef Q_OS_ANDROID
    QVERIFY(missing.shouldEncodeVideo);
#else
    QVERIFY(!missing.shouldEncodeVideo);
#endif
    QVERIFY(missing.includeAudio);
    QCOMPARE(missing.frameRate, 25);
    QVERIFY(missing.frameExportConfig);
    QVERIFY(!missing.frameExportConfig->hasProperty(QStringLiteral("quality")));
}

void KisAnimationRenderingOptionsPersistenceTest::lastUsedReadsAnimationExportConfiguration()
{
    KisAnimationRenderingOptions saved;
    populateOptions(&saved);
    KisImageConfig(false).setExportConfiguration(QStringLiteral("ANIMATION_EXPORT"), saved.toProperties());

    comparePersistedOptions(KisAnimationRenderingOptions::loadLastUsed());
}

QTEST_GUILESS_MAIN(KisAnimationRenderingOptionsPersistenceTest)

#include "kis_animation_rendering_options_persistence_test.moc"

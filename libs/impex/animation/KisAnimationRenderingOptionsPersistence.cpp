/*
 *  SPDX-FileCopyrightText: 2019 Dmitry Kazakov <dimula73@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include "KisAnimationRenderingOptions.h"

#include <kis_image_config.h>

KisAnimationRenderingOptions KisAnimationRenderingOptions::loadLastUsed()
{
    KisAnimationRenderingOptions options;
    options.fromProperties(KisImageConfig(true).exportConfiguration(QStringLiteral("ANIMATION_EXPORT")));
    return options;
}

KisPropertiesConfigurationSP KisAnimationRenderingOptions::toProperties() const
{
    KisPropertiesConfigurationSP config = new KisPropertiesConfiguration();

    config->setProperty("basename", basename);
    config->setProperty("last_document_path", lastDocumentPath);
    config->setProperty("directory", directory);
    config->setProperty("first_frame", firstFrame);
    config->setProperty("last_frame", lastFrame);
    config->setProperty("sequence_start", sequenceStart);
    config->setProperty("frame_mimetype", frameMimeType);

    config->setProperty("encode_video", shouldEncodeVideo);
#ifndef Q_OS_ANDROID
    config->setProperty("delete_sequence", shouldDeleteSequence);
#endif
    config->setProperty("only_unique_frames", wantsOnlyUniqueFrameSequence);

    config->setProperty("framerate", frameRate);
    config->setProperty("height", height);
    config->setProperty("width", width);
    config->setProperty("include_audio", includeAudio);
    config->setProperty("filename", videoFileName);

#ifdef Q_OS_ANDROID
    config->setProperty("video_format_key", videoFormatKey);
    config->setProperty("video_format_preferences_json", videoFormatPreferencesJson);
#else
    config->setProperty("video_mimetype", videoMimeType);
    config->setProperty("ffmpeg_path", ffmpegPath);
    config->setProperty("custom_ffmpeg_options", customFFMpegOptions);
#endif

    config->setPrefixedProperties("frame_export/", frameExportConfig);

    return config;
}

void KisAnimationRenderingOptions::fromProperties(KisPropertiesConfigurationSP config)
{
    basename = config->getPropertyLazy("basename", basename);
    lastDocumentPath = config->getPropertyLazy("last_document_path", "");
    directory = config->getPropertyLazy("directory", directory);
    firstFrame = config->getPropertyLazy("first_frame", 0);
    lastFrame = config->getPropertyLazy("last_frame", 0);
    sequenceStart = config->getPropertyLazy("sequence_start", 0);
    frameMimeType = config->getPropertyLazy("frame_mimetype", frameMimeType);

#ifdef Q_OS_ANDROID
    bool encodeVideoDefault = true;
#else
    bool encodeVideoDefault = false;
#endif
    shouldEncodeVideo = config->getPropertyLazy("encode_video", encodeVideoDefault);
#ifndef Q_OS_ANDROID
    shouldDeleteSequence = config->getPropertyLazy("delete_sequence", false);
#endif
    wantsOnlyUniqueFrameSequence = config->getPropertyLazy("only_unique_frames", false);

    frameRate = config->getPropertyLazy("framerate", 25);
    height = config->getPropertyLazy("height", 0);
    width = config->getPropertyLazy("width", 0);
    includeAudio = config->getPropertyLazy("include_audio", true);
    videoFileName = config->getPropertyLazy("filename", "");

#ifdef Q_OS_ANDROID
    videoFormatKey = config->getPropertyLazy("video_format_key", QString());
    videoFormatPreferencesJson = config->getPropertyLazy("video_format_preferences_json", QString());
#else
    videoMimeType = config->getPropertyLazy("video_mimetype", videoMimeType);
    ffmpegPath = config->getPropertyLazy("ffmpeg_path", "");
    customFFMpegOptions = config->getPropertyLazy("custom_ffmpeg_options", "");
#endif

    frameExportConfig = new KisPropertiesConfiguration();
    config->getPrefixedProperties("frame_export/", frameExportConfig);
}

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisMediaEncoderWrapper.h>

#include <QTest>

#include <type_traits>

namespace
{
struct WidgetToken {
    int value;
};

QWidget *widget(WidgetToken &token)
{
    return reinterpret_cast<QWidget *>(&token);
}

class FormatProbe final : public KisMediaEncoderFormat
{
public:
    explicit FormatProbe(bool *destroyed)
        : m_destroyed(destroyed)
    {
    }

    ~FormatProbe() override
    {
        *m_destroyed = true;
    }

    Type type() const override
    {
        return Type::AndroidMediaEncoder;
    }

    QString key() const override
    {
        return QStringLiteral("encoder-κλειδί");
    }

    QString title() const override
    {
        return QStringLiteral("メディア変換");
    }

    QString extension() const override
    {
        return QStringLiteral(".vidéo");
    }

    bool supportsAudio() const override
    {
        return true;
    }

    QWidget *createPreferencesWidget(const QVariantMap &preferences) const override
    {
        createPreferences = preferences;
        return widgetResult;
    }

    void resetPreferencesWidget(QWidget *value) const override
    {
        resetWidget = value;
        ++resetCount;
    }

    QVariantMap getPreferencesFromWidget(QWidget *value) const override
    {
        readWidget = value;
        return preferencesResult;
    }

    mutable QVariantMap createPreferences;
    mutable QWidget *resetWidget = nullptr;
    mutable QWidget *readWidget = nullptr;
    mutable int resetCount = 0;
    QWidget *widgetResult = nullptr;
    QVariantMap preferencesResult;

private:
    bool *m_destroyed;
};
} // namespace

class KisMediaEncoderFormatAndSettingsContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void formatIdentityPreservesStableMetadata();
    void preferencesPreserveBorrowedWidgetAndMapDelivery();
    void settingsPreserveTextAndInputOrder();
    void settingsPreserveFormatMapAndDimensions();
    void settingsPreserveTimelineAndCopyIndependence();
    void runnableExecutionAndCancellationSignaturesRemainStable();
    void runnableCompletionSignalSignaturesRemainStable();
    void runnableFailureAndProgressSignalSignaturesRemainStable();
    void wrapperTypeConstructionAndLifetimeSchemaRemainStable();
    void wrapperExecutionAndFormatLookupSignaturesRemainStable();
    void wrapperLifecycleSignalSignaturesRemainStable();
};

void KisMediaEncoderFormatAndSettingsContractTest::formatIdentityPreservesStableMetadata()
{
    QCOMPARE(int(KisMediaEncoderFormat::Type::AndroidMediaEncoder), 0);
    QCOMPARE(int(KisMediaEncoderFormat::Type::LibavMediaEncoder), 1);

    bool destroyed = false;
    KisMediaEncoderFormat *format = new FormatProbe(&destroyed);
    QCOMPARE(format->type(), KisMediaEncoderFormat::Type::AndroidMediaEncoder);
    QCOMPARE(format->key(), QStringLiteral("encoder-κλειδί"));
    QCOMPARE(format->title(), QStringLiteral("メディア変換"));
    QCOMPARE(format->extension(), QStringLiteral(".vidéo"));
    QVERIFY(format->supportsAudio());
    delete format;
    QVERIFY(destroyed);
}

void KisMediaEncoderFormatAndSettingsContractTest::preferencesPreserveBorrowedWidgetAndMapDelivery()
{
    bool destroyed = false;
    FormatProbe format(&destroyed);
    WidgetToken token{73};
    const QVariantMap input{{QStringLiteral("品質"), 91}, {QStringLiteral("codec"), QStringLiteral("av1")}};
    const QVariantMap output{{QStringLiteral("bitrate"), 48000}};
    format.widgetResult = widget(token);
    format.preferencesResult = output;

    QCOMPARE(format.createPreferencesWidget(input), widget(token));
    QCOMPARE(format.createPreferences, input);
    format.resetPreferencesWidget(widget(token));
    QCOMPARE(format.resetWidget, widget(token));
    QCOMPARE(format.resetCount, 1);
    QCOMPARE(format.getPreferencesFromWidget(widget(token)), output);
    QCOMPARE(format.readWidget, widget(token));
    QCOMPARE(token.value, 73);
}

void KisMediaEncoderFormatAndSettingsContractTest::settingsPreserveTextAndInputOrder()
{
    KisMediaEncoderWrapperSettings settings{};
    settings.outputFile = QStringLiteral("出力/映像.webm");
    settings.inputFiles = {QStringLiteral("frame-一.png"), QStringLiteral("frame-二.png")};
    settings.audioFile = QStringLiteral("音声/曲.flac");
    settings.scaleFilter = QStringLiteral("lanczos");

    QCOMPARE(settings.outputFile, QStringLiteral("出力/映像.webm"));
    QCOMPARE(settings.inputFiles, QStringList({QStringLiteral("frame-一.png"), QStringLiteral("frame-二.png")}));
    QCOMPARE(settings.audioFile, QStringLiteral("音声/曲.flac"));
    QCOMPARE(settings.scaleFilter, QStringLiteral("lanczos"));
}

void KisMediaEncoderFormatAndSettingsContractTest::settingsPreserveFormatMapAndDimensions()
{
    bool destroyed = false;
    FormatProbe format(&destroyed);
    KisMediaEncoderWrapperSettings settings{};
    settings.format = &format;
    settings.formatPreferences = {{QStringLiteral("profile"), QStringLiteral("main10")},
                                  {QStringLiteral("quality"), 88}};
    settings.outputSize = QSize(3840, 2160);

    QCOMPARE(settings.format, &format);
    QCOMPARE(settings.formatPreferences.value(QStringLiteral("profile")), QVariant(QStringLiteral("main10")));
    QCOMPARE(settings.formatPreferences.value(QStringLiteral("quality")), QVariant(88));
    QCOMPARE(settings.outputSize, QSize(3840, 2160));
}

void KisMediaEncoderFormatAndSettingsContractTest::settingsPreserveTimelineAndCopyIndependence()
{
    KisMediaEncoderWrapperSettings original{};
    original.inputFps = 24;
    original.outputFps = 60;
    original.firstFrameSec = 3;
    original.lastFrameSec = 17;
    original.audioSeekFrame = 72;

    KisMediaEncoderWrapperSettings copy = original;
    original.inputFps = 30;
    original.firstFrameSec = 5;

    QCOMPARE(copy.inputFps, 24);
    QCOMPARE(copy.outputFps, 60);
    QCOMPARE(copy.firstFrameSec, 3);
    QCOMPARE(copy.lastFrameSec, 17);
    QCOMPARE(copy.audioSeekFrame, 72);
    QCOMPARE(original.inputFps, 30);
    QCOMPARE(original.firstFrameSec, 5);
}

#define ASSERT_MEDIA_ENCODER_SIGNATURE(Type, Method, Signature)                                                        \
    static_assert(std::is_same_v<decltype(static_cast<Signature>(&Type::Method)), Signature>)

void KisMediaEncoderFormatAndSettingsContractTest::runnableExecutionAndCancellationSignaturesRemainStable()
{
    using Runnable = KisMediaEncoderRunnable;
    using VoidMember = void (Runnable::*)();

    static_assert(std::is_class_v<Runnable>);
    static_assert(std::is_base_of_v<QObject, Runnable>);
    static_assert(std::is_base_of_v<QRunnable, Runnable>);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Runnable, run, VoidMember);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Runnable, slotHandleCancelRequested, VoidMember);
}

void KisMediaEncoderFormatAndSettingsContractTest::runnableCompletionSignalSignaturesRemainStable()
{
    using Runnable = KisMediaEncoderRunnable;
    using VoidMember = void (Runnable::*)();

    ASSERT_MEDIA_ENCODER_SIGNATURE(Runnable, sigStarted, VoidMember);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Runnable, sigCompleted, VoidMember);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Runnable, sigCancelled, VoidMember);
}

void KisMediaEncoderFormatAndSettingsContractTest::runnableFailureAndProgressSignalSignaturesRemainStable()
{
    using Runnable = KisMediaEncoderRunnable;
    using Failure = void (Runnable::*)(const QString &);
    using Progress = void (Runnable::*)(int);

    ASSERT_MEDIA_ENCODER_SIGNATURE(Runnable, sigFailed, Failure);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Runnable, sigProgressUpdated, Progress);
}

void KisMediaEncoderFormatAndSettingsContractTest::wrapperTypeConstructionAndLifetimeSchemaRemainStable()
{
    using Wrapper = KisMediaEncoderWrapper;

    static_assert(std::is_class_v<Wrapper>);
    static_assert(std::is_base_of_v<QObject, Wrapper>);
    static_assert(std::is_constructible_v<Wrapper, QObject *>);
    static_assert(std::is_default_constructible_v<Wrapper>);
    static_assert(std::has_virtual_destructor_v<Wrapper>);
}

void KisMediaEncoderFormatAndSettingsContractTest::wrapperExecutionAndFormatLookupSignaturesRemainStable()
{
    using Wrapper = KisMediaEncoderWrapper;
    using FormatLookup = KisMediaEncoderFormat *(*)(const QString &);
    using Formats = const QVector<KisMediaEncoderFormat *> &(*)();
    using Start = KisImportExportErrorCode (Wrapper::*)(const KisMediaEncoderWrapperSettings &, bool);
    using StartNonBlocking = void (Wrapper::*)(const KisMediaEncoderWrapperSettings &);

    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, start, Start);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, startNonBlocking, StartNonBlocking);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, getFormatByKey, FormatLookup);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, getSupportedFormats, Formats);
}

void KisMediaEncoderFormatAndSettingsContractTest::wrapperLifecycleSignalSignaturesRemainStable()
{
    using Wrapper = KisMediaEncoderWrapper;
    using Error = void (Wrapper::*)(QString);
    using Progress = void (Wrapper::*)(int);
    using VoidMember = void (Wrapper::*)();

    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, reset, VoidMember);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, sigStarted, VoidMember);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, sigFinished, VoidMember);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, sigFinishedWithError, Error);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, sigProgressUpdated, Progress);
    ASSERT_MEDIA_ENCODER_SIGNATURE(Wrapper, sigCancelRequested, VoidMember);
}

#undef ASSERT_MEDIA_ENCODER_SIGNATURE

QTEST_GUILESS_MAIN(KisMediaEncoderFormatAndSettingsContractTest)

#include "KisMediaEncoderFormatAndSettingsContractTest.moc"

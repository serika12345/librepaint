/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisFFMpegWrapper.h>

#include <QTest>

#include <type_traits>
#include <utility>

class KisFFMpegWrapperSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ffmpegSettingsValueSchemaRemainsStable();
    void ffmpegWrapperIdentityAndErrorSchemaRemainStable();
    void ffmpegDiscoveryAndColorConversionSignaturesRemainStable();
    void ffmpegProcessAndProbeSignaturesRemainStable();
    void ffmpegSignalSignaturesRemainStable();
};

void KisFFMpegWrapperSchemaContractTest::ffmpegSettingsValueSchemaRemainsStable()
{
    QVERIFY(std::is_aggregate_v<KisFFMpegWrapperSettings>);
    QVERIFY(std::is_copy_constructible_v<KisFFMpegWrapperSettings>);
    QVERIFY(std::is_copy_assignable_v<KisFFMpegWrapperSettings>);

    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::processPath), QString>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::args), QStringList>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::outputFile), QString>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::storeOutput), bool>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::logPath), QString>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::defaultPrependArgs), QStringList>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::batchMode), bool>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::binaryOutput), bool>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::totalFrames), int>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::progressMessage), QString>));
    QVERIFY((std::is_same_v<decltype(KisFFMpegWrapperSettings::progressIndeterminate), bool>));

    KisFFMpegWrapperSettings defaults{};
    QVERIFY(defaults.processPath.isEmpty());
    QVERIFY(defaults.args.isEmpty());
    QVERIFY(defaults.outputFile.isEmpty());
    QVERIFY(!defaults.storeOutput);
    QVERIFY(defaults.logPath.isEmpty());
    QCOMPARE(defaults.defaultPrependArgs, QStringList({QStringLiteral("-hide_banner"), QStringLiteral("-y")}));
    QVERIFY(!defaults.batchMode);
    QVERIFY(!defaults.binaryOutput);
    QCOMPARE(defaults.totalFrames, 0);
    QVERIFY(defaults.progressMessage.isEmpty());
    QVERIFY(!defaults.progressIndeterminate);

    defaults.processPath = QStringLiteral("ffmpeg-original");
    defaults.args = {QStringLiteral("-input"), QStringLiteral("frame-original.png")};
    defaults.outputFile = QStringLiteral("output-original.webm");
    defaults.storeOutput = true;
    defaults.logPath = QStringLiteral("log-original.txt");
    defaults.defaultPrependArgs = {QStringLiteral("-nostdin")};
    defaults.batchMode = true;
    defaults.binaryOutput = true;
    defaults.totalFrames = 73;
    defaults.progressMessage = QStringLiteral("progress-original");
    defaults.progressIndeterminate = true;

    KisFFMpegWrapperSettings copy = defaults;
    defaults.processPath = QStringLiteral("ffmpeg-mutated");
    defaults.args.append(QStringLiteral("mutated"));
    defaults.outputFile = QStringLiteral("output-mutated.webm");
    defaults.storeOutput = false;
    defaults.logPath = QStringLiteral("log-mutated.txt");
    defaults.defaultPrependArgs.append(QStringLiteral("-mutated"));
    defaults.batchMode = false;
    defaults.binaryOutput = false;
    defaults.totalFrames = 91;
    defaults.progressMessage = QStringLiteral("progress-mutated");
    defaults.progressIndeterminate = false;

    QCOMPARE(copy.processPath, QStringLiteral("ffmpeg-original"));
    QCOMPARE(copy.args, QStringList({QStringLiteral("-input"), QStringLiteral("frame-original.png")}));
    QCOMPARE(copy.outputFile, QStringLiteral("output-original.webm"));
    QVERIFY(copy.storeOutput);
    QCOMPARE(copy.logPath, QStringLiteral("log-original.txt"));
    QCOMPARE(copy.defaultPrependArgs, QStringList({QStringLiteral("-nostdin")}));
    QVERIFY(copy.batchMode);
    QVERIFY(copy.binaryOutput);
    QCOMPARE(copy.totalFrames, 73);
    QCOMPARE(copy.progressMessage, QStringLiteral("progress-original"));
    QVERIFY(copy.progressIndeterminate);
}

void KisFFMpegWrapperSchemaContractTest::ffmpegWrapperIdentityAndErrorSchemaRemainStable()
{
    QVERIFY((std::is_base_of_v<QObject, KisFFMpegWrapper>));
    QVERIFY(std::is_constructible_v<KisFFMpegWrapper>);
    QVERIFY((std::is_constructible_v<KisFFMpegWrapper, QObject *>));
    QVERIFY(std::is_destructible_v<KisFFMpegWrapper>);
    QVERIFY(std::has_virtual_destructor_v<KisFFMpegWrapper>);

    QVERIFY(std::is_enum_v<FFProbeErrorCodes>);
    QCOMPARE(int(FFProbeErrorCodes::NONE), 0);
    QCOMPARE(int(FFProbeErrorCodes::UNSUPPORTED_CODEC), 1);
    QCOMPARE(int(FFProbeErrorCodes::INVALID_JSON), 2);
    QVERIFY((std::is_same_v<decltype(FFMPEG_TIMEOUT), const int>));
    QCOMPARE(FFMPEG_TIMEOUT, 600000);
}

void KisFFMpegWrapperSchemaContractTest::ffmpegDiscoveryAndColorConversionSignaturesRemainStable()
{
    using ColorPrimariesFromName = ColorPrimaries (*)(QString);
    using ConfiguredFFMpegLocation = QString (*)();
    using FindFFMpeg = QJsonObject (*)(const QString &);
    using FindFFProbe = QJsonObject (*)(const QString &);
    using FindProcessInfo = QJsonObject (*)(const QString &, const QString &, bool);
    using FindProcessPath = QJsonObject (*)(const QString &, const QString &, bool);
    using GetSupportedCodecs = QStringList (*)(const QJsonObject &);
    using SetConfiguredFFMpegLocation = void (*)(QString &);
    using TransferCharacteristicsFromName = TransferCharacteristics (*)(QString);

    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::colorPrimariesFromName), ColorPrimariesFromName>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::configuredFFMpegLocation), ConfiguredFFMpegLocation>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::findFFMpeg), FindFFMpeg>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::findFFProbe), FindFFProbe>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::findProcessInfo), FindProcessInfo>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::findProcessPath), FindProcessPath>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::getSupportedCodecs), GetSupportedCodecs>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::setConfiguredFFMpegLocation), SetConfiguredFFMpegLocation>));
    QVERIFY((
        std::is_same_v<decltype(&KisFFMpegWrapper::transferCharacteristicsFromName), TransferCharacteristicsFromName>));
}

void KisFFMpegWrapperSchemaContractTest::ffmpegProcessAndProbeSignaturesRemainStable()
{
    using FFMpegProbe = QJsonObject (KisFFMpegWrapper::*)(const QString &, const QString &, bool);
    using FFProbe = QJsonObject (KisFFMpegWrapper::*)(const QString &, const QString &);
    using Reset = void (KisFFMpegWrapper::*)();
    using RunProcessAndReturn = QByteArray (*)(const QString &, const QStringList &, int);
    using Start = KisImportExportErrorCode (KisFFMpegWrapper::*)(const KisFFMpegWrapperSettings &);
    using StartNonBlocking = void (KisFFMpegWrapper::*)(const KisFFMpegWrapperSettings &);
    using WaitForFinished = bool (KisFFMpegWrapper::*)(int);

    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::ffmpegProbe), FFMpegProbe>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::ffprobe), FFProbe>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::reset), Reset>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::runProcessAndReturn), RunProcessAndReturn>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::start), Start>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::startNonBlocking), StartNonBlocking>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::waitForFinished), WaitForFinished>));

    using DefaultRunResult = decltype(KisFFMpegWrapper::runProcessAndReturn(std::declval<const QString &>(),
                                                                            std::declval<const QStringList &>()));
    using DefaultWaitResult = decltype(std::declval<KisFFMpegWrapper &>().waitForFinished());
    QVERIFY((std::is_same_v<DefaultRunResult, QByteArray>));
    QVERIFY((std::is_same_v<DefaultWaitResult, bool>));
}

void KisFFMpegWrapperSchemaContractTest::ffmpegSignalSignaturesRemainStable()
{
    using Finished = void (KisFFMpegWrapper::*)();
    using FinishedWithError = void (KisFFMpegWrapper::*)(QString);
    using ProgressUpdated = void (KisFFMpegWrapper::*)(int);
    using ReadLine = void (KisFFMpegWrapper::*)(int, QString);
    using ReadBuffer = void (KisFFMpegWrapper::*)(QByteArray);
    using Started = void (KisFFMpegWrapper::*)();

    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::sigFinished), Finished>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::sigFinishedWithError), FinishedWithError>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::sigProgressUpdated), ProgressUpdated>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::sigReadLine), ReadLine>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::sigReadSTDERR), ReadBuffer>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::sigReadSTDOUT), ReadBuffer>));
    QVERIFY((std::is_same_v<decltype(&KisFFMpegWrapper::sigStarted), Started>));
}

QTEST_APPLESS_MAIN(KisFFMpegWrapperSchemaContractTest)

#include "KisFFMpegWrapperSchemaContractTest.moc"

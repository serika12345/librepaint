/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisFFMpegWrapper.h>

#include <QTest>

#include <utility>

class KisFFMpegWrapperSchemaContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void ffmpegSettingsValueSchemaRemainsStable();
    void ffmpegWrapperIdentityAndErrorSchemaRemainStable();
};

void KisFFMpegWrapperSchemaContractTest::ffmpegSettingsValueSchemaRemainsStable()
{


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

    QCOMPARE(int(FFProbeErrorCodes::NONE), 0);
    QCOMPARE(int(FFProbeErrorCodes::UNSUPPORTED_CODEC), 1);
    QCOMPARE(int(FFProbeErrorCodes::INVALID_JSON), 2);
    QCOMPARE(FFMPEG_TIMEOUT, 600000);
}

QTEST_APPLESS_MAIN(KisFFMpegWrapperSchemaContractTest)

#include "KisFFMpegWrapperSchemaContractTest.moc"

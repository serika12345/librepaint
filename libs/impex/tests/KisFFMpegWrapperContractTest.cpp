/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <KisFFMpegWrapper.h>

#include <QCoreApplication>
#include <QFile>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QTextStream>

namespace
{
constexpr auto SuccessChildArgument = "--ffmpeg-wrapper-contract-success";
constexpr auto FailureChildArgument = "--ffmpeg-wrapper-contract-failure";

int runChildProcess(const QStringList &arguments)
{
    if (arguments.contains(QLatin1String(SuccessChildArgument))) {
        QTextStream(stdout) << "completed\n";
        return 0;
    }

    if (arguments.contains(QLatin1String(FailureChildArgument))) {
        QTextStream(stderr) << "Invalid input frame\n";
        return 3;
    }

    return -1;
}
}

class KisFFMpegWrapperContractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void successfulProcessNotifiesCompletionAndWritesLog();
    void failedProcessReportsFailureAndDiagnostic();
};

KisFFMpegWrapperSettings childProcessSettings(const QTemporaryDir &directory, const QString &childArgument)
{
    KisFFMpegWrapperSettings settings;
    settings.processPath = QCoreApplication::applicationFilePath();
    settings.args = {childArgument};
    settings.outputFile = directory.filePath(QStringLiteral("rendered-video"));
    settings.defaultPrependArgs.clear();
    settings.batchMode = true;
    return settings;
}

void KisFFMpegWrapperContractTest::successfulProcessNotifiesCompletionAndWritesLog()
{
    // Consumer: artists exporting or importing animation through FFmpeg.
    // Operation: run an external encoder process that completes successfully.
    // Observable result: the wrapper returns success, emits start/finish, and records the command.
    // Failure impact: the UI reports a failed export or import despite a completed encoder process.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    KisFFMpegWrapper wrapper;
    QSignalSpy started(&wrapper, &KisFFMpegWrapper::sigStarted);
    QSignalSpy finished(&wrapper, &KisFFMpegWrapper::sigFinished);
    QSignalSpy failed(&wrapper, &KisFFMpegWrapper::sigFinishedWithError);

    const KisFFMpegWrapperSettings settings = childProcessSettings(directory, QLatin1String(SuccessChildArgument));
    QVERIFY(wrapper.start(settings).isOk());

    QCOMPARE(started.count(), 1);
    QCOMPARE(finished.count(), 1);
    QCOMPARE(failed.count(), 0);

    QFile log(settings.outputFile + QStringLiteral(".log"));
    QVERIFY(log.open(QIODevice::ReadOnly));
    QVERIFY(log.readAll().contains(SuccessChildArgument));
}

void KisFFMpegWrapperContractTest::failedProcessReportsFailureAndDiagnostic()
{
    // Consumer: artists whose selected FFmpeg operation fails.
    // Operation: run an external encoder process that returns an error.
    // Observable result: the wrapper returns failure and emits the process diagnostic.
    // Failure impact: the UI continues as if animation frames or video output were created.
    QTemporaryDir directory;
    QVERIFY(directory.isValid());

    KisFFMpegWrapper wrapper;
    QSignalSpy finished(&wrapper, &KisFFMpegWrapper::sigFinished);
    QSignalSpy failed(&wrapper, &KisFFMpegWrapper::sigFinishedWithError);

    const KisFFMpegWrapperSettings settings = childProcessSettings(directory, QLatin1String(FailureChildArgument));
    QVERIFY(!wrapper.start(settings).isOk());

    QCOMPARE(finished.count(), 0);
    QCOMPARE(failed.count(), 1);
    QVERIFY(failed.constFirst().constFirst().toString().contains(QStringLiteral("Invalid input frame")));
}

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);

    const int childResult = runChildProcess(application.arguments());
    if (childResult >= 0) {
        return childResult;
    }

    KisFFMpegWrapperContractTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "KisFFMpegWrapperContractTest.moc"

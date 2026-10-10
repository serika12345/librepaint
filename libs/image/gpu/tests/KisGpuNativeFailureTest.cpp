/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "KisGpuDevice.h"
#include <QtTest>
#include <QCoreApplication>
#include <QProcess>
#include <future>
#include <stdexcept>

namespace {
int failureCase(const QString &name)
{
    KisGpuDevice device;
    if (name.startsWith(QStringLiteral("submit"))) {
        const auto queue = wgpuDeviceGetQueue(device.device());
        const auto encoder = wgpuDeviceCreateCommandEncoder(device.device(), nullptr);
        const auto commands = wgpuCommandEncoderFinish(encoder, nullptr);
        // A valid command buffer loses its device before reaching the native submission boundary.
        wgpuDeviceDestroy(device.device());
        bool refused = true;
        if (name == QStringLiteral("submit-index")) {
            refused = wgpuQueueSubmitForIndex(queue, 1, &commands) == 0;
        } else {
            wgpuQueueSubmit(queue, 1, &commands);
        }
        const bool notified = !device.available() && !device.lastError().isEmpty();
        wgpuCommandBufferRelease(commands);
        wgpuCommandEncoderRelease(encoder);
        wgpuQueueRelease(queue);
        return refused && notified ? 0 : 1;
    }
    const bool write = name == QStringLiteral("write-map");
    WGPUBufferDescriptor descriptor{};
    descriptor.size = 4;
    descriptor.usage = write ? WGPUBufferUsage_MapWrite | WGPUBufferUsage_CopySrc
                             : WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst;
    const auto buffer = wgpuDeviceCreateBuffer(device.device(), &descriptor);
    std::promise<WGPUMapAsyncStatus> promise;
    auto mapped = promise.get_future();
    WGPUBufferMapCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowSpontaneous;
    callback.userdata1 = &promise;
    callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void *data, void *) {
        static_cast<std::promise<WGPUMapAsyncStatus> *>(data)->set_value(status);
    };
    wgpuBufferMapAsync(buffer, write ? WGPUMapMode_Write : WGPUMapMode_Read, 0, 4, callback);
    wgpuDevicePoll(device.device(), true, nullptr);
    if (mapped.get() != WGPUMapAsyncStatus_Success) return 2;
    // A destroyed buffer has no mapped range even if its earlier map operation succeeded.
    wgpuBufferDestroy(buffer);
    const void *pixels = write ? wgpuBufferGetMappedRange(buffer, 0, 4)
                               : wgpuBufferGetConstMappedRange(buffer, 0, 4);
    const bool notified = !device.available() && !device.lastError().isEmpty();
    wgpuBufferRelease(buffer);
    return pixels == nullptr && notified ? 0 : 1;
}
}

/** Consumers are GPU submission, saved pixel readback, and device recreation after loss. */
class KisGpuNativeFailureTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void nativeFailureReturnsToTheCaller_data() {
        QTest::addColumn<QString>("operation");
        for (const auto *name : {"submit-index", "submit", "read-map", "write-map"}) {
            QTest::newRow(name) << QString::fromLatin1(name);
        }
    }
    void nativeFailureReturnsToTheCaller() {
        QFETCH(QString, operation);
        QProcess process;
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--gpu-failure-case"), operation});
        QVERIFY(process.waitForStarted(5000));
        if (!process.waitForFinished(10000)) {
            process.kill();
            process.waitForFinished();
            QFAIL("Native GPU failure handling did not return");
        }
        const QByteArray diagnostic = process.readAll();
        QVERIFY2(process.exitStatus() == QProcess::NormalExit, diagnostic.constData());
        QVERIFY2(process.exitCode() == 0, diagnostic.constData());
    }
};

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);
    if (argc == 3 && QString::fromLocal8Bit(argv[1]) == QStringLiteral("--gpu-failure-case")) {
        try {
            return failureCase(QString::fromLocal8Bit(argv[2]));
        } catch (const std::exception &error) {
            qCritical() << error.what();
            return 2;
        }
    }
    KisGpuNativeFailureTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "KisGpuNativeFailureTest.moc"

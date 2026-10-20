/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFile>
#include <QGuiApplication>
#include <QTest>
#include <gpu/KisGpuDevice.h>
#include <gpu/KisGpuReadbackBatch_p.h>
#include <cmath>
#include <cstring>
#include <vector>

using namespace LibrePaint::Image::Gpu;
using namespace LibrePaint::Image::Gpu::Detail;

namespace {
quint32 floatBits(float value)
{
    quint32 bits;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}
}

class KisGpuProjectionArithmeticTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void normalRgba8QuotientsPreserveNearestEvenRounding();
};

void KisGpuProjectionArithmeticTest::normalRgba8QuotientsPreserveNearestEvenRounding()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    QFile shared(QStringLiteral(":/librepaint/gpu/shaders/rgba.wgsl"));
    QFile over(QStringLiteral(":/librepaint/gpu/shaders/over.wgsl"));
    QVERIFY(shared.open(QIODevice::ReadOnly));
    QVERIFY(over.open(QIODevice::ReadOnly));
    // Exercise the production quotient on every opacity/source/destination
    // alpha combination that reaches division during normal RGBA8 projection.
    const QByteArray source = shared.readAll() + '\n' + over.readAll() + R"(
@compute @workgroup_size(64)
fn quotientContract(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let index = invocation.x;
    if (index < arrayLength(&outputPixels)) {
        outputPixels[index] = projectionDivide(basePixels[index], layerPixels[index]);
    }
}
)";
    WGPUShaderSourceWGSL code{};
    code.chain.sType = WGPUSType_ShaderSourceWGSL;
    code.code = {source.constData(), size_t(source.size())};
    WGPUShaderModuleDescriptor shaderDescriptor{};
    shaderDescriptor.nextInChain = &code.chain;
    NativeShader shader(wgpuDeviceCreateShaderModule(device->nativeDevice(), &shaderDescriptor));
    QVERIFY2(shader && device->diagnostic().isEmpty(), qPrintable(device->diagnostic()));
    WGPUComputePipelineDescriptor pipelineDescriptor{};
    pipelineDescriptor.compute.module = shader.get();
    pipelineDescriptor.compute.entryPoint = {"quotientContract", WGPU_STRLEN};
    NativeComputePipeline pipeline(wgpuDeviceCreateComputePipeline(device->nativeDevice(), &pipelineDescriptor));
    QVERIFY2(pipeline && device->diagnostic().isEmpty(), qPrintable(device->diagnostic()));
    NativeBindingLayout layout(wgpuComputePipelineGetBindGroupLayout(pipeline.get(), 0));

    constexpr quint32 count = 255 * 254;
    constexpr quint64 bytes = count * sizeof(quint32);
    auto numerator = createBuffer(device->nativeDevice(), bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto denominator = createBuffer(device->nativeDevice(), bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto output = createBuffer(device->nativeDevice(), bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    QVERIFY(numerator && denominator && output);
    WGPUBindGroupEntry entries[3]{};
    WGPUBuffer buffers[] = {numerator.get(), denominator.get(), output.get()};
    for (quint32 index = 0; index < 3; ++index) {
        entries[index].binding = index;
        entries[index].buffer = buffers[index];
        entries[index].size = bytes;
    }
    WGPUBindGroupDescriptor bindingsDescriptor{};
    bindingsDescriptor.layout = layout.get();
    bindingsDescriptor.entryCount = 3;
    bindingsDescriptor.entries = entries;
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &bindingsDescriptor));
    QVERIFY2(bindings && device->diagnostic().isEmpty(), qPrintable(device->diagnostic()));

    std::vector<quint32> numerators(count), denominators(count), expected(count);
    for (int opacity = 1; opacity <= 255; ++opacity) {
        // Volatile stores establish the binary32 multiplication boundaries;
        // std::fma supplies the single-rounded destination alpha addition.
        const float factor = float(opacity) / 255.0f;
        for (int sourceAlpha = 1; sourceAlpha <= 255; ++sourceAlpha) {
            const volatile float alpha = factor * float(sourceAlpha);
            for (int destinationAlpha = 1; destinationAlpha < 255; ++destinationAlpha) {
                const auto index = (sourceAlpha - 1) * 254 + destinationAlpha - 1;
                const volatile float weighted = alpha * float(255 - destinationAlpha);
                const float combined = std::fma(float(weighted), 1.0f / 255.0f, float(destinationAlpha));
                numerators[index] = floatBits(alpha);
                denominators[index] = floatBits(combined);
                expected[index] = floatBits(float(alpha) / combined);
            }
        }
        wgpuQueueWriteBuffer(device->queue(), numerator.get(), 0, numerators.data(), bytes);
        wgpuQueueWriteBuffer(device->queue(), denominator.get(), 0, denominators.data(), bytes);
        const auto commands = device->beginCommands();
        QVERIFY(commands);
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        QVERIFY(pass);
        wgpuComputePassEncoderSetPipeline(pass, pipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, (count + 63) / 64, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        const auto readback = ReadbackBatch::create(*device, 1);
        QVERIFY(readback && readback->recordBuffer(*device, commands, 0, output.get(), bytes));
        QVERIFY2(device->submitCommands(true), qPrintable(device->diagnostic()));
        const auto &actual = readback->results.front()->data;
        QCOMPARE(actual.size(), qsizetype(bytes));
        for (quint32 index = 0; index < count; ++index) {
            quint32 bits;
            std::memcpy(&bits, actual.constData() + index * sizeof(bits), sizeof(bits));
            if (bits != expected[index]) QFAIL(qPrintable(QStringLiteral(
                "opacity=%1 sourceAlpha=%2 destinationAlpha=%3 numerator=%4 denominator=%5 actual=%6 expected=%7")
                .arg(opacity).arg(index / 254 + 1).arg(index % 254 + 1)
                .arg(numerators[index], 8, 16, QLatin1Char('0')).arg(denominators[index], 8, 16, QLatin1Char('0'))
                .arg(bits, 8, 16, QLatin1Char('0')).arg(expected[index], 8, 16, QLatin1Char('0'))));
        }
    }
}

int main(int argc, char **argv)
{
    QGuiApplication app(argc, argv);
    KisGpuProjectionArithmeticTest test;
    return QTest::qExec(&test, argc, argv);
}
#include "KisGpuProjectionArithmeticTest.moc"

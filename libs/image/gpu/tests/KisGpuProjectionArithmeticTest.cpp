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
#include <limits>
#include <set>
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
    void normalRgba8AlphaAndQuotientsPreserveNearestEvenRounding();
    void normalRgba8ColorsPreserveBothRoundingSteps();
};

void KisGpuProjectionArithmeticTest::normalRgba8AlphaAndQuotientsPreserveNearestEvenRounding()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    QFile shared(QStringLiteral(":/librepaint/gpu/shaders/rgba.wgsl"));
    QFile arithmetic(QStringLiteral(":/librepaint/gpu/shaders/binary32.wgsl"));
    QFile separable(QStringLiteral(":/librepaint/gpu/shaders/separable.wgsl"));
    QFile normal(QStringLiteral(":/librepaint/gpu/shaders/normal-rgba8.wgsl"));
    QFile over(QStringLiteral(":/librepaint/gpu/shaders/over.wgsl"));
    QVERIFY(shared.open(QIODevice::ReadOnly));
    QVERIFY(arithmetic.open(QIODevice::ReadOnly));
    QVERIFY(separable.open(QIODevice::ReadOnly));
    QVERIFY(normal.open(QIODevice::ReadOnly));
    QVERIFY(over.open(QIODevice::ReadOnly));
    // Exercise alpha normalization and division on every opacity/source/
    // destination alpha combination that reaches normal RGBA8 division.
    const QByteArray source = shared.readAll() + '\n' + arithmetic.readAll() + '\n' + separable.readAll() + '\n' + normal.readAll() + '\n' + over.readAll() + R"(
@compute @workgroup_size(64)
fn quotientContract(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let index = invocation.x;
    if (index < arrayLength(&basePixels)) {
        outputPixels[index * 4u] = projectionDivide(basePixels[index], layerPixels[index]);
        outputPixels[index * 4u + 1u] = projectionAlpha(basePixels[index], index % 254u + 1u);
        outputPixels[index * 4u + 2u] = projectionScaleByte(basePixels[index], 1u);
        outputPixels[index * 4u + 3u] = projectionMultiply(basePixels[index], 0x3f000000u);
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
    constexpr quint64 outputBytes = bytes * 4;
    auto numerator = createBuffer(device->nativeDevice(), bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto denominator = createBuffer(device->nativeDevice(), bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto output = createBuffer(device->nativeDevice(), outputBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    QVERIFY(numerator && denominator && output);
    WGPUBindGroupEntry entries[3]{};
    WGPUBuffer buffers[] = {numerator.get(), denominator.get(), output.get()};
    for (quint32 index = 0; index < 3; ++index) {
        entries[index].binding = index;
        entries[index].buffer = buffers[index];
        entries[index].size = index == 2 ? outputBytes : bytes;
    }
    WGPUBindGroupDescriptor bindingsDescriptor{};
    bindingsDescriptor.layout = layout.get();
    bindingsDescriptor.entryCount = 3;
    bindingsDescriptor.entries = entries;
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &bindingsDescriptor));
    QVERIFY2(bindings && device->diagnostic().isEmpty(), qPrintable(device->diagnostic()));

    std::vector<quint32> numerators(count), denominators(count), expected(count);
    // Canvas opacity retains binary32 values before final RGBA8 quantization,
    // including zero, very small values and gradual underflow with a mask.
    std::vector<float> opacities{0.0f, 0.37f, 0.5f, 1e-15f,
                                std::numeric_limits<float>::min(),
                                std::nextafter(std::numeric_limits<float>::min(), 0.0f),
                                std::numeric_limits<float>::denorm_min(),
                                3 * std::numeric_limits<float>::denorm_min()};
    for (int opacity = 1; opacity <= 255; ++opacity) opacities.push_back(float(opacity) / 255.0f);
    for (const float factor : opacities) {
        // Volatile stores establish binary32 multiplication boundaries. The
        // binary64 product and addition supply a reference independent of the
        // host's binary32 fma implementation for this finite RGBA8 domain.
        for (int sourceAlpha = 1; sourceAlpha <= 255; ++sourceAlpha) {
            const volatile float alpha = factor * float(sourceAlpha);
            for (int destinationAlpha = 1; destinationAlpha < 255; ++destinationAlpha) {
                const auto index = (sourceAlpha - 1) * 254 + destinationAlpha - 1;
                const volatile float weighted = alpha * float(255 - destinationAlpha);
                const float combined = float(double(weighted) * double(1.0f / 255.0f) + double(destinationAlpha));
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
        QVERIFY(readback && readback->recordBuffer(*device, commands, 0, output.get(), outputBytes));
        QVERIFY2(device->submitCommands(true), qPrintable(device->diagnostic()));
        const auto &actual = readback->results.front()->data;
        QCOMPARE(actual.size(), qsizetype(outputBytes));
        for (quint32 index = 0; index < count; ++index) {
            quint32 bits;
            std::memcpy(&bits, actual.constData() + index * 4 * sizeof(bits), sizeof(bits));
            if (bits != expected[index]) QFAIL(qPrintable(QStringLiteral(
                "opacity=%1 sourceAlpha=%2 destinationAlpha=%3 numerator=%4 denominator=%5 actual=%6 expected=%7")
                .arg(floatBits(factor), 8, 16, QLatin1Char('0')).arg(index / 254 + 1).arg(index % 254 + 1)
                .arg(numerators[index], 8, 16, QLatin1Char('0')).arg(denominators[index], 8, 16, QLatin1Char('0'))
                .arg(bits, 8, 16, QLatin1Char('0')).arg(expected[index], 8, 16, QLatin1Char('0'))));
            std::memcpy(&bits, actual.constData() + (index * 4 + 1) * sizeof(bits), sizeof(bits));
            if (bits != denominators[index]) QFAIL(qPrintable(QStringLiteral(
                "alpha opacity=%1 sourceAlpha=%2 destinationAlpha=%3 actual=%4 expected=%5")
                .arg(floatBits(factor), 8, 16, QLatin1Char('0')).arg(index / 254 + 1).arg(index % 254 + 1)
                .arg(bits, 8, 16, QLatin1Char('0')).arg(denominators[index], 8, 16, QLatin1Char('0'))));
            std::memcpy(&bits, actual.constData() + (index * 4 + 2) * sizeof(bits), sizeof(bits));
            QCOMPARE(bits, numerators[index]);
            float alpha;
            std::memcpy(&alpha, &numerators[index], sizeof(alpha));
            const volatile float masked = alpha * 0.5f;
            std::memcpy(&bits, actual.constData() + (index * 4 + 3) * sizeof(bits), sizeof(bits));
            QCOMPARE(bits, floatBits(masked));
        }
    }
}

void KisGpuProjectionArithmeticTest::normalRgba8ColorsPreserveBothRoundingSteps()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    QFile shared(QStringLiteral(":/librepaint/gpu/shaders/rgba.wgsl"));
    QFile arithmetic(QStringLiteral(":/librepaint/gpu/shaders/binary32.wgsl"));
    QFile separable(QStringLiteral(":/librepaint/gpu/shaders/separable.wgsl"));
    QFile normal(QStringLiteral(":/librepaint/gpu/shaders/normal-rgba8.wgsl"));
    QFile over(QStringLiteral(":/librepaint/gpu/shaders/over.wgsl"));
    QVERIFY(shared.open(QIODevice::ReadOnly));
    QVERIFY(arithmetic.open(QIODevice::ReadOnly));
    QVERIFY(separable.open(QIODevice::ReadOnly));
    QVERIFY(normal.open(QIODevice::ReadOnly));
    QVERIFY(over.open(QIODevice::ReadOnly));
    // Normal layer projection supplies the same blend to every RGB channel.
    // Saving and exporting must retain the single-rounded binary32 color,
    // including its subsequent nearest-even quantization to an RGBA8 byte.
    const QByteArray source = shared.readAll() + '\n' + arithmetic.readAll() + '\n' + separable.readAll() + '\n' + normal.readAll() + '\n' + over.readAll() + R"(
@compute @workgroup_size(64)
fn colorContract(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let index = invocation.x;
    if (index < arrayLength(&outputPixels)) {
        outputPixels[index] = projectionColor(index >> 8u, index & 255u, parameters.opacityBits);
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
    pipelineDescriptor.compute.entryPoint = {"colorContract", WGPU_STRLEN};
    NativeComputePipeline pipeline(wgpuDeviceCreateComputePipeline(device->nativeDevice(), &pipelineDescriptor));
    QVERIFY2(pipeline && device->diagnostic().isEmpty(), qPrintable(device->diagnostic()));
    NativeBindingLayout layout(wgpuComputePipelineGetBindGroupLayout(pipeline.get(), 0));
    constexpr quint32 count = 256 * 256;
    constexpr quint64 bytes = count * sizeof(quint32);
    auto output = createBuffer(device->nativeDevice(), bytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    auto parameters = createBuffer(device->nativeDevice(), 16, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    QVERIFY(output && parameters);
    WGPUBindGroupEntry entries[2]{};
    entries[0].binding = 2;
    entries[0].buffer = output.get();
    entries[0].size = bytes;
    entries[1].binding = 3;
    entries[1].buffer = parameters.get();
    entries[1].size = 16;
    WGPUBindGroupDescriptor bindingsDescriptor{};
    bindingsDescriptor.layout = layout.get();
    bindingsDescriptor.entryCount = 2;
    bindingsDescriptor.entries = entries;
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &bindingsDescriptor));
    QVERIFY2(bindings && device->diagnostic().isEmpty(), qPrintable(device->diagnostic()));

    std::set<quint32> blends;
    for (int opacity = 1; opacity <= 255; ++opacity) {
        const float factor = float(opacity) / 255.0f;
        for (int sourceAlpha : {1, 128, 255}) {
            const volatile float alpha = factor * float(sourceAlpha);
            for (int destinationAlpha : {1, 128, 255}) {
                const volatile float weighted = alpha * float(255 - destinationAlpha);
                const float combined = float(double(weighted) * double(1.0f / 255.0f) + double(destinationAlpha));
                const volatile float blend = destinationAlpha == 255 ? alpha * (1.0f / 255.0f) : float(alpha) / combined;
                blends.insert(floatBits(blend));
            }
        }
    }
    // Probe half-byte boundaries and their adjacent binary32 values, from
    // the smallest color difference through the complete channel range.
    for (int difference : {1, 2, 3, 127, 128, 254, 255}) {
        for (int byte = 0; byte < difference; ++byte) {
            const float midpoint = (float(byte) + 0.5f) / float(difference);
            blends.insert(floatBits(std::nextafter(midpoint, 0.0f)));
            blends.insert(floatBits(midpoint));
            blends.insert(floatBits(std::nextafter(midpoint, 1.0f)));
        }
    }
    for (float blend : {0.0f, 1.0f, 1e-15f, std::numeric_limits<float>::min(),
                        std::numeric_limits<float>::denorm_min()}) blends.insert(floatBits(blend));
    for (quint32 blendBits : blends) {
        const quint32 values[4] = {blendBits, 0, 0, 0};
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0, values, sizeof(values));
        const auto commands = device->beginCommands();
        QVERIFY(commands);
        const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
        QVERIFY(pass);
        wgpuComputePassEncoderSetPipeline(pass, pipeline.get());
        wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
        wgpuComputePassEncoderDispatchWorkgroups(pass, count / 64, 1, 1);
        wgpuComputePassEncoderEnd(pass);
        wgpuComputePassEncoderRelease(pass);
        const auto readback = ReadbackBatch::create(*device, 1);
        QVERIFY(readback && readback->recordBuffer(*device, commands, 0, output.get(), bytes));
        QVERIFY2(device->submitCommands(true), qPrintable(device->diagnostic()));
        const auto &actual = readback->results.front()->data;
        QCOMPARE(actual.size(), qsizetype(bytes));
        float blend;
        std::memcpy(&blend, &blendBits, sizeof(blend));
        for (quint32 index = 0; index < count; ++index) {
            const int destination = int(index >> 8), source = int(index & 255);
            // At byte-changing blend values, binary64 retains the exact product
            // and sum before the float store supplies the first rounding.
            // Smaller blends preserve the destination byte even if binary64
            // rounds away the change. This avoids a platform fma dependency.
            const volatile float color = float(double(source - destination) * double(blend) + double(destination));
            const auto expected = quint32(std::nearbyint(color));
            quint32 value;
            std::memcpy(&value, actual.constData() + index * sizeof(value), sizeof(value));
            if (value != expected) QFAIL(qPrintable(QStringLiteral(
                "source=%1 destination=%2 blend=%3 actual=%4 expected=%5")
                .arg(source).arg(destination).arg(blendBits, 8, 16, QLatin1Char('0')).arg(value).arg(expected)));
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

/*
 * SPDX-FileCopyrightText: 2026 LibrePaint contributors
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include <QFile>
#include <QDir>
#include <QGuiApplication>
#include <QTest>
#include <gpu/KisGpuDevice.h>
#include <gpu/KisGpuReadbackBatch_p.h>
#include <algorithm>
#include <array>
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
    void normalRgba8ColorsStayWithinOneStep();
    void normalRgba16StaysWithinOneEightBitStep();
};

void KisGpuProjectionArithmeticTest::normalRgba8ColorsStayWithinOneStep()
{
    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    QFile shared(QStringLiteral(":/librepaint/gpu/shaders/rgba.wgsl"));
    QFile arithmetic(QStringLiteral(":/librepaint/gpu/shaders/binary32.wgsl"));
    QFile separable(QStringLiteral(":/librepaint/gpu/shaders/separable.wgsl"));
    QFile doubles(QStringLiteral(":/librepaint/gpu/shaders/binary64.wgsl"));
    QFile color(QStringLiteral(":/librepaint/gpu/shaders/color-blend.wgsl"));
    QFile normal(QStringLiteral(":/librepaint/gpu/shaders/normal-rgba8.wgsl"));
    QFile native(QStringLiteral(":/librepaint/gpu/shaders/normal-rgba16.wgsl"));
    QFile copy(QStringLiteral(":/librepaint/gpu/shaders/copy.wgsl"));
    QFile separable16(QStringLiteral(":/librepaint/gpu/shaders/separable-rgba16.wgsl"));
    QFile over(QStringLiteral(":/librepaint/gpu/shaders/over.wgsl"));
    QVERIFY(shared.open(QIODevice::ReadOnly));
    QVERIFY(arithmetic.open(QIODevice::ReadOnly));
    QVERIFY(separable.open(QIODevice::ReadOnly));
    QVERIFY(doubles.open(QIODevice::ReadOnly));
    QVERIFY(color.open(QIODevice::ReadOnly));
    QVERIFY(normal.open(QIODevice::ReadOnly));
    QVERIFY(native.open(QIODevice::ReadOnly));
    QVERIFY(copy.open(QIODevice::ReadOnly));
    QVERIFY(separable16.open(QIODevice::ReadOnly));
    QVERIFY(over.open(QIODevice::ReadOnly));
    // Normal layer projection supplies the same blend to every RGB channel.
    // Saving and exporting must retain the single-rounded binary32 color,
    // including its subsequent nearest-even quantization to an RGBA8 byte.
    const QByteArray source = QByteArray("const COLOR_BLEND_MODE_A: u32 = 0u;\nconst COLOR_BLEND_MODE_B: u32 = 0u;\nconst SEPARABLE_BLEND_ENABLED: bool = true;\nconst COUPLED_PIXEL_FORMAT: u32 = 0u;\nconst WORD_BLEND_MODE_A: u32 = 0u;\nconst WORD_BLEND_MODE_B: u32 = 0u;\n") + shared.readAll() + '\n' + arithmetic.readAll() + '\n' + doubles.readAll() + '\n' + color.readAll() + '\n' + separable.readAll() + '\n' + normal.readAll() + '\n' + native.readAll() + '\n' + separable16.readAll() + '\n' + copy.readAll() + '\n' + over.readAll() + R"(
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
    std::array<quint32, 12> values{};
    auto parameters = createBuffer(device->nativeDevice(), sizeof(values), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst);
    QVERIFY(output && parameters);
    WGPUBindGroupEntry entries[2]{};
    entries[0].binding = 2;
    entries[0].buffer = output.get();
    entries[0].size = bytes;
    entries[1].binding = 3;
    entries[1].buffer = parameters.get();
    entries[1].size = sizeof(values);
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
        values[0] = blendBits;
        wgpuQueueWriteBuffer(device->queue(), parameters.get(), 0, values.data(), sizeof(values));
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
            // #107: one 8-bit step.
            if ((value > expected ? value - expected : expected - value) > 1u) QFAIL(qPrintable(QStringLiteral(
                "source=%1 destination=%2 blend=%3 actual=%4 expected=%5")
                .arg(source).arg(destination).arg(blendBits, 8, 16, QLatin1Char('0')).arg(value).arg(expected)));
        }
    }
}


void KisGpuProjectionArithmeticTest::normalRgba16StaysWithinOneEightBitStep()
{
    // Layer projection, native marker painting and saved pixels consume this
    // arithmetic. Expected values come from the native CPU compositor.
    struct Input {
        std::array<quint32, 4> destination;
        std::array<quint32, 4> source;
        quint32 opacity, mask, writable, padding = 0;
    };
    std::vector<Input> inputs;
    const auto append = [&](std::array<quint32, 4> destination, std::array<quint32, 4> source,
                            float opacity, quint32 mask = 255, quint32 writable = 15) {
        inputs.push_back({destination, source, floatBits(opacity), mask, writable});
    };
    // This recorded native CPU case distinguishes fused and separate rounding.
    append({6428, 56750, 11086, 49993}, {7435, 7030, 44014, 53742}, 1.0f);
    quint32 random = 0x19ab45de;
    const auto next = [&]() { random = random * 1664525u + 1013904223u; return random >> 16; };
    for (int i = 0; i < 8192; ++i) {
        std::array<quint32, 4> destination, source;
        for (auto &value : destination) value = next();
        for (auto &value : source) value = next();
        for (float opacity : {1.0f, 0.5f, 0.37f}) {
            append(destination, source, opacity);
            append(destination, source, opacity, next() & 255u);
        }
    }
    const std::array<float, 9> opacities{0.0f, 1.0f, 0.5f, 1e-15f,
        std::numeric_limits<float>::min(), std::numeric_limits<float>::denorm_min(),
        std::nextafter(std::numeric_limits<float>::min(), 0.0f),
        std::nextafter(0.5f, 0.0f), std::nextafter(0.5f, 1.0f)};
    for (quint32 alpha : {0u, 1u, 257u, 32767u, 65534u, 65535u}) {
        for (quint32 sourceAlpha : {0u, 1u, 257u, 32767u, 65534u, 65535u}) {
            for (float opacity : opacities) {
                for (quint32 mask : {0u, 1u, 128u, 254u, 255u}) {
                    for (quint32 writable = 0; writable < 16; ++writable)
                        append({65534, 257, 1, alpha}, {1, 65534, 32767, sourceAlpha}, opacity, mask, writable);
                }
            }
        }
    }
    // Half-component boundaries and adjacent binary32 blends cover both signs
    // of the color difference, including the full 16-bit range and low bits.
    for (quint32 difference : {1u, 2u, 3u, 257u, 32767u, 65534u, 65535u}) {
        for (quint32 step = 0; step <= 16; ++step) {
            const quint32 component = qMin(difference - 1, step * difference / 16);
            const float midpoint = (float(component) + 0.5f) / float(difference);
            for (float opacity : {std::nextafter(midpoint, 0.0f), midpoint, std::nextafter(midpoint, 1.0f)}) {
                append({0, difference, 65535 - difference, 65535},
                       {difference, 0, 65535, 65535}, opacity);
            }
        }
    }
    std::vector<std::array<quint32, 4>> expected;
    expected.reserve(inputs.size());
    for (const auto &input : inputs) {
        auto pixel = input.destination;
        float opacity;
        std::memcpy(&opacity, &input.opacity, sizeof(opacity));
        const volatile float normalizedSource = float(input.source[3]) * (1.0f / 65535.0f);
        const volatile float opaqueSource = normalizedSource * opacity;
        const volatile float mask = float(input.mask) * (1.0f / 255.0f);
        const volatile float alpha = opaqueSource * mask;
        if (alpha != 0.0f) {
            const volatile float normalizedDestination = float(pixel[3]) * (1.0f / 65535.0f);
            float destinationAlpha = normalizedDestination;
            float blend;
            if (!(input.writable & 8u) || pixel[3] == 65535) blend = alpha;
            else if (pixel[3] == 0) {
                destinationAlpha = alpha;
                blend = 1.0f;
                if (input.writable != 15) pixel = {};
            } else {
                const volatile float remaining = 1.0f - normalizedDestination;
                destinationAlpha = std::fma(float(remaining), float(alpha), float(normalizedDestination));
                const volatile float quotient = alpha / destinationAlpha;
                blend = quotient;
            }
            for (int channel = 0; channel < 3; ++channel) {
                if (input.writable & (1u << channel)) {
                    pixel[channel] = quint32(std::nearbyint(std::fma(
                        float(int(input.source[channel]) - int(pixel[channel])), blend, float(pixel[channel]))));
                }
            }
            if (input.writable & 8u) {
                const volatile float scaled = destinationAlpha * 65535.0f;
                pixel[3] = quint32(std::nearbyint(float(scaled)));
            }
        }
        expected.push_back(pixel);
    }
    QCOMPARE(expected.front(), (std::array<quint32, 4>{7291, 14159, 39292, 62738}));

    QString diagnostic;
    const auto device = Device::createNative(diagnostic);
    QVERIFY2(device, qPrintable(diagnostic));
    QFile arithmetic(QStringLiteral(":/librepaint/gpu/shaders/binary32.wgsl"));
    QFile native(QStringLiteral(":/librepaint/gpu/shaders/normal-rgba16.wgsl"));
    QVERIFY(arithmetic.open(QIODevice::ReadOnly));
    QVERIFY2(native.open(QIODevice::ReadOnly), "Native RGBA16 composition shader is unavailable");
    const QByteArray source = arithmetic.readAll() + '\n' + native.readAll() + R"(
struct Input { destination: vec4<u32>, source: vec4<u32>, values: vec4<u32>, }
@group(0) @binding(0) var<storage, read> inputs: array<Input>;
@group(0) @binding(1) var<storage, read_write> outputs: array<vec4<u32>>;
@compute @workgroup_size(64)
fn nativeContract(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let index = invocation.x;
    if (index >= arrayLength(&inputs)) { return; }
    let input = inputs[index];
    let normalized = nativeNormalizedAlpha(input.source.w);
    let opaque = projectionMultiply(normalized, input.values.x);
    let mask = projectionScaleByte(0x3b808081u, input.values.y);
    let alpha = projectionMultiply(opaque, mask);
    outputs[index] = normalRgba16(input.destination, input.source.xyz, alpha, input.values.z);
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
    pipelineDescriptor.compute.entryPoint = {"nativeContract", WGPU_STRLEN};
    NativeComputePipeline pipeline(wgpuDeviceCreateComputePipeline(device->nativeDevice(), &pipelineDescriptor));
    QVERIFY2(pipeline && device->diagnostic().isEmpty(), qPrintable(device->diagnostic()));
    NativeBindingLayout layout(wgpuComputePipelineGetBindGroupLayout(pipeline.get(), 0));
    const quint64 inputBytes = inputs.size() * sizeof(Input), outputBytes = expected.size() * sizeof(expected.front());
    auto input = createBuffer(device->nativeDevice(), inputBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst);
    auto output = createBuffer(device->nativeDevice(), outputBytes, WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    QVERIFY(input && output);
    wgpuQueueWriteBuffer(device->queue(), input.get(), 0, inputs.data(), inputBytes);
    WGPUBindGroupEntry entries[2]{};
    entries[0].binding = 0; entries[0].buffer = input.get(); entries[0].size = inputBytes;
    entries[1].binding = 1; entries[1].buffer = output.get(); entries[1].size = outputBytes;
    WGPUBindGroupDescriptor bindingsDescriptor{};
    bindingsDescriptor.layout = layout.get();
    bindingsDescriptor.entryCount = 2;
    bindingsDescriptor.entries = entries;
    NativeBindings bindings(wgpuDeviceCreateBindGroup(device->nativeDevice(), &bindingsDescriptor));
    QVERIFY2(bindings && device->diagnostic().isEmpty(), qPrintable(device->diagnostic()));
    const auto commands = device->beginCommands();
    QVERIFY(commands);
    const auto pass = wgpuCommandEncoderBeginComputePass(commands, nullptr);
    QVERIFY(pass);
    wgpuComputePassEncoderSetPipeline(pass, pipeline.get());
    wgpuComputePassEncoderSetBindGroup(pass, 0, bindings.get(), 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, (inputs.size() + 63) / 64, 1, 1);
    wgpuComputePassEncoderEnd(pass);
    wgpuComputePassEncoderRelease(pass);
    const auto readback = ReadbackBatch::create(*device, 1);
    QVERIFY(readback && readback->recordBuffer(*device, commands, 0, output.get(), outputBytes));
    QVERIFY2(device->submitCommands(true), qPrintable(device->diagnostic()));
    const auto &actual = readback->results.front()->data;
    QCOMPARE(actual.size(), qsizetype(outputBytes));
    for (size_t index = 0; index < expected.size(); ++index) {
        std::array<quint32, 4> pixel;
        std::memcpy(pixel.data(), actual.constData() + index * sizeof(pixel), sizeof(pixel));
        // #107: GPU arithmetic is f32; components stay within one 8-bit step (257 for U16).
        // Alpha agrees on being zero; fully transparent pixels carry no color.
        const auto near = [](quint32 a, quint32 b) { return (a > b ? a - b : b - a) <= 257u; };
        const bool transparent = expected[index][3] == 0;
        const bool withinStep = (pixel[3] == 0) == transparent &&
            (transparent || std::equal(pixel.begin(), pixel.end(), expected[index].begin(), near));
        if (!withinStep) {
            QFAIL(qPrintable(QStringLiteral("case=%1 opacity=%2 mask=%3 writable=%4 actual=%5,%6,%7,%8 expected=%9,%10,%11,%12")
                .arg(index).arg(inputs[index].opacity, 8, 16, QLatin1Char('0'))
                .arg(inputs[index].mask).arg(inputs[index].writable)
                .arg(pixel[0]).arg(pixel[1]).arg(pixel[2]).arg(pixel[3])
                .arg(expected[index][0]).arg(expected[index][1]).arg(expected[index][2]).arg(expected[index][3])));
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

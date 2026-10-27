// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// flags holds transparent-base normalization in bit0 and RGBA16 precision in bit1.
// blendModeOpacity holds LayerBlendMode in the low byte and opacity in the next.
struct CompositeParameters { opacityBits: u32, flags: u32, channelMask: u32, blendModeOpacity: u32, }
@group(0) @binding(0) var<storage, read> basePixels: array<u32>;
@group(0) @binding(1) var<storage, read> layerPixels: array<u32>;
@group(0) @binding(2) var<storage, read_write> outputPixels: array<u32>;
@group(0) @binding(3) var<uniform> parameters: CompositeParameters;
struct CompositePairParameters { first: CompositeParameters, second: CompositeParameters, }
@group(0) @binding(4) var<storage, read> secondLayerPixels: array<u32>;
@group(0) @binding(5) var<uniform> pairParameters: CompositePairParameters;
// Four alpha bytes per word. Opaque coverage is bound for layers without a mask.
@group(0) @binding(6) var<storage, read> firstMaskCoverage: array<u32>;
@group(0) @binding(7) var<storage, read> secondMaskCoverage: array<u32>;

fn coverageAlpha(words: u32, index: u32) -> u32 {
    return (words >> ((index & 3u) * 8u)) & 255u;
}

fn projectionOver(base: u32, layer: u32, values: CompositeParameters, maskAlpha: u32) -> u32 {
    var destination = unpackPixel(base);
    if ((values.flags & 1u) != 0u && destination.w == 0u) { destination = vec4<u32>(0u); }
    var source = unpackPixel(layer);
    if (maskAlpha != 255u) { source.w = multiplyByte(source.w, maskAlpha); }
    if ((values.blendModeOpacity & 255u) != 0u) {
        return separableProjectionPixel(destination, source, values.blendModeOpacity >> 8u, values.channelMask, values.blendModeOpacity & 255u);
    }
    if (source.w == 0u) { return packPixel(destination); }
    if (values.channelMask == 15u && source.w == 255u && values.opacityBits == 0x3f800000u) {
        return packPixel(source);
    }
    // Full-opacity byte colors have binary32 error below 2^-14. The odd
    // divisor 255 of an opaque base stays farther from every half-byte.
    if (values.opacityBits == 0x3f800000u && destination.w == 255u) {
        for (var channel = 0u; channel < 3u; channel += 1u) {
            if ((values.channelMask & (1u << channel)) != 0u) {
                destination[channel] = (source[channel] * source.w + destination[channel] * (255u - source.w) + 127u) / 255u;
            }
        }
        return packPixel(destination);
    }
    if (values.opacityBits == 0x3f800000u && values.channelMask == 15u && destination.w != 0u) {
        let denominator = source.w * 255u + destination.w * (255u - source.w);
        let numerator = source.xyz * (source.w * 255u) + destination.xyz * (destination.w * (255u - source.w));
        let quotient = numerator / vec3<u32>(denominator);
        let doubledRemainder = (numerator - quotient * denominator) * 2u;
        let gap = abs(vec3<i32>(doubledRemainder) - vec3<i32>(i32(denominator)));
        // Rounding boundaries retain the precise calculation below.
        if (all(gap > vec3<i32>(16))) {
            // The distance exceeds 8/65025 in color units, beyond the error.
            let rounded = quotient + select(vec3<u32>(0u), vec3<u32>(1u), doubledRemainder > vec3<u32>(denominator));
            return packPixel(vec4<u32>(rounded, (denominator + 127u) / 255u));
        }
    }
    let sourceAlpha = projectionScaleByte(values.opacityBits, source.w);
    return normalRgba8(destination, source.xyz, sourceAlpha, values.channelMask);
}

fn projectionWords(base: vec2<u32>, layer: vec2<u32>, values: CompositeParameters, maskAlpha: u32) -> vec2<u32> {
    if ((values.flags & 2u) == 0u) {
        return vec2<u32>(projectionOver(base.x, layer.x, values, maskAlpha), 0u);
    }
    var destination = nativePixel(base.x, base.y, true);
    if ((values.flags & 1u) != 0u && destination.w == 0u) { destination = vec4<u32>(0u); }
    var source = nativePixel(layer.x, layer.y, true);
    if (maskAlpha != 255u) { source.w = mul65535(source.w, maskAlpha * 257u); }
    let sourceAlpha = projectionMultiply(nativeNormalizedAlpha(source.w), values.opacityBits);
    return nativePixelWords(normalRgba16(destination, source.xyz, sourceAlpha, values.channelMask), true);
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let position = invocation.xy;
    if (any(position >= vec2<u32>(256u))) { return; }
    let index = position.y * 256u + position.x;
    let wide = (parameters.flags & 2u) != 0u;
    let stride = select(1u, 2u, wide);
    let word = index * stride;
    let last = word + stride - 1u;
    let maskAlpha = coverageAlpha(firstMaskCoverage[index >> 2u], index);
    let pixel = projectionWords(vec2<u32>(basePixels[word], basePixels[last]),
        vec2<u32>(layerPixels[word], layerPixels[last]), parameters, maskAlpha);
    outputPixels[word] = pixel.x;
    if (wide) { outputPixels[last] = pixel.y; }
}

@compute @workgroup_size(8, 8)
fn mainPair(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let position = invocation.xy;
    if (any(position >= vec2<u32>(256u))) { return; }
    let index = position.y * 256u + position.x;
    let wide = (pairParameters.first.flags & 2u) != 0u;
    let stride = select(1u, 2u, wide);
    let word = index * stride;
    let last = word + stride - 1u;
    let firstMask = coverageAlpha(firstMaskCoverage[index >> 2u], index);
    let secondMask = coverageAlpha(secondMaskCoverage[index >> 2u], index);
    let first = projectionWords(vec2<u32>(basePixels[word], basePixels[last]),
        vec2<u32>(layerPixels[word], layerPixels[last]), pairParameters.first, firstMask);
    let pixel = projectionWords(first, vec2<u32>(secondLayerPixels[word], secondLayerPixels[last]),
        pairParameters.second, secondMask);
    outputPixels[word] = pixel.x;
    if (wide) { outputPixels[last] = pixel.y; }
}

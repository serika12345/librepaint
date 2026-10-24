// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct Parameters {
    tile: vec4<i32>,
    brush: vec4<u32>,
    counts: vec4<u32>,
    mirror: vec4<i32>,
}
struct Segment { endpoints: vec4<i32>, values: vec4<u32>, }
@group(0) @binding(0) var<uniform> parameters: Parameters;
@group(0) @binding(1) var<storage, read> geometryWords: array<u32>;
@group(0) @binding(2) var<storage, read> basePixels: array<u32>;
@group(0) @binding(3) var<storage, read_write> outputPixels: array<u32>;
@group(0) @binding(4) var<storage, read> fillEdges: array<Segment>;
@group(0) @binding(5) var<storage, read> previousCoverage: array<u32>;
@group(0) @binding(6) var<storage, read_write> retainedCoverage: array<u32>;

fn geometryFour(index: u32) -> vec4<u32> {
    return vec4<u32>(geometryWords[index], geometryWords[index + 1u], geometryWords[index + 2u], geometryWords[index + 3u]);
}
fn gaussianDab(index: u32) -> MaskDab {
    let offset = index * 24u;
    return MaskDab(geometryFour(offset), geometryFour(offset + 4u), geometryFour(offset + 8u),
                   geometryFour(offset + 12u), geometryFour(offset + 16u), geometryFour(offset + 20u));
}

fn washAlpha(destination: u32, coverage: u32, values: vec4<u32>) -> u32 {
    let flow = values.y;
    var opacity = values.x;
    var average = values.z;
    if (values.w == 0u) {
        opacity = floatMultiply(opacity, flow);
        average = floatMultiply(average, flow);
    }
    let opacityByte = floatMultiply(0x437f0000u, opacity);
    let averageByte = floatMultiply(0x437f0000u, average);
    let mask = floatMultiply(bitcast<u32>(f32(coverage)), 0x3b808081u);
    let source = floatMultiply(mask, opacityByte);
    if (source == 0u) { return destination; }
    let current = bitcast<u32>(f32(destination));
    var fullFlow = current;
    if (average > opacity) {
        if (averageByte > current) {
            fullFlow = floatAdd(floatMultiply(floatSubtract(averageByte, source), floatDivide(current, averageByte)), source);
        }
    } else if (opacityByte > current) {
        fullFlow = floatAdd(floatMultiply(floatSubtract(opacityByte, current), mask), current);
    }
    if (flow == 0x3f800000u) { return projectionByte(fullFlow); }
    var zeroFlow = current;
    if (values.w == 0u) {
        zeroFlow = floatSubtract(floatAdd(source, current), floatMultiply(floatMultiply(source, current), 0x3b808081u));
    }
    return projectionByte(floatAdd(floatMultiply(floatSubtract(fullFlow, zeroFlow), flow), zeroFlow));
}

fn gaussianPixel(pixel: vec2<i32>, base: vec4<u32>, color: vec4<u32>) -> u32 {
    var alpha = 0u;
    for (var index = 0u; index < parameters.counts.x; index += 1u) {
        let dab = gaussianDab(index);
        let point = pixel - bitcast<vec2<i32>>(dab.coefficient.zw);
        if (any(point < vec2<i32>(0)) || any(point >= vec2<i32>(dab.size.xy))) { continue; }
        var coverage: u32;
        if (dab.size.z > 1u) { coverage = smallCircleAlpha(dab, vec2<u32>(point)); }
        else { coverage = circleAlpha(dab, vec2<u32>(point)); }
        alpha = washAlpha(alpha, coverage, dab.application);
    }
    let opacity = bitcast<u32>(parameters.tile.z);
    if (alpha == 0u || opacity == 0u) { return packPixel(base); }
    if (parameters.brush.z != 0u) {
        // Erase rounds canvas opacity to a byte before its two alpha products.
        let opacityByte = u32(bitcast<f32>(floatAdd(projectionScaleByte(opacity, 255u), 0x3f000000u)));
        return packPixel(vec4<u32>(base.xyz, mul255(base.w, 255u - mul255(alpha, opacityByte))));
    }
    let sourceAlpha = projectionScaleByte(opacity, alpha);
    var resultAlpha = base.w;
    var blend: u32;
    if (base.w == 255u) { blend = projectionMultiply(sourceAlpha, 0x3b808081u); }
    else if (base.w == 0u) {
        resultAlpha = projectionByte(sourceAlpha);
        blend = 0x3f800000u;
    } else {
        let value = projectionAlpha(sourceAlpha, base.w);
        resultAlpha = projectionByte(value);
        blend = projectionDivide(sourceAlpha, value);
    }
    var result = color.xyz;
    if (blend != 0x3f800000u) {
        for (var channel = 0u; channel < 3u; channel += 1u) {
            result[channel] = projectionColor(base[channel], color[channel], blend);
        }
    }
    return packPixel(vec4<u32>(result, resultAlpha));
}

// Low word first; 16-bit products preserve the complete 64-bit result.
fn product(a: u32, b: u32) -> vec2<u32> {
    let low = (a & 65535u) * (b & 65535u);
    let crossA = (a >> 16u) * (b & 65535u);
    let crossB = (a & 65535u) * (b >> 16u);
    let middle = (low >> 16u) + (crossA & 65535u) + (crossB & 65535u);
    return vec2<u32>((low & 65535u) | (middle << 16u),
                     (a >> 16u) * (b >> 16u) + (crossA >> 16u) + (crossB >> 16u) + (middle >> 16u));
}
fn add64(a: vec2<u32>, b: vec2<u32>) -> vec2<u32> {
    let low = a.x + b.x;
    return vec2<u32>(low, a.y + b.y + select(0u, 1u, low < a.x));
}
fn negate64(a: vec2<u32>) -> vec2<u32> { return add64(~a, vec2<u32>(1u, 0u)); }
fn signedProduct(a: i32, b: i32) -> vec2<u32> {
    let value = product(u32(abs(a)), u32(abs(b)));
    if ((a < 0) != (b < 0)) { return negate64(value); }
    return value;
}
fn lessEqual64(a: vec2<u32>, b: vec2<u32>) -> bool {
    return a.y < b.y || (a.y == b.y && a.x <= b.x);
}
fn divide64(n: vec2<u32>, d: u32) -> u32 {
    if (n.y == 0u) { return n.x / d; }
    var remainder = n.y;
    var result = 0u;
    for (var bit = 31; bit >= 0; bit -= 1) {
        remainder = (remainder << 1u) | ((n.x >> u32(bit)) & 1u);
        if (remainder >= d) {
            remainder -= d;
            result |= 1u << u32(bit);
        }
    }
    return result;
}
fn interpolate(start: i32, delta: i32, along: u32, length: u32) -> i32 {
    let offset = i32(divide64(product(u32(abs(delta)), along), length));
    if (delta < 0) { return start - offset; }
    return start + offset;
}
fn inside(point: vec2<i32>, center: vec2<i32>, radius: u32) -> bool {
    let delta = vec2<u32>(abs(point - center));
    if (any(delta > vec2<u32>(radius))) { return false; }
    return lessEqual64(add64(product(delta.x, delta.x), product(delta.y, delta.y)), product(radius, radius));
}

fn reflected(point: vec2<i32>, reflection: u32) -> vec2<i32> {
    return select(point, parameters.mirror.xy * 2 - point,
                  vec2<bool>((reflection & 1u) != 0u, (reflection & 2u) != 0u));
}

fn fillInside(point: vec2<i32>) -> bool {
    var insidePolygon = false;
    var winding = 0;
    for (var i = 0u; i < arrayLength(&fillEdges); i += 1u) {
        let a = fillEdges[i].endpoints.xy;
        let b = fillEdges[i].endpoints.zw;
        if ((a.y > point.y) == (b.y > point.y)) { continue; }
        let cross = add64(signedProduct(b.x - a.x, point.y - a.y),
                          negate64(signedProduct(point.x - a.x, b.y - a.y)));
        let positive = i32(cross.y) >= 0 && any(cross != vec2<u32>(0u));
        let negative = i32(cross.y) < 0;
        if (b.y > a.y && positive) {
            insidePolygon = !insidePolygon;
            winding += 1;
        } else if (b.y < a.y && negative) {
            insidePolygon = !insidePolygon;
            winding -= 1;
        }
    }
    if ((bitcast<u32>(parameters.mirror.w) & 4u) != 0u) { return winding != 0; }
    return insidePolygon;
}

fn paintAlpha(destination: vec4<u32>, source: vec4<u32>, alpha: u32) -> vec4<u32> {
    if (alpha == 0u) { return destination; }
    if (parameters.brush.z != 0u) {
        return vec4<u32>(destination.xyz, mul255(destination.w, 255u - alpha));
    }
    if ((parameters.counts.w & 1u) != 0u) {
        return vec4<u32>((source.xyz * alpha + destination.xyz * (255u - alpha) + vec3<u32>(127u)) / 255u, destination.w);
    }
    return over(destination, vec4<u32>(source.xyz, alpha));
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let local = invocation.xy;
    if (any(local >= vec2<u32>(256u))) { return; }
    let index = local.y * 256u + local.x;
    if (parameters.counts.z != 0u) { outputPixels[index] = parameters.brush.x; return; }
    let pixel = parameters.tile.xy + vec2<i32>(local);
    if ((parameters.counts.w & 512u) != 0u) {
        outputPixels[index] = gaussianPixel(pixel, unpackPixel(basePixels[index]), unpackPixel(parameters.brush.x));
        return;
    }
    var mask = 0u;
    if ((parameters.counts.w & 128u) != 0u) { mask = previousCoverage[index]; }
    let mirrorMode = (parameters.counts.w >> 5u) & 3u;
    let figureFlags = bitcast<u32>(parameters.mirror.w);
    if ((figureFlags & 2u) == 0u) {
        for (var reflection = 0u; reflection < 4u && mask != 65535u; reflection += 1u) {
            if ((reflection & mirrorMode) != reflection) { continue; }
            let center = reflected(pixel * 16 + vec2<i32>(8), reflection);
            for (var s = 0u; s < parameters.counts.x && mask != 65535u; s += 1u) {
                let segment = Segment(bitcast<vec4<i32>>(geometryFour(s * 8u)), geometryFour(s * 8u + 4u));
                let maximumReach = i32(parameters.counts.y / 2u + 8u);
                if (any(center < min(segment.endpoints.xy, segment.endpoints.zw) - vec2<i32>(maximumReach)) ||
                    any(center > max(segment.endpoints.xy, segment.endpoints.zw) + vec2<i32>(maximumReach))) { continue; }
                let delta = segment.endpoints.zw - segment.endpoints.xy;
                let length = segment.values.w;
                let step = parameters.brush.w;
                let first = (step - segment.values.z) % step;
                var low = 0u;
                var high = 0u;
                if (length != 0u) {
                    let relative = center - segment.endpoints.xy;
                    let dotValue = add64(signedProduct(relative.x, delta.x), signedProduct(relative.y, delta.y));
                    var projection = 0u;
                    if (i32(dotValue.y) >= 0) {
                        if (lessEqual64(product(length, length), dotValue)) { projection = length; }
                        else { projection = divide64(dotValue, length); }
                    }
                    let reach = parameters.counts.y / 2u + 32u;
                    if (projection > reach + first) { low = (projection - reach - first) / step; }
                    high = min(length, projection + reach);
                    if (high < first) { continue; }
                    high = (high - first) / step;
                }
                for (var dab = low; dab <= high; dab += 1u) {
                    var along = first + dab * step;
                    if (length == 0u) { along = 0u; }
                    if (along > length) { break; }
                    var position = segment.endpoints.xy;
                    var pressure = segment.values.x;
                    if (length != 0u) {
                        position.x = interpolate(position.x, delta.x, along, length);
                        position.y = interpolate(position.y, delta.y, along, length);
                        pressure = u32(interpolate(i32(pressure), i32(segment.values.y) - i32(pressure), along, length));
                    }
                    let radius = divide64(product(parameters.counts.y, pressure), 131070u);
                    if (radius == 0u) { continue; }
                    for (var y = 0u; y < 4u; y += 1u) {
                        for (var x = 0u; x < 4u; x += 1u) {
                            let bit = 1u << (y * 4u + x);
                            if ((mask & bit) == 0u && inside(reflected(pixel * 16 + vec2<i32>(i32(x * 4u + 2u), i32(y * 4u + 2u)), reflection), position, radius)) { mask |= bit; }
                        }
                    }
                }
            }
        }
    }
    if ((parameters.counts.w & 256u) != 0u) { retainedCoverage[index] = mask; }
    var fillMask = 0u;
    if ((figureFlags & 1u) != 0u) {
        for (var reflection = 0u; reflection < 4u && fillMask != 65535u; reflection += 1u) {
            if ((reflection & mirrorMode) != reflection) { continue; }
            for (var y = 0u; y < 4u; y += 1u) {
                for (var x = 0u; x < 4u; x += 1u) {
                    let bit = 1u << (y * 4u + x);
                    let point = reflected(pixel * 16 + vec2<i32>(i32(x * 4u + 2u), i32(y * 4u + 2u)), reflection);
                    if ((fillMask & bit) == 0u && fillInside(point)) { fillMask |= bit; }
                }
            }
        }
    }
    var destination = unpackPixel(basePixels[index]);
    let strokeColor = unpackPixel(parameters.brush.x);
    if ((figureFlags & 1u) != 0u) {
        let fillColor = unpackPixel(bitcast<u32>(parameters.mirror.z));
        let fillAlpha = (fillColor.w * countOneBits(fillMask) + 8u) / 16u;
        let strokeAlpha = (strokeColor.w * countOneBits(mask) + 8u) / 16u;
        // The circular marker retains the first color at equal coverage.
        let source = select(fillColor, strokeColor, strokeAlpha > fillAlpha);
        destination = paintAlpha(destination, source, mul255(max(fillAlpha, strokeAlpha), parameters.brush.y));
    } else {
        let alpha = (mul255(strokeColor.w, parameters.brush.y) * countOneBits(mask) + 8u) / 16u;
        destination = paintAlpha(destination, strokeColor, alpha);
    }
    let writable = parameters.counts.w >> 1u;
    let original = unpackPixel(basePixels[index]);
    destination = select(original, destination, vec4<bool>((writable & 1u) != 0u,
        (writable & 2u) != 0u, (writable & 4u) != 0u, (writable & 8u) != 0u));
    outputPixels[index] = packPixel(destination);
}

// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct Parameters {
    tile: vec4<i32>,
    brush: vec4<u32>,
    counts: vec4<u32>,
    mirror: vec4<i32>,
    selection: vec4<i32>,
    pattern: vec4<u32>,
}
struct Segment { endpoints: vec4<i32>, values: vec4<u32>, }
struct FigureFillEdge { endpoints: vec4<i32>, mirroredRounding: vec4<i32>, }
@group(0) @binding(0) var<uniform> parameters: Parameters;
@group(0) @binding(1) var<storage, read> geometryWords: array<u32>;
@group(0) @binding(2) var<storage, read> basePixels: array<u32>;
@group(0) @binding(3) var<storage, read_write> outputPixels: array<u32>;
@group(0) @binding(4) var<storage, read> fillEdges: array<FigureFillEdge>;
@group(0) @binding(5) var<storage, read> previousCoverage: array<u32>;
@group(0) @binding(6) var<storage, read_write> retainedCoverage: array<u32>;
@group(0) @binding(7) var<storage, read> selectionCoverage: array<u32>;
@group(0) @binding(8) var<storage, read> patternPixels: array<u32>;

// Pattern pixels repeat from the document origin; negative document
// coordinates wrap into the same tile.
fn patternPixel(pixel: vec2<i32>) -> vec4<u32> {
    let width = max(parameters.pattern.x, 1u);
    let height = max(parameters.pattern.y, 1u);
    var x = pixel.x % i32(width);
    if (x < 0) { x += i32(width); }
    var y = pixel.y % i32(height);
    if (y < 0) { y += i32(height); }
    return unpackPixel(patternPixels[u32(y) * width + u32(x)]);
}

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

fn gaussianPixel(pixel: vec2<i32>, base: vec4<u32>, color: vec4<u32>, selectionAlpha: u32) -> u32 {
    // Binary selections keep the historical projection byte arithmetic exactly.
    let selectionScale = projectionDivide(projectionFloat(vec2<u32>(selectionAlpha, 0u), 0),
                                          projectionFloat(vec2<u32>(255u, 0u), 0));
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
        var eraseAlpha = mul255(alpha, opacityByte);
        if (selectionAlpha != 255u) {
            eraseAlpha = projectionByte(projectionMultiply(
                projectionFloat(vec2<u32>(eraseAlpha, 0u), 0), selectionScale));
        }
        return packPixel(vec4<u32>(base.xyz, mul255(base.w, 255u - eraseAlpha)));
    }
    var sourceAlpha = projectionScaleByte(opacity, alpha);
    if (selectionAlpha != 255u) { sourceAlpha = projectionMultiply(sourceAlpha, selectionScale); }
    if (parameters.pattern.w != 0u) {
        // Brush coverage uses the same separable blend as fill and composition.
        // Alpha lock and channel protection stay in the final write mask.
        let writable = parameters.counts.w >> 1u;
        let writeMask = select(writable, writable & 7u, (parameters.counts.w & 1u) != 0u);
        let blended = unpackPixel(separableProjectionPixel(base,
            vec4<u32>(color.xyz, projectionByte(sourceAlpha)), 255u, 15u, parameters.pattern.w));
        return packPixel(select(base, blended, vec4<bool>((writeMask & 1u) != 0u,
            (writeMask & 2u) != 0u, (writeMask & 4u) != 0u, (writeMask & 8u) != 0u)));
    }
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

// Signed 64-bit arithmetic keeps 26.6 endpoints valid across the document
// coordinate range. Cell intersections use Qt's floor division, followed by
// signed area >> 9 before absolute coverage (qtbase v6.11.1 qgrayraster.c).
fn cellSigned(value: i32) -> vec2<u32> {
    return vec2<u32>(bitcast<u32>(value), select(0u, 0xffffffffu, value < 0));
}
fn cellShift(value: vec2<u32>) -> vec2<u32> {
    return vec2<u32>(value.x << 2u, (value.y << 2u) | (value.x >> 30u));
}
fn cellMultiply(value: vec2<u32>, factor: u32) -> vec2<u32> {
    let low = product(value.x, factor);
    return vec2<u32>(low.x, low.y + value.y * factor);
}
fn cellLessEqual(a: vec2<u32>, b: vec2<u32>) -> bool {
    if (a.y != b.y) { return i32(a.y) < i32(b.y); }
    return a.x <= b.x;
}
fn cellFloorDivide(numerator: vec2<u32>, denominator: u32) -> vec2<u32> {
    let negative = i32(numerator.y) < 0;
    var magnitude = numerator;
    if (negative) { magnitude = negate64(magnitude); }
    var remainder = magnitude.y % denominator;
    var low = 0u;
    for (var bit = 31; bit >= 0; bit -= 1) {
        remainder = (remainder << 1u) | ((magnitude.x >> u32(bit)) & 1u);
        if (remainder >= denominator) {
            remainder -= denominator;
            low |= 1u << u32(bit);
        }
    }
    var quotient = vec2<u32>(low, magnitude.y / denominator);
    if (negative) {
        if (remainder != 0u) { quotient = add64(quotient, vec2<u32>(1u, 0u)); }
        quotient = negate64(quotient);
    }
    return quotient;
}
fn cellRowX(origin: vec2<u32>, yOrigin: i32, delta: vec2<i32>, y: i32) -> vec2<u32> {
    var numerator = add64(cellShift(signedProduct(-yOrigin, delta.x)), signedProduct(y, delta.x));
    if (delta.y < 0) { numerator = negate64(numerator); }
    return add64(cellShift(origin), cellFloorDivide(numerator, u32(abs(delta.y))));
}
// The quotient is at most one pixel (256 subpixels). Nine comparisons avoid
// narrowing a wide edge's denominator or its clipped intersection numerator.
fn cellHeight(along: vec2<u32>, width: vec2<u32>, height: i32) -> i32 {
    let numerator = cellMultiply(along, u32(abs(height)));
    var quotient = 0u;
    for (var bit = 8; bit >= 0; bit -= 1) {
        let candidate = quotient | (1u << u32(bit));
        if (lessEqual64(cellMultiply(width, candidate), numerator)) { quotient = candidate; }
    }
    if (height < 0) {
        if (any(cellMultiply(width, quotient) != numerator)) { quotient += 1u; }
        return -i32(quotient);
    }
    return i32(quotient);
}
fn fillEdgeArea(edge: FigureFillEdge, pixel: vec2<i32>, reflection: u32) -> i32 {
    let a = edge.endpoints.xy;
    let b = edge.endpoints.zw;
    var originX = cellSigned(a.x);
    var originY = cellSigned(a.y);
    var delta = b - a;
    if ((reflection & 1u) != 0u) {
        originX = add64(cellSigned(parameters.mirror.x * 8), negate64(originX));
        originX = add64(originX, cellSigned(edge.mirroredRounding.x));
        delta.x = -delta.x + edge.mirroredRounding.z - edge.mirroredRounding.x;
    }
    if ((reflection & 2u) != 0u) {
        originY = add64(cellSigned(parameters.mirror.y * 8), negate64(originY));
        originY = add64(originY, cellSigned(edge.mirroredRounding.y));
        delta.y = -delta.y + edge.mirroredRounding.w - edge.mirroredRounding.y;
    }
    originX = add64(originX, negate64(cellSigned(pixel.x * 64)));
    originY = add64(originY, negate64(cellSigned(pixel.y * 64)));
    let endY = add64(originY, cellSigned(delta.y));
    let rowStart = vec2<u32>(0u);
    let rowEnd = vec2<u32>(64u, 0u);
    if (delta.y == 0 || (cellLessEqual(originY, rowStart) && cellLessEqual(endY, rowStart)) ||
        (cellLessEqual(rowEnd, originY) && cellLessEqual(rowEnd, endY))) { return 0; }
    // An intersecting edge's relative y endpoints fit its bounded signed delta.
    let yOrigin = i32(originY.x);
    let y1 = clamp(yOrigin, 0, 64) * 4;
    let y2 = clamp(i32(endY.x), 0, 64) * 4;
    let x1 = cellRowX(originX, yOrigin, delta, y1);
    let x2 = cellRowX(originX, yOrigin, delta, y2);
    let zero = vec2<u32>(0u);
    let right = vec2<u32>(256u, 0u);
    if (cellLessEqual(x1, zero) && cellLessEqual(x2, zero)) { return 512 * (y2 - y1); }
    if (cellLessEqual(right, x1) && cellLessEqual(right, x2)) { return 0; }
    if (cellLessEqual(x1, x2)) {
        let width = add64(x2, negate64(x1));
        var ya = y1;
        var yb = y2;
        var xa = i32(x1.x);
        var xb = i32(x2.x);
        if (cellLessEqual(x1, zero)) {
            ya += cellHeight(negate64(x1), width, y2 - y1);
            xa = 0;
        }
        if (cellLessEqual(right, x2)) {
            yb = y1 + cellHeight(add64(right, negate64(x1)), width, y2 - y1);
            xb = 256;
        }
        return 512 * (yb - y1) - (xa + xb) * (yb - ya);
    }
    let width = add64(x1, negate64(x2));
    var ya = y1;
    var yb = y2;
    var xa = i32(x1.x);
    var xb = i32(x2.x);
    if (cellLessEqual(right, x1)) {
        ya += cellHeight(add64(x1, negate64(right)), width, y2 - y1);
        xa = 256;
    }
    if (cellLessEqual(x2, zero)) {
        yb = y1 + cellHeight(x1, width, y2 - y1);
        xb = 0;
    }
    return 512 * (y2 - ya) - (xa + xb) * (yb - ya);
}
fn fillPixelCoverage(pixel: vec2<i32>, reflection: u32) -> u32 {
    var area = vec2<u32>(0u);
    for (var i = 0u; i < arrayLength(&fillEdges); i += 1u) {
        area = add64(area, cellSigned(fillEdgeArea(fillEdges[i], pixel, reflection)));
    }
    // Arithmetic right shift of a negative area rounds down before abs.
    var magnitude = area;
    if (i32(area.y) < 0) { magnitude = add64(negate64(area), vec2<u32>(511u, 0u)); }
    if ((bitcast<u32>(parameters.mirror.w) & 4u) == 0u) {
        let coverage = (magnitude.x >> 9u) & 511u;
        if (coverage > 256u) { return 512u - coverage; }
        return min(coverage, 255u);
    }
    if (magnitude.y != 0u) { return 255u; }
    return min(magnitude.x >> 9u, 255u);
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
    let selection = parameters.selection;
    let maskSelection = (parameters.counts.w & 4096u) != 0u;
    let softSelection = (parameters.counts.w & 8192u) != 0u;
    var selectionAlpha = 255u;
    if (maskSelection || (selection.z > 0 && selection.w > 0)) {
        var inside = false;
        if (maskSelection && softSelection) {
            // Four alpha bytes per word; the renderer binds the frozen coverage tile.
            let coverageIndex = index;
            selectionAlpha = (selectionCoverage[coverageIndex >> 2u] >> ((coverageIndex & 3u) * 8u)) & 255u;
        } else if (maskSelection) {
            // One bit per tile pixel; the renderer binds the frozen coverage tile.
            let coverageIndex = index;
            inside = ((selectionCoverage[coverageIndex >> 5u] >> (coverageIndex & 31u)) & 1u) != 0u;
        } else {
            let outside = pixel.x < selection.x || pixel.y < selection.y ||
                pixel.x >= selection.x + selection.z || pixel.y >= selection.y + selection.w;
            inside = !outside;
            if ((parameters.counts.w & 2048u) != 0u) {
                let center = vec2<f32>(f32(selection.x) + f32(selection.z) * 0.5,
                                       f32(selection.y) + f32(selection.w) * 0.5);
                let radius = vec2<f32>(f32(selection.z) * 0.5, f32(selection.w) * 0.5);
                let relative = (vec2<f32>(f32(pixel.x) + 0.5, f32(pixel.y) + 0.5) - center) / radius;
                inside = dot(relative, relative) <= 1.0;
            }
        }
        let inverted = (parameters.counts.w & 1024u) != 0u;
        if (maskSelection && softSelection) {
            selectionAlpha = select(selectionAlpha, 255u - selectionAlpha, inverted);
            if (selectionAlpha == 0u) {
                outputPixels[index] = basePixels[index];
                return;
            }
        } else {
            var masked = !inside;
            if (inverted) { masked = inside; }
            if (masked) {
                outputPixels[index] = basePixels[index];
                return;
            }
        }
    }
    if ((parameters.counts.w & 512u) != 0u) {
        outputPixels[index] = gaussianPixel(pixel, unpackPixel(basePixels[index]), unpackPixel(parameters.brush.x), selectionAlpha);
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
    var fillCoverage = 0u;
    if ((figureFlags & 1u) != 0u) {
        for (var reflection = 0u; reflection < 4u; reflection += 1u) {
            if ((reflection & mirrorMode) != reflection) { continue; }
            let coverage = fillPixelCoverage(pixel, reflection);
            fillCoverage = 255u - mul255(255u - fillCoverage, 255u - coverage);
        }
    }
    var destination = unpackPixel(basePixels[index]);
    let strokeColor = unpackPixel(parameters.brush.x);
    let strokeBlendMode = parameters.pattern.w;
    var writable = parameters.counts.w >> 1u;
    if ((parameters.counts.w & 1u) != 0u && parameters.brush.z == 0u) { writable &= 7u; }
    if ((figureFlags & 1u) != 0u) {
        var fillColor = unpackPixel(bitcast<u32>(parameters.mirror.z));
        if ((figureFlags & 8u) != 0u) { fillColor = patternPixel(pixel); }
        let fillAlpha = mul255(mul255(fillColor.w, fillCoverage), selectionAlpha);
        let strokeAlpha = mul255((strokeColor.w * countOneBits(mask) + 8u) / 16u, selectionAlpha);
        // The circular marker retains the first color at equal coverage.
        let source = select(fillColor, strokeColor, strokeAlpha > fillAlpha);
        let alpha = mul255(max(fillAlpha, strokeAlpha), parameters.brush.y);
        // Fill-only figures may freeze a separable blend mode. Masking writable
        // channels after blending preserves alpha lock and hidden RGB exactly
        // like the normal fill path below.
        if (parameters.pattern.z != 0u && parameters.brush.z == 0u) {
            destination = unpackPixel(separableProjectionPixel(
                destination, vec4<u32>(source.xyz, alpha), 255u, 15u, parameters.pattern.z));
        } else if (strokeBlendMode != 0u && parameters.brush.z == 0u && strokeAlpha > fillAlpha) {
            // A blended outline uses its own coverage; the fill keeps its
            // separate fill blend mode or source-over fallback.
            destination = unpackPixel(separableProjectionPixel(
                destination, vec4<u32>(strokeColor.xyz, alpha), 255u, 15u, strokeBlendMode));
        } else {
            destination = paintAlpha(destination, source, alpha);
        }
    } else {
        let alpha = mul255((mul255(strokeColor.w, parameters.brush.y) * countOneBits(mask) + 8u) / 16u, selectionAlpha);
        if (strokeBlendMode != 0u) {
            destination = unpackPixel(separableProjectionPixel(
                destination, vec4<u32>(strokeColor.xyz, alpha), 255u, 15u, strokeBlendMode));
        } else {
            destination = paintAlpha(destination, strokeColor, alpha);
        }
    }
    let original = unpackPixel(basePixels[index]);
    destination = select(original, destination, vec4<bool>((writable & 1u) != 0u,
        (writable & 2u) != 0u, (writable & 4u) != 0u, (writable & 8u) != 0u));
    outputPixels[index] = packPixel(destination);
}

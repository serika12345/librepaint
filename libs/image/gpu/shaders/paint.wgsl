// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct Parameters {
    origin: vec2<i32>,
    color: u32,
    canvasOpacityBits: u32,
    opacityValue: u32,
    erase: u32,
    count: u32,
    paintFlags: u32,
    mirror: vec4<i32>,
    selection: vec4<i32>,
    pattern: vec4<u32>,
    native: vec4<u32>,
}
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


fn paintBase(index: u32) -> vec4<u32> {
    let wide = parameters.native.z != 0u;
    let stride = select(1u, 2u, wide);
    let word = index * stride;
    return nativePixel(basePixels[word], basePixels[word + stride - 1u], wide);
}
fn writePaintPixel(index: u32, pixel: vec4<u32>) {
    let wide = parameters.native.z != 0u;
    let word = index * select(1u, 2u, wide);
    let packed = nativePixelWords(pixel, wide);
    outputPixels[word] = packed.x;
    if (wide) { outputPixels[word + 1u] = packed.y; }
}

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
struct MarkerMaskDab {
    center: vec4<u32>,
    circle: vec4<u32>,
    bounds: vec4<i32>,
}
fn markerDab(index: u32) -> MarkerMaskDab {
    let offset = index * 12u;
    return MarkerMaskDab(geometryFour(offset), geometryFour(offset + 4u), bitcast<vec4<i32>>(geometryFour(offset + 8u)));
}
fn markerInteger(value: i32) -> Word64 {
    let negative = value < 0;
    let magnitude = select(bitcast<u32>(value), 0u - bitcast<u32>(value), negative);
    return doubleFloat(Word64(magnitude, 0u), 0, select(0u, 0x80000000u, negative));
}
fn markerCoverage(dab: MarkerMaskDab, pixel: vec2<i32>) -> u32 {
    if (any(pixel < dab.bounds.xy) || any(pixel >= dab.bounds.xy + dab.bounds.zw)) { return 0u; }
    let dx = doubleSubtract(markerInteger(pixel.x), dab.center.xy);
    let dy = doubleSubtract(markerInteger(pixel.y), dab.center.zw);
    let distance = doubleAdd(doubleMultiply(dx, dx), doubleMultiply(dy, dy));
    let fade = doubleMultiply(doubleSubtract(distance, dab.circle.xy), dab.circle.zw);
    if ((fade.y & 0x80000000u) != 0u || all(fade == Word64(0u))) { return 255u; }
    let one = Word64(0u, 0x3ff00000u);
    if (!wordLessEqual(fade, one)) { return 0u; }
    return min(255u, doubleTruncate(doubleMultiply(doubleSubtract(one, fade), doubleInteger(255u))));
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
    let opacityValue = floatMultiply(0x437f0000u, opacity);
    let averageByte = floatMultiply(0x437f0000u, average);
    let mask = floatMultiply(bitcast<u32>(f32(coverage)), 0x3b808081u);
    let source = floatMultiply(mask, opacityValue);
    if (source == 0u) { return destination; }
    let current = bitcast<u32>(f32(destination));
    var fullFlow = current;
    if (average > opacity) {
        if (averageByte > current) {
            fullFlow = floatAdd(floatMultiply(floatSubtract(averageByte, source), floatDivide(current, averageByte)), source);
        }
    } else if (opacityValue > current) {
        fullFlow = floatAdd(floatMultiply(floatSubtract(opacityValue, current), mask), current);
    }
    if (flow == 0x3f800000u) { return projectionComponent(fullFlow); }
    var zeroFlow = current;
    if (values.w == 0u) {
        zeroFlow = floatSubtract(floatAdd(source, current), floatMultiply(floatMultiply(source, current), 0x3b808081u));
    }
    return projectionComponent(floatAdd(floatMultiply(floatSubtract(fullFlow, zeroFlow), flow), zeroFlow));
}

fn gaussianCoverage(pixel: vec2<i32>) -> u32 {
    var alpha = 0u;
    for (var index = 0u; index < parameters.count; index += 1u) {
        let dab = gaussianDab(index);
        let point = pixel - bitcast<vec2<i32>>(dab.coefficient.zw);
        if (any(point < vec2<i32>(0)) || any(point >= vec2<i32>(dab.size.xy))) { continue; }
        var coverage: u32;
        if (dab.size.z > 1u) { coverage = smallCircleAlpha(dab, vec2<u32>(point)); }
        else { coverage = circleAlpha(dab, vec2<u32>(point)); }
        alpha = washAlpha(alpha, coverage, dab.application);
    }
    return alpha;
}


fn cellNegate(a: vec2<u32>) -> vec2<u32> { return wordAdd(~a, vec2<u32>(1u, 0u)); }
fn cellProductSigned(a: i32, b: i32) -> vec2<u32> {
    let value = wordProduct(u32(abs(a)), u32(abs(b)));
    if ((a < 0) != (b < 0)) { return cellNegate(value); }
    return value;
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
    let low = wordProduct(value.x, factor);
    return vec2<u32>(low.x, low.y + value.y * factor);
}
fn cellLessEqual(a: vec2<u32>, b: vec2<u32>) -> bool {
    if (a.y != b.y) { return i32(a.y) < i32(b.y); }
    return a.x <= b.x;
}
fn cellFloorDivide(numerator: vec2<u32>, denominator: u32) -> vec2<u32> {
    let negative = i32(numerator.y) < 0;
    var magnitude = numerator;
    if (negative) { magnitude = cellNegate(magnitude); }
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
        if (remainder != 0u) { quotient = wordAdd(quotient, vec2<u32>(1u, 0u)); }
        quotient = cellNegate(quotient);
    }
    return quotient;
}
fn cellRowX(origin: vec2<u32>, yOrigin: i32, delta: vec2<i32>, y: i32) -> vec2<u32> {
    var numerator = wordAdd(cellShift(cellProductSigned(-yOrigin, delta.x)), cellProductSigned(y, delta.x));
    if (delta.y < 0) { numerator = cellNegate(numerator); }
    return wordAdd(cellShift(origin), cellFloorDivide(numerator, u32(abs(delta.y))));
}
// The quotient is at most one pixel (256 subpixels). Nine comparisons avoid
// narrowing a wide edge's denominator or its clipped intersection numerator.
fn cellHeight(along: vec2<u32>, width: vec2<u32>, height: i32) -> i32 {
    let numerator = cellMultiply(along, u32(abs(height)));
    var quotient = 0u;
    for (var bit = 8; bit >= 0; bit -= 1) {
        let candidate = quotient | (1u << u32(bit));
        if (wordLessEqual(cellMultiply(width, candidate), numerator)) { quotient = candidate; }
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
        originX = wordAdd(cellSigned(parameters.mirror.x * 8), cellNegate(originX));
        originX = wordAdd(originX, cellSigned(edge.mirroredRounding.x));
        delta.x = -delta.x + edge.mirroredRounding.z - edge.mirroredRounding.x;
    }
    if ((reflection & 2u) != 0u) {
        originY = wordAdd(cellSigned(parameters.mirror.y * 8), cellNegate(originY));
        originY = wordAdd(originY, cellSigned(edge.mirroredRounding.y));
        delta.y = -delta.y + edge.mirroredRounding.w - edge.mirroredRounding.y;
    }
    originX = wordAdd(originX, cellNegate(cellSigned(pixel.x * 64)));
    originY = wordAdd(originY, cellNegate(cellSigned(pixel.y * 64)));
    let endY = wordAdd(originY, cellSigned(delta.y));
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
        let width = wordAdd(x2, cellNegate(x1));
        var ya = y1;
        var yb = y2;
        var xa = i32(x1.x);
        var xb = i32(x2.x);
        if (cellLessEqual(x1, zero)) {
            ya += cellHeight(cellNegate(x1), width, y2 - y1);
            xa = 0;
        }
        if (cellLessEqual(right, x2)) {
            yb = y1 + cellHeight(wordAdd(right, cellNegate(x1)), width, y2 - y1);
            xb = 256;
        }
        return 512 * (yb - y1) - (xa + xb) * (yb - ya);
    }
    let width = wordAdd(x1, cellNegate(x2));
    var ya = y1;
    var yb = y2;
    var xa = i32(x1.x);
    var xb = i32(x2.x);
    if (cellLessEqual(right, x1)) {
        ya += cellHeight(wordAdd(x1, cellNegate(right)), width, y2 - y1);
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
        area = wordAdd(area, cellSigned(fillEdgeArea(fillEdges[i], pixel, reflection)));
    }
    // Arithmetic right shift of a negative area rounds down before abs.
    var magnitude = area;
    if (i32(area.y) < 0) { magnitude = wordAdd(cellNegate(area), vec2<u32>(511u, 0u)); }
    if ((bitcast<u32>(parameters.mirror.w) & 4u) == 0u) {
        let coverage = (magnitude.x >> 9u) & 511u;
        if (coverage > 256u) { return 512u - coverage; }
        return min(coverage, 255u);
    }
    if (magnitude.y != 0u) { return 255u; }
    return min(magnitude.x >> 9u, 255u);
}


@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let local = invocation.xy;
    if (any(local >= vec2<u32>(256u))) { return; }
    let index = local.y * 256u + local.x;
    if ((parameters.paintFlags & 16384u) != 0u) {
        writePaintPixel(index, nativePixel(parameters.color, parameters.native.x, parameters.native.z != 0u));
        return;
    }
    let pixel = parameters.origin + vec2<i32>(local);
    let selection = parameters.selection;
    let maskSelection = (parameters.paintFlags & 4096u) != 0u;
    let softSelection = (parameters.paintFlags & 8192u) != 0u;
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
            if ((parameters.paintFlags & 2048u) != 0u) {
                let center = vec2<f32>(f32(selection.x) + f32(selection.z) * 0.5,
                                       f32(selection.y) + f32(selection.w) * 0.5);
                let radius = vec2<f32>(f32(selection.z) * 0.5, f32(selection.w) * 0.5);
                let relative = (vec2<f32>(f32(pixel.x) + 0.5, f32(pixel.y) + 0.5) - center) / radius;
                inside = dot(relative, relative) <= 1.0;
            }
        }
        let inverted = (parameters.paintFlags & 1024u) != 0u;
        if (maskSelection && softSelection) {
            selectionAlpha = select(selectionAlpha, 255u - selectionAlpha, inverted);
            if (selectionAlpha == 0u) {
                writePaintPixel(index, paintBase(index));
                return;
            }
        } else {
            var masked = !inside;
            if (inverted) { masked = inside; }
            if (masked) {
                writePaintPixel(index, paintBase(index));
                return;
            }
        }
    }
    var strokeCoverage = 0u;
    if ((parameters.paintFlags & 512u) != 0u) {
        strokeCoverage = gaussianCoverage(pixel);
    } else {
        if ((parameters.paintFlags & 128u) != 0u) { strokeCoverage = previousCoverage[index]; }
        if ((bitcast<u32>(parameters.mirror.w) & 2u) == 0u) {
            for (var i = 0u; i < parameters.count && strokeCoverage != 255u; i += 1u) {
                strokeCoverage = max(strokeCoverage, markerCoverage(markerDab(i), pixel));
            }
        }
        if ((parameters.paintFlags & 256u) != 0u) { retainedCoverage[index] = strokeCoverage; }
    }
    let figureFlags = bitcast<u32>(parameters.mirror.w);
    let mirrorMode = (parameters.paintFlags >> 5u) & 3u;
    var fillCoverage = 0u;
    if ((figureFlags & 1u) != 0u) {
        for (var reflection = 0u; reflection < 4u; reflection += 1u) {
            if ((reflection & mirrorMode) != reflection) { continue; }
            let coverage = fillPixelCoverage(pixel, reflection);
            fillCoverage = 255u - mul255(255u - fillCoverage, 255u - coverage);
        }
    }
    let base = paintBase(index);
    var source = nativePixel(parameters.color, parameters.native.x, parameters.native.z != 0u).xyz;
    var coverage = strokeCoverage;
    var blendMode = parameters.pattern.w;
    if ((figureFlags & 1u) != 0u) {
        var fillColor = nativePixel(bitcast<u32>(parameters.mirror.z), parameters.native.y, parameters.native.z != 0u);
        if ((figureFlags & 8u) != 0u) { fillColor = patternPixel(pixel); }
        let fillAlpha = mul255(fillColor.w, fillCoverage);
        // Native marker writes retain the fill color at equal temporary alpha.
        if (fillAlpha >= coverage) {
            source = fillColor.xyz;
            coverage = fillAlpha;
            blendMode = parameters.pattern.z;
        }
    }
    var writable = (parameters.paintFlags >> 1u) & 15u;
    if ((parameters.paintFlags & 1u) != 0u && parameters.erase == 0u) { writable &= 7u; }
    if (parameters.erase != 0u) {
        var resultAlpha: u32;
        if (parameters.native.z != 0u) {
            let masked = mul65535(coverage * 257u, selectionAlpha * 257u);
            let alpha = mul65535(masked, parameters.opacityValue);
            resultAlpha = mul65535(base.w, 65535u - alpha);
        } else {
            let alpha = mul255(mul255(coverage, selectionAlpha), parameters.opacityValue);
            resultAlpha = mul255(base.w, 255u - alpha);
        }
        // Native erase applies alpha independently of protected color channels.
        writePaintPixel(index, vec4<u32>(base.xyz, resultAlpha));
    } else if (blendMode == 0u) {
        var sourceAlpha: u32;
        if (parameters.native.z != 0u) {
            sourceAlpha = projectionMultiply(nativeNormalizedAlpha(coverage * 257u), parameters.canvasOpacityBits);
        } else {
            sourceAlpha = projectionScaleByte(parameters.canvasOpacityBits, coverage);
        }
        if (selectionAlpha != 255u) {
            sourceAlpha = projectionMultiply(sourceAlpha, projectionScaleByte(0x3b808081u, selectionAlpha));
        }
        if (parameters.native.z != 0u) {
            writePaintPixel(index, normalRgba16(base, source, sourceAlpha, writable));
        } else {
            writePaintPixel(index, unpackPixel(normalRgba8(base, source, sourceAlpha, writable)));
        }
    } else {
        let alpha = mul255(mul255(coverage, selectionAlpha), parameters.opacityValue);
        let blended = unpackPixel(separableProjectionPixel(base, vec4<u32>(source, alpha), 255u, 15u, blendMode));
        writePaintPixel(index, select(base, blended, vec4<bool>((writable & 1u) != 0u,
            (writable & 2u) != 0u, (writable & 4u) != 0u, (writable & 8u) != 0u)));
    }
}

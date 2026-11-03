// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct Parameters {
    origin: vec2<i32>,
    color: u32,
    canvasOpacity: f32,
    opacityValue: u32,
    erase: u32,
    count: u32,
    paintFlags: u32,
    mirror: vec4<i32>,
    selection: vec4<i32>,
    pattern: vec4<u32>,
    native: vec4<u32>,
    composition: vec4<i32>,
    destination: vec4<i32>,
    patternMap0: vec4<f32>,
    patternMap1: vec4<f32>,
    patternSampling: vec4<u32>,
    texture: vec4<i32>,
    textureStorage: vec4<u32>,
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
    let pixel = nativePixel(basePixels[word], basePixels[word + stride - 1u], wide);
    if ((parameters.paintFlags & 524288u) != 0u) { return vec4<u32>(vec3<u32>(pixel.w), 255u); }
    return pixel;
}
fn writePaintPixel(index: u32, pixel: vec4<u32>) {
    let wide = parameters.native.z != 0u;
    let word = index * select(1u, 2u, wide);
    var result = pixel;
    if ((parameters.paintFlags & 524288u) != 0u) {
        let alpha = mul255(pixel.x, pixel.w);
        result = select(vec4<u32>(), vec4<u32>(255u, 255u, 255u, alpha), alpha != 0u);
    }
    let packed = nativePixelWords(result, wide);
    outputPixels[word] = packed.x;
    if (wide) { outputPixels[word + 1u] = packed.y; }
}

// Pattern pixels repeat from the document origin; negative document
// coordinates wrap into the same tile.
fn patternTexel(pixel: vec2<i32>) -> vec4<u32> {
    let width = max(parameters.pattern.x, 1u);
    let height = max(parameters.pattern.y, 1u);
    var x = pixel.x % i32(width);
    if (x < 0) { x += i32(width); }
    var y = pixel.y % i32(height);
    if (y < 0) { y += i32(height); }
    let wide = parameters.patternSampling.x != 0u;
    let stride = select(1u, 2u, wide);
    let index = (u32(y) * width + u32(x)) * stride;
    var color = nativePixel(patternPixels[index], patternPixels[index + stride - 1u], wide);
    if (!wide && parameters.native.z != 0u) { color *= 257u; }
    return color;
}
struct PatternCoordinate { pixel: i32, fraction: f32, }
fn patternCoordinate(value: f32, period: u32) -> PatternCoordinate {
    let integral = floor(value);
    var wrapped = i32(integral % f32(period));
    if (wrapped < 0) { wrapped += i32(period); }
    return PatternCoordinate(wrapped, value - integral);
}
fn patternQuotient(total: f32, denominator: u32, maximum: u32) -> u32 {
    return u32(clamp(floor(total / f32(denominator) + 0.5), 0.0, f32(maximum)));
}
fn patternPixel(pixel: vec2<i32>) -> vec4<u32> {
    let x = f32(pixel.x); let y = f32(pixel.y);
    let sx = (((parameters.patternMap0.x * x) + (parameters.patternMap0.z * y)) + parameters.patternMap1.x);
    let sy = (((parameters.patternMap0.y * x) + (parameters.patternMap0.w * y)) + parameters.patternMap1.y);
    let mappedX = patternCoordinate(sx, parameters.pattern.x);
    let mappedY = patternCoordinate(sy, parameters.pattern.y);
    let left = mappedX.pixel; let top = mappedY.pixel;
    if (parameters.patternSampling.y == 0u) { return patternTexel(vec2<i32>(left, top)); }
    let hx = mappedX.fraction; let vy = mappedY.fraction;
    let one = 1.0; let half = 0.5;
    let complements = array<f32, 2>((one - hx), (one - vy));
    let horizontal = array<f32, 2>(complements[0], hx);
    let vertical = array<f32, 2>(complements[1], vy);
    var totals = array<f32, 3>(0.0, 0.0, 0.0);
    var alpha = 0u; var weightSum = 0u;
    for (var row = 0u; row < 2u; row++) {
        for (var column = 0u; column < 2u; column++) {
            // Native subpixel access rounds each independent weight, then
            // normalizes by their actual sum (which can differ from 255).
            let weight = u32(max((((horizontal[column] * vertical[row]) * f32(255u)) + half), 0.0));
            let color = patternTexel(vec2<i32>(left + i32(column), top + i32(row)));
            let amount = color.w * weight;
            weightSum += weight; alpha += amount;
            for (var channel = 0u; channel < 3u; channel++) {
                totals[channel] = (totals[channel] + (f32(color[channel]) * f32(amount)));
            }
        }
    }
    if (alpha == 0u) { return vec4<u32>(0u); }
    let maximum = select(255u, 65535u, parameters.native.z != 0u);
    return vec4<u32>(patternQuotient(totals[0], alpha, maximum), patternQuotient(totals[1], alpha, maximum),
                     patternQuotient(totals[2], alpha, maximum), min(maximum, (alpha + weightSum / 2u) / weightSum));
}

fn geometryFour(index: u32) -> vec4<u32> {
    return vec4<u32>(geometryWords[index], geometryWords[index + 1u], geometryWords[index + 2u], geometryWords[index + 3u]);
}
struct MarkerMaskDab {
    center: vec2<f32>,
    circle: vec2<f32>,
    bounds: vec4<i32>,
}
fn markerDab(index: u32) -> MarkerMaskDab {
    let offset = index * 8u;
    let values = bitcast<vec4<f32>>(geometryFour(offset));
    return MarkerMaskDab(values.xy, values.zw, bitcast<vec4<i32>>(geometryFour(offset + 4u)));
}

fn markerCoverage(dab: MarkerMaskDab, pixel: vec2<i32>) -> u32 {
    if (any(pixel < dab.bounds.xy) || any(pixel >= dab.bounds.xy + dab.bounds.zw)) { return 0u; }
    let delta = vec2<f32>(pixel - dab.bounds.xy) - dab.center;
    let fade = (dot(delta, delta) - dab.circle.x) * dab.circle.y;
    if (fade <= 0.0) { return 255u; }
    if (fade >= 1.0) { return 0u; }
    return u32(255.0 * (1.0 - fade));
}

fn gaussianDab(index: u32) -> MaskDab {
    let offset = index * 32u;
    let coordinates = bitcast<vec4<f32>>(geometryFour(offset + 4u));
    let shape = geometryFour(offset + 8u);
    let application = geometryFour(offset + 16u);
    return MaskDab(geometryFour(offset), coordinates.xy, coordinates.zw,
        bitcast<vec2<f32>>(shape.xy), shape.zw, bitcast<vec2<i32>>(geometryFour(offset + 12u).xy),
        BrushApplication(bitcast<f32>(application.x), bitcast<f32>(application.y), bitcast<f32>(application.z), application.w != 0u),
        geometryFour(offset + 20u), geometryFour(offset + 24u),
        bitcast<vec4<i32>>(geometryFour(offset + 28u)));
}

fn paperMultiply(a: u32, b: u32, maximum: u32) -> u32 {
    if (maximum == 65535u) { return mul65535(a, b); }
    return multiplyByte(a, b);
}
fn paperCoverage(dab: MaskDab, pixel: vec2<i32>, coverage: u32, maximum: u32) -> u32 {
    if (parameters.texture.x == 0) { return coverage; }
    let period = parameters.texture.xy;
    var sourcePixel = pixel;
    if ((dab.texture.w & 1u) != 0u) { sourcePixel.x = parameters.mirror.x / 8 - 1 - sourcePixel.x; }
    if ((dab.texture.w & 2u) != 0u) { sourcePixel.y = parameters.mirror.y / 8 - 1 - sourcePixel.y; }
    sourcePixel += dab.paperOffset.xy;
    var point = (sourcePixel - parameters.texture.zw) % period;
    if (point.x < 0) { point.x += period.x; }
    if (point.y < 0) { point.y += period.y; }
    let index = u32(point.y * period.x + point.x);
    let byte = (patternPixels[parameters.textureStorage.x + (index >> 2u)] >> ((index & 3u) * 8u)) & 255u;
    let source = byte * select(1u, 257u, maximum == 65535u);
    let strength = select(dab.texture.x, dab.texture.y, maximum == 65535u);
    let soft = (dab.texture.z & 256u) != 0u;
    let mode = dab.texture.z & 255u;
    if (mode == 1u) {
        let removed = select(source + maximum - strength, paperMultiply(source, strength, maximum), soft);
        return coverage - min(coverage, removed);
    }
    if (soft) {
        let effective = maximum - paperMultiply(maximum - source, strength, maximum);
        if (mode == 2u) { return min(effective, coverage); }
        return paperMultiply(effective, coverage, maximum);
    }
    if (mode == 2u) { return min(source, paperMultiply(coverage, strength, maximum)); }
    if (maximum == 65535u) { return multiplyThreeWords(source, coverage, strength); }
    return multiplyThreeBytes(source, coverage, strength);
}

fn washAlpha(destination: u32, coverage: u32, values: BrushApplication) -> u32 {
    let flow = values.flow;
    var opacity = values.opacity;
    var average = values.average;
    if (!values.creamy) {
        opacity = (opacity * flow);
        average = (average * flow);
    }
    let opacityValue = (255.0 * opacity);
    let averageByte = (255.0 * average);
    let mask = (f32(coverage) * 0.00392156886);
    let source = (mask * opacityValue);
    if (source == 0.0) { return destination; }
    let current = f32(destination);
    var fullFlow = current;
    if (average > opacity) {
        if (averageByte > current) {
            fullFlow = (((averageByte - source) * safeRatio(current, averageByte)) + source);
        }
    } else if (opacityValue > current) {
        fullFlow = (((opacityValue - current) * mask) + current);
    }
    if (flow == 1.0) { return roundComponent(fullFlow); }
    var zeroFlow = current;
    if (!values.creamy) {
        zeroFlow = ((source + current) - ((source * current) * 0.00392156886));
    }
    return roundComponent((((fullFlow - zeroFlow) * flow) + zeroFlow));
}

// Optimized native U16 WASH normalizes stored words, interpolates in
// binary32, then rounds each retained dab back to a word.
fn maskWashAlpha(destination: u32, coverage: u32, values: BrushApplication) -> u32 {
    let flow = values.flow;
    var opacity = values.opacity;
    var average = values.average;
    if (!values.creamy) {
        opacity = (opacity * flow);
        average = (average * flow);
    }
    // Generic GrayA8 WASH quantizes its settings before each dab and uses
    // integer interpolation. RGB keeps binary32 intermediates instead.
    let opacityByte = u32(((opacity * 255.0) + 0.5));
    let averageByte = u32(((average * 255.0) + 0.5));
    let source = multiplyByte(coverage, opacityByte);
    var full = destination;
    if (averageByte > opacityByte) {
        if (averageByte > destination) { full = blendByte(source, averageByte, divideByte(destination, averageByte)); }
    } else if (opacityByte > destination) { full = blendByte(destination, opacityByte, coverage); }
    if (flow == 1.0) { return full; }
    var zero = destination;
    if (!values.creamy) { zero = source + destination - multiplyByte(source, destination); }
    let flowByte = u32(((flow * 255.0) + 0.5));
    return blendByte(zero, full, flowByte);
}

fn washWordAlpha(destination: u32, coverage: u32, values: BrushApplication) -> u32 {
    let flow = values.flow;
    var opacity = values.opacity;
    var average = values.average;
    if (!values.creamy) {
        opacity = (opacity * flow);
        average = (average * flow);
    }
    let mask = nativeNormalizedAlpha(coverage);
    let current = nativeNormalizedAlpha(destination);
    let source = (mask * opacity);
    var full = current;
    if (average > opacity) {
        if (average > current) {
            full = ((average - source) * safeRatio(current, average) + source);
        }
    } else if (opacity > current) {
        full = ((opacity - current) * mask + current);
    }
    var alpha = full;
    if (flow != 1.0) {
        var zero = current;
        if (!values.creamy) { zero = (-source * current + (source + current)); }
        alpha = ((full - zero) * flow + zero);
    }
    return roundComponent((alpha * 65535.0));
}

fn generatedDabCoverage(dab: MaskDab, pixel: vec2<i32>, local: vec2<i32>, maximum: u32, gray: bool) -> u32 {
    if ((dab.size.w & 64u) != 0u) { return maximum; }
    var point = local;
    let scale = select(1u, 257u, maximum == 65535u);
    if ((dab.size.w & 4u) != 0u) { point.x = i32(dab.size.x) - 1 - point.x; }
    if ((dab.size.w & 8u) != 0u) { point.y = i32(dab.size.y) - 1 - point.y; }
    var coverage: u32;
    if ((dab.size.w & 256u) != 0u) { coverage = imageBrushAlpha(dab, vec2<u32>(point)) * scale; }
    else if ((dab.size.w & 1u) != 0u) {
        if (dab.size.z > 1u) { coverage = smallStandardCircleAlpha(dab, vec2<u32>(point)) * scale; }
        else { coverage = standardCircleAlpha(dab, vec2<u32>(point), maximum, gray); }
    } else if ((dab.size.w & 32u) != 0u) {
        coverage = unaliasedGaussianAlpha(dab, vec2<u32>(point), maximum, gray);
    } else if (dab.size.z > 1u) { coverage = smallCircleAlpha(dab, vec2<u32>(point)) * scale; }
    else { coverage = circleAlpha(dab, vec2<u32>(point), maximum, gray); }
    if ((dab.texture.z & 512u) != 0u) {
        let opacity = select(coverage, (coverage - (coverage >> 8u) + 128u) >> 8u, maximum == 65535u);
        let tolerance = (dab.texture.z >> 16u) & 255u;
        let transparent = dab.texture.z >> 24u;
        if (opacity > tolerance) { coverage = maximum; }
        else if (opacity <= transparent) { coverage = 0u; }
    }
    if ((dab.size.w & 128u) != 0u) { return coverage; }
    return paperCoverage(dab, pixel, coverage, maximum);
}

fn washByteColor(destination: u32, source: u32, blend: f32) -> u32 {
    let change = f32(i32(source) - i32(destination)) * blend;
    return u32(round(clamp(f32(destination) + change, 0.0, 255.0)));
}

// WASH fills a transparent source before painting its generated outline.
// Preserve native per-dab color interpolation as well as the accumulated alpha.
fn generatedBrushSource(pixel: vec2<i32>, initial: vec4<u32>, withColor: bool) -> vec4<u32> {
    var result = initial;
    var maskAlpha = 0u;
    let word = parameters.native.z != 0u;
    let primaryMaximum = select(255u, 65535u, word);
    let gray = (parameters.paintFlags & 524288u) != 0u;
    let foreground = nativePixel(parameters.color, parameters.native.x, word).xyz;
    for (var index = 0u; index < parameters.count; index += 1u) {
        let dab = gaussianDab(index);
        let point = pixel - dab.origin;
        if (any(point < vec2<i32>(0)) || any(point >= vec2<i32>(dab.size.xy))) { continue; }
        let masking = (dab.size.w & 128u) != 0u;
        let maximum = select(primaryMaximum, 255u, masking);
        let coverage = generatedDabCoverage(dab, pixel, point, maximum, gray || masking);
        if (masking) {
            maskAlpha = maskWashAlpha(maskAlpha, coverage, dab.application);
            continue;
        }
        if (withColor) {
            var opacity = dab.application.opacity;
            if (!dab.application.creamy) { opacity = (opacity * dab.application.flow); }
            if (gray) {
                let opacityByte = u32(((opacity * 255.0) + 0.5));
                let alpha = multiplyByte(coverage, opacityByte);
                if (result.w == 0u) { result = vec4<u32>(foreground, result.w); }
                else { result = vec4<u32>(vec3<u32>(blendByte(result.x, foreground.x, alpha)), result.w); }
            } else if (word) {
                let alpha = (nativeNormalizedAlpha(coverage) * opacity);
                if (result.w == 0u) { result = vec4<u32>(foreground, result.w); }
                else {
                    for (var c = 0u; c < 3u; c += 1u) {
                        result[c] = nativeColor(result[c], foreground[c], alpha);
                    }
                }
            } else {
                let mask = (f32(coverage) * 0.00392156886);
                let source = (mask * (255.0 * opacity));
                if (source == 0.0) { continue; }
                let alpha = (source * 0.00392156886);
                if (result.w == 0u) { result = vec4<u32>(foreground, result.w); }
                else {
                    for (var c = 0u; c < 3u; c += 1u) {
                        result[c] = washByteColor(result[c], foreground[c], alpha);
                    }
                }
            }
        }
        if (word) { result.w = washWordAlpha(result.w, coverage, dab.application); }
        else if (gray) { result.w = maskWashAlpha(result.w, coverage, dab.application); }
        else { result.w = washAlpha(result.w, coverage, dab.application); }
    }
    if ((parameters.paintFlags & 2097152u) != 0u && result.w != 0u) {
        result.w = min(primaryMaximum, result.w + maskAlpha * select(1u, 257u, word));
    }
    return result;
}

fn cellAdd(a: vec2<u32>, b: vec2<u32>) -> vec2<u32> {
    let low = a.x + b.x;
    return vec2<u32>(low, a.y + b.y + select(0u, 1u, low < a.x));
}
fn cellUnsignedLessEqual(a: vec2<u32>, b: vec2<u32>) -> bool {
    return a.y < b.y || (a.y == b.y && a.x <= b.x);
}
fn cellProduct(a: u32, b: u32) -> vec2<u32> {
    let low = (a & 65535u) * (b & 65535u);
    let first = (a >> 16u) * (b & 65535u);
    let second = (b >> 16u) * (a & 65535u);
    let middle = (low >> 16u) + (first & 65535u) + (second & 65535u);
    return vec2<u32>((low & 65535u) | (middle << 16u),
        (a >> 16u) * (b >> 16u) + (first >> 16u) + (second >> 16u) + (middle >> 16u));
}

fn cellNegate(a: vec2<u32>) -> vec2<u32> { return cellAdd(~a, vec2<u32>(1u, 0u)); }
fn cellProductSigned(a: i32, b: i32) -> vec2<u32> {
    let value = cellProduct(u32(abs(a)), u32(abs(b)));
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
    let low = cellProduct(value.x, factor);
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
        if (remainder != 0u) { quotient = cellAdd(quotient, vec2<u32>(1u, 0u)); }
        quotient = cellNegate(quotient);
    }
    return quotient;
}
fn cellRowX(origin: vec2<u32>, yOrigin: i32, delta: vec2<i32>, y: i32) -> vec2<u32> {
    var numerator = cellAdd(cellShift(cellProductSigned(-yOrigin, delta.x)), cellProductSigned(y, delta.x));
    if (delta.y < 0) { numerator = cellNegate(numerator); }
    return cellAdd(cellShift(origin), cellFloorDivide(numerator, u32(abs(delta.y))));
}
// The quotient is at most one pixel (256 subpixels). Nine comparisons avoid
// narrowing a wide edge's denominator or its clipped intersection numerator.
fn cellHeight(along: vec2<u32>, width: vec2<u32>, height: i32) -> i32 {
    let numerator = cellMultiply(along, u32(abs(height)));
    var quotient = 0u;
    for (var bit = 8; bit >= 0; bit -= 1) {
        let candidate = quotient | (1u << u32(bit));
        if (cellUnsignedLessEqual(cellMultiply(width, candidate), numerator)) { quotient = candidate; }
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
        originX = cellAdd(cellSigned(parameters.mirror.x * 8), cellNegate(originX));
        originX = cellAdd(originX, cellSigned(edge.mirroredRounding.x));
        delta.x = -delta.x + edge.mirroredRounding.z - edge.mirroredRounding.x;
    }
    if ((reflection & 2u) != 0u) {
        originY = cellAdd(cellSigned(parameters.mirror.y * 8), cellNegate(originY));
        originY = cellAdd(originY, cellSigned(edge.mirroredRounding.y));
        delta.y = -delta.y + edge.mirroredRounding.w - edge.mirroredRounding.y;
    }
    originX = cellAdd(originX, cellNegate(cellSigned(pixel.x * 64)));
    originY = cellAdd(originY, cellNegate(cellSigned(pixel.y * 64)));
    let endY = cellAdd(originY, cellSigned(delta.y));
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
        let width = cellAdd(x2, cellNegate(x1));
        var ya = y1;
        var yb = y2;
        var xa = i32(x1.x);
        var xb = i32(x2.x);
        if (cellLessEqual(x1, zero)) {
            ya += cellHeight(cellNegate(x1), width, y2 - y1);
            xa = 0;
        }
        if (cellLessEqual(right, x2)) {
            yb = y1 + cellHeight(cellAdd(right, cellNegate(x1)), width, y2 - y1);
            xb = 256;
        }
        return 512 * (yb - y1) - (xa + xb) * (yb - ya);
    }
    let width = cellAdd(x1, cellNegate(x2));
    var ya = y1;
    var yb = y2;
    var xa = i32(x1.x);
    var xb = i32(x2.x);
    if (cellLessEqual(right, x1)) {
        ya += cellHeight(cellAdd(x1, cellNegate(right)), width, y2 - y1);
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
        area = cellAdd(area, cellSigned(fillEdgeArea(fillEdges[i], pixel, reflection)));
    }
    // Arithmetic right shift of a negative area rounds down before abs.
    var magnitude = area;
    if (i32(area.y) < 0) { magnitude = cellAdd(cellNegate(area), vec2<u32>(511u, 0u)); }
    if ((bitcast<u32>(parameters.mirror.w) & 4u) == 0u) {
        let coverage = (magnitude.x >> 9u) & 511u;
        if (coverage > 256u) { return 512u - coverage; }
        return min(coverage, 255u);
    }
    if (magnitude.y != 0u) { return 255u; }
    return min(magnitude.x >> 9u, 255u);
}


fn compositePaintPixel(base: vec4<u32>, sourceColor: vec3<u32>, coverage: u32, sourceHasColor: bool,
                       blendMode: u32, selectionAlpha: u32, opacityValue: u32, opacity: f32,
                       pixel: vec2<i32>, local: vec2<u32>) -> vec4<u32> {
    var source = sourceColor;
    let partialWordSource = (parameters.paintFlags & 32768u) != 0u;
    let clearsSelectedPixels = (parameters.paintFlags & 131072u) != 0u;
    if ((parameters.paintFlags & 65536u) != 0u && blendMode != 130u) {
        let bounds = parameters.destination;
        if (bounds.z <= 0 || bounds.w <= 0 || pixel.x < bounds.x || pixel.y < bounds.y ||
            pixel.x >= bounds.x + bounds.z || pixel.y >= bounds.y + bounds.w) {
            return base;
        }
    }
    let wordCoverage = coverage;
    var writable = (parameters.paintFlags >> 1u) & 15u;
    if ((parameters.paintFlags & 1u) != 0u && parameters.erase == 0u) { writable &= 7u; }
    if ((parameters.paintFlags & 524288u) != 0u && parameters.erase == 0u) {
        // Alpha8 destinations are converted to opaque GrayA8 before native
        // composition. Gray uses integer Over/Copy rather than the optimized
        // four-channel RGB implementations.
        if (blendMode == 0u) {
            let alpha = multiplyThreeBytes(coverage, selectionAlpha, opacityValue);
            let gray = blendByte(base.x, source.x, alpha);
            return vec4<u32>(vec3<u32>(gray), 255u);
        }
        if (blendMode == 130u) {
            let opacity = multiplyByte(selectionAlpha, opacityValue);
            var result = base;
            if (opacity == 255u) { result = vec4<u32>(source, coverage); }
            else if (opacity != 0u) {
                let alpha = blendByte(255u, coverage, opacity);
                var gray = 0u;
                if (alpha != 0u) {
                    let mixed = blendByte(base.x, multiplyByte(source.x, coverage), opacity);
                    gray = min(255u, divideByte(mixed, alpha));
                }
                result = vec4<u32>(vec3<u32>(gray), alpha);
            }
            return result;
        }
    }
    if (parameters.erase == 0u && blendMode == 130u) {
        let block = (local.y / 64u) * 4u + local.x / 64u;
        if ((parameters.native.w & (1u << block)) == 0u) { return base; }
        if (!sourceHasColor) { source = vec3<u32>(0u); }
        let selectedOpacity = opacity * f32(selectionAlpha) / 255.0;
        return copyPixel(base, vec4<u32>(source, coverage), selectedOpacity, writable, parameters.native.z != 0u);
    }
    if (parameters.erase == 0u && parameters.native.z != 0u && (blendMode >= 127u && blendMode <= 129u)) {
        let block = (local.y / 64u) * 4u + local.x / 64u;
        if ((parameters.native.w & (1u << block)) == 0u) { return base; }
        if (!sourceHasColor) { source = vec3<u32>(0u); }
        return specialAlphaRgba16Pixel(base, vec4<u32>(source, wordCoverage),
            opacityValue, writable, selectionAlpha * 257u, blendMode);
    }
    if (parameters.erase == 0u && parameters.native.z != 0u && colorIsCoupled(blendMode)) {
        let block = (local.y / 64u) * 4u + local.x / 64u;
        if ((parameters.native.w & (1u << block)) == 0u) { return base; }
        let alpha = appliedWordAlpha(wordCoverage, selectionAlpha * 257u, opacityValue, blendMode);
        return coupledRgba16Pixel(base, vec4<u32>(source, wordCoverage), alpha, writable, blendMode);
    }
    if (partialWordSource) {
        let block = (local.y / 64u) * 4u + local.x / 64u;
        if ((parameters.native.w & (1u << block)) == 0u) { return base; }
        let alpha = appliedWordAlpha(wordCoverage, selectionAlpha * 257u, opacityValue, blendMode);
        return separableRgba16Pixel(base, vec4<u32>(source, wordCoverage), alpha, writable, blendMode);
    }
    if (parameters.erase == 0u && (blendMode == 41u || blendMode == 42u || transparentSourceAffectsBlend(blendMode))) {
        let block = (local.y / 64u) * 4u + local.x / 64u;
        if ((parameters.native.w & (1u << block)) == 0u) {
            return base;
        }
        if (blendMode == 127u) {
            if (!sourceHasColor) { source = vec3<u32>(0u); }
            return unpackPixel(markerRgba8Pixel(base, vec4<u32>(source, coverage), opacityValue, writable, selectionAlpha));
        }
        let alpha = multiplyThreeBytes(coverage, selectionAlpha, opacityValue);
        if (blendMode == 128u || blendMode == 129u) {
            return unpackPixel(specialAlphaRgba8Pixel(base, vec4<u32>(source, alpha), 255u, writable, blendMode));
        }
        if (colorIsCoupled(blendMode)) {
            return unpackPixel(coupledRgba8Pixel(base, vec4<u32>(source, coverage), alpha, writable, blendMode));
        }
        return unpackPixel(destinationRgba8(base, vec4<u32>(source, coverage), alpha, writable, blendMode));
    }
    if (parameters.erase != 0u) {
        var resultAlpha: u32;
        if (parameters.native.z != 0u) {
            let masked = mul65535(wordCoverage, selectionAlpha * 257u);
            let alpha = mul65535(masked, opacityValue);
            resultAlpha = mul65535(base.w, 65535u - alpha);
        } else {
            let alpha = mul255(mul255(coverage, selectionAlpha), opacityValue);
            resultAlpha = mul255(base.w, 255u - alpha);
        }
        // Native erase applies alpha independently of protected color channels.
        var result = vec4<u32>(base.xyz, resultAlpha);
        if (clearsSelectedPixels) {
            let defaultPixel = nativePixel(parameters.color, parameters.native.x, parameters.native.z != 0u);
            var defaultByteAlpha = defaultPixel.w;
            var resultByteAlpha = resultAlpha;
            if (parameters.native.z != 0u) {
                defaultByteAlpha = (defaultPixel.w - (defaultPixel.w >> 8u) + 128u) >> 8u;
                resultByteAlpha = (resultAlpha - (resultAlpha >> 8u) + 128u) >> 8u;
            }
            if (defaultByteAlpha == 0u && resultByteAlpha == 0u) { result = defaultPixel; }
        }
        return result;
    } else if (blendMode == 0u) {
        var sourceAlpha: f32;
        if (parameters.native.z != 0u) {
            sourceAlpha = (nativeNormalizedAlpha(wordCoverage) * opacity);
        } else {
            sourceAlpha = (opacity * f32(coverage));
        }
        if (selectionAlpha != 255u) {
            sourceAlpha = (sourceAlpha * (0.00392156886 * f32(selectionAlpha)));
        }
        if (parameters.native.z != 0u) {
            return normalRgba16(base, source, sourceAlpha, writable);
        } else {
            return unpackPixel(normalRgba8(base, source, sourceAlpha, writable));
        }
    } else {
        // Generic CPU operations quantize the three-factor product once.
        // Erase and channel-copy retain their distinct two-factor orders.
        if (parameters.native.z != 0u) {
            let alpha = appliedWordAlpha(wordCoverage, selectionAlpha * 257u, opacityValue, blendMode);
            return separableRgba16Pixel(base, vec4<u32>(source, wordCoverage), alpha, writable, blendMode);
        }
        var alpha = multiplyThreeBytes(coverage, selectionAlpha, opacityValue);
        if (blendMode == 26u) {
            alpha = mul255(mul255(coverage, selectionAlpha), opacityValue);
        } else if (blendMode >= 28u && blendMode <= 30u) {
            alpha = mul255(coverage, mul255(selectionAlpha, opacityValue));
        }
        let blended = unpackPixel(separableProjectionPixel(base, vec4<u32>(source, alpha), 255u, 15u, blendMode));
        return select(base, blended, vec4<bool>((writable & 1u) != 0u,
            (writable & 2u) != 0u, (writable & 4u) != 0u, (writable & 8u) != 0u));
    }
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
    let composition = parameters.composition;
    if (composition.z > 0 && composition.w > 0 &&
        (pixel.x < composition.x || pixel.y < composition.y ||
         pixel.x >= composition.x + composition.z || pixel.y >= composition.y + composition.w)) {
        writePaintPixel(index, paintBase(index));
        return;
    }
    let selection = parameters.selection;
    let maskSelection = (parameters.paintFlags & 4096u) != 0u;
    let softSelection = (parameters.paintFlags & 8192u) != 0u;
    let partialWordSource = (parameters.paintFlags & 32768u) != 0u;
    let clearsSelectedPixels = (parameters.paintFlags & 131072u) != 0u;
    let emptySourceComposition = clearsSelectedPixels || (parameters.erase == 0u &&
        (partialWordSource || parameters.pattern.w == 130u || parameters.pattern.z == 130u || parameters.pattern.w == 41u || parameters.pattern.w == 42u ||
         parameters.pattern.z == 41u || parameters.pattern.z == 42u ||
         transparentSourceAffectsBlend(parameters.pattern.w) || transparentSourceAffectsBlend(parameters.pattern.z)));
    let insideSelectionBounds = pixel.x >= selection.x && pixel.y >= selection.y &&
        pixel.x < selection.x + selection.z && pixel.y < selection.y + selection.w;
    var selectionAlpha = 255u;
    if ((parameters.paintFlags & 262144u) != 0u) {
        let extent = parameters.patternSampling.zw;
        let local = pixel - selection.xy;
        if (any(local < vec2<i32>()) || any(vec2<u32>(local) >= extent)) {
            writePaintPixel(index, paintBase(index));
            return;
        }
        selectionAlpha = selectionCoverage[u32(local.y) * extent.x + u32(local.x)];
        if (selectionAlpha == 0u) {
            writePaintPixel(index, paintBase(index));
            return;
        }
    } else if (maskSelection || (selection.z > 0 && selection.w > 0)) {
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
            if (selectionAlpha == 0u && (!emptySourceComposition || !insideSelectionBounds)) {
                writePaintPixel(index, paintBase(index));
                return;
            }
        } else {
            var masked = !inside;
            if (inverted) { masked = inside; }
            if (masked) {
                if (emptySourceComposition && insideSelectionBounds) { selectionAlpha = 0u; }
                else {
                    writePaintPixel(index, paintBase(index));
                    return;
                }
            }
        }
    }
    let figureFlags = bitcast<u32>(parameters.mirror.w);
    let generatedFilled = GENERATED_BRUSH_ENABLED && (figureFlags & 1u) != 0u;
    var strokeCoverage = 0u;
    if (!GENERATED_BRUSH_ENABLED) {
        if ((parameters.paintFlags & 128u) != 0u) { strokeCoverage = previousCoverage[index]; }
        if ((bitcast<u32>(parameters.mirror.w) & 2u) == 0u) {
            for (var i = 0u; i < parameters.count && strokeCoverage != 255u; i += 1u) {
                strokeCoverage = max(strokeCoverage, markerCoverage(markerDab(i), pixel));
            }
        }
        if ((parameters.paintFlags & 256u) != 0u) { retainedCoverage[index] = strokeCoverage; }
    }
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
    let foreground = nativePixel(parameters.color, parameters.native.x, parameters.native.z != 0u).xyz;
    var source = foreground;
    var coverage = strokeCoverage;
    if (parameters.native.z != 0u && !GENERATED_BRUSH_ENABLED) { coverage *= 257u; }
    var sourceHasColor = strokeCoverage != 0u;
    var blendMode = parameters.pattern.w;
    var generatedInitial = vec4<u32>();
    var directFill = vec4<u32>();
    var directFillHasColor = false;
    if ((figureFlags & 1u) != 0u) {
        var fillColor = nativePixel(bitcast<u32>(parameters.mirror.z), parameters.native.y, parameters.native.z != 0u);
        if ((figureFlags & 8u) != 0u) { fillColor = patternPixel(pixel); }
        if ((figureFlags & 16u) != 0u) { fillColor = gradientPixel(pixel); }
        // Region post-filters can select outside the finite fill source. The
        // native source device supplies transparent black there, including Copy.
        if ((parameters.paintFlags & 262144u) != 0u &&
            (any(pixel < vec2<i32>()) || any(pixel >= selection.zw))) { fillColor = vec4<u32>(); }
        var fillAlpha = mul255(fillColor.w, fillCoverage);
        if (parameters.native.z != 0u) { fillAlpha = mul65535(fillColor.w, fillCoverage * 257u); }
        directFill = vec4<u32>(fillColor.xyz, fillAlpha);
        directFillHasColor = fillCoverage != 0u;
        if (generatedFilled) {
            if (fillAlpha != 0u) { generatedInitial = vec4<u32>(fillColor.xyz, fillAlpha); }
            if ((figureFlags & 2u) != 0u) { blendMode = parameters.pattern.z; }
        // Native marker writes retain the fill color at equal temporary alpha.
        } else if (fillAlpha >= coverage) {
            source = fillColor.xyz;
            sourceHasColor = fillCoverage != 0u;
            coverage = fillAlpha;
            blendMode = parameters.pattern.z;
        }
    }
    let direct = GENERATED_BRUSH_ENABLED && (parameters.paintFlags & 4194304u) != 0u;
    if (!direct && GENERATED_BRUSH_ENABLED) {
        let temporary = generatedBrushSource(pixel, generatedInitial, generatedFilled);
        coverage = temporary.w;
        sourceHasColor = coverage != 0u;
        if (generatedFilled) {
            source = temporary.xyz;
            sourceHasColor = any(temporary != vec4<u32>());
        }
    }
    // WASH merges its accumulated source once. BUILDUP applies fill and each
    // dab in order through the same call, keeping the full compositor single.
    var result = base;
    let fillCount = select(0u, 1u, generatedFilled);
    let applications = select(1u, parameters.count + fillCount, direct);
    let wide = parameters.native.z != 0u;
    let maximum = select(255u, 65535u, wide);
    let gray = (parameters.paintFlags & 524288u) != 0u;
    for (var application = 0u; application < applications; application++) {
        var color = source;
        var alpha = coverage;
        var hasColor = sourceHasColor;
        var mode = blendMode;
        var opacityValue = parameters.opacityValue;
        var opacity = parameters.canvasOpacity;
        if (direct) {
            if (generatedFilled && application == 0u) {
                color = directFill.xyz;
                alpha = directFill.w;
                hasColor = directFillHasColor;
                mode = parameters.pattern.z;
            } else {
                let dab = gaussianDab(application - fillCount);
                let point = pixel - dab.origin;
                if (any(point < vec2<i32>(0)) || any(point >= vec2<i32>(dab.size.xy))) { continue; }
                color = foreground;
                alpha = generatedDabCoverage(dab, pixel, point, maximum, gray);
                hasColor = true;
                mode = parameters.pattern.w;
                let scale = select(255.0, 65535.0, wide);
                opacityValue = u32(((dab.application.opacity * scale) + 0.5));
                opacity = dab.application.opacity;
            }
        }
        result = compositePaintPixel(result, color, alpha, hasColor, mode,
            selectionAlpha, opacityValue, opacity, pixel, local);
    }
    writePaintPixel(index, result);
}

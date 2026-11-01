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
    composition: vec4<i32>,
    destination: vec4<i32>,
    patternMap0: vec4<u32>,
    patternMap1: vec4<u32>,
    patternMap2: vec4<u32>,
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
struct PatternCoordinate { pixel: i32, fraction: Word64, }
fn patternCoordinate(value: Word64, period: u32) -> PatternCoordinate {
    let magnitude = doubleMagnitude(value);
    let integral = doubleFloor(magnitude);
    var fraction = doubleSubtract(magnitude, integral);
    var wrapped = 0u;
    if (integral.y < 0x41e00000u) {
        wrapped = doubleTruncate(integral) % period;
    } else {
        let power = doublePower(integral);
        let bits = wordRight(doubleMantissa(integral), u32(-power));
        // Resource dimensions are bounded by the owned pixel byte capacity,
        // so twice a remainder fits in an unsigned word.
        for (var bit = 52; bit >= 0; bit--) {
            wrapped = (wrapped * 2u + (wordRight(bits, u32(bit)).x & 1u)) % period;
        }
    }
    if ((value.y & 0x80000000u) != 0u) {
        if (any(fraction != Word64(0u))) {
            wrapped = (wrapped + 1u) % period;
            fraction = doubleSubtract(Word64(0u, 0x3ff00000u), fraction);
        }
        if (wrapped != 0u) { wrapped = period - wrapped; }
    }
    return PatternCoordinate(i32(wrapped), fraction);
}
fn patternQuotient(total: Word64, denominator: u32, maximum: u32) -> u32 {
    let rounded = wordAdd(total, Word64(denominator / 2u, 0u));
    var result = 0u;
    var bit = select(128u, 32768u, maximum == 65535u);
    loop {
        if (bit == 0u) { break; }
        let candidate = result | bit;
        if (wordLessEqual(wordProduct(candidate, denominator), rounded)) { result = candidate; }
        bit >>= 1u;
    }
    return result;
}
fn patternPixel(pixel: vec2<i32>) -> vec4<u32> {
    let x = markerInteger(pixel.x); let y = markerInteger(pixel.y);
    let sx = doubleAdd(doubleAdd(doubleMultiply(parameters.patternMap0.xy, x),
                                doubleMultiply(parameters.patternMap1.xy, y)), parameters.patternMap2.xy);
    let sy = doubleAdd(doubleAdd(doubleMultiply(parameters.patternMap0.zw, x),
                                doubleMultiply(parameters.patternMap1.zw, y)), parameters.patternMap2.zw);
    let mappedX = patternCoordinate(sx, parameters.pattern.x);
    let mappedY = patternCoordinate(sy, parameters.pattern.y);
    let left = mappedX.pixel; let top = mappedY.pixel;
    if (parameters.patternSampling.y == 0u) { return patternTexel(vec2<i32>(left, top)); }
    let hx = mappedX.fraction; let vy = mappedY.fraction;
    let one = Word64(0u, 0x3ff00000u); let half = Word64(0u, 0x3fe00000u);
    let complements = array<Word64, 2>(doubleSubtract(one, hx), doubleSubtract(one, vy));
    let horizontal = array<Word64, 2>(complements[0], hx);
    let vertical = array<Word64, 2>(complements[1], vy);
    var totals = array<Word64, 3>(Word64(0u), Word64(0u), Word64(0u));
    var alpha = 0u; var weightSum = 0u;
    for (var row = 0u; row < 2u; row++) {
        for (var column = 0u; column < 2u; column++) {
            // Native subpixel access rounds each independent weight, then
            // normalizes by their actual sum (which can differ from 255).
            let weight = doubleTruncate(doubleAdd(doubleMultiply(doubleMultiply(horizontal[column], vertical[row]), doubleInteger(255u)), half));
            let color = patternTexel(vec2<i32>(left + i32(column), top + i32(row)));
            let amount = color.w * weight;
            weightSum += weight; alpha += amount;
            for (var channel = 0u; channel < 3u; channel++) {
                totals[channel] = wordAdd(totals[channel], wordProduct(color[channel], amount));
            }
        }
    }
    if (alpha == 0u) { return vec4<u32>(0u); }
    let maximum = select(255u, 65535u, parameters.native.z != 0u);
    return vec4<u32>(patternQuotient(totals[0], alpha, maximum), patternQuotient(totals[1], alpha, maximum),
                     patternQuotient(totals[2], alpha, maximum), min(maximum, (alpha + weightSum / 2u) / weightSum));
}

// Gradient colors are imported resources. All per-pixel geometry, seam
// interpolation and native-depth quantization belong to canonical GPU pixels.
fn gradientLess(a: Word64, b: Word64) -> bool {
    if (all(doubleMagnitude(a) == Word64(0u)) && all(doubleMagnitude(b) == Word64(0u))) { return false; }
    let negativeA = (a.y & 0x80000000u) != 0u;
    let negativeB = (b.y & 0x80000000u) != 0u;
    if (negativeA != negativeB) { return negativeA; }
    if (negativeA) { return !wordLessEqual(doubleMagnitude(a), doubleMagnitude(b)); }
    return !wordLessEqual(b, a);
}
fn gradientAtan2(y: Word64, x: Word64) -> Word64 {
    let pi = Word64(0x54442d18u, 0x400921fbu);
    let halfPi = Word64(0x54442d18u, 0x3ff921fbu);
    let tinyPi = Word64(0x33145c07u, 0x3ca1a626u);
    let negativeX = (x.y & 0x80000000u) != 0u;
    let negativeY = (y.y & 0x80000000u) != 0u;
    var angle = Word64(0u);
    if (all(doubleMagnitude(y) == Word64(0u))) { angle = select(Word64(0u), pi, negativeX); }
    else if (all(doubleMagnitude(x) == Word64(0u))) { angle = halfPi; }
    else {
        angle = positiveColorAtan(doubleMagnitude(doubleDivide(y, x)));
        if (negativeX) { angle = doubleSubtract(pi, doubleSubtract(angle, tinyPi)); }
    }
    if (negativeY) { return doubleNegate(angle); }
    return angle;
}
fn gradientAngle(px: Word64, py: Word64, dx: Word64, dy: Word64) -> Word64 {
    let pi = Word64(0x54442d18u, 0x400921fbu);
    var angle = doubleSubtract(doubleAdd(gradientAtan2(py, px), pi), doubleAdd(gradientAtan2(dy, dx), pi));
    if (gradientLess(angle, Word64(0u))) { angle = doubleAdd(angle, Word64(0x54442d18u, 0x401921fbu)); }
    return angle;
}
fn gradientPosition(pixel: vec2<i32>) -> Word64 {
    let zero = Word64(0u); let epsilon = Word64(0u, 0x3cb00000u);
    let dx = doubleSubtract(parameters.patternMap1.xy, parameters.patternMap0.xy);
    let dy = doubleSubtract(parameters.patternMap1.zw, parameters.patternMap0.zw);
    let length = doubleSqrt(doubleAdd(doubleMultiply(dx, dx), doubleMultiply(dy, dy)));
    if (parameters.pattern.y < 4u && gradientLess(length, epsilon)) { return zero; }
    let px = doubleSubtract(markerInteger(pixel.x), parameters.patternMap0.xy);
    let py = doubleSubtract(markerInteger(pixel.y), parameters.patternMap0.zw);
    if (parameters.pattern.y >= 4u) {
        let angle = gradientAngle(px, py, dx, dy);
        let pi = Word64(0x54442d18u, 0x400921fbu);
        let one = Word64(0u, 0x3ff00000u);
        let turn = doubleDivide(angle, Word64(0x54442d18u, 0x401921fbu));
        if (parameters.pattern.y == 4u) { return turn; }
        if (parameters.pattern.y == 5u) {
            if (gradientLess(angle, pi)) { return doubleDivide(angle, pi); }
            return doubleSubtract(one, doubleDivide(doubleSubtract(angle, pi), pi));
        }
        var radius = Word64(0u);
        if (!gradientLess(length, epsilon)) {
            radius = doubleDivide(doubleSqrt(doubleAdd(doubleMultiply(px, px), doubleMultiply(py, py))), length);
        }
        if (parameters.pattern.y == 7u) { return doubleAdd(radius, doubleSubtract(one, turn)); }
        return doubleAdd(radius, turn);
    }
    if (parameters.pattern.y == 2u) {
        return doubleDivide(doubleSqrt(doubleAdd(doubleMultiply(px, px), doubleMultiply(py, py))), length);
    }
    let nx = doubleDivide(dx, length); let ny = doubleDivide(dy, length);
    if (parameters.pattern.y == 3u) {
        if (!gradientLess(epsilon, length)) { return zero; }
        let first = doubleMagnitude(doubleAdd(doubleMultiply(doubleNegate(ny), px), doubleMultiply(nx, py)));
        let second = doubleMagnitude(doubleAdd(doubleMultiply(doubleNegate(ny), doubleNegate(py)), doubleMultiply(nx, px)));
        return doubleDivide(select(second, first, wordLessEqual(second, first)), length);
    }
    var t = doubleDivide(doubleAdd(doubleMultiply(px, nx), doubleMultiply(py, ny)), length);
    if (parameters.pattern.y == 1u && gradientLess(t, doubleNegate(epsilon))) { t = doubleNegate(t); }
    return t;
}
fn gradientRepeated(value: Word64) -> Word64 {
    let one = Word64(0u, 0x3ff00000u); let epsilon = Word64(0u, 0x3cb00000u);
    if (parameters.patternSampling.y == 0u) {
        if (gradientLess(value, epsilon)) { return Word64(0u); }
        if (gradientLess(doubleSubtract(one, epsilon), value)) { return one; }
        return value;
    }
    let integral = doubleFloor(doubleMagnitude(value));
    if (parameters.patternSampling.y == 1u) {
        var integer = integral;
        if ((value.y & 0x80000000u) != 0u) { integer = doubleNegate(integer); }
        if (gradientLess(value, epsilon)) { integer = doubleSubtract(integer, one); }
        return doubleSubtract(value, integer);
    }
    if (parameters.pattern.y >= 6u) {
        var fraction = doubleSubtract(doubleMagnitude(value), integral);
        let halfIntegral = doubleFloor(doubleMultiply(doubleMagnitude(value), doubleInteger(2u)));
        if (patternCoordinate(halfIntegral, 2u).pixel == 1) { fraction = doubleSubtract(one, fraction); }
        return doubleMultiply(fraction, doubleInteger(2u));
    }
    var fraction = doubleSubtract(doubleMagnitude(value), integral);
    if (patternCoordinate(integral, 2u).pixel == 1) { fraction = doubleSubtract(one, fraction); }
    return fraction;
}
fn gradientCached(value: Word64) -> vec4<u32> {
    let count = parameters.pattern.x;
    let index = min(count - 1u, doubleTruncate(doubleAdd(
        doubleMultiply(value, doubleInteger(count - 1u)), Word64(0u, 0x3fe00000u))));
    return nativePixel(patternPixels[index * 2u], patternPixels[index * 2u + 1u], true);
}
fn gradientColor(pixel: vec2<i32>) -> vec4<u32> {
    let one = Word64(0u, 0x3ff00000u); let half = Word64(0u, 0x3fe00000u);
    var t = gradientPosition(pixel);
    let dx = doubleSubtract(parameters.patternMap1.xy, parameters.patternMap0.xy);
    let dy = doubleSubtract(parameters.patternMap1.zw, parameters.patternMap0.zw);
    let length = doubleSqrt(doubleAdd(doubleMultiply(dx, dx), doubleMultiply(dy, dy)));
    let antiAlias = parameters.patternMap2.xy;
    let hasAntialias = gradientLess(Word64(0u, 0x3cb00000u), antiAlias);
    let shape = parameters.pattern.y;
    let spiralSeam = shape >= 6u && parameters.patternSampling.y == 0u && hasAntialias;
    var threshold = doubleDivide(antiAlias, length);
    var seamSmoothing = parameters.patternSampling.y == 1u && (shape < 4u || shape >= 6u) && hasAntialias;
    var distance = Word64(0u);
    var seam = Word64(0u);
    if ((shape == 4u && hasAntialias) || spiralSeam) {
        let px = doubleSubtract(markerInteger(pixel.x), parameters.patternMap0.xy);
        let py = doubleSubtract(markerInteger(pixel.y), parameters.patternMap0.zw);
        distance = doubleSqrt(doubleAdd(doubleMultiply(px, px), doubleMultiply(py, py)));
        var singularity = doubleInteger(8u);
        if (spiralSeam) { singularity = doubleDivide(length, doubleInteger(32u)); }
        threshold = antiAlias;
        if (gradientLess(distance, singularity)) { threshold = doubleDivide(doubleMultiply(distance, antiAlias), singularity); }
        if (any(distance != Word64(0u))) {
            threshold = doubleDivide(threshold, doubleMultiply(Word64(0x54442d18u, 0x401921fbu), distance));
        }
        seamSmoothing = any(distance != Word64(0u)) && (!spiralSeam || gradientLess(distance, length));
        if (spiralSeam) { seam = gradientRepeated(doubleDivide(gradientAngle(px, py, dx, dy), Word64(0x54442d18u, 0x401921fbu))); }
    }
    let center = seamSmoothing && shape >= 1u && shape <= 3u && !gradientLess(threshold, t);
    if (!center) { t = gradientRepeated(t); }
    if ((parameters.patternSampling.x & 1u) != 0u) { t = doubleSubtract(one, t); }
    var color = gradientCached(t);
    if (!spiralSeam) { seam = t; }
    let reverseThreshold = doubleSubtract(one, threshold);
    if (seamSmoothing && !center && (!gradientLess(threshold, seam) || !gradientLess(seam, reverseThreshold))) {
        let twice = doubleMultiply(doubleInteger(2u), threshold);
        var s = doubleDivide(doubleSubtract(seam, reverseThreshold), twice);
        if (!gradientLess(threshold, seam)) { s = doubleAdd(half, doubleDivide(seam, twice)); }
        let weight = min(32767u, doubleTruncate(doubleAdd(doubleMultiply(doubleSubtract(one, s), doubleInteger(32767u)), half)));
        var first = gradientCached(one); var second = gradientCached(Word64(0u));
        if (spiralSeam) {
            if ((parameters.patternSampling.x & 1u) != 0u) {
                distance = doubleSubtract(length, distance); first = gradientCached(Word64(0u));
            }
            let radial = gradientCached(doubleDivide(distance, length));
            second = radial;
            if (shape == 7u) { second = first; first = radial; }
        }
        let a = first.w * weight; let b = second.w * (32767u - weight);
        let alpha = a + b;
        color = vec4<u32>();
        if (alpha != 0u) {
            for (var channel = 0u; channel < 3u; channel++) {
                color[channel] = patternQuotient(wordAdd(wordProduct(first[channel], a), wordProduct(second[channel], b)), alpha, 65535u);
            }
            color.w = (alpha + 16383u) / 32767u;
        }
    }
    return color;
}
fn gradientPixel(pixel: vec2<i32>) -> vec4<u32> {
    let color = gradientColor(pixel);
    let wide = parameters.native.z != 0u;
    if ((parameters.patternSampling.x & 2u) == 0u) {
        if (wide) { return color; }
        return (color - (color >> vec4<u32>(8u)) + vec4<u32>(128u)) >> vec4<u32>(8u);
    }
    let noiseIndex = ((u32(pixel.y) & 63u) << 6u) | (u32(pixel.x) & 63u);
    let noise = f32(patternPixels[parameters.pattern.x * 2u + noiseIndex]) * (1.0 / 4096.0) + (1.0 / 8192.0);
    let scale = select(1.0 / 256.0, 1.0 / 65536.0, wide);
    let maximum = select(255.0, 65535.0, wide);
    var result = vec4<u32>();
    for (var channel = 0u; channel < 4u; channel++) {
        let normalized = floatDivide(bitcast<u32>(f32(color[channel])), bitcast<u32>(65535.0));
        let delta = floatMultiply(floatSubtract(bitcast<u32>(noise), normalized), bitcast<u32>(scale));
        let value = floatMultiply(floatAdd(normalized, delta), bitcast<u32>(maximum));
        result[channel] = u32(clamp(bitcast<f32>(floatAdd(value, bitcast<u32>(0.5))), 0.0, maximum));
    }
    return result;
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
    let offset = index * 36u;
    return MaskDab(geometryFour(offset), geometryFour(offset + 4u), geometryFour(offset + 8u),
                   geometryFour(offset + 12u), geometryFour(offset + 16u), geometryFour(offset + 20u),
                   geometryFour(offset + 24u), geometryFour(offset + 28u), bitcast<vec4<i32>>(geometryFour(offset + 32u)));
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

// Optimized native U16 WASH normalizes stored words, interpolates in
// binary32, then rounds each retained dab back to a word.
fn maskWashAlpha(destination: u32, coverage: u32, values: vec4<u32>) -> u32 {
    let flow = values.y;
    var opacity = values.x;
    var average = values.z;
    if (values.w == 0u) {
        opacity = floatMultiply(opacity, flow);
        average = floatMultiply(average, flow);
    }
    // Generic GrayA8 WASH quantizes its settings before each dab and uses
    // integer interpolation. RGB keeps binary32 intermediates instead.
    let opacityByte = u32(bitcast<f32>(floatAdd(floatMultiply(opacity, 0x437f0000u), 0x3f000000u)));
    let averageByte = u32(bitcast<f32>(floatAdd(floatMultiply(average, 0x437f0000u), 0x3f000000u)));
    let source = multiplyByte(coverage, opacityByte);
    var full = destination;
    if (averageByte > opacityByte) {
        if (averageByte > destination) { full = blendByte(source, averageByte, divideByte(destination, averageByte)); }
    } else if (opacityByte > destination) { full = blendByte(destination, opacityByte, coverage); }
    if (flow == 0x3f800000u) { return full; }
    var zero = destination;
    if (values.w == 0u) { zero = source + destination - multiplyByte(source, destination); }
    let flowByte = u32(bitcast<f32>(floatAdd(floatMultiply(flow, 0x437f0000u), 0x3f000000u)));
    return blendByte(zero, full, flowByte);
}

fn washWordAlpha(destination: u32, coverage: u32, values: vec4<u32>) -> u32 {
    let flow = values.y;
    var opacity = values.x;
    var average = values.z;
    if (values.w == 0u) {
        opacity = floatMultiply(opacity, flow);
        average = floatMultiply(average, flow);
    }
    let mask = nativeNormalizedAlpha(coverage);
    let current = nativeNormalizedAlpha(destination);
    let source = floatMultiply(mask, opacity);
    var full = current;
    if (average > opacity) {
        if (average > current) {
            full = floatMultiplyAdd(floatSubtract(average, source), floatDivide(current, average), source);
        }
    } else if (opacity > current) {
        full = floatMultiplyAdd(floatSubtract(opacity, current), mask, current);
    }
    var alpha = full;
    if (flow != 0x3f800000u) {
        var zero = current;
        if (values.w == 0u) { zero = floatMultiplyAdd(source ^ 0x80000000u, current, floatAdd(source, current)); }
        alpha = floatMultiplyAdd(floatSubtract(full, zero), flow, zero);
    }
    return projectionComponent(floatMultiply(alpha, 0x477fff00u));
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

fn washByteColor(destination: u32, source: u32, blend: u32) -> u32 {
    if (destination == source || blend < 0x3b000000u) { return destination; }
    if (blend == 0x3f800000u) { return source; }
    let difference = i32(source) - i32(destination);
    // Native byte WASH rounds the product before adding the destination.
    // The bounded byte difference and blend >= 2^-9 fit the sum in 64 bits.
    let roundedProduct = projectionFloat(projectionProduct(u32(abs(difference)), projectionMantissa(blend)),
                                         projectionPower(blend));
    let power = projectionPower(roundedProduct);
    let product = vec2<u32>(projectionMantissa(roundedProduct), 0u);
    let base = projectionLeft(vec2<u32>(destination, 0u), u32(-power));
    var value: vec2<u32>;
    if (difference < 0) { value = projectionSubtract(base, product); }
    else { value = projectionAdd(base, product); }
    return projectionComponent(projectionFloat(value, power));
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
        let point = pixel - bitcast<vec2<i32>>(dab.coefficient.zw);
        if (any(point < vec2<i32>(0)) || any(point >= vec2<i32>(dab.size.xy))) { continue; }
        let masking = (dab.size.w & 128u) != 0u;
        let maximum = select(primaryMaximum, 255u, masking);
        let coverage = generatedDabCoverage(dab, pixel, point, maximum, gray || masking);
        if (masking) {
            maskAlpha = maskWashAlpha(maskAlpha, coverage, dab.application);
            continue;
        }
        if (withColor) {
            var opacity = dab.application.x;
            if (dab.application.w == 0u) { opacity = floatMultiply(opacity, dab.application.y); }
            if (gray) {
                let opacityByte = u32(bitcast<f32>(floatAdd(floatMultiply(opacity, 0x437f0000u), 0x3f000000u)));
                let alpha = multiplyByte(coverage, opacityByte);
                if (result.w == 0u) { result = vec4<u32>(foreground, result.w); }
                else { result = vec4<u32>(vec3<u32>(blendByte(result.x, foreground.x, alpha)), result.w); }
            } else if (word) {
                let alpha = floatMultiply(nativeNormalizedAlpha(coverage), opacity);
                if (result.w == 0u) { result = vec4<u32>(foreground, result.w); }
                else {
                    for (var c = 0u; c < 3u; c += 1u) {
                        result[c] = nativeColor(result[c], foreground[c], alpha);
                    }
                }
            } else {
                let mask = floatMultiply(bitcast<u32>(f32(coverage)), 0x3b808081u);
                let source = floatMultiply(mask, floatMultiply(0x437f0000u, opacity));
                if (source == 0u) { continue; }
                let alpha = floatMultiply(source, 0x3b808081u);
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


fn compositePaintPixel(base: vec4<u32>, sourceColor: vec3<u32>, coverage: u32, sourceHasColor: bool,
                       blendMode: u32, selectionAlpha: u32, opacityValue: u32, opacityBits: u32,
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
        let opacity = projectionMultiply(opacityBits, projectionMultiply(bitcast<u32>(f32(selectionAlpha)), 0x3b808081u));
        return copyPixel(base, vec4<u32>(source, coverage), opacity, writable, parameters.native.z != 0u);
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
        var sourceAlpha: u32;
        if (parameters.native.z != 0u) {
            sourceAlpha = projectionMultiply(nativeNormalizedAlpha(wordCoverage), opacityBits);
        } else {
            sourceAlpha = projectionScaleByte(opacityBits, coverage);
        }
        if (selectionAlpha != 255u) {
            sourceAlpha = projectionMultiply(sourceAlpha, projectionScaleByte(0x3b808081u, selectionAlpha));
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
    let generatedFilled = (parameters.paintFlags & 512u) != 0u && (figureFlags & 1u) != 0u;
    var strokeCoverage = 0u;
    if ((parameters.paintFlags & 512u) == 0u) {
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
    if (parameters.native.z != 0u && (parameters.paintFlags & 512u) == 0u) { coverage *= 257u; }
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
    let direct = (parameters.paintFlags & 4194304u) != 0u;
    if (!direct && (parameters.paintFlags & 512u) != 0u) {
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
        var opacityBits = parameters.canvasOpacityBits;
        if (direct) {
            if (generatedFilled && application == 0u) {
                color = directFill.xyz;
                alpha = directFill.w;
                hasColor = directFillHasColor;
                mode = parameters.pattern.z;
            } else {
                let dab = gaussianDab(application - fillCount);
                let point = pixel - bitcast<vec2<i32>>(dab.coefficient.zw);
                if (any(point < vec2<i32>(0)) || any(point >= vec2<i32>(dab.size.xy))) { continue; }
                color = foreground;
                alpha = generatedDabCoverage(dab, pixel, point, maximum, gray);
                hasColor = true;
                mode = parameters.pattern.w;
                let scale = select(0x437f0000u, 0x477fff00u, wide);
                opacityValue = u32(bitcast<f32>(floatAdd(floatMultiply(dab.application.x, scale), 0x3f000000u)));
                opacityBits = dab.application.x;
            }
        }
        result = compositePaintPixel(result, color, alpha, hasColor, mode,
            selectionAlpha, opacityValue, opacityBits, pixel, local);
    }
    writePaintPixel(index, result);
}

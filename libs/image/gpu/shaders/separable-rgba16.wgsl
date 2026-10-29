// SPDX-FileCopyrightText: 2011 Silvio Heinrich <plassy@web.de>
// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

fn multiplyThreeWords(a: u32, b: u32, c: u32) -> u32 {
    let product = projectionProduct(a * b, c);
    // n = high*65536 + low. This folds the exact 48-bit product through
    // two divisions by 65535; both quotients and high+low fit 32 bits.
    let high = (product.y << 16u) | (product.x >> 16u);
    let low = product.x & 65535u;
    return (high + (high + low) / 65535u) / 65535u;
}
fn divideWord(a: u32, b: u32) -> u32 { return (a * 65535u + b / 2u) / b; }
fn interpolateWord(destination: u32, source: u32, alpha: u32) -> u32 {
    let difference = i32(source) - i32(destination);
    let change = u32(abs(difference)) * alpha / 65535u;
    return select(destination + change, destination - change, difference < 0);
}
fn dodgeWord(source: u32, destination: u32) -> u32 {
    if (source == 65535u) { return select(65535u, 0u, destination == 0u); }
    return min(65535u, divideWord(destination, 65535u - source));
}
fn burnWord(source: u32, destination: u32) -> u32 {
    if (destination == 65535u) { return 65535u; }
    if (source == 0u) { return 0u; }
    return 65535u - min(65535u, divideWord(65535u - destination, source));
}
fn glowWord(source: u32, destination: u32) -> u32 {
    if (destination == 65535u) { return 65535u; }
    return min(65535u, divideWord(mul65535(source, source), 65535u - destination));
}
fn heatWord(source: u32, destination: u32) -> u32 {
    if (source == 65535u) { return 65535u; }
    if (destination == 0u) { return 0u; }
    return 65535u - min(65535u, divideWord(mul65535(65535u - source, 65535u - source), destination));
}
fn heatGlowWord(source: u32, destination: u32) -> u32 {
    if (source + destination > 65535u) { return heatWord(source, destination); }
    if (source == 0u) { return 0u; }
    return glowWord(source, destination);
}
fn freezeReflectWord(source: u32, destination: u32) -> u32 {
    if (source + destination > 65535u) { return heatWord(destination, source); }
    if (destination == 0u) { return 0u; }
    return glowWord(destination, source);
}
fn glowHeatWord(source: u32, destination: u32) -> u32 {
    if (destination == 65535u) { return 65535u; }
    if (source + destination > 65535u) { return glowWord(source, destination); }
    return heatWord(source, destination);
}
fn penumbraWord(source: u32, destination: u32) -> u32 {
    if (destination == 65535u) { return 65535u; }
    if (source + destination < 65535u) { return min(65535u, divideWord(source, 65535u - destination)) / 2u; }
    if (source == 0u) { return 0u; }
    return 65535u - min(65535u, divideWord(65535u - destination, source) / 2u);
}
fn parallelWord(source: u32, destination: u32) -> u32 {
    if (source == 0u || destination == 0u) { return 0u; }
    let denominator = projectionAdd(vec2<u32>(divideWord(65535u, source), 0u), vec2<u32>(divideWord(65535u, destination), 0u));
    // 2*65535^2 and the sum of the two rounded reciprocals need 33 bits.
    // A 33-bit denominator gives a quotient of at most one. Otherwise the
    // carry before each remainder shift makes exact 33/32 division possible.
    let low = 0xfffc0002u;
    if (denominator.y != 0u) { return select(0u, 1u, low >= denominator.x); }
    var remainder = 1u;
    var quotient = 0u;
    for (var bit = 31; bit >= 0; bit -= 1) {
        let carry = remainder >> 31u;
        remainder = (remainder << 1u) | ((low >> u32(bit)) & 1u);
        if (carry != 0u || remainder >= denominator.x) {
            remainder -= denominator.x;
            quotient |= 1u << u32(bit);
        }
    }
    return min(65535u, quotient);
}
fn normalizedWordColor(word: u32) -> Word64 {
    // The native lookup divides in binary32 before promoting the color.
    return doubleFromFloat(projectionDivide(bitcast<u32>(f32(word)), 0x477fff00u));
}
fn scaledWordColor(value: Word64) -> u32 {
    if ((value.y & 0x80000000u) != 0u) { return 0u; }
    let maximum = Word64(0u, 0x40efffe0u);
    let scaled = doubleMultiply(value, maximum);
    if (!wordLessEqual(scaled, maximum)) { return 65535u; }
    return doubleTruncate(doubleAdd(scaled, Word64(0u, 0x3fe00000u)));
}
fn moduloUnit(value: Word64) -> Word64 {
    let divisor = Word64(1u, 0x3ff00000u);
    let quotient = doubleFloor(doubleDivide(value, divisor));
    return doubleSubtract(value, doubleMultiply(quotient, divisor));
}
fn moduloShiftValue(source: Word64, destination: Word64) -> Word64 {
    if (all(source == Word64(0u, 0x3ff00000u)) && all(destination == Word64(0u))) { return Word64(0u); }
    return moduloUnit(doubleAdd(source, destination));
}
fn divisiveModuloValue(source: Word64, destination: Word64, realEpsilon: bool) -> Word64 {
    let one = Word64(0u, 0x3ff00000u);
    var inverse: Word64;
    if (all(source == Word64(0u))) { inverse = select(one, Word64(0u, 0x43300000u), realEpsilon); }
    else { inverse = doubleDivide(one, source); }
    return moduloUnit(doubleMultiply(inverse, destination));
}
fn doubleCeilingIsOdd(value: Word64) -> bool {
    let integral = doubleFloor(value);
    return ((doubleTruncate(integral) + select(0u, 1u, any(integral != value))) & 1u) != 0u;
}
fn divisiveModuloContinuousWord(sourceWord: u32, destinationWord: u32) -> u32 {
    if (destinationWord == 0u) { return 0u; }
    let source = normalizedWordColor(sourceWord);
    let destination = normalizedWordColor(destinationWord);
    var value = divisiveModuloValue(source, destination, true);
    if (sourceWord == 0u) { return doubleTruncate(value); }
    if (!doubleCeilingIsOdd(doubleDivide(destination, source))) {
        value = doubleSubtract(Word64(0u, 0x3ff00000u), value);
    }
    return scaledWordColor(value);
}
fn softLightWordColor(source: u32, destination: u32, svg: bool) -> u32 {
    let s = normalizedWordColor(source);
    let d = normalizedWordColor(destination);
    let one = Word64(0u, 0x3ff00000u);
    let two = Word64(0u, 0x40000000u);
    if (source > 32767u) {
        var curve: Word64;
        if (!svg || destination > 16383u) { curve = doubleSqrt(d); }
        else {
            curve = doubleMultiply(doubleAdd(doubleMultiply(doubleSubtract(doubleMultiply(Word64(0u, 0x40300000u), d), Word64(0u, 0x40280000u)), d), Word64(0u, 0x40100000u)), d);
        }
        return scaledWordColor(doubleAdd(d, doubleMultiply(doubleSubtract(doubleMultiply(two, s), one), doubleSubtract(curve, d))));
    }
    return scaledWordColor(doubleSubtract(d, doubleMultiply(doubleMultiply(doubleSubtract(one, doubleMultiply(two, s)), d), doubleSubtract(one, d))));
}
fn ifsWordColor(mode: u32, source: u32, destination: u32) -> u32 {
    let s = normalizedWordColor(source);
    let d = normalizedWordColor(destination);
    let one = Word64(0u, 0x3ff00000u);
    let inverseSource = doubleSubtract(one, s);
    let inverseDestination = doubleSubtract(one, d);
    switch mode {
        case 73u: { return scaledWordColor(doubleAdd(doubleMultiply(s, inverseDestination), doubleSqrt(d))); }
        case 77u: { return scaledWordColor(doubleSubtract(one, doubleAdd(doubleMultiply(inverseDestination, s), doubleSqrt(inverseSource)))); }
        case 74u: {
            if (source <= 32767u) { return scaledWordColor(doubleSubtract(doubleSubtract(one, doubleMultiply(inverseSource, s)), doubleMultiply(inverseDestination, inverseSource))); }
            return scaledWordColor(doubleAdd(doubleSubtract(s, doubleMultiply(inverseDestination, inverseSource)), doubleMultiply(inverseSource, inverseSource)));
        }
        default: {
            if (source <= 32767u) { return scaledWordColor(doubleAdd(doubleMultiply(inverseSource, s), doubleMultiply(s, d))); }
            return scaledWordColor(doubleSubtract(doubleAdd(doubleMultiply(s, d), s), doubleMultiply(s, s)));
        }
    }
}
fn gammaDarkWord(source: u32, destination: u32) -> u32 {
    if (destination == 0u || destination == 65535u) { return destination; }
    if (source == 0u) { return 0u; }
    return scaledWordColor(wordColorPower(normalizedWordColor(destination),
        doubleDivide(Word64(0u, 0x3ff00000u), normalizedWordColor(source))));
}
fn superLightWordPower(word: u32, curve: u32) -> Word64 {
    let point = vec2<i32>(i32(word & 255u), i32(word >> 8u));
    return Word64(packPixel(textureLoad(blendCurveValues, point, i32(curve * 2u), 0)),
        packPixel(textureLoad(blendCurveValues, point, i32(curve * 2u + 1u), 0)));
}
fn powerWordColor(mode: u32, source: u32, destination: u32) -> u32 {
    let s = normalizedWordColor(source);
    let d = normalizedWordColor(destination);
    let one = Word64(0u, 0x3ff00000u);
    let two = Word64(0u, 0x40000000u);
    let easyFactor = Word64(0x09f8b874u, 0x3ff0a3d7u);
    switch mode {
        case 70u, 71u: {
            if (WORD_BLEND_MODE_A != 70u && WORD_BLEND_MODE_B != 70u && WORD_BLEND_MODE_A != 71u && WORD_BLEND_MODE_B != 71u) { return source; }
            var value: Word64;
            if (mode == 70u) {
                let factor = Word64(0xaaaaaaabu, 0x4002aaaau);
                value = wordColorPower(doubleAdd(wordColorPower(doubleInteger(destination), factor),
                    wordColorPower(doubleInteger(source), factor)), Word64(0xdb6db73du, 0x3fdb6db6u));
            } else {
                let s2 = doubleInteger(source * source);
                let d2 = doubleInteger(destination * destination);
                value = doubleSqrt(doubleSqrt(doubleAdd(doubleMultiply(d2, d2), doubleMultiply(s2, s2))));
            }
            // Native P-Norm takes raw integer colors and truncates after its
            // power, rather than using normalized colors and half-up scaling.
            if (!wordLessEqual(value, Word64(0u, 0x40efffe0u))) { return 65535u; }
            return doubleTruncate(value);
        }
        case 69u: {
            if (WORD_BLEND_MODE_A != 69u && WORD_BLEND_MODE_B != 69u) { return source; } return 65535u - gammaDarkWord(65535u - source, 65535u - destination); }
        case 76u: {
            if (WORD_BLEND_MODE_A != 76u && WORD_BLEND_MODE_B != 76u) { return source; } return gammaDarkWord(source, destination); }
        case 68u: {
            if (WORD_BLEND_MODE_A != 68u && WORD_BLEND_MODE_B != 68u) { return source; }
            if (destination == 0u || destination == 65535u) { return destination; }
            return scaledWordColor(wordColorPower(d, s));
        }
        case 65u: {
            if (WORD_BLEND_MODE_A != 65u && WORD_BLEND_MODE_B != 65u) { return source; }
            return scaledWordColor(wordColorPower(d, wordColorPower(two,
                doubleMultiply(two, doubleSubtract(Word64(0u, 0x3fe00000u), s)))));
        }
        case 75u: {
            if (WORD_BLEND_MODE_A != 75u && WORD_BLEND_MODE_B != 75u) { return source; }
            if (source == 65535u) { return 65535u; }
            return scaledWordColor(wordColorPower(d, doubleMultiply(doubleSubtract(one, s), easyFactor)));
        }
        case 72u: {
            if (WORD_BLEND_MODE_A != 72u && WORD_BLEND_MODE_B != 72u) { return source; }
            let inverseFactor = Word64(0x590b2164u, 0x3fd642c8u);
            let combined = doubleAdd(superLightWordPower(destination, select(0u, 1u, source <= 32767u)),
                superLightWordPower(source, 2u));
            let value = wordColorPower(combined, inverseFactor);
            if (source <= 32767u) { return scaledWordColor(doubleSubtract(one, value)); }
            return scaledWordColor(value);
        }
        default: {
            if (WORD_BLEND_MODE_A != 79u && WORD_BLEND_MODE_B != 79u) { return source; }
            let sourceValue = select(s, Word64(0xffffdcd1u, 0x3fefffffu), source == 65535u);
            return scaledWordColor(doubleSubtract(one, wordColorPower(doubleSubtract(one, sourceValue), doubleMultiply(d, easyFactor))));
        }
    }
}
fn arcTangentWordColor(source: u32, destination: u32) -> u32 {
    if (source == 0u) { return 0u; }
    if (destination == 0u) { return 65535u; }
    let angle = positiveColorAtan(doubleDivide(normalizedWordColor(source), normalizedWordColor(destination)));
    return scaledWordColor(doubleDivide(doubleMultiply(Word64(0u, 0x40000000u), angle), Word64(0x54442d18u, 0x400921fbu)));
}
fn cosineWordColor(word: u32) -> Word64 {
    let point = vec2<i32>(i32(word & 255u), i32(word >> 8u));
    return Word64(packPixel(textureLoad(separableColorValues, point, 0, 0)),
        packPixel(textureLoad(separableColorValues, point, 1, 0)));
}
fn interpolationWordColor(source: u32, destination: u32) -> u32 {
    if (source == 0u && destination == 0u) { return 0u; }
    let quarter = Word64(0u, 0x3fd00000u);
    return scaledWordColor(doubleSubtract(doubleSubtract(Word64(0u, 0x3fe00000u),
        doubleMultiply(quarter, cosineWordColor(source))), doubleMultiply(quarter, cosineWordColor(destination))));
}
fn separableWordChannelMode(mode: u32, source: u32, destination: u32) -> u32 {
    switch mode {
        case 1u: { return mul65535(source, destination); }
        case 2u: { return source + destination - mul65535(source, destination); }
        case 3u: { return min(source, destination); }
        case 4u: { return max(source, destination); }
        case 5u, 6u: { return min(65535u, source + destination); }
        case 7u, 23u: { return u32(abs(i32(destination) - i32(source))); }
        case 8u: { return u32(clamp(i32(source + destination) - 2 * i32(mul65535(source, destination)), 0, 65535)); }
        case 9u: { return u32(max(0, i32(destination) - i32(source))); }
        case 10u, 11u: { return u32(max(0, i32(source + destination) - 65535)); }
        case 12u: { return u32(clamp(i32(source + destination) - 32767, 0, 65535)); }
        case 13u: { return u32(clamp(i32(destination) - i32(source) + 32767, 0, 65535)); }
        case 14u, 15u: {
            let s = select(source, destination, mode == 14u);
            let d = select(destination, source, mode == 14u);
            if (s == 32767u) { return d; }
            if (s > 32767u) { return 2u * s - 65535u + d - mul65535(2u * s - 65535u, d); }
            return mul65535(2u * s, d);
        }
        case 16u: { return u32(max(i32(2u * source) - 65535, min(i32(destination), i32(2u * source)))); }
        case 17u: {
            if (destination == 0u) { return 0u; }
            if (source == 0u) { return 65535u; }
            return min(65535u, divideWord(destination, source));
        }
        case 18u: { return dodgeWord(source, destination); }
        case 19u: { return burnWord(source, destination); }
        case 20u: {
            if (destination > 32767u) { return dodgeWord(source, destination); }
            return burnWord(source, destination);
        }
        case 21u: {
            if (source < 32767u) {
                if (source == 0u) { return select(0u, 65535u, destination == 65535u); }
                return 65535u - min(65535u, (65535u - destination) * 65535u / (2u * source));
            }
            if (source == 65535u) { return select(65535u, 0u, destination == 0u); }
            return min(65535u, destination * 65535u / (2u * (65535u - source)));
        }
        case 22u: { return u32(clamp(i32(2u * source + destination) - 65535, 0, 65535)); }
        case 24u: { return (source + destination) * 32767u / 65535u; }
        case 25u: { return select(0u, 65535u, source + destination > 65535u); }
        case 31u: { return source ^ destination; }
        case 32u: { return source | destination; }
        case 33u: { return source & destination; }
        case 34u: { return 65535u ^ (source & destination); }
        case 35u: { return 65535u ^ (source | destination); }
        case 36u: { return 65535u ^ (source ^ destination); }
        case 37u: { return source | (65535u - destination); }
        case 38u: { return (65535u - source) & destination; }
        case 39u: { return (65535u - source) | destination; }
        case 40u: { return source & (65535u - destination); }
        case 43u: { return glowWord(destination, source); }
        case 44u: { return glowWord(source, destination); }
        case 45u: { return heatWord(destination, source); }
        case 46u: { return heatWord(source, destination); }
        case 47u: { return glowHeatWord(source, destination); }
        case 48u: { return heatGlowWord(source, destination); }
        case 49u: { return glowHeatWord(destination, source); }
        case 50u: { return freezeReflectWord(source, destination); }
        case 51u: { return (freezeReflectWord(source, destination) + heatGlowWord(source, destination)) * 32767u / 65535u; }
        case 52u: { return parallelWord(source, destination); }
        case 53u: {
            if (destination == 0u) { return 0u; }
            if (source == 65535u) { return 65535u; }
            if (source >= 32767u) { return min(65535u, destination * 65535u / (2u * (65535u - source))); }
            return destination * 2u * source / 65535u;
        }
        case 54u: { return min(65535u, mul65535(destination, source + destination - mul65535(source, destination)) +
            mul65535(mul65535(source, destination), 65535u - destination)); }
        case 55u: { return 65535u - u32(abs(65535 - i32(source) - i32(destination))); }
        case 56u: { return penumbraWord(destination, source); }
        case 57u: { return penumbraWord(source, destination); }
        case 58u: {
            if (source == 0u) { return 0u; }
            if (destination > source) { return penumbraWord(source, destination); }
            return penumbraWord(destination, source);
        }
        case 59u: { return u32(clamp(3 * i32(destination) - 2 * (65535 - i32(source)), 0, 65535)); }
        case 60u: { return scaledWordColor(doubleSqrt(doubleMultiply(normalizedWordColor(source), normalizedWordColor(destination)))); }
        case 65u, 68u, 69u, 70u, 71u, 72u, 75u, 76u, 79u: {
            if ((WORD_BLEND_MODE_A >= 65u && WORD_BLEND_MODE_A <= 79u) || (WORD_BLEND_MODE_B >= 65u && WORD_BLEND_MODE_B <= 79u)) {
                return powerWordColor(mode, source, destination);
            }
            return source;
        }
        case 61u: {
            if (WORD_BLEND_MODE_A != 61u && WORD_BLEND_MODE_B != 61u) { return source; } return interpolationWordColor(source, destination); }
        case 62u: {
            if (WORD_BLEND_MODE_A != 62u && WORD_BLEND_MODE_B != 62u) { return source; }
            let first = interpolationWordColor(source, destination);
            return interpolationWordColor(first, first);
        }
        case 63u: {
            if (WORD_BLEND_MODE_A != 63u && WORD_BLEND_MODE_B != 63u) { return source; }
            if (source == 65535u) { return 65535u; }
            return arcTangentWordColor(destination, 65535u - source);
        }
        case 64u: {
            if (WORD_BLEND_MODE_A != 64u && WORD_BLEND_MODE_B != 64u) { return source; }
            if (destination == 65535u) { return 65535u; }
            return arcTangentWordColor(source, 65535u - destination);
        }
        case 86u: {
            if (WORD_BLEND_MODE_A != 86u && WORD_BLEND_MODE_B != 86u) { return source; } return arcTangentWordColor(source, destination); }
        case 66u: { return softLightWordColor(source, destination, true); }
        case 67u: { return softLightWordColor(source, destination, false); }
        case 73u, 74u, 77u, 78u: { return ifsWordColor(mode, source, destination); }
        case 87u: { return scaledWordColor(doubleMagnitude(doubleSubtract(doubleSqrt(normalizedWordColor(destination)), doubleSqrt(normalizedWordColor(source))))); }
        case 80u: { return destination % (source + 1u); }
        case 81u: { return mul65535(divisiveModuloContinuousWord(source, destination), source); }
        case 82u: { return scaledWordColor(divisiveModuloValue(normalizedWordColor(source), normalizedWordColor(destination), false)); }
        case 83u: { return divisiveModuloContinuousWord(source, destination); }
        case 84u: { return scaledWordColor(moduloShiftValue(normalizedWordColor(source), normalizedWordColor(destination))); }
        case 85u: {
            if (source == 65535u && destination == 0u) { return 65535u; }
            let s = normalizedWordColor(source);
            let d = normalizedWordColor(destination);
            var value = moduloShiftValue(s, d);
            if (destination != 0u && !doubleCeilingIsOdd(doubleAdd(s, d))) {
                value = doubleSubtract(Word64(0u, 0x3ff00000u), value);
            }
            return scaledWordColor(value);
        }
        default: { return source; }
    }
}
fn appliedWordAlpha(source: u32, selection: u32, opacity: u32, mode: u32) -> u32 {
    if (mode == 26u) { return mul65535(mul65535(source, selection), opacity); }
    if (mode >= 28u && mode <= 30u) { return mul65535(source, mul65535(selection, opacity)); }
    return multiplyThreeWords(source, selection, opacity);
}
fn separableWordChannel(mode: u32, source: u32, destination: u32) -> u32 {
    if (WORD_BLEND_MODE_A != 0u && mode == WORD_BLEND_MODE_A) {
        return separableWordChannelMode(WORD_BLEND_MODE_A, source, destination);
    }
    if (WORD_BLEND_MODE_B != 0u && mode == WORD_BLEND_MODE_B) {
        return separableWordChannelMode(WORD_BLEND_MODE_B, source, destination);
    }
    return source;
}
fn separableRgba16Pixel(base: vec4<u32>, source: vec4<u32>, alpha: u32, mask: u32, mode: u32) -> vec4<u32> {
    if (!SEPARABLE_BLEND_ENABLED) { return base; }
    var destination = base;
    let oldAlpha = base.w;
    if (mode == 26u) { return vec4<u32>(base.xyz, mul65535(65535u - alpha, oldAlpha)); }
    if (mask != 15u && oldAlpha == 0u) { destination = vec4<u32>(0u); }
    let locked = (mask & 8u) == 0u;
    if (mode >= 28u && mode <= 30u) {
        let channel = mode - 28u;
        if ((mask & (1u << channel)) != 0u) { destination[channel] = interpolateWord(destination[channel], source[channel], alpha); }
        return destination;
    }
    if (mode == 41u || mode == 42u) {
        if (mode == 41u) {
            if (!locked) { destination.w = mul65535(oldAlpha, alpha); }
        } else {
            if (source.w != 0u) { for (var channel = 0u; channel < 3u; channel += 1u) {
                if ((mask & (1u << channel)) != 0u) { destination[channel] = interpolateWord(source[channel], destination[channel], oldAlpha); }
            } }
            if (!locked) { destination.w = alpha; }
        }
        return destination;
    }
    if (alpha == 0u) { return destination; }
    if (mode == 27u) {
        if (oldAlpha == 65535u) { return destination; }
        let unionAlpha = oldAlpha + alpha - mul65535(oldAlpha, alpha);
        for (var channel = 0u; channel < 3u; channel += 1u) {
            if ((mask & (1u << channel)) == 0u) { continue; }
            if (oldAlpha == 0u) { destination[channel] = source[channel]; }
            else {
                let blended = interpolateWord(mul65535(source[channel], alpha), destination[channel], oldAlpha);
                destination[channel] = divideWord(blended, unionAlpha) & 65535u;
            }
        }
        if (!locked) { destination.w = unionAlpha; }
        return destination;
    }
    let nextAlpha = select(alpha + oldAlpha - mul65535(alpha, oldAlpha), oldAlpha, locked);
    for (var i = 0u; i < 3u; i += 1u) {
        if ((mask & (1u << i)) == 0u) { continue; }
        let product = separableWordChannel(mode, source[i], destination[i]);
        if (locked || oldAlpha == 65535u) {
            if (oldAlpha != 0u) { destination[i] = interpolateWord(destination[i], product, alpha); }
        } else if (oldAlpha == 0u) { destination[i] = source[i]; }
        else if (alpha == 65535u) { destination[i] = interpolateWord(source[i], product, oldAlpha); }
        else {
            let value = (multiplyThreeWords(65535u - alpha, oldAlpha, destination[i]) +
                multiplyThreeWords(65535u - oldAlpha, alpha, source[i]) + multiplyThreeWords(oldAlpha, alpha, product)) & 65535u;
            destination[i] = divideWord(value, nextAlpha) & 65535u;
        }
    }
    destination.w = nextAlpha;
    return destination;
}
fn specialAlphaRgba16PixelMode(base: vec4<u32>, source: vec4<u32>, opacity: u32, mask: u32, selection: u32, mode: u32) -> vec4<u32> {
    var destination = base;
    let oldAlpha = base.w;
    let locked = (mask & 8u) == 0u;
    if (mask != 15u && oldAlpha == 0u) { destination = vec4<u32>(0u); }
    if (mode == 127u) {
        let selected = mul65535(source.w, selection);
        let alpha = mul65535(selected, opacity);
        var weight = 65535u;
        if (oldAlpha != 0u) { weight = min(65535u, divideWord(alpha, oldAlpha)); }
        for (var i = 0u; i < 3u; i += 1u) {
            if ((mask & (1u << i)) != 0u) { destination[i] = interpolateWord(destination[i], source[i], weight); }
        }
        if (!locked && opacity > oldAlpha) { destination.w = interpolateWord(oldAlpha, opacity, selected); }
        return destination;
    }
    let alpha = multiplyThreeWords(source.w, selection, opacity);
    if (mode == 128u) {
        if (alpha == 0u || oldAlpha == 65535u) { return destination; }
        let d = projectionDivide(bitcast<u32>(f32(oldAlpha)), 0x477fff00u);
        let a = projectionDivide(bitcast<u32>(f32(alpha)), 0x477fff00u);
        let difference = floatSubtract(d, a);
        let one = Word64(0u, 0x3ff00000u);
        let weight = floatFromDouble(doubleDivide(one, doubleAdd(one, finiteColorExp(doubleMultiply(Word64(0u, 0xc0440000u), doubleFromFloat(difference))))));
        let first = floatMultiply(d, weight);
        let second = doubleMultiply(doubleFromFloat(a), doubleSubtract(one, doubleFromFloat(weight)));
        var combined = floatFromDouble(doubleAdd(doubleFromFloat(first), second));
        if (colorLess(combined, 0u)) { combined = 0u; }
        if (colorLess(COLOR_ONE, combined)) { combined = COLOR_ONE; }
        if (colorLess(combined, d)) { combined = d; }
        let inverseAlpha = floatSubtract(COLOR_ONE, combined);
        let inverseDestination = floatSubtract(COLOR_ONE, d);
        let denominator = floatAdd(inverseDestination, bitcast<u32>(1e-16f));
        let fakeOpacity = floatSubtract(COLOR_ONE, projectionDivide(inverseAlpha, denominator));
        let nextAlpha = colorComponentWord(combined);
        let colorWeight = colorComponentWord(fakeOpacity);
        for (var i = 0u; i < 3u; i += 1u) {
            if ((mask & (1u << i)) == 0u) { continue; }
            if (oldAlpha == 0u) { destination[i] = source[i]; }
            else if (nextAlpha != 0u) {
                let mixed = interpolateWord(mul65535(destination[i], oldAlpha), source[i], colorWeight);
                destination[i] = min(65535u, divideWord(mixed, nextAlpha));
            }
        }
        if (!locked) { destination.w = nextAlpha; }
        return destination;
    }
    let nextAlpha = alpha + oldAlpha - mul65535(alpha, oldAlpha);
    if (select(nextAlpha != 0u, oldAlpha != 0u, locked)) {
        let a = projectionDivide(bitcast<u32>(f32(alpha)), 0x477fff00u);
        for (var i = 0u; i < 3u; i += 1u) {
            if ((mask & (1u << i)) == 0u) { continue; }
            let s = projectionDivide(bitcast<u32>(f32(source[i])), 0x477fff00u);
            let d = projectionDivide(bitcast<u32>(f32(destination[i])), 0x477fff00u);
            destination[i] = colorComponentWord(floatAdd(floatMultiply(s, a), d));
        }
    }
    if (!locked) { destination.w = nextAlpha; }
    return destination;
}
fn specialAlphaRgba16Pixel(base: vec4<u32>, source: vec4<u32>, opacity: u32, mask: u32, selection: u32, mode: u32) -> vec4<u32> {
    if ((WORD_BLEND_MODE_A >= 127u && WORD_BLEND_MODE_A <= 129u) && mode == WORD_BLEND_MODE_A) {
        return specialAlphaRgba16PixelMode(base, source, opacity, mask, selection, WORD_BLEND_MODE_A);
    }
    if ((WORD_BLEND_MODE_B >= 127u && WORD_BLEND_MODE_B <= 129u) && mode == WORD_BLEND_MODE_B) {
        return specialAlphaRgba16PixelMode(base, source, opacity, mask, selection, WORD_BLEND_MODE_B);
    }
    return base;
}

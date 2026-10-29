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
fn separableWordChannel(mode: u32, source: u32, destination: u32) -> u32 {
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
        default: { return source; }
    }
}
fn appliedWordAlpha(source: u32, selection: u32, opacity: u32, mode: u32) -> u32 {
    if (mode == 26u) { return mul65535(mul65535(source, selection), opacity); }
    if (mode >= 28u && mode <= 30u) { return mul65535(source, mul65535(selection, opacity)); }
    return multiplyThreeWords(source, selection, opacity);
}
fn separableRgba16Pixel(base: vec4<u32>, source: vec4<u32>, alpha: u32, mask: u32, mode: u32) -> vec4<u32> {
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

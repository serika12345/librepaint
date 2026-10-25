// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Integer separable RGBA8 blending shared by layer composition and fill.
// Mode values match LayerBlendMode: Normal=0, Multiply=1, Screen=2,
// Darken=3, Lighten=4, Addition=5, LinearDodge=6.

fn multiplyByte(a: u32, b: u32) -> u32 {
    let product = a * b + 128u;
    return ((product >> 8u) + product) >> 8u;
}
fn multiplyThreeBytes(a: u32, b: u32, c: u32) -> u32 {
    let product = a * b * c + 0x7f5bu;
    return ((product >> 7u) + product) >> 16u;
}
fn blendByte(destination: u32, source: u32, alpha: u32) -> u32 {
    let weighted = (i32(source) - i32(destination)) * i32(alpha) + 128;
    return u32((((weighted >> 8u) + weighted) >> 8u) + i32(destination)) & 255u;
}
fn separableProjectionPixel(base: vec4<u32>, source: vec4<u32>, opacity: u32, mask: u32, mode: u32) -> u32 {
    var destination = base;
    if (mask != 15u && destination.w == 0u) {
        destination = vec4<u32>(0u);
    }
    let alpha = multiplyThreeBytes(source.w, 255u, opacity);
    if (alpha == 0u) {
        return packPixel(destination);
    }
    let locked = (mask & 8u) == 0u;
    let oldAlpha = destination.w;
    var nextAlpha = oldAlpha;
    if (!locked) {
        nextAlpha = alpha + oldAlpha - multiplyByte(alpha, oldAlpha);
    }
    for (var channel = 0u; channel < 3u; channel += 1u) {
        if ((mask & (1u << channel)) == 0u) {
            continue;
        }
        let multiplied = multiplyByte(source[channel], destination[channel]);
        var product = multiplied;
        switch mode {
            case 2u: {
                product = source[channel] + destination[channel] - multiplied;
            }
            case 3u: {
                product = min(source[channel], destination[channel]);
            }
            case 4u: {
                product = max(source[channel], destination[channel]);
            }
            case 5u, 6u: {
                product = min(255u, source[channel] + destination[channel]);
            }
            default: {}
        }
        if (locked || oldAlpha == 255u) {
            if (oldAlpha != 0u) {
                destination[channel] = blendByte(destination[channel], product, alpha);
            }
        } else if (oldAlpha == 0u) {
            destination[channel] = source[channel];
        } else if (alpha == 255u) {
            destination[channel] = blendByte(source[channel], product, oldAlpha);
        } else {
            let result = (multiplyThreeBytes(255u - alpha, oldAlpha, destination[channel]) +
                multiplyThreeBytes(255u - oldAlpha, alpha, source[channel]) +
                multiplyThreeBytes(oldAlpha, alpha, product)) & 255u;
            destination[channel] = ((result * 255u + nextAlpha / 2u) / nextAlpha) & 255u;
        }
    }
    destination.w = nextAlpha;
    return packPixel(destination);
}

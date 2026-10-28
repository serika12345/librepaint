// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Integer separable RGBA8 blending shared by layer composition and fill.
// Mode values match LayerBlendMode: Normal=0, Multiply=1, Screen=2,
// Darken=3, Lighten=4, Addition=5, LinearDodge=6,
// Difference=7, Exclusion=8, Subtract=9, LinearBurn=10,
// InverseSubtract=11, GrainMerge=12, GrainExtract=13, Overlay=14,
// HardLight=15, PinLight=16, Divide=17, ColorDodge=18, ColorBurn=19,
// HardMix=20, VividLight=21, LinearLight=22, Equivalence=23,
// Allanon=24, HardMixPhotoshop=25.

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
            case 7u: {
                let high = max(source[channel], destination[channel]);
                let low = min(source[channel], destination[channel]);
                product = high - low;
            }
            case 8u: {
                let sum = source[channel] + destination[channel];
                let twice = 2u * multiplied;
                if (sum > twice) { product = min(255u, sum - twice); } else { product = 0u; }
            }
            case 9u: {
                if (destination[channel] > source[channel]) {
                    product = destination[channel] - source[channel];
                } else {
                    product = 0u;
                }
            }
            case 10u, 11u: {
                let sum = source[channel] + destination[channel];
                if (sum > 255u) { product = sum - 255u; } else { product = 0u; }
            }
            case 12u: {
                let sum = source[channel] + destination[channel];
                if (sum > 128u) { product = min(255u, sum - 128u); } else { product = 0u; }
            }
            case 13u: {
                let sum = destination[channel] + 128u;
                if (sum > source[channel]) { product = min(255u, sum - source[channel]); } else { product = 0u; }
            }
            case 14u: {
                if (source[channel] == 127u) { product = destination[channel]; }
                else if (source[channel] > 127u) {
                    let drive = 2u * source[channel] - 255u;
                    product = drive + destination[channel] - (drive * destination[channel]) / 255u;
                } else {
                    product = (2u * source[channel] * destination[channel]) / 255u;
                }
            }
            case 15u: {
                if (destination[channel] == 127u) { product = source[channel]; }
                else if (destination[channel] > 127u) {
                    let drive = 2u * destination[channel] - 255u;
                    product = drive + source[channel] - (drive * source[channel]) / 255u;
                } else {
                    product = (2u * destination[channel] * source[channel]) / 255u;
                }
            }
            case 16u: {
                let twice = 2u * source[channel];
                let lower = min(destination[channel], twice);
                var upper = 0u;
                if (twice > 255u) { upper = twice - 255u; }
                product = max(upper, lower);
            }
            case 17u: {
                if (destination[channel] == 0u) { product = 0u; }
                else if (source[channel] == 0u) { product = 255u; }
                else { product = min(255u, (destination[channel] * 255u) / source[channel]); }
            }
            case 18u: {
                if (source[channel] == 255u) {
                    if (destination[channel] == 0u) { product = 0u; } else { product = 255u; }
                } else {
                    product = min(255u, (destination[channel] * 255u) / (255u - source[channel]));
                }
            }
            case 19u: {
                if (destination[channel] == 255u) { product = 255u; }
                else if (source[channel] == 0u) { product = 0u; }
                else {
                    let burned = min(255u, ((255u - destination[channel]) * 255u) / source[channel]);
                    product = 255u - burned;
                }
            }
            case 20u: {
                if (destination[channel] > 127u) {
                    if (source[channel] == 255u) {
                        if (destination[channel] == 0u) { product = 0u; } else { product = 255u; }
                    } else {
                        product = min(255u, (destination[channel] * 255u) / (255u - source[channel]));
                    }
                } else {
                    if (destination[channel] == 255u) { product = 255u; }
                    else if (source[channel] == 0u) { product = 0u; }
                    else {
                        let burned = min(255u, ((255u - destination[channel]) * 255u) / source[channel]);
                        product = 255u - burned;
                    }
                }
            }
            case 21u: {
                if (source[channel] < 127u) {
                    if (source[channel] == 0u) {
                        if (destination[channel] == 255u) { product = 255u; } else { product = 0u; }
                    } else {
                        let inverted = ((255u - destination[channel]) * 255u) / (2u * source[channel]);
                        if (inverted >= 255u) { product = 0u; } else { product = 255u - inverted; }
                    }
                } else if (source[channel] == 255u) {
                    if (destination[channel] == 0u) { product = 0u; } else { product = 255u; }
                } else {
                    product = min(255u, (destination[channel] * 255u) / (2u * (255u - source[channel])));
                }
            }
            case 22u: {
                let linearSum = 2u * source[channel] + destination[channel];
                if (linearSum > 255u) { product = min(255u, linearSum - 255u); } else { product = 0u; }
            }
            case 23u: {
                if (destination[channel] > source[channel]) {
                    product = destination[channel] - source[channel];
                } else {
                    product = source[channel] - destination[channel];
                }
            }
            case 24u: {
                product = ((source[channel] + destination[channel]) * 127u) / 255u;
            }
            case 25u: {
                let mixSum = source[channel] + destination[channel];
                if (mixSum > 255u) { product = 255u; } else { product = 0u; }
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

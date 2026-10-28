// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Integer RGBA8 blending shared by layer composition, fill and stroke coverage.
// Mode values match LayerBlendMode: Normal=0, Multiply=1, Screen=2,
// Darken=3, Lighten=4, Addition=5, LinearDodge=6,
// Difference=7, Exclusion=8, Subtract=9, LinearBurn=10,
// InverseSubtract=11, GrainMerge=12, GrainExtract=13, Overlay=14,
// HardLight=15, PinLight=16, Divide=17, ColorDodge=18, ColorBurn=19,
// HardMix=20, VividLight=21, LinearLight=22, Equivalence=23,
// Allanon=24, HardMixPhotoshop=25, Erase=26, Behind=27,
// CopyRed=28, CopyGreen=29, CopyBlue=30, BitwiseXor=31, BitwiseOr=32,
// BitwiseAnd=33, BitwiseNand=34, BitwiseNor=35, BitwiseXnor=36,
// Implication=37, NotImplication=38, Converse=39, NotConverse=40,
// DestinationIn=41, DestinationAtop=42, Reflect=43, Glow=44, Freeze=45, Heat=46,
// GlowHeat=47, HeatGlow=48, ReflectFreeze=49, FreezeReflect=50, QuadraticHybrid=51,
// Parallel=52, HardOverlay=53, SoftLightPegtopDelphi=54, Negation=55,
// PenumbraA=56, PenumbraB=57, FlatLight=58, HardMixSofterPhotoshop=59.
// Nonlinear RGBA8 modes GeometricMean through AdditiveSubtractive occupy 60..87.
@group(0) @binding(9) var nonlinearRgba8: texture_2d_array<u32>;

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
fn divideByte(a: u32, b: u32) -> u32 { return (a * 255u + b / 2u) / b; }
fn glowByte(source: u32, destination: u32) -> u32 {
    if (destination == 255u) { return 255u; }
    return min(255u, divideByte(multiplyByte(source, source), 255u - destination));
}
fn heatByte(source: u32, destination: u32) -> u32 {
    if (source == 255u) { return 255u; }
    if (destination == 0u) { return 0u; }
    return 255u - min(255u, divideByte(multiplyByte(255u - source, 255u - source), destination));
}
fn heatGlowByte(source: u32, destination: u32) -> u32 {
    if (source + destination > 255u) { return heatByte(source, destination); }
    if (source == 0u) { return 0u; }
    return glowByte(source, destination);
}
fn freezeReflectByte(source: u32, destination: u32) -> u32 {
    if (source + destination > 255u) { return heatByte(destination, source); }
    if (destination == 0u) { return 0u; }
    return glowByte(destination, source);
}
fn glowHeatByte(source: u32, destination: u32) -> u32 {
    if (destination == 255u) { return 255u; }
    if (source + destination > 255u) { return glowByte(source, destination); }
    return heatByte(source, destination);
}
fn penumbraByte(source: u32, destination: u32) -> u32 {
    if (destination == 255u) { return 255u; }
    if (source + destination < 255u) { return min(255u, divideByte(source, 255u - destination)) / 2u; }
    if (source == 0u) { return 0u; }
    return 255u - min(255u, divideByte(255u - destination, source) / 2u);
}
fn quantizedBlendChannel(mode: u32, source: u32, destination: u32) -> u32 {
    switch mode {
        case 43u: { return glowByte(destination, source); }
        case 44u: { return glowByte(source, destination); }
        case 45u: { return heatByte(destination, source); }
        case 46u: { return heatByte(source, destination); }
        case 47u: { return glowHeatByte(source, destination); }
        case 48u: { return heatGlowByte(source, destination); }
        case 49u: { return glowHeatByte(destination, source); }
        case 50u: { return freezeReflectByte(source, destination); }
        case 51u: { return (freezeReflectByte(source, destination) + heatGlowByte(source, destination)) * 127u / 255u; }
        case 52u: {
            if (source == 0u || destination == 0u) { return 0u; }
            return min(255u, 130050u / (divideByte(255u, source) + divideByte(255u, destination)));
        }
        case 53u: {
            if (destination == 0u) { return 0u; }
            if (source == 255u) { return 255u; }
            if (source >= 127u) { return min(255u, destination * 255u / (2u * (255u - source))); }
            return destination * 2u * source / 255u;
        }
        case 54u: {
            return min(255u, multiplyByte(destination, source + destination - multiplyByte(source, destination)) +
                             multiplyByte(multiplyByte(source, destination), 255u - destination));
        }
        case 55u: { return 255u - u32(abs(255 - i32(source) - i32(destination))); }
        case 56u: { return penumbraByte(destination, source); }
        case 57u: { return penumbraByte(source, destination); }
        case 58u: {
            if (source == 0u) { return 0u; }
            if (destination > source) { return penumbraByte(source, destination); }
            return penumbraByte(destination, source);
        }
        case 59u: { return u32(clamp(3 * i32(destination) - 2 * (255 - i32(source)), 0, 255)); }
        default: {
            if (mode >= 60u && mode <= 87u) {
                let index = mode - 60u;
                return textureLoad(nonlinearRgba8, vec2<i32>(i32(destination), i32(source)), i32(index / 4u), 0)[index % 4u];
            }
            return multiplyByte(source, destination);
        }
    }
}
fn destinationRgba8(base: vec4<u32>, source: vec4<u32>, alpha: u32, mask: u32, mode: u32) -> u32 {
    var destination = base;
    if (mask != 15u && destination.w == 0u) { destination = vec4<u32>(0u); }
    let locked = (mask & 8u) == 0u;
    if (mode == 41u) {
        if (!locked) { destination.w = multiplyByte(destination.w, alpha); }
    } else {
        if (source.w != 0u) {
            for (var channel = 0u; channel < 3u; channel += 1u) {
                if ((mask & (1u << channel)) != 0u) {
                    destination[channel] = blendByte(source[channel], destination[channel], destination.w);
                }
            }
        }
        if (!locked) { destination.w = alpha; }
    }
    return packPixel(destination);
}
fn separableProjectionPixel(base: vec4<u32>, source: vec4<u32>, opacity: u32, mask: u32, mode: u32) -> u32 {
    if (colorIsCoupled(mode)) { return coupledRgba8Pixel(base, source, multiplyThreeBytes(source.w, 255u, opacity), mask, mode); }
    if (mode == 41u || mode == 42u) {
        return destinationRgba8(base, source, multiplyThreeBytes(source.w, 255u, opacity), mask, mode);
    }
    var destination = base;
    if (mask != 15u && destination.w == 0u) {
        destination = vec4<u32>(0u);
    }
    let alpha = multiplyThreeBytes(source.w, 255u, opacity);
    let locked = (mask & 8u) == 0u;
    let oldAlpha = destination.w;
    if (mode >= 28u && mode <= 30u) {
        let channel = mode - 28u;
        if ((mask & (1u << channel)) != 0u) {
            destination[channel] = blendByte(destination[channel], source[channel], multiplyByte(source.w, opacity));
        }
        return packPixel(destination);
    }
    if (alpha == 0u) { return packPixel(destination); }
    if (mode == 26u) {
        // Erase lowers the destination alpha and leaves the color channels alone,
        // exactly like the CPU operation, which ignores channel flags.
        let erased = multiplyByte(source.w, opacity);
        destination.w = multiplyByte(255u - erased, oldAlpha);
        return packPixel(destination);
    }
    if (mode == 27u) {
        if (oldAlpha == 255u) { return packPixel(destination); }
        let unionAlpha = oldAlpha + alpha - multiplyByte(oldAlpha, alpha);
        for (var channel = 0u; channel < 3u; channel += 1u) {
            if ((mask & (1u << channel)) == 0u) { continue; }
            if (oldAlpha == 0u) { destination[channel] = source[channel]; }
            else {
                let blended = blendByte(multiplyByte(source[channel], alpha), destination[channel], oldAlpha);
                destination[channel] = ((blended * 255u + unionAlpha / 2u) / unionAlpha) & 255u;
            }
        }
        if (!locked) { destination.w = unionAlpha; }
        return packPixel(destination);
    }
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
                if (sum > 127u) { product = min(255u, sum - 127u); } else { product = 0u; }
            }
            case 13u: {
                let sum = destination[channel] + 127u;
                if (sum > source[channel]) { product = min(255u, sum - source[channel]); } else { product = 0u; }
            }
            case 14u: {
                if (destination[channel] == 127u) { product = source[channel]; }
                else if (destination[channel] > 127u) {
                    let drive = 2u * destination[channel] - 255u;
                    product = drive + source[channel] - multiplyByte(drive, source[channel]);
                } else {
                    product = multiplyByte(2u * destination[channel], source[channel]);
                }
            }
            case 15u: {
                if (source[channel] == 127u) { product = destination[channel]; }
                else if (source[channel] > 127u) {
                    let drive = 2u * source[channel] - 255u;
                    product = drive + destination[channel] - multiplyByte(drive, destination[channel]);
                } else {
                    product = multiplyByte(2u * source[channel], destination[channel]);
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
                else { product = min(255u, (destination[channel] * 255u + source[channel] / 2u) / source[channel]); }
            }
            case 18u: {
                if (source[channel] == 255u) {
                    if (destination[channel] == 0u) { product = 0u; } else { product = 255u; }
                } else {
                    product = min(255u, (destination[channel] * 255u + (255u - source[channel]) / 2u) / (255u - source[channel]));
                }
            }
            case 19u: {
                if (destination[channel] == 255u) { product = 255u; }
                else if (source[channel] == 0u) { product = 0u; }
                else {
                    let burned = min(255u, ((255u - destination[channel]) * 255u + source[channel] / 2u) / source[channel]);
                    product = 255u - burned;
                }
            }
            case 20u: {
                if (destination[channel] > 127u) {
                    if (source[channel] == 255u) {
                        if (destination[channel] == 0u) { product = 0u; } else { product = 255u; }
                    } else {
                        product = min(255u, (destination[channel] * 255u + (255u - source[channel]) / 2u) / (255u - source[channel]));
                    }
                } else {
                    if (destination[channel] == 255u) { product = 255u; }
                    else if (source[channel] == 0u) { product = 0u; }
                    else {
                        let burned = min(255u, ((255u - destination[channel]) * 255u + source[channel] / 2u) / source[channel]);
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
            case 31u: { product = source[channel] ^ destination[channel]; }
            case 32u: { product = source[channel] | destination[channel]; }
            case 33u: { product = source[channel] & destination[channel]; }
            case 34u: { product = 255u ^ (source[channel] & destination[channel]); }
            case 35u: { product = 255u ^ (source[channel] | destination[channel]); }
            case 36u: { product = 255u ^ (source[channel] ^ destination[channel]); }
            case 37u: { product = source[channel] | (255u - destination[channel]); }
            case 38u: { product = (255u - source[channel]) & destination[channel]; }
            case 39u: { product = (255u - source[channel]) | destination[channel]; }
            case 40u: { product = source[channel] & (255u - destination[channel]); }
            default: { product = quantizedBlendChannel(mode, source[channel], destination[channel]); }
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

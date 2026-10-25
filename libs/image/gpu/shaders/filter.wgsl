// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct FilterParameters {
    // Tile origin x/y, filter kind, coverage flags.
    tile: vec4<i32>,
    // Selection rectangle x/y/width/height.
    selection: vec4<i32>,
    // Raw HSV/HSL/HSY/YUV Adjustment values: h, s, v and
    // type | legacy << 8 | colorize << 9.
    adjustment: vec4<i32>,
    // Effective document luma coefficients rounded once to binary32.
    luma: vec4<f32>,
}

@group(0) @binding(0) var<storage, read> basePixels: array<u32>;
@group(0) @binding(1) var<storage, read_write> outputPixels: array<u32>;
@group(0) @binding(2) var<uniform> parameters: FilterParameters;
@group(0) @binding(3) var<storage, read> selectionCoverage: array<u32>;

// Coverage flags. A freehand mask is always byte coverage: the renderer
// expands binary masks before binding them, matching layer composition.
const COVERAGE_PRESENT: u32 = 1u;
const COVERAGE_MASK: u32 = 2u;
const COVERAGE_ELLIPSE: u32 = 4u;
const COVERAGE_INVERTED: u32 = 8u;

// Filter kinds frozen by the accepted operation.
const FILTER_INVERT_COLORS: u32 = 0u;
const FILTER_GRAYSCALE: u32 = 1u;
const FILTER_HSV_ADJUSTMENT: u32 = 2u;

// Product HSV/HSL Adjustment modes selected by the product `type` key. HSY
// and YUV, like the product Colorize option, also use the document luma
// coefficients frozen with the operation.
const HSV_MODE_VALUE: u32 = 0u;
const HSV_MODE_LIGHTNESS: u32 = 1u;
const HSV_MODE_INTENSITY: u32 = 2u;
const HSV_MODE_LUMA: u32 = 3u;
const HSV_MODE_YUV: u32 = 4u;

const HSV_LEGACY_FLAG: i32 = 256;
const HSV_COLORIZE_FLAG: i32 = 512;

// Epsilon values from the product's modern HSV transform and legacy color
// conversion rules.
const HSV_EPSILON: f32 = 1e-9;
const CONVERSION_EPSILON: f32 = 1e-6;

// Straight RGBA8 bytes in one word: red is the low byte, alpha the high byte.
fn selectedness(pixel: vec2<i32>, index: u32, flags: u32) -> u32 {
    if ((flags & COVERAGE_PRESENT) == 0u) { return 255u; }
    let inverted = (flags & COVERAGE_INVERTED) != 0u;
    if ((flags & COVERAGE_MASK) != 0u) {
        let coverage = (selectionCoverage[index >> 2u] >> ((index & 3u) * 8u)) & 255u;
        return select(coverage, 255u - coverage, inverted);
    }
    let selection = parameters.selection;
    let outside = pixel.x < selection.x || pixel.y < selection.y ||
        pixel.x >= selection.x + selection.z || pixel.y >= selection.y + selection.w;
    var inside = !outside;
    if ((flags & COVERAGE_ELLIPSE) != 0u) {
        let center = vec2<f32>(f32(selection.x) + f32(selection.z) * 0.5,
                               f32(selection.y) + f32(selection.w) * 0.5);
        let radius = vec2<f32>(f32(selection.z) * 0.5, f32(selection.w) * 0.5);
        let relative = (vec2<f32>(f32(pixel.x) + 0.5, f32(pixel.y) + 0.5) - center) / radius;
        inside = dot(relative, relative) <= 1.0;
    }
    if (inverted) { inside = !inside; }
    return select(0u, 255u, inside);
}

// ---- HSV/HSL Adjustment ---------------------------------------------------
//
// This mirrors the product filter's `hsv_adjustment` color transformation for
// the modes whose rule is a pure RGB point operation. The raw product values
// are frozen in the operation: hue in [-180, 180] degrees, saturation and
// value in [-100, 100] percent. The CPU filter scales them by 180 and 100
// respectively before the pixel loop.
//
// Stored channels are decoded to binary32 RGB. The output is clamped and
// rounded half up to RGBA8, with alpha carried through unchanged.
//
// Neutral pixels follow the selected product mode exactly. In the modern HSV
// policy a pixel with zero chroma ignores hue and saturation and only follows
// the value slider; in the legacy compatibility mode the product converts hue
// through its pre-4.3 rule, so a neutral pixel can gain color when saturation
// rises.

fn fmodValue(a: f32, b: f32) -> f32 {
    return a - trunc(a / b) * b;
}

fn normalizeDegrees(angle: f32) -> f32 {
    var value = angle;
    if (value < 0.0) { value = 360.0 + fmodValue(value, 360.0); }
    if (value >= 360.0) { value = fmodValue(value, 360.0); }
    return value;
}

fn writeSimple(sextant: i32, x: f32, m: f32, M: f32) -> vec3<f32> {
    if (sextant == 0) { return vec3<f32>(M, x + m, m); }
    if (sextant == 1) { return vec3<f32>(x + m, M, m); }
    if (sextant == 2) { return vec3<f32>(m, M, x + m); }
    if (sextant == 3) { return vec3<f32>(m, x + m, M); }
    if (sextant == 4) { return vec3<f32>(x + m, m, M); }
    if (sextant == 5) { return vec3<f32>(M, m, x + m); }
    return vec3<f32>(0.0);
}

fn modeHasChroma(mode: u32, value: f32) -> bool {
    if (mode == HSV_MODE_VALUE) { return value > HSV_EPSILON; }
    return value > HSV_EPSILON && value < 1.0 - HSV_EPSILON;
}

fn modeValue(mode: u32, rgb: vec3<f32>, minimum: f32, maximum: f32, luma: vec3<f32>) -> f32 {
    if (mode == HSV_MODE_LIGHTNESS) { return 0.5 * (maximum + minimum); }
    if (mode == HSV_MODE_INTENSITY) { return (rgb.x + rgb.y + rgb.z) / 3.0; }
    if (mode == HSV_MODE_LUMA) {
        return rgb.x * luma.x + rgb.y * luma.y + rgb.z * luma.z;
    }
    return maximum;
}

fn modeFixChroma(mode: u32, chroma: f32, value: f32) -> f32 {
    if (mode == HSV_MODE_LUMA) { return chroma; }
    if (mode == HSV_MODE_LIGHTNESS) {
        if (value >= 0.5) { return min(chroma, 2.0 - 2.0 * value); }
        return min(chroma, 2.0 * value);
    }
    if (mode == HSV_MODE_INTENSITY) {
        let oneThird = 1.0 / 3.0;
        if (value >= oneThird) { return min(chroma, 1.5 * (1.0 - value)); }
        return min(chroma, 3.0 * value);
    }
    return min(value, chroma);
}

fn modeWrite(mode: u32, sextant: i32, x: f32, chroma: f32, value: f32,
             luma: vec3<f32>) -> vec3<f32> {
    if (mode == HSV_MODE_LUMA) {
        // The HCY policy writes the pure hue and then shifts all components by
        // the difference between the requested luma and the written luma.
        let base = writeSimple(sextant, x, 0.0, chroma);
        let shift = value - (base.x * luma.x + base.y * luma.y + base.z * luma.z);
        return base + vec3<f32>(shift);
    }
    if (mode == HSV_MODE_LIGHTNESS) {
        let maximum = value + 0.5 * chroma;
        let minimum = value - 0.5 * chroma;
        return writeSimple(sextant, x, minimum, maximum);
    }
    if (mode == HSV_MODE_INTENSITY) {
        let oneThird = 1.0 / 3.0;
        let minimum = value - oneThird * (chroma + x);
        let maximum = chroma + minimum;
        return writeSimple(sextant, x, minimum, maximum);
    }
    return writeSimple(sextant, x, value - chroma, value);
}

// The modern product transformation (`HSVTransform` with the HSV, HSL, HCI or
// HCY policy). Hue and saturation adjustments do not create chroma on a
// neutral pixel; the value adjustment moves both value and chroma toward the
// requested endpoint, exactly as the product rule does.
fn modernAdjustment(rgb: vec3<f32>, dh: f32, ds: f32, dv: f32, mode: u32,
                     luma: vec3<f32>) -> vec3<f32> {
    let maximum = max(rgb.x, max(rgb.y, rgb.z));
    let minimum = min(rgb.x, min(rgb.y, rgb.z));
    var chroma = maximum - minimum;
    var value = modeValue(mode, rgb, minimum, maximum, luma);
    var hue = 0.0;
    if (!modeHasChroma(mode, value)) {
        chroma = 0.0;
        hue = 0.0;
        if (dv < 0.0) {
            value = value * (dv + 1.0);
        } else {
            value = value + dv * (1.0 - value);
        }
    } else {
        if (chroma > HSV_EPSILON) {
            if (rgb.x == maximum) {
                hue = (rgb.y - rgb.z) / chroma;
            } else if (rgb.y == maximum) {
                hue = 2.0 + (rgb.z - rgb.x) / chroma;
            } else {
                hue = 4.0 + (rgb.x - rgb.y) / chroma;
            }
            hue = hue * 60.0;
            hue = hue + dh * 180.0;
            hue = normalizeDegrees(hue);
            if (ds > 0.0) {
                chroma = min(1.0, chroma * (1.0 + ds + 2.0 * (ds * ds)));
            } else {
                chroma = chroma * (ds + 1.0);
            }
        } else {
            hue = 0.0;
        }
        let destination = select(0.0, 1.0, dv > 0.0);
        let valueCoefficient = destination - value;
        let chromaCoefficient = 0.0 - chroma;
        let movement = abs(dv);
        value = value + movement * valueCoefficient;
        chroma = chroma + movement * chromaCoefficient;
        value = clamp(value, 0.0, 1.0);
        chroma = modeFixChroma(mode, chroma, value);
    }
    if (value <= HSV_EPSILON) { return vec3<f32>(0.0); }
    hue = hue / 60.0;
    let sextant = i32(hue);
    let fraction = hue - f32(sextant);
    var x: f32;
    if ((sextant & 1) != 0) {
        x = chroma - chroma * fraction;
    } else {
        x = chroma * fraction;
    }
    return modeWrite(mode, sextant, x, chroma, value, luma);
}

// ---- Pre-4.3 compatibility conversions -----------------------------------
//
// These reproduce KoColorConversions' RGBToHSV/HSVToRGB, RGBToHSL/HSLToRGB,
// RGBToHCI/HCIToRGB and RGBToHCY/HCYToRGB for the legacy mode. The product
// enables this path only through its `compatibilityMode` key, which is frozen
// with the values.

fn legacyRgbToHsv(rgb: vec3<f32>) -> vec3<f32> {
    let maximum = max(rgb.x, max(rgb.y, rgb.z));
    let minimum = min(rgb.x, min(rgb.y, rgb.z));
    var value = maximum;
    var saturation = 0.0;
    if (maximum > CONVERSION_EPSILON) {
        saturation = (maximum - minimum) / maximum;
    }
    var hue = -1.0;
    if (saturation < CONVERSION_EPSILON) {
        hue = -1.0;
    } else {
        let delta = maximum - minimum;
        if (rgb.x == maximum) {
            hue = (rgb.y - rgb.z) / delta;
        } else if (rgb.y == maximum) {
            hue = 2.0 + (rgb.z - rgb.x) / delta;
        } else {
            hue = 4.0 + (rgb.x - rgb.y) / delta;
        }
        hue = hue * 60.0;
        if (hue < 0.0) { hue = hue + 360.0; }
    }
    return vec3<f32>(hue, saturation, value);
}

fn legacyHsvToRgb(hsv: vec3<f32>) -> vec3<f32> {
    var hue = hsv.x;
    let saturation = hsv.y;
    let value = hsv.z;
    if (saturation < CONVERSION_EPSILON || hue == -1.0) { return vec3<f32>(value); }
    if (hue > 360.0 - CONVERSION_EPSILON) { hue = hue - 360.0; }
    hue = hue / 60.0;
    let sextant = i32(floor(hue));
    let fraction = hue - f32(sextant);
    let p = value * (1.0 - saturation);
    let q = value * (1.0 - (saturation * fraction));
    let t = value * (1.0 - (saturation * (1.0 - fraction)));
    if (sextant == 0) { return vec3<f32>(value, t, p); }
    if (sextant == 1) { return vec3<f32>(q, value, p); }
    if (sextant == 2) { return vec3<f32>(p, value, t); }
    if (sextant == 3) { return vec3<f32>(p, q, value); }
    if (sextant == 4) { return vec3<f32>(t, p, value); }
    if (sextant == 5) { return vec3<f32>(value, p, q); }
    return vec3<f32>(0.0);
}

fn legacyRgbToHsl(rgb: vec3<f32>) -> vec3<f32> {
    let maximum = max(rgb.x, max(rgb.y, rgb.z));
    let minimum = min(rgb.x, min(rgb.y, rgb.z));
    let lightness = (minimum + maximum) / 2.0;
    if (lightness <= 0.0) { return vec3<f32>(-1.0, 0.0, lightness); }
    let vm = maximum - minimum;
    if (vm <= 0.0) { return vec3<f32>(-1.0, 0.0, lightness); }
    let saturation = vm / select(2.0 - maximum - minimum, maximum + minimum, lightness <= 0.5);
    let r2 = (maximum - rgb.x) / vm;
    let g2 = (maximum - rgb.y) / vm;
    let b2 = (maximum - rgb.z) / vm;
    var hue: f32;
    if (rgb.x == maximum) {
        if (rgb.y == minimum) { hue = 5.0 + b2; } else { hue = 1.0 - g2; }
    } else if (rgb.y == maximum) {
        if (rgb.z == minimum) { hue = 1.0 + r2; } else { hue = 3.0 - b2; }
    } else {
        if (rgb.x == minimum) { hue = 3.0 + g2; } else { hue = 5.0 - r2; }
    }
    hue = hue * 60.0;
    hue = fmodValue(hue, 360.0);
    return vec3<f32>(hue, saturation, lightness);
}

fn legacyHslToRgb(hsl: vec3<f32>) -> vec3<f32> {
    var hue = hsl.x;
    let saturation = hsl.y;
    let lightness = hsl.z;
    let value = select(lightness + saturation - lightness * saturation,
                       lightness * (1.0 + saturation), lightness <= 0.5);
    if (value <= 0.0) { return vec3<f32>(0.0); }
    let minimum = lightness + lightness - value;
    let sv = (value - minimum) / value;
    hue = fmodValue(hue, 360.0);
    hue = hue / 60.0;
    let sextant = i32(hue);
    let fraction = hue - f32(sextant);
    let vsf = value * sv * fraction;
    let mid1 = minimum + vsf;
    let mid2 = value - vsf;
    if (sextant == 0) { return vec3<f32>(value, mid1, minimum); }
    if (sextant == 1) { return vec3<f32>(mid2, value, minimum); }
    if (sextant == 2) { return vec3<f32>(minimum, value, mid1); }
    if (sextant == 3) { return vec3<f32>(minimum, mid2, value); }
    if (sextant == 4) { return vec3<f32>(mid1, minimum, value); }
    if (sextant == 5) { return vec3<f32>(value, minimum, mid2); }
    return vec3<f32>(0.0);
}

fn legacyRgbToHci(rgb: vec3<f32>) -> vec3<f32> {
    let minimum = min(rgb.x, min(rgb.y, rgb.z));
    let maximum = max(rgb.x, max(rgb.y, rgb.z));
    let intensity = (rgb.x + rgb.y + rgb.z) / 3.0;
    var hue = 0.0;
    var saturation = 0.0;
    let chroma = maximum - minimum;
    if (chroma != 0.0) {
        if (maximum == rgb.x) {
            if (minimum == rgb.z) { hue = (rgb.y - rgb.z) / chroma; }
            else { hue = (rgb.y - rgb.z) / chroma + 6.0; }
        } else if (maximum == rgb.y) {
            hue = (rgb.z - rgb.x) / chroma + 2.0;
        } else if (maximum == rgb.z) {
            hue = (rgb.x - rgb.y) / chroma + 4.0;
        }
        hue = hue / 6.0;
        saturation = 1.0 - (minimum / intensity);
    }
    return vec3<f32>(hue, saturation, intensity);
}

fn legacyHciToRgb(hci: vec3<f32>) -> vec3<f32> {
    var hue = hci.x;
    let chroma = select(hci.y, 0.0, hci.y < 0.0);
    let intensity = select(hci.z, 0.0, hci.z < 0.0);
    if (hue > 1.0 || hue < 0.0) { hue = fmodValue(hue, 1.0); }
    let oneThird = 1.0 / 3.0;
    let sextant = i32(hue * 6.0);
    let x = (1.0 - abs(fmodValue(hue * 6.0, 2.0) - 1.0)) * chroma;
    var r = 0.0;
    var g = 0.0;
    var b = 0.0;
    if (sextant == 0) { r = chroma; g = x; b = 0.0; }
    else if (sextant == 1) { r = x; g = chroma; b = 0.0; }
    else if (sextant == 2) { r = 0.0; g = chroma; b = x; }
    else if (sextant == 3) { r = 0.0; g = x; b = chroma; }
    else if (sextant == 4) { r = x; g = 0.0; b = chroma; }
    else if (sextant == 5) { r = chroma; g = 0.0; b = x; }
    let m = intensity - (oneThird * (r + g + b));
    return vec3<f32>(r + m, g + m, b + m);
}

// The HCY functions are the qreal conversions used by the product's legacy
// HSY rule. The modern HSY rule uses the HCYPolicy above instead.
fn legacyRgbToHcy(rgb: vec3<f32>, luma: vec3<f32>) -> vec3<f32> {
    let minimum = min(rgb.x, min(rgb.y, rgb.z));
    let maximum = max(rgb.x, max(rgb.y, rgb.z));
    var lumaValue = luma.x * rgb.x + luma.y * rgb.y + luma.z * rgb.z;
    var chroma = maximum - minimum;
    var hue = 0.0;
    if (chroma == 0.0) {
        hue = 0.0;
    } else {
        if (maximum == rgb.x) {
            if (minimum == rgb.z) {
                hue = (rgb.y - rgb.z) / chroma;
            } else {
                hue = (rgb.y - rgb.z) / chroma + 6.0;
            }
        } else if (maximum == rgb.y) {
            hue = (rgb.z - rgb.x) / chroma + 2.0;
        } else if (maximum == rgb.z) {
            hue = (rgb.x - rgb.y) / chroma + 4.0;
        }
        hue = hue / 6.0;
    }
    if (chroma < 0.0) { chroma = 0.0; }
    if (lumaValue < 0.0) { lumaValue = 0.0; }
    return vec3<f32>(clamp(hue, 0.0, 1.0), chroma, lumaValue);
}

fn legacyHcyToRgb(hcy: vec3<f32>, luma: vec3<f32>) -> vec3<f32> {
    var hue = hcy.x;
    let chroma = hcy.y;
    let lumaValue = hcy.z;
    if (hue > 1.0 || hue < 0.0) { hue = fmodValue(hue * 2.0, 2.0) / 2.0; }
    let fract = i32(hue * 6.0);
    let x = (1.0 - abs(fmodValue(hue * 6.0, 2.0) - 1.0)) * chroma;
    var r = 0.0;
    var g = 0.0;
    var b = 0.0;
    if (fract == 0) { r = chroma; g = x; b = 0.0; }
    else if (fract == 1) { r = x; g = chroma; b = 0.0; }
    else if (fract == 2) { r = 0.0; g = chroma; b = x; }
    else if (fract == 3) { r = 0.0; g = x; b = chroma; }
    else if (fract == 4) { r = x; g = 0.0; b = chroma; }
    else if (fract == 5) { r = chroma; g = 0.0; b = x; }
    // The product conversion orders the weights R, B, G.
    let shift = lumaValue - (luma.x * r + luma.z * b + luma.y * g);
    return vec3<f32>(r + shift, g + shift, b + shift);
}

// The YUV product rule: hue scales the blue chroma, saturation the red
// chroma and value offsets luma. It has no legacy variant.
fn yuvAdjustment(rgb: vec3<f32>, dh: f32, ds: f32, dv: f32, luma: vec3<f32>) -> vec3<f32> {
    let lumaValue = rgb.x * luma.x + rgb.y * luma.y + rgb.z * luma.z;
    let chromaBlue = 0.5 * ((rgb.z - lumaValue) / (1.0 - luma.z)) + 0.5;
    let chromaRed = 0.5 * ((rgb.x - lumaValue) / (1.0 - luma.x)) + 0.5;
    let adjustedBlue = chromaBlue * (dh + 1.0);
    let adjustedRed = chromaRed * (ds + 1.0);
    let adjustedLuma = lumaValue + dv;
    let blueChroma = adjustedBlue - 0.5;
    let redChroma = adjustedRed - 0.5;
    let negB = 1.0 - luma.z;
    let negR = 1.0 - luma.x;
    let red = adjustedLuma + redChroma * (negR / 0.5);
    let green = adjustedLuma - blueChroma * ((luma.z * negB) / (0.5 * luma.y)) -
        redChroma * ((luma.x * negR) / (0.5 * luma.y));
    let blue = adjustedLuma + blueChroma * (negB / 0.5);
    return vec3<f32>(red, green, blue);
}

// The product Colorize rule: the adjusted luma becomes the HSL lightness and
// the hue and saturation sliders set the HSL hue and saturation. The product
// applies its legacy HSL conversion here regardless of the compatibility flag.
fn colorizeAdjustment(rgb: vec3<f32>, dh: f32, ds: f32, dv: f32,
                      luma: vec3<f32>) -> vec3<f32> {
    var hue = dh * 360.0;
    if (hue >= 360.0) { hue = 0.0; }
    let saturation = ds;
    var lumaValue = rgb.x * luma.x + rgb.y * luma.y + rgb.z * luma.z;
    if (dv > 0.0) {
        lumaValue = lumaValue * (1.0 - dv);
        lumaValue = lumaValue + (1.0 - (1.0 - dv));
    } else if (dv < 0.0) {
        lumaValue = lumaValue * (dv + 1.0);
    }
    return legacyHslToRgb(vec3<f32>(hue, saturation, lumaValue));
}

fn legacyAdjustment(rgb: vec3<f32>, dh: f32, ds: f32, dv: f32, mode: u32,
                    luma: vec3<f32>) -> vec3<f32> {
    if (mode == HSV_MODE_LUMA) {
        let hcy = legacyRgbToHcy(rgb, luma);
        let hue = normalizeDegrees(hcy.x * 360.0 + dh * 180.0);
        let saturation = hcy.y * (ds + 1.0);
        let lumaValue = hcy.z + dv;
        return legacyHcyToRgb(vec3<f32>(hue / 360.0, saturation, lumaValue), luma);
    }
    if (mode == HSV_MODE_LIGHTNESS) {
        let hsl = legacyRgbToHsl(rgb);
        let hue = normalizeDegrees(hsl.x + dh * 180.0);
        let saturation = hsl.y * (ds + 1.0);
        var lightness = hsl.z;
        if (dv < 0.0) {
            lightness = lightness * (dv + 1.0);
        } else {
            lightness = lightness + dv * (1.0 - lightness);
        }
        return legacyHslToRgb(vec3<f32>(hue, saturation, lightness));
    }
    if (mode == HSV_MODE_INTENSITY) {
        let hci = legacyRgbToHci(rgb);
        let rotated = hci.x * 360.0;
        let hue = normalizeDegrees(rotated + dh * 180.0);
        let saturation = hci.y * (ds + 1.0);
        let intensity = hci.z + dv;
        return legacyHciToRgb(vec3<f32>(hue / 360.0, saturation, intensity));
    }
    let hsv = legacyRgbToHsv(rgb);
    let hue = normalizeDegrees(hsv.x + dh * 180.0);
    let saturation = hsv.y + ds;
    let value = hsv.z + dv;
    return legacyHsvToRgb(vec3<f32>(hue, saturation, value));
}

fn adjustChannel(value: f32) -> u32 {
    return u32(floor(clamp(value * 255.0, 0.0, 255.0) + 0.5));
}

fn hsvAdjustmentPixel(base: vec4<u32>, adjustment: vec4<i32>) -> vec4<u32> {
    let mode = u32(adjustment.w & 255);
    let legacy = (adjustment.w & HSV_LEGACY_FLAG) != 0;
    let colorize = (adjustment.w & HSV_COLORIZE_FLAG) != 0;
    var dh = f32(adjustment.x);
    let ds = f32(adjustment.y) / 100.0;
    let dv = f32(adjustment.z) / 100.0;
    // The product scales the hue slider by 180 for the HSV/HSL/HSI/HSY
    // adjustments and by 100 for YUV. Colorize uses the displayed 0..360
    // range for types 0 through 3 and the 0..100 Yellow-Blue range for YUV.
    if (colorize) {
        dh = dh / select(360.0, 100.0, mode == HSV_MODE_YUV);
    } else if (mode == HSV_MODE_YUV) {
        dh = dh / 100.0;
    } else {
        dh = dh / 180.0;
    }
    let luma = vec3<f32>(parameters.luma.x, parameters.luma.y, parameters.luma.z);
    let rgb = vec3<f32>(f32(base.x) / 255.0, f32(base.y) / 255.0, f32(base.z) / 255.0);
    var result: vec3<f32>;
    if (colorize) {
        result = colorizeAdjustment(rgb, dh, ds, dv, luma);
    } else if (mode == HSV_MODE_YUV) {
        result = yuvAdjustment(rgb, dh, ds, dv, luma);
    } else if (legacy) {
        result = legacyAdjustment(rgb, dh, ds, dv, mode, luma);
    } else {
        result = modernAdjustment(rgb, dh, ds, dv, mode, luma);
    }
    // Alpha is carried through unchanged, matching the product transformation
    // and the other GPU filter kinds.
    return vec4<u32>(adjustChannel(result.x), adjustChannel(result.y),
                     adjustChannel(result.z), base.w);
}

// Grayscale is the Rec.709 luminance of the stored sRGB-encoded RGBA8 bytes:
// 0.2126 R + 0.7152 G + 0.0722 B. The integer coefficients sum to 10000, so
// neutral gray (R = G = B) is preserved exactly, and the rule is linear and
// neutral in the stored sRGB encoding because no transfer-function
// linearization is applied. Adding 5000 before the division rounds half up,
// making the RGBA8 result independent of floating-point behavior. Alpha is
// carried through unchanged, matching Invert Colors.
fn filtered(base: vec4<u32>, kind: u32) -> vec4<u32> {
    if (kind == FILTER_GRAYSCALE) {
        let luminance = (2126u * base.x + 7152u * base.y + 722u * base.z + 5000u) / 10000u;
        return vec4<u32>(luminance, luminance, luminance, base.w);
    }
    if (kind == FILTER_HSV_ADJUSTMENT) {
        return hsvAdjustmentPixel(base, parameters.adjustment);
    }
    return vec4<u32>(255u - base.x, 255u - base.y, 255u - base.z, base.w);
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let local = invocation.xy;
    if (any(local >= vec2<u32>(256u))) { return; }
    let index = local.y * 256u + local.x;
    let flags = u32(parameters.tile.w);
    let selected = selectedness(parameters.tile.xy + vec2<i32>(local), index, flags);
    if (selected == 0u) {
        outputPixels[index] = basePixels[index];
        return;
    }
    let base = unpackPixel(basePixels[index]);
    let result = filtered(base, u32(parameters.tile.z));
    if (selected == 255u) {
        outputPixels[index] = packPixel(result);
        return;
    }
    // Fractional coverage interpolates between the source and the filtered
    // pixel; alpha is carried through unchanged so transparent pixels stay
    // transparent.
    let keep = 255u - selected;
    outputPixels[index] = packPixel(vec4<u32>(
        (base.x * keep + result.x * selected + 127u) / 255u,
        (base.y * keep + result.y * selected + 127u) / 255u,
        (base.z * keep + result.z * selected + 127u) / 255u,
        base.w));
}

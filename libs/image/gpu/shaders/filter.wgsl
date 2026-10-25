// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct FilterParameters {
    // Tile origin x/y, filter kind, coverage flags.
    tile: vec4<i32>,
    // Selection rectangle x/y/width/height.
    selection: vec4<i32>,
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
    return vec4<u32>(255u - base.x, 255u - base.y, 255u - base.z, base.w);
}

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

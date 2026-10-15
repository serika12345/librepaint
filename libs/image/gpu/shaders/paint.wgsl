// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

struct Parameters {
    tile: vec4<i32>,
    brush: vec4<u32>,
    counts: vec4<u32>,
}
struct Segment { endpoints: vec4<i32>, values: vec4<u32>, }
@group(0) @binding(0) var<uniform> parameters: Parameters;
@group(0) @binding(1) var<storage, read> segments: array<Segment>;
@group(0) @binding(2) var<storage, read> basePixels: array<u32>;
@group(0) @binding(3) var<storage, read_write> outputPixels: array<u32>;

// Low word first; 16-bit products preserve the complete 64-bit result.
fn product(a: u32, b: u32) -> vec2<u32> {
    let low = (a & 65535u) * (b & 65535u);
    let crossA = (a >> 16u) * (b & 65535u);
    let crossB = (a & 65535u) * (b >> 16u);
    let middle = (low >> 16u) + (crossA & 65535u) + (crossB & 65535u);
    return vec2<u32>((low & 65535u) | (middle << 16u),
                     (a >> 16u) * (b >> 16u) + (crossA >> 16u) + (crossB >> 16u) + (middle >> 16u));
}
fn add64(a: vec2<u32>, b: vec2<u32>) -> vec2<u32> {
    let low = a.x + b.x;
    return vec2<u32>(low, a.y + b.y + select(0u, 1u, low < a.x));
}
fn negate64(a: vec2<u32>) -> vec2<u32> { return add64(~a, vec2<u32>(1u, 0u)); }
fn signedProduct(a: i32, b: i32) -> vec2<u32> {
    let value = product(u32(abs(a)), u32(abs(b)));
    if ((a < 0) != (b < 0)) { return negate64(value); }
    return value;
}
fn lessEqual64(a: vec2<u32>, b: vec2<u32>) -> bool {
    return a.y < b.y || (a.y == b.y && a.x <= b.x);
}
fn divide64(n: vec2<u32>, d: u32) -> u32 {
    if (n.y == 0u) { return n.x / d; }
    var remainder = n.y;
    var result = 0u;
    for (var bit = 31; bit >= 0; bit -= 1) {
        remainder = (remainder << 1u) | ((n.x >> u32(bit)) & 1u);
        if (remainder >= d) {
            remainder -= d;
            result |= 1u << u32(bit);
        }
    }
    return result;
}
fn interpolate(start: i32, delta: i32, along: u32, length: u32) -> i32 {
    let offset = i32(divide64(product(u32(abs(delta)), along), length));
    if (delta < 0) { return start - offset; }
    return start + offset;
}
fn inside(point: vec2<i32>, center: vec2<i32>, radius: u32) -> bool {
    let delta = vec2<u32>(abs(point - center));
    if (any(delta > vec2<u32>(radius))) { return false; }
    return lessEqual64(add64(product(delta.x, delta.x), product(delta.y, delta.y)), product(radius, radius));
}

@compute @workgroup_size(8, 8)
fn main(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let local = invocation.xy;
    if (any(local >= vec2<u32>(256u))) { return; }
    let index = local.y * 256u + local.x;
    if (parameters.counts.z != 0u) { outputPixels[index] = parameters.brush.x; return; }
    let pixel = parameters.tile.xy + vec2<i32>(local);
    if (any(pixel >= parameters.tile.zw)) { outputPixels[index] = basePixels[index]; return; }
    let center = pixel * 16 + vec2<i32>(8);
    var mask = 0u;
    for (var s = 0u; s < parameters.counts.x && mask != 65535u; s += 1u) {
        let segment = segments[s];
        let maximumReach = i32(parameters.counts.y / 2u + 8u);
        if (any(center < min(segment.endpoints.xy, segment.endpoints.zw) - vec2<i32>(maximumReach)) ||
            any(center > max(segment.endpoints.xy, segment.endpoints.zw) + vec2<i32>(maximumReach))) { continue; }
        let delta = segment.endpoints.zw - segment.endpoints.xy;
        let length = segment.values.w;
        let step = parameters.brush.w;
        let first = (step - segment.values.z) % step;
        var low = 0u;
        var high = 0u;
        if (length != 0u) {
            let relative = center - segment.endpoints.xy;
            let dotValue = add64(signedProduct(relative.x, delta.x), signedProduct(relative.y, delta.y));
            var projection = 0u;
            if (i32(dotValue.y) >= 0) {
                if (lessEqual64(product(length, length), dotValue)) { projection = length; }
                else { projection = divide64(dotValue, length); }
            }
            let reach = parameters.counts.y / 2u + 32u;
            if (projection > reach + first) { low = (projection - reach - first) / step; }
            high = min(length, projection + reach);
            if (high < first) { continue; }
            high = (high - first) / step;
        }
        for (var dab = low; dab <= high; dab += 1u) {
            var along = first + dab * step;
            if (length == 0u) { along = 0u; }
            if (along > length) { break; }
            var position = segment.endpoints.xy;
            var pressure = segment.values.x;
            if (length != 0u) {
                position.x = interpolate(position.x, delta.x, along, length);
                position.y = interpolate(position.y, delta.y, along, length);
                pressure = u32(interpolate(i32(pressure), i32(segment.values.y) - i32(pressure), along, length));
            }
            let radius = divide64(product(parameters.counts.y, pressure), 131070u);
            if (radius == 0u) { continue; }
            for (var y = 0u; y < 4u; y += 1u) {
                for (var x = 0u; x < 4u; x += 1u) {
                    let bit = 1u << (y * 4u + x);
                    if ((mask & bit) == 0u && inside(pixel * 16 + vec2<i32>(i32(x * 4u + 2u), i32(y * 4u + 2u)), position, radius)) { mask |= bit; }
                }
            }
        }
    }
    var destination = unpackPixel(basePixels[index]);
    var source = unpackPixel(parameters.brush.x);
    let alpha = (mul255(source.w, parameters.brush.y) * countOneBits(mask) + 8u) / 16u;
    if (alpha == 0u) { outputPixels[index] = basePixels[index]; return; }
    if (parameters.brush.z != 0u) {
        destination.w = mul255(destination.w, 255u - alpha);
    } else if (parameters.counts.w != 0u) {
        destination = vec4<u32>((source.xyz * alpha + destination.xyz * (255u - alpha) + vec3<u32>(127u)) / 255u, destination.w);
    } else {
        source.w = alpha;
        destination = over(destination, source);
    }
    outputPixels[index] = packPixel(destination);
}

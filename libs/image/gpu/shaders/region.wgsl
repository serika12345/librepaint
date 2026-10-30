// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct RegionParameters {
    size: vec2<u32>, seed: vec2<u32>,
    format: u32, threshold: u32, spread: u32, flags: u32,
    implicit: vec4<u32>, selection: vec4<i32>
}
@group(0) @binding(0) var<uniform> parameters: RegionParameters;
@group(0) @binding(1) var<storage, read> atlas: array<u32>;
@group(0) @binding(2) var<storage, read> directory: array<vec2<i32>>;
@group(0) @binding(3) var<storage, read> selection: array<u32>;
@group(0) @binding(4) var<storage, read_write> labels: array<atomic<u32>>;
@group(0) @binding(5) var<storage, read_write> coverage: array<u32>;
@group(0) @binding(6) var<storage, read_write> status: array<atomic<u32>>;
@group(0) @binding(7) var<storage, read> conversion: array<u32>;
// The same workspace holds conversion intermediates, then Lab16 and parent
// labels in disjoint words. Dispatch boundaries freeze each completed stage.
fn labelStride() -> u32 {
    if (NATIVE_LAB_KIND == 2u) { return 6u; }
    if (NATIVE_LAB_KIND == 1u) { return 3u; }
    return 1u;
}
fn parentIndex(index: u32) -> u32 {
    return index * labelStride() + select(0u, 2u, NATIVE_LAB_KIND != 0u);
}
fn convertedLab(index: u32) -> vec3<u32> {
    let first = atomicLoad(&labels[index * labelStride()]);
    let second = atomicLoad(&labels[index * labelStride() + 1u]);
    return vec3<u32>(first & 65535u, first >> 16u, second & 65535u);
}
fn storeLab(index: u32, value: vec3<u32>) {
    atomicStore(&labels[index * labelStride()], value.x | (value.y << 16u));
    atomicStore(&labels[index * labelStride() + 1u], value.z);
}
fn entry(point: vec2<u32>) -> vec2<i32> {
    return directory[(point.y / 256u) * ((parameters.size.x + 255u) / 256u) + point.x / 256u];
}
fn pixel(point: vec2<u32>) -> vec4<u32> {
    let offset = entry(point).x;
    if (offset < 0) { return parameters.implicit; }
    let stride = select(1u, 2u, parameters.format != 0u);
    let word = u32(offset) + ((point.y % 256u) * 256u + point.x % 256u) * stride;
    return nativePixel(atlas[word], atlas[word + stride - 1u], parameters.format != 0u);
}
fn alphaByte(value: u32) -> u32 {
    if (parameters.format == 0u) { return value; }
    return (value - (value >> 8u) + 128u) >> 8u;
}
fn selected(point: vec2<u32>) -> u32 {
    if ((parameters.flags & 1u) == 0u) { return 255u; }
    var value = 0u;
    let p = vec2<i32>(point);
    let rect = parameters.selection;
    if ((parameters.flags & 4u) != 0u) {
        let offset = entry(point).y;
        if (offset >= 0) {
            let index = (point.y % 256u) * 256u + point.x % 256u;
            if ((parameters.flags & 8u) != 0u) {
                value = (selection[u32(offset) + (index >> 2u)] >> ((index & 3u) * 8u)) & 255u;
            } else {
                value = select(0u, 255u, ((selection[u32(offset) + (index >> 5u)] >> (index & 31u)) & 1u) != 0u);
            }
        }
    } else if (all(p >= rect.xy) && all(p < rect.xy + rect.zw)) {
        value = 255u;
        if ((parameters.flags & 2u) != 0u) {
            let radius = vec2<f32>(rect.zw) * 0.5;
            let relative = (vec2<f32>(point) + vec2<f32>(0.5) - vec2<f32>(rect.xy) - radius) / radius;
            value = select(0u, 255u, dot(relative, relative) <= 1.0);
        }
    }
    return select(value, 255u - value, (parameters.flags & 16u) != 0u);
}
fn opacity(difference: u32) -> u32 {
    if (parameters.spread == 100u) { return select(0u, 255u, difference <= parameters.threshold); }
    if (parameters.threshold == 0u || difference >= parameters.threshold) { return 0u; }
    return min(255u, (parameters.threshold - difference) * 25500u / (parameters.threshold * (100u - parameters.spread)));
}
@compute @workgroup_size(8, 8) fn convert(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let index = p.y * parameters.size.x + p.x;
    let input = pixel(p).rgb * select(257u, 1u, parameters.format != 0u);
    if (NATIVE_LAB_KIND == 1u) {
        storeLab(index, nativeLabClut(input));
    } else if (NATIVE_LAB_KIND == 2u) {
        let value = nativeLabLinearComponent(input, invocation.z);
        let offset = index * 6u + invocation.z * 2u;
        atomicStore(&labels[offset], value.x);
        atomicStore(&labels[offset + 1u], value.y);
    }
}
@compute @workgroup_size(8, 8) fn encode(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let index = p.y * parameters.size.x + p.x;
    let offset = index * 6u;
    let fx = Word64(atomicLoad(&labels[offset]), atomicLoad(&labels[offset + 1u]));
    let fy = Word64(atomicLoad(&labels[offset + 2u]), atomicLoad(&labels[offset + 3u]));
    let fz = Word64(atomicLoad(&labels[offset + 4u]), atomicLoad(&labels[offset + 5u]));
    storeLab(index, nativeLabEncode(fx, fy, fz));
}
@compute @workgroup_size(8, 8) fn classify(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let index = p.y * parameters.size.x + p.x;
    if ((parameters.flags & 64u) == 0u && selected(parameters.seed) == 0u) {
        coverage[index] = 0u;
        atomicStore(&labels[parentIndex(index)], 0xffffffffu);
        return;
    }
    let reference = pixel(parameters.seed);
    let seedAlpha = alphaByte(reference.a);
    var difference = 255u;
    let value = pixel(p);
    let alpha = alphaByte(value.a);
    if (parameters.threshold == 1u) {
        difference = select(255u, 0u, all(reference == value) || (seedAlpha == 0u && alpha == 0u));
    } else if (seedAlpha == 0u || alpha == 0u) {
        difference = (100u * u32(abs(i32(seedAlpha) - i32(alpha))) + 127u) / 255u;
    } else if (NATIVE_LAB_KIND != 0u && conversion[0] != 0u) {
        difference = nativeLabDifference(convertedLab(parameters.seed.y * parameters.size.x + parameters.seed.x),
            convertedLab(index), reference.a, value.a, parameters.format != 0u);
    } else {
        atomicStore(&status[0], 1u);
    }
    var weight = opacity(difference);
    if ((parameters.flags & 32u) != 0u && selected(p) == 0u) { weight = 0u; }
    coverage[index] = weight;
    atomicStore(&labels[parentIndex(index)], select(0xffffffffu, index, weight != 0u));
}
fn root(index: u32) -> u32 {
    var current = index;
    loop {
        let parent = atomicLoad(&labels[parentIndex(current)]);
        if (parent == current || parent == 0xffffffffu) { return parent; }
        let grandparent = atomicLoad(&labels[parentIndex(parent)]);
        // Parents only decrease, so compression cannot introduce a cycle.
        atomicMin(&labels[parentIndex(current)], grandparent);
        current = parent;
    }
    return current;
}
fn join(a: u32, b: u32) {
    loop {
        let first = root(a); let second = root(b);
        if (first == second || first == 0xffffffffu || second == 0xffffffffu) { return; }
        let high = max(first, second); let low = min(first, second);
        let changed = atomicCompareExchangeWeak(&labels[parentIndex(high)], high, low);
        if (changed.exchanged) { return; }
    }
}
@compute @workgroup_size(8, 8) fn connect(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let index = p.y * parameters.size.x + p.x;
    if (coverage[index] == 0u) { return; }
    if (p.x + 1u < parameters.size.x && coverage[index + 1u] != 0u) { join(index, index + 1u); }
    if (p.y + 1u < parameters.size.y && coverage[index + parameters.size.x] != 0u) { join(index, index + parameters.size.x); }
}
@compute @workgroup_size(8, 8) fn resolve(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let index = p.y * parameters.size.x + p.x;
    var included = coverage[index] != 0u;
    if ((parameters.flags & 64u) == 0u) {
        let seed = parameters.seed.y * parameters.size.x + parameters.seed.x;
        let component = root(seed);
        included = component != 0xffffffffu && root(index) == component;
    }
    coverage[index] = select(0u, min(coverage[index], selected(p)), included);
    if (coverage[index] != 0u) {
        atomicMin(&status[1], p.x); atomicMin(&status[2], p.y);
        atomicMax(&status[3], p.x); atomicMax(&status[4], p.y);
    }
}

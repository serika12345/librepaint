// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct RegionParameters {
    size: vec2<u32>, seed: vec2<u32>,
    format: u32, threshold: u32, spread: u32, flags: u32,
    implicit: vec4<u32>, selection: vec4<i32>, boundary: vec4<u32>,
    documentSize: vec2<u32>, origin: vec2<i32>, level: vec4<u32>
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
fn morphRadius() -> u32 { return parameters.flags >> 16u; }
fn morphLevels() -> u32 { return 31u - countLeadingZeros(2u * morphRadius() + 1u); }
fn labelStride() -> u32 {
    var native = 1u;
    if (NATIVE_LAB_KIND == 2u) { native = 6u; }
    if (NATIVE_LAB_KIND == 1u) { native = 3u; }
    return max(native, morphLevels() + 1u);
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
fn tileIndex(value: i32) -> i32 { return value >> 8u; }
fn entry(point: vec2<u32>) -> vec2<i32> {
    let world = vec2<i32>(point) + parameters.origin;
    let first = vec2<i32>(tileIndex(parameters.origin.x), tileIndex(parameters.origin.y));
    let columns = (parameters.origin.x + i32(parameters.size.x) + 255) / 256 - first.x;
    let tile = vec2<i32>(tileIndex(world.x), tileIndex(world.y)) - first;
    return directory[u32(tile.y * columns + tile.x)];
}
fn pixel(point: vec2<u32>) -> vec4<u32> {
    let offset = entry(point).x;
    if (offset < 0) { return parameters.implicit; }
    let world = vec2<u32>(vec2<i32>(point) + parameters.origin);
    let stride = select(1u, 2u, parameters.format != 0u);
    let word = u32(offset) + ((world.y % 256u) * 256u + world.x % 256u) * stride;
    return nativePixel(atlas[word], atlas[word + stride - 1u], parameters.format != 0u);
}
fn alphaByte(value: u32) -> u32 {
    if (parameters.format == 0u) { return value; }
    return (value - (value >> 8u) + 128u) >> 8u;
}
fn selected(point: vec2<u32>) -> u32 {
    if ((parameters.flags & 1u) == 0u) { return 255u; }
    var value = 0u;
    let p = vec2<i32>(point) + parameters.origin;
    let rect = parameters.selection;
    if ((parameters.flags & 4u) != 0u) {
        let offset = entry(point).y;
        if (offset >= 0) {
            let local = p - vec2<i32>(tileIndex(p.x), tileIndex(p.y)) * 256;
            let index = u32(local.y * 256 + local.x);
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
            let relative = (vec2<f32>(p) + vec2<f32>(0.5) - vec2<f32>(rect.xy) - radius) / radius;
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
    let world = vec2<i32>(p) + parameters.origin;
    if (any(world < vec2<i32>()) || any(world >= vec2<i32>(parameters.documentSize))) { return; }
    var index = p.y * parameters.size.x + p.x;
    let channels = select(1u, 3u, NATIVE_LAB_KIND == 2u);
    var rgba = pixel(p);
    if (invocation.z >= channels) {
        if (any(p != parameters.seed)) { return; }
        index = parameters.size.x * parameters.size.y;
        rgba = parameters.boundary;
    }
    let input = rgba.rgb * select(257u, 1u, parameters.format != 0u);
    if (NATIVE_LAB_KIND == 1u) {
        storeLab(index, nativeLabClut(input));
    } else if (NATIVE_LAB_KIND == 2u) {
        let value = nativeLabLinearComponent(input, invocation.z % channels);
        let offset = index * labelStride() + (invocation.z % channels) * 2u;
        atomicStore(&labels[offset], value.x);
        atomicStore(&labels[offset + 1u], value.y);
    }
}
@compute @workgroup_size(8, 8) fn encode(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let world = vec2<i32>(p) + parameters.origin;
    if (any(world < vec2<i32>()) || any(world >= vec2<i32>(parameters.documentSize))) { return; }
    var index = p.y * parameters.size.x + p.x;
    if (invocation.z != 0u) {
        if (any(p != parameters.seed)) { return; }
        index = parameters.size.x * parameters.size.y;
    }
    let offset = index * labelStride();
    let fx = Word64(atomicLoad(&labels[offset]), atomicLoad(&labels[offset + 1u]));
    let fy = Word64(atomicLoad(&labels[offset + 2u]), atomicLoad(&labels[offset + 3u]));
    let fz = Word64(atomicLoad(&labels[offset + 4u]), atomicLoad(&labels[offset + 5u]));
    storeLab(index, nativeLabEncode(fx, fy, fz));
}
@compute @workgroup_size(8, 8) fn classify(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let index = p.y * parameters.size.x + p.x;
    let world = vec2<i32>(p) + parameters.origin;
    if (any(world < vec2<i32>()) || any(world >= vec2<i32>(parameters.documentSize))) {
        coverage[index] = 0u;
        atomicStore(&labels[parentIndex(index)], 0xffffffffu);
        return;
    }
    if ((parameters.flags & 64u) == 0u && selected(parameters.seed) == 0u) {
        coverage[index] = 0u;
        atomicStore(&labels[parentIndex(index)], 0xffffffffu);
        return;
    }
    let boundary = (parameters.flags & 128u) != 0u;
    let reference = select(pixel(parameters.seed), parameters.boundary, boundary);
    let seedAlpha = alphaByte(reference.a);
    var difference = 255u;
    let value = pixel(p);
    let alpha = alphaByte(value.a);
    if (parameters.threshold == 1u) {
        difference = select(255u, 0u, all(reference == value) || (seedAlpha == 0u && alpha == 0u));
    } else if (seedAlpha == 0u || alpha == 0u) {
        difference = (100u * u32(abs(i32(seedAlpha) - i32(alpha))) + 127u) / 255u;
    } else if (NATIVE_LAB_KIND != 0u && conversion[0] != 0u) {
        difference = nativeLabDifference(convertedLab(select(parameters.seed.y * parameters.size.x + parameters.seed.x,
            parameters.size.x * parameters.size.y, boundary)),
            convertedLab(index), reference.a, value.a, parameters.format != 0u);
    } else {
        atomicStore(&status[0], 1u);
    }
    let similar = opacity(difference);
    var weight = select(similar, 255u - similar, boundary);
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
    if ((parameters.flags & 256u) != 0u) {
        coverage[index] = select(0u, coverage[index], included);
        if (coverage[index] != 0u) {
            let bounds = scratchIndex();
            atomicMin(&labels[bounds], p.x); atomicMin(&labels[bounds + 1u], p.y);
            atomicMax(&labels[bounds + 2u], p.x); atomicMax(&labels[bounds + 3u], p.y);
        }
    } else {
        coverage[index] = select(0u, min(coverage[index], selected(p)), included);
        if (coverage[index] != 0u) {
            atomicMin(&status[1], p.x); atomicMin(&status[2], p.y);
            atomicMax(&status[3], p.x); atomicMax(&status[4], p.y);
        }
    }
}

fn scratchIndex() -> u32 {
    return (parameters.size.x * parameters.size.y + select(0u, 1u, (parameters.flags & 128u) != 0u)) * labelStride();
}
fn roundedBounds(index: u32) -> vec4<i32> {
    let first = vec2<u32>(atomicLoad(&labels[index]), atomicLoad(&labels[index + 1u]));
    let last = vec2<u32>(atomicLoad(&labels[index + 2u]), atomicLoad(&labels[index + 3u]));
    if (any(first > last)) { return vec4<i32>(); }
    let origin = first / vec2<u32>(64u) * vec2<u32>(64u);
    let end = (last / vec2<u32>(64u) + vec2<u32>(1u)) * vec2<u32>(64u);
    return vec4<i32>(vec2<i32>(origin), vec2<i32>(end));
}
fn rawRectangle() -> vec4<i32> {
    if ((parameters.flags & 64u) != 0u) {
        let end = (parameters.documentSize + vec2<u32>(63u)) / vec2<u32>(64u) * vec2<u32>(64u);
        return vec4<i32>(-parameters.origin, vec2<i32>(end) - parameters.origin);
    }
    return roundedBounds(scratchIndex());
}
fn filterRectangle() -> vec4<i32> {
    if (morphRadius() != 0u) { return roundedBounds(scratchIndex() + 4u); }
    return rawRectangle();
}
fn rawCoverage(point: vec2<i32>, rect: vec4<i32>) -> i32 {
    if (any(point < rect.xy) || any(point >= rect.zw) || any(point < vec2<i32>()) || any(vec2<u32>(point) >= parameters.size)) { return 0; }
    return i32(coverage[u32(point.y) * parameters.size.x + u32(point.x)]);
}
fn extremum(a: u32, b: u32) -> u32 {
    return select(min(a, b), max(a, b), (parameters.flags & 512u) != 0u);
}
fn verticalValue(point: vec2<i32>, level: u32) -> u32 {
    if (any(point < vec2<i32>()) || any(point >= vec2<i32>(parameters.size))) { return 0u; }
    let index = u32(point.y) * parameters.size.x + u32(point.x);
    if (level == 0u) { return coverage[index]; }
    return atomicLoad(&labels[index * labelStride() + level - 1u]);
}
// Forward power-of-two column intervals permit constant-time vertical queries
// for every circular-mask column, without radius-squared pixel work.
@compute @workgroup_size(8, 8) fn vertical(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let level = parameters.level.x;
    let value = extremum(verticalValue(vec2<i32>(p), level - 1u),
        verticalValue(vec2<i32>(p) + vec2<i32>(0, i32(1u << (level - 1u))), level - 1u));
    atomicStore(&labels[(p.y * parameters.size.x + p.x) * labelStride() + level - 1u], value);
}
fn verticalSpan(point: vec2<i32>, radius: i32) -> u32 {
    if (point.x < 0 || point.x >= i32(parameters.size.x)) { return 0u; }
    let first = max(0, point.y - radius);
    let end = min(i32(parameters.size.y), point.y + radius + 1);
    if (end <= first) { return 0u; }
    if ((parameters.flags & 1024u) != 0u &&
        (first != point.y - radius || end != point.y + radius + 1)) { return 0u; }
    let level = 31u - countLeadingZeros(u32(end - first));
    return extremum(verticalValue(vec2<i32>(point.x, first), level),
        verticalValue(vec2<i32>(point.x, end - i32(1u << level)), level));
}
@compute @workgroup_size(8, 8) fn morph(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let point = vec2<i32>(p);
    let radius = i32(morphRadius());
    var rect = rawRectangle();
    let empty = any(rect.xy >= rect.zw);
    if ((parameters.flags & 512u) != 0u) { rect += vec4<i32>(-radius, -radius, radius, radius); }
    var value = select(255u, 0u, (parameters.flags & 512u) != 0u);
    if (empty || any(point < rect.xy) || any(point >= rect.zw)) { value = 0u; }
    else {
        for (var x = -radius; x <= radius; x += 1) {
            let extent = i32(conversion[conversion[5] + u32(x + radius)]);
            value = extremum(value, verticalSpan(point + vec2<i32>(x, 0), extent));
            if (((parameters.flags & 512u) != 0u && value == 255u) ||
                ((parameters.flags & 1024u) != 0u && value == 0u)) { break; }
        }
    }
    atomicStore(&labels[(p.y * parameters.size.x + p.x) * labelStride() + morphLevels()], value);
}
@compute @workgroup_size(8, 8) fn morphed(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let index = p.y * parameters.size.x + p.x;
    let value = atomicLoad(&labels[index * labelStride() + morphLevels()]);
    if ((parameters.flags & 2048u) != 0u) {
        coverage[index] = value;
        if (value != 0u) {
            let bounds = scratchIndex() + 4u;
            atomicMin(&labels[bounds], p.x); atomicMin(&labels[bounds + 1u], p.y);
            atomicMax(&labels[bounds + 2u], p.x); atomicMax(&labels[bounds + 3u], p.y);
        }
    } else {
        coverage[index] = min(value, selected(p));
        if (coverage[index] != 0u) {
            atomicMin(&status[1], p.x); atomicMin(&status[2], p.y);
            atomicMax(&status[3], p.x); atomicMax(&status[4], p.y);
        }
    }
}
struct SpanExtreme { distance: i32, difference: i32, valid: bool }
fn spanExtreme(point: vec2<i32>, rect: vec4<i32>, along: vec2<i32>, across: vec2<i32>, direction: i32,
               average: i32, gradient: i32, currentDifference: i32) -> SpanExtreme {
    var result = SpanExtreme(0, 0, true);
    for (var step = 0; step < 30; step += 1) {
        result.distance += select(1, 2, step >= 20);
        let first = point + direction * result.distance * along;
        let second = first + across;
        let a = rawCoverage(first, rect);
        let b = rawCoverage(second, rect);
        result.difference = ((a + b) >> 1u) - average;
        if (abs(result.difference) > gradient) {
            var nextDifference: i32;
            if ((currentDifference < 0 && result.difference < 0) || (currentDifference > 0 && result.difference > 0)) {
                nextDifference = ((b + rawCoverage(second + across, rect)) >> 1u) - average;
            } else {
                nextDifference = ((a + rawCoverage(first - across, rect)) >> 1u) - average;
            }
            result.valid = abs(nextDifference) <= gradient;
            break;
        }
    }
    return result;
}
fn antialiasedCoverage(point: vec2<i32>, rect: vec4<i32>) -> u32 {
    if (any(point < rect.xy) || any(point >= rect.zw)) { return 0u; }
    let nw = rawCoverage(point + vec2<i32>(-1, -1), rect);
    let n = rawCoverage(point + vec2<i32>(0, -1), rect);
    let ne = rawCoverage(point + vec2<i32>(1, -1), rect);
    let w = rawCoverage(point + vec2<i32>(-1, 0), rect);
    let m = rawCoverage(point, rect);
    let e = rawCoverage(point + vec2<i32>(1, 0), rect);
    let sw = rawCoverage(point + vec2<i32>(-1, 1), rect);
    let s = rawCoverage(point + vec2<i32>(0, 1), rect);
    let se = rawCoverage(point + vec2<i32>(1, 1), rect);
    let rowN = (nw >> 2u) + (n >> 1u) + (ne >> 2u);
    let rowM = (w >> 2u) + (m >> 1u) + (e >> 2u);
    let rowS = (sw >> 2u) + (s >> 1u) + (se >> 2u);
    let colW = (nw >> 2u) + (w >> 1u) + (sw >> 2u);
    let colM = (n >> 2u) + (m >> 1u) + (s >> 2u);
    let colE = (ne >> 2u) + (e >> 1u) + (se >> 2u);
    let gn = abs(rowM - rowN); let gs = abs(rowS - rowM);
    let gw = abs(colM - colW); let ge = abs(colE - colM);
    let vertical = max(gn, gs); let horizontal = max(gw, ge);
    if (max(vertical, horizontal) < 4) { return u32(m); }
    var neighbor: i32;
    var gradient: i32;
    var along: vec2<i32>;
    var across: vec2<i32>;
    if (vertical > horizontal) {
        neighbor = select(s, n, gn > gs);
        gradient = max(gn, gs);
        along = vec2<i32>(1, 0); across = vec2<i32>(0, select(1, -1, gn > gs));
    } else {
        neighbor = select(e, w, gw > ge);
        gradient = max(gw, ge);
        along = vec2<i32>(0, 1); across = vec2<i32>(select(1, -1, gw > ge), 0);
    }
    let average = (neighbor + m) >> 1u;
    let difference = m - average;
    let negative = spanExtreme(point, rect, along, across, -1, average, gradient >> 2u, difference);
    let positive = spanExtreme(point, rect, along, across, 1, average, gradient >> 2u, difference);
    var nearest = positive;
    if (negative.distance < positive.distance) { nearest = negative; }
    if (!nearest.valid || ((difference < 0) == (nearest.difference < 0))) { return u32(m); }
    let interpolation = ((nearest.distance << 8u) / (negative.distance + positive.distance)) + 128;
    return u32(neighbor + (((m - neighbor) * interpolation) >> 8u)) & 255u;
}
@compute @workgroup_size(8, 8) fn antiAlias(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let index = p.y * parameters.size.x + p.x;
    atomicStore(&labels[parentIndex(index)], antialiasedCoverage(vec2<i32>(p), filterRectangle()));
}
@compute @workgroup_size(8, 8) fn filtered(@builtin(global_invocation_id) invocation: vec3<u32>) {
    let p = invocation.xy;
    if (any(p >= parameters.size)) { return; }
    let index = p.y * parameters.size.x + p.x;
    let value = min(atomicLoad(&labels[parentIndex(index)]), selected(p));
    coverage[index] = value;
    if (value != 0u) {
        atomicMin(&status[1], p.x); atomicMin(&status[2], p.y);
        atomicMax(&status[3], p.x); atomicMax(&status[4], p.y);
    }
}

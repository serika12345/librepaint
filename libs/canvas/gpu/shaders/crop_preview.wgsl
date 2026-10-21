// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct Parameters {
    bounds: vec4<f32>, size: vec2<f32>, count: u32, mode: u32, color: vec4<f32>, clipA: vec4<f32>, clipB: vec4<f32>
}
@group(0) @binding(0) var<uniform> p: Parameters;
// Each edge stores (top x, bottom x, top y, signed height).
@group(0) @binding(1) var<storage, read> rows: array<vec4<f32>>;
struct Vertex { @builtin(position) position: vec4<f32>, @location(0) @interpolate(flat) line: u32 }
@vertex fn vertexMain(@builtin(vertex_index) index: u32, @builtin(instance_index) line: u32) -> Vertex {
    let uv = vec2<f32>(select(0., 1., index == 1u || index >= 4u),
                       select(0., 1., index == 2u || index == 3u || index == 5u));
    var bounds = p.bounds;
    if ((p.mode & 1u) != 0u) { bounds = rows[3u * line + 1u]; }
    let position = mix(bounds.xy, bounds.zw, uv);
    return Vertex(vec4(position.x * 2. / p.size.x - 1., 1. - position.y * 2. / p.size.y, 0., 1.), line);
}
fn edgeCoverage(edge: vec4<f32>, center: vec2<f32>) -> f32 {
    let height = abs(edge.w);
    let top = max(edge.z, center.y - .5);
    let bottom = min(edge.z + height, center.y + .5);
    if (bottom <= top) { return 0.; }
    let first = center.x + .5 - mix(edge.x, edge.y, (top - edge.z) / height);
    let last = center.x + .5 - mix(edge.x, edge.y, (bottom - edge.z) / height);
    let low = min(first, last);
    let high = max(first, last);
    let direction = select(-1., 1., edge.w > 0.);
    if (high <= 0.) { return 0.; }
    if (low >= 1.) { return (bottom - top) * direction; }
    // Integrate the linear width clipped to [0, 1]. Its two boundary
    // crossings divide the row into empty, linear and fully covered parts.
    let span = high - low;
    var area = clamp(low, 0., 1.);
    if (span > 0.) {
        let clipped = clamp(vec2(low, high), vec2(0.), vec2(1.));
        area = ((clipped.x + clipped.y) * .5 * (clipped.y - clipped.x) + max(high - 1., 0.)) / span;
    }
    return area * (bottom - top) * direction;
}
fn directedEdge(first: vec2<f32>, last: vec2<f32>) -> vec4<f32> {
    let low = select(first, last, last.y < first.y);
    let high = select(last, first, last.y < first.y);
    return vec4(low.x, high.x, low.y, last.y - first.y);
}
fn quadCoverage(a: vec2<f32>, b: vec2<f32>, c: vec2<f32>, d: vec2<f32>, center: vec2<f32>) -> f32 {
    return abs(edgeCoverage(directedEdge(a, b), center) + edgeCoverage(directedEdge(b, c), center) +
               edgeCoverage(directedEdge(c, d), center) + edgeCoverage(directedEdge(d, a), center));
}
fn rowIntersection(x0: i32, x1: i32, y0: i32, y1: i32, column: i32) -> i32 {
    let increasing = x1 > x0;
    if (column <= min(x0, x1)) { return select(y1, y0, increasing); }
    if (column >= max(x0, x1)) { return select(y0, y1, increasing); }
    return y0 + ((column - x0) * (y1 - y0)) / (x1 - x0);
}
fn edgeCellArea(edge: vec4<f32>, center: vec2<f32>) -> i32 {
    let height = abs(edge.w);
    let pixel = vec2<i32>(center);
    let top = max(edge.z, f32(pixel.y));
    let bottom = min(edge.z + height, f32(pixel.y + 1));
    if (bottom <= top) { return 0; }
    // Outline vertices use a 1/64 grid. Match the scanline's 1/256
    // intersection grid before accumulating its signed integer cell area.
    let x0 = i32(floor(mix(edge.x, edge.y, (top - edge.z) / height) * 256.));
    let x1 = i32(floor(mix(edge.x, edge.y, (bottom - edge.z) / height) * 256.));
    let y0 = i32(floor((top - f32(pixel.y)) * 256.));
    let y1 = i32(floor((bottom - f32(pixel.y)) * 256.));
    let left = pixel.x * 256;
    let right = left + 256;
    let first = clamp(x0 - left, 0, 256);
    let last = clamp(x1 - left, 0, 256);
    var area = (512 - first - last) * (y1 - y0);
    if (x0 != x1) {
        let atLeft = rowIntersection(x0, x1, y0, y1, left);
        let atRight = rowIntersection(x0, x1, y0, y1, right);
        let full = select(y1 - atLeft, atLeft - y0, x1 > x0);
        let partial = select(atLeft - atRight, atRight - atLeft, x1 > x0);
        area = full * 512 + (512 - first - last) * partial;
    }
    return select(-area, area, edge.w > 0.);
}
fn lineCoverage(line: u32, center: vec2<f32>) -> u32 {
    let data = bitcast<vec4<i32>>(rows[3u * line]);
    let vertical = bitcast<vec4<i32>>(rows[3u * line + 2u]).x != 0;
    let pixel = vec2<i32>(center);
    let major = select(pixel.x, pixel.y, vertical);
    let minor = select(pixel.y, pixel.x, vertical);
    let first = data.x >> 6;
    let last = data.y >> 6;
    if (major < first || major > last) { return 0u; }
    var length = 64;
    if (first == last) { length = data.y - data.x; }
    else if (major == first) { length = 64 - (data.x & 63); }
    else if (major == last) { length = data.y & 63; }
    let interpolated = data.z + (major - first) * data.w;
    let base = interpolated >> 16;
    let fraction = (interpolated >> 8) & 255;
    var width = 0;
    if (minor == base) { width = 255 - fraction; }
    else if (minor == base + 1) { width = fraction; }
    return u32((width * length) >> 6);
}
fn quantize(area: f32) -> u32 { return min(255u, u32(clamp(area, 0., 1.) * 256.)); }
@fragment fn fragmentMain(vertex: Vertex) -> @location(0) vec4<f32> {
    let position = vertex.position.xy;
    var opacity = 0u;
    if ((p.mode & 1u) != 0u) {
        opacity = lineCoverage(vertex.line, position);
    } else {
        var area = 0.;
        var cellArea = 0;
        for (var i = 0u; i < p.count; i++) {
            if ((p.mode & 32u) != 0u) { cellArea += edgeCellArea(rows[i], position); }
            else { area += edgeCoverage(rows[i], position); }
        }
        // Round signed winding before taking its magnitude, including when
        // several handle contours overlap within this physical pixel.
        let winding = u32(abs(cellArea >> 9));
        if ((p.mode & 8u) != 0u) {
            let parity = winding & 511u;
            opacity = min(255u, select(parity, 512u - parity, parity > 256u));
        } else if ((p.mode & 16u) != 0u) {
            opacity = u32(clamp(abs(area), 0., 1.) * 255.);
        } else { opacity = min(255u, winding); }
    }
    if ((p.mode & 2u) != 0u && (any(position < p.clipA.xy) || any(position >= p.clipA.zw))) { discard; }
    if ((p.mode & 4u) != 0u) {
        let clip = quantize(quadCoverage(p.clipA.xy, p.clipA.zw, p.clipB.xy, p.clipB.zw, position));
        opacity = (opacity * clip + 127u) / 255u;
    }
    if (opacity == 0u) { discard; }
    let product = vec4<u32>(p.color) * opacity;
    return vec4<f32>((product + (product >> vec4(8u)) + vec4(128u)) >> vec4(8u)) / 255.;
}

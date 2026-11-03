// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct Parameters {
    matrix: mat4x4<f32>, clip: vec4<f32>, channels: vec4<u32>, background: vec4<u32>,
    checkerFirst: vec4<u32>, checkerSecond: vec4<u32>,
    checkerX: vec4<f32>, checkerY: vec4<f32>, checker: vec4<u32>
}
@group(0) @binding(0) var<uniform> p: Parameters;
@group(0) @binding(1) var<storage, read> pixels: array<u32>;
@group(0) @binding(2) var<storage, read> conversion: array<u32>;

fn parameter(offset: u32, index: u32) -> f32 {
    return bitcast<f32>(conversion[offset + 2u + index]);
}
// Round the represented f32 value to a U16 word using its exact binary
// significand. LCMS multiplies this f32 input in double before rounding.
fn tableInputWord(input: f32) -> u32 {
    let bits = bitcast<u32>(clamp(input, 0., 1.));
    let exponent = (bits >> 23u) & 255u;
    if (exponent < 110u) { return 0u; }
    if (exponent >= 127u) { return 65535u; }
    let significand = (bits & 8388607u) | 8388608u;
    let lowProduct = (significand & 65535u) * 65535u;
    let highProduct = (significand >> 16u) * 65535u + (lowProduct >> 16u);
    let shift = 134u - exponent;
    return (highProduct >> shift) + ((highProduct >> (shift - 1u)) & 1u);
}
fn wordByte(word: u32) -> u32 {
    return ((word * 65281u + 8388608u) >> 24u) & 255u;
}
fn curveValue(offset: u32, input: f32) -> f32 {
    let kind = bitcast<i32>(conversion[offset]);
    if (kind == 0) {
        let domain = conversion[offset + 1u] - 1u;
        let word = tableInputWord(input);
        if (word == 65535u) { return f32(conversion[offset + 2u + domain]) / 65535.; }
        // Split domain*word before division by 65535. GPU binding capacity
        // bounds the domain; no product needs an overflowing signed integer.
        let lowProduct = (domain & 65535u) * word;
        let highProduct = (domain >> 16u) * word + (lowProduct >> 16u);
        let sum = highProduct + (lowProduct & 65535u);
        let cell = highProduct + sum / 65535u;
        let remainder = sum % 65535u;
        let rest = remainder + (remainder + 32767u) / 65535u;
        let first = conversion[offset + 2u + cell];
        let second = conversion[offset + 3u + cell];
        // LCMS LinearInterp uses unsigned modular multiplication even for a
        // descending table; the returned word keeps the low 16 bits.
        let difference = (second - first) * rest + 32768u;
        return f32(((difference >> 16u) + first) & 65535u) / 65535.;
    }
    // Match LCMS DefaultEvalParametricFn's piecewise domains. Matrix outputs
    // may leave [0,1]; only table lookup and final display quantization saturate.
    let x = input;
    let g = parameter(offset, 0u);
    if (kind == 1 || kind == -1) {
        if (x < 0.) { return select(0., x, abs(g - 1.) < 0.0001); }
        if (kind == 1) { return pow(x, g); }
        return pow(x, 1. / g);
    }
    let a = parameter(offset, 1u);
    let b = parameter(offset, 2u);
    if (kind == 2) {
        if (x >= -b / a) { return pow(max(0., a*x+b), g); }
        return 0.;
    }
    if (kind == -2) {
        if (x < 0.) { return 0.; }
        return max(0., (pow(x, 1./g)-b)/a);
    }
    let c = parameter(offset, 3u);
    if (kind == 3) {
        if (x >= max(0., -b/a)) {
            let e = a*x+b;
            if (e > 0.) { return pow(e, g)+c; }
            return 0.;
        }
        return c;
    }
    if (kind == -3) {
        if (x >= c) {
            if (x > c) { return (pow(x-c, 1./g)-b)/a; }
            return 0.;
        }
        return -b/a;
    }
    let d = parameter(offset, 4u);
    if (kind == 4) {
        if (x >= d) { return pow(max(0., a*x+b), g); }
        return c*x;
    }
    if (kind == -4) {
        let boundary = pow(max(0., a*d+b), g);
        if (x >= boundary) { return (pow(max(0., x), 1./g)-b)/a; }
        if (abs(c) < 0.0001) { return 0.; }
        return x/c;
    }
    let e = parameter(offset, 5u);
    let f = parameter(offset, 6u);
    if (kind == 5) {
        if (x >= d) { return pow(max(0., a*x+b), g)+e; }
        return c*x+f;
    }
    if (x >= c*d+f) {
        if (x < e) { return 0.; }
        return (pow(x-e, 1./g)-b)/a;
    }
    if (abs(c) < 0.0001) { return 0.; }
    return (x-f)/c;
}
// Two 16-bit words per conversion word, low half first.
fn clutWord(table: u32, index: u32) -> u32 {
    let word = conversion[table + (index >> 1u)];
    return (word >> (16u * (index & 1u))) & 65535u;
}
// LCMS TetrahedralInterp16: fixed-domain nodes, signed modular accumulation and
// the same six >= branches that select the cumulative corner order.
fn clutRgb(stage: u32, input: vec3<u32>) -> vec3<u32> {
    let grid = vec3<u32>(conversion[stage + 2u], conversion[stage + 3u], conversion[stage + 4u]);
    let table = stage + 6u;
    let domain = grid - vec3(1u);
    let product = input * domain;
    let fixed = product + (product + vec3(32767u)) / vec3(65535u);
    let cell = fixed >> vec3(16u);
    let rest = fixed & vec3(65535u);
    let strides = vec3<u32>(grid.y * grid.z * 3u, grid.z * 3u, 3u);
    let base = cell.x * strides.x + cell.y * strides.y + cell.z * strides.z;
    // The step along an axis is zero at the top node, as in LCMS.
    let steps = vec3<u32>(
        select(strides.x, 0u, input.x == 65535u),
        select(strides.y, 0u, input.y == 65535u),
        select(strides.z, 0u, input.z == 65535u));
    var order: array<u32, 3>;
    order[0u] = 0u; order[1u] = 1u; order[2u] = 2u;
    if (rest.x >= rest.y) {
        if (rest.y >= rest.z) { }
        else if (rest.z >= rest.x) { order[0u] = 2u; order[1u] = 0u; order[2u] = 1u; }
        else { order[0u] = 0u; order[1u] = 2u; order[2u] = 1u; }
    } else {
        if (rest.x >= rest.z) { order[0u] = 1u; order[1u] = 0u; order[2u] = 2u; }
        else if (rest.y >= rest.z) { order[0u] = 1u; order[1u] = 2u; order[2u] = 0u; }
        else { order[0u] = 2u; order[1u] = 1u; order[2u] = 0u; }
    }
    var positions: array<u32, 4>;
    positions[0u] = base; positions[1u] = 0u; positions[2u] = 0u; positions[3u] = 0u;
    var remainders: array<u32, 3>;
    remainders[0u] = 0u; remainders[1u] = 0u; remainders[2u] = 0u;
    for (var step = 0u; step < 3u; step++) {
        let axis = order[step];
        remainders[step] = rest[axis];
        positions[step + 1u] = positions[step] + steps[axis];
    }
    var output = vec3<u32>();
    for (var channel = 0u; channel < 3u; channel++) {
        let c0 = i32(clutWord(table, positions[0u] + channel));
        let c1 = i32(clutWord(table, positions[1u] + channel)) - c0;
        let c2 = i32(clutWord(table, positions[2u] + channel)) - i32(clutWord(table, positions[1u] + channel));
        let c3 = i32(clutWord(table, positions[3u] + channel)) - i32(clutWord(table, positions[2u] + channel));
        var sum = c1 * i32(remainders[0u]) + c2 * i32(remainders[1u]) + c3 * i32(remainders[2u]) + 0x8001;
        sum = sum + (sum >> 16u);
        output[channel] = (u32(c0 + (sum >> 16u))) & 65535u;
    }
    return output;
}
fn displayRgb(input: vec3<f32>) -> vec3<f32> {
    var color = input;
    var offset = 1u;
    for (var i = 0u; i < conversion[0]; i++) {
        let kind = conversion[offset];
        if (kind == 0u) {
            color = vec3(curveValue(conversion[offset+2u], color.r),
                         curveValue(conversion[offset+3u], color.g),
                         curveValue(conversion[offset+4u], color.b));
        } else if (kind == 1u) {
            var next: vec3<f32>;
            for (var row = 0u; row < 3u; row++) {
                let start = offset + 2u + row*3u;
                next[row] = dot(color, vec3(bitcast<f32>(conversion[start]),
                    bitcast<f32>(conversion[start+1u]), bitcast<f32>(conversion[start+2u])))
                    + bitcast<f32>(conversion[offset+11u+row]);
            }
            color = next;
        } else {
            let words = vec3<u32>(tableInputWord(color.r), tableInputWord(color.g),
                                  tableInputWord(color.b));
            color = vec3<f32>(clutRgb(offset, words)) / 65535.;
        }
        offset = conversion[offset+1u];
    }
    return color;
}
struct Vertex { @builtin(position) position: vec4<f32>, @location(0) uv: vec2<f32> }
@vertex fn vertexMain(@builtin(vertex_index) index: u32) -> Vertex {
    // Two triangles: (0,0), (1,0), (0,1), (0,1), (1,0), (1,1).
    let uv = vec2<f32>(select(0., 1., index == 1u || index >= 4u),
                       select(0., 1., index == 2u || index == 3u || index == 5u));
    return Vertex(p.matrix * vec4(uv, 0., 1.), uv);
}
// Match the established Qt 8-bit channel multiplication before composition.
fn scaleDisplayBytes(pixel: vec4<u32>, opacity: u32) -> vec4<u32> {
    let product = pixel * opacity;
    return (product + (product >> vec4(8u)) + vec4(128u)) >> vec4(8u);
}
fn overBytes(source: vec4<u32>, destination: vec4<u32>) -> vec4<u32> {
    return source + scaleDisplayBytes(destination, 255u - source.a);
}
fn backgroundAt(position: vec2<f32>) -> vec4<u32> {
    if (p.checker.x == 0u) { return p.background; }
    let point = vec2(dot(p.checkerX.xy, position) + p.checkerX.z,
                     dot(p.checkerY.xy, position) + p.checkerY.z);
    let cell = vec2<i32>(floor(point / f32(p.checker.y)));
    let second = overBytes(p.checkerSecond, p.background);
    return select(second, overBytes(p.checkerFirst, second), ((cell.x ^ cell.y) & 1) == 0);
}
@fragment fn backgroundMain(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {
    return vec4<f32>(backgroundAt(position.xy)) / 255.;
}
@fragment fn fragmentMain(@location(0) uv: vec2<f32>, @builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {
    if (any(uv < p.clip.xy) || any(uv >= p.clip.zw)) { discard; }
    let pos = vec2<u32>(clamp(floor(uv * 256.), vec2(0.), vec2(255.)));
    let index = pos.y * 256u + pos.x;
    var words: vec4<u32>;
    if (p.channels.z != 0u) {
        let first = pixels[index * 2u];
        let second = pixels[index * 2u + 1u];
        words = vec4<u32>(first & 65535u, first >> 16u,
                          second & 65535u, second >> 16u);
    } else {
        let pixel = pixels[index];
        let bytes = vec4<u32>(pixel & 255u, (pixel >> 8u) & 255u,
                              (pixel >> 16u) & 255u, pixel >> 24u);
        // LCMS expands 8-bit components to their 16-bit representation.
        words = (bytes << vec4(8u)) | bytes;
    }
    let displayed = displayRgb(vec3<f32>(words.rgb) / 65535.);
    let straight = vec4<u32>(wordByte(tableInputWord(displayed.r)),
                             wordByte(tableInputWord(displayed.g)),
                             wordByte(tableInputWord(displayed.b)),
                             wordByte(tableInputWord(f32(words.a) / 65535.)));
    let mask = p.channels.x;
    let colors = mask & 7u;
    var visual: vec4<u32>;
    if (p.channels.y == 0u && (colors == 1u || colors == 2u || colors == 4u)) {
        let index = select(select(2u, 1u, colors == 2u), 0u, colors == 1u);
        visual = vec4(vec3(straight[index]), straight.a);
    } else {
        let enabled = vec4<bool>((mask & 1u) != 0u, (mask & 2u) != 0u,
                                  (mask & 4u) != 0u, (mask & 8u) != 0u);
        visual = select(vec4(0u), straight, enabled);
    }
    let source = scaleDisplayBytes(vec4<u32>(visual.rgb, 255u), visual.a);
    return vec4<f32>(overBytes(source, backgroundAt(position.xy))) / 255.;
}

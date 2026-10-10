// SPDX-License-Identifier: GPL-2.0-or-later
struct Parameters {
    rowX: vec4<f32>,
    rowY: vec4<f32>,
    background: vec4<f32>,
}
@group(0) @binding(0) var<uniform> parameters: Parameters;
@group(0) @binding(1) var image: texture_2d<f32>;

@vertex
fn vertex(@builtin(vertex_index) index: u32) -> @builtin(position) vec4<f32> {
    let corner = vec2<f32>(f32((index << 1u) & 2u), f32(index & 2u));
    return vec4(corner * 2.0 - vec2(1.0), 0.0, 1.0);
}

fn premultiply(color: vec4<f32>) -> vec4<f32> {
    return vec4(color.rgb * color.a, color.a);
}

fn loadPixel(position: vec2<i32>) -> vec4<f32> {
    if (any(position < vec2(0)) || any(position >= vec2<i32>(textureDimensions(image)))) {
        return vec4(0.0);
    }
    return premultiply(textureLoad(image, position, 0));
}

fn sampleImage(position: vec2<f32>) -> vec4<f32> {
    let dimensions = vec2<f32>(textureDimensions(image));
    if (parameters.rowX.w == 0.0) {
        if (any(position < vec2(0.0)) || any(position >= dimensions)) { return vec4(0.0); }
        return loadPixel(vec2<i32>(floor(position)));
    }
    if (any(position < vec2(-0.5)) || any(position > dimensions + vec2(0.5))) { return vec4(0.0); }
    let centered = position - vec2(0.5);
    let base = vec2<i32>(floor(centered));
    let fraction = fract(centered);
    let top = mix(loadPixel(base), loadPixel(base + vec2(1, 0)), fraction.x);
    let bottom = mix(loadPixel(base + vec2(0, 1)), loadPixel(base + vec2(1, 1)), fraction.x);
    return mix(top, bottom, fraction.y);
}

@fragment
fn fragment(@builtin(position) position: vec4<f32>) -> @location(0) vec4<f32> {
    let coordinate = vec2(dot(parameters.rowX.xyz, vec3(position.xy, 1.0)),
                          dot(parameters.rowY.xyz, vec3(position.xy, 1.0)));
    let source = sampleImage(coordinate);
    let result = source + premultiply(parameters.background) * (1.0 - source.a);
    if (result.a == 0.0) { return vec4(0.0); }
    let straight = vec4(result.rgb / result.a, result.a);
    return round(clamp(straight, vec4(0.0), vec4(1.0)) * 255.0) / 255.0;
}

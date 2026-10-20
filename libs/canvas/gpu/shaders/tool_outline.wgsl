// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
struct Parameters { viewport: vec2<f32>, padding: vec2<f32>, color: vec4<f32> }
@group(0) @binding(0) var<uniform> p: Parameters;
@vertex fn vertexMain(@location(0) point: vec2<f32>) -> @builtin(position) vec4<f32> {
    return vec4(2. * point.x / p.viewport.x - 1., 1. - 2. * point.y / p.viewport.y, 0., 1.);
}
@fragment fn fragmentMain() -> @location(0) vec4<f32> { return p.color; }

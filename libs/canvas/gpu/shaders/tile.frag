// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#version 430
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
layout(std140, binding = 0) uniform Parameters { mat4 matrix; vec4 clip; } p;
layout(std430, binding = 1) readonly buffer Pixels { uint rgba[]; } pixels;
void main()
{
    if (any(lessThan(uv, p.clip.xy)) || any(greaterThanEqual(uv, p.clip.zw))) discard;
    ivec2 pos = clamp(ivec2(floor(uv * 256.0)), ivec2(0), ivec2(255));
    uint pixel = pixels.rgba[pos.y * 256 + pos.x];
    vec4 straight = vec4(pixel & 255u, (pixel >> 8) & 255u, (pixel >> 16) & 255u, pixel >> 24) / 255.0;
    color = vec4(straight.rgb * straight.a, straight.a);
}

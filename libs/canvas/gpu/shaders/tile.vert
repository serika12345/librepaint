// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
#version 430
layout(location = 0) out vec2 uv;
layout(std140, binding = 0) uniform Parameters { mat4 matrix; vec4 clip; } p;
void main()
{
    const vec2 corners[6] = vec2[6](vec2(0, 0), vec2(1, 0), vec2(0, 1),
                                  vec2(0, 1), vec2(1, 0), vec2(1, 1));
    uv = corners[gl_VertexIndex];
    gl_Position = p.matrix * vec4(uv, 0, 1);
}

#version 450
// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later
layout(binding = 0) uniform sampler2D frameImage;
layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 color;
void main()
{
    color = texture(frameImage, uv);
}

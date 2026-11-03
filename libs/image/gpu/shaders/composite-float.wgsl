// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

fn safeRatio(numerator: f32, denominator: f32) -> f32 {
    if (numerator == 0.0) { return 0.0; }
    return numerator / denominator;
}
fn alphaOverByte(source: f32, destination: u32) -> f32 {
    return f32(destination) + source * (255.0 - f32(destination)) / 255.0;
}
fn roundComponent(value: f32) -> u32 { return u32(round(max(value, 0.0))); }
fn mixByteFloat(destination: u32, source: u32, blend: f32) -> u32 {
    return roundComponent(f32(destination) + f32(i32(source) - i32(destination)) * blend);
}

// SPDX-FileCopyrightText: 2026 LibrePaint contributors
// SPDX-License-Identifier: GPL-2.0-or-later

// Opacity and normalized components use their f32 storage bit patterns.
// All arithmetic below runs as native f32.
fn projectionScaleByte(value: u32, factor: u32) -> u32 {
    return bitcast<u32>(bitcast<f32>(value) * f32(factor));
}

fn projectionMultiply(a: u32, b: u32) -> u32 {
    return bitcast<u32>(bitcast<f32>(a) * bitcast<f32>(b));
}
fn projectionAlpha(sourceAlpha: u32, destinationAlpha: u32) -> u32 {
    let destination = f32(destinationAlpha);
    return bitcast<u32>(destination + bitcast<f32>(sourceAlpha) * (255.0 - destination) / 255.0);
}
fn projectionDivide(a: u32, b: u32) -> u32 {
    if (a == 0u) { return 0u; }
    return bitcast<u32>(bitcast<f32>(a) / bitcast<f32>(b));
}
fn projectionComponent(value: u32) -> u32 {
    return u32(round(max(bitcast<f32>(value), 0.0)));
}
fn projectionColor(destination: u32, source: u32, blend: u32) -> u32 {
    // Below 2^-9 the change is less than half a byte.
    if (blend < 0x3b000000u) { return destination; }
    let difference = f32(i32(source) - i32(destination));
    return u32(round(max(f32(destination) + difference * bitcast<f32>(blend), 0.0)));
}

fn floatAdd(a: u32, b: u32) -> u32 { return bitcast<u32>(bitcast<f32>(a) + bitcast<f32>(b)); }
fn floatSubtract(a: u32, b: u32) -> u32 { return bitcast<u32>(bitcast<f32>(a) - bitcast<f32>(b)); }
fn floatMultiply(a: u32, b: u32) -> u32 { return bitcast<u32>(bitcast<f32>(a) * bitcast<f32>(b)); }
fn floatMultiplyAdd(a: u32, b: u32, c: u32) -> u32 {
    return bitcast<u32>(bitcast<f32>(a) * bitcast<f32>(b) + bitcast<f32>(c));
}
fn floatDivide(a: u32, b: u32) -> u32 {
    if ((a & 0x7fffffffu) == 0u) { return 0u; }
    return bitcast<u32>(bitcast<f32>(a) / bitcast<f32>(b));
}
fn floatSqrt(value: u32) -> u32 { return bitcast<u32>(sqrt(max(bitcast<f32>(value), 0.0))); }

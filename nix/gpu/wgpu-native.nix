# SPDX-FileCopyrightText: 2026 LibrePaint contributors
# SPDX-License-Identifier: GPL-2.0-or-later
{ lib, stdenv, wgpu-native, vulkan-loader }:
wgpu-native.overrideAttrs (previous: {
  patches = (previous.patches or [ ]) ++ [ ./recover-device-loss.patch ./recover-surface-lifetime.patch ];
  cargoDeps = wgpu-native.cargoDeps;
  postFixup = (previous.postFixup or "") + lib.optionalString stdenv.hostPlatform.isLinux ''
    patchelf --add-rpath ${lib.makeLibraryPath [ vulkan-loader ]} $out/lib/libwgpu_native.so
  '';
})

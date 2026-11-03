{ wgpu-native }:

wgpu-native.overrideAttrs (old: {
  patches = (old.patches or [ ]) ++ [ ./patches/report-device-wait-errors.patch ];
})

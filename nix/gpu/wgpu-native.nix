# The unmodified nixpkgs wgpu-native comes from the binary cache. GPU loss is
# handled by the application (libs/image/gpu/KisGpuDevice.cpp), which never
# calls the native operations that abort on a lost device.
{ wgpu-native }:

wgpu-native

{ pkgs }:

let
  libdwarfSource = pkgs.fetchzip {
    url = "https://github.com/davea42/libdwarf-code/archive/8c56a98e99ae367e57b02a5d7f7dab01add878e9.tar.gz";
    hash = "sha256-8VfHO18o5opEpru1g9U94rhF1HyeMk8W4CvoCLKWFQ4=";
  };
  zlibSource = pkgs.fetchzip {
    url = "https://github.com/madler/zlib/archive/21767c654d31d2dccdde4330529775c6c5fd5389.tar.gz";
    hash = "sha256-bIm5+uHv12/x2uqEbZ4/VGzUJnDzW9C3GkyHo3EnC1A=";
  };
in
pkgs.stdenv.mkDerivation {
  pname = "drmingw";
  version = "0.9.11";

  src = pkgs.fetchzip {
    url = "https://github.com/jrfonseca/drmingw/archive/refs/tags/0.9.11.tar.gz";
    hash = "sha256-g7K0JI76Bb5c4C6VIFxtG0AoB51n7yAdDLQ/OpS22YM=";
  };

  strictDeps = true;
  nativeBuildInputs = [
    pkgs.buildPackages.cmake
    pkgs.buildPackages.ninja
  ];

  cmakeGenerator = "Ninja";
  cmakeBuildType = "Release";

  postPatch = ''
    rm -rf thirdparty/libdwarf thirdparty/zlib
    mkdir -p thirdparty/libdwarf thirdparty/zlib
    cp -a ${libdwarfSource}/. thirdparty/libdwarf/
    cp -a ${zlibSource}/. thirdparty/zlib/
    chmod -R u+w thirdparty/libdwarf thirdparty/zlib

    # Nixpkgs' MinGW toolchain uses the POSIX thread model.  DrMingw does not
    # use the affected C++ threading facilities, matching the upstream MSYS2
    # packaging adjustment for this source release.
    substituteInPlace CMakeLists.txt \
      --replace-fail \
        'message (SEND_ERROR "Win32 threads required.")' \
        'message (WARNING "Win32 threads required.")'
  '';

  meta = {
    description = "Just-in-time crash debugger for MinGW applications";
    homepage = "https://github.com/jrfonseca/drmingw";
    license = pkgs.lib.licenses.lgpl21Plus;
    platforms = [ "x86_64-windows" ];
  };
}

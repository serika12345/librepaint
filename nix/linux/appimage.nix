{
  auditLinuxAppImage,
  librepaint,
  mkLinuxAppImage,
  pkgs,
}:

let
  name = "LibrePaint-${librepaint.version}-x86_64.AppImage";
  unverified = mkLinuxAppImage {
    program = "${librepaint}/bin/LibrePaint";
    pname = "LibrePaint";
    inherit name;

    # Runtime store paths remain intact, while development-only content inside
    # otherwise necessary outputs is excluded from the compressed filesystem.
    squashfsArgs = [
      "-comp"
      "zstd"
      "-Xcompression-level"
      "19"
      "-wildcards"
      "-e"
      "'nix/store/*/include'"
      "'nix/store/*/include/*'"
      "'nix/store/*/lib/cmake'"
      "'nix/store/*/lib/cmake/*'"
      "'nix/store/*/lib/kexec-tools'"
      "'nix/store/*/lib/kexec-tools/*'"
      "'nix/store/*/lib/*.pri'"
      "'nix/store/*/lib/*.prl'"
      "'nix/store/*/lib/pkgconfig'"
      "'nix/store/*/lib/pkgconfig/*'"
      "'nix/store/*/mkspecs'"
      "'nix/store/*/mkspecs/*'"
      "'nix/store/*/share/aclocal'"
      "'nix/store/*/share/aclocal/*'"
      "'nix/store/*/share/cmake'"
      "'nix/store/*/share/cmake/*'"
      "'nix/store/*/share/doc'"
      "'nix/store/*/share/doc/*'"
      "'nix/store/*/share/gdb'"
      "'nix/store/*/share/gdb/*'"
      "'nix/store/*/share/gir-*'"
      "'nix/store/*/share/gir-*/*'"
      "'nix/store/*/share/info'"
      "'nix/store/*/share/info/*'"
      "'nix/store/*/share/man'"
      "'nix/store/*/share/man/*'"
      "'nix/store/*/share/pkgconfig'"
      "'nix/store/*/share/pkgconfig/*'"
      "'nix/store/*/share/systemtap'"
      "'nix/store/*/share/systemtap/*'"
      "'nix/store/*/share/vala'"
      "'nix/store/*/share/vala/*'"
      "'nix/store/*/nix-support'"
      "'nix/store/*/nix-support/*'"
      "'nix/store/*-dev'"
      "'nix/store/*-static'"
      "'nix/store/*-pkg-config-*'"
      "'nix/store/*-xorgproto-*'"
      "'nix/store/*-expand-response-params'"
      "'nix/store/*-find-xml-catalogs-hook'"
      "'nix/store/*-strip.sh'"
      "'nix/store/*-libdrm-*-bin'"
      "'nix/store/*-freeglut-*'"
      "'nix/store/*-glew-*'"
      "'nix/store/*-glu-*'"
      "'nix/store/*-libxxf86vm-*'"
      "'${pkgs.lib.removePrefix "/" (toString pkgs.dbus)}'"
      "'nix/store/*/lib/*.a'"
      "'nix/store/*/lib/*.la'"
      "'nix/store/*/lib/*/include'"
      "'nix/store/*/share/*/cmake'"
      "'nix/store/*/share/*/gdb'"
      "'... *.a'"
      "'... *.cmake'"
      "'... *.gir'"
      "'... *.la'"
      "'... *.pc'"
      "'... *.pri'"
      "'... *.prl'"
      "'... *.vapi'"
      "'... assistant'"
      "'... designer'"
      "'... lconvert'"
      "'... linguist'"
      "'... lrelease'"
      "'... lupdate'"
      "'... ocio*'"
      "'... protoc*'"
      "'... qmlcachegen'"
      "'... qmldom'"
      "'... qmllint'"
      "'... qmlplugindump'"
      "'... qmlprofiler'"
      "'... qmlscene'"
      "'... qmltestrunner'"
      "'... qtdiag'"
    ];
  };
in
# Keep validation in the final packaging stage so the application build and
# source-independent dependencies remain reusable across policy changes.
pkgs.runCommand name
  {
    nativeBuildInputs = [
      pkgs.binutils
      pkgs.python3
      pkgs.squashfsTools
    ];
  }
  ''
    python3 ${auditLinuxAppImage} ${unverified} \
      --readelf ${pkgs.binutils}/bin/readelf \
      --unsquashfs ${pkgs.squashfsTools}/bin/unsquashfs
    cp ${unverified} "$out"
  ''

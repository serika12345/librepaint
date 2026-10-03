{
  appImageAppRun,
  appImageExtraFiles,
  appImageRuntime,
  auditLinuxAppImage,
  librepaint,
  pkgs,
}:

let
  name = "LibrePaint-${librepaint.version}-x86_64.AppImage";
  runtimeClosure = pkgs.closureInfo { rootPaths = [ librepaint ]; };
  # Runtime store paths remain intact; development outputs and tools are
  # excluded while assembling the filesystem, before its dependency audit.
  exclusions = pkgs.writeText "librepaint-appimage-exclusions" (
    pkgs.lib.concatStringsSep "\n" [
      "nix/store/*/include"
      "nix/store/*/include/*"
      "nix/store/*/lib/cmake"
      "nix/store/*/lib/cmake/*"
      "nix/store/*/lib/kexec-tools"
      "nix/store/*/lib/kexec-tools/*"
      "nix/store/*/lib/*.pri"
      "nix/store/*/lib/*.prl"
      "nix/store/*/lib/pkgconfig"
      "nix/store/*/lib/pkgconfig/*"
      "nix/store/*/mkspecs"
      "nix/store/*/mkspecs/*"
      "nix/store/*/share/aclocal"
      "nix/store/*/share/aclocal/*"
      "nix/store/*/share/cmake"
      "nix/store/*/share/cmake/*"
      "nix/store/*/share/doc"
      "nix/store/*/share/doc/*"
      "nix/store/*/share/gdb"
      "nix/store/*/share/gdb/*"
      "nix/store/*/share/gir-*"
      "nix/store/*/share/gir-*/*"
      "nix/store/*/share/info"
      "nix/store/*/share/info/*"
      "nix/store/*/share/man"
      "nix/store/*/share/man/*"
      "nix/store/*/share/pkgconfig"
      "nix/store/*/share/pkgconfig/*"
      "nix/store/*/share/systemtap"
      "nix/store/*/share/systemtap/*"
      "nix/store/*/share/vala"
      "nix/store/*/share/vala/*"
      "nix/store/*/nix-support"
      "nix/store/*/nix-support/*"
      "nix/store/*-dev"
      "nix/store/*-static"
      "nix/store/*-pkg-config-*"
      "nix/store/*-xorgproto-*"
      "nix/store/*-expand-response-params"
      "nix/store/*-find-xml-catalogs-hook"
      "nix/store/*-strip.sh"
      "nix/store/*-libdrm-*-bin"
      "nix/store/*-freeglut-*"
      "nix/store/*-glew-*"
      "nix/store/*-glu-*"
      "nix/store/*-libxxf86vm-*"
      "${pkgs.lib.removePrefix "/" (toString pkgs.dbus)}"
      "nix/store/*/lib/*.a"
      "nix/store/*/lib/*.la"
      "nix/store/*/lib/*/include"
      "nix/store/*/share/*/cmake"
      "nix/store/*/share/*/gdb"
      "*.a"
      "*.cmake"
      "*.gir"
      "*.la"
      "*.pc"
      "*.pri"
      "*.prl"
      "*.vapi"
      "assistant"
      "designer"
      "lconvert"
      "linguist"
      "lrelease"
      "lupdate"
      "ocio*"
      "protoc*"
      "qmlcachegen"
      "qmldom"
      "qmllint"
      "qmlplugindump"
      "qmlprofiler"
      "qmlscene"
      "qmltestrunner"
      "qtdiag"
    ]
  );
  # Staging and compression depend only on packaging inputs.
  unverified =
    pkgs.runCommand name
      {
        nativeBuildInputs = [
          pkgs.patchelf
          pkgs.python3
          pkgs.rsync
          pkgs.squashfsTools
        ];
      }
      ''
        mkdir root
        rsync -a -r --chmod=Du+w --files-from=${runtimeClosure}/store-paths \
          --exclude-from=${exclusions} / root/
        ln -s ${librepaint}/bin/LibrePaint root/entrypoint
        rsync -a --chmod=Du+w ${appImageAppRun}/ root/
        ${appImageExtraFiles} ${librepaint}/bin/LibrePaint
        cp -a extras/. root/

        # Nixpkgs bootstrap removes these libraries' build-time libc references
        # with nuke-refs. Drop its inert markers from the staged ELF search paths.
        python3 - <<'PYTHON'
        from pathlib import Path
        import stat
        import subprocess

        for pattern in ("*-libidn2-*/lib/libidn2.so.*.*", "*-libunistring-*/lib/libunistring.so.*.*"):
            for library in Path("root/nix/store").glob(pattern):
                if library.is_symlink() or not library.is_file():
                    continue
                rpath = subprocess.check_output(["patchelf", "--print-rpath", str(library)], text=True).rstrip("\n")
                entries = rpath.split(":")
                retained = [entry for entry in entries
                            if not entry.startswith("/nix/store/eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee-")]
                if entries == retained:
                    continue
                if retained == [""]:
                    raise ValueError(f"{library}: removing erased markers would hide an empty RPATH entry")
                mode = stat.S_IMODE(library.stat().st_mode)
                library.chmod(mode | stat.S_IWUSR)
                try:
                    operation = ["--set-rpath", ":".join(retained)] if retained else ["--remove-rpath"]
                    subprocess.run(["patchelf", *operation, str(library)], check=True)
                finally:
                    library.chmod(mode)
        PYTHON

        mksquashfs root "$out" -noappend -all-root -comp zstd -Xcompression-level 19 \
          -processors "$NIX_BUILD_CORES" -offset $(stat -L -c%s ${appImageRuntime})
        dd if=${appImageRuntime} of="$out" conv=notrunc status=none
        chmod 755 "$out"
      '';
in
# Audit changes reuse the compressed filesystem and inspect the final artifact.
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

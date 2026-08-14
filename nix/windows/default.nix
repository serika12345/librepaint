{
  pkgs,
  source,
}:

let
  inherit (pkgs) lib;
  buildPkgs = pkgs.buildPackages;
  librepaintUnwrapped = import ./krita.nix {
    inherit pkgs source;
  };
  librepaintGmic = (pkgs.krita-plugin-gmic.override {
    cmake = buildPkgs.cmake;
    fftw = pkgs.fftw;
    krita-unwrapped = librepaintUnwrapped;
    kdePackages = librepaintUnwrapped.windowsGmicKdePackages;
    qt6 = librepaintUnwrapped.windowsGmicQtPackages;
  }).overrideAttrs (old: {
    buildInputs = (old.buildInputs or [ ]) ++ [ pkgs.windows.pthreads ];
  });
  pythonVersion = librepaintUnwrapped.windowsPython.pythonVersion;
  pythonCompactVersion = lib.replaceStrings [ "." ] [ "" ] pythonVersion;
  pythonSitePackages = librepaintUnwrapped.windowsPython.sitePackages;
  portableRuntimeDependencies =
    librepaintUnwrapped.windowsRuntimeDependencies
    ++ [ librepaintGmic ];

  # Windows PE import tables retain DLL names rather than Nix store paths.
  # Keep the declared runtime inputs as closure roots, then copy their target
  # DLLs next to the executable for a distributable directory.
  runtimeClosure = buildPkgs.closureInfo {
    rootPaths = [ librepaintUnwrapped ] ++ portableRuntimeDependencies;
  };

  librepaint = buildPkgs.runCommand "librepaint-windows-${librepaintUnwrapped.version}"
    {
      nativeBuildInputs = [
        buildPkgs.coreutils
        buildPkgs.diffutils
        buildPkgs.findutils
        buildPkgs.gnugrep
        buildPkgs.gnused
        buildPkgs.zip
      ];
    }
    ''
      cp -a ${librepaintUnwrapped}/. "$out"
      chmod -R u+w "$out"
      cp -aL ${librepaintGmic}/. "$out/"
      chmod -R u+w "$out"
      mkdir -p "$out/bin/plugins" "$out/bin/qml" "$out/bin/translations"
      mkdir -p "$out/lib/site-packages" "$out/python"
      fontStorePaths="$TMPDIR/font-store-paths"
      : > "$fontStorePaths"

      # The unwrapped result links shared libraries from its Nix closure into
      # bin.  Resolve these links before assembling the portable tree.
      while IFS= read -r -d "" link; do
        target="$(readlink -f "$link")"
        rm "$link"
        cp "$target" "$link"
      done < <(find "$out/bin" -maxdepth 1 -type l -print0)

      copy_dll() {
        sourceDll="$1"
        targetDll="$out/bin/$(basename "$sourceDll")"

        if test -e "$targetDll"; then
          cmp --silent "$sourceDll" "$targetDll"
        else
          cp "$sourceDll" "$targetDll"
        fi
      }

      while IFS= read -r storePath; do
        if test "$storePath" = ${lib.escapeShellArg (toString librepaintUnwrapped)}; then
          continue
        fi

        if test -d "$storePath/bin"; then
          while IFS= read -r -d "" dll; do
            copy_dll "$dll"
          done < <(find -L "$storePath/bin" -maxdepth 1 -type f -iname '*.dll' -print0)
        fi

        qtPlugins="$storePath/lib/qt-6/plugins"
        if test -d "$qtPlugins"; then
          while IFS= read -r -d "" plugin; do
            relativePlugin="''${plugin#"$qtPlugins"/}"
            targetPlugin="$out/bin/plugins/$relativePlugin"
            mkdir -p "$(dirname "$targetPlugin")"
            if test -e "$targetPlugin"; then
              cmp --silent "$plugin" "$targetPlugin"
            else
              cp "$plugin" "$targetPlugin"
            fi
          done < <(find -L "$qtPlugins" -type f -iname '*.dll' -print0)
        fi

        qtQml="$storePath/lib/qt-6/qml"
        if test -d "$qtQml"; then
          chmod -R u+w "$out/bin/qml"
          cp -aL "$qtQml"/. "$out/bin/qml/"
          chmod -R u+w "$out/bin/qml"
        fi

        qtTranslations="$storePath/share/qt-6/translations"
        if test -d "$qtTranslations"; then
          chmod -R u+w "$out/bin/translations"
          cp -aL "$qtTranslations"/. "$out/bin/translations/"
          chmod -R u+w "$out/bin/translations"
        fi

        if test -d "$storePath/lib/mlt-7"; then
          mkdir -p "$out/lib/mlt-7"
          chmod -R u+w "$out/lib/mlt-7"
          cp -aL "$storePath/lib/mlt-7"/. "$out/lib/mlt-7/"
          chmod -R u+w "$out/lib/mlt-7"
        fi
        if test -d "$storePath/share/mlt-7"; then
          mkdir -p "$out/share/mlt-7"
          chmod -R u+w "$out/share/mlt-7"
          cp -aL "$storePath/share/mlt-7"/. "$out/share/mlt-7/"
          chmod -R u+w "$out/share/mlt-7"
        fi
        if test -d "$storePath/share/poppler"; then
          mkdir -p "$out/share/poppler"
          chmod -R u+w "$out/share/poppler"
          cp -aL "$storePath/share/poppler"/. "$out/share/poppler/"
          chmod -R u+w "$out/share/poppler"
        fi

        packageSite="$storePath/${pythonSitePackages}"
        if test -d "$packageSite" \
          && test -n "$(find -L "$packageSite" -type f -iname '*.pyd' -print -quit)"; then
          chmod -R u+w "$out/lib/site-packages"
          cp -aL "$packageSite"/. "$out/lib/site-packages/"
          chmod -R u+w "$out/lib/site-packages"
        fi

        if test -d "$storePath/etc/fonts" && ! test -e "$out/etc/fonts/fonts.conf"; then
          mkdir -p "$out/etc"
          cp -aL "$storePath/etc/fonts" "$out/etc/"
          chmod -R u+w "$out/etc/fonts"
        fi
        if test -d "$storePath/etc/fonts" \
          && test -d "$storePath/share/fontconfig" \
          && ! test -e "$out/share/fontconfig"; then
          mkdir -p "$out/share"
          cp -aL "$storePath/share/fontconfig" "$out/share/"
        fi

        # Fonts named by Fontconfig's generated configuration are Nix store
        # paths.  Preserve those fonts in the portable tree and rewrite the
        # configuration after the closure scan below.
        if test -d "$storePath/share/fonts"; then
          printf '%s\n' "$storePath" >> "$fontStorePaths"
        fi

        # The MinGW compiler package keeps its runtime DLLs under the target
        # triple rather than directly in bin.
        while IFS= read -r -d "" dll; do
          copy_dll "$dll"
        done < <(
          find -L "$storePath" -type f -iname '*.dll' \
            ! -path '*/qt-6/plugins/*' \
            ! -path '*/qt-6/qml/*' \
            ! -path '*/kritaplugins/*' \
            -print0
        )
      done < ${runtimeClosure}/store-paths

      cp ${pkgs.ffmpeg}/bin/ffmpeg.exe "$out/bin/ffmpeg.exe"
      cp ${pkgs.ffmpeg}/bin/ffprobe.exe "$out/bin/ffprobe.exe"

      pythonStdlib=${librepaintUnwrapped.windowsPython}/lib/python${pythonVersion}
      (
        cd "$pythonStdlib"
        find . -type f \
          ! -path './site-packages/*' \
          ! -path './lib-dynload/*' \
          ! -iname '*.pyd' \
          -print \
          | LC_ALL=C sort \
          | zip -X -9 "$out/python/python${pythonCompactVersion}.zip" -@
      )
      while IFS= read -r -d "" extension; do
        cp "$extension" "$out/python/$(basename "$extension")"
      done < <(
        find -L ${librepaintUnwrapped.windowsPython} -type f -iname '*.pyd' \
          ! -path '*/site-packages/*' -print0
      )

      for pythonPackage in ${lib.escapeShellArgs (map toString librepaintUnwrapped.windowsPythonPackages)}; do
        packageSite="$pythonPackage/${pythonSitePackages}"
        if test -d "$packageSite"; then
          chmod -R u+w "$out/lib/site-packages"
          cp -aL "$packageSite"/. "$out/lib/site-packages/"
          chmod -R u+w "$out/lib/site-packages"
        fi
      done

      fontConfig="$out/etc/fonts/fonts.conf"
      if test -f "$fontConfig"; then
        while IFS= read -r storePath; do
          if grep -Fq "<dir>$storePath</dir>" "$fontConfig"; then
            fontPackage="$(basename "$storePath")"
            bundlePath="../../share/fonts/$fontPackage"
            mkdir -p "$out/share/fonts/$fontPackage"
            cp -aL "$storePath/share/fonts"/. "$out/share/fonts/$fontPackage/"
            sed -i \
              "s|<dir>$storePath</dir>|<dir prefix=\"relative\">$bundlePath</dir>|g" \
              "$fontConfig"
          fi
        done < "$fontStorePaths"
        sed -i \
          -e 's|<include ignore_missing="yes">/etc/fonts/conf.d</include>|<include ignore_missing="yes" prefix="relative">conf.d</include>|' \
          -e '\|<cachedir>/var/cache/fontconfig</cachedir>|d' \
          "$fontConfig"
        rm -f "$out/etc/fonts/conf.d/README"
      fi

      cat > "$out/bin/qt.conf" <<'EOF'
      [Paths]
      Prefix=..
      Plugins=bin/plugins
      Qml2Imports=bin/qml
      QmlImports=bin/qml
      Translations=bin/translations
      EOF

      test -x "$out/bin/LibrePaint.exe"
      test -f "$out/bin/krita.dll"
      test -f "$out/bin/qt.conf"
      test -d "$out/bin/plugins/platforms"
      test -f "$out/etc/fonts/fonts.conf"
      test -z "$(find "$out" -type l -print -quit)"
    '';

  librepaintArchive = buildPkgs.runCommand "librepaint-windows-archive-${librepaintUnwrapped.version}"
    {
      nativeBuildInputs = [
        buildPkgs.coreutils
        buildPkgs.findutils
        buildPkgs.zip
      ];
    }
    ''
      mkdir -p "$out"
      cd ${librepaint}
      find . -print | LC_ALL=C sort | zip -X -9 "$out/LibrePaint-${librepaintUnwrapped.version}-x86_64-windows.zip" -@
    '';

  windowsDependencies = buildPkgs.runCommand "librepaint-windows-dependencies"
    { }
    ''
      mkdir -p "$out/nix-support"
      printf '%s\n' ${lib.escapeShellArgs (map toString librepaintUnwrapped.windowsRuntimeDependencies)} \
        > "$out/nix-support/windows-dependency-members"
      printf '%s\n' ${lib.escapeShellArg (toString librepaintGmic)} \
        >> "$out/nix-support/windows-dependency-members"
    '';
in
{
  inherit librepaint librepaintArchive librepaintGmic librepaintUnwrapped windowsDependencies;
}

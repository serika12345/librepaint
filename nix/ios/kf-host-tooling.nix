{
  pkgs,
  versions,
}:

let
  lib = pkgs.lib;

  kfVersion = versions.KRITA_IOS_KF_VERSION;
  qtVersion = versions.KRITA_IOS_QT_VERSION;

  hostEcm = pkgs.kdePackages.extra-cmake-modules;
  hostKConfig = pkgs.kdePackages.kconfig;
  hostQt = pkgs.qt6Packages.qtbase;
  hostQtTools = pkgs.qt6Packages.qttools;

  sourceLocks = {
    ecm = {
      archiveName = "extra-cmake-modules-${kfVersion}.tar.xz";
      archiveSha256 = "8f74af329e4c918358bfe54316f8cf4d530c28444595a3e2fb15075f72877f17";
    };
    kconfig = {
      archiveName = "kconfig-${kfVersion}.tar.xz";
      archiveSha256 = "5572e3e098f7cf5d06c78cd941924333c0e84646d2a18aa6b4c9a6f7eae8a5c9";
    };
    qttools = {
      archiveName = "qttools-everywhere-src-${qtVersion}.tar.xz";
      archiveSha256 = "8dab8f3611496486a470ad5f115ceea584f36bc22a2b8b6f6ebdbafbb8160693";
    };
  };

  # Keep the upstream nixpkgs split explicit even though aarch64-darwin is not
  # an advertised platform for the full package. The bootstrap therefore uses
  # only its source and the smaller compiler derivation below.
  nixpkgsKConfigLayout = {
    outputs = [
      "out"
      "dev"
      "devtools"
    ];
    compilerOutput = "out";
    compilerPath = "libexec/kf6/kconfig_compiler_kf6";
    compilerTargetsOutput = "dev";
    compilerTargetsPath = "lib/cmake/KF6Config/KF6ConfigCompilerTargets.cmake";
  };

  qtToolsLayout = {
    outputs = [
      "out"
      "dev"
    ];
    linguistToolsDir = "lib/cmake/Qt6LinguistTools";
    requiredPaths = [
      "bin/lconvert"
      "bin/lrelease"
      "lib/cmake/Qt6LinguistTools/Qt6LinguistToolsConfig.cmake"
      "lib/cmake/Qt6LinguistTools/Qt6LinguistToolsTargets.cmake"
    ];
  };

  qtToolsContractCheck = pkgs.runCommand "qt-linguist-host-contract-${qtVersion}" { } ''
    source_archive=${lib.escapeShellArg (toString hostQtTools.src)}
    case "$source_archive" in
      *-${sourceLocks.qttools.archiveName}) ;;
      *)
        echo "error: unexpected QtTools source archive: $source_archive" >&2
        exit 1
        ;;
    esac
    actual_source_sha256="$(sha256sum "$source_archive" | cut -d ' ' -f 1)"
    if test "$actual_source_sha256" != "${sourceLocks.qttools.archiveSha256}"; then
      echo "error: QtTools source hash is $actual_source_sha256; expected ${sourceLocks.qttools.archiveSha256}" >&2
      exit 1
    fi

    for relative_path in ${lib.escapeShellArgs qtToolsLayout.requiredPaths}; do
      test -e ${hostQtTools}/"$relative_path"
    done
    test "$(${hostQtTools}/bin/lrelease -version)" = "lrelease version ${qtVersion}"

    mkdir -p "$out"
    touch "$out/passed"
  '';

  # Building the complete host KConfig package would pull its GUI and QML
  # runtime closure into every iOS framework build.  Only the XML-based code
  # generator is executable during a cross build, so keep it in a dedicated
  # native derivation with Qt Core/Xml as its sole Qt dependency.
  kconfigCompiler = pkgs.stdenv.mkDerivation {
    pname = "krita-kconfig-compiler-host";
    version = kfVersion;
    src = hostKConfig.src;

    strictDeps = true;
    # The compiler uses only Qt Core/Xml and never loads Qt plugins.  Keep its
    # native executable unwrapped so the output contract remains one file.
    dontWrapQtApps = true;
    nativeBuildInputs = [
      hostEcm
      pkgs.cmake
      pkgs.coreutils
      pkgs.file
      pkgs.findutils
      pkgs.gnugrep
      pkgs.gnused
      pkgs.ninja
    ];
    buildInputs = [ hostQt ];

    cmakeBuildType = "Release";
    cmakeFlags = [
      "-DBUILD_QCH=OFF"
      "-DBUILD_SHARED_LIBS=OFF"
      "-DBUILD_TESTING=OFF"
      "-DCMAKE_DISABLE_FIND_PACKAGE_Qt6LinguistTools=TRUE"
      "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE"
      "-DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE"
      "-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE"
      "-DCMAKE_IGNORE_PREFIX_PATH=/opt/homebrew;/usr/local"
      "-DECM_DIR=${hostEcm}/share/ECM/cmake"
      "-DKCONFIG_USE_GUI=OFF"
      "-DKCONFIG_USE_QML=OFF"
      "-DKF_SKIP_PO_PROCESSING=ON"
      "-DQt6_DIR=${hostQt}/lib/cmake/Qt6"
      "-DUSE_DBUS=OFF"
    ];

    preConfigure = ''
      verify_source_archive() {
        source_archive="$1"
        expected_name="$2"
        expected_sha256="$3"

        case "$source_archive" in
          *-"$expected_name") ;;
          *)
            echo "error: unexpected source archive: $source_archive (expected $expected_name)" >&2
            exit 1
            ;;
        esac

        actual_sha256="$(sha256sum "$source_archive" | cut -d ' ' -f 1)"
        if test "$actual_sha256" != "$expected_sha256"; then
          echo "error: $expected_name hash is $actual_sha256; expected $expected_sha256" >&2
          exit 1
        fi
      }

      verify_source_archive \
        ${lib.escapeShellArg (toString hostKConfig.src)} \
        ${lib.escapeShellArg sourceLocks.kconfig.archiveName} \
        ${lib.escapeShellArg sourceLocks.kconfig.archiveSha256}
      verify_source_archive \
        ${lib.escapeShellArg (toString hostEcm.src)} \
        ${lib.escapeShellArg sourceLocks.ecm.archiveName} \
        ${lib.escapeShellArg sourceLocks.ecm.archiveSha256}

      test -f ${hostEcm}/share/ECM/cmake/ECMConfig.cmake
      test -f ${hostEcm}/share/ECM/cmake/ECMConfigVersion.cmake
    '';

    buildPhase = ''
      runHook preBuild
      cmake --build . --target kconfig_compiler --parallel "$NIX_BUILD_CORES"
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall

      compiler_list="$NIX_BUILD_TOP/kconfig-compiler-list"
      find . -type f -name kconfig_compiler_kf6 -perm -111 -print > "$compiler_list"
      compiler_count="$(wc -l < "$compiler_list" | tr -d ' ')"
      if test "$compiler_count" -ne 1; then
        echo "error: expected exactly one native kconfig_compiler_kf6; found $compiler_count" >&2
        cat "$compiler_list" >&2
        exit 1
      fi

      compiler="$(sed -n '1p' "$compiler_list")"
      install -Dm755 "$compiler" "$out/libexec/kf6/kconfig_compiler_kf6"

      runHook postInstall
    '';

    doInstallCheck = true;
    installCheckPhase = ''
      runHook preInstallCheck

      compiler="$out/libexec/kf6/kconfig_compiler_kf6"
      if ! test -x "$compiler"; then
        echo "error: host KConfig compiler is missing or not executable: $compiler" >&2
        exit 1
      fi

      actual_version="$("$compiler" --version)"
      if test "$actual_version" != "kconfig_compiler ${kfVersion}"; then
        echo "error: host KConfig compiler version is '$actual_version'; expected 'kconfig_compiler ${kfVersion}'" >&2
        exit 1
      fi

      compiler_description="$(file -b "$compiler")"
      for required_marker in 'Mach-O' '64-bit' 'arm64' 'executable'; do
        case "$compiler_description" in
          *"$required_marker"*) ;;
          *)
            echo "error: host KConfig compiler is not a native arm64 Mach-O executable: $compiler_description" >&2
            exit 1
            ;;
        esac
      done

      extra_entries="$(find "$out" \( -type f -o -type l \) ! -path "$compiler" -print)"
      if test -n "$extra_entries"; then
        echo "error: host KConfig output contains entries other than kconfig_compiler_kf6" >&2
        printf '%s\n' "$extra_entries" >&2
        exit 1
      fi

      runHook postInstallCheck
    '';

    passthru = {
      inherit
        hostEcm
        hostQt
        nixpkgsKConfigLayout
        sourceLocks
        ;
      patches = [ ];
    };

    meta = {
      description = "Minimal native KConfig compiler for Krita iOS cross builds";
      license = hostKConfig.meta.license;
      platforms = [ "aarch64-darwin" ];
    };
  };

  compilerTargets = pkgs.writeText "KF6ConfigCompilerTargets.cmake" ''
    if(NOT TARGET KF6::kconfig_compiler)
      add_executable(KF6::kconfig_compiler IMPORTED GLOBAL)
      set_target_properties(KF6::kconfig_compiler PROPERTIES
        IMPORTED_LOCATION "${kconfigCompiler}/libexec/kf6/kconfig_compiler_kf6"
      )
    endif()
  '';

  kf6HostTooling =
    pkgs.runCommand "krita-kf6-host-tooling-${kfVersion}"
      {
        version = kfVersion;
        passthru = {
          inherit
            hostEcm
            hostQt
            hostQtTools
            kconfigCompiler
            nixpkgsKConfigLayout
            qtToolsContractCheck
            qtToolsLayout
            sourceLocks
            ;
          ecmDir = "${hostEcm}/share/ECM/cmake";
          linguistToolsDir = "${hostQtTools}/lib/cmake/Qt6LinguistTools";
        };
      }
      ''
        mkdir -p "$out/KF6Config"
        ln -s ${compilerTargets} "$out/KF6Config/KF6ConfigCompilerTargets.cmake"

        test -x ${kconfigCompiler}/libexec/kf6/kconfig_compiler_kf6
        test -f "$out/KF6Config/KF6ConfigCompilerTargets.cmake"
        grep -Fq 'KF6::kconfig_compiler' "$out/KF6Config/KF6ConfigCompilerTargets.cmake"
        grep -Fq ${lib.escapeShellArg "${kconfigCompiler}/libexec/kf6/kconfig_compiler_kf6"} \
          "$out/KF6Config/KF6ConfigCompilerTargets.cmake"
      '';
in
assert lib.assertMsg (
  hostEcm.version == kfVersion
) "host ECM ${hostEcm.version} does not match pinned KF ${kfVersion}";
assert lib.assertMsg (
  hostEcm.outputs == [ "out" ]
) "host ECM must remain a single architecture-independent output";
assert lib.assertMsg (
  hostKConfig.version == kfVersion
) "host KConfig ${hostKConfig.version} does not match pinned KF ${kfVersion}";
assert lib.assertMsg (
  hostKConfig.outputs == nixpkgsKConfigLayout.outputs
) "host KConfig output split changed; expected out, dev, and devtools";
assert lib.assertMsg (
  hostKConfig ? dev
) "host KConfig no longer exposes the dev output containing CompilerTargets";
assert lib.assertMsg (
  hostQt.version == qtVersion
) "host Qt ${hostQt.version} does not match pinned Qt ${qtVersion}";
assert lib.assertMsg (
  hostQtTools.version == qtVersion
) "host Qt tools ${hostQtTools.version} do not match pinned Qt ${qtVersion}";
assert lib.assertMsg (
  hostQtTools.outputs == qtToolsLayout.outputs
) "host Qt tools output split changed; expected out and dev";
{
  inherit
    hostEcm
    hostQt
    hostQtTools
    kconfigCompiler
    kf6HostTooling
    nixpkgsKConfigLayout
    qtToolsContractCheck
    qtToolsLayout
    sourceLocks
    ;
}

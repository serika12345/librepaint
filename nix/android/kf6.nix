{
  androidAbi,
  androidNdkRoot,
  androidSdkRoot,
  frameworkManifestFile,
  pkgs,
  qtbase,
  sourceDependencies,
}:

let
  inherit (pkgs) lib;
  manifest = lib.importJSON frameworkManifestFile;
  packageByName = builtins.listToAttrs (
    map (package: {
      name = package.name;
      value = package;
    }) manifest.packages
  );
  mkCMakePackage = import ./mk-cmake-package.nix {
    inherit androidAbi androidNdkRoot pkgs;
  };
  hostEcm = pkgs.kdePackages.extra-cmake-modules;
  hostKConfig = pkgs.kdePackages.kconfig;
  hostQt = qtbase.hostQt;
  hostQtTools = pkgs.qt6.qttools;
  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  androidSysroot = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/sysroot";
  androidPlatformLibDir = "${androidSysroot}/usr/lib/${androidTriple}/28";

  cmakeBoolean = value: if value then "ON" else "OFF";
  booleanFlags = values: lib.mapAttrsToList (name: value: "-D${name}=${cmakeBoolean value}") values;

  mkFramework =
    {
      name,
      sourcePackage,
      frameworkDependencies ? [ ],
      extraNativeBuildInputs ? [ ],
      extraRequiredPaths ? [ ],
      libraries,
      postInstall ? "",
      preConfigureCommands ? "",
    }:
    let
      packageSpec = packageByName.${name};
      packageBooleanFlags = booleanFlags (packageSpec.configure_locks.cache_boolean_locks or { });
    in
    assert lib.assertMsg (
      sourcePackage.version == packageSpec.version
    ) "KF6 ${name} source version must match the pinned framework manifest";
    mkCMakePackage {
      pname = "${name}-android-${androidAbi}";
      inherit (packageSpec) version;
      src = sourcePackage.src;
      cmakeToolchainFile = "${qtbase}/lib/cmake/Qt6/qt.toolchain.cmake";
      dependencies = [ qtbase ] ++ frameworkDependencies;
      nativeBuildInputs = [
        pkgs.gettext
        pkgs.python3
      ]
      ++ extraNativeBuildInputs;
      inherit postInstall preConfigureCommands;
      cmakeFlags = [
        "-DBUILD_COVERAGE=OFF"
        "-DBUILD_DESIGNERPLUGIN=OFF"
        "-DBUILD_PYTHON_BINDINGS=OFF"
        "-DBUILD_QCH=OFF"
        "-DBUILD_SHARED_LIBS=ON"
        "-DBUILD_TESTING=OFF"
        "-DBUILD_WITH_QT6=ON"
        "-DECM_DIR=${hostEcm}/share/ECM/cmake"
        "-DENABLE_CLAZY=OFF"
        "-DKDE_INSTALL_INCLUDEDIR=include"
        "-DKDE_INSTALL_LIBDIR=lib"
        "-DKDE_INSTALL_LIBEXECDIR=libexec"
        "-DKDE_INSTALL_USE_QT_SYS_PATHS=OFF"
        "-DKF6_HOST_TOOLING=${lib.getDev hostKConfig}/lib/cmake"
        "-DKF_IGNORE_PLATFORM_CHECK=ON"
        "-DKF_SKIP_PO_PROCESSING=ON"
        "-DEGL_INCLUDE_DIR=${androidSysroot}/usr/include"
        "-DEGL_LIBRARY=${androidPlatformLibDir}/libEGL.so"
        "-DGLESv2_INCLUDE_DIR=${androidSysroot}/usr/include"
        "-DGLESv2_LIBRARY=${androidPlatformLibDir}/libGLESv2.so"
        "-DQT_HOST_PATH=${hostQt}"
        "-DQT_HOST_PATH_CMAKE_DIR=${hostQt}/lib/cmake"
        "-DQt6HostInfo_DIR=${hostQt}/lib/cmake/Qt6HostInfo"
        "-DQt6CoreTools_DIR=${hostQt}/lib/cmake/Qt6CoreTools"
        "-DQt6GuiTools_DIR=${hostQt}/lib/cmake/Qt6GuiTools"
        "-DQt6WidgetsTools_DIR=${hostQt}/lib/cmake/Qt6WidgetsTools"
        "-DQt6LinguistTools_DIR=${hostQtTools}/lib/cmake/Qt6LinguistTools"
      ]
      ++ packageBooleanFlags;
      requiredPaths =
        builtins.filter (path: lib.hasPrefix "lib/cmake/" path) packageSpec.required_paths
        ++ map (library: "lib/${library}") libraries
        ++ extraRequiredPaths;
      meta = {
        description = "KDE Frameworks ${name} built from source for Android ${androidAbi}";
        inherit (sourcePackage.meta) license;
      };
    };

  frameworks = rec {
    kconfig = mkFramework {
      name = "kconfig";
      sourcePackage = pkgs.kdePackages.kconfig;
      libraries = [
        "libKF6ConfigCore.so"
        "libKF6ConfigGui.so"
      ];
    };

    kwidgetsaddons = mkFramework {
      name = "kwidgetsaddons";
      sourcePackage = pkgs.kdePackages.kwidgetsaddons;
      libraries = [ "libKF6WidgetsAddons.so" ];
    };

    kcodecs = mkFramework {
      name = "kcodecs";
      sourcePackage = pkgs.kdePackages.kcodecs;
      libraries = [ "libKF6Codecs.so" ];
    };

    kcompletion = mkFramework {
      name = "kcompletion";
      sourcePackage = pkgs.kdePackages.kcompletion;
      frameworkDependencies = [
        kcodecs
        kconfig
        kwidgetsaddons
      ];
      libraries = [ "libKF6Completion.so" ];
    };

    kcoreaddons = mkFramework {
      name = "kcoreaddons";
      sourcePackage = pkgs.kdePackages.kcoreaddons;
      libraries = [ "libKF6CoreAddons.so" ];
    };

    kguiaddons = mkFramework {
      name = "kguiaddons";
      sourcePackage = pkgs.kdePackages.kguiaddons;
      extraNativeBuildInputs = [ pkgs.jdk17_headless ];
      extraRequiredPaths = [
        "jar/KF6GuiAddons.aar"
        "lib/KF6GuiAddons_${androidAbi}-android-dependencies.xml"
      ];
      libraries = [ "libKF6GuiAddons.so" ];
      preConfigureCommands = ''
        substituteInPlace src/CMakeLists.txt \
          --replace-fail '    add_subdirectory(android)' '    # The AAR is assembled reproducibly during installation.'
      '';
      postInstall = ''
        aarRoot="$NIX_BUILD_TOP/kguiaddons-aar"
        mkdir -p "$aarRoot/classes" "$aarRoot/content"
        ${pkgs.jdk17_headless}/bin/javac \
          -source 8 \
          -target 8 \
          -encoding UTF-8 \
          -classpath ${androidSdkRoot}/platforms/android-35/android.jar \
          -d "$aarRoot/classes" \
          src/android/org/kde/guiaddons/*.java
        ${pkgs.jdk17_headless}/bin/jar \
          --create \
          --file "$aarRoot/content/classes.jar" \
          --date=1980-01-01T00:00:02Z \
          -C "$aarRoot/classes" .
        cp src/android/AndroidManifest.xml "$aarRoot/content/AndroidManifest.xml"
        mkdir -p "$out/jar"
        ${pkgs.jdk17_headless}/bin/jar \
          --create \
          --no-manifest \
          --file "$out/jar/KF6GuiAddons.aar" \
          --date=1980-01-01T00:00:02Z \
          -C "$aarRoot/content" .
        install -Dm644 \
          src/android/KF6GuiAddons-android-dependencies.xml \
          "$out/lib/KF6GuiAddons_${androidAbi}-android-dependencies.xml"
      '';
    };

    ki18n = mkFramework {
      name = "ki18n";
      sourcePackage = pkgs.kdePackages.ki18n;
      frameworkDependencies = [ sourceDependencies.libintl ];
      libraries = [ "libKF6I18n.so" ];
    };

    kitemviews = mkFramework {
      name = "kitemviews";
      sourcePackage = pkgs.kdePackages.kitemviews;
      libraries = [ "libKF6ItemViews.so" ];
    };

    kcolorscheme = mkFramework {
      name = "kcolorscheme";
      sourcePackage = pkgs.kdePackages.kcolorscheme;
      frameworkDependencies = [
        kconfig
        kguiaddons
        ki18n
      ];
      libraries = [ "libKF6ColorScheme.so" ];
    };
  };
in
assert lib.assertMsg (
  manifest.frameworks_version == pkgs.kdePackages.kconfig.version
) "Android KF6 packages must use the pinned framework version";
frameworks

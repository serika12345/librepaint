{
  description = "Reproducible LibrePaint builds for supported platforms";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    nix-appimage = {
      url = "github:ralismark/nix-appimage";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    inputs@{ nixpkgs, ... }:
    let
      system = "aarch64-darwin";
      pkgs = import nixpkgs { inherit system; };
      linuxSystem = "x86_64-linux";
      linuxPkgs = import nixpkgs { system = linuxSystem; };
      mkLibrepaintBuildSource =
        packageSet:
        packageSet.lib.cleanSourceWith {
          name = "librepaint-source";
          src = ./.;
          filter =
            path: _type:
            let
              relativePath = packageSet.lib.removePrefix "${toString ./.}/" (toString path);
              topLevel = builtins.head (packageSet.lib.splitString "/" relativePath);
            in
            !(builtins.elem topLevel [
              ".cache"
              ".git"
              ".github"
              ".gitlab"
              "AGENTS.md"
              "TODO.md"
              "build-ios"
              "docs"
              "flake.lock"
              "flake.nix"
              "logs"
              "nix"
              "result"
            ])
            && !(packageSet.lib.hasPrefix "README" topLevel);
        };
      librepaintBuildSource = mkLibrepaintBuildSource pkgs;
      linuxBuildSource = mkLibrepaintBuildSource linuxPkgs;
      linuxAppImageAppRun = import ./nix/linux/appimage-apprun.nix {
        nixAppImage = inputs.nix-appimage;
        pkgs = linuxPkgs;
      };
      mkLinuxAppImage = inputs.nix-appimage.lib.${linuxSystem}.mkAppImage.override {
        mkappimage-apprun = linuxAppImageAppRun;
      };
      iosPackages = import ./nix/ios {
        inherit pkgs;
        versionsFile = ./packaging/ios/versions.env;
        dependencyManifestFile = ./packaging/ios/deps/dependencies.json;
        qtManifestFile = ./packaging/ios/qt/modules.json;
        frameworkManifestFile = ./packaging/ios/frameworks/frameworks.json;
        pluginProfileFile = ./packaging/ios/manifests/initial-plugin-profile.json;
        kritaSource = librepaintBuildSource;
      };
      macosPackages = import ./nix/macos {
        inherit pkgs;
        source = librepaintBuildSource;
      };
      linuxPackages = import ./nix/linux {
        pkgs = linuxPkgs;
        source = linuxBuildSource;
        inherit mkLinuxAppImage;
      };
      linuxAndroidPackages = import ./nix/android {
        pkgs = linuxPkgs;
        source = linuxBuildSource;
      };
    in
    {
      # macOS is the native default package. iOS package outputs cross-compile
      # inside individual Nix derivations with the pinned Xcode SDK input.
      packages.${system} = {
        default = macosPackages.librepaint;
        librepaint-macos = macosPackages.librepaint;
        macos-dependencies = macosPackages.macosDependencies;

        source-zlib = pkgs.zlib.src;
        source-libdeflate = pkgs.libdeflate.src;
        source-libpng = pkgs.libpng.src;
        source-libjpeg-turbo = pkgs.libjpeg_turbo.src;
        source-libwebp = pkgs.libwebp.src;
        source-libtiff = pkgs.libtiff.src;
        source-openjpeg = pkgs.openjpeg.src;
        source-imath = pkgs.imath.src;
        source-openexr = pkgs.openexr.src;
        source-libffi = pkgs.libffiReal.src;
        source-pcre2 = pkgs.pcre2.src;
        source-glib = pkgs.glib.src;
        source-json-c = pkgs.json_c.src;
        source-libmypaint = pkgs.libmypaint.src;
        source-expat = pkgs.expat.src;
        source-boost = pkgs.boost.src;
        source-immer = pkgs.immer.src;
        source-zug = pkgs.zug.src;
        source-lager = pkgs.lager.src;
        source-eigen = pkgs.eigen.src;
        source-exiv2 = pkgs.exiv2.src;
        source-lcms2 = pkgs.lcms2.src;
        source-xsimd = pkgs.xsimd.src;
        source-quazip = pkgs.qt6Packages.quazip.src;
        source-freetype = pkgs.freetype.src;
        source-harfbuzz = pkgs.harfbuzz.src;
        source-fontconfig = pkgs.fontconfig.src;
        source-fribidi = pkgs.fribidi.src;
        source-giflib = pkgs.giflib.src;
        source-brotli = pkgs.brotli.src;
        source-libhwy = pkgs.libhwy.src;
        source-libde265 = pkgs.libde265.src;
        source-x265 = pkgs.x265.src;
        source-libaom = pkgs.libaom.src;
        source-libheif = pkgs.libheif.src;
        source-libjxl = pkgs.libjxl.src;
        source-libraw = pkgs.libraw.src;
        source-yaml-cpp = pkgs.yaml-cpp.src;
        source-pystring = pkgs.pystring.src;
        source-minizip-ng = pkgs.minizip-ng.src;
        source-opencolorio = pkgs.opencolorio.src;
        source-poppler = pkgs.poppler.src;
        source-libkdcraw = pkgs.kdePackages.libkdcraw.src;
        source-kseexpr = iosPackages.kseexpr-source;
        source-gettext = pkgs.gettext.src;
        source-libunibreak = pkgs.libunibreak.src;
        source-qtbase = pkgs.qt6Packages.qtbase.src;
        source-qtsvg = pkgs.qt6Packages.qtsvg.src;
        source-qt5compat = pkgs.qt6Packages.qt5compat.src;
        source-ecm = pkgs.kdePackages.extra-cmake-modules.src;
        source-kconfig = pkgs.kdePackages.kconfig.src;
        source-kcodecs = pkgs.kdePackages.kcodecs.src;
        source-kwidgetsaddons = pkgs.kdePackages.kwidgetsaddons.src;
        source-kcompletion = pkgs.kdePackages.kcompletion.src;
        source-kcoreaddons = pkgs.kdePackages.kcoreaddons.src;
        source-kguiaddons = pkgs.kdePackages.kguiaddons.src;
        source-ki18n = pkgs.kdePackages.ki18n.src;
        source-kitemviews = pkgs.kdePackages.kitemviews.src;
        source-kcolorscheme = pkgs.kdePackages.kcolorscheme.src;

        host-qtbase = pkgs.qt6Packages.qtbase;
        host-qttools = pkgs.qt6Packages.qttools;
        host-ecm = pkgs.kdePackages.extra-cmake-modules;

        inherit (iosPackages)
          glib-ios
          json-c-ios
          brotli-ios
          giflib-ios
          libffi-ios
          libaom-ios
          libde265-ios
          libheif-ios
          libhwy-ios
          libjxl-ios
          libkdcraw-ios
          libraw-ios
          libmypaint-ios
          pcre2-ios
          boost-ios
          eigen-ios
          exiv2-ios
          expat-ios
          fribidi-ios
          fontconfig-ios
          freetype-ios
          harfbuzz-ios
          host-kconfig-compiler
          immer-ios
          imath-ios
          ios-base-dependencies
          ios-dependencies
          ipa-permissions-check
          kcodecs-ios
          kcolorscheme-ios
          kcompletion-ios
          kconfig-ios
          kcoreaddons-ios
          kf6-consumer-check
          kf6-ios-dependencies
          kguiaddons-ios
          ki18n-ios
          kitemviews-ios
          krita-ios-app
          krita-ios-incremental-env
          krita-ios-ipa
          lager-ios
          lcms2-ios
          kf6-host-tooling
          libintl-ios
          libdeflate-ios
          libjpeg-turbo-ios
          libtiff-ios
          libunibreak-ios
          libwebp-ios
          openexr-ios
          openjpeg-ios
          opencolorio-ios
          poppler-ios
          pystring-ios
          libpng-ios
          qt5compat-ios
          qt-ios-dependencies
          qtbase-ios
          qtsvg-ios
          qt-xcrun-shim
          qttools-host-contract-check
          quazip-ios
          kseexpr-ios
          minizip-ng-ios
          xsimd-ios
          x265-ios
          yaml-cpp-ios
          zlib-ios
          zug-ios
          kwidgetsaddons-ios
          ;

        librepaint-ios-app = iosPackages.krita-ios-app;
        librepaint-ios-incremental-env = iosPackages.krita-ios-incremental-env;
        librepaint-ios-ipa = iosPackages.krita-ios-ipa;
      };

      packages.${linuxSystem} = {
        default = linuxPackages.librepaint;
        librepaint-linux = linuxPackages.librepaint;
        librepaint-linux-appimage = linuxPackages.librepaintAppImage;
        linux-dependencies = linuxPackages.linuxDependencies;
        librepaint-android = linuxAndroidPackages.librepaint;
        android-dependencies = linuxAndroidPackages.androidDependencies;
      };

      checks.${system} = {
        librepaint-macos = macosPackages.librepaint;
        macos-dependencies = macosPackages.macosDependencies;

        inherit (iosPackages)
          glib-ios
          json-c-ios
          brotli-ios
          giflib-ios
          libffi-ios
          libaom-ios
          libde265-ios
          libheif-ios
          libhwy-ios
          libjxl-ios
          libkdcraw-ios
          libraw-ios
          libmypaint-ios
          pcre2-ios
          boost-consumer-check
          boost-ios
          eigen-ios
          exiv2-consumer-check
          exiv2-ios
          expat-ios
          fribidi-consumer-check
          fribidi-ios
          fontconfig-consumer-check
          fontconfig-ios
          freetype-consumer-check
          freetype-ios
          harfbuzz-consumer-check
          harfbuzz-ios
          host-kconfig-compiler
          immer-consumer-check
          immer-ios
          imath-ios
          ios-base-dependencies
          ios-dependencies
          ipa-permissions-check
          kcodecs-ios
          kcolorscheme-ios
          kcompletion-ios
          kconfig-ios
          kcoreaddons-ios
          kf6-consumer-check
          kf6-ios-dependencies
          kguiaddons-ios
          ki18n-ios
          kitemviews-ios
          lager-consumer-check
          lager-ios
          lcms2-ios
          kf6-host-tooling
          libintl-consumer-check
          libintl-ios
          libdeflate-ios
          libjpeg-turbo-consumer-check
          libjpeg-turbo-ios
          libtiff-ios
          libunibreak-consumer-check
          libunibreak-ios
          libwebp-ios
          openexr-ios
          openjpeg-ios
          opencolorio-ios
          poppler-ios
          pystring-ios
          libpng-ios
          qt5compat-ios
          qt-ios-dependencies
          qtbase-ios
          qtsvg-ios
          qttools-host-contract-check
          quazip-ios
          kseexpr-ios
          minizip-ng-ios
          xsimd-consumer-check
          xsimd-ios
          x265-ios
          yaml-cpp-ios
          zlib-ios
          zug-consumer-check
          zug-ios
          kwidgetsaddons-ios
          ;
      };

      checks.${linuxSystem} = {
        librepaint-linux = linuxPackages.librepaint;
        linux-dependencies = linuxPackages.linuxDependencies;
      };

      devShells.${system} = {
        default = macosPackages.devShell;
        librepaint-macos = macosPackages.devShell;

        librepaint-ios = pkgs.mkShellNoCC {
          packages = with pkgs; [
            bash
            cmake
            coreutils
            file
            git
            gnugrep
            gnused
            gnumake
            gperf
            gettext
            jq
            meson
            ninja
            nixfmt
            pkg-config
            python3
            xz
          ];

          shellHook = ''
            export LIBREPAINT_IOS_REPO_ROOT="$PWD"
            export LIBREPAINT_IOS_BUILD_ROOT="$PWD/build-ios"
            export LIBREPAINT_IOS_LOG_ROOT="$PWD/logs/ios"
            export KRITA_IOS_REPO_ROOT="$LIBREPAINT_IOS_REPO_ROOT"
            export KRITA_IOS_BUILD_ROOT="$LIBREPAINT_IOS_BUILD_ROOT"
            export KRITA_IOS_LOG_ROOT="$LIBREPAINT_IOS_LOG_ROOT"
            echo "iPadOS development shell"
            echo "  host check: packaging/ios/scripts/check-host.sh"
            echo "  smoke test: packaging/ios/scripts/build-smoke.sh device"
          '';
        };

        librepaint-ios-incremental = iosPackages.krita-ios-incremental-env;
        krita-ios-incremental = iosPackages.krita-ios-incremental-env;
      };

      devShells.${linuxSystem} = {
        default = linuxPackages.devShell;
        librepaint-linux = linuxPackages.devShell;
        librepaint-android = linuxAndroidPackages.devShell;
      };

      formatter.${system} = pkgs.nixfmt;
      formatter.${linuxSystem} = linuxPkgs.nixfmt;
    };
}

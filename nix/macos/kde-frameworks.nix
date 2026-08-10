{ pkgs }:

let
  inherit (pkgs) lib;

  commonFlags = [
    "-DBUILD_COVERAGE=OFF"
    "-DBUILD_DESIGNERPLUGIN=OFF"
    "-DBUILD_PYTHON_BINDINGS=OFF"
    "-DBUILD_QCH=OFF"
    "-DBUILD_SHARED_LIBS=ON"
    "-DBUILD_TESTING=OFF"
    "-DENABLE_CLAZY=OFF"
    "-DKDE_INSTALL_USE_QT_SYS_PATHS=OFF"
    "-DKDE_L10N_AUTO_TRANSLATIONS=OFF"
    "-DKDE_L10N_SYNC_TRANSLATIONS=OFF"
    "-DKF_IGNORE_PLATFORM_CHECK=ON"
  ];

  mkDarwinFramework =
    final: args:
    final.mkKdeDerivation (
      args
      // {
        extraCmakeFlags = commonFlags ++ (args.extraCmakeFlags or [ ]);
        meta = (args.meta or { }) // {
          platforms = lib.platforms.darwin;
        };
      }
    );

  kde = pkgs.kdePackages.overrideScope (
    final: _prev: {
      kconfig = mkDarwinFramework final {
        pname = "kconfig";
        extraNativeBuildInputs = [ final.qttools ];
        extraCmakeFlags = [
          "-DKCONFIG_USE_GUI=ON"
          "-DKCONFIG_USE_QML=OFF"
          "-DUSE_DBUS=OFF"
        ];
      };

      kwidgetsaddons = mkDarwinFramework final {
        pname = "kwidgetsaddons";
        extraNativeBuildInputs = [ final.qttools ];
      };

      kcodecs = mkDarwinFramework final {
        pname = "kcodecs";
        extraNativeBuildInputs = [
          final.qttools
          pkgs.gperf
        ];
      };

      kcompletion = mkDarwinFramework final {
        pname = "kcompletion";
        extraNativeBuildInputs = [ final.qttools ];
      };

      kcoreaddons = mkDarwinFramework final {
        pname = "kcoreaddons";
        extraNativeBuildInputs = [
          final.qttools
          pkgs.shared-mime-info
        ];
      };

      kguiaddons = mkDarwinFramework final {
        pname = "kguiaddons";
        excludeDependencies = [
          "plasma-wayland-protocols"
          "wayland"
          "wayland-protocols"
        ];
        extraCmakeFlags = [
          "-DBUILD_GEO_SCHEME_HANDLER=OFF"
          "-DCMAKE_DISABLE_FIND_PACKAGE_Qt6WaylandClient=TRUE"
          "-DCMAKE_DISABLE_FIND_PACKAGE_Wayland=TRUE"
          "-DCMAKE_DISABLE_FIND_PACKAGE_X11=TRUE"
          "-DUSE_DBUS=OFF"
        ];
      };

      ki18n = mkDarwinFramework final {
        pname = "ki18n";
        extraNativeBuildInputs = [ pkgs.python3 ];
        propagatedNativeBuildInputs = [ pkgs.gettext ];
        extraCmakeFlags = [
          "-DBUILD_WITH_QML=OFF"
          "-DFALLBACK_KI18N_PYTHON_EXECUTABLE=python3"
          "-DKI18N_EMBEDDED_ISO_CODES_CACHE=OFF"
        ];
      };

      kitemviews = mkDarwinFramework final {
        pname = "kitemviews";
        extraNativeBuildInputs = [ final.qttools ];
      };

      kcolorscheme = mkDarwinFramework final {
        pname = "kcolorscheme";
      };

      libkdcraw = mkDarwinFramework final {
        pname = "libkdcraw";
        extraNativeBuildInputs = [ pkgs.pkg-config ];
        extraBuildInputs = [ pkgs.libraw ];
      };
    }
  );
in
{
  inherit (kde)
    kcodecs
    kcolorscheme
    kcompletion
    kconfig
    kcoreaddons
    kguiaddons
    ki18n
    kitemviews
    kwidgetsaddons
    libkdcraw
    ;
}

{
  pkgs,
  source,
}:

let
  inherit (pkgs) lib;
  buildPkgs = pkgs.buildPackages;
  # Qt's host tools do not need systemd/journald.  Leaving it enabled brings
  # a hardware TPM integration suite into the native tool dependency graph.
  hostQtBase = buildPkgs.qt6Packages.qtbase.override {
    systemdSupport = false;
    qttranslations = null;
  };
  hostQtPackageSet = buildPkgs // {
    qt6 = buildPkgs.qt6 // {
      qtbase = hostQtBase;
    };
  };
  # KDE Frameworks invokes lrelease while building translations.  Expose just
  # that Linux executable: adding the complete qttools package to a target
  # build would run its host Qt setup hook beside the target Qt hook.
  hostQtToolsPackage = (buildPkgs.qt6Packages.qttools.override {
    qtbase = hostQtBase;
    qtdeclarative = null;
  }).overrideAttrs (_: {
    propagatedBuildInputs = [ hostQtBase ];
    # KDE only runs lrelease.  Avoid compiling Qt Assistant and Designer,
    # which are unrelated desktop applications and dominate this host build.
    cmakeFlags = [
      "-DFEATURE_assistant=OFF"
      "-DFEATURE_designer=OFF"
      "-DFEATURE_distancefieldgenerator=OFF"
      "-DFEATURE_pixeltool=OFF"
    ];
  });
  hostQtTools = buildPkgs.runCommand "qttools-host-lrelease" { } ''
    mkdir -p "$out/bin"
    ln -s ${hostQtToolsPackage}/bin/lrelease "$out/bin/lrelease"
  '';
  gmicQtTools = buildPkgs.runCommand "qttools-host-gmic" { } ''
    mkdir -p "$out/bin" "$out/lib/cmake"
    for tool in lcheck lconvert lrelease lrelease-pro ltext2id lupdate lupdate-pro; do
      ln -s ${hostQtToolsPackage}/bin/$tool "$out/bin/$tool"
    done
    cp -a ${hostQtToolsPackage}/lib/cmake/Qt6LinguistTools \
      "$out/lib/cmake/Qt6LinguistTools"
  '';
  # Qt translation catalogs are platform-independent.  Generate them with
  # the native Linguist tools, then expose only the data directory to the
  # Windows closure so no host Qt setup hook enters a target build.
  hostQtTranslationsPackage = buildPkgs.qt6Packages.qttranslations.override {
    qttools = hostQtToolsPackage;
  };
  qtTranslationsData = buildPkgs.runCommand "qttranslations-windows-data" { } ''
    mkdir -p "$out/share/qt-6"
    cp -a ${hostQtTranslationsPackage}/translations \
      "$out/share/qt-6/translations"
  '';
  hostPythonPackages = buildPkgs.python3Packages;
  hostPyQt6 = (hostPythonPackages.pyqt6.override {
    withLocation = false;
    withMultimedia = false;
    withPdf = false;
    withSerialPort = false;
    withSpeech = false;
    withWebSockets = false;
  }).overrideAttrs (old: {
    # FindPyQt6 only imports QtCore to obtain version and SIP metadata.  D-Bus
    # integration pulls target introspection machinery into a native Python
    # environment during cross evaluation and is unrelated to binding source
    # generation.
    dependencies = lib.filter (
      input: !(lib.hasInfix "dbus-python" (lib.toLower (lib.getName input)))
    ) (old.dependencies or [ ]);
    propagatedBuildInputs = lib.filter (
      input: !(lib.hasInfix "dbus-python" (lib.toLower (lib.getName input)))
    ) (old.propagatedBuildInputs or [ ]);
  });
  hostPython = buildPkgs.python3.withPackages (pythonPackages: [
    hostPyQt6
    pythonPackages.setuptools
    pythonPackages.sip
  ]);
  hostPythonPath = hostPythonPackages.makePythonPath [
    hostPyQt6
    hostPythonPackages.setuptools
    hostPythonPackages.sip
  ];
  # nixpkgs' MinGW support is maintained against the Fedora Windows patch
  # series for Python 3.11.  The default Python 3.14 package still applies
  # that 3.11 patch series verbatim, including files removed with distutils.
  # Use the supported target interpreter while retaining the current native
  # Python for generators; Krita only requires Python 3.8 or newer.
  targetPython = pkgs.python311;
  targetPythonPackages = pkgs.python311Packages;
  angle = import ./google-angle.nix { inherit pkgs; };

  # Python extension builders execute with the native interpreter, so
  # setuptools otherwise adds that interpreter's Unix headers even when the
  # extension itself is compiled for Windows.  Give PyQt's SIP extension the
  # target headers first; they carry the MinGW feature definitions and are
  # ABI-matched to the portable Python runtime shipped with Krita.
  useTargetPythonHeaders = input:
    if lib.hasInfix "pyqt6-sip" (lib.toLower (lib.getName input)) then
      input.overrideAttrs (old: {
        buildInputs = (old.buildInputs or [ ]) ++ [ pkgs.windows.pthreads ];
        NIX_CFLAGS_COMPILE = lib.concatStringsSep " " [
          (old.NIX_CFLAGS_COMPILE or "")
          "-I${targetPython}/include/python3.11"
        ];
        postPatch = (old.postPatch or "") + ''
          substituteInPlace setup.py \
            --replace-fail \
              "module = Extension('PyQt6.sip', module_src)" \
              "module = Extension('PyQt6.sip', module_src, include_dirs=['${targetPython}/include/python3.11', '${pkgs.windows.pthreads}/include'], library_dirs=['${targetPython}/lib/python3.11/config-3.11'], libraries=['python3.11'])"
        '';
      })
    else
      input;
  drmingw = import ./drmingw.nix { inherit pkgs; };
  qt = pkgs.qt6Packages.overrideScope (final: previous: {
    # qtbase runs host tools while cross compiling.  Supply the lean host
    # variant above instead of the default Linux desktop Qt build.
    qtbase = (previous.qtbase.override {
      pkgsBuildBuild = hostQtPackageSet;
      # MinGW links OpenGL through the Windows SDK's opengl32.dll.  nixpkgs'
      # generic Qt expression otherwise pulls in libglvnd, which in turn
      # requires a Unix X11 implementation that cannot be configured for
      # this target.
      libGL = null;
      vulkan-loader = null;
    }).overrideAttrs (old: {
      # Krita's official Windows builds use its Qt fork.  In addition to the
      # HDR texture colour-space API consumed by the small colour selector,
      # this carries the ANGLE, tablet, shortcut and backing-store fixes that
      # are part of Krita's Windows feature set.
      # The fork currently records 6.11.0 in its module metadata, while the
      # Qt modules supplied by this nixpkgs revision are 6.11.1 and require an
      # exact base version.  Keep the forked implementation and expose it as
      # the patch-level-compatible version consumed by those modules.
      version = "6.11.1";
      src = pkgs.fetchFromGitHub {
        owner = "dimula73";
        repo = "qtbase";
        rev = "3423b8a5a0b93a7ecb3b06db914d9fd72a68fe8e";
        hash = "sha256-5xNM6NvMAOTP5MsGVzQwzWAbJXrTk5CmYONJ/in9zoE=";
      };
      postPatch = (old.postPatch or "") + ''
        substituteInPlace .cmake.conf \
          --replace-fail 'set(QT_REPO_MODULE_VERSION "6.11.0")' \
                         'set(QT_REPO_MODULE_VERSION "6.11.1")'
        substituteInPlace src/plugins/platforms/windows/qwindowseglcontext.cpp \
          --replace-fail '<VersionHelpers.h>' '<versionhelpers.h>'
      '';
      propagatedBuildInputs = (old.propagatedBuildInputs or [ ]) ++ [ angle ];
      cmakeFlags = (old.cmakeFlags or [ ]) ++ [
        "-DQT_FEATURE_opengl=ON"
        "-DQT_FEATURE_egl=ON"
      ];
    });

    qmake = previous.qmake.override {
      qtbase = final.qtbase;
    };

    qtlanguageserver = previous.qtlanguageserver.override {
      qtbase = final.qtbase;
    };

    qtshadertools = previous.qtshadertools.override {
      qtbase = final.qtbase;
    };

    qtdeclarative = (previous.qtdeclarative.override {
      qtbase = final.qtbase;
      qtlanguageserver = final.qtlanguageserver;
      qtshadertools = final.qtshadertools;
      qtsvg = final.qtsvg;
    }).overrideAttrs (old: {
      # nixpkgs points this at .../Qt6ShaderTools, while the native qsb
      # package exports its executable from .../Qt6ShaderToolsTools.  Without
      # qsb Qt Declarative silently omits all Qt Quick modules.
      cmakeFlags = (old.cmakeFlags or [ ]) ++ [
        "-DQt6ShaderToolsTools_DIR=${buildPkgs.qt6Packages.qtshadertools}/lib/cmake/Qt6ShaderToolsTools"
        "-DQt6QuickTools_DIR=${buildPkgs.qt6Packages.qtdeclarative}/lib/cmake/Qt6QuickTools"
        "-DFEATURE_quick=ON"
      ];
    });

    # Core5Compat provides the Qt 5-era C++ APIs used by LibrePaint.  Preserve
    # its Qt Declarative dependency because Krita's storyboard, SVG text and
    # text-property interfaces are implemented with Qt Quick/QML.
    qt5compat = (previous.qt5compat.override {
      qtbase = final.qtbase;
      qtdeclarative = final.qtdeclarative;
    }).overrideAttrs (old: {
      propagatedBuildInputs = map (
        input:
        if lib.hasPrefix "qtbase" (lib.getName input) then final.qtbase else input
      ) (old.propagatedBuildInputs or [ ]);
    });

    qtsvg = (previous.qtsvg.override {
      qtbase = final.qtbase;
    }).overrideAttrs (old: {
      propagatedBuildInputs = map (
        input:
        if lib.hasPrefix "qtbase" (lib.getName input) then final.qtbase else input
      ) (old.propagatedBuildInputs or [ ]);
    });

    qttools = previous.qttools.override {
      qtbase = final.qtbase;
      qtdeclarative = final.qtdeclarative;
    };

  });
  # qmake validates its compiler while loading the Windows mkspec, before it
  # reads assignments from a generated project file or from its command line.
  # Supply CROSS_COMPILE from a dedicated mkspec so that even that initial
  # validation uses the Nix MinGW wrapper.
  targetQmakeConf = buildPkgs.writeText "qmake-mingw.conf" ''
    CROSS_COMPILE = ${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}
    include(${qt.qtbase}/mkspecs/win32-g++/qmake.conf)
    QT_TOOL.moc.binary = ${hostQtBase}/libexec/moc
    QT_TOOL.rcc.binary = ${hostQtBase}/libexec/rcc
    QT_TOOL.uic.binary = ${hostQtBase}/libexec/uic
  '';
  targetQmakeSpec = buildPkgs.runCommand "qmake-mingw-mkspec" { } ''
    mkdir -p "$out"
    ln -s ${targetQmakeConf} "$out/qmake.conf"
    ln -s ${qt.qtbase}/mkspecs/win32-g++/qplatformdefs.h "$out/qplatformdefs.h"
  '';
  targetQmake = buildPkgs.writeShellScript "qmake-mingw" ''
    export QMAKEPATH=${lib.getDev qt.qtbase}:${lib.getDev qt.qtdeclarative}:${lib.getDev qt.qtsvg}:${lib.getDev qt.qttools}

    if [ "$#" -gt 0 ] && [ "$1" = "-query" ]; then
      exec ${qt.qtbase}/bin/qmake "$@"
    fi

    exec ${qt.qtbase}/bin/qmake -spec ${targetQmakeSpec} \
      QMAKE_CC=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}gcc \
      QMAKE_CXX=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}g++ \
      QMAKE_LINK=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}g++ \
      QMAKE_LINK_C=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}gcc \
      QMAKE_LIB=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}ar \
      QMAKE_RC=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}windres \
      QMAKE_STRIP=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}strip \
      QMAKE_OBJCOPY=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}objcopy \
      QMAKE_NM=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}nm \
      QMAKE_MOC=${hostQtBase}/libexec/moc \
      QMAKE_RCC=${hostQtBase}/libexec/rcc \
      QMAKE_UIC=${hostQtBase}/libexec/uic \
      "$@"
  '';
  # `mkKdeDerivation` enables Python bindings for several frameworks by
  # default.  They are not used by LibrePaint, and disabling them keeps the
  # Windows dependency graph free of PySide and its unsupported Apple helper.
  kde = pkgs.kdePackages.overrideScope (_: previous:
    let
      inputName = input:
        if builtins.isAttrs input then
          lib.getName input
        else
          lib.getName (builtins.baseNameOf (toString input));
      tuneFramework =
        {
          package,
          removePythonBindings ? false,
        }:
        package.overrideAttrs (old: {
          # These are libraries or build-time CMake modules, not end-user Qt
          # applications.  The portable bundle performs its own deployment.
          dontWrapQtApps = true;
          # Qt Base's target prefix cannot contain the Linux lrelease tool.
          # Tell CMake where the corresponding host LinguistTools package is
          # before ECM enables Framework translation targets.
          cmakeFlags = (old.cmakeFlags or [ ]) ++ [
            "-DQt6LinguistTools_DIR=${hostQtToolsPackage}/lib/cmake/Qt6LinguistTools"
            # QtQml's target package deliberately contains Windows DLLs but
            # delegates its generators to the native Qt Declarative package.
            # KDE's QML macros resolve Qt6Qml through the target prefix, so
            # expose the matching host tools explicitly instead of making
            # CMake search for Linux executables below that Windows prefix.
            "-DQt6QmlTools_DIR=${buildPkgs.qt6Packages.qtdeclarative}/lib/cmake/Qt6QmlTools"
            # Qt Quick's target package has the same host-tool split.  In
            # particular KConfig's public QML types require the native
            # qmlcachegen helpers exported by Qt6QuickTools.
            "-DQt6QuickTools_DIR=${buildPkgs.qt6Packages.qtdeclarative}/lib/cmake/Qt6QuickTools"
          ];
          nativeBuildInputs = map (
            input:
            if lib.hasPrefix "qttools" (inputName input) then hostQtTools else input
          ) (lib.filter (
            input:
            let
              name = inputName input;
            in
            !lib.hasPrefix "qmllint-validate-hook" name
            && !lib.hasPrefix "wrap-qt6-apps-hook" name
            && (!removePythonBindings || (!lib.hasPrefix "python3" name && !lib.hasPrefix "shiboken6" name))
          ) (old.nativeBuildInputs or [ ]));
          buildInputs = map (
            input:
            if lib.hasPrefix "qtbase" (inputName input) then qt.qtbase
            else if lib.hasPrefix "qtdeclarative" (inputName input) then qt.qtdeclarative
            else if lib.hasPrefix "qtsvg" (inputName input) then qt.qtsvg
            else if lib.hasPrefix "qt5compat" (inputName input) then qt.qt5compat
            else input
          ) (lib.filter (
            input:
            let
              name = inputName input;
            in
            !lib.hasPrefix "qtwayland" name
            && !lib.hasPrefix "pyside6" name
            && !lib.hasPrefix "plasma-wayland-protocols" name
            && !lib.hasPrefix "wayland" name
          ) (old.buildInputs or [ ]));
          propagatedBuildInputs = map (
            input:
            if lib.hasPrefix "qtbase" (inputName input) then qt.qtbase
            else if lib.hasPrefix "qtdeclarative" (inputName input) then qt.qtdeclarative
            else if lib.hasPrefix "qtsvg" (inputName input) then qt.qtsvg
            else if lib.hasPrefix "qt5compat" (inputName input) then qt.qt5compat
            else input
          ) (lib.filter (
            input:
            let name = inputName input;
            in !lib.hasPrefix "qtwayland" name
              && !lib.hasPrefix "pyside6" name
              && !lib.hasPrefix "plasma-wayland-protocols" name
              && !lib.hasPrefix "wayland" name
          ) (old.propagatedBuildInputs or [ ]));
        });
    in
    {
    # Framework package values from the original scope have already captured
    # Qt inputs.  Replace their completed dependency lists individually, so
    # every build uses the target Qt libraries and the Linux lrelease shim.
    extra-cmake-modules = tuneFramework { package = previous.extra-cmake-modules; };
    kcodecs = tuneFramework { package = previous.kcodecs; };
    kcolorscheme = tuneFramework { package = previous.kcolorscheme; };
    kcompletion = tuneFramework { package = previous.kcompletion; };
    kconfig = (tuneFramework { package = previous.kconfig; }).overrideAttrs (old: {
      # KConfig's QML types are part of the public framework on Windows too.
      # Its generic cross expression only carries Qt Base, so add the target
      # Qt Quick package explicitly instead of allowing CMake to fall back to
      # the Qt Base prefix where the Quick configuration cannot exist.
      buildInputs = (old.buildInputs or [ ]) ++ [ qt.qtdeclarative ];
    });
    kcoreaddons = tuneFramework {
      package = previous.kcoreaddons;
      removePythonBindings = true;
    };
    kguiaddons = (tuneFramework {
      package = previous.kguiaddons;
      removePythonBindings = true;
    }).overrideAttrs (old: {
      # KGuiAddons exports a QML companion module when Qt QML is available.
      # Keep it in the Windows framework build just as in the desktop build.
      buildInputs = (old.buildInputs or [ ]) ++ [ qt.qtdeclarative ];
    });
    ki18n = (tuneFramework { package = previous.ki18n; }).overrideAttrs (old: {
      # The original cross expression receives Gettext as a Linux-side
      # translation tool.  KI18n also links libintl, so expose the Windows
      # library to CMake separately.
      buildInputs = (old.buildInputs or [ ]) ++ [ pkgs.gettext ];
    });
    kitemviews = tuneFramework { package = previous.kitemviews; };
    kwidgetsaddons = tuneFramework { package = previous.kwidgetsaddons; removePythonBindings = true; };
    libkdcraw = tuneFramework { package = previous.libkdcraw; };
    mlt = (previous.mlt.override {
      qtbase = qt.qtbase;
      qtsvg = qt.qtsvg;
      qt5compat = qt.qt5compat;
      wrapQtAppsHook = qt.wrapQtAppsHook;
    }).overrideAttrs (old: {
      # MLT's generic nixpkgs expression adds Unix display dependencies and
      # the optional OpenCV effects module even for a MinGW target.  Krita's
      # animation renderer uses the framework, Qt and FFmpeg modules.  Its
      # command wrapper is also a Unix shell artifact; the portable
      # application supplies MLT's paths directly at startup.
      buildInputs = lib.filter (
        input:
        let name = lib.toLower (inputName input);
        in !lib.hasPrefix "libx11" name
      && !(lib.hasInfix "opencv" name)
      && !(lib.hasInfix "frei0r" name)
      && !lib.hasPrefix "movit" name
      && !lib.hasPrefix "pango" name
      && !lib.hasPrefix "sox" name
      ) (old.buildInputs or [ ]) ++ [
        pkgs.windows.dlfcn
        pkgs.windows.pthreads
      ];
      nativeBuildInputs = [
        buildPkgs.cmake
        buildPkgs.pkg-config
        buildPkgs.which
      ];
      postPatch = (old.postPatch or "") + ''
        # MLT assumes iconv and winpthreads share one installation prefix and
        # derives the pthread import library by rewriting Iconv_LIBRARY.  Nix
        # keeps the two libraries in separate outputs, so name it directly.
        substituteInPlace CMakeLists.txt \
          --replace-fail 'string(REPLACE "iconv" "pthread" MLT_PTHREAD_LIBS "''${Iconv_LIBRARY}")' \
                         'set(MLT_PTHREAD_LIBS "${pkgs.windows.pthreads}/lib/libpthread.dll.a")'
        substituteInPlace src/modules/gdk/producer_pixbuf.c \
          --replace-fail '#include <BaseTsd.h>' '#include <basetsd.h>'
      '';
      cmakeFlags = (old.cmakeFlags or [ ]) ++ [
        "-DMOD_OPENCV=OFF"
        "-DMOD_FREI0R=OFF"
        "-DMOD_MOVIT=OFF"
        "-DMOD_SOX=OFF"
      ];
      dontWrapQtApps = true;
      preFixup = "";
    });
    poppler = (previous.poppler.override { qtbase = qt.qtbase; }).overrideAttrs (old: {
      # The portable bundle installs Poppler's CMap data beside its DLLs.
      # Resolve that data relative to the loaded module instead of retaining
      # the Nix store prefix used during the cross build.
      # Krita consumes Poppler through its Qt 6 frontend.  The Cairo and GLib
      # frontends are separate Unix-oriented APIs and pull X11 into a Windows
      # build without providing any Krita functionality.
      cmakeFlags = (old.cmakeFlags or [ ]) ++ [
        "-DENABLE_RELOCATABLE=ON"
        "-DENABLE_CAIRO=OFF"
        "-DENABLE_GLIB=OFF"
      ];
      propagatedBuildInputs = lib.filter
        (input: !(lib.hasPrefix "cairo" (lib.toLower (inputName input))))
        (old.propagatedBuildInputs or [ ]);
    });

    # ECM supplies CMake macros and is never executed as a Qt application.
    # The target Qt setup hook nevertheless asks it to select application
    # wrapping behavior, which is meaningless while cross compiling.
    quazip = previous.quazip.overrideAttrs (_: {
      # QuaZip needs Qt Core5Compat for its public CMake configuration.
      buildInputs = [ pkgs.bzip2 pkgs.zlib qt.qtbase ];
      propagatedBuildInputs = [ qt.qt5compat ];
      cmakeFlags = [ "-DQUAZIP_BZIP2=ON" ];
    });

  });

  # Build the target bindings against the exact Qt scope used by LibrePaint.
  # Krita's scripting interface needs the core, GUI, widgets, SVG, print and
  # QML bindings; unrelated browser, positioning and speech bindings would
  # introduce separate application stacks into the portable package.
  targetPyQt6 = (targetPythonPackages.pyqt6.override {
    qt6Packages = qt;
    withLocation = false;
    withMultimedia = false;
    withPdf = false;
    withSerialPort = false;
    withSpeech = false;
    withWebSockets = false;
  }).overrideAttrs (old: {
    # Four PyQt feature probes are linked as target executables and normally
    # run immediately.  A Linux builder cannot execute those PE binaries, so
    # feed their results from the exact Qt configuration selected above.  The
    # remaining probes are compile/link checks and continue to run normally.
    postPatch = (old.postPatch or "") + ''
      substituteInPlace project.py \
        --replace-fail \
          'from pyqtbuild import PyQtBindings, PyQtProject, QmakeTargetInstallable' \
          'from pyqtbuild import PyQtBindings as _PyQtBindings, PyQtProject, QmakeTargetInstallable

class PyQtBindings(_PyQtBindings):
    _target_test_output = {
        "QtCore": ["shared"],
        "QtGui": ["PyQt_OpenGL_ES2", "PyQt_XCB", "PyQt_Wayland"],
        "QtNetwork": [],
        "QtPrintSupport": [],
    }

    def is_buildable(self):
        if self.name in self._target_test_output:
            return self.handle_test_output(self._target_test_output[self.name])

        return super().is_buildable()'
      substituteInPlace project.py \
        --replace-fail \
          '        if self.license_dir is None:' \
          '        self.py_include_dir = "${targetPython}/include/python3.11"
        self.py_pylib_dir = "${targetPython}/lib/python3.11/config-3.11"
        self.py_pylib_lib = "python3.11"
        self.py_pylib_shlib = "${targetPython}/bin/libpython3.11.dll"
        self.link_full_dll = True

        if self.license_dir is None:'
    '';
    # SIP invokes qmake directly while probing and building each binding,
    # outside CMake's cross-toolchain setup.  Override qmake's tool commands
    # in every generated project so those probes and the final modules use
    # the MinGW wrappers instead of unprefixed Linux compiler names.
    pypaBuildFlags = (old.pypaBuildFlags or [ ]) ++ [
      "--config-setting=--qmake=${targetQmake}"
    ] ++ map
      (setting: "--config-setting=--qmake-setting=${setting}")
      [
        "QMAKE_CC=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}gcc"
        "QMAKE_CXX=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}g++"
        "QMAKE_LINK=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}g++"
        "QMAKE_LINK_C=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}gcc"
        "QMAKE_LIB=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}ar"
        "QMAKE_RC=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}windres"
        "QMAKE_STRIP=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}strip"
        "QMAKE_OBJCOPY=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}objcopy"
        "QMAKE_NM=${pkgs.stdenv.cc}/bin/${pkgs.stdenv.cc.targetPrefix}nm"
        "QMAKE_MOC=${hostQtBase}/libexec/moc"
        "QMAKE_RCC=${hostQtBase}/libexec/rcc"
        "QMAKE_UIC=${hostQtBase}/libexec/uic"
        "LIBS+=${targetPython}/lib/python3.11/config-3.11/libpython3.11.dll.a"
      ];
    # Krita's Python plug-ins use the Qt, SVG, print and QML bindings.  The
    # optional D-Bus binding is neither loaded nor shipped on Windows, and its
    # Unix daemon library cannot be executed while Meson configures a MinGW
    # cross build.
    dependencies = map useTargetPythonHeaders (lib.filter (
      input: !(lib.hasInfix "dbus-python" (lib.toLower (lib.getName input)))
    ) (old.dependencies or [ ]));
    propagatedBuildInputs = map useTargetPythonHeaders (lib.filter (
      input: !(lib.hasInfix "dbus-python" (lib.toLower (lib.getName input)))
    ) (old.propagatedBuildInputs or [ ]));
    buildInputs = (map useTargetPythonHeaders (lib.filter (
      input:
      let name = lib.toLower (lib.getName input);
      in !(lib.hasInfix "dbus" name)
        && !lib.hasPrefix "qtquick3d" name
        && !lib.hasPrefix "qtquicktimeline" name
        && !lib.hasPrefix "qtwebchannel" name
        && !lib.hasPrefix "qtwebsockets" name
    ) (old.buildInputs or [ ]))) ++ [ qt.qttools ];
    nativeBuildInputs = map useTargetPythonHeaders (lib.filter (
      input:
      let name = lib.toLower (lib.getName input);
      in !lib.hasPrefix "qtquick3d" name
        && !lib.hasPrefix "qtquicktimeline" name
        && !lib.hasPrefix "qtwebchannel" name
        && !lib.hasPrefix "qtwebsockets" name
    ) (old.nativeBuildInputs or [ ]));
  });
  targetKSeExpr = (pkgs.kseexpr.override {
    qt6 = qt;
    kdePackages = kde;
  }).overrideAttrs (old: {
    cmakeFlags = (old.cmakeFlags or [ ]) ++ [
      "-DQt6LinguistTools_DIR=${hostQtToolsPackage}/lib/cmake/Qt6LinguistTools"
    ];
    nativeBuildInputs = (old.nativeBuildInputs or [ ]) ++ [
      buildPkgs.bison
      buildPkgs.flex
      # Only lrelease executes on the build machine.  The complete host
      # QtTools package propagates host QtBase into the target Qt setup hook,
      # which correctly rejects the resulting mixed Qt dependency set.
      hostQtTools
    ];
    buildInputs = lib.filter (
      input:
      let name = lib.toLower (lib.getName input);
      in !lib.hasPrefix "bison" name
        && !lib.hasPrefix "flex" name
        && !lib.hasPrefix "qttools" name
    ) (old.buildInputs or [ ]);
  });

  # These tools execute while building, so they must be built for the Linux
  # build machine rather than for the Windows target.
  nativeBuildInputs = [
    buildPkgs.cmake
    buildPkgs.gettext
    buildPkgs.icoutils
    buildPkgs.ninja
    buildPkgs.pkg-config
    hostPython
    hostQtTools
  ];

  buildInputs = [
    pkgs.bzip2
    pkgs.boost
    pkgs.eigen_5
    pkgs.exiv2
    pkgs.ffmpeg
    pkgs.fftw
    pkgs.fontconfig
    pkgs.freetype
    pkgs.fribidi
    pkgs.glib
    pkgs.gsl
    pkgs.giflib
    pkgs.harfbuzz
    pkgs.immer
    targetKSeExpr
    pkgs.lager
    pkgs.libaom
    pkgs.libheif
    pkgs.libjpeg_turbo
    pkgs.libjxl
    pkgs.libmypaint
    pkgs.libpng
    pkgs.libraw
    pkgs.libunibreak
    pkgs.libwebp
    pkgs.lcms2
    pkgs.libtiff
    pkgs.opencolorio
    pkgs.openexr
    pkgs.openjpeg
    pkgs.windows.pthreads
    targetPython
    targetPyQt6
    pkgs.SDL2
    pkgs.xsimd
    pkgs.zug
    pkgs.zlib
    qt.qt5compat
    qt.qtbase
    qt.qtdeclarative
    qt.qtsvg
    qt.qttools
  ]
  ++ (with kde; [
    extra-cmake-modules
    kcolorscheme
    kcompletion
    kconfig
    kcoreaddons
    kguiaddons
    ki18n
    kitemviews
    kwidgetsaddons
    libkdcraw
    mlt
    poppler
    quazip
  ]);

  # The portable stage uses this set as explicit roots before collecting every
  # target DLL.  Include the MinGW runtime, which is not a CMake dependency.
  windowsRuntimeDependencies = buildInputs ++ [
    angle
    drmingw
    pkgs.poppler_data
    pkgs.stdenv.cc.cc
    qtTranslationsData
  ];
  sourcePreparer = buildPkgs.writeText "librepaint-windows-prepare-source" ''
    substituteInPlace plugins/impex/jp2/jp2_converter.cc \
      --replace-fail '<openjpeg.h>' '<${pkgs.openjpeg.incDir}/openjpeg.h>'

    # CMake compiles and links against the target Python while running the
    # binding generators with a native interpreter on the Linux build host.
    substituteInPlace CMakeLists.txt \
      --replace-fail \
        'find_package(Python 3.8 COMPONENTS Development Interpreter)' \
        'find_package(Python 3.8 COMPONENTS Development)
    set(Python_EXECUTABLE ${hostPython}/bin/python3)
    set(Python_Interpreter_FOUND TRUE)
    set(Python_FOUND TRUE)'
    substituteInPlace cmake/modules/FindPythonLibrary.cmake \
      --replace-fail \
        'find_package(Python 3.8 REQUIRED COMPONENTS Development Interpreter)' \
        'find_package(Python 3.8 REQUIRED COMPONENTS Development)
    set(Python_EXECUTABLE ${hostPython}/bin/python3)
    set(Python_Interpreter_FOUND TRUE)'
    substituteInPlace cmake/modules/FindSIP.cmake cmake/modules/FindPyQt6.cmake \
      --replace-fail 'if (WIN32)' 'if (WIN32 AND NOT CMAKE_CROSSCOMPILING)'
    substituteInPlace cmake/modules/FindSIP.cmake \
      --replace-fail 'PYTHONPATH=''${_pyqt5_python_path}' 'PYTHONPATH=${hostPythonPath}'
    substituteInPlace cmake/modules/FindPyQt6.cmake \
      --replace-fail 'PYTHONPATH=''${_pyqt6_python_path}' 'PYTHONPATH=${hostPythonPath}'
    substituteInPlace cmake/modules/SIPMacros.cmake \
      --replace-fail 'PYTHONPATH=''${_krita_python_path}' 'PYTHONPATH=${hostPythonPath}'
    substituteInPlace cmake/modules/SIPMacros.cmake \
      --replace-fail \
        'if (WIN32)
            set(_krita_python_path' \
        'if (WIN32 AND NOT CMAKE_CROSSCOMPILING)
            set(_krita_python_path'

    # Preserve Krita's component names across WebP's config-mode probe, then
    # validate the mapped demux and mux targets in FindWebP.
    substituteInPlace cmake/modules/FindWebP.cmake \
      --replace-fail \
        'find_package(WebP QUIET NO_MODULE
    HINTS ''${PC_WEBP_CONFIG_DIR} /usr/share/WebP/cmake /usr/local/share/WebP/cmake
)' \
        'set(_krita_webp_components "''${WebP_FIND_COMPONENTS}")
set(WebP_FIND_COMPONENTS)
find_package(WebP QUIET NO_MODULE
    HINTS ''${PC_WEBP_CONFIG_DIR} /usr/share/WebP/cmake /usr/local/share/WebP/cmake
)
set(WebP_FIND_COMPONENTS "''${_krita_webp_components}")'

    substituteInPlace cmake/modules/Findlibjpeg-turbo.cmake \
      --replace-fail \
        'find_package(libjpeg-turbo QUIET NO_MODULE)' \
        'set(_krita_jpeg_turbo_components "''${libjpeg-turbo_FIND_COMPONENTS}")
set(libjpeg-turbo_FIND_COMPONENTS)
find_package(libjpeg-turbo QUIET NO_MODULE)
set(libjpeg-turbo_FIND_COMPONENTS "''${_krita_jpeg_turbo_components}")'

    # Link animated WebP through an import archive generated from the complete
    # MinGW DLL export table during build preparation.
    substituteInPlace plugins/impex/webp/CMakeLists.txt \
      --replace-fail 'WebP::webp WebP::libwebpmux' \
                     'WebP::webp "$ENV{KRITA_WEBP_MUX_LIBRARY}"'
  '';
  buildPreparer = buildPkgs.writeText "librepaint-windows-prepare-build" ''
    unset _PYTHON_HOST_PLATFORM
    unset _PYTHON_SYSCONFIGDATA_NAME

    auxiliaryDir="''${LIBREPAINT_WINDOWS_AUXILIARY_DIR:-''${TMPDIR:?}}"
    mkdir -p "$auxiliaryDir"
    webpMuxDef="$auxiliaryDir/libwebpmux.def"
    {
      printf 'EXPORTS\n'
      ${pkgs.stdenv.cc.bintools.bintools}/bin/${pkgs.stdenv.cc.targetPrefix}objdump \
        -p ${pkgs.libwebp}/bin/libwebpmux.dll \
        | awk '/^.*\[[[:space:]]*[0-9]+\] [+]base\[[[:space:]]*[0-9]+\]/ && $NF != "RVA" { print $NF }'
    } > "$webpMuxDef"
    export KRITA_WEBP_MUX_LIBRARY="$auxiliaryDir/libwebpmux.dll.a"
    ${pkgs.stdenv.cc.bintools.bintools}/bin/${pkgs.stdenv.cc.targetPrefix}dlltool \
      --input-def "$webpMuxDef" \
      --dllname libwebpmux.dll \
      --output-lib "$KRITA_WEBP_MUX_LIBRARY"
  '';
in
pkgs.stdenv.mkDerivation {
  pname = "librepaint-windows-unwrapped";
  version = "1.0.2";
  src = source;

  strictDeps = true;
  dontWrapQtApps = true;
  cmakeGenerator = "Ninja";
  cmakeBuildType = "Release";

  inherit nativeBuildInputs buildInputs;

  postPatch = ''
    source ${sourcePreparer}
  '';

  cmakeFlags = [
    "-DALLOW_UNSTABLE=QT6"
    "-DBUILD_KRITA_QT_DESIGNER_PLUGINS=ON"
    "-DBUILD_TESTING=OFF"
    "-DBUILD_WITH_QT6=ON"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_FFTW3=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_GIF=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_GSL=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_HEIF=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_JPEGXL=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_JPEG=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_KDcrawQt6=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_KSeExpr=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_LibMyPaint=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_Mlt7=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_OpenColorIO=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_OpenEXR=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_OpenJPEG=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_Poppler=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_PyQt6=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_Python=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_PythonLibrary=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_QUAZIP=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_Qt6Quick=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_Qt6QuickControls2=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_Qt6QuickWidgets=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_SIP=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_TIFF=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_WebP=TRUE"
    "-DCMAKE_REQUIRE_FIND_PACKAGE_libjpeg-turbo=TRUE"
    "-DFOUNDATION_BUILD=ON"
    "-DKDE_INSTALL_ICONDIR=share/icons"
    "-DGETTEXT_MSGFMT_EXECUTABLE=${buildPkgs.gettext}/bin/msgfmt"
    "-DHAVE_HDR=ON"
    "-DQt6LinguistTools_DIR=${hostQtToolsPackage}/lib/cmake/Qt6LinguistTools"
    "-DQt6QmlTools_DIR=${buildPkgs.qt6Packages.qtdeclarative}/lib/cmake/Qt6QmlTools"
    "-DQt6QuickTools_DIR=${buildPkgs.qt6Packages.qtdeclarative}/lib/cmake/Qt6QuickTools"
    "-DPython_EXECUTABLE=${hostPython}/bin/python3"
    "-DPython_INCLUDE_DIR=${targetPython}/include/python3.11"
    "-DPython_LIBRARY=${targetPython}/lib/python3.11/config-3.11/libpython3.11.dll.a"
    "-DUSE_DRMINGW=ON"
  ];

  preConfigure = ''
    source ${buildPreparer}
  '';

  PYTHONPATH = hostPythonPath;

  postInstall = ''
    test -f "$out/bin/krita.exe"
    test -f "$out/bin/krita.dll"
    test -d "$out/lib/kritaplugins"

    mv "$out/bin/krita.exe" "$out/bin/LibrePaint.exe"
    if test -f "$out/bin/krita.com"; then
      mv "$out/bin/krita.com" "$out/bin/LibrePaint.com"
    fi
  '';

  enableParallelBuilding = true;

  passthru = {
    inherit
      buildPreparer
      hostPythonPath
      sourcePreparer
      windowsRuntimeDependencies
      ;
    windowsGmicKdePackages = {
      extra-cmake-modules = kde.extra-cmake-modules;
      kcoreaddons = kde.kcoreaddons;
    };
    windowsGmicQtPackages = {
      # G'MIC only invokes lrelease from qttools.  Use the tool-only wrapper
      # so its native stock-Qt setup hook is not mixed with the target Krita
      # Qt fork in the same cross-build environment.
      qttools = gmicQtTools;
      qtbase = qt.qtbase;
    };
    windowsPython = targetPython;
    windowsDrMingw = drmingw;
    windowsPythonPackages = [
      targetPyQt6
    ];
  };

  meta = {
    description = "LibrePaint digital painting application for 64-bit Windows";
    homepage = "https://github.com/serika12345/librepaint";
    license = lib.licenses.gpl3Only;
    mainProgram = "LibrePaint.exe";
    platforms = [ "x86_64-windows" ];
  };
}

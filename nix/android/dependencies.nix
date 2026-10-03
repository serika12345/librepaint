{
  androidAbi,
  androidNdkRoot,
  manifestFile,
  pkgs,
  qt5compat,
  qtbase,
}:

let
  inherit (pkgs) lib;
  manifest = lib.importJSON manifestFile;
  packageByName = builtins.listToAttrs (
    map (package: {
      name = package.name;
      value = package;
    }) manifest.packages
  );
  spec = name: packageByName.${name};

  mkCMakePackage = import ./mk-cmake-package.nix {
    inherit androidAbi androidNdkRoot pkgs;
  };
  mkAutotoolsPackage = import ./mk-autotools-package.nix {
    inherit androidAbi androidNdkRoot pkgs;
  };
  mkMesonPackage = import ./mk-meson-package.nix {
    inherit androidAbi androidNdkRoot pkgs;
  };

  cmakePackage =
    name: sourcePackage: dependencies: extra:
    let
      packageSpec = spec name;
    in
    assert lib.assertMsg (
      sourcePackage.version == packageSpec.version
    ) "${name} source version must match the pinned source manifest";
    mkCMakePackage (
      {
        pname = "${name}-android-${androidAbi}";
        inherit dependencies;
        inherit (packageSpec) version;
        src = sourcePackage.src;
        cmakeFlags = packageSpec.cmake_args or [ ];
        requiredPaths = (packageSpec.required_paths or [ ]) ++ (packageSpec.artifacts or [ ]);
        meta = {
          description = "${name} built from source for Android ${androidAbi}";
          inherit (sourcePackage.meta) license;
        };
      }
      // extra
    );

  autotoolsPackage =
    name: sourcePackage: dependencies: extra:
    let
      packageSpec = spec name;
    in
    assert lib.assertMsg (
      sourcePackage.version == packageSpec.version
    ) "${name} source version must match the pinned source manifest";
    mkAutotoolsPackage (
      {
        pname = "${name}-android-${androidAbi}";
        inherit dependencies;
        inherit (packageSpec) version;
        src = sourcePackage.src;
        configureFlags = packageSpec.configure_args or [ ];
        requiredPaths = (packageSpec.required_paths or [ ]) ++ (packageSpec.artifacts or [ ]);
        meta = {
          description = "${name} built from source for Android ${androidAbi}";
          inherit (sourcePackage.meta) license;
        };
      }
      // extra
    );

  zlib = cmakePackage "zlib" pkgs.zlib [ ] {
    postInstall = ''
      substituteInPlace "$out/lib/cmake/zlib/ZLIBConfig.cmake" \
        --replace-fail 'set(_ZLIB_supported_components "shared" "static")' \
        'set(_ZLIB_supported_components "static")'
      cat >> "$out/lib/cmake/zlib/ZLIBConfig.cmake" <<'EOF'
      if(TARGET ZLIB::ZLIBSTATIC AND NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB INTERFACE IMPORTED)
        set_property(TARGET ZLIB::ZLIB PROPERTY INTERFACE_LINK_LIBRARIES ZLIB::ZLIBSTATIC)
      endif()
      EOF
    '';
  };
  libdeflate = cmakePackage "libdeflate" pkgs.libdeflate [ ] { };
  expat = cmakePackage "expat" pkgs.expat [ ] { };
  libpng = cmakePackage "libpng" pkgs.libpng [ zlib ] { };
  libjpeg-turbo = cmakePackage "libjpeg-turbo" pkgs.libjpeg_turbo [ ] { };
  libwebp = cmakePackage "libwebp" pkgs.libwebp [ ] {
    postInstall = ''
      for pc in libwebp.pc libsharpyuv.pc; do
        substituteInPlace "$out/lib/pkgconfig/$pc" \
          --replace-fail \
            'Libs.private:   -pthread' \
            'Libs.private:   -pthread -lm'
      done
    '';
  };
  libtiff =
    cmakePackage "libtiff" pkgs.libtiff
      [
        zlib
        libjpeg-turbo
        libdeflate
      ]
      {
        postInstall = ''
          substituteInPlace "$out/lib/cmake/tiff/tiff-targets.cmake" \
            --replace-fail 'Deflate::Deflate' 'libdeflate::libdeflate_static' \
            --replace-fail 'JPEG::JPEG' 'libjpeg-turbo::jpeg-static'
              substituteInPlace "$out/lib/cmake/tiff/tiff-config.cmake" \
                --replace-fail \
                  'include("''${CMAKE_CURRENT_LIST_DIR}/tiff-targets.cmake")' \
              'include(CMakeFindDependencyMacro)
          find_dependency(libdeflate CONFIG)
          find_dependency(libjpeg-turbo CONFIG)
          include("''${CMAKE_CURRENT_LIST_DIR}/tiff-targets.cmake")'
        '';
      };
  openjpeg = cmakePackage "openjpeg" pkgs.openjpeg [ ] { };
  imath = cmakePackage "imath" pkgs.imath [ ] { };
  openexr =
    cmakePackage "openexr" pkgs.openexr
      [
        imath
        libdeflate
      ]
      {
        cmakeFlags = (spec "openexr").cmake_args ++ [ "-DBUILD_TESTING=OFF" ];
      };
  json-c = cmakePackage "json-c" pkgs.json_c [ ] { };
  lcms2 = cmakePackage "lcms2" pkgs.lcms2 [ ] { };
  exiv2 = cmakePackage "exiv2" pkgs.exiv2 [ zlib ] { };
  freetype = cmakePackage "freetype" pkgs.freetype [
    zlib
    libpng
  ] { };
  harfbuzz = cmakePackage "harfbuzz" pkgs.harfbuzz [ freetype ] {
    cmakeFlags = [
      "-DHB_HAVE_FREETYPE=ON"
      "-DHB_HAVE_GLIB=OFF"
      "-DHB_HAVE_ICU=OFF"
      "-DHB_HAVE_GRAPHITE2=OFF"
      "-DHB_BUILD_UTILS=OFF"
      "-DHB_BUILD_SUBSET=OFF"
      "-DHB_BUILD_RASTER=OFF"
      "-DHB_BUILD_VECTOR=OFF"
      "-DBUILD_FRAMEWORK=OFF"
    ];
  };
  brotli = cmakePackage "brotli" pkgs.brotli [ ] { };
  libhwy = cmakePackage "libhwy" pkgs.libhwy [ ] { };
  libjxl =
    cmakePackage "libjxl" pkgs.libjxl
      [
        brotli
        libhwy
        lcms2
      ]
      {
        # libjxl's FindHWY module promotes its imported target globally. Highway's
        # package config creates a directory-scoped target that is not visible to
        # libjxl's sibling directories.
        cmakeFlags = (spec "libjxl").cmake_args ++ [ "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=FALSE" ];
      };

  fftw = mkCMakePackage {
    pname = "fftw-android-${androidAbi}";
    inherit (pkgs.fftw) version src;
    cmakeFlags = [
      "-DBUILD_SHARED_LIBS=OFF"
      "-DBUILD_TESTS=OFF"
      "-DENABLE_THREADS=ON"
    ]
    ++ lib.optionals (androidAbi == "x86_64") [
      "-DENABLE_SSE=ON"
      "-DENABLE_SSE2=ON"
    ];
    requiredPaths = [
      "include/fftw3.h"
      "lib/libfftw3.a"
    ];
    meta = {
      description = "FFTW built from source for Android ${androidAbi}";
      inherit (pkgs.fftw.meta) license;
    };
  };
  gsl = mkAutotoolsPackage {
    pname = "gsl-android-${androidAbi}";
    inherit (pkgs.gsl) version src;
    configureFlags = [ "--disable-dependency-tracking" ];
    requiredPaths = [
      "include/gsl/gsl_math.h"
      "lib/libgsl.a"
      "lib/libgslcblas.a"
    ];
    meta = {
      description = "GNU Scientific Library built from source for Android ${androidAbi}";
      inherit (pkgs.gsl.meta) license;
    };
  };

  immer = cmakePackage "immer" pkgs.immer [ ] { };
  zug = cmakePackage "zug" pkgs.zug [ ] { };

  boostSpec = spec "boost";
  boost = pkgs.stdenvNoCC.mkDerivation {
    pname = "boost-android-${androidAbi}";
    inherit (boostSpec) version;
    src = pkgs.boost.src;
    dontConfigure = true;
    dontBuild = true;
    installPhase = ''
      runHook preInstall
      mkdir -p "$out/include" "$out/lib/cmake/Boost-${boostSpec.version}"
      cp -a boost "$out/include/boost"
      cat > "$out/lib/cmake/Boost-${boostSpec.version}/BoostConfig.cmake" <<'EOF'
      get_filename_component(_BOOST_PREFIX "''${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
      set(Boost_FOUND TRUE)
      set(Boost_VERSION "${boostSpec.version}")
      set(Boost_VERSION_STRING "${boostSpec.version}")
      if(NOT TARGET Boost::headers)
        add_library(Boost::headers INTERFACE IMPORTED)
        set_target_properties(Boost::headers PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "''${_BOOST_PREFIX}/include")
      endif()
      if(NOT TARGET Boost::boost)
        add_library(Boost::boost INTERFACE IMPORTED)
        set_target_properties(Boost::boost PROPERTIES INTERFACE_LINK_LIBRARIES Boost::headers)
      endif()
      if(NOT TARGET Boost::disable_autolinking)
        add_library(Boost::disable_autolinking INTERFACE IMPORTED)
        set_target_properties(Boost::disable_autolinking PROPERTIES INTERFACE_COMPILE_DEFINITIONS BOOST_ALL_NO_LIB)
      endif()
      EOF
      test -f "$out/include/boost/version.hpp"
      runHook postInstall
    '';
    meta = {
      description = "Boost headers prepared from source for Android ${androidAbi}";
      inherit (pkgs.boost.meta) license;
      platforms = [ "x86_64-linux" ];
      sourceProvenance = with lib.sourceTypes; [ fromSource ];
    };
    passthru = { inherit androidAbi; };
  };

  lager = cmakePackage "lager" pkgs.lager [
    boost
    zug
  ] { };
  eigen = cmakePackage "eigen" pkgs.eigen [ ] { };
  xsimd = cmakePackage "xsimd" pkgs.xsimd [ ] { };

  libffi = autotoolsPackage "libffi" pkgs.libffiReal [ ] {
    postInstall = ''
      rm -f "$out/lib/libffi.la"
    '';
  };
  pcre2 = autotoolsPackage "pcre2" pkgs.pcre2 [ ] {
    postInstall = ''
      rm -f "$out/lib/libpcre2-8.la"
    '';
  };
  fribidiSpec = spec "fribidi";
  fribidi = mkMesonPackage {
    pname = "fribidi-android-${androidAbi}";
    inherit (fribidiSpec) version;
    src = pkgs.fribidi.src;
    mesonFlags = fribidiSpec.meson_args;
    requiredPaths = fribidiSpec.required_paths ++ fribidiSpec.artifacts;
    meta = {
      description = "FriBidi built from source for Android ${androidAbi}";
      inherit (pkgs.fribidi.meta) license;
    };
  };
  glibSpec = spec "glib";
  proxyLibintlSource = pkgs.fetchurl {
    url = "https://github.com/frida/proxy-libintl/archive/refs/tags/0.5.tar.gz";
    hash = "sha256-96HL11ebqvV1xm+dmftilemwaEoosJWWfP2heFdZUwM=";
  };
  glib = mkMesonPackage {
    pname = "glib-android-${androidAbi}";
    inherit (glibSpec) version;
    src = pkgs.glib.src;
    dependencies = [
      zlib
      libffi
      pcre2
    ];
    mesonFlags = glibSpec.meson_args ++ [
      "-Dlibmount=disabled"
      "-Dselinux=disabled"
    ];
    nativeBuildInputs = [ pkgs.gettext ];
    # GLib requires dcgettext(), which Android's libintl-lite does not expose.
    # Materialize GLib's pinned proxy-libintl fallback from a fixed source
    # archive so Meson never downloads it during the sandboxed build.
    preConfigureCommands = ''
      mkdir -p subprojects/proxy-libintl-0.5
      tar --extract \
        --file ${proxyLibintlSource} \
        --strip-components=1 \
        --directory subprojects/proxy-libintl-0.5
    '';
    postInstall = ''
      mv "$out/lib/libintl.a" "$out/lib/libglibintl.a"
      rm "$out/include/libintl.h"
      for pc in \
        gio-2.0.pc \
        girepository-2.0.pc \
        glib-2.0.pc \
        gmodule-no-export-2.0.pc \
        gobject-2.0.pc \
        gthread-2.0.pc; do
        substituteInPlace "$out/lib/pkgconfig/$pc" \
          --replace-fail '-lintl' '-lglibintl'
      done
      rm -rf "$out/bin" "$out/share/bash-completion" "$out/share/man"
    '';
    requiredPaths = glibSpec.required_paths ++ glibSpec.artifacts;
    meta = {
      description = "GLib built from source for Android ${androidAbi}";
      inherit (pkgs.glib.meta) license;
    };
  };
  libmypaint =
    autotoolsPackage "libmypaint" pkgs.libmypaint
      [
        glib
        json-c
      ]
      {
        autoreconf = true;
        nativeBuildInputs = [
          pkgs.gettext
          pkgs.glib.dev
          pkgs.intltool
          pkgs.python3
        ];
        preConfigure = ''
          substituteInPlace configure.ac \
            --replace-fail '  IT_PROG_INTLTOOL' 'IT_PROG_INTLTOOL'
          python3 generate.py mypaint-brush-settings-gen.h brushsettings-gen.h
        '';
        postInstall = ''
          rm -f "$out/lib/libmypaint.la"
        '';
      };

  libunibreak = autotoolsPackage "libunibreak" pkgs.libunibreak [ ] {
    autoreconf = true;
    configureFlags = [ ];
  };
  fontconfig =
    autotoolsPackage "fontconfig" pkgs.fontconfig
      [
        expat
        freetype
        libpng
        zlib
      ]
      {
        buildTargets = (spec "fontconfig").make_targets;
        configureCache = (spec "fontconfig").configure_cache;
        configureFlags = [
          "--disable-docs"
          "--disable-docbook"
          "--disable-nls"
          "--disable-cache-build"
          "--disable-iconv"
          "--disable-libxml2"
          "--with-default-fonts=/system/fonts"
          "--with-add-fonts=/system/fonts"
        ];
        installTargets = (spec "fontconfig").make_install_targets;
        nativeBuildInputs = [
          pkgs.gperf
          pkgs.python3
        ];
      };

  libintlSpec = spec "libintl";
  libintl = mkCMakePackage {
    pname = "libintl-android-${androidAbi}";
    inherit (libintlSpec) version;
    src = pkgs.fetchurl {
      url = "https://github.com/j-jorge/libintl-lite/archive/ba1514607d02ce3711d828e784a7e9e2bb25aa84.tar.gz";
      hash = "sha256-bP6TtjJMp1XattlfcMYXw6VqVVYN6C0EHF3A2rIjDO0=";
    };
    postInstall = ''
      mkdir -p "$out/lib/pkgconfig"
      cat > "$out/lib/pkgconfig/intl.pc" <<EOF
      prefix=$out
      exec_prefix=$out
      libdir=$out/lib
      includedir=$out/include

      Name: intl
      Description: libintl-lite for Android
      Version: ${libintlSpec.version}
      Libs: -L$out/lib -lintl -lc++_shared
      Cflags: -I$out/include
      EOF
    '';
    requiredPaths = libintlSpec.required_paths ++ libintlSpec.artifacts;
    meta = {
      description = "libintl-lite built from source for Android ${androidAbi}";
      license = lib.licenses.boost;
    };
  };

  androidTriple =
    if androidAbi == "arm64-v8a" then "aarch64-linux-android" else "x86_64-linux-android";
  toolBin = "${androidNdkRoot}/toolchains/llvm/prebuilt/linux-x86_64/bin";
  giflibSpec = spec "giflib";
  giflib = pkgs.stdenv.mkDerivation {
    pname = "giflib-android-${androidAbi}";
    inherit (giflibSpec) version;
    src = pkgs.giflib.src;
    strictDeps = true;
    dontPatchELF = true;
    dontStrip = true;
    buildPhase = ''
      runHook preBuild
      make -j"$NIX_BUILD_CORES" libgif.a \
        CC=${toolBin}/${androidTriple}28-clang \
        AR=${toolBin}/llvm-ar \
        RANLIB=${toolBin}/llvm-ranlib
      runHook postBuild
    '';
    installPhase = ''
      runHook preInstall
      install -Dm644 gif_lib.h "$out/include/gif_lib.h"
      install -Dm644 libgif.a "$out/lib/libgif.a"
      runHook postInstall
    '';
    doInstallCheck = true;
    installCheckPhase = ''
      test -f "$out/include/gif_lib.h"
      test -s "$out/lib/libgif.a"
    '';
    passthru = { inherit androidAbi; };
    meta = {
      description = "giflib built from source for Android ${androidAbi}";
      inherit (pkgs.giflib.meta) license;
      platforms = [ "x86_64-linux" ];
      sourceProvenance = with lib.sourceTypes; [ fromSource ];
    };
  };

  quazipSpec = spec "quazip";
  quazip = mkCMakePackage {
    pname = "quazip-android-${androidAbi}";
    inherit (quazipSpec) version;
    src = pkgs.qt6Packages.quazip.src;
    cmakeToolchainFile = "${qtbase}/lib/cmake/Qt6/qt.toolchain.cmake";
    dependencies = [
      qtbase
      qt5compat
      zlib
    ];
    cmakeFlags = quazipSpec.cmake_args ++ [
      "-DBUILD_SHARED_LIBS=OFF"
      "-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH=${qt5compat}"
      "-DQT_HOST_PATH=${qtbase.hostQt}"
      "-DQUAZIP_INSTALL=ON"
      "-DQUAZIP_USE_QT_ZLIB=OFF"
      "-DZLIB_ROOT=${zlib}"
    ];
    requiredPaths = quazipSpec.required_paths ++ quazipSpec.artifacts;
    meta = {
      description = "QuaZip built from source with Qt 6 for Android ${androidAbi}";
      inherit (pkgs.qt6Packages.quazip.meta) license;
    };
  };
in
{
  inherit
    boost
    brotli
    eigen
    exiv2
    expat
    fontconfig
    fftw
    freetype
    fribidi
    giflib
    glib
    gsl
    harfbuzz
    imath
    immer
    json-c
    lager
    lcms2
    libdeflate
    libffi
    libhwy
    libintl
    libjpeg-turbo
    libjxl
    libmypaint
    libpng
    libtiff
    libunibreak
    libwebp
    openexr
    openjpeg
    pcre2
    quazip
    xsimd
    zlib
    zug
    ;
}

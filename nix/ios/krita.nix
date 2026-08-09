{
  diffutils,
  gettext,
  gnugrep,
  kcodecs-ios,
  kcolorscheme-ios,
  kcompletion-ios,
  kconfig-ios,
  kcoreaddons-ios,
  kfHostTooling,
  kguiaddons-ios,
  ki18n-ios,
  kitemviews-ios,
  kritaSource,
  pluginProfileFile,
  kwidgetsaddons-ios,
  lib,
  mkIOSCMakePackage,
  pkg-config,
  python3,
  qt5compat-ios,
  qtXcrunShim,
  qtbase-ios,
  qtsvg-ios,
  quazip-ios,
  toolchain,
  unzip,
  zlib-ios,
  expat-ios,
  libpng-ios,
  freetype-ios,
  harfbuzz-ios,
  fontconfig-ios,
  lcms2-ios,
  libdeflate-ios,
  eigen-ios,
  xsimd-ios,
  libunibreak-ios,
  libjpeg-turbo-ios,
  json-c-ios,
  libwebp-ios,
  libtiff-ios,
  libmypaint-ios,
  openjpeg-ios,
  imath-ios,
  openexr-ios,
  exiv2-ios,
  boost-ios,
  immer-ios,
  zug-ios,
  lager-ios,
  libintl-ios,
  fribidi-ios,
  giflib-ios,
  kseexpr-ios,
  libheif-ios,
  libjxl-ios,
  libkdcraw-ios,
  opencolorio-ios,
  poppler-ios,
}:

let
  librePaintVersion = "1.0.2";
  hostEcm = kfHostTooling.hostEcm;
  hostQt = kfHostTooling.hostQt;
  hostQtTools = kfHostTooling.hostQtTools;
  kf6HostTooling = kfHostTooling.kf6HostTooling;

  targetDependencies = [
    zlib-ios
    libdeflate-ios
    expat-ios
    libpng-ios
    freetype-ios
    harfbuzz-ios
    fontconfig-ios
    lcms2-ios
    eigen-ios
    xsimd-ios
    libunibreak-ios
    libjpeg-turbo-ios
    json-c-ios
    libwebp-ios
    libtiff-ios
    libmypaint-ios
    openjpeg-ios
    imath-ios
    openexr-ios
    exiv2-ios
    boost-ios
    immer-ios
    zug-ios
    lager-ios
    libintl-ios
    fribidi-ios
    giflib-ios
    libheif-ios
    libjxl-ios
    libkdcraw-ios
    opencolorio-ios
    poppler-ios
    kseexpr-ios
    qtbase-ios
    qtsvg-ios
    qt5compat-ios
    quazip-ios
    kconfig-ios
    kwidgetsaddons-ios
    kcodecs-ios
    kcompletion-ios
    kcoreaddons-ios
    kguiaddons-ios
    ki18n-ios
    kitemviews-ios
    kcolorscheme-ios
  ];

  pluginProfile = builtins.fromJSON (builtins.readFile pluginProfileFile);
in
assert lib.assertMsg (
  builtins.length targetDependencies == 46
  && builtins.length (lib.unique (map toString targetDependencies)) == 46
) "Krita iOS must consume the complete 46-package direct target dependency set";
assert lib.assertMsg (pluginProfile.schema == 1) "unsupported Krita iOS plugin profile schema";
assert lib.assertMsg (
  builtins.length pluginProfile.targets == 162
  && builtins.length (lib.unique pluginProfile.targets) == 162
) "Krita iOS initial plugin target inventory changed";
mkIOSCMakePackage {
  pname = "krita-ios-app";
  version = librePaintVersion;
  src = kritaSource;

  inherit targetDependencies;

  appleSdkResolver = qtXcrunShim;
  cmakeToolchainFile = "${qtbase-ios}/lib/cmake/Qt6/qt.toolchain.cmake";
  enableFullAppleToolchain = true;
  enableTargetPkgConfig = true;
  tryCompileTargetType = null;

  nativeBuildInputs = [
    gettext
    gnugrep
    pkg-config
    python3
  ];
  nativeInstallCheckInputs = [
    diffutils
    unzip
  ];

  buildTargets = [ "krita" ];
  installScripts = [
    "krita/data/cmake_install.cmake"
    "plugins/cmake_install.cmake"
  ];

  cmakeFlags = [
    "-DBUILD_TESTING:BOOL=OFF"
    "-DBUILD_WITH_QT6:BOOL=ON"
    "-DCMAKE_DISABLE_FIND_PACKAGE_GSL:BOOL=TRUE"
    "-DCMAKE_DISABLE_FIND_PACKAGE_FFTW3:BOOL=TRUE"
    "-DCMAKE_DISABLE_FIND_PACKAGE_Mlt7:BOOL=TRUE"
    "-DCMAKE_DISABLE_FIND_PACKAGE_Qt6Quick:BOOL=TRUE"
    "-DCMAKE_DISABLE_FIND_PACKAGE_Qt6QuickWidgets:BOOL=TRUE"
    "-DCMAKE_DISABLE_FIND_PACKAGE_Qt6WaylandClient:BOOL=TRUE"
    "-DCMAKE_FIND_PACKAGE_TARGETS_GLOBAL:BOOL=ON"
    "-DCMAKE_FIND_USE_PACKAGE_REGISTRY:BOOL=OFF"
    "-DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY:BOOL=OFF"
    "-DCMAKE_IGNORE_PREFIX_PATH:STRING=/opt/homebrew;/usr/local"
    "-DECM_DIR:PATH=${hostEcm}/share/ECM/cmake"
    "-DGETTEXT_MSGFMT_EXECUTABLE:FILEPATH=${gettext}/bin/msgfmt"
    "-DGETTEXT_MSGMERGE_EXECUTABLE:FILEPATH=${gettext}/bin/msgmerge"
    "-DKF6_HOST_TOOLING:PATH=${kf6HostTooling}"
    "-DKF_IGNORE_PLATFORM_CHECK:BOOL=ON"
    "-DKRITA_ENABLE_PCH:BOOL=OFF"
    "-DLIBREPAINT_VERSION_STRING:STRING=${librePaintVersion}"
    "-DKRITA_IOS_BUILD_QML_MODULES:BOOL=OFF"
    "-DKRITA_IOS_BUILD_PLUGINS:BOOL=ON"
    "-DKRITA_IOS_PLATFORM:STRING=DEVICE"
    "-DKRITA_IOS_PLUGIN_BASIC_TOOLS:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_CODEC_IMPEX:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_DEFAULT_PAINTOPS:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_MYPAINT:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_OPTIONAL_IMPEX:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_SEEXPR_GENERATOR:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_TOUCH_UI:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_JPEG:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_KRA:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_LAYER_DOCKER:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_LCMS_ENGINE:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_ORA:BOOL=ON"
    "-DKRITA_IOS_PLUGIN_PNG:BOOL=ON"
    "-DPython3_EXECUTABLE:FILEPATH=${python3}/bin/python3"
    "-DPython_EXECUTABLE:FILEPATH=${python3}/bin/python3"
    "-DQT_APPLE_SDK:STRING=iphoneos"
    "-DQT_HOST_PATH:PATH=${hostQt}"
    "-DQT_HOST_PATH_CMAKE_DIR:PATH=${hostQt}/lib/cmake"
    "-DQT_XCRUN:FILEPATH=${qtXcrunShim}/bin/xcrun"
    "-DQuaZip-Qt6_DIR:PATH=${quazip-ios}/lib/cmake/QuaZip-Qt6-1.5"
    "-DQt6Core5Compat_DIR:PATH=${qt5compat-ios}/lib/cmake/Qt6Core5Compat"
    "-DQt6LinguistTools_DIR:PATH=${hostQtTools}/lib/cmake/Qt6LinguistTools"
    "-DQt6Svg_DIR:PATH=${qtsvg-ios}/lib/cmake/Qt6Svg"
    "-DQt6SvgWidgets_DIR:PATH=${qtsvg-ios}/lib/cmake/Qt6SvgWidgets"
  ];

  requiredPaths = [
    "krita.app/1024-apps-krita.png"
    "krita.app/Info.plist"
    "krita.app/LaunchScreen.storyboard"
    "krita.app/krita"
    "krita.app/share/color/icc/krita"
    "krita.app/share/krita/actions/iostouchui.action"
    "krita.app/share/krita/actions/krita.action"
    "krita.app/share/krita/actions/kritamenu.action"
    "krita.app/share/krita/bundles/Krita_4_Default_Resources.bundle"
    "krita.app/share/doc/librepaint/non-code-licenses/CC-BY-3.0.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/CC-BY-SA-3.0.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/CC-BY-SA-4.0.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/CC0-1.0.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/GPL-2.0-or-later.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/GPL-3.0-only.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/GPL-3.0-or-later.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/LGPL-2.0-or-later.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/LGPL-3.0-only.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/LGPL-3.0-or-later.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/LicenseRef-ICC-License.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/default-resource-bundle-licenses.json"
    "krita.app/share/doc/librepaint/non-code-licenses/non-code-licenses.md"
    "krita.app/share/doc/librepaint/non-code-licenses/qtbase-icc-attribution.json"
    "krita.app/share/doc/librepaint/non-code-licenses/retained-functional-assets.md"
    "krita.app/share/doc/librepaint/non-code-licenses/static-dependency-resources.json"
    "krita.app/share/doc/librepaint/non-code-licenses/white-brand-assets.json"
    "krita.app/share/doc/librepaint/non-code-licenses/bundles/README"
    "krita.app/share/doc/librepaint/non-code-licenses/profiles/elles-icc-profiles/plain-text-README-for-elles-well-behaved-icc-profiles.txt"
    "krita.app/share/doc/librepaint/non-code-licenses/profiles/ycbcr-icc-profiles/LICENSE-PROFILES.txt"
  ];

  postConfigure = ''
    ${python3}/bin/python3 \
      "${kritaSource}/packaging/ios/scripts/replace-brand-art-with-white.py" \
      --manifest "${kritaSource}/packaging/ios/manifests/white-brand-assets.json" \
      --audit-ios-classification

    ${python3}/bin/python3 \
      "${kritaSource}/packaging/ios/scripts/audit-default-resource-bundle.py" \
      --bundle "${kritaSource}/krita/data/bundles/Krita_4_Default_Resources.bundle" \
      --manifest "${kritaSource}/packaging/ios/manifests/default-resource-bundle-licenses.json"

    ${python3}/bin/python3 \
      "${kritaSource}/packaging/ios/scripts/audit-static-dependency-resources.py" \
      --qtbase-source-tar "${qtbase-ios.src}" \
      --kcolorscheme-source-tar "${kcolorscheme-ios.src}" \
      --kwidgetsaddons-source-tar "${kwidgetsaddons-ios.src}"

    ${python3}/bin/python3 \
      "${kritaSource}/packaging/ios/scripts/audit-user-visible-branding.py" \
      --source-root "${kritaSource}" \
      --xgettext "${gettext}/bin/xgettext"

    check_cache_value() {
      name="$1"
      expected="$2"
      count="$(grep -Ec "^$name:[^=]*=" CMakeCache.txt || true)"
      if test "$count" -ne 1; then
        echo "error: expected one Krita cache entry for $name; found $count" >&2
        exit 1
      fi
      actual="$(sed -n "s/^$name:[^=]*=//p" CMakeCache.txt)"
      if test "$actual" != "$expected"; then
        echo "error: Krita cache $name is '$actual'; expected '$expected'" >&2
        exit 1
      fi
    }

    check_cache_value CMAKE_SYSTEM_NAME iOS
    check_cache_value CMAKE_OSX_ARCHITECTURES ${lib.escapeShellArg toolchain.architecture}
    check_cache_value CMAKE_OSX_DEPLOYMENT_TARGET ${lib.escapeShellArg toolchain.deploymentTarget}
    check_cache_value CMAKE_OSX_SYSROOT iphoneos
    check_cache_value BUILD_TESTING OFF
    check_cache_value BUILD_WITH_QT6 ON
    check_cache_value LIBREPAINT_VERSION_STRING ${lib.escapeShellArg librePaintVersion}
    check_cache_value KRITA_IOS_BUILD_PLUGINS ON
    check_cache_value KRITA_IOS_BUILD_QML_MODULES OFF
    check_cache_value KF6_HOST_TOOLING ${lib.escapeShellArg (toString kf6HostTooling)}
    check_cache_value Qt6LinguistTools_DIR \
      ${lib.escapeShellArg "${hostQtTools}/lib/cmake/Qt6LinguistTools"}

    # KoConfig.h normally embeds CMAKE_BINARY_DIR for a developer-only
    # launch guard. Keep the guard while making the shipped binary independent
    # of Nix's per-build temporary directory.
    grep -Fx "#define KRITA_BUILD_DIR \"$PWD\"" KoConfig.h
    substituteInPlace KoConfig.h \
      --replace-fail "#define KRITA_BUILD_DIR \"$PWD\"" \
      '#define KRITA_BUILD_DIR "/build"'
  '';

  postInstall = ''
    if ! test -d bin/krita.app; then
      echo "error: Krita build did not produce bin/krita.app" >&2
      exit 1
    fi

    install -Dm644 \
      "${kritaSource}/krita/krita.action" \
      "$out/share/krita/actions/krita.action"
    install -Dm644 \
      "${kritaSource}/krita/kritamenu.action" \
      "$out/share/krita/actions/kritamenu.action"

    # CMake installs desktop service metadata for host integrations that do not
    # exist on iPadOS. Prune only that exact generated directory before the
    # runtime share tree is copied into the application bundle.
    rm -rf -- "$out/share/applications"

    cp -R bin/krita.app "$out/krita.app"
    cp -R "$out/share" "$out/krita.app/share"
    rm -rf "$out/share"
  '';

  postInstallCheck = ''
        app="$out/krita.app"
        binary="$app/krita"

        ${python3}/bin/python3 \
          "${kritaSource}/packaging/ios/scripts/audit-static-dependency-resources.py" \
          --binary "$binary" \
          --build-ninja "$PWD/build.ninja" \
          --nm "${toolchain.nm}"

        architectures="$(${toolchain.lipo} -archs "$binary")"
        if test "$architectures" != "${toolchain.architecture}"; then
          echo "error: Krita contains '$architectures'; expected ${toolchain.architecture}" >&2
          exit 1
        fi
        build_metadata="$(${toolchain.vtool} -show-build "$binary")"
        grep -Eq 'platform[[:space:]]+IOS([[:space:]]|$)' <<<"$build_metadata"
        grep -Eq 'minos[[:space:]]+${toolchain.deploymentTarget}([[:space:]]|$)' <<<"$build_metadata"
        grep -Eq 'sdk[[:space:]]+${toolchain.sdkVersion}([[:space:]]|$)' <<<"$build_metadata"

        binary_symbols="$(${toolchain.nm} "$binary")"
        for qt_svg_plugin in QSvgIconPlugin QSvgPlugin; do
          if ! grep -Fq "qt_static_plugin_$qt_svg_plugin" <<<"$binary_symbols"; then
            echo "error: Krita omits the static Qt SVG plugin $qt_svg_plugin" >&2
            exit 1
          fi
        done
        if ! grep -Fq "qt_static_plugin_kritaiostouchui_factory" <<<"$binary_symbols"; then
          echo "error: Krita omits the static iPadOS touch UI plugin" >&2
          exit 1
        fi

        ${python3}/bin/python3 - "$app/Info.plist" <<'PY'
    import plistlib
    import sys

    with open(sys.argv[1], "rb") as handle:
        info = plistlib.load(handle)

    expected = {
        "CFBundleExecutable": "krita",
        "CFBundleIdentifier": "local.librepaint.ipad",
        "CFBundleDisplayName": "LibrePaint",
        "CFBundleShortVersionString": "${librePaintVersion}",
        "CFBundleVersion": "${librePaintVersion}",
        "LSRequiresIPhoneOS": True,
        "LSSupportsOpeningDocumentsInPlace": True,
        "MinimumOSVersion": "${toolchain.deploymentTarget}",
        "UIDeviceFamily": [2],
        "UIFileSharingEnabled": True,
    }
    for key, value in expected.items():
        if info.get(key) != value:
            raise SystemExit(f"{key} is {info.get(key)!r}; expected {value!r}")
    PY

        if test -e "$app/_CodeSignature" || test -e "$app/embedded.mobileprovision"; then
          echo "error: immutable Krita app output must remain unsigned" >&2
          exit 1
        fi
        if find "$app" -type l -print -quit | grep -q .; then
          echo "error: Krita app contains a symlink" >&2
          exit 1
        fi
        if test -e "$app/share/applications" || test -L "$app/share/applications"; then
          echo "error: desktop application metadata was packaged for iPadOS" >&2
          exit 1
        fi

        bundle_count="$(find "$app/share/krita/bundles" -type f -name '*.bundle' | wc -l | tr -d ' ')"
        profile_count="$(find "$app/share/color/icc/krita" -type f \( -name '*.icc' -o -name '*.icm' \) | wc -l | tr -d ' ')"
        action_count="$(find "$app/share/krita/actions" -type f -name '*.action' | wc -l | tr -d ' ')"
        test "$bundle_count" -eq 1
        test "$profile_count" -eq 31
        test "$action_count" -gt 0
        test -s "$app/share/krita/actions/iostouchui.action"
        test -s "$app/share/krita/bundles/Krita_4_Default_Resources.bundle"
        ${python3}/bin/python3 \
          "${kritaSource}/packaging/ios/scripts/audit-default-resource-bundle.py" \
          --bundle "$app/share/krita/bundles/Krita_4_Default_Resources.bundle" \
          --manifest "${kritaSource}/packaging/ios/manifests/default-resource-bundle-licenses.json"
        for notice in \
          CC-BY-3.0.txt \
          CC-BY-SA-3.0.txt \
          CC-BY-SA-4.0.txt \
          CC0-1.0.txt \
          GPL-2.0-or-later.txt \
          GPL-3.0-only.txt \
          GPL-3.0-or-later.txt \
          LGPL-2.0-or-later.txt \
          LGPL-3.0-only.txt \
          LGPL-3.0-or-later.txt \
          LicenseRef-ICC-License.txt \
          default-resource-bundle-licenses.json \
          non-code-licenses.md \
          qtbase-icc-attribution.json \
          retained-functional-assets.md \
          static-dependency-resources.json \
          white-brand-assets.json; do
          test -s "$app/share/doc/librepaint/non-code-licenses/$notice"
        done
        test -s "$app/share/doc/librepaint/non-code-licenses/bundles/README"
        test -s "$app/share/doc/librepaint/non-code-licenses/profiles/elles-icc-profiles/plain-text-README-for-elles-well-behaved-icc-profiles.txt"
        test -s "$app/share/doc/librepaint/non-code-licenses/profiles/ycbcr-icc-profiles/LICENSE-PROFILES.txt"
        test ! -e "$app/share/color/icc/krita/scRGB.icm"
        test ! -e "$app/share/color/icc/krita/cmyk.icm"
        test ! -e "$app/share/color/icc/krita/krita25_lcms-builtin-sRGB_g100-truegamma.icc"
        grep -q '<Action name="copy_merged">' "$app/share/krita/actions/kritamenu.action"
        grep -q '<Action name="view_show_ios_touch_ui">' "$app/share/krita/actions/iostouchui.action"

        preset_bundle_count=0
        while IFS= read -r bundle; do
          if ${unzip}/bin/unzip -Z1 "$bundle" | grep '^paintoppresets/' >/dev/null; then
            preset_bundle_count=$((preset_bundle_count + 1))
          fi
        done < <(find "$app/share/krita/bundles" -type f -name '*.bundle' | sort)
        if test "$preset_bundle_count" -eq 0; then
          echo "error: no resource bundle contains brush presets" >&2
          exit 1
        fi
  '';

  passthru = {
    bundleIdentifier = "local.librepaint.ipad";
    inherit pluginProfile targetDependencies;
    unsigned = true;
  };

  meta = {
    description = "Unsigned LibrePaint application bundle for arm64 iPadOS";
    license = lib.licenses.gpl3Plus;
  };
}

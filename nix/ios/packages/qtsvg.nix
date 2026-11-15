{
  gnugrep,
  lib,
  mkIOSCMakePackage,
  ninja,
  packageSpec,
  qt6Packages,
  qtbase-ios,
  qtXcrunShim,
  toolchain,
  zlib-ios,
}:

let
  qtsvg = qt6Packages.qtsvg;
  hostQt = qt6Packages.qtbase;

  staticArchives = packageSpec.artifacts.static_archives;
  standaloneObjects = packageSpec.artifacts.standalone_objects;
  requiredPaths = packageSpec.artifacts.required_paths ++ staticArchives ++ standaloneObjects;

  cacheStringLocks = packageSpec.configure.cache_string_locks;
  cacheBooleanLocks = packageSpec.configure.cache_boolean_locks;
  commonCacheStringLocks = {
    CMAKE_BUILD_TYPE = "Release";
    CMAKE_OSX_ARCHITECTURES = toolchain.architecture;
    CMAKE_OSX_DEPLOYMENT_TARGET = toolchain.deploymentTarget;
    CMAKE_OSX_SYSROOT = "iphoneos";
    CMAKE_SYSTEM_NAME = "iOS";
  };
  qtCacheStringLocks = removeAttrs cacheStringLocks (builtins.attrNames commonCacheStringLocks);

  expectedCacheBooleanLocks = {
    BUILD_SHARED_LIBS = false;
    CMAKE_FIND_PACKAGE_TARGETS_GLOBAL = true;
    QT_BUILD_DOCS = false;
    QT_BUILD_EXAMPLES = false;
    QT_BUILD_TESTS = false;
    QT_BUILD_TOOLS_BY_DEFAULT = false;
    QT_GENERATE_SBOM = false;
    QT_WILL_BUILD_TOOLS = false;
  };

  cmakeBoolean = value: if value then "ON" else "OFF";
  cacheBooleanFlags = lib.mapAttrsToList (name: value: "-D${name}=${cmakeBoolean value}") (
    removeAttrs cacheBooleanLocks [ "QT_WILL_BUILD_TOOLS" ]
  );
  qtCacheStringFlags = lib.mapAttrsToList (name: value: "-D${name}=${value}") qtCacheStringLocks;
  inputFlags = lib.mapAttrsToList (
    name: value: "-D${name}=${value}"
  ) packageSpec.configure.input_locks;
  enabledFeatureFlags = map (
    feature: "-DFEATURE_${feature}=ON"
  ) packageSpec.configure.feature_locks.enabled;
  disabledFeatureFlags = map (
    feature: "-DFEATURE_${feature}=OFF"
  ) packageSpec.configure.feature_locks.disabled;

  cacheStringCheckScript = lib.concatStrings (
    lib.mapAttrsToList (name: value: ''
      check_cache_string ${lib.escapeShellArg name} ${lib.escapeShellArg value}
    '') cacheStringLocks
  );
  cacheBooleanCheckScript = lib.concatStrings (
    lib.mapAttrsToList (name: value: ''
      check_cache_boolean ${lib.escapeShellArg name} ${lib.escapeShellArg (cmakeBoolean value)}
    '') cacheBooleanLocks
  );
  inputCheckScript = lib.concatStrings (
    lib.mapAttrsToList (name: value: ''
      check_cache_string ${lib.escapeShellArg name} ${lib.escapeShellArg value}
    '') packageSpec.configure.input_locks
  );
  enabledFeatureCheckScript = lib.concatMapStrings (feature: ''
    check_cache_boolean ${lib.escapeShellArg "FEATURE_${feature}"} ON
  '') packageSpec.configure.feature_locks.enabled;
  disabledFeatureCheckScript = lib.concatMapStrings (feature: ''
    check_cache_boolean ${lib.escapeShellArg "FEATURE_${feature}"} OFF
  '') packageSpec.configure.feature_locks.disabled;

  # A Qt add-on installed into its own Nix output cannot leave qmake's
  # QT_INSTALL_LIBS placeholder on archive dependencies. qmake resolves that
  # placeholder to the base Qt prefix, while Svg lives in this package and the
  # remaining archives live in qtbase-ios.
  prlDependencies = {
    "lib/libQt6Svg.prl" = {
      own = [ ];
      qtbase = [
        "libQt6Core.a"
        "libQt6Gui.a"
        "libQt6BundledPcre2.a"
      ];
    };
    "lib/libQt6SvgWidgets.prl" = {
      own = [ "libQt6Svg.a" ];
      qtbase = [
        "libQt6Core.a"
        "libQt6Gui.a"
        "libQt6Widgets.a"
        "libQt6BundledPcre2.a"
      ];
    };
    "plugins/iconengines/libqsvgicon.prl" = {
      own = [ "libQt6Svg.a" ];
      qtbase = [
        "libQt6Core.a"
        "libQt6Gui.a"
        "libQt6BundledPcre2.a"
      ];
    };
    "plugins/imageformats/libqsvg.prl" = {
      own = [ "libQt6Svg.a" ];
      qtbase = [
        "libQt6Core.a"
        "libQt6Gui.a"
        "libQt6BundledPcre2.a"
      ];
    };
  };

  rewritePrlDependency = owner: archive: ''
    needle=${lib.escapeShellArg "$$[QT_INSTALL_LIBS]/${archive}"}
    count="$(grep -Fo -- "$needle" "$prl" | wc -l | tr -d ' ' || true)"
    if test "$count" -ne 2; then
      echo "error: $relative_prl contains $count occurrences of $needle; expected 2" >&2
      exit 1
    fi
    ${
      if owner == "own" then
        ''replacement="$out/lib/${archive}"''
      else
        "replacement=${lib.escapeShellArg "${qtbase-ios}/lib/${archive}"}"
    }
    substituteInPlace "$prl" --replace-fail "$needle" "$replacement"
  '';
  prlRewriteScript = lib.concatStrings (
    lib.mapAttrsToList (relativePrl: dependencies: ''
      relative_prl=${lib.escapeShellArg relativePrl}
      prl="$out/$relative_prl"
      if ! test -f "$prl" || test -L "$prl"; then
        echo "error: qtsvg PRL is not a regular non-symlink file: $relative_prl" >&2
        exit 1
      fi
      ${lib.concatMapStrings (rewritePrlDependency "qtbase") dependencies.qtbase}
      ${lib.concatMapStrings (rewritePrlDependency "own") dependencies.own}
    '') prlDependencies
  );

  checkRewrittenPrlDependency = owner: archive: ''
    ${
      if owner == "own" then
        ''expected_dependency="$out/lib/${archive}"''
      else
        "expected_dependency=${lib.escapeShellArg "${qtbase-ios}/lib/${archive}"}"
    }
    count="$(grep -Fo -- "$expected_dependency" "$prl" | wc -l | tr -d ' ' || true)"
    if test "$count" -ne 2; then
      echo "error: $relative_prl contains $count references to $expected_dependency; expected 2" >&2
      exit 1
    fi
  '';
  prlCheckScript = lib.concatStrings (
    lib.mapAttrsToList (relativePrl: dependencies: ''
      relative_prl=${lib.escapeShellArg relativePrl}
      prl="$out/$relative_prl"
      grep -Fxq 'QMAKE_PRL_CONFIG = static' "$prl"
      grep -Fxq 'QMAKE_PRL_VERSION = ${packageSpec.version}' "$prl"
      ${lib.concatMapStrings (checkRewrittenPrlDependency "qtbase") dependencies.qtbase}
      ${lib.concatMapStrings (checkRewrittenPrlDependency "own") dependencies.own}
    '') prlDependencies
  );

  modulePriPaths = [
    "mkspecs/modules/qt_lib_svg.pri"
    "mkspecs/modules/qt_lib_svg_private.pri"
    "mkspecs/modules/qt_lib_svgwidgets.pri"
  ];
  pluginPriPaths = [
    "mkspecs/modules/qt_plugin_qsvg.pri"
    "mkspecs/modules/qt_plugin_qsvgicon.pri"
  ];

  dependencyEntry = dependency: {
    key = dependency.outPath;
    value = dependency;
  };
  allowedTargetDependencyClosure = map (entry: entry.value) (
    builtins.genericClosure {
      startSet = map dependencyEntry [
        qtbase-ios
        zlib-ios
      ];
      operator = entry: map dependencyEntry (entry.value.propagatedBuildInputs or [ ]);
    }
  );
  allowedTargetStorePaths = map toString allowedTargetDependencyClosure;
in
assert lib.assertMsg (packageSpec.name == "qtsvg") "Qt package manifest entry must be qtsvg";
assert lib.assertMsg (packageSpec.version == qtsvg.version)
  "Qt SVG target source version ${qtsvg.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.version == hostQt.version
) "Qt SVG host version ${hostQt.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.source.flake_attr == "source-qtsvg"
) "Qt SVG source manifest must use the source-qtsvg flake output";
assert lib.assertMsg (
  packageSpec.source.archive_name == "qtsvg-everywhere-src-${packageSpec.version}.tar.xz"
) "Qt SVG source archive name does not match its pinned version";
assert lib.assertMsg (
  packageSpec.source.archive_sha256
  == "e4ab39534ec97987b1b9b60ef7f4d3253d5912a69f8c713a01753174a4029331"
) "Qt SVG source archive hash differs from the audited Qt 6.12.0 archive";
assert lib.assertMsg (
  packageSpec.dependencies.target_packages == [ "zlib" ]
) "Qt SVG target package dependencies must be exactly [ zlib ]";
assert lib.assertMsg (
  packageSpec.dependencies.qt_modules == [ "qtbase" ]
) "Qt SVG target Qt module dependencies must be exactly [ qtbase ]";
assert lib.assertMsg (
  packageSpec.host_tools.flake_attr == "host-qtbase"
) "Qt SVG host-tool manifest must use the host-qtbase flake output";
assert lib.assertMsg (
  packageSpec.host_tools.version == hostQt.version
) "Qt SVG host-tool manifest version must match the host Qt derivation";
assert lib.assertMsg (
  packageSpec.host_tools.cmake_packages == [
    "Qt6HostInfo"
    "Qt6CoreTools"
    "Qt6GuiTools"
    "Qt6WidgetsTools"
  ]
) "Qt SVG host CMake package contract changed";
assert lib.assertMsg (
  packageSpec.host_tools.executables == [
    "libexec/syncqt"
    "libexec/moc"
    "libexec/cmake_automoc_parser"
  ]
) "Qt SVG host executable contract changed";
assert lib.assertMsg (
  packageSpec.configure.sdk_lock == {
    allow_absolute_path = false;
    cmake_cache_value = "iphoneos";
    name = "iphoneos";
  }
) "Qt SVG must preserve a logical iphoneos SDK in installed configuration";
assert lib.assertMsg (
  packageSpec.configure.languages == [
    "C"
    "CXX"
    "OBJC"
    "OBJCXX"
  ]
) "Qt SVG language manifest must match its audited C/C++/Objective-C build";
assert lib.assertMsg (
  lib.getAttrs (builtins.attrNames commonCacheStringLocks) cacheStringLocks == commonCacheStringLocks
) "Qt SVG common CMake cache locks disagree with the pinned iOS toolchain";
assert lib.assertMsg (
  qtCacheStringLocks == { QT_QMAKE_TARGET_MKSPEC = "macx-ios-clang"; }
) "Qt SVG has an unsupported project-specific string cache lock";
assert lib.assertMsg (
  cacheBooleanLocks == expectedCacheBooleanLocks
) "Qt SVG boolean cache locks differ from the audited library-only configuration";
assert lib.assertMsg (
  packageSpec.configure.input_locks == { }
) "Qt SVG unexpectedly declares an input-selection lock";
assert lib.assertMsg (
  packageSpec.configure.feature_locks == {
    disabled = [ "pkg_config" ];
    enabled = [ ];
  }
) "Qt SVG feature locks differ from the audited configuration";
assert lib.assertMsg (
  packageSpec.artifacts.expected_counts == {
    standalone_objects = 2;
    static_archives = 4;
  }
) "Qt SVG artifact counts differ from the audited static output";
assert lib.assertMsg (
  packageSpec.artifacts.exact_static_archive_set && packageSpec.artifacts.exact_standalone_object_set
) "Qt SVG output sets must both be exact";
assert lib.assertMsg (
  builtins.length staticArchives == packageSpec.artifacts.expected_counts.static_archives
) "Qt SVG static archive count does not match its manifest";
assert lib.assertMsg (
  builtins.length standaloneObjects == packageSpec.artifacts.expected_counts.standalone_objects
) "Qt SVG standalone object count does not match its manifest";
assert lib.assertMsg (
  staticArchives == [
    "lib/libQt6Svg.a"
    "lib/libQt6SvgWidgets.a"
    "plugins/iconengines/libqsvgicon.a"
    "plugins/imageformats/libqsvg.a"
  ]
  &&
    standaloneObjects == [
      "plugins/iconengines/objects-Release/QSvgIconPlugin_init/QSvgIconPlugin_init.cpp.o"
      "plugins/imageformats/objects-Release/QSvgPlugin_init/QSvgPlugin_init.cpp.o"
    ]
) "Qt SVG Apple object manifest differs from the audited output set";
assert lib.assertMsg (
  lib.unique staticArchives == staticArchives && lib.unique standaloneObjects == standaloneObjects
) "Qt SVG output manifest contains duplicate artifacts";
assert lib.assertMsg (
  packageSpec.artifacts.forbidden.path_globs == [
    "**/*.app"
    "**/*.dylib"
    "**/*.framework"
    "**/*.so"
    "**/*.so.*"
    "doc/**"
    "examples/**"
    "sbom/**"
    "tests/**"
  ]
) "Qt SVG forbidden output-glob contract changed";
assert lib.assertMsg (
  packageSpec.artifacts.forbidden.reference_literals == [ toolchain.xcodeApp ]
  && packageSpec.artifacts.forbidden.reference_environment_values == [ "NIX_BUILD_TOP" ]
) "Qt SVG forbidden reference contract changed";
mkIOSCMakePackage {
  pname = "qtsvg-ios";
  inherit (packageSpec) version;
  src = qtsvg.src;

  targetDependencies = [
    qtbase-ios
    zlib-ios
  ];

  appleSdkResolver = qtXcrunShim;
  cmakeToolchainFile = "${qtbase-ios}/lib/cmake/Qt6/qt.toolchain.cmake";
  enableFullAppleToolchain = true;
  inspectAllAppleObjects = true;
  tryCompileTargetType = null;
  nativeBuildInputs = [ gnugrep ];

  cmakeFlags = [
    "-DCMAKE_FIND_PACKAGE_PREFER_CONFIG=TRUE"
    "-DQT_APPLE_SDK=iphoneos"
    "-DQT_HOST_PATH:PATH=${hostQt}"
    "-DQT_HOST_PATH_CMAKE_DIR:PATH=${hostQt}/lib/cmake"
    "-DQT_XCRUN:FILEPATH=${qtXcrunShim}/bin/xcrun"
  ]
  ++ qtCacheStringFlags
  ++ cacheBooleanFlags
  ++ inputFlags
  ++ enabledFeatureFlags
  ++ disabledFeatureFlags;

  inherit requiredPaths;
  staticArchives = [ ];

  postUnpack = ''
    source_archive=${lib.escapeShellArg (toString qtsvg.src)}
    case "$source_archive" in
      *-${packageSpec.source.archive_name}) ;;
      *)
        echo "error: unexpected Qt SVG source archive name: $source_archive" >&2
        exit 1
        ;;
    esac
    actual_source_sha256="$(sha256sum "$source_archive" | cut -d ' ' -f 1)"
    if test "$actual_source_sha256" != "${packageSpec.source.archive_sha256}"; then
      echo "error: Qt SVG source hash is $actual_source_sha256; expected ${packageSpec.source.archive_sha256}" >&2
      exit 1
    fi
  '';

  postConfigure = ''
    check_cache_string() {
      name="$1"
      expected="$2"
      count="$(grep -Ec "^$name:[^=]*=" CMakeCache.txt || true)"
      if test "$count" -ne 1; then
        echo "error: expected exactly one Qt SVG cache entry for $name; found $count" >&2
        exit 1
      fi
      actual="$(sed -n "s/^$name:[^=]*=//p" CMakeCache.txt)"
      if test "$actual" != "$expected"; then
        echo "error: Qt SVG cache $name is '$actual'; expected '$expected'" >&2
        exit 1
      fi
    }

    check_cache_boolean() {
      name="$1"
      expected="$2"
      count="$(grep -Ec "^$name:[^=]*=" CMakeCache.txt || true)"
      if test "$count" -ne 1; then
        echo "error: expected exactly one Qt SVG boolean cache entry for $name; found $count" >&2
        exit 1
      fi
      actual="$(sed -n "s/^$name:[^=]*=//p" CMakeCache.txt)"
      case "$actual" in
        1 | ON | TRUE | YES) actual=ON ;;
        0 | OFF | FALSE | NO) actual=OFF ;;
        *)
          echo "error: Qt SVG cache $name has non-boolean value '$actual'" >&2
          exit 1
          ;;
      esac
      if test "$actual" != "$expected"; then
        echo "error: Qt SVG cache $name is '$actual'; expected '$expected'" >&2
        exit 1
      fi
    }

    check_qtbase_feature() {
      module="$1"
      module_lower="$2"
      feature="$3"
      pri="${qtbase-ios}/mkspecs/modules/qt_lib_$module_lower.pri"
      targets="${qtbase-ios}/lib/cmake/Qt6''${module}/Qt6''${module}Targets.cmake"

      if ! test -f "$pri" || ! test -f "$targets"; then
        echo "error: qtbase metadata for $module is incomplete" >&2
        exit 1
      fi

      pri_count="$(grep -Fc "QT.$module_lower.enabled_features = " "$pri" || true)"
      if test "$pri_count" -ne 1; then
        echo "error: qtbase metadata has $pri_count enabled-feature lists for $module" >&2
        exit 1
      fi
      pri_features="$(sed -n "s/^QT\.$module_lower\.enabled_features = //p" "$pri")"
      case " $pri_features " in
        *" $feature "*) ;;
        *)
          echo "error: qtbase $module qmake metadata does not enable $feature" >&2
          exit 1
          ;;
      esac

      targets_count="$(grep -Fc '  QT_ENABLED_PUBLIC_FEATURES "' "$targets" || true)"
      if test "$targets_count" -ne 1; then
        echo "error: qtbase CMake metadata has $targets_count enabled-feature lists for $module" >&2
        exit 1
      fi
      target_features="$(sed -n 's/^  QT_ENABLED_PUBLIC_FEATURES "\(.*\)"$/\1/p' "$targets" | tr ';' ' ')"
      case " $target_features " in
        *" $feature "*) ;;
        *)
          echo "error: qtbase $module CMake metadata does not enable $feature" >&2
          exit 1
          ;;
      esac
    }

    ${cacheStringCheckScript}
    ${cacheBooleanCheckScript}
    ${inputCheckScript}
    ${enabledFeatureCheckScript}
    ${disabledFeatureCheckScript}

    check_cache_boolean CMAKE_FIND_PACKAGE_PREFER_CONFIG ON
    check_cache_string CMAKE_C_COMPILER ${lib.escapeShellArg toolchain.cc}
    check_cache_string CMAKE_CXX_COMPILER ${lib.escapeShellArg toolchain.cxx}
    check_cache_string CMAKE_OBJC_COMPILER ${lib.escapeShellArg toolchain.cc}
    check_cache_string CMAKE_OBJCXX_COMPILER ${lib.escapeShellArg toolchain.cxx}
    check_cache_string CMAKE_AR ${lib.escapeShellArg toolchain.ar}
    check_cache_string CMAKE_RANLIB ${lib.escapeShellArg toolchain.ranlib}
    check_cache_string CMAKE_STRIP ${lib.escapeShellArg toolchain.strip}
    check_cache_string CMAKE_LINKER ${lib.escapeShellArg toolchain.ld}
    check_cache_string CMAKE_NM ${lib.escapeShellArg toolchain.nm}
    check_cache_string CMAKE_INSTALL_NAME_TOOL ${lib.escapeShellArg toolchain.installNameTool}
    check_cache_string CMAKE_OTOOL ${lib.escapeShellArg toolchain.otool}
    check_cache_string CMAKE_TAPI ${lib.escapeShellArg toolchain.tapi}
    check_cache_string CMAKE_MAKE_PROGRAM ${lib.escapeShellArg "${ninja}/bin/ninja"}
    check_cache_string CMAKE_TOOLCHAIN_FILE ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6/qt.toolchain.cmake"}
    check_cache_string QT_APPLE_SDK iphoneos
    check_cache_string QT_HOST_PATH ${lib.escapeShellArg (toString hostQt)}
    check_cache_string QT_HOST_PATH_CMAKE_DIR ${lib.escapeShellArg "${hostQt}/lib/cmake"}
    check_cache_string QT_INTERNAL_APPLE_SDK_VERSION ${lib.escapeShellArg toolchain.sdkVersion}
    check_cache_string QT_INTERNAL_XCODE_VERSION ${lib.escapeShellArg toolchain.xcodeVersion}
    check_cache_string QT_XCRUN ${lib.escapeShellArg "${qtXcrunShim}/bin/xcrun"}
    check_cache_string Qt6_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6"}
    check_cache_string Qt6BuildInternals_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6BuildInternals"}
    check_cache_string Qt6BundledLibjpeg_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6BundledLibjpeg"}
    check_cache_string Qt6BundledPcre2_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6BundledPcre2"}
    check_cache_string Qt6Core_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6Core"}
    check_cache_string Qt6CorePrivate_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6CorePrivate"}
    check_cache_string Qt6EntryPointPrivate_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6EntryPointPrivate"}
    check_cache_string Qt6Gui_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6Gui"}
    check_cache_string Qt6GuiPrivate_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6GuiPrivate"}
    check_cache_string Qt6OpenGL_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6OpenGL"}
    check_cache_string Qt6OpenGLPrivate_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6OpenGLPrivate"}
    check_cache_string Qt6Widgets_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6Widgets"}
    check_cache_string Qt6WidgetsPrivate_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6WidgetsPrivate"}
    check_cache_string Qt6Network_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6Network"}
    check_cache_string Qt6NetworkPrivate_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6NetworkPrivate"}
    check_cache_string Qt6Xml_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6Xml"}
    check_cache_string Qt6XmlPrivate_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6XmlPrivate"}
    check_cache_string Qt6HostInfo_DIR ${lib.escapeShellArg "${hostQt}/lib/cmake/Qt6HostInfo"}
    check_cache_string Qt6CoreTools_DIR ${lib.escapeShellArg "${hostQt}/lib/cmake/Qt6CoreTools"}
    check_cache_string Qt6GuiTools_DIR ${lib.escapeShellArg "${hostQt}/lib/cmake/Qt6GuiTools"}
    check_cache_string Qt6WidgetsTools_DIR ${lib.escapeShellArg "${hostQt}/lib/cmake/Qt6WidgetsTools"}
    check_cache_string ZLIB_DIR ${lib.escapeShellArg "${zlib-ios}/lib/cmake/zlib"}

    check_qtbase_feature Core core xmlstreamreader
    check_qtbase_feature Gui gui cssparser
    check_qtbase_feature Gui gui imageformatplugin

    for relative_tool in ${lib.escapeShellArgs packageSpec.host_tools.executables}; do
      if ! test -x "${hostQt}/$relative_tool"; then
        echo "error: pinned host Qt tool is missing or not executable: $relative_tool" >&2
        exit 1
      fi
    done
    for package in ${lib.escapeShellArgs packageSpec.host_tools.cmake_packages}; do
      if ! test -f "${hostQt}/lib/cmake/$package/$package"Config.cmake; then
        echo "error: pinned host Qt CMake package is missing: $package" >&2
        exit 1
      fi
    done

    # qtsvg reuses qtbase's toolchain results. It does not repeat Qt's own
    # --show-sdk-path query, but CMake still resolves the logical SDK and Qt
    # still validates the SDK and Xcode versions. Compare the observed set,
    # rather than copying qtbase's stricter four-call requirement.
    xcrun_log="$NIX_BUILD_TOP/qt-xcrun-shim.log"
    if ! test -s "$xcrun_log" || test -L "$xcrun_log"; then
      echo "error: Qt SVG did not produce a regular non-empty xcrun shim log" >&2
      exit 1
    fi
    if grep -Ev -- '^(-sdk iphoneos --show-sdk-path|--sdk iphoneos --show-sdk-path|--sdk iphoneos --show-sdk-version|xcodebuild -version)$' "$xcrun_log"; then
      echo "error: Qt SVG made an unsupported xcrun invocation" >&2
      exit 1
    fi

    expected_xcrun_set="$NIX_BUILD_TOP/qtsvg-xcrun.expected"
    actual_xcrun_set="$NIX_BUILD_TOP/qtsvg-xcrun.actual"
    printf '%s\n' \
      '-sdk iphoneos --show-sdk-path' \
      '--sdk iphoneos --show-sdk-version' \
      'xcodebuild -version' \
      | LC_ALL=C sort > "$expected_xcrun_set"
    LC_ALL=C sort -u "$xcrun_log" > "$actual_xcrun_set"
    if ! cmp -s "$expected_xcrun_set" "$actual_xcrun_set"; then
      echo "error: Qt SVG xcrun invocation set differs from its audited add-on contract" >&2
      echo "expected:" >&2
      cat "$expected_xcrun_set" >&2
      echo "actual:" >&2
      cat "$actual_xcrun_set" >&2
      exit 1
    fi
  '';

  postInstall = ''
    for archive in libQt6Core.a libQt6Gui.a libQt6Widgets.a libQt6BundledPcre2.a; do
      if ! test -f "${qtbase-ios}/lib/$archive"; then
        echo "error: qtbase-ios omits a qtsvg PRL dependency: $archive" >&2
        exit 1
      fi
    done
    ${prlRewriteScript}

    rewrite_pri_line() {
      pri="$1"
      expected="$2"
      replacement="$3"
      count="$(grep -Fxc -- "$expected" "$pri" || true)"
      if test "$count" -ne 1; then
        echo "error: $pri contains $count exact copies of '$expected'; expected 1" >&2
        exit 1
      fi
      substituteInPlace "$pri" --replace-fail "$expected" "$replacement"
    }

    svg_pri="$out/mkspecs/modules/qt_lib_svg.pri"
    svg_private_pri="$out/mkspecs/modules/qt_lib_svg_private.pri"
    svgwidgets_pri="$out/mkspecs/modules/qt_lib_svgwidgets.pri"
    for pri in "$svg_pri" "$svg_private_pri" "$svgwidgets_pri"; do
      if ! test -f "$pri" || test -L "$pri"; then
        echo "error: Qt SVG module PRI is not a regular non-symlink file: $pri" >&2
        exit 1
      fi
    done

    rewrite_pri_line "$svg_pri" \
      ${lib.escapeShellArg "QT.svg.libs = $$QT_MODULE_LIB_BASE"} \
      "QT.svg.libs = $out/lib"
    rewrite_pri_line "$svg_pri" \
      ${lib.escapeShellArg "QT.svg.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtSvg"} \
      "QT.svg.includes = $out/include $out/include/QtSvg"
    rewrite_pri_line "$svg_pri" \
      ${lib.escapeShellArg "QT.svg.bins = $$QT_MODULE_BIN_BASE"} \
      'QT.svg.bins ='

    rewrite_pri_line "$svg_private_pri" \
      ${lib.escapeShellArg "QT.svg_private.libs = $$QT_MODULE_LIB_BASE"} \
      "QT.svg_private.libs = $out/lib"
    rewrite_pri_line "$svg_private_pri" \
      ${lib.escapeShellArg "QT.svg_private.includes = $$QT_MODULE_INCLUDE_BASE/QtSvg/${packageSpec.version} $$QT_MODULE_INCLUDE_BASE/QtSvg/${packageSpec.version}/QtSvg"} \
      "QT.svg_private.includes = $out/include/QtSvg/${packageSpec.version} $out/include/QtSvg/${packageSpec.version}/QtSvg"

    rewrite_pri_line "$svgwidgets_pri" \
      ${lib.escapeShellArg "QT.svgwidgets.libs = $$QT_MODULE_LIB_BASE"} \
      "QT.svgwidgets.libs = $out/lib"
    rewrite_pri_line "$svgwidgets_pri" \
      ${lib.escapeShellArg "QT.svgwidgets.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtSvgWidgets"} \
      "QT.svgwidgets.includes = $out/include $out/include/QtSvgWidgets"
    rewrite_pri_line "$svgwidgets_pri" \
      ${lib.escapeShellArg "QT.svgwidgets.bins = $$QT_MODULE_BIN_BASE"} \
      'QT.svgwidgets.bins ='

    add_plugin_pri_path() {
      plugin="$1"
      pri="$2"
      if ! test -f "$pri" || test -L "$pri"; then
        echo "error: Qt SVG plugin PRI is not a regular non-symlink file: $pri" >&2
        exit 1
      fi
      if grep -Eq "^QT_PLUGIN\.$plugin\.PATH[[:space:]]*=" "$pri"; then
        echo "error: Qt SVG plugin PRI already declares an unexpected path: $pri" >&2
        exit 1
      fi
      printf '\nQT_PLUGIN.%s.PATH = %s\n' "$plugin" "$out/plugins" >> "$pri"
    }
    add_plugin_pri_path qsvg "$out/mkspecs/modules/qt_plugin_qsvg.pri"
    add_plugin_pri_path qsvgicon "$out/mkspecs/modules/qt_plugin_qsvgicon.pri"

    prl_paths=(${lib.escapeShellArgs (builtins.attrNames prlDependencies)})
    if grep -E -n -- '\$\$\[QT_INSTALL_LIBS\]/libQt6[^ ;]*\.a' "''${prl_paths[@]/#/$out/}"; then
      echo "error: a Qt SVG PRL retains an ambiguous Qt archive location" >&2
      exit 1
    fi
  '';

  postInstallCheck = ''
    expected_archives="$NIX_BUILD_TOP/qtsvg-archives.expected"
    actual_archives="$NIX_BUILD_TOP/qtsvg-archives.actual"
    printf '%s\n' ${lib.escapeShellArgs staticArchives} | LC_ALL=C sort > "$expected_archives"
    find "$out" -name '*.a' -print \
      | sed "s|^$out/||" \
      | LC_ALL=C sort > "$actual_archives"
    if ! cmp -s "$expected_archives" "$actual_archives"; then
      echo "error: Qt SVG static archive set differs from the locked manifest" >&2
      echo "expected:" >&2
      cat "$expected_archives" >&2
      echo "actual:" >&2
      cat "$actual_archives" >&2
      exit 1
    fi

    expected_objects="$NIX_BUILD_TOP/qtsvg-objects.expected"
    actual_objects="$NIX_BUILD_TOP/qtsvg-objects.actual"
    printf '%s\n' ${lib.escapeShellArgs standaloneObjects} | LC_ALL=C sort > "$expected_objects"
    find "$out" -name '*.o' -print \
      | sed "s|^$out/||" \
      | LC_ALL=C sort > "$actual_objects"
    if ! cmp -s "$expected_objects" "$actual_objects"; then
      echo "error: Qt SVG standalone object set differs from the locked manifest" >&2
      echo "expected:" >&2
      cat "$expected_objects" >&2
      echo "actual:" >&2
      cat "$actual_objects" >&2
      exit 1
    fi

    for relative_object in ${lib.escapeShellArgs (staticArchives ++ standaloneObjects)}; do
      if ! test -f "$out/$relative_object" || test -L "$out/$relative_object"; then
        echo "error: Qt SVG Apple object is not a regular non-symlink file: $relative_object" >&2
        exit 1
      fi
    done

    for relative_path in ${lib.escapeShellArgs packageSpec.artifacts.forbidden.paths}; do
      if test -e "$out/$relative_path" || test -L "$out/$relative_path"; then
        echo "error: Qt SVG installed forbidden target output: $relative_path" >&2
        exit 1
      fi
    done
    for directory_name in doc docs examples sbom tests; do
      forbidden_directory="$(find "$out" \( -type d -o -type l \) -name "$directory_name" -print -quit)"
      if test -n "$forbidden_directory"; then
        echo "error: Qt SVG installed forbidden documentation or test data: $forbidden_directory" >&2
        exit 1
      fi
    done
    for executable_root in "$out/bin" "$out/libexec"; do
      if test -e "$executable_root" || test -L "$executable_root"; then
        echo "error: Qt SVG output contains a host or target tool directory: $executable_root" >&2
        exit 1
      fi
    done

    forbidden_file="$(find "$out" \( \
      \( \( -type d -o -type l \) \( -name '*.app' -o -name '*.framework' \) \) -o \
      \( \( -type f -o -type l \) \
         \( -name '*.dylib' -o -name '*.so' -o -name '*.so.*' \) \) \
      \) -print -quit)"
    if test -n "$forbidden_file"; then
      echo "error: Qt SVG installed a non-static target: $forbidden_file" >&2
      exit 1
    fi

    while IFS= read -r -d "" executable; do
      if file "$executable" | grep -Fq 'Mach-O'; then
        echo "error: Qt SVG installed a host or target Mach-O tool: $executable" >&2
        exit 1
      fi
    done < <(find "$out" -type f -perm -0100 -print0)

    if grep -R -a -l -F ${lib.escapeShellArg (toString qtsvg.src)} "$out"; then
      echo "error: Qt SVG output refers to its source archive" >&2
      exit 1
    fi
    for native_store_path in \
      ${lib.escapeShellArg (toString hostQt)} \
      ${lib.escapeShellArg (toString qtXcrunShim)}; do
      if grep -R -a -l -F "$native_store_path" "$out"; then
        echo "error: Qt SVG output refers to a native build input: $native_store_path" >&2
        exit 1
      fi
    done

    ${prlCheckScript}
    prl_paths=(${lib.escapeShellArgs (builtins.attrNames prlDependencies)})
    for index in "''${!prl_paths[@]}"; do
      prl_paths[$index]="$out/''${prl_paths[$index]}"
    done
    if grep -E -n -- '\$\$\[QT_INSTALL_LIBS\]/libQt6[^ ;]*\.a' "''${prl_paths[@]}"; then
      echo "error: a Qt SVG PRL retains an ambiguous Qt archive location" >&2
      exit 1
    fi

    module_pri_paths=(${lib.escapeShellArgs modulePriPaths})
    plugin_pri_paths=(${lib.escapeShellArgs pluginPriPaths})
    for index in "''${!module_pri_paths[@]}"; do
      module_pri_paths[$index]="$out/''${module_pri_paths[$index]}"
    done
    for index in "''${!plugin_pri_paths[@]}"; do
      plugin_pri_paths[$index]="$out/''${plugin_pri_paths[$index]}"
    done
    if grep -E -n -- '\$\$QT_MODULE_(LIB|INCLUDE|BIN)_BASE' "''${module_pri_paths[@]}"; then
      echo "error: Qt SVG module PRI retains a base-prefix placeholder" >&2
      exit 1
    fi

    grep -Fxc "QT.svg.libs = $out/lib" "''${module_pri_paths[0]}" | grep -Fxq 1
    grep -Fxc "QT.svg.includes = $out/include $out/include/QtSvg" "''${module_pri_paths[0]}" | grep -Fxq 1
    grep -Fxc 'QT.svg.bins =' "''${module_pri_paths[0]}" | grep -Fxq 1
    grep -Fxc "QT.svg_private.libs = $out/lib" "''${module_pri_paths[1]}" | grep -Fxq 1
    grep -Fxc "QT.svg_private.includes = $out/include/QtSvg/${packageSpec.version} $out/include/QtSvg/${packageSpec.version}/QtSvg" "''${module_pri_paths[1]}" | grep -Fxq 1
    grep -Fxc "QT.svgwidgets.libs = $out/lib" "''${module_pri_paths[2]}" | grep -Fxq 1
    grep -Fxc "QT.svgwidgets.includes = $out/include $out/include/QtSvgWidgets" "''${module_pri_paths[2]}" | grep -Fxq 1
    grep -Fxc 'QT.svgwidgets.bins =' "''${module_pri_paths[2]}" | grep -Fxq 1
    grep -Fxc "QT_PLUGIN.qsvg.PATH = $out/plugins" "''${plugin_pri_paths[0]}" | grep -Fxq 1
    grep -Fxc "QT_PLUGIN.qsvgicon.PATH = $out/plugins" "''${plugin_pri_paths[1]}" | grep -Fxq 1

    metadata_paths=("''${prl_paths[@]}" "''${module_pri_paths[@]}" "''${plugin_pri_paths[@]}")

    while IFS= read -r store_path; do
      test -n "$store_path" || continue
      allowed=false
      if test "$store_path" = "$out"; then
        allowed=true
      else
        for allowed_path in ${lib.escapeShellArgs allowedTargetStorePaths}; do
          if test "$store_path" = "$allowed_path"; then
            allowed=true
            break
          fi
        done
      fi
      if test "$allowed" != true; then
        echo "error: Qt SVG qmake metadata refers to undeclared or native store input: $store_path" >&2
        exit 1
      fi
    done < <(grep -ahoE '/nix/store/[0-9a-z]{32}-[^/;"[:space:]]+' "''${metadata_paths[@]}" | LC_ALL=C sort -u || true)

    svg_targets="$out/lib/cmake/Qt6Svg/Qt6SvgTargets.cmake"
    svg_release="$out/lib/cmake/Qt6Svg/Qt6SvgTargets-release.cmake"
    widgets_targets="$out/lib/cmake/Qt6SvgWidgets/Qt6SvgWidgetsTargets.cmake"
    widgets_release="$out/lib/cmake/Qt6SvgWidgets/Qt6SvgWidgetsTargets-release.cmake"
    image_plugin_targets="$out/lib/cmake/Qt6Gui/Qt6QSvgPluginTargets.cmake"
    image_plugin_release="$out/lib/cmake/Qt6Gui/Qt6QSvgPluginTargets-release.cmake"
    icon_plugin_targets="$out/lib/cmake/Qt6Gui/Qt6QSvgIconPluginTargets.cmake"
    icon_plugin_release="$out/lib/cmake/Qt6Gui/Qt6QSvgIconPluginTargets-release.cmake"

    check_cmake_line() {
      cmake_file="$1"
      expected_line="$2"
      if ! test -f "$cmake_file" || test -L "$cmake_file"; then
        echo "error: Qt SVG CMake target metadata is not a regular non-symlink file: $cmake_file" >&2
        exit 1
      fi
      line_count="$(grep -Fxc -- "$expected_line" "$cmake_file" || true)"
      if test "$line_count" -ne 1; then
        echo "error: $cmake_file contains $line_count exact copies of '$expected_line'; expected 1" >&2
        exit 1
      fi
    }

    check_cmake_line "$svg_targets" \
      'add_library(Qt6::Svg STATIC IMPORTED)'
    check_cmake_line "$svg_release" \
      '  IMPORTED_LOCATION_RELEASE "''${_IMPORT_PREFIX}/lib/libQt6Svg.a"'
    check_cmake_line "$widgets_targets" \
      'add_library(Qt6::SvgWidgets STATIC IMPORTED)'
    check_cmake_line "$widgets_release" \
      '  IMPORTED_LOCATION_RELEASE "''${_IMPORT_PREFIX}/lib/libQt6SvgWidgets.a"'
    check_cmake_line "$image_plugin_targets" \
      'add_library(Qt6::QSvgPlugin STATIC IMPORTED)'
    check_cmake_line "$image_plugin_targets" \
      'add_library(Qt6::QSvgPlugin_init OBJECT IMPORTED)'
    check_cmake_line "$image_plugin_release" \
      '  IMPORTED_LOCATION_RELEASE "''${_IMPORT_PREFIX}/plugins/imageformats/libqsvg.a"'
    check_cmake_line "$image_plugin_release" \
      '  IMPORTED_OBJECTS_RELEASE "''${_IMPORT_PREFIX}/plugins/imageformats/objects-Release/QSvgPlugin_init/QSvgPlugin_init.cpp.o"'
    check_cmake_line "$icon_plugin_targets" \
      'add_library(Qt6::QSvgIconPlugin STATIC IMPORTED)'
    check_cmake_line "$icon_plugin_targets" \
      'add_library(Qt6::QSvgIconPlugin_init OBJECT IMPORTED)'
    check_cmake_line "$icon_plugin_release" \
      '  IMPORTED_LOCATION_RELEASE "''${_IMPORT_PREFIX}/plugins/iconengines/libqsvgicon.a"'
    check_cmake_line "$icon_plugin_release" \
      '  IMPORTED_OBJECTS_RELEASE "''${_IMPORT_PREFIX}/plugins/iconengines/objects-Release/QSvgIconPlugin_init/QSvgIconPlugin_init.cpp.o"'

    svg_link_count="$(grep -Fc '  INTERFACE_LINK_LIBRARIES "' "$svg_targets" || true)"
    if test "$svg_link_count" -ne 1 \
      || ! grep -F '  INTERFACE_LINK_LIBRARIES "' "$svg_targets" | grep -Fq 'WrapZLIB::WrapZLIB'; then
      echo "error: Qt6::Svg does not have exactly one link interface containing WrapZLIB" >&2
      exit 1
    fi

    widgets_link_count="$(grep -Fc '  INTERFACE_LINK_LIBRARIES "' "$widgets_targets" || true)"
    if test "$widgets_link_count" -ne 1 \
      || ! grep -F '  INTERFACE_LINK_LIBRARIES "' "$widgets_targets" | grep -Fq 'Qt6::Svg' \
      || ! grep -F '  INTERFACE_LINK_LIBRARIES "' "$widgets_targets" | grep -Fq 'Qt6::Widgets'; then
      echo "error: Qt6::SvgWidgets does not have exactly one expected link interface" >&2
      exit 1
    fi

    for plugin_targets in "$image_plugin_targets" "$icon_plugin_targets"; do
      plugin_svg_link_count="$(grep -F '  INTERFACE_LINK_LIBRARIES "' "$plugin_targets" \
        | grep -Fc 'Qt6::Svg' || true)"
      if test "$plugin_svg_link_count" -ne 1; then
        echo "error: Qt SVG plugin target does not have exactly one link interface containing Qt6::Svg: $plugin_targets" >&2
        exit 1
      fi
    done

    version_header="$out/include/QtSvg/qtsvgversion.h"
    grep -Eq '^#define[[:space:]]+QTSVG_VERSION_STR[[:space:]]+"${packageSpec.version}"([[:space:]]|$)' "$version_header"
    for module in Svg SvgWidgets; do
      module_json="$out/modules/$module.json"
      grep -Eq '"repository"[[:space:]]*:[[:space:]]*"qtsvg"' "$module_json"
      grep -Eq '"version"[[:space:]]*:[[:space:]]*"${packageSpec.version}"' "$module_json"
      grep -Eq '"name"[[:space:]]*:[[:space:]]*"'$module'"' "$module_json"
      grep -Eq '"name"[[:space:]]*:[[:space:]]*"iOS"' "$module_json"
      grep -Eq '"variant"[[:space:]]*:[[:space:]]*"iphoneos"' "$module_json"
      grep -Eq '"architecture"[[:space:]]*:[[:space:]]*"${toolchain.architecture}"' "$module_json"
      grep -Eq '"static"[[:space:]]*:[[:space:]]*true' "$module_json"
    done
  '';

  passthru = {
    iosQtModule = packageSpec.name;
    iosQtStandaloneObjects = standaloneObjects;
    iosQtStaticArchives = staticArchives;
  };

  meta = {
    description = "Static Qt ${packageSpec.version} SVG modules for the pinned Krita iPadOS target";
    inherit (qtsvg.meta) license;
  };
}

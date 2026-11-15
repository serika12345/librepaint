{
  lib,
  gnugrep,
  mkIOSCMakePackage,
  ninja,
  packageSpec,
  qt6Packages,
  qtbase-ios,
  qtXcrunShim,
  toolchain,
}:

let
  qt5compat = qt6Packages.qt5compat;
  hostQt = qt6Packages.qtbase;

  staticArchives = packageSpec.artifacts.static_archives;
  standaloneObjects = packageSpec.artifacts.standalone_objects;
  requiredPaths = packageSpec.artifacts.required_paths ++ staticArchives ++ standaloneObjects;

  cacheStringLocks = packageSpec.configure.cache_string_locks;
  cacheBooleanLocks = packageSpec.configure.cache_boolean_locks;
  inputLocks = packageSpec.configure.input_locks;
  commonCacheStringLocks = {
    CMAKE_BUILD_TYPE = "Release";
    CMAKE_OSX_ARCHITECTURES = toolchain.architecture;
    CMAKE_OSX_DEPLOYMENT_TARGET = toolchain.deploymentTarget;
    CMAKE_OSX_SYSROOT = "iphoneos";
    CMAKE_SYSTEM_NAME = "iOS";
  };
  moduleCacheStringLocks = removeAttrs cacheStringLocks (builtins.attrNames commonCacheStringLocks);

  cmakeBoolean = value: if value then "ON" else "OFF";
  cacheBooleanFlags = lib.mapAttrsToList (name: value: "-D${name}=${cmakeBoolean value}") (
    removeAttrs cacheBooleanLocks [ "QT_WILL_BUILD_TOOLS" ]
  );
  moduleCacheStringFlags = lib.mapAttrsToList (
    name: value: "-D${name}=${value}"
  ) moduleCacheStringLocks;
  inputFlags = lib.mapAttrsToList (name: value: "-D${name}=${value}") inputLocks;
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
    '') inputLocks
  );
  enabledFeatureCheckScript = lib.concatMapStrings (feature: ''
    check_cache_boolean ${lib.escapeShellArg "FEATURE_${feature}"} ON
  '') packageSpec.configure.feature_locks.enabled;
  disabledFeatureCheckScript = lib.concatMapStrings (feature: ''
    check_cache_boolean ${lib.escapeShellArg "FEATURE_${feature}"} OFF
  '') packageSpec.configure.feature_locks.disabled;

  dependencyEntry = dependency: {
    key = dependency.outPath;
    value = dependency;
  };
  allowedTargetDependencyClosure = map (entry: entry.value) (
    builtins.genericClosure {
      startSet = map dependencyEntry [ qtbase-ios ];
      operator = entry: map dependencyEntry (entry.value.propagatedBuildInputs or [ ]);
    }
  );
  allowedTargetStorePaths = map toString allowedTargetDependencyClosure;
in
assert lib.assertMsg (
  packageSpec.name == "qt5compat"
) "Qt package manifest entry must be qt5compat";
assert lib.assertMsg (packageSpec.version == qt5compat.version)
  "Qt5Compat target source version ${qt5compat.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.version == hostQt.version
) "Qt5Compat host Qt version ${hostQt.version} does not match iOS manifest ${packageSpec.version}";
assert lib.assertMsg (
  packageSpec.source.flake_attr == "source-qt5compat"
) "Qt5Compat source manifest must use the source-qt5compat flake output";
assert lib.assertMsg (
  packageSpec.source.archive_name == "qt5compat-everywhere-src-${packageSpec.version}.tar.xz"
) "Qt5Compat source archive name does not match its pinned version";
assert lib.assertMsg (
  packageSpec.source.archive_sha256
  == "78cf1c283795312a7fa31c73eb7f7f556b48374adc484f10ad5a3c0bcb59264e"
) "Qt5Compat source archive hash differs from the audited Qt 6.12.0 archive";
assert lib.assertMsg (
  packageSpec.dependencies.target_packages == [ ]
  && packageSpec.dependencies.qt_modules == [ "qtbase" ]
) "Qt5Compat must depend directly and only on the target Qtbase module";
assert lib.assertMsg (
  packageSpec.host_tools.flake_attr == "host-qtbase"
  && packageSpec.host_tools.version == hostQt.version
) "Qt5Compat host-tool manifest must use the matching host Qtbase";
assert lib.assertMsg (
  packageSpec.host_tools.cmake_packages == [
    "Qt6HostInfo"
    "Qt6CoreTools"
    "Qt6GuiTools"
    "Qt6WidgetsTools"
  ]
) "Qt5Compat host CMake package contract changed";
assert lib.assertMsg (
  packageSpec.host_tools.executables == [
    "libexec/syncqt"
    "libexec/moc"
    "libexec/cmake_automoc_parser"
  ]
) "Qt5Compat host executable contract changed";
assert lib.assertMsg (
  packageSpec.configure.sdk_lock == {
    allow_absolute_path = false;
    cmake_cache_value = "iphoneos";
    name = "iphoneos";
  }
) "Qt5Compat must preserve a logical iphoneos SDK in its cache";
assert lib.assertMsg (
  packageSpec.configure.languages == [
    "C"
    "CXX"
    "OBJC"
    "OBJCXX"
  ]
) "Qt5Compat language manifest must match the module project";
assert lib.assertMsg (
  lib.getAttrs (builtins.attrNames commonCacheStringLocks) cacheStringLocks == commonCacheStringLocks
) "Qt5Compat common CMake cache locks disagree with the pinned iOS toolchain";
assert lib.assertMsg (
  moduleCacheStringLocks == { QT_QMAKE_TARGET_MKSPEC = "macx-ios-clang"; }
) "Qt5Compat has an unsupported project-specific string cache lock";
assert lib.assertMsg (
  cacheBooleanLocks == {
    BUILD_SHARED_LIBS = false;
    CMAKE_FIND_PACKAGE_TARGETS_GLOBAL = true;
    QT_BUILD_DOCS = false;
    QT_BUILD_EXAMPLES = false;
    QT_BUILD_TESTS = false;
    QT_BUILD_TOOLS_BY_DEFAULT = false;
    QT_GENERATE_SBOM = false;
    QT_WILL_BUILD_TOOLS = false;
  }
) "Qt5Compat boolean cache contract changed";
assert lib.assertMsg (inputLocks == { }) "Qt5Compat must not accept unlocked configure inputs";
assert lib.assertMsg (
  packageSpec.configure.feature_locks.enabled == [
    "big_codecs"
    "codecs"
    "textcodec"
  ]
  &&
    packageSpec.configure.feature_locks.disabled == [
      "iconv"
      "pkg_config"
    ]
) "Qt5Compat feature contract changed";
assert lib.assertMsg (
  packageSpec.artifacts.exact_static_archive_set && packageSpec.artifacts.exact_standalone_object_set
) "Qt5Compat output sets must both be exact";
assert lib.assertMsg (
  staticArchives == [ "lib/libQt6Core5Compat.a" ] && standaloneObjects == [ ]
) "Qt5Compat Apple object manifest changed";
assert lib.assertMsg (
  packageSpec.artifacts.expected_counts == {
    standalone_objects = 0;
    static_archives = 1;
  }
) "Qt5Compat artifact counts do not match its exact output sets";
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
) "Qt5Compat forbidden output-glob contract changed";
assert lib.assertMsg (
  packageSpec.artifacts.forbidden.reference_literals == [ toolchain.xcodeApp ]
  && packageSpec.artifacts.forbidden.reference_environment_values == [ "NIX_BUILD_TOP" ]
) "Qt5Compat forbidden reference contract changed";
mkIOSCMakePackage {
  pname = "qt5compat-ios";
  inherit (packageSpec) version;
  src = qt5compat.src;

  targetDependencies = [ qtbase-ios ];

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
  ++ moduleCacheStringFlags
  ++ cacheBooleanFlags
  ++ inputFlags
  ++ enabledFeatureFlags
  ++ disabledFeatureFlags;

  inherit requiredPaths;
  staticArchives = [ ];

  postUnpack = ''
    source_archive=${lib.escapeShellArg (toString qt5compat.src)}
    case "$source_archive" in
      *-${packageSpec.source.archive_name}) ;;
      *)
        echo "error: unexpected Qt5Compat source archive name: $source_archive" >&2
        exit 1
        ;;
    esac
    actual_source_sha256="$(sha256sum "$source_archive" | cut -d ' ' -f 1)"
    if test "$actual_source_sha256" != "${packageSpec.source.archive_sha256}"; then
      echo "error: Qt5Compat source hash is $actual_source_sha256; expected ${packageSpec.source.archive_sha256}" >&2
      exit 1
    fi
  '';

  postConfigure = ''
    check_cache_string() {
      name="$1"
      expected="$2"
      count="$(grep -Ec "^$name:[^=]*=" CMakeCache.txt || true)"
      if test "$count" -ne 1; then
        echo "error: expected exactly one Qt5Compat cache entry for $name; found $count" >&2
        exit 1
      fi
      actual="$(sed -n "s/^$name:[^=]*=//p" CMakeCache.txt)"
      if test "$actual" != "$expected"; then
        echo "error: Qt5Compat cache $name is '$actual'; expected '$expected'" >&2
        exit 1
      fi
    }

    check_cache_boolean() {
      name="$1"
      expected="$2"
      count="$(grep -Ec "^$name:[^=]*=" CMakeCache.txt || true)"
      if test "$count" -ne 1; then
        echo "error: expected exactly one Qt5Compat boolean cache entry for $name; found $count" >&2
        exit 1
      fi
      actual="$(sed -n "s/^$name:[^=]*=//p" CMakeCache.txt)"
      case "$actual" in
        1 | ON | TRUE | YES) actual=ON ;;
        0 | OFF | FALSE | NO) actual=OFF ;;
        *)
          echo "error: Qt5Compat cache $name has non-boolean value '$actual'" >&2
          exit 1
          ;;
      esac
      if test "$actual" != "$expected"; then
        echo "error: Qt5Compat cache $name is '$actual'; expected '$expected'" >&2
        exit 1
      fi
    }

    ${cacheStringCheckScript}
    ${cacheBooleanCheckScript}
    ${inputCheckScript}
    ${enabledFeatureCheckScript}
    ${disabledFeatureCheckScript}

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
    check_cache_string QT_XCRUN ${lib.escapeShellArg "${qtXcrunShim}/bin/xcrun"}
    check_cache_string Qt6_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6"}
    check_cache_string Qt6Core_DIR ${lib.escapeShellArg "${qtbase-ios}/lib/cmake/Qt6Core"}
    check_cache_string Qt6HostInfo_DIR ${lib.escapeShellArg "${hostQt}/lib/cmake/Qt6HostInfo"}
    check_cache_string Qt6CoreTools_DIR ${lib.escapeShellArg "${hostQt}/lib/cmake/Qt6CoreTools"}

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

    xcrun_log="$NIX_BUILD_TOP/qt-xcrun-shim.log"
    if ! test -s "$xcrun_log" || test -L "$xcrun_log"; then
      echo "error: Qt5Compat/CMake did not call the pinned xcrun shim" >&2
      exit 1
    fi
    if grep -Ev '^(-sdk iphoneos --show-sdk-path|--sdk iphoneos --show-sdk-path|--sdk iphoneos --show-sdk-version|xcodebuild -version)$' "$xcrun_log"; then
      echo "error: Qt5Compat/CMake made an unsupported xcrun invocation" >&2
      exit 1
    fi
    expected_xcrun_set="$NIX_BUILD_TOP/qt5compat-xcrun.expected"
    actual_xcrun_set="$NIX_BUILD_TOP/qt5compat-xcrun.actual"
    printf '%s\n' \
      '-sdk iphoneos --show-sdk-path' \
      '--sdk iphoneos --show-sdk-version' \
      'xcodebuild -version' \
      | LC_ALL=C sort > "$expected_xcrun_set"
    LC_ALL=C sort -u "$xcrun_log" > "$actual_xcrun_set"
    if ! cmp -s "$expected_xcrun_set" "$actual_xcrun_set"; then
      echo "error: Qt5Compat xcrun invocation set differs from its audited add-on contract" >&2
      echo "expected:" >&2
      cat "$expected_xcrun_set" >&2
      echo "actual:" >&2
      cat "$actual_xcrun_set" >&2
      exit 1
    fi
  '';

  postInstall = ''
    prl="$out/lib/libQt6Core5Compat.prl"
    public_pri="$out/mkspecs/modules/qt_lib_core5compat.pri"
    private_pri="$out/mkspecs/modules/qt_lib_core5compat_private.pri"
    if ! test -f "$prl" || test -L "$prl"; then
      echo "error: Qt5Compat PRL is missing or not a regular file" >&2
      exit 1
    fi
    for pri in "$public_pri" "$private_pri"; do
      if ! test -f "$pri" || test -L "$pri"; then
        echo "error: Qt5Compat qmake metadata is missing or not a regular file: $pri" >&2
        exit 1
      fi
    done

    core_reference='$$[QT_INSTALL_LIBS]/libQt6Core.a'
    bundled_pcre2_reference='$$[QT_INSTALL_LIBS]/libQt6BundledPcre2.a'
    if test "$(grep -Fo "$core_reference" "$prl" | wc -l | tr -d ' ')" -ne 2; then
      echo "error: Qt5Compat PRL does not contain exactly two relocatable QtCore references" >&2
      exit 1
    fi
    if test "$(grep -Fo "$bundled_pcre2_reference" "$prl" | wc -l | tr -d ' ')" -ne 2; then
      echo "error: Qt5Compat PRL does not contain exactly two relocatable bundled PCRE2 references" >&2
      exit 1
    fi

    substituteInPlace "$prl" \
      --replace-fail "$core_reference" "${qtbase-ios}/lib/libQt6Core.a" \
      --replace-fail "$bundled_pcre2_reference" "${qtbase-ios}/lib/libQt6BundledPcre2.a"
    substituteInPlace "$public_pri" \
      --replace-fail \
        'QT.core5compat.libs = $$QT_MODULE_LIB_BASE' \
        "QT.core5compat.libs = $out/lib" \
      --replace-fail \
        'QT.core5compat.includes = $$QT_MODULE_INCLUDE_BASE $$QT_MODULE_INCLUDE_BASE/QtCore5Compat' \
        "QT.core5compat.includes = $out/include $out/include/QtCore5Compat" \
      --replace-fail \
        'QT.core5compat.bins = $$QT_MODULE_BIN_BASE' \
        'QT.core5compat.bins ='
    substituteInPlace "$private_pri" \
      --replace-fail \
        'QT.core5compat_private.libs = $$QT_MODULE_LIB_BASE' \
        "QT.core5compat_private.libs = $out/lib" \
      --replace-fail \
        'QT.core5compat_private.includes = $$QT_MODULE_INCLUDE_BASE/QtCore5Compat/${packageSpec.version} $$QT_MODULE_INCLUDE_BASE/QtCore5Compat/${packageSpec.version}/QtCore5Compat' \
        "QT.core5compat_private.includes = $out/include/QtCore5Compat/${packageSpec.version} $out/include/QtCore5Compat/${packageSpec.version}/QtCore5Compat"

    test "$(grep -Foc '${qtbase-ios}/lib/libQt6Core.a' "$prl")" -eq 2
    test "$(grep -Foc '${qtbase-ios}/lib/libQt6BundledPcre2.a' "$prl")" -eq 2
    if grep -Fq '$$[QT_INSTALL_LIBS]/libQt6Core.a' "$prl" \
      || grep -Fq '$$[QT_INSTALL_LIBS]/libQt6BundledPcre2.a' "$prl"; then
      echo "error: Qt5Compat PRL still contains a self-output Qtbase archive reference" >&2
      exit 1
    fi
    test "$(grep -Fxc "QT.core5compat.libs = $out/lib" "$public_pri")" -eq 1
    test "$(grep -Fxc "QT.core5compat.includes = $out/include $out/include/QtCore5Compat" "$public_pri")" -eq 1
    test "$(grep -Fxc 'QT.core5compat.bins =' "$public_pri")" -eq 1
    test "$(grep -Fxc "QT.core5compat_private.libs = $out/lib" "$private_pri")" -eq 1
    test "$(grep -Fxc "QT.core5compat_private.includes = $out/include/QtCore5Compat/${packageSpec.version} $out/include/QtCore5Compat/${packageSpec.version}/QtCore5Compat" "$private_pri")" -eq 1
    if grep -Eq '\$\$QT_MODULE_(LIB|INCLUDE|BIN)_BASE' "$public_pri" "$private_pri"; then
      echo "error: Qt5Compat qmake metadata still contains a shared-prefix placeholder" >&2
      exit 1
    fi
  '';

  postInstallCheck = ''
    expected_archives="$NIX_BUILD_TOP/qt5compat-archives.expected"
    actual_archives="$NIX_BUILD_TOP/qt5compat-archives.actual"
    printf '%s\n' ${lib.escapeShellArgs staticArchives} | LC_ALL=C sort > "$expected_archives"
    find "$out" -name '*.a' -print \
      | sed "s|^$out/||" \
      | LC_ALL=C sort > "$actual_archives"
    if ! cmp -s "$expected_archives" "$actual_archives"; then
      echo "error: Qt5Compat static archive set differs from the locked manifest" >&2
      echo "expected:" >&2
      cat "$expected_archives" >&2
      echo "actual:" >&2
      cat "$actual_archives" >&2
      exit 1
    fi

    expected_objects="$NIX_BUILD_TOP/qt5compat-objects.expected"
    actual_objects="$NIX_BUILD_TOP/qt5compat-objects.actual"
    : > "$expected_objects"
    ${lib.optionalString (standaloneObjects != [ ]) ''
      printf '%s\n' ${lib.escapeShellArgs standaloneObjects} | LC_ALL=C sort > "$expected_objects"
    ''}
    find "$out" -name '*.o' -print \
      | sed "s|^$out/||" \
      | LC_ALL=C sort > "$actual_objects"
    if ! cmp -s "$expected_objects" "$actual_objects"; then
      echo "error: Qt5Compat standalone object set differs from the locked manifest" >&2
      echo "expected:" >&2
      cat "$expected_objects" >&2
      echo "actual:" >&2
      cat "$actual_objects" >&2
      exit 1
    fi

    for relative_object in ${lib.escapeShellArgs (staticArchives ++ standaloneObjects)}; do
      if ! test -f "$out/$relative_object" || test -L "$out/$relative_object"; then
        echo "error: Qt5Compat Apple object is not a regular non-symlink file: $relative_object" >&2
        exit 1
      fi
    done

    for relative_path in ${lib.escapeShellArgs packageSpec.artifacts.forbidden.paths}; do
      if test -e "$out/$relative_path" || test -L "$out/$relative_path"; then
        echo "error: Qt5Compat installed forbidden target output: $relative_path" >&2
        exit 1
      fi
    done
    for tool_root in "$out/bin" "$out/libexec"; do
      if test -e "$tool_root" || test -L "$tool_root"; then
        echo "error: Qt5Compat installed a target or host tool directory: $tool_root" >&2
        exit 1
      fi
    done
    for directory in doc examples sbom tests share/doc; do
      if test -e "$out/$directory" || test -L "$out/$directory"; then
        echo "error: Qt5Compat installed forbidden directory: $directory" >&2
        exit 1
      fi
    done
    forbidden_file="$(find "$out" \( \
      \( \( -type d -o -type l \) \( -name '*.app' -o -name '*.framework' \) \) -o \
      \( \( -type f -o -type l \) \
         \( -name '*.dylib' -o -name '*.so' -o -name '*.so.*' \) \) \
      \) -print -quit)"
    if test -n "$forbidden_file"; then
      echo "error: Qt5Compat installed a non-static target: $forbidden_file" >&2
      exit 1
    fi

    while IFS= read -r -d "" executable; do
      if file "$executable" | grep -Fq 'Mach-O'; then
        echo "error: Qt5Compat installed a host or target Mach-O tool: $executable" >&2
        exit 1
      fi
    done < <(find "$out" -type f -perm -0100 -print0)

    prl="$out/lib/libQt6Core5Compat.prl"
    test "$(grep -Foc '${qtbase-ios}/lib/libQt6Core.a' "$prl")" -eq 2
    test "$(grep -Foc '${qtbase-ios}/lib/libQt6BundledPcre2.a' "$prl")" -eq 2
    test -f "${qtbase-ios}/lib/libQt6Core.a"
    test -f "${qtbase-ios}/lib/libQt6BundledPcre2.a"

    public_pri="$out/mkspecs/modules/qt_lib_core5compat.pri"
    private_pri="$out/mkspecs/modules/qt_lib_core5compat_private.pri"
    test "$(grep -Fxc "QT.core5compat.libs = $out/lib" "$public_pri")" -eq 1
    test "$(grep -Fxc "QT.core5compat.includes = $out/include $out/include/QtCore5Compat" "$public_pri")" -eq 1
    test "$(grep -Fxc 'QT.core5compat.bins =' "$public_pri")" -eq 1
    test "$(grep -Fxc "QT.core5compat_private.libs = $out/lib" "$private_pri")" -eq 1
    test "$(grep -Fxc "QT.core5compat_private.includes = $out/include/QtCore5Compat/${packageSpec.version} $out/include/QtCore5Compat/${packageSpec.version}/QtCore5Compat" "$private_pri")" -eq 1

    metadata_paths=("$prl" "$public_pri" "$private_pri")
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
        echo "error: Qt5Compat qmake metadata refers to undeclared or native store input: $store_path" >&2
        exit 1
      fi
    done < <(grep -ahoE '/nix/store/[0-9a-z]{32}-[^/;"[:space:]]+' "''${metadata_paths[@]}" | LC_ALL=C sort -u || true)

    version_impl="$out/lib/cmake/Qt6Core5Compat/Qt6Core5CompatConfigVersionImpl.cmake"
    test "$(grep -Fxc 'set(PACKAGE_VERSION "${packageSpec.version}")' "$version_impl")" -eq 1

    feature_header="$out/include/QtCore5Compat/qtcore5compat-config.h"
    grep -Eq '^#define QT_FEATURE_big_codecs[[:space:]]+1$' "$feature_header"
    grep -Eq '^#define QT_FEATURE_codecs[[:space:]]+1$' "$feature_header"
    grep -Eq '^#define QT_FEATURE_textcodec[[:space:]]+1$' "$feature_header"
    grep -Eq '^#define QT_FEATURE_iconv[[:space:]]+-1$' "$feature_header"

    if grep -R -a -l -F ${lib.escapeShellArg (toString qt5compat.src)} "$out"; then
      echo "error: Qt5Compat output refers to its source archive" >&2
      exit 1
    fi
    for native_store_path in \
      ${lib.escapeShellArg (toString hostQt)} \
      ${lib.escapeShellArg (toString qtXcrunShim)}; do
      if grep -R -a -l -F "$native_store_path" "$out"; then
        echo "error: Qt5Compat output refers to a native build input: $native_store_path" >&2
        exit 1
      fi
    done
  '';

  passthru = {
    iosQtModule = packageSpec.name;
    iosQtStaticArchives = staticArchives;
    iosQtStandaloneObjects = standaloneObjects;
  };

  meta = {
    description = "Static Qt ${packageSpec.version} Core5Compat module for the pinned Krita iPadOS target";
    inherit (qt5compat.meta) license;
  };
}

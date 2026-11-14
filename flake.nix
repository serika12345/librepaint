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
    assert (!(builtins.pathExists ./.git || builtins.pathExists ./build)) || throw "LibrePaint must be evaluated as a Git tree (.#… in a worktree, or git+file://…), not as a path: flake or a getFlake of a plain path: that copies .git, build/ and .cache/ into the Nix store. Use builtins.getFlake \"git+file://$PWD\" or nix eval .#attr.";
    let
      system = "aarch64-darwin";
      # The KDE Frameworks are built without their unused Python bindings.
      # The Windows set leaves it out: its MinGW frameworks never pulled
      # PySide6, so the overlay would only rebuild them unchanged.
      kdeOverlay = import ./nix/kde/overlay.nix;
      pkgs = import nixpkgs {
        inherit system;
      };
      linuxSystem = "x86_64-linux";
      linuxPkgs = import nixpkgs {
        system = linuxSystem;
      };
      # The current nixpkgs KDE/Qt MinGW graph is not marked as supported.
      # Keep the platform allowance local to the Windows cross set; all native
      # package outputs retain their normal platform checks.
      windowsPkgs = import nixpkgs {
        system = linuxSystem;
        crossSystem = { config = "x86_64-w64-mingw32"; };
        config = {
          allowBroken = true;
          allowUnsupportedSystem = true;
        };
        overlays = [
          # Every entry below adjusts a MinGW target package.  The guard keeps
          # the Linux build tools of the cross build identical to stock
          # nixpkgs, so they come from the binary cache.
          (final: previous: previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
            # nixpkgs' MinGW patch series currently targets CPython 3.11.
            # Keep the target package-set aliases on that supported series so
            # indirect consumers such as OpenColorIO and PyQt do not fall
            # back to the default, unbuildable CPython 3.14 cross package.
            # Native generators continue to use buildPackages.python3.
            # PyQt6 6.12.0 is used on the target side too, to match the Qt 6.12.0 the plugins link.
            python311 = previous.python311.override {
              packageOverrides = previous.lib.composeExtensions
                (import ./nix/pyqt6-overrides.nix { lib = previous.lib; })
                (_: pythonPrevious: {
                  # pybind11's default cross policy builds its internal test
                  # extension modules when host and build CPUs have equal word
                  # sizes.  Those tests use the native interpreter headers and
                  # cannot be linked into Windows modules.  The installed
                  # headers, CMake metadata and setup hook used by OpenColorIO
                  # are independent of these test-only modules.
                  pybind11 = pythonPrevious.pybind11.override { buildTests = false; };
                });
            };
            python311Packages = final.python311.pkgs;
            python3 =
              if previous.stdenv.hostPlatform.isMinGW then
                final.python311
              else
                previous.python3;
            python3Packages =
              if previous.stdenv.hostPlatform.isMinGW then
                final.python311Packages
              else
                previous.python3Packages;

            # The Fedora-derived MinGW CPython uses winpthreads internally,
            # so every consumer of its public headers needs pthread.h as
            # well.  Boost.Python is built as part of the complete Boost
            # package and does not inherit that dependency from Python.
            boost = previous.boost.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                buildInputs = (old.buildInputs or [ ]) ++ [ previous.windows.pthreads ];
              }
            );

            # OpenColorIO deliberately converts LUT paths to UTF-16 on
            # Windows.  MSVC accepts std::wstring directly in ifstream while
            # MinGW's libstdc++ exposes the equivalent wchar_t pointer
            # overload.  Preserve Unicode path handling and pass that overload
            # the string storage explicitly.
            opencolorio = (previous.opencolorio.override {
              pythonBindings = false;
              buildApps = false;
            }).overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                # OpenColorIO's own MinGW headers are built with winpthreads.
                buildInputs = (old.buildInputs or [ ]) ++ [ previous.windows.pthreads ];
                postPatch = (old.postPatch or "") + ''
                  substituteInPlace src/OpenColorIO/transforms/FileTransform.cpp \
                    --replace-fail \
                      'Platform::filenameToUTF(filepath), mode' \
                      'Platform::filenameToUTF(filepath).c_str(), mode'
                '';
              }
            );

            # Cross-package setup hooks need a shell executable on the Linux
            # build machine.  nixpkgs' MinGW splice selects bashNonInteractive
            # for the target instead, although GNU Bash does not support a
            # plain Win32 target.  Preserve the expected multi-output package
            # shape and expose the native shell only to those build-time
            # wrappers; no shell is copied into the portable application.
            bashNonInteractive =
              if previous.stdenv.hostPlatform.isMinGW then
                previous.bashNonInteractive.overrideAttrs (_: {
                  phases = [ "installPhase" ];
                  dontUnpack = true;
                  dontConfigure = true;
                  dontBuild = true;
                  patches = [ ];
                  prePatch = "";
                  postPatch = "";
                  preInstall = "";
                  postInstall = "";
                  installPhase = ''
                    mkdir -p "$out/bin" "$dev" "$man" "$doc" "$info"
                    ln -s ${linuxPkgs.bashNonInteractive}/bin/bash "$out/bin/bash"
                    ln -s bash "$out/bin/sh"
                  '';
                })
              else
                previous.bashNonInteractive;

            # libaom's optional VMAF tuning support is an encoder quality
            # analysis aid, not part of AV1 encode/decode.  libvmaf requires
            # POSIX pthread headers and cannot be built for a plain MinGW
            # target, so retain the complete AV1 codec without that aid.
            libaom = previous.libaom.override { enableVmaf = false; };

            # Krita consumes FFTW through its C API.  The default package also
            # builds Fortran bindings, which pulls a complete MinGW Fortran
            # compiler into this cross graph without adding an application
            # capability.  Keep the threaded and SIMD-enabled C library while
            # leaving that language binding and its compiler-only OpenMP path
            # out.
            fftw = (previous.fftw.override { withDoc = false; }).overrideAttrs (old: {
              nativeBuildInputs = previous.lib.filter
                (input: !(previous.lib.hasInfix "gfortran" (previous.lib.getName input)))
                (old.nativeBuildInputs or [ ]);
              configureFlags = previous.lib.filter
                (flag: flag != "--enable-openmp")
                (old.configureFlags or [ ]) ++ [
                "--disable-fortran"
                "--disable-openmp"
                "--with-our-malloc"
                "--with-combined-threads"
              ];
              postPatch = (old.postPatch or "") + ''
                substituteInPlace threads/Makefile.am threads/Makefile.in \
                  --replace-fail \
                    '-version-info @SHARED_VERSION_INFO@' \
                    '-version-info @SHARED_VERSION_INFO@ -no-undefined'
              '';
              postInstall = (old.postInstall or "") + ''
                ln -s libfftw3.dll.a "$out/lib/libfftw3_threads.dll.a"
              '';
            });

            # LibrePaint loads JPEG XL through the library only.  The command
            # line tools, benchmarks, examples, tests and the gdk-pixbuf
            # loader are separate optional programs, so build none of them.
            libjxl = (previous.libjxl.override { enablePlugins = false; }).overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                # MinGW's wide-SIMD decoder paths fail on Windows, including
                # an AVX-512 aligned stack store. Highway orders AVX targets
                # below AVX2's bit, so this mask keeps every consumer on SSE.
                # Remove this Issue #79 workaround when the supplied codec's
                # AVX2 and AVX-512 paths pass Windows decoding and round trips,
                # including the regression tracked by Issue #68.
                NIX_CFLAGS_COMPILE = previous.lib.concatStringsSep " " [
                  (old.NIX_CFLAGS_COMPILE or "")
                  "-DHWY_DISABLED_TARGETS=(HWY_AVX2|(HWY_AVX2-1))"
                ];
                cmakeFlags = (old.cmakeFlags or [ ]) ++ [
                  "-DJPEGXL_ENABLE_TOOLS=OFF"
                  "-DJPEGXL_ENABLE_BENCHMARK=OFF"
                  "-DJPEGXL_ENABLE_EXAMPLES=OFF"
                  "-DBUILD_TESTING=OFF"
                ];
                doCheck = false;
                # Without the loader plugin libjxl has no use for gdk-pixbuf,
                # which would otherwise pull GLib into the Windows graph.
                nativeBuildInputs = previous.lib.filter
                  (input: !(previous.lib.hasPrefix "gdk-pixbuf" (previous.lib.getName input)))
                  (old.nativeBuildInputs or [ ]);
                # LibrePaint needs only the headers/library, not the test or
                # profiling inputs (see docs/architecture/DEPENDENCIES.md).
                buildInputs = previous.lib.filter
                  (input:
                    let name = previous.lib.getName input;
                    in
                    !(previous.lib.hasPrefix "gdk-pixbuf" name)
                    && !(previous.lib.hasPrefix "gperftools" name)
                    && !(previous.lib.hasPrefix "gtest" name))
                  (old.buildInputs or [ ]);
                postInstall = (old.postInstall or "") + ''
                  moveToOutput "bin/benchmark_xl.exe" "$benchmark"
                  mkdir -p "$benchmark"
                '';
              }
            );

            # nixpkgs removes every x265 archive after installation, which
            # also removes MinGW's DLL import library.  Preserve that import
            # library so libheif can link its HEVC encoder while continuing
            # to omit the redundant static codec archive.
            x265 = previous.x265.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                postPatch = (old.postPatch or "") + ''
                  substituteInPlace CMakeLists.txt \
                    --replace-fail \
                      'set_target_properties(cli PROPERTIES OUTPUT_NAME x265)' \
                      'set_target_properties(cli PROPERTIES OUTPUT_NAME x265 ARCHIVE_OUTPUT_NAME x265-cli)'
                '';
                postInstall = ''
                  rm -f "$out/lib/libx265.a"
                  ln -s "$out"/bin/*.dll "$out/lib"
                '';
              }
            );

            # Krita consumes libheif itself for HEIF and AVIF, whose encoding
            # uses libaom and decoding uses libaom and dav1d.  The example
            # programs and the gdk-pixbuf loader are outside that path.
            libheif = previous.libheif.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                buildInputs = previous.lib.filter
                  (input:
                    let name = previous.lib.getName input;
                    in name != "rav1e" && !(previous.lib.hasPrefix "gdk-pixbuf" name))
                  (old.buildInputs or [ ]);
                cmakeFlags = (old.cmakeFlags or [ ]) ++ [
                  "-DWITH_RAV1E=OFF"
                  "-DWITH_GDK_PIXBUF=OFF"
                  "-DWITH_EXAMPLES=OFF"
                ];
                postInstall = ''
                  mkdir -p "$bin" "$man"
                '';
              }
            );

            # Keep this Windows-only allowance for nixpkgs' broken MinGW target.
            # Animation export needs libx264, libx265, libtheora, libwebp and
            # built-in APNG/GIF with only scale/palette filters; drop the rest.
            # nixpkgs' MLT pins ffmpeg_8, so unify the portable application on
            # that series and ship one FFmpeg instead of two.
            ffmpeg_8 = (previous.ffmpeg_8.override {
              # OpenAPV's current MinGW build unconditionally links POSIX
              # pthreads.  Krita's animation import/export uses FFmpeg's
              # established image and video codecs, not APV encoding.
              withOpenapv = false;
              # ZVBI implements broadcast teletext and has no Win32 thread
              # backend.  It is outside Krita's animation media paths.
              withZvbi = false;
              # LibrePaint reads no Blu-ray, encrypted or streamed media, no
              # tracked music modules and no GPU compute, and its text paths
              # use Qt rather than FFmpeg's drawtext and subtitle filters.
              withBluray = false;
              withGnutls = false;
              withGmp = false;
              withOpenmpt = false;
              withRist = false;
              withSrt = false;
              withSsh = false;
              withOpencl = false;
              withVulkan = false;
              withZimg = false;
              withSoxr = false;
              withSpeex = false;
              withVidStab = false;
              withXvid = false;
              withXml2 = false;
              withOpenjpeg = false;
              withMp3lame = false;
              withFontconfig = false;
              withFreetype = false;
              withFribidi = false;
              withHarfbuzz = false;
              withSdl2 = false;
              withNetwork = false;
              buildFfplay = false;
            }).overrideAttrs (old: {
              meta = (old.meta or { }) // { broken = false; };
            });
            ffmpeg = final.ffmpeg_8;

            # giflib 6.1.3 has adopted a new Makefile, so the bundled MinGW
            # patch (written for 5.2) no longer applies.  Replace it only for
            # the Windows target; native build tools retain nixpkgs defaults.
            giflib = previous.giflib.overrideAttrs (_:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                patches = [ ./nix/windows/giflib-mingw.patch ];
              }
            );

            # The upstream exiv2 package enables its API documentation and
            # executable test suite.  Neither is part of LibrePaint's Windows
            # runtime, and both select target-side Graphviz and Python tools.
            # Keep just the Linux tools that configure the target library.
            exiv2 = previous.exiv2.overrideAttrs (old: {
              outputs = previous.lib.filter (output: output != "doc") (old.outputs or [ ]);
              nativeBuildInputs = [
                previous.buildPackages.cmake
                previous.buildPackages.gettext
                previous.buildPackages.removeReferencesTo
              ];
              buildInputs = (old.buildInputs or [ ]) ++ [ previous.gettext ];
              nativeCheckInputs = [ ];
              buildFlags = [ "all" ];
              cmakeFlags = previous.lib.filter (flag: flag != "-DEXIV2_BUILD_DOC=ON") (old.cmakeFlags or [ ]) ++ [ "-DEXIV2_BUILD_DOC=OFF" ];
              # nixpkgs removes compiler references from ELF library names.
              # MinGW installs DLLs instead, so the original glob has no
              # matches and causes the generic fixup phase to fail.
              preFixup = "";
              postFixup = ''
                # CMake's MinGW import archive is truncated to its two DLL
                # metadata members when output splitting moves it to $lib.
                # Regenerate it from the completed DLL export table, keeping
                # the Windows library itself source-built and self-contained.
                exiv2Def="$TMPDIR/exiv2.def"
                {
                  printf 'EXPORTS\n'
                  ${previous.stdenv.cc.bintools.bintools}/bin/${previous.stdenv.cc.targetPrefix}objdump \
                    -p "$out/bin/libexiv2.dll" \
                    | awk '/^.*\[[[:space:]]*[0-9]+\] [+]base\[[[:space:]]*[0-9]+\]/ && $NF != "RVA" { print $NF }'
                } > "$exiv2Def"
                ${previous.stdenv.cc.bintools.bintools}/bin/${previous.stdenv.cc.targetPrefix}dlltool \
                  --input-def "$exiv2Def" \
                  --dllname libexiv2.dll \
                  --output-lib "$lib/lib/libexiv2.dll.a"
              '';
              doCheck = false;
            });

            # LibrePaint consumes Eigen headers only.  Documentation generation
            # selects a target Graphviz executable, which cannot run while
            # cross compiling and is not needed in the packaged application.
            eigen_5 = previous.eigen_5.override { withDoc = false; };

            # The MinGW build of libwebp's optional img2webp command fails to
            # link.  Qt only needs the core decoder library; leave image
            # conversion programs out of the Windows dependency closure.
            libwebp = previous.libwebp.override {
              gifSupport = false;
              jpegSupport = false;
              pngSupport = false;
              tiffSupport = false;
            };

            # LibRaw's bundled examples include a POSIX pthread sample.  They
            # are not needed by LibrePaint and cannot compile for MinGW.
            libraw = previous.libraw.overrideAttrs (old: {
              configureFlags = (old.configureFlags or [ ]) ++ [
                "--disable-examples"
              ];
              doCheck = false;
            });

            # LibrePaint needs only the headers/library, not the test or
            # profiling inputs (see docs/architecture/DEPENDENCIES.md).
            zug = previous.zug.overrideAttrs (old: {
              # Building the tests makes configure look for catch2.
              cmakeFlags = (old.cmakeFlags or [ ]) ++ [ "-Dzug_BUILD_TESTS=OFF" ];
              doCheck = false;
              buildInputs = previous.lib.filter
                (input: !(previous.lib.hasPrefix "catch2" (previous.lib.getName input)))
                (old.buildInputs or [ ]);
            });

            # LibrePaint needs only the headers/library, not the test or
            # profiling inputs (see docs/architecture/DEPENDENCIES.md).
            lager = previous.lager.overrideAttrs (old: {
              doCheck = false;
              buildInputs = previous.lib.filter
                (input:
                  let name = previous.lib.getName input;
                  in !(previous.lib.hasPrefix "catch2" name) && !(previous.lib.hasPrefix "cereal" name))
                (old.buildInputs or [ ]);
            });

            # xsimd is a header-only dependency in LibrePaint.  Its package
            # normally compiles an extensive test suite even though MinGW
            # tests cannot run on the Linux builder.
            xsimd = previous.xsimd.overrideAttrs (old: {
              cmakeFlags = previous.lib.filter (flag: flag != "-DBUILD_TESTS=ON") (old.cmakeFlags or [ ]) ++ [ "-DBUILD_TESTS=OFF" ];
              doCheck = false;
              # LibrePaint needs only the headers/library, not the test or
              # profiling inputs (see docs/architecture/DEPENDENCIES.md).
              buildInputs = previous.lib.filter
                (input: !(previous.lib.hasPrefix "doctest" (previous.lib.getName input)))
                (old.buildInputs or [ ]);
            });

            # SQLite's Tcl extension is not needed by Qt.  Disabling it avoids
            # building a target Tcl interpreter solely for that optional module.
            sqlite = previous.sqlite.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                nativeBuildInputs = previous.lib.filter (input: previous.lib.getName input != "tcl") (old.nativeBuildInputs or [ ]);
                configureFlags = previous.lib.filter (flag: !(previous.lib.hasPrefix "--with-tcl=" flag)) (old.configureFlags or [ ]) ++ [ "--disable-tcl" ];
                doCheck = false;
              }
            );

            graphite2 = previous.graphite2.override {
              cmake = previous.buildPackages.cmake;
              pkg-config = previous.buildPackages.pkg-config;
              python3 = previous.buildPackages.python3;
            };

            # LibrePaint uses HarfBuzz only for shaping and layout.  The
            # GLib and GObject APIs are separate convenience bindings that
            # nixpkgs enables on Windows, and nothing in the Windows graph
            # selects them.
            harfbuzz = previous.harfbuzz.overrideAttrs (old: {
              buildInputs = previous.lib.filter
                (input: previous.lib.getName input != "glib")
                (old.buildInputs or [ ]);
              mesonFlags = (old.mesonFlags or [ ]) ++ [
                "-Dglib=disabled"
                "-Dgobject=disabled"
                "-Dintrospection=disabled"
              ];
            });

            # libmypaint selects GLib support only for its introspection data
            # or --with-glib.  LibrePaint uses the brush engine directly, so
            # keep the plain C library without that optional dependency.
            # GLib previously supplied libintl transitively; the translated
            # brush setting names still need gettext's dgettext.
            libmypaint = previous.libmypaint.overrideAttrs (old: {
              buildInputs = previous.lib.filter
                (input: previous.lib.getName input != "glib")
                (old.buildInputs or [ ]) ++ [ previous.gettext ];
            });

            # Fontconfig's configure probes select the Unix locale switcher
            # when cross compiling with MinGW, although that API is absent on
            # Windows.  The small upstream-compatible fallback preserves the
            # formatting path without requiring an external Windows package.
            fontconfig = previous.fontconfig.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                patches = (old.patches or [ ]) ++ [ ./nix/windows/fontconfig-mingw.patch ];
                doInstallCheck = false;
              }
            );
          })
        ];
      };
      mkLibrepaintBuildSource =
        packageSet: excludeAndroidPackage:
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
              ".direnv"
              ".git"
              ".github"
              ".gitlab"
              "AGENTS.md"
              "build-ios"
              "docs"
              "flake.lock"
              "flake.nix"
              "logs"
              "nix"
              "result"
            ])
            && !(builtins.elem relativePath [
              "packaging/macos/fonts.conf"
              "packaging/macos/macos-apptodmg.py"
              "packaging/macos/macos-bundle-launcher.c"
              "packaging/macos/macos-deploy.py"
              "packaging/macos/qt.conf"
              "scripts/platform/audit-linux-appimage.py"
              "scripts/platform/audit-macos-bundle.py"
              "scripts/platform/audit-windows-package.py"
              "scripts/platform/check-macos-release-dmg"
              "scripts/platform/check-release-assets"
              "scripts/platform/sign-android-draft-release"
              "scripts/platform/verify-android-release-apk.py"
              "scripts/tests/test_android_release_apk.py"
              "scripts/tests/test_linux_release_appimage.py"
              "scripts/tests/test_macos_release_bundle.py"
              "scripts/tests/test_windows_release_package.py"
            ])
            && !(excludeAndroidPackage && (
              relativePath == "packaging/android/apk"
              || packageSet.lib.hasPrefix "packaging/android/apk/" relativePath
            ))
            && !(packageSet.lib.hasPrefix "README" topLevel);
        };
      # Policy files use an independent source so policy-only edits preserve
      # the application compilation cache.
      mkLibrepaintPolicySource =
        packageSet:
        packageSet.lib.cleanSourceWith {
          name = "librepaint-policy-source";
          src = ./.;
          filter =
            path: _type:
            let
              relativePath = packageSet.lib.removePrefix "${toString ./.}/" (toString path);
              topLevel = builtins.head (packageSet.lib.splitString "/" relativePath);
            in
            !(builtins.elem topLevel [
              ".cache"
              ".direnv"
              ".git"
              "build"
              "build-ios"
              "logs"
              "result"
            ])
            && !(packageSet.lib.hasPrefix "result-" topLevel);
        };
      librepaintBuildSource = mkLibrepaintBuildSource pkgs false;
      linuxBuildSource = mkLibrepaintBuildSource linuxPkgs false;
      linuxAndroidCompileSource = mkLibrepaintBuildSource linuxPkgs true;
      librepaintPolicySource = mkLibrepaintPolicySource pkgs;
      linuxPolicySource = mkLibrepaintPolicySource linuxPkgs;
      linuxAppImageAppRun = import ./nix/linux/appimage-apprun.nix {
        nixAppImage = inputs.nix-appimage;
        pkgs = linuxPkgs;
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
        appImageRuntime = inputs.nix-appimage.packages.${linuxSystem}.appimage-runtimes.appimage-type2-runtime;
        appImageAppRun = linuxAppImageAppRun;
        appImageExtraFiles = "${inputs.nix-appimage}/extra-files.sh";
        auditLinuxAppImage = ./scripts/platform/audit-linux-appimage.py;
      };
      linuxAndroidPackages = import ./nix/android {
        pkgs = linuxPkgs;
        source = linuxAndroidCompileSource;
        overlays = [ kdeOverlay ];
      };
      linuxAndroidX86_64Packages = import ./nix/android {
        pkgs = linuxPkgs;
        source = linuxAndroidCompileSource;
        overlays = [ kdeOverlay ];
        androidAbi = "x86_64";
        packageName = "librepaint-android-x86_64";
      };
      windowsPackages = import ./nix/windows {
        pkgs = windowsPkgs;
        hostPkgs = linuxPkgs;
        source = linuxBuildSource;
        auditWindowsPackage = ./scripts/platform/audit-windows-package.py;
        # Keep the fork build available until Q4 stage 7 passes; the default
        # of nix/windows is the official Qt.
        useQtFork = true;
      };
      # Q4 (removing the Qt fork) uses the official Qt 6.12.0 from nixpkgs.
      # Keep it a separate package set so the fork build and its cached
      # dependencies stay untouched.
      windowsUpstreamQtPackages = import ./nix/windows {
        pkgs = windowsPkgs;
        hostPkgs = linuxPkgs;
        source = linuxBuildSource;
        auditWindowsPackage = ./scripts/platform/audit-windows-package.py;
        useQtFork = false;
        pnameSuffix = "-upstream-qt";
      };
      policyTools =
        packageSet: with packageSet; [
          bash
          (python3.withPackages (packages: [ packages.pefile ]))
        ];
      mkDocsShell =
        packageSet:
        packageSet.mkShellNoCC {
          packages = with packageSet; [ bash coreutils d2 ];

          shellHook = ''
            echo "LibrePaint documentation development shell"
            echo "  render: scripts/docs/render-architecture.sh"
          '';
        };
      mkTestShell =
        packageSet: application:
        packageSet.mkShell {
          inputsFrom = [ application ];
          packages = policyTools packageSet ++ (with packageSet; [
            ccache
            clang-tools
            cmake
            ninja
            git
            ripgrep
          ]);

          shellHook = ''
            export LIBREPAINT_TEST_SHELL=1
            export LIBREPAINT_REPO_ROOT="$PWD"
            export LIBREPAINT_BUILD_ROOT="$PWD/build"
            export LIBREPAINT_CACHE_ROOT="$PWD/.cache/librepaint"
            export CCACHE_DIR="$LIBREPAINT_CACHE_ROOT/ccache/native"
            export CCACHE_BASEDIR="$PWD"
            export CCACHE_COMPILERCHECK=content
            export CCACHE_NOHASHDIR=true
            export NINJA_STATUS='[%f/%t] '
            mkdir -p "$CCACHE_DIR"
            echo "LibrePaint native test and policy shell"
            echo "  build:  build-incremental native build [target]"
            echo "  quick:  ./scripts/verify-quick"
            echo "  target: ./scripts/run-test <target> [ctest-regex]"
            echo "  full:   ./scripts/verify"
          '';
        };
      mkGovernanceCheck =
        packageSet: policySource:
        packageSet.runCommand "librepaint-governance" {
          nativeBuildInputs = policyTools packageSet;
        } ''
          cp -R ${policySource} source
          chmod -R u+w source
          cd source
          export LIBREPAINT_TEST_SHELL=1
          ${packageSet.bash}/bin/bash ./scripts/verify-quick
          mkdir -p "$out"
          touch "$out/passed"
        '';
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
        librepaint-android-native = linuxAndroidPackages.nativeBuild;
        librepaint-android-update-baseline = linuxAndroidPackages.librepaintUpdateBaseline;
        android-application-dependencies = linuxAndroidPackages.androidApplicationDependencies;
        android-dependencies = linuxAndroidPackages.androidDependencies;
        android-kf6 = linuxAndroidPackages.androidKf6;
        android-source-dependencies = linuxAndroidPackages.androidSourceDependencies;
        qtbase-android = linuxAndroidPackages.qtbase;
        qt5compat-android = linuxAndroidPackages.qt5compat;
        qtdeclarative-android = linuxAndroidPackages.qtdeclarative;
        qtimageformats-android = linuxAndroidPackages.qtimageformats;
        qtlanguageserver-android = linuxAndroidPackages.qtlanguageserver;
        qtshadertools-android = linuxAndroidPackages.qtshadertools;
        qtsvg-android = linuxAndroidPackages.qtsvg;
        zlib-android = linuxAndroidPackages.zlib;
        librepaint-android-x86_64 = linuxAndroidX86_64Packages.librepaint;
        librepaint-android-x86_64-native = linuxAndroidX86_64Packages.nativeBuild;
        librepaint-android-x86_64-update-baseline = linuxAndroidX86_64Packages.librepaintUpdateBaseline;
        android-x86_64-application-dependencies = linuxAndroidX86_64Packages.androidApplicationDependencies;
        android-x86_64-dependencies = linuxAndroidX86_64Packages.androidDependencies;
        android-x86_64-kf6 = linuxAndroidX86_64Packages.androidKf6;
        android-x86_64-source-dependencies = linuxAndroidX86_64Packages.androidSourceDependencies;
        qtbase-android-x86_64 = linuxAndroidX86_64Packages.qtbase;
        qt5compat-android-x86_64 = linuxAndroidX86_64Packages.qt5compat;
        qtdeclarative-android-x86_64 = linuxAndroidX86_64Packages.qtdeclarative;
        qtimageformats-android-x86_64 = linuxAndroidX86_64Packages.qtimageformats;
        qtlanguageserver-android-x86_64 = linuxAndroidX86_64Packages.qtlanguageserver;
        qtshadertools-android-x86_64 = linuxAndroidX86_64Packages.qtshadertools;
        qtsvg-android-x86_64 = linuxAndroidX86_64Packages.qtsvg;
        zlib-android-x86_64 = linuxAndroidX86_64Packages.zlib;
        librepaint-windows = windowsPackages.librepaint;
        librepaint-windows-archive = windowsPackages.librepaintArchive;
        librepaint-windows-unwrapped = windowsPackages.librepaintUnwrapped;
        windows-dependencies = windowsPackages.windowsDependencies;
        librepaint-windows-upstream-qt = windowsUpstreamQtPackages.librepaint;
        librepaint-windows-upstream-qt-archive = windowsUpstreamQtPackages.librepaintArchive;
        librepaint-windows-upstream-qt-unwrapped = windowsUpstreamQtPackages.librepaintUnwrapped;
        windows-upstream-qt-dependencies = windowsUpstreamQtPackages.dependencyInputs;
        pyqt6-612-check =
          let
            py = (import ./nix/pyqt6.nix {
              inherit (linuxPkgs) lib;
              python = linuxPkgs.python3;
            }).withPackages (p: [ p.pyqt6 p.pyqt6-sip p.sip ]);
          in
          linuxPkgs.runCommand "pyqt6-612-check" {
            nativeBuildInputs = [ py ];
            QT_QPA_PLATFORM = "offscreen";
          } ''
            set -e
            python -c '
            from PyQt6 import QtCore, QtGui, QtWidgets, QtXml, QtQml, QtSvg
            assert QtCore.PYQT_VERSION_STR == "6.12.0", QtCore.PYQT_VERSION_STR
            assert QtCore.QT_VERSION_STR == "6.12.0", QtCore.QT_VERSION_STR
            print("PyQt6", QtCore.PYQT_VERSION_STR)
            print("Qt", QtCore.QT_VERSION_STR)
            ' > versions.txt
            python ${./cmake/modules/FindPyQt6.py} > find-pyqt6.out
            grep -q 'pyqt_version_str:6.12.0' find-pyqt6.out
            python -c "import sipbuild; print(sipbuild.__file__)" > sipbuild.out
            python ${./cmake/modules/FindSIP.py} > find-sip.out
            grep -q 'sip_version_str:6.17.3' find-sip.out
            mkdir -p "$out"
            cp versions.txt find-pyqt6.out sipbuild.out find-sip.out "$out/"
          '';
      };

      checks.${system} = {
        governance = mkGovernanceCheck pkgs librepaintPolicySource;
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
        governance = mkGovernanceCheck linuxPkgs linuxPolicySource;
        librepaint-linux = linuxPackages.librepaint;
        linux-dependencies = linuxPackages.linuxDependencies;
      };

      devShells.${system} = {
        default = macosPackages.devShell;
        docs = mkDocsShell pkgs;
        librepaint-macos = macosPackages.devShell;
        librepaint-test = mkTestShell pkgs macosPackages.librepaint;
        test = mkTestShell pkgs macosPackages.librepaint;

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
            echo "iOS development shell"
            echo "  host check: packaging/ios/scripts/check-host.sh"
            echo "  smoke test: packaging/ios/scripts/build-smoke.sh device"
          '';
        };

        librepaint-ios-incremental = iosPackages.krita-ios-incremental-env;
        krita-ios-incremental = iosPackages.krita-ios-incremental-env;
      };

      devShells.${linuxSystem} = {
        default = linuxPackages.devShell;
        docs = mkDocsShell linuxPkgs;
        android-release = import ./nix/android/release-shell.nix { pkgs = linuxPkgs; };
        librepaint-linux = linuxPackages.devShell;
        librepaint-test = mkTestShell linuxPkgs linuxPackages.librepaintUnwrapped;
        test = mkTestShell linuxPkgs linuxPackages.librepaintUnwrapped;
        librepaint-android = linuxAndroidPackages.devShell;
        librepaint-android-x86_64 = linuxAndroidX86_64Packages.devShell;
        librepaint-windows = windowsPackages.devShell;
        librepaint-windows-upstream-qt = windowsUpstreamQtPackages.devShell;
      };

      formatter.${system} = pkgs.nixfmt;
      formatter.${linuxSystem} = linuxPkgs.nixfmt;
    };
}

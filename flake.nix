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
          (final: previous: {
            # nixpkgs' MinGW patch series currently targets CPython 3.11.
            # Keep the target package-set aliases on that supported series so
            # indirect consumers such as OpenColorIO and PyQt do not fall
            # back to the default, unbuildable CPython 3.14 cross package.
            # Native generators continue to use buildPackages.python3.
            python311 = previous.python311.override {
              packageOverrides = _: pythonPrevious: {
                # pybind11's default cross policy builds its internal test
                # extension modules when host and build CPUs have equal word
                # sizes.  Those tests use the native interpreter headers and
                # cannot be linked into Windows modules.  The installed
                # headers, CMake metadata and setup hook used by OpenColorIO
                # are independent of these test-only modules.
                pybind11 = pythonPrevious.pybind11.override { buildTests = false; };
              };
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
            opencolorio = previous.opencolorio.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                # FindPython must run a native interpreter while compiling and
                # linking the extension against the ABI-matched Windows
                # interpreter.  Its public MinGW headers use winpthreads.
                buildInputs = (old.buildInputs or [ ]) ++ [ previous.windows.pthreads ];
                cmakeFlags = (old.cmakeFlags or [ ]) ++ [
                  "-DPython_EXECUTABLE=${previous.buildPackages.python311}/bin/python3.11"
                  "-DPython_INCLUDE_DIR=${final.python311}/include/python3.11"
                  "-DPython_LIBRARY=${final.python311}/lib/python3.11/config-3.11/libpython3.11.dll.a"
                ];
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

            # nixpkgs enables mbedTLS' pthread-backed locking on every
            # platform, but does not add the MinGW pthread implementation to
            # the Windows build.  Keep its thread-safe crypto configuration
            # and provide winpthreads, which also makes the public
            # mbedtls/threading.h usable by consumers such as librist.
            mbedtls = previous.mbedtls.overrideAttrs (old: {
              propagatedBuildInputs = (old.propagatedBuildInputs or [ ]) ++ [
                previous.windows.pthreads
              ];
              NIX_LDFLAGS = (old.NIX_LDFLAGS or "") + " -lpthread";
            });

            # librist otherwise assumes its bundled Win32 pthread and time
            # shims even when MinGW winpthreads is present.  Select the
            # supported MinGW pthread path so clock_gettime is detected and
            # linked once.  FFmpeg consumes the library; librist's standalone
            # utilities and its executable tests are not application runtime
            # components.
            librist = previous.librist.overrideAttrs (old: {
              mesonFlags = (old.mesonFlags or [ ]) ++ [
                "-Dhave_mingw_pthreads=true"
                "-Dtest=false"
                "-Dbuilt_tools=false"
              ];
            });

            # Gnulib's cross probe sees the MinGW C runtime's internal
            # wcwidth symbol but <wchar.h> does not declare a callable public
            # function.  Use Gnulib's own iswprint fallback on Windows; this
            # only supplies the terminal-column helper and keeps the complete
            # GnuTLS protocol and codec support intact.
            gnutls = previous.gnutls.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                postPatch = (old.postPatch or "") + ''
                  sed -i 's/^#if HAVE_WCWIDTH$/#if HAVE_WCWIDTH \&\& !defined _WIN32/' \
                    gl/wcwidth.c src/gl/wcwidth.c
                '';
              }
            );

            # curl's MinGW executable records the build shell selected by
            # libtool even though the Windows program never invokes it.  The
            # reference crosses from a target output back to the Linux build
            # machine and violates Nix's output separation.  Scrub that
            # non-runtime build string while retaining curl and libcurl.
            curl = previous.curl.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                nativeBuildInputs = (old.nativeBuildInputs or [ ]) ++ [
                  linuxPkgs.removeReferencesTo
                ];
                postFixup = (old.postFixup or "") + ''
                  if [ -d "$bin" ]; then
                    remove-references-to -t ${linuxPkgs.bashNonInteractive} "$bin"/bin/*
                  fi
                '';
              }
            );

            # Zix enables its executable test suite during every build.  Those
            # target binaries are not part of Sord/Lilv/MLT at runtime and
            # cannot be run in the Linux cross environment, so build the full
            # Windows library without constructing the test executables.
            zix = previous.zix.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                buildInputs = (old.buildInputs or [ ]) ++ [
                  previous.windows.pthreads
                ];
                mesonFlags = (old.mesonFlags or [ ]) ++ [ "-Dtests=disabled" ];
              }
            );

            # The cross pkg-config wrapper already prefixes xproto's
            # includedir, while libX11's configure script prefixes it again.
            # Pass the concrete target include directory to keep Qt's OpenGL
            # compatibility dependency buildable on MinGW.
            libx11 = previous.libx11.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                configureFlags = (old.configureFlags or [ ]) ++ [
                  "--with-keysymdefdir=${previous.xorgproto}/include/X11"
                ];
                NIX_CFLAGS_COMPILE = (old.NIX_CFLAGS_COMPILE or "")
                  + " -Wno-error=pointer-to-int-cast";
                postPatch = (old.postPatch or "") + ''
                  sed -i '/#include <config.h>/a #include <stdint.h>' \
                    modules/im/ximcp/imDefIc.c
                  substituteInPlace modules/im/ximcp/imDefIc.c \
                    --replace-fail \
                      '(XIMStyle)p->value' \
                      '(XIMStyle)(uintptr_t)p->value'
                  sed -i '/#include "Xlibint.h"/a #ifdef _WIN32\n#undef CreateWindow\n#endif' \
                    src/xcms/cmsCmap.c
                '';
              }
            );

            # libdv serializes its decoder tables with pthread mutexes on all
            # targets.  Supply MinGW's pthread implementation so MLT retains
            # DV import and export support in the Windows package.
            libdv = previous.libdv.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                buildInputs = (old.buildInputs or [ ]) ++ [
                  previous.windows.pthreads
                ];
                NIX_LDFLAGS = (old.NIX_LDFLAGS or "") + " -lpthread";
                postPatch = (old.postPatch or "") + ''
                  substituteInPlace libdv/encode.c \
                    --replace-fail \
                      'swab( pcm[j]+i, audio.data + (i*2+j)*channels, 2);' \
                      'swab((char *)(pcm[j] + i), (char *)(audio.data + (i*2+j)*channels), 2);'
                  substituteInPlace libdv/headers.c \
                    --replace-fail \
                      'if (localtime_r(datetime, &now_t) != NULL )' \
                      $'#ifdef _WIN32\n\tif (localtime_s(&now_t, datetime) == 0)\n#else\n\tif (localtime_r(datetime, &now_t) != NULL)\n#endif'
                  substituteInPlace libdv/Makefile.am libdv/Makefile.in \
                    --replace-fail \
                      'libdv_la_LDFLAGS = -version-info 4:3:0' \
                      $'libdv_la_LDFLAGS = -version-info 4:3:0 -no-undefined\nlibdv_la_LIBADD = $(PTHREAD_LIBS)'
                '';
                buildPhase = ''
                  runHook preBuild
                  make -C libdv libdv.la
                  runHook postBuild
                '';
                installPhase = ''
                  runHook preInstall
                  make -C libdv install-libLTLIBRARIES install-pkgincludeHEADERS
                  mkdir -p "$out/lib/pkgconfig"
                  install -m 0644 libdv.pc "$out/lib/pkgconfig/libdv.pc"
                  runHook postInstall
                '';
              }
            );

            # libsamplerate passes a complete module-definition file to
            # libtool's -export-symbols option.  Current libtool then wraps
            # that file in another EXPORTS section, producing a malformed
            # MinGW DLL definition.  Supply the symbol-list format expected
            # by libtool; the exported API remains identical, including the
            # newer clone and channel-query entry points.
            libsamplerate = previous.libsamplerate.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                postPatch = (old.postPatch or "") + ''
                  sed -i \
                    -e '/^EXPORTS/d' \
                    -e 's/[[:space:]]*@[^[:space:]]*[[:space:]]*$//' \
                    Win32/libsamplerate-0.def
                '';
              }
            );

            # xvidcore's MinGW install supplies a DLL import archive, but the
            # generic nixpkgs cleanup removes it together with the static
            # archive.  Keep the import archive under the conventional
            # lib-prefixed name expected by FFmpeg's linker probe.
            xvidcore = previous.xvidcore.overrideAttrs (_:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                buildInputs = [ previous.windows.pthreads ];
                postInstall = ''
                  rm -f "$out/lib/xvidcore.a"
                  ln -s xvidcore.dll.a "$out/lib/libxvidcore.dll.a"
                '';
              }
            );

            # libjxl obtains the GDK Pixbuf loader directory from the target
            # pkg-config file.  That value is an absolute path to the already
            # built gdk-pixbuf package, so CMake otherwise attempts to modify
            # a read-only dependency during installation.  Install the loader
            # in libjxl's own output, where the bundle can collect it together
            # with the complete JPEG XL codec.
            libjxl = previous.libjxl.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                postPatch = (old.postPatch or "") + ''
                  substituteInPlace plugins/gdk-pixbuf/CMakeLists.txt \
                    --replace-fail \
                      'execute_process(COMMAND ''${PKG_CONFIG_EXECUTABLE} gdk-pixbuf-2.0 --variable gdk_pixbuf_moduledir --define-variable=prefix=''${CMAKE_INSTALL_PREFIX} OUTPUT_VARIABLE GDK_PIXBUF_MODULEDIR OUTPUT_STRIP_TRAILING_WHITESPACE)' \
                      'set(GDK_PIXBUF_MODULEDIR "'$out'/${previous.gdk-pixbuf.moduleDir}")'
                '';
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

            # libheif's optional example programs use WebP's mux API but the
            # upstream CMake targets omit libwebpmux when cross-linking for
            # Windows.  Krita consumes libheif itself (whose HEIF/AVIF codec
            # support links successfully), not these sample command-line
            # programs.  Keep the complete library and GDK loader while
            # leaving the non-runtime examples out of the target build.
            libheif = previous.libheif.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                cmakeFlags = (old.cmakeFlags or [ ]) ++ [
                  "-DWITH_EXAMPLES=OFF"
                ];
                postInstall = ''
                  mkdir -p "$bin" "$man"
                '';
              }
            );

            # Vamp's window template uses M_PI before the implementation file
            # defines its Visual C++ fallback.  GCC 15 diagnoses that ordering
            # while parsing the template.  Put the same fallback beside the
            # <cmath> include so the complete host SDK remains available to
            # Rubber Band and MLT on Windows.
            vamp-plugin-sdk = previous.vamp-plugin-sdk.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                postPatch = (old.postPatch or "") + ''
                  substituteInPlace src/vamp-hostsdk/Window.h \
                    --replace-fail \
                      '#include <cstdlib>' \
                      $'#include <cstdlib>\n\n#ifndef M_PI\n#define M_PI 3.14159265358979232846\n#endif'
                '';
                postBuild = (old.postBuild or "") + ''
                  ln -s vamp-simple-host.exe host/vamp-simple-host
                  ln -s vamp-rdf-template-generator.exe \
                    rdf/generator/vamp-rdf-template-generator
                '';
              }
            );

            # PortAudio's JACK backend is a Unix integration and creates a
            # dependency cycle with JACK's Windows PortAudio adapter.  Keep
            # the native Windows backends and make that PortAudio available
            # to JACK so its Windows adapter and driver are built as well.
            portaudio = previous.portaudio.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                buildInputs = previous.lib.filter
                  (input: !(previous.lib.hasPrefix "libjack" (previous.lib.toLower (previous.lib.getName input))))
                  (old.buildInputs or [ ]);
              }
            );

            # JACK recognises "win32", not "windows", as its Waf platform
            # name.  The latter silently selects no platform and leaves the
            # Windows sources and include list unset.  Select the recognised
            # platform and defensively initialise the process-object includes
            # to JACK's Win32 implementation.
            libjack2 = previous.libjack2.overrideAttrs (old:
              previous.lib.optionalAttrs previous.stdenv.hostPlatform.isMinGW {
                buildInputs = (old.buildInputs or [ ]) ++ [
                  previous.tre
                  previous.windows.pthreads
                  final.portaudio
                ];
                wafConfigureFlags = map
                  (flag: if flag == "--platform=windows" then "--platform=win32" else flag)
                  (old.wafConfigureFlags or [ ]);
                postPatch = (old.postPatch or "") + ''
                  sed -i \
                    "/^    process = bld(features=/a\\    env_includes = ['../windows', '../windows/portaudio']" \
                    common/wscript
                  substituteInPlace windows/JackWinNamedPipeServerChannel.cpp \
                    --replace-fail \
                      'snprintf(fServerName, sizeof(fServerName), server_name);' \
                      'snprintf(fServerName, sizeof(fServerName), "%s", server_name);'
                '';
              }
            );

            # On 64-bit Windows libevent correctly represents SOCKET as an
            # intptr-sized evutil_socket_t, while Unbound's compatibility API
            # retains an int descriptor.  Route callbacks through Unbound's
            # existing adapter on Windows as well, and give the adapter the
            # ABI expected by libevent before narrowing to Unbound's API.
            unbound =
              if previous.stdenv.hostPlatform.isMinGW then
                previous.unbound.overrideAttrs (old: {
                  postPatch = (old.postPatch or "") + ''
                    sed -i \
                      -e 's/^#if UB_EV_TIMEOUT /#if defined(UB_ON_WINDOWS) || UB_EV_TIMEOUT /' \
                      -e 's/void my_ ## C (int fd/void my_ ## C (evutil_socket_t fd/' \
                      -e 's/{ (C)(fd, UB_EV_BITS(bits), arg); }/{ (C)((int)fd, UB_EV_BITS(bits), arg); }/' \
                      -e 's/))(int, short, void\*)$/))(evutil_socket_t, short, void*)/' \
                      util/ub_event.c util/ub_event_pluggable.c
                    sed -i \
                      -e '/UB_EV_BITS_CB(comm_point_udp_ancil_callback)/i #if defined(AF_INET6) \&\& defined(IPV6_PKTINFO) \&\& defined(HAVE_RECVMSG)' \
                      -e '/UB_EV_BITS_CB(comm_point_udp_ancil_callback)/a #endif' \
                      -e '/else if(cb == comm_point_udp_ancil_callback)/i #if defined(AF_INET6) \&\& defined(IPV6_PKTINFO) \&\& defined(HAVE_RECVMSG)' \
                      -e '/return my_comm_point_udp_ancil_callback;/a #endif' \
                      util/ub_event.c util/ub_event_pluggable.c
                    sed -i \
                      -e 's/, fd, bits, cb, arg)/, fd, NATIVE_BITS(bits), NATIVE_BITS_CB(cb), arg)/' \
                      -e 's/, fd, cb, arg)/, fd, NATIVE_BITS_CB(cb), arg)/' \
                      util/ub_event.c
                  '';
                })
              else
                previous.unbound;

            # NSPR chooses a bare `nsinstall` command solely from the MinGW
            # host tuple.  During a Linux-to-Windows cross build the helper is
            # a freshly built Linux executable under config/, so retain
            # NSPR's normal relative path to that build tool.  GCC 15 defaults
            # to C23, where `bool` is a keyword, so rename NSPR's two legacy
            # local variables without changing their WinSock representation.
            nspr = previous.nspr.overrideAttrs (old: {
              postPatch = (old.postPatch or "") + ''
                sed -i 's/\<bool\>/boolValue/g' nspr/pr/src/io/prmapopt.c
                sed -i 's/PRInt32 rvSent;/DWORD rvSent;/g' nspr/pr/src/io/prsocket.c
                sed -i \
                  -e 's/^  unsigned long bytes, key;$/  DWORD bytes;\n  ULONG_PTR key;/' \
                  -e 's/^  int bytes;$/  DWORD bytes;/' \
                  -e 's/^  PRUint32 bytes;$/  DWORD bytes;/' \
                  -e 's/^  PRInt32 bytes;$/  DWORD bytes;/' \
                  nspr/pr/src/md/windows/ntio.c
                substituteInPlace nspr/pr/src/md/windows/ntio.c \
                  --replace-fail \
'InterlockedCompareExchange((PVOID*)&desc->outcome, (PVOID)mwstatus,
                                     (PVOID)PR_MW_PENDING) ==
          (PVOID)PR_MW_PENDING' \
'InterlockedCompareExchange((LONG volatile*)&desc->outcome, (LONG)mwstatus,
                                     (LONG)PR_MW_PENDING) ==
          (LONG)PR_MW_PENDING' \
                  --replace-fail \
'PRInt32 _PR_MD_SOCKETAVAILABLE(PRFileDesc* fd) {
  PRInt32 result;

  if (ioctlsocket(fd->secret->md.osfd, FIONREAD, &result) < 0) {
    PR_SetError(PR_BAD_DESCRIPTOR_ERROR, WSAGetLastError());
    return -1;
  }
  return result;
}' \
'PRInt32 _PR_MD_SOCKETAVAILABLE(PRFileDesc* fd) {
  u_long result;

  if (ioctlsocket(fd->secret->md.osfd, FIONREAD, &result) < 0) {
    PR_SetError(PR_BAD_DESCRIPTOR_ERROR, WSAGetLastError());
    return -1;
  }
  return (PRInt32)result;
}'
                substituteInPlace nspr/pr/src/md/windows/ntmisc.c \
                  --replace-fail \
'  if (exitCode != NULL &&
      GetExitCodeProcess(process->md.handle, exitCode) == FALSE) {
    PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
    return PR_FAILURE;
  }' \
'  if (exitCode != NULL) {
    DWORD nativeExitCode;
    if (GetExitCodeProcess(process->md.handle, &nativeExitCode) == FALSE) {
      PR_SetError(PR_UNKNOWN_ERROR, GetLastError());
      return PR_FAILURE;
    }
    *exitCode = (PRInt32)nativeExitCode;
  }'
                substituteInPlace nspr/pr/src/md/windows/ntthread.c \
                  --replace-fail \
'PRInt32 _PR_MD_GETTHREADAFFINITYMASK(PRThread* thread, PRUint32* mask) {
  PRInt32 rv, system_mask;

  rv = GetProcessAffinityMask(GetCurrentProcess(), mask, &system_mask);

  return rv ? 0 : -1;
}' \
'PRInt32 _PR_MD_GETTHREADAFFINITYMASK(PRThread* thread, PRUint32* mask) {
  DWORD_PTR processMask, systemMask;
  PRInt32 rv;

  rv = GetProcessAffinityMask(GetCurrentProcess(), &processMask, &systemMask);
  if (rv) {
    *mask = (PRUint32)processMask;
  }

  return rv ? 0 : -1;
}'
              '';
              postConfigure = (old.postConfigure or "") + ''
                sed -i \
                  's|^NSINSTALL[[:space:]]*=[[:space:]]*nsinstall$|NSINSTALL = $(MOD_DEPTH)/config/$(OBJDIR_NAME)/nsinstall|' \
                  config/autoconf.mk
                sed -i \
                  's|^RC[[:space:]]*=.*$|RC = ${previous.stdenv.cc.targetPrefix}windres|' \
                  config/autoconf.mk
              '';
            });

            # NSS' gyp files use the literal `win` for both native Windows
            # and MinGW.  nixpkgs passes `Windows`, which silently selects
            # the Unix source and header branches.  Select NSS' existing
            # MinGW path and use GNU import-library names and flags.
            nss = previous.nss.overrideAttrs (old: {
              postPatch = (old.postPatch or "") + ''
                substituteInPlace coreconf/config.gypi \
                  --replace-fail \
                    "'nspr_libs%': ['libnspr4.lib', 'libplc4.lib', 'libplds4.lib']" \
                    "'nspr_libs%': ['-lplds4', '-lplc4', '-lnspr4']" \
                  --replace-fail \
                    "'zlib_libs%': []" \
                    "'zlib_libs%': ['-lz']" \
                  --replace-fail \
                    "              '_WINDOWS'," \
                    "              '_WINDOWS', 'WIN32', 'WIN64', '_AMD64_',"
                substituteInPlace coreconf/config.gypi \
                  --replace-fail \
                    "[ '_type==\"shared_library\"', {
        'product_dir': '<(nss_dist_obj_dir)/lib'
      }" \
                    "[ '_type==\"shared_library\"', {
        'product_dir': '<(nss_dist_obj_dir)/lib',
        'conditions': [
          [ 'OS==\"win\" and cc_use_gnu_ld==1', {
            'product_extension': 'dll',
            'product_prefix': \"\",
          }],
        ],
      }" \
                  --replace-fail \
"          [ 'cc_use_gnu_ld==1', {
            'ldflags': [
              '-Wl,--version-script,<(INTERMEDIATE_DIR)/out.>(mapfile)',
            ],
          }],
          [ 'cc_use_gnu_ld!=1 and OS==\"win\"', {" \
"          [ 'cc_use_gnu_ld==1 and OS!=\"win\"', {
            'ldflags': [
              '-Wl,--version-script,<(INTERMEDIATE_DIR)/out.>(mapfile)',
            ],
          }],
          [ 'OS==\"win\"', {"
                sed -i \
                  -e "/'-W3',/d" \
                  -e "/'-w44267',/d" \
                  -e "/'-w44244',/d" \
                  -e "/'-w44018',/d" \
                  -e "/'-w44312',/d" \
                  -e "/'cflags': \['-WX'\]/d" \
                  coreconf/config.gypi
                sed -i 's/!rindex(mod->dllName/!strrchr(mod->dllName/' \
                  lib/pk11wrap/pk11load.c
                sed -i 's/\<bool\>/boolValue/g' lib/dev/ckhelper.c
                sed -i \
                  -e 's/\<false\>/falseValue/g' \
                  -e 's/\<true\>/trueValue/g' \
                  -e 's/\<bool\>/boolValue/g' \
                  cmd/pwdecrypt/pwdecrypt.c
                sed -i 's/<Windows\.h>/<windows.h>/' nss-tool/common/util.cc
                sed -i "s/'advapi32\.lib'/'-ladvapi32'/" \
                  nss-tool/nss_tool.gyp
                sed -i \
                  's/OS=="linux" or OS=="android" or OS=="dragonfly"/OS=="win" or OS=="linux" or OS=="android" or OS=="dragonfly"/g' \
                  lib/freebl/freebl.gyp
                while IFS= read -r -d "" defFile; do
                  sed -i -e '/^;/d' -e 's/[[:space:]]*;.*$//' "$defFile"
                done < <(find . -name '*.def' -print0)
              '';
              preBuild = (old.preBuild or "") + ''
                mkdir -p .gyp-mingw
                cp -r \
                  ${previous.buildPackages.python3Packages.gyp}/${previous.buildPackages.python3.sitePackages}/gyp \
                  .gyp-mingw/
                chmod -R u+w .gyp-mingw
                substituteInPlace .gyp-mingw/gyp/generator/ninja.py \
                  --replace-fail \
"      elif self.flavor == 'win' and ext == 'rc':
        command = 'rc'
        obj_ext = '.res'
        has_rc_source = True
      else:" \
"      elif self.flavor == 'win' and ext == 'rc':
        command = 'rc'
        obj_ext = '.res'
        has_rc_source = True
      elif ext == 'def':
        outputs.append(self.GypPathToNinja(source))
        continue
      else:"
                export PYTHONPATH="$PWD/.gyp-mingw:$PYTHONPATH"
              '';
              buildPhase = builtins.replaceStrings
                [ "-DOS=Windows" ]
                [ "-DOS=win -Dcc_use_gnu_ld=1 -Dno_zdefs=1 -Ddisable_werror=1 -Dsign_libs=0" ]
                old.buildPhase;
            });

            # The MinGW build produces RNNoise's DLL but intentionally skips
            # its Unix-style command-line demo.  nixpkgs' unconditional copy
            # of that demo is packaging-only; MLT consumes librnnoise itself.
            rnnoise = previous.rnnoise.overrideAttrs (_: {
              postInstall = "";
            });

            # FFmpeg supports MinGW, but nixpkgs marks the 64-bit target as
            # broken pending its own cross-build issue.  Keep the allowance
            # local to this Windows package set so the actual build can supply
            # Krita's bundled ffmpeg/ffprobe and MLT backend.
            ffmpeg = (previous.ffmpeg.override {
              # OpenAPV's current MinGW build unconditionally links POSIX
              # pthreads.  Krita's animation import/export uses FFmpeg's
              # established image and video codecs, not APV encoding.
              withOpenapv = false;
              # ZVBI implements broadcast teletext and has no Win32 thread
              # backend.  It is outside Krita's animation media paths.
              withZvbi = false;
            }).overrideAttrs (old: {
              meta = (old.meta or { }) // { broken = false; };
            });

            # Rubber Band's JNI binding is unrelated to MLT's native audio
            # filter and selects a target JDK that does not exist for MinGW.
            # Keep the C++ time-stretching library while omitting standalone
            # plugin formats and command-line programs from this dependency.
            rubberband = previous.rubberband.overrideAttrs (old: {
              nativeBuildInputs = [
                previous.buildPackages.meson
                previous.buildPackages.ninja
                previous.buildPackages.pkg-config
              ];
              buildInputs = (old.buildInputs or [ ]) ++ [
                previous.windows.pthreads
              ];
              mesonFlags = (old.mesonFlags or [ ]) ++ [
                "-Djni=disabled"
                "-Dladspa=disabled"
                "-Dlv2=disabled"
                "-Dvamp=disabled"
                "-Dcmdline=disabled"
              ];
            });

            # RtAudio 5's JACK backend is a Unix implementation whose pthread
            # condition variables cannot be combined with the Win32 critical
            # sections selected by its Windows platform layer.  Use RtAudio's
            # native WASAPI backend; JACK itself remains available separately
            # with its WinMME and PortAudio drivers.
            rtaudio = previous.rtaudio.override { jackSupport = false; };

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

            # xsimd is a header-only dependency in LibrePaint.  Its package
            # normally compiles an extensive test suite even though MinGW
            # tests cannot run on the Linux builder.
            xsimd = previous.xsimd.overrideAttrs (old: {
              cmakeFlags = previous.lib.filter (flag: flag != "-DBUILD_TESTS=ON") (old.cmakeFlags or [ ]) ++ [ "-DBUILD_TESTS=OFF" ];
              doCheck = false;
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

            # Qt's Linux host tools bring this in through systemd.  Its
            # hardware-backed integration suite is unrelated to the Windows
            # cross build and cannot affect the resulting application.
            tpm2-tss = previous.tpm2-tss.overrideAttrs (_: {
              doCheck = false;
            });

            # GLib's Nix expression selects target-side Python tooling when
            # cross compiling.  These tools execute during the build, so use
            # the Linux build-platform variants and avoid a target Python
            # closure in the Windows runtime graph.
            glib = (previous.glib.override {
              docutils = previous.buildPackages.docutils;
              gettext = previous.buildPackages.gettext;
              libxslt = previous.buildPackages.libxslt;
              meson = previous.buildPackages.meson;
              ninja = previous.buildPackages.ninja;
              pkg-config = previous.buildPackages.pkg-config;
              perl = previous.buildPackages.perl;
              python3 = previous.buildPackages.python3;
              python3Packages = previous.buildPackages.python3Packages;
            }).overrideAttrs (old: {
              # Sysprof captures Linux performance data and its implementation
              # requires endian.h.  GLib keeps this optional input enabled on
              # Windows in nixpkgs, but LibrePaint does not use the feature.
              buildInputs = previous.lib.filter (input: input != previous.libsysprof-capture) (old.buildInputs or [ ]);
              mesonFlags = (old.mesonFlags or [ ]) ++ [ "-Dsysprof=disabled" ];
              # The argument above is the Linux Gettext because msgfmt runs
              # during the build.  GLib also links libintl at runtime, so add
              # the Windows library separately for Meson and dependants.
              propagatedBuildInputs = (old.propagatedBuildInputs or [ ]) ++ [ previous.gettext ];
            });

            graphite2 = previous.graphite2.override {
              cmake = previous.buildPackages.cmake;
              pkg-config = previous.buildPackages.pkg-config;
              python3 = previous.buildPackages.python3;
            };

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
      windowsPackages = import ./nix/windows {
        pkgs = windowsPkgs;
        source = linuxBuildSource;
      };
      mkDocsShell =
        packageSet:
        packageSet.mkShellNoCC {
          packages = with packageSet; [
            bash
            coreutils
            d2
            diffutils
            findutils
            git
            librsvg
            lychee
            markdownlint-cli2
            ripgrep
            shellcheck
          ];

          shellHook = ''
            echo "LibrePaint documentation development shell"
            echo "  validate: scripts/docs/check-architecture.sh"
            echo "  render:   scripts/docs/render-architecture.sh"
          '';
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
        librepaint-windows = windowsPackages.librepaint;
        librepaint-windows-archive = windowsPackages.librepaintArchive;
        librepaint-windows-unwrapped = windowsPackages.librepaintUnwrapped;
        windows-dependencies = windowsPackages.windowsDependencies;
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
        docs = mkDocsShell pkgs;
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
        librepaint-linux = linuxPackages.devShell;
        librepaint-android = linuxAndroidPackages.devShell;
      };

      formatter.${system} = pkgs.nixfmt;
      formatter.${linuxSystem} = linuxPkgs.nixfmt;
    };
}

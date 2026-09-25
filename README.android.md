# Building LibrePaint for Android

LibrePaint builds Android application packages for `arm64-v8a` and `x86_64`
on an x86_64 Linux host. Technical target, package, and output names containing
`krita` remain where the application and packaging interfaces require their
stable identifiers.

The Nix build owns the complete toolchain and dependency graph. It builds Qt
6.11.1, KDE Frameworks 6.28.0, the remaining C and C++ dependencies,
LibrePaint, and the packaging layer as separate derivations. The Android SDK
and NDK are pinned vendor inputs. Normal builds use only fixed Nix inputs and
the recorded Gradle dependency cache; they do not download dependencies.

## Reproducible packages

Build the ARM64 APK and Android App Bundle on x86_64 Linux with:

```shell
nix build path:.#librepaint-android
```

The output contains:

```text
result/LibrePaint-arm64-v8a.apk
result/LibrePaint-arm64-v8a.aab
```

Build the x86_64 diagnostic package with:

```shell
nix build path:.#librepaint-android-x86_64
```

The output contains `LibrePaint-x86_64.apk` and `LibrePaint-x86_64.aab`.
Each package build automatically checks its ABI, ELF machine type and 16 KiB
load-segment alignment, the single shared C++ runtime, Qt 6 and KF6 runtime
contents, Android plugins, application assets, manifest, minimum SDK, and
target SDK. The audit rejects Qt 5, KF5, and the old Qt 5 Java namespace.

The dependency layers are independently addressable when diagnosing or
warming a binary cache:

```shell
nix build path:.#android-source-dependencies
nix build path:.#qtbase-android
nix build path:.#android-kf6
nix build path:.#android-application-dependencies
nix build path:.#android-dependencies
```

The corresponding x86_64 outputs are
`android-x86_64-source-dependencies`, `qtbase-android-x86_64`,
`android-x86_64-kf6`, `android-x86_64-application-dependencies`, and
`android-x86_64-dependencies`. Application source changes preserve the Qt,
KF6, and external dependency derivations.
Binary caches may substitute an identical derivation; the build definition
does not depend on artifacts produced by another LibrePaint or Krita build.

## Fixed toolchain contract

Both ABIs use the same versions and settings:

- Qt `6.11.1` and KDE Frameworks `6.28.0`
- Android NDK `27.3.13750724`
- compile SDK and target SDK `35`
- Android SDK Build-Tools `35.0.0`
- minimum SDK `28`
- JDK `17`, Gradle `8.13`, and Android Gradle Plugin `8.12`
- shared LLVM C++ runtime and C++17 application language mode

The fixed dependency sources and hashes live in `nix/android/`, the iOS
dependency manifests reused for source metadata, `flake.lock`, and
`nix/android/gradle-deps.json`. Updating a version also requires updating its
fixed hash and successfully rebuilding both ABI graphs.
Pinned dependency-recipe patches and configuration flags are build metadata.
No library, Qt installation, APK, or other output from a LibrePaint or Krita
build is an input to this graph.

Update the Gradle response lock with the metadata-only Android project before
the final product build:

```shell
gradle_update_script="$(nix build --no-link --print-out-paths \
  path:.#librepaint-android.gradleDepsUpdate)"
"$gradle_update_script"
```

This project resolves the same Android Gradle Plugin and AndroidX graph and
assembles a minimal release APK. It does not compile LibrePaint, Qt, KF6, or
the C/C++ dependency graph.

## Incremental source development

After the repository test shell has been created, use the shared development
environment described in `docs/architecture/DEVELOPMENT.md`. The standard
entry points select a persistent Ninja tree and compiler cache:

```shell
build-incremental android configure
build-incremental android build krita
build-incremental android package-product
build-incremental android-x86_64 configure
build-incremental android-x86_64 build krita
```

`android` selects `arm64-v8a`; `android-x86_64` selects the Waydroid and
x86_64-device profile. The equivalent ABI-specific development shells are:

```shell
nix develop path:.#librepaint-android
nix develop path:.#librepaint-android-x86_64
```

CMake creates the application as a Qt 6 Android executable and finalizes it
with `qt_finalize_executable()`. `QT_ANDROID_PACKAGE_SOURCE_DIR` connects
`packaging/android/apk` to `androiddeployqt`. The generated targets
`create-apk-krita` and `create-aab-krita` produce the APK and AAB without the
Qt 5 ECM deployment layer.

## Android Qt Test execution

Build one selected Qt Test as an audited unsigned Android APK:

```shell
build-incremental android-x86_64 package-test KisCurveOptionModelTest
```

Run it on the one connected Android target, or pass an explicit ADB serial:

```shell
adb connect <waydroid-or-device-address>
build-incremental android-x86_64 run-test KisCurveOptionModelTest [adb-serial]
```

The runner validates the device ABI, signs a runtime copy with a test-only key
created below the repository cache, installs that copy, starts the Qt 6
Activity, collects xUnit XML and logcat output, stops the application, and
removes the test package. The unsigned audited APK remains unchanged. Results
are stored below the selected persistent build tree in
`test-results/<target>/`.

Use the ARM64 profile with a physical ARM64 device in the same way:

```shell
build-incremental android run-test KisCurveOptionModelTest [adb-serial]
```

## Signing and installation

Nix produces unsigned release packages so release signing remains outside the
reproducible compilation derivation. Sign an APK with the authorized project
key before publication or installation, then install it with the pinned
platform tools:

```shell
adb install -r <signed-apk>
adb logcat
```

The application identifier is `org.krita`; the native application library
retains the stable `krita` target name.

## Qt accessibility compatibility

The Android Activity sets `QT_ANDROID_DISABLE_ACCESSIBILITY=1` before Qt
initialization. Qt 6.11.1 can otherwise abort when an accessibility query is
waiting for the Qt event loop while a second OpenGL-backed top-level surface,
such as the new-document dialog, is created. Keep this compatibility setting
until the minimum Qt version supports Android accessibility together with
multi-window surface creation, then remove the setting and repeat the physical
device dialog and accessibility checks.

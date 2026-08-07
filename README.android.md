# Building LibrePaint for Android

The Android target produces APK files. Technical target, package, and output
names containing `krita` are retained where the build system requires them for
compatibility.

This repository does not bootstrap Android dependencies. A local build
therefore requires an Android-compatible Qt/KDE/dependency prefix prepared
separately; this document starts at the LibrePaint CMake configure step.

## Required Android toolchain

The APK project currently declares the following Android versions in
`packaging/android/apk/build.gradle`:

- Android NDK `27.3.13750724`
- compile SDK and target SDK `35`
- Android SDK Build-Tools `35.0.0`
- minimum SDK `24`

Install the command-line tools (or Android Studio), accept the SDK licences,
and install the matching packages. For example, using the `sdkmanager` from
your SDK installation:

```shell
<android-sdk>/cmdline-tools/latest/bin/sdkmanager --licenses
<android-sdk>/cmdline-tools/latest/bin/sdkmanager \
    "platform-tools" \
    "platforms;android-35" \
    "build-tools;35.0.0" \
    "ndk;27.3.13750724"
```

You also need CMake, Ninja, a JDK compatible with the Android Gradle plugin,
Qt for the selected Android ABI, and Android builds of all dependencies that
LibrePaint's CMake configure reports as required. The repository does not
currently provide a clean-machine bootstrap for that dependency prefix.

## Configure and build

The root CMake project exports the technical target `krita` through
`ECMAndroidDeployQt.cmake`. It passes `packaging/android/apk` to Qt's
`androiddeployqt`, which creates the Gradle project used to assemble an APK.
Do not run the Gradle wrapper in the source directory before that generated
project exists: values such as the Qt Android source directory and compile SDK
are supplied by `androiddeployqt`.

The following is the general shape of the CMake configure. Replace every
angle-bracketed path with the matching local path and add any options required
by the prepared dependency prefix:

```shell
cmake -S <librepaint-source> -B <android-build> -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=<android-ndk>/build/cmake/android.toolchain.cmake \
    -DANDROID_SDK_ROOT=<android-sdk> \
    -DANDROID_ABI=arm64-v8a \
    -DANDROID_PLATFORM=android-24 \
    -DANDROID_STL=c++_shared \
    -DCMAKE_PREFIX_PATH=<android-dependency-prefix> \
    -DCMAKE_FIND_ROOT_PATH=<android-dependency-prefix> \
    -DCMAKE_INSTALL_PREFIX=<android-install-prefix> \
    -DQTANDROID_EXPORTED_TARGET=krita \
    -DANDROID_APK_DIR=<librepaint-source>/packaging/android/apk \
    -DBUILD_TESTING=OFF

cmake --build <android-build> --target install --parallel
```

The exact APK target name is created by the installed ECM/Qt Android tooling.
List the generated targets, then build the `create-apk` target that it reports:

```shell
cmake --build <android-build> --target help
cmake --build <android-build> --target <reported-create-apk-target>
```

If you need to invoke Gradle separately, do so only inside the project emitted
by `androiddeployqt`. Confirm the available tasks first:

```shell
cd <androiddeployqt-generated-project>
./gradlew tasks --all
```

The checked-in Gradle configuration supports `assembleDebug`,
`assembleNightly`, and `assembleRelease`. It reads `ANDROID_ABI`,
`ANDROID_NDK_ROOT` (or `ANDROID_NDK_HOME`), and `KRITA_INSTALL_PREFIX` when
copying the installed native libraries and resources. Release signing is not
configured in this repository.

Gradle writes APKs under the generated project's
`build/outputs/apk/<variant>/` directory.

## Install and inspect

Install a generated APK on a connected device with `adb` from the Android SDK
platform tools:

```shell
adb install -r <path-to-generated.apk>
```

For runtime diagnostics, use:

```shell
adb logcat
```

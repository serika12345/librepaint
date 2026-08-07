# Local AppImage build

These scripts build a local LibrePaint AppImage from this checkout. They do not
download, clone, publish, sign, or update dependencies.

Prepare an architecture-compatible dependency prefix separately and set its
absolute path in `LIBREPAINT_DEPS_PATH`. The prefix must contain the Qt and KDE
Frameworks development files, Python/PyQt, the multimedia and image libraries
used by the build, translations and MIME data, and `linuxdeployqt` plus its
AppImage runtime. The legacy name `KRITA_DEPS_PATH` is accepted only as a
fallback when `LIBREPAINT_DEPS_PATH` is unset.

The host must be a Debian-compatible Linux system with the normal C/C++ build
toolchain and `bash`, `cmake`, `dpkg`, `git`, `nproc`, `patchelf`, `realpath`,
and `rsync`. Use absolute paths and run these two commands from the repository
root:

```sh
LIBREPAINT_DEPS_PATH=/absolute/path/to/dependencies packaging/linux/appimage/build-krita.sh /absolute/path/to/appimage-work "$PWD"
LIBREPAINT_DEPS_PATH=/absolute/path/to/dependencies packaging/linux/appimage/build-image.sh /absolute/path/to/appimage-work "$PWD"
```

The second command writes `LibrePaint-<version>-<revision>-<architecture>.AppImage`
to the AppImage work directory.

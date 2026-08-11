{
  nixAppImage,
  pkgs,
}:

# Keep nix-appimage's userns-chroot AppRun intact for distribution targets.  A
# locally built AppImage can additionally use its identical, already verified
# Nix closure from the host.  That preserves NixOS' /run/opengl-driver links,
# whose store targets are otherwise intentionally hidden by userns-chroot.
pkgs.pkgsStatic.runCommandCC "librepaint-appimage-apprun" { } ''
        mkdir -p "$out/mountroot"
        cp "${nixAppImage}/appruns/userns-chroot/main.c" main.c

        substituteInPlace main.c \
      --replace-fail \
        'void child_main(char** argv)' \
        'static void exec_host_entrypoint_if_available(char** argv)
  {
      if (getenv("NIX_APPIMAGE_FORCE_CHROOT")) {
        return;
      }

      const char* entrypoint = strprintf("%s/entrypoint", appdir);
      char exe[PATH_MAX + 1];
      ssize_t exe_size = readlink(entrypoint, exe, PATH_MAX);
      free((void*) entrypoint);
      if (exe_size < 0) {
        return;
      }
      exe[exe_size] = 0;

      if (access(exe, X_OK) == 0) {
        execv(exe, argv);
        fprintf(stderr, "%s: cannot exec host entrypoint %s: %s\\n", argv0, exe, strerror(errno));
      }
  }

  void child_main(char** argv)'

    substituteInPlace main.c \
      --replace-fail \
        '// get uid, gid before going to new namespace' \
        'exec_host_entrypoint_if_available(argv);

    // get uid, gid before going to new namespace'

    $CC main.c -o "$out/AppRun"
''

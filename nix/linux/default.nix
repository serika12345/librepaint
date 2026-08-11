{
  mkLinuxAppImage,
  pkgs,
  source,
}:

let
  inherit (pkgs) lib;
  krita = import ./krita.nix {
    inherit pkgs source;
  };
  librepaintAppImage = import ./appimage.nix {
    inherit mkLinuxAppImage;
    librepaint = krita.librepaint;
  };

  # This derivation deliberately contains no LibrePaint source input.  Its
  # manifest is scanned by Nix, which makes each member a runtime reference and
  # therefore a GC-safe, cacheable dependency closure.
  linuxDependencies =
    pkgs.runCommand "librepaint-linux-dependencies"
      {
        passthru.linuxDependencyMembers = krita.librepaint.linuxDependencyMembers;
      }
      ''
        mkdir -p "$out/nix-support"
        printf '%s\n' ${lib.escapeShellArgs (map toString krita.librepaint.linuxDependencyMembers)} \
          > "$out/nix-support/linux-dependency-members"
      '';

  devShell = pkgs.mkShell {
    inputsFrom = [ krita.librepaintUnwrapped ];
    packages = [ pkgs.nixfmt ];

    shellHook = ''
      echo "LibrePaint Linux development shell"
      echo "  dependencies: nix build .#linux-dependencies"
      echo "  build:        nix build .#librepaint-linux"
      echo "  AppImage:     nix build .#librepaint-linux-appimage"
    '';
  };
in
assert lib.assertMsg (
  builtins.length krita.librepaint.linuxDependencyMembers
  == builtins.length (lib.unique (map toString krita.librepaint.linuxDependencyMembers))
) "Linux dependency aggregate must contain unique direct inputs";
{
  inherit devShell librepaintAppImage linuxDependencies;
  inherit (krita) librepaint librepaintGmic librepaintUnwrapped;
}

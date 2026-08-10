{
  pkgs,
  source,
}:

let
  inherit (pkgs) lib;
  frameworks = import ./kde-frameworks.nix { inherit pkgs; };
  kseexpr = pkgs.kseexpr.override {
    kdePackages = pkgs.kdePackages // {
      inherit (frameworks) ki18n;
    };
  };
  librepaint = import ./krita.nix {
    inherit
      pkgs
      source
      frameworks
      kseexpr
      ;
  };
  macosDependencyMembers = librepaint.macosDependencyMembers;
  # Keep a source-independent reference to every direct input used by the
  # macOS app recipe. The manifest makes the members part of this output's
  # runtime closure without trying to merge their overlapping directory trees.
  macosDependencies =
    pkgs.runCommand "librepaint-macos-dependencies"
      {
        passthru.macosDependencyMembers = macosDependencyMembers;
      }
      ''
        mkdir -p "$out/nix-support"
        printf '%s\n' ${lib.escapeShellArgs (map toString macosDependencyMembers)} \
          > "$out/nix-support/macos-dependency-members"
      '';
  devShell = pkgs.mkShell {
    inputsFrom = [ librepaint ];
    packages = [ pkgs.nixfmt ];

    shellHook = ''
      echo "LibrePaint macOS development shell"
      echo "  build: nix build .#librepaint-macos"
    '';
  };
in
assert lib.assertMsg (
  builtins.length macosDependencyMembers == 57
  && builtins.length (lib.unique (map toString macosDependencyMembers)) == 57
) "macOS dependency aggregate must contain exactly 57 unique direct inputs";
{
  inherit
    devShell
    frameworks
    kseexpr
    librepaint
    macosDependencies
    ;
}

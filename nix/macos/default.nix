{
  pkgs,
  source,
}:

let
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
  devShell = pkgs.mkShell {
    inputsFrom = [ librepaint ];
    packages = [ pkgs.nixfmt ];

    shellHook = ''
      echo "LibrePaint macOS development shell"
      echo "  build: nix build .#librepaint-macos"
    '';
  };
in
{
  inherit
    devShell
    frameworks
    kseexpr
    librepaint
    ;
}

{ lib }:

let
  applicationSource = src: import ../application-source.nix { inherit lib src; };
  base = applicationSource ./application-source/base;
  sdkChange = applicationSource ./application-source/sdk-change;
  paintChange = applicationSource ./application-source/paint-change;
in
assert lib.assertMsg (
  base.outPath == sdkChange.outPath
) "SDK tooling and consumer changes must preserve the application source hash";
assert lib.assertMsg (
  base.outPath != paintChange.outPath
) "Painting implementation changes must invalidate the application source hash";
true

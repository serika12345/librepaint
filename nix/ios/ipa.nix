{
  coreutils,
  findutils,
  krita-ios-app,
  lib,
  python3,
  stdenvNoCC,
  unzip,
  zip,
}:

stdenvNoCC.mkDerivation {
  pname = "krita-ios-unsigned-ipa";
  inherit (krita-ios-app) version;

  dontUnpack = true;
  strictDeps = true;
  nativeBuildInputs = [
    coreutils
    findutils
    python3
    unzip
    zip
  ];

  installPhase = ''
    runHook preInstall

    stage="$NIX_BUILD_TOP/ipa-stage"
    mkdir -p "$stage/Payload" "$out"
    cp -R ${krita-ios-app}/krita.app "$stage/Payload/krita.app"

    # Nix store paths are intentionally immutable (0555 directories, 0444
    # data files).  Those modes must not leak into the IPA: importers such as
    # LiveContainer restore ZIP permissions before patching and signing the
    # app, and cannot recursively clean up a read-only bundle afterwards.
    # The same helper is used by the host-side incremental deployment path so
    # both producers enforce one bundle and archive contract.
    ${python3}/bin/python3 ${./ipa-permissions.py} \
      normalize-app "$stage/Payload/krita.app"

    chmod 0755 "$stage/Payload"
    find "$stage" -exec touch -h -t 198001010000 {} +

    entry_list="$NIX_BUILD_TOP/ipa-entries"
    (
      cd "$stage"
      {
        find Payload -type d -exec printf '%s/\n' {} \;
        find Payload -type f -print
      } | LC_ALL=C sort > "$entry_list"
      ZIPOPT= ZIP= zip -nw -MM -X -9 -q \
        "$out/LibrePaint-iPad-unsigned.ipa" -@ < "$entry_list"
    )

    runHook postInstall
  '';

  doInstallCheck = true;
  installCheckPhase = ''
        runHook preInstallCheck

        ipa="$out/LibrePaint-iPad-unsigned.ipa"
        unzip -tq "$ipa"

        ${python3}/bin/python3 ${./ipa-permissions.py} check-ipa "$ipa" \
          --staged-app "$NIX_BUILD_TOP/ipa-stage/Payload/krita.app"

        ${python3}/bin/python3 - "$ipa" <<'PY'
    import hashlib
    import sys
    import zipfile

    ipa = sys.argv[1]
    with zipfile.ZipFile(ipa) as archive:
        entries = archive.infolist()
        names = [entry.filename for entry in entries]
        required = {
            "Payload/krita.app/Info.plist",
            "Payload/krita.app/krita",
            "Payload/krita.app/share/krita/actions/krita.action",
            "Payload/krita.app/share/krita/bundles/Krita_4_Default_Resources.bundle",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/CC-BY-3.0.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/CC-BY-SA-3.0.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/CC-BY-SA-4.0.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/CC0-1.0.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/GPL-2.0-or-later.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/GPL-3.0-only.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/GPL-3.0-or-later.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/LGPL-2.0-or-later.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/LGPL-3.0-only.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/LGPL-3.0-or-later.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/LicenseRef-ICC-License.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/default-resource-bundle-licenses.json",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/non-code-licenses.md",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/qtbase-icc-attribution.json",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/retained-functional-assets.md",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/static-dependency-resources.json",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/white-brand-assets.json",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/bundles/README",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/profiles/elles-icc-profiles/plain-text-README-for-elles-well-behaved-icc-profiles.txt",
            "Payload/krita.app/share/doc/librepaint/non-code-licenses/profiles/ycbcr-icc-profiles/LICENSE-PROFILES.txt",
        }
        missing = sorted(required.difference(names))
        if missing:
            raise SystemExit(f"IPA is missing required entries: {missing}")
        if names != sorted(names):
            raise SystemExit("IPA central directory is not byte-order sorted")
        if any(entry.date_time != (1980, 1, 1, 0, 0, 0) for entry in entries):
            raise SystemExit("IPA contains a non-normalized ZIP timestamp")

    with open(ipa, "rb") as handle:
        print(f"unsigned IPA sha256: {hashlib.sha256(handle.read()).hexdigest()}")
    PY

        runHook postInstallCheck
  '';

  passthru = {
    app = krita-ios-app;
    bundleIdentifier = krita-ios-app.bundleIdentifier;
    unsigned = true;
  };

  meta = {
    description = "Deterministic unsigned LibrePaint IPA for arm64 iPadOS";
    license = lib.licenses.gpl3Plus;
    platforms = [ "aarch64-darwin" ];
  };
}

{ fetchurl }:

# Versioned upstream release; the mutable continuous asset cannot be refetched
# reliably after it is replaced. The raw static executable is prepended to SquashFS.
fetchurl {
  name = "appimage-type2-runtime-20251108-x86_64";
  url = "https://github.com/AppImage/type2-runtime/releases/download/20251108/runtime-x86_64";
  hash = "sha256-L8qLRDySUQ8Ug6iD9gBhrQm0a5eLJjHIB82HOkfsJg0=";
}

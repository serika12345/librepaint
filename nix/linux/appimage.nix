{
  librepaint,
  mkLinuxAppImage,
}:

# This is intentionally a final packaging stage.  mkAppImage embeds the
# complete Nix closure of the wrapped LibrePaint executable in a Type-2
# AppImage, leaving linux-dependencies source-independent and cacheable.
mkLinuxAppImage {
  program = "${librepaint}/bin/LibrePaint";
  pname = "LibrePaint";
  name = "LibrePaint-${librepaint.version}-x86_64.AppImage";
}

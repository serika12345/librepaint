#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs an Xorg server with the dummy video driver in a Docker container, so
# the xcb display path can be checked against a real X server while the host
# stays in its Wayland session. The only input device of the server is the
# given evdev node (the virtual pen tablet of virtual_tablet.py), read by the
# libinput driver inside the container. The server adds the device from udev
# as a desktop X server does, and its input class matches only that node; the
# container can open no other input node, so the host's keyboard and mouse
# stay with the host session. The container uses an empty image and the
# host's Nix store read-only, so nothing is downloaded and nothing outside
# OUT_DIR changes apart from the image librepaint-empty:1 and the container
# while it runs.
#
# Usage: xorg-server.sh start OUT_DIR /dev/input/eventN
#        xorg-server.sh stop
#   start writes OUT_DIR/xorg.conf, OUT_DIR/xauth and OUT_DIR/Xorg.log and
#   returns when the server answers on DISPLAY :XORG_DISPLAY (default 99).
#   The tools come from build/perf/xorg-env (nix-build xorg-env.nix).
set -euo pipefail

name=librepaint-xorg
image=librepaint-empty:1
display=${XORG_DISPLAY:-99}
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)

case "${1:-}" in
  stop)
    docker rm -f "$name" > /dev/null 2>&1 || true
    exit 0
    ;;
  start)
    [ "$#" -eq 3 ] || { sed -n '16,20p' "$0" >&2; exit 2; }
    ;;
  *)
    sed -n '16,20p' "$0" >&2
    exit 2
    ;;
esac
mkdir -p "$2"
out=$(cd "$2" && pwd)
device=$3
env=$(realpath "$root/build/perf/xorg-env")
[ -x "$env/bin/Xorg" ] || { echo "build $root/build/perf/xorg-env with nix-build $here/xorg-env.nix first" >&2; exit 1; }

docker image inspect "$image" > /dev/null 2>&1 || tar -cf - --files-from /dev/null | docker import - "$image" > /dev/null
docker rm -f "$name" > /dev/null 2>&1 || true

# xkbcomp runs through /bin/sh, the only file the empty image needs.
mkdir -p "$out/bin"
ln -sfn "$(realpath "$env/bin/busybox")" "$out/bin/sh"

cat > "$out/xorg.conf" << CONF
Section "ServerFlags"
  Option "DontVTSwitch" "true"
EndSection
Section "InputClass"
  Identifier "pen"
  MatchDevicePath "$device"
  Driver "libinput"
EndSection
Section "Device"
  Identifier "dummy"
  Driver "dummy"
  VideoRam 256000
EndSection
Section "Monitor"
  Identifier "monitor"
  HorizSync 5.0-1000.0
  VertRefresh 5.0-200.0
  Modeline "1920x1080" 148.50 1920 2008 2052 2200 1080 1084 1089 1125 +hsync +vsync
EndSection
Section "Screen"
  Identifier "screen"
  Device "dummy"
  Monitor "monitor"
  DefaultDepth 24
  SubSection "Display"
    Depth 24
    Modes "1920x1080"
    Virtual 1920 1080
  EndSubSection
EndSection
Section "ServerLayout"
  Identifier "layout"
  Screen "screen"
EndSection
CONF

rm -f "$out/xauth"; : > "$out/xauth"
"$env/bin/xauth" -q -f "$out/xauth" add ":$display" . "$(od -An -N16 -tx1 /dev/urandom | tr -d ' \n')"

# --network host shares the abstract X socket with host clients, so the socket
# file can stay in the container's /tmp. --uts host keeps the hostname that
# the xauth entry names. /run/udev lets libinput read the device's udev
# properties. Mesa at /run/opengl-driver gives the server GLX.
docker run -d --rm --name "$name" --network host --uts host \
  -v /nix/store:/nix/store:ro -v "$out:$out" -v "$out/bin:/bin:ro" \
  -v /run/udev:/run/udev:ro -v "$(realpath "$env/opengl-driver"):/run/opengl-driver:ro" --device "$device" --tmpfs /tmp -e "PATH=$env/bin" \
  "$image" "$env/bin/Xorg" ":$display" -config "$out/xorg.conf" -modulepath "$env/lib/xorg/modules" \
  -logfile "$out/Xorg.log" -nolisten tcp -noreset -novtswitch -sharevts -keeptty -auth "$out/xauth" > /dev/null

for _ in $(seq 50); do
  DISPLAY=:$display XAUTHORITY=$out/xauth "$env/bin/xdotool" getdisplaygeometry > /dev/null 2>&1 && exit 0
  docker inspect "$name" > /dev/null 2>&1 || break
  sleep 0.2
done
echo "Xorg did not start; see $out/Xorg.log" >&2
docker rm -f "$name" > /dev/null 2>&1 || true
exit 1

#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs KWin, the KDE Plasma compositor, on a 1920x1080 virtual output in a
# Docker container, so the native Wayland display path can be checked on
# KWin while the host stays in its GNOME session. KWin's virtual backend reads
# no input devices; input reaches it only through its fake input protocol
# (kwin_click.py), so a pen tablet cannot be checked on it. KWin renders with
# the host's GPU render nodes, which draw without driving a display, and runs
# as the calling user. The container uses an empty image and the host's Nix
# store read-only, so nothing is downloaded and nothing outside OUT_DIR
# changes apart from the image librepaint-empty:1 and the container while it
# runs.
#
# Usage: kwin-server.sh start OUT_DIR
#        kwin-server.sh stop
#   start returns when the Wayland socket OUT_DIR/runtime/wayland-lp and
#   KWin's D-Bus session bus OUT_DIR/runtime/bus answer. KWin's log is
#   OUT_DIR/kwin.log. The tools come from build/perf/kwin-env
#   (nix-build kwin-env.nix).
set -euo pipefail

name=librepaint-kwin
image=librepaint-empty:1
here=$(cd "$(dirname "$0")" && pwd)
root=$(cd "$here/../../.." && pwd)

case "${1:-}" in
  stop)
    docker rm -f "$name" > /dev/null 2>&1 || true
    exit 0
    ;;
  start)
    [ "$#" -eq 2 ] || { sed -n '15,20p' "$0" >&2; exit 2; }
    ;;
  *)
    sed -n '15,20p' "$0" >&2
    exit 2
    ;;
esac
mkdir -p "$2"
out=$(cd "$2" && pwd)
env=$(realpath "$root/build/perf/kwin-env")
[ -x "$env/bin/kwin_wayland" ] || { echo "build $root/build/perf/kwin-env with nix-build $here/kwin-env.nix first" >&2; exit 1; }

docker image inspect "$image" > /dev/null 2>&1 || tar -cf - --files-from /dev/null | docker import - "$image" > /dev/null
docker rm -f "$name" > /dev/null 2>&1 || true

rm -rf "$out/runtime" "$out/home"
mkdir -p "$out/runtime" "$out/home" "$out/bin"
chmod 700 "$out/runtime"
ln -sfn "$(realpath "$env/bin/busybox")" "$out/bin/sh"
# dbus-daemon looks up the user that runs it.
mkdir -p "$out/etc"
printf '%s:x:%s:%s::%s:/bin/sh\n' "$(id -un)" "$(id -u)" "$(id -g)" "$out/home" > "$out/etc/passwd"
printf '%s:x:%s:\n' "$(id -gn)" "$(id -g)" > "$out/etc/group"
cat > "$out/session.sh" << RUN
#!/bin/sh
$env/bin/dbus-daemon --config-file=$env/share/dbus-1/session.conf --address=unix:path=$out/runtime/bus --nofork --nopidfile &
sleep 1
export DBUS_SESSION_BUS_ADDRESS=unix:path=$out/runtime/bus
exec $env/bin/kwin_wayland --virtual --width 1920 --height 1080 --socket wayland-lp
RUN
chmod +x "$out/session.sh"

render=()
for node in /dev/dri/renderD*; do render+=(--device "$node"); done
docker run -d --name "$name" --network host --user "$(id -u):$(id -g)" --group-add "$(stat -c %g /dev/dri/renderD128)" \
  -v /nix/store:/nix/store:ro -v "$out:$out" -v "$out/bin:/bin:ro" -v "$out/etc/passwd:/etc/passwd:ro" -v "$out/etc/group:/etc/group:ro" -v /run/udev:/run/udev:ro \
  -v "$(realpath "$env/opengl-driver"):/run/opengl-driver:ro" "${render[@]}" --tmpfs /tmp \
  -e "PATH=$env/bin" -e QT_FORCE_STDERR_LOGGING=1 -e "HOME=$out/home" -e "XDG_RUNTIME_DIR=$out/runtime" -e "XDG_DATA_DIRS=$env/share" \
  -e "QT_PLUGIN_PATH=$env/lib/qt-6/plugins" -e "FONTCONFIG_FILE=$env/fonts.conf" \
  "$image" "$out/session.sh" > /dev/null

for _ in $(seq 50); do
  if [ -S "$out/runtime/wayland-lp" ] && DBUS_SESSION_BUS_ADDRESS=unix:path=$out/runtime/bus \
    "$env/bin/dbus-send" --session --print-reply --dest=org.kde.KWin /KWin org.freedesktop.DBus.Peer.Ping > /dev/null 2>&1; then
    docker logs "$name" > "$out/kwin.log" 2>&1 || true
    exit 0
  fi
  docker inspect "$name" > /dev/null 2>&1 || break
  sleep 0.2
done
docker logs "$name" > "$out/kwin.log" 2>&1 || true
echo "KWin did not start; see $out/kwin.log" >&2
docker rm -f "$name" > /dev/null 2>&1 || true
exit 1

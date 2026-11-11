#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Automatic Q4 check on the Windows tablet from one entry point.

The latency measurement (`run.py`) replays a trajectory and measures the
delay. This check replaces the trajectory with a fixed sequence of short
steps that answer the questions the Q4 stages otherwise ask a person to
confirm on the device:

- clipboard formats and pixels for an image with transparency;
- the window title while two documents are open and the tabs are switched;
- the Japanese keyboard layout (00000411) with Ctrl+Tab and the modifier
  state after the keys are released;
- blank and unchanged frames while the window is resized and the settings
  dialog is opened and closed;
- the stage 2/3 pen checks that synthetic pen input can reach.

`run-remote.sh` runs the steps on the tablet and writes evidence files
(`facts.json`, `probe.csv`, screen captures, `flicker.csv`, ...). This tool
reads those files and writes one JSON with a pass/fail verdict per item and
the path of every piece of evidence. The flicker verdict is relative to the
fork build in the same invocation, because a fixed frame count does not
describe what the user sees; the other items have absolute limits from the
scenario file.

Usage:
  check.py --scenario scripts/perf/scenarios/q4-e2e.json \
           --hosts build/perf/hosts.json --label q4-e2e --only windows \
           [--targets q4-e2e-stage3,q4-e2e-stage10,q4-e2e-fork]
  check.py --analyze-only RUN_ROOT
"""

import argparse
import csv
import datetime
import json
import os
import re
import struct
import subprocess
import sys
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
PEN = os.path.join(HERE, "pen-latency")
sys.path.insert(0, PEN)
import display_check  # noqa: E402

RUN_REMOTE = os.path.join(PEN, "run-remote.sh")
ITEMS = (
    "clipboard",
    "tabs",
    "keyboard",
    "flicker",
    "pen.enter_leave",
    "pen.edge_line",
    "pen.single_line",
    "pen.long_press",
    "pen.dot",
    "pen.nonclient_return",
    "pen.first_line",
)
PEN_ITEMS = ITEMS[4:]
NOT_REPRODUCIBLE = (
    {
        "id": "pen.proximity",
        "title": "実物のペンの近接",
        "reason": "合成ペン入力は近接の状態を作れるが、実物のペンの近接の距離とタイミングは再現できない。",
    },
    {
        "id": "touch.finger",
        "title": "指のタッチ",
        "reason": "指のタッチとペンへの持ち替えは合成マウス／ペンでは再現できない。",
    },
    {
        "id": "pen.two_finger",
        "title": "2 本指のズームとパン",
        "reason": "2 本指のジェスチャーは合成ポインター API で再現できない。",
    },
)


def read_text(path):
    with open(path, encoding="utf-8", errors="replace") as handle:
        return handle.read()


def parse_key_values(text):
    values = {}
    for line in text.splitlines():
        key, separator, value = line.strip().partition("=")
        if separator:
            values[key.strip()] = value.strip()
    return values


def read_formats(path):
    if not os.path.exists(path):
        return []
    with open(path, encoding="utf-8", errors="replace", newline="") as handle:
        return list(csv.DictReader(handle))


def _paeth(a, b, c):
    p = a + b - c
    pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
    if pa <= pb and pa <= pc:
        return a
    if pb <= pc:
        return b
    return c


def read_png_rgba(path):
    """Read an 8-bit RGB or RGBA, non-interlaced PNG as RGBA bytes."""
    with open(path, "rb") as handle:
        data = handle.read()
    if data[:8] != b"\x89PNG\r\n\x1a\n":
        raise ValueError("%s is not a PNG" % path)
    position = 8
    width = height = channels = None
    compressed = []
    while position < len(data):
        length = struct.unpack(">I", data[position:position + 4])[0]
        kind = data[position + 4:position + 8]
        chunk = data[position + 8:position + 8 + length]
        position += 12 + length
        if kind == b"IHDR":
            width, height, depth, color, _, _, interlace = struct.unpack(">IIBBBBB", chunk)
            if depth != 8 or color not in (2, 6) or interlace != 0:
                raise ValueError("unsupported PNG: depth %d colour %d interlace %d" % (depth, color, interlace))
            channels = 3 if color == 2 else 4
        elif kind == b"IDAT":
            compressed.append(chunk)
        elif kind == b"IEND":
            break
    if width is None:
        raise ValueError("%s has no IHDR" % path)
    raw = zlib.decompress(b"".join(compressed))
    stride = width * channels
    out = bytearray(width * height * 4)
    previous = bytearray(stride)
    position = 0
    for y in range(height):
        filter_type = raw[position]
        position += 1
        row = bytearray(raw[position:position + stride])
        position += stride
        if filter_type == 1:
            for x in range(channels, stride):
                row[x] = (row[x] + row[x - channels]) & 0xFF
        elif filter_type == 2:
            for x in range(stride):
                row[x] = (row[x] + previous[x]) & 0xFF
        elif filter_type == 3:
            for x in range(stride):
                left = row[x - channels] if x >= channels else 0
                row[x] = (row[x] + ((left + previous[x]) >> 1)) & 0xFF
        elif filter_type == 4:
            for x in range(stride):
                left = row[x - channels] if x >= channels else 0
                upper = previous[x]
                upper_left = previous[x - channels] if x >= channels else 0
                row[x] = (row[x] + _paeth(left, upper, upper_left)) & 0xFF
        elif filter_type != 0:
            raise ValueError("unsupported PNG filter %d" % filter_type)
        target = 4 * y * width
        for x in range(width):
            source = x * channels
            out[target + 4 * x] = row[source]
            out[target + 4 * x + 1] = row[source + 1]
            out[target + 4 * x + 2] = row[source + 2]
            out[target + 4 * x + 3] = row[source + 3] if channels == 4 else 255
        previous = row
    return width, height, out


def compare_pngs(first, second, tolerance=1):
    width_a, height_a, pixels_a = read_png_rgba(first)
    width_b, height_b, pixels_b = read_png_rgba(second)
    result = {
        "width": [width_a, width_b],
        "height": [height_a, height_b],
        "pixels": width_a * height_a,
        "mismatched": 0,
        "max_channel_diff": 0,
        "alpha_mismatched": 0,
    }
    if (width_a, height_a) != (width_b, height_b):
        result["mismatched"] = result["pixels"]
        return result
    for index in range(0, len(pixels_a), 4):
        alpha_a, alpha_b = pixels_a[index + 3], pixels_b[index + 3]
        if abs(alpha_a - alpha_b) > tolerance:
            result["alpha_mismatched"] += 1
            result["mismatched"] += 1
            continue
        if alpha_a == 0 and alpha_b == 0:
            continue
        diff = max(abs(pixels_a[index + channel] - pixels_b[index + channel]) for channel in range(3))
        if diff > result["max_channel_diff"]:
            result["max_channel_diff"] = diff
        if diff > tolerance:
            result["mismatched"] += 1
    return result


def read_probe_events(path):
    """Return {event: [(qpc, x, y, extra), ...]} and the QPC frequency."""
    if not os.path.exists(path):
        return None, None
    frequency = None
    lines = []
    for line in read_text(path).splitlines():
        if line.startswith("# frequency="):
            frequency = int(line.split("=", 1)[1])
        elif line and not line.startswith("#"):
            lines.append(line)
    events = {}
    for row in csv.DictReader(lines):
        try:
            entry = (int(row["time"]), float(row["x"]), float(row["y"]), int(row["extra"]))
        except (KeyError, TypeError, ValueError):
            continue
        events.setdefault(row["event"], []).append(entry)
    for series in events.values():
        series.sort()
    return events, frequency


def read_action_lines(path):
    """Read a line-based action log into a list of {key: value}."""
    actions = []
    if not os.path.exists(path):
        return actions
    for line in read_text(path).splitlines():
        values = parse_key_values(line)
        if values:
            actions.append(values)
    return actions


def action_time(actions, name):
    for entry in actions:
        if entry.get("action") == name:
            for key in ("qpc", "start"):
                if entry.get(key):
                    try:
                        return int(entry[key])
                    except ValueError:
                        return None
    return None


def between(series, start, end):
    return [entry for entry in series if start <= entry[0] <= end]


def canvas_box(path):
    if not os.path.exists(path):
        return None
    try:
        data = json.loads(read_text(path))
        return [int(value) for value in data["canvas"]]
    except (KeyError, TypeError, ValueError):
        return None


def count_dark(picture, box=None, minimum=0):
    x0, y0, width, height = 0, 0, picture.width, picture.height
    if box:
        x0, y0, width, height = box
    count = 0
    for y in range(max(0, y0), min(picture.height, y0 + height)):
        for x in range(max(0, x0), min(picture.width, x0 + width)):
            color = picture.pixel(x, y)
            if (color[0] + color[1] + color[2]) / 3.0 < 110:
                count += 1
    return count


def dark_runs(picture, x, y0, y1):
    runs = 0
    inside = False
    for y in range(max(0, y0), min(picture.height, y1)):
        color = picture.pixel(x, y)
        dark = (color[0] + color[1] + color[2]) / 3.0 < 110
        if dark and not inside:
            runs += 1
        inside = dark
    return runs


def dark_components(picture, box, minimum=2):
    """Count 8-connected dark components inside a box."""
    x0, y0, width, height = box
    seen = set()
    components = 0
    for y in range(max(0, y0), min(picture.height, y0 + height)):
        for x in range(max(0, x0), min(picture.width, x0 + width)):
            color = picture.pixel(x, y)
            if (color[0] + color[1] + color[2]) / 3.0 >= 110 or (x, y) in seen:
                continue
            stack = [(x, y)]
            area = 0
            while stack:
                px, py = stack.pop()
                if (px, py) in seen:
                    continue
                if px < x0 or py < y0 or px >= x0 + width or py >= y0 + height:
                    continue
                seen.add((px, py))
                pixel = picture.pixel(px, py)
                if (pixel[0] + pixel[1] + pixel[2]) / 3.0 >= 110:
                    continue
                area += 1
                stack.extend(((px + 1, py), (px - 1, py), (px, py + 1), (px, py - 1)))
            if area >= minimum:
                components += 1
    return components


def item(identifier, title, verdict, reasons, evidence, metrics=None):
    return {
        "id": identifier,
        "title": title,
        "verdict": verdict,
        "reasons": list(reasons),
        "evidence": evidence,
        "metrics": metrics or {},
    }


def relative(base, path):
    return os.path.relpath(path, base).replace(os.sep, "/")


def judge_clipboard(base, facts, checks):
    settings = checks.get("clipboard", {})
    formats_path = os.path.join(base, "clipboard-formats.txt")
    formats = read_formats(formats_path)
    names = [row.get("name", "") for row in formats]
    identifiers = set()
    for row in formats:
        try:
            identifiers.add(int(row.get("id", "0")))
        except ValueError:
            pass
    png = [name for name in names if name.lower() in ("png", "image/png")
           or name.lower().endswith(';value="png"')]
    dib = 8 in identifiers or "CF_DIB" in names
    dibv5 = 17 in identifiers or "CF_DIBV5" in names
    bitmap = 2 in identifiers or "CF_BITMAP" in names
    png_path = os.path.join(base, facts.get("clipboard", {}).get("png", "clipboard-png.png"))
    source_path = os.path.join(base, facts.get("clipboard", {}).get("source", "clipboard-source.png"))
    reasons = []
    metrics = {"formats": len(formats), "png_formats": png, "dib": dib, "dibv5": dibv5,
               "bitmap": bitmap}
    if not formats:
        reasons.append("clipboard-formats.txt がない（コピーが届いていない）")
    if not png:
        reasons.append("PNG 形式がない（PNG か image/png が必要）")
    if dib or dibv5 or bitmap:
        reasons.append("古い DIB 系の形式がある（CF_DIB=%s CF_DIBV5=%s CF_BITMAP=%s）" % (dib, dibv5, bitmap))
    if os.path.exists(png_path) and os.path.exists(source_path):
        try:
            same = compare_pngs(source_path, png_path, int(settings.get("pixel_tolerance", 1)))
            metrics.update(same)
            if same["mismatched"]:
                reasons.append("PNG の画素が元と一致しない（%d 画素、最大差 %d）" % (
                    same["mismatched"], same["max_channel_diff"]))
        except ValueError as error:
            reasons.append("PNG を読めない: %s" % error)
    else:
        reasons.append("clipboard-png.png か clipboard-source.png がない")
    evidence = [relative(base, formats_path), relative(base, png_path), relative(base, source_path)]
    return item("clipboard", "クリップボードの形式と画素", "pass" if not reasons else "fail",
                reasons, evidence, metrics)


def judge_tabs(base, facts, checks):
    path = os.path.join(base, "tabs.txt")
    values = parse_key_values(read_text(path)) if os.path.exists(path) else {}
    tabs = facts.get("tabs", {}) or {}
    initial = tabs.get("initial") or values.get("initial", "")
    after = tabs.get("after") or values.get("after", "")
    back = tabs.get("back") or values.get("back", "")
    expected_a = values.get("expected_a") or facts.get("expected_a", "")
    expected_b = values.get("expected_b") or facts.get("expected_b", "")
    reasons = []
    if not initial or not after or not back:
        reasons.append("窓の題名を読めていない")
    if expected_a and expected_a not in initial:
        reasons.append("最初の題名に文書 A の名前がない: %r" % initial)
    if expected_b and expected_b not in after:
        reasons.append("切り替えた後の題名に文書 B の名前がない: %r" % after)
    if expected_a and expected_a not in back:
        reasons.append("戻した後の題名に文書 A の名前がない: %r" % back)
    if initial and after and initial == after:
        reasons.append("Ctrl+Tab の前後で題名が変わらない")
    metrics = {"initial": initial, "after": after, "back": back,
               "expected_a": expected_a, "expected_b": expected_b}
    return item("tabs", "タブの題名", "pass" if not reasons else "fail",
                reasons, [relative(base, path)], metrics)


def _key_down(states):
    for match in re.finditer(r"([a-z]+)=(-?\d+);", states or ""):
        if match.group(1) != "async" and int(match.group(2)) < 0:
            return True
    return False


def judge_keyboard(base, facts, checks):
    path = os.path.join(base, "keyboard.txt")
    values = parse_key_values(read_text(path)) if os.path.exists(path) else {}
    keyboard = facts.get("keyboard", {}) or {}
    layout = keyboard.get("layout") or values.get("layout", "")
    sent = keyboard.get("send") or values.get("send", "")
    before = keyboard.get("title_before") or values.get("title_before", "")
    after = keyboard.get("title_after") or values.get("title_after", "")
    states = keyboard.get("key_states") or values.get("key_states", "")
    loaded = 0
    match = re.search(r"loaded=(\d+)", layout or "")
    if match:
        loaded = int(match.group(1))
    tabs_values = {}
    tabs_path = os.path.join(base, "tabs.txt")
    if os.path.exists(tabs_path):
        tabs_values = parse_key_values(read_text(tabs_path))
    expected_b = tabs_values.get("expected_b", "")
    reasons = []
    if not loaded:
        reasons.append("LoadKeyboardLayout で 00000411 を読み込めていない: %r" % layout)
    if "posted=1" not in (layout or "") and "posted=True" not in (layout or ""):
        reasons.append("窓へ配列の変更を送っていない: %r" % layout)
    if not sent:
        reasons.append("SendInput の結果がない")
    if not before or not after or before == after:
        reasons.append("Ctrl+Tab の前後で題名が変わらない")
    elif expected_b and expected_b not in after:
        reasons.append("Ctrl+Tab の後の題名に文書 B の名前がない: %r" % after)
    if _key_down(states) or (keyboard.get("modifier_down") or values.get("modifier_down")) in ("1", 1, True):
        reasons.append("キーを離した後に修飾キーが残っている: %r" % states)
    metrics = {"layout": layout, "send": sent, "title_before": before,
               "title_after": after, "key_states": states, "loaded": loaded}
    return item("keyboard", "日本語配列の Ctrl+Tab と修飾キー", "pass" if not reasons else "fail",
                reasons, [relative(base, path)], metrics)


def judge_flicker(base, facts, checks):
    settings = checks.get("flicker", {})
    path = os.path.join(base, "flicker.csv")
    rows = []
    if os.path.exists(path):
        with open(path, encoding="utf-8", errors="replace", newline="") as handle:
            rows = list(csv.DictReader(handle))
    blank_ratio = float(settings.get("blank_white_ratio", 0.98))
    blank_dark = int(settings.get("blank_dark_pixels", 20))
    stale_changed = float(settings.get("stale_changed_ratio", 0.002))
    blank = stale = 0
    frames = len(rows)
    for row in rows:
        try:
            white = float(row.get("canvas_white_ratio", "-1"))
            dark = int(float(row.get("canvas_dark_pixels", "0")))
            changed = float(row.get("changed_ratio", "-1"))
            rect_changed = row.get("rect_changed") == "1"
        except (TypeError, ValueError):
            continue
        if white >= blank_ratio and dark <= blank_dark:
            blank += 1
        if rect_changed and 0 <= changed <= stale_changed:
            stale += 1
    log_path = os.path.join(base, "flicker-log.txt")
    actions = read_action_lines(log_path)
    dialog_opened = bool((facts.get("flicker", {}) or {}).get("dialog_opened"))
    reasons = []
    if not rows:
        reasons.append("flicker.csv がない（監視が動いていない）")
    if not dialog_opened:
        reasons.append("設定のダイアログを開けなかった")
    metrics = {"frames": frames, "blank_frames": blank, "stale_frames": stale,
               "resize_actions": sum(1 for entry in actions if entry.get("action") == "resize"),
               "dialog_opened": dialog_opened,
               "monitor_exit": (facts.get("flicker", {}) or {}).get("monitor_exit")}
    evidence = [relative(base, path), relative(base, log_path)]
    verdict = "check"
    if reasons:
        verdict = "fail"
    else:
        reasons.append("フォーク版との比較待ち（同じ実行に fork の対象を入れる）")
    return item("flicker", "窓の大きさの変更とダイアログのちらつき", verdict,
                reasons, evidence, metrics)


def _probe_strokes(events, start, end):
    if not events:
        return 0, 0
    return (len(between(events.get("stroke_begin", []), start, end)),
            len(between(events.get("stroke_paint", []), start, end)))


def _has_outline(events, start, end):
    return (any(entry[3] & 4 for entry in between(events.get("outline_state", []), start, end))
            or any(entry[3] & 4 for entry in between(events.get("outline_gate", []), start, end)))


def judge_pen(base, facts, checks):
    settings = checks.get("pen", {})
    pen = facts.get("pen", {}) or {}
    probe_path = os.path.join(base, pen.get("probe", "pen/probe.csv"))
    events, frequency = read_probe_events(probe_path)
    action_path = os.path.join(base, pen.get("actions", "pen/actions.txt"))
    actions = read_action_lines(action_path)
    canvas = canvas_box(os.path.join(base, "pen/canvas.json")) or pen.get("canvas")
    captures = pen.get("captures", {}) or {}
    items = []

    def capture(name):
        path = os.path.join(base, pen.get("dir", "pen"), captures.get(name, name + ".bmp"))
        return path if os.path.exists(path) else None

    # Stage 2/3 items 1 and 2: the outline follows the canvas edge.
    enter = action_time(actions, "enter")
    leave = action_time(actions, "leave")
    reasons = []
    if not events:
        reasons.append("probe.csv がない（LIBREPAINT_LATENCY_LOG が届いていない）")
    else:
        enters = between(events.get("qt_event", []), enter or 0, (enter or 0) + 8000000)
        leaves = between(events.get("qt_event", []), leave or 0, (leave or 0) + 8000000)
        outline_in = _has_outline(events, enter or 0, (leave or 0) + 1000000)
        outline_after = _has_outline(events, (leave or 0) + 1000000,
                                     max((entry[0] for entry in events.get("qt_event", [])), default=0))
        if not any(entry[1] == 7 for entry in enters):
            reasons.append("キャンバスへ入った Enter がない")
        if not any(entry[1] == 8 for entry in leaves):
            reasons.append("キャンバスから出た Leave がない")
        if not outline_in:
            reasons.append("キャンバスの中の輪郭の矩形がない")
        if outline_after:
            reasons.append("キャンバスから出た後に輪郭の矩形が残っている")
    items.append(item("pen.enter_leave", "キャンバスの出入りの輪郭",
                      "pass" if not reasons else "fail", reasons,
                      [relative(base, probe_path), relative(base, action_path)] +
                      [relative(base, capture("inside")), relative(base, capture("left"))]
                      if capture("inside") and capture("left") else [relative(base, probe_path)],
                      {"frequency": frequency}))

    # Stage 2 item 3/6: a stroke across the edge is one stroke, not two.
    start = action_time(actions, "edge-start")
    end = action_time(actions, "edge-end")
    begins, paints = _probe_strokes(events, start or 0, end or 0)
    reasons = []
    picture = None
    edge = capture("edge")
    if edge:
        try:
            picture = display_check.load(edge)
        except ValueError:
            picture = None
    if not events:
        reasons.append("probe.csv がない")
    elif begins != 1:
        reasons.append("縁をまたぐ線の stroke_begin が 1 回でない（%d 回）" % begins)
    if paints < 1:
        reasons.append("縁をまたぐ線が描かれていない（stroke_paint=%d）" % paints)
    if picture is None:
        reasons.append("縁をまたぐ線の画面の取り込みがない")
    elif canvas and count_dark(picture, [canvas[0] + int(canvas[2] * 0.9), canvas[1],
                                         max(1, int(canvas[2] * 0.1)), canvas[3]]) < 1:
        reasons.append("縁をまたぐ線が画面の右端に写っていない")
    items.append(item("pen.edge_line", "縁をまたぐ線", "pass" if not reasons else "fail",
                      reasons, [relative(base, probe_path), relative(base, edge or probe_path)],
                      {"stroke_begin": begins, "stroke_paint": paints}))

    # Stage 2 item 6: one line, not a double line.
    start = action_time(actions, "single-start")
    end = action_time(actions, "single-end")
    begins, paints = _probe_strokes(events, start or 0, end or 0)
    reasons = []
    runs = None
    single = capture("single")
    if single:
        try:
            picture = display_check.load(single)
            if canvas:
                x = canvas[0] + int(canvas[2] * 0.5)
                runs = dark_runs(picture, x, canvas[1] + int(canvas[3] * 0.15),
                                 canvas[1] + int(canvas[3] * 0.35))
        except ValueError:
            runs = None
    if begins != 1:
        reasons.append("1 本の線の stroke_begin が 1 回でない（%d 回）" % begins)
    if paints < 1:
        reasons.append("線が描かれていない（stroke_paint=%d）" % paints)
    if runs is None:
        reasons.append("1 本の線の画面の取り込みがない")
    elif runs != 1:
        reasons.append("線が %d 本に分かれて写っている（二重の線）" % runs)
    items.append(item("pen.single_line", "二重の線がないこと", "pass" if not reasons else "fail",
                      reasons, [relative(base, probe_path), relative(base, single or probe_path)],
                      {"stroke_begin": begins, "stroke_paint": paints, "dark_runs": runs}))

    # Stage 3 item 13: a long press does not open the context menu.
    start = action_time(actions, "hold-start")
    end = action_time(actions, "hold-end")
    begins, paints = _probe_strokes(events, start or 0, end or 0)
    popup_path = os.path.join(base, pen.get("popups", "pen/popups.txt"))
    popup_lines = read_text(popup_path).splitlines() if os.path.exists(popup_path) else []
    menu = []
    for line in popup_lines:
        parts = line.split("|")
        if len(parts) < 3:
            continue
        cls, title = parts[1], parts[2]
        if (cls in ("#32768",) or "QMenu" in cls or "Popup" in cls
                or (title and cls not in ("tooltips_class32", "IME", "MSCTFIME UI"))):
            menu.append(line)
    reasons = []
    if not popup_lines:
        reasons.append("長押しの間の窓の一覧がない")
    if menu:
        reasons.append("長押しでメニューらしい窓が出た: %s" % "; ".join(menu[:3]))
    if begins < 1 or paints < 1:
        reasons.append("長押しの押下が線になっていない（begin=%d paint=%d）" % (begins, paints))
    items.append(item("pen.long_press", "長押しで右クリックのメニューが出ないこと",
                      "pass" if not reasons else "fail", reasons,
                      [relative(base, probe_path), relative(base, popup_path)],
                      {"stroke_begin": begins, "stroke_paint": paints, "menu_windows": len(menu)}))

    # Stage 3 item 14: a short tap makes one dot.
    start = action_time(actions, "dot-start")
    end = action_time(actions, "dot-end")
    begins, paints = _probe_strokes(events, start or 0, end or 0)
    reasons = []
    components = None
    dot = capture("dot")
    if dot:
        try:
            picture = display_check.load(dot)
            if canvas:
                box = [canvas[0] + int(canvas[2] * 0.3) - 40,
                       canvas[1] + int(canvas[3] * 0.6) - 40, 80, 80]
                components = dark_components(picture, box)
        except ValueError:
            components = None
    if begins != 1:
        reasons.append("点の押下が 1 回でない（%d 回）" % begins)
    if paints < 1:
        reasons.append("点が描かれていない（stroke_paint=%d）" % paints)
    if components is None:
        reasons.append("点の画面の取り込みがない")
    elif components != 1:
        reasons.append("点が %d 個に分かれて写っている" % components)
    items.append(item("pen.dot", "点が 1 つ", "pass" if not reasons else "fail",
                      reasons, [relative(base, probe_path), relative(base, dot or probe_path)],
                      {"stroke_begin": begins, "stroke_paint": paints, "components": components}))

    # Stage 3 items 10-12: title bar and frame, then back to the canvas.
    cursor_path = os.path.join(base, pen.get("cursors", "pen/cursors.txt"))
    cursor = parse_key_values(read_text(cursor_path)) if os.path.exists(cursor_path) else {}
    handles = {}
    for name in ("inside", "titlebar", "return"):
        match = re.search(r"handle=(\d+)", cursor.get(name, ""))
        handles[name] = match.group(1) if match else None
    return_time = action_time(actions, "return-inside")
    reasons = []
    if not handles.get("inside") or not handles.get("return"):
        reasons.append("カーソルの記録がない")
    elif handles["inside"] != handles["return"]:
        reasons.append("キャンバスへ戻った後のカーソルが元と違う（%s -> %s）" % (
            handles["inside"], handles["return"]))
    if handles.get("titlebar") and handles.get("inside") and handles["titlebar"] == handles["inside"]:
        reasons.append("タイトルバーでもキャンバスと同じカーソルのまま")
    if events and return_time:
        enters = [entry for entry in events.get("qt_event", []) if entry[0] >= return_time and entry[1] == 7]
        if not enters:
            reasons.append("キャンバスへ戻った後の Enter がない")
        if not _has_outline(events, return_time, return_time + 20000000):
            reasons.append("キャンバスへ戻った後の輪郭がない")
    items.append(item("pen.nonclient_return", "タイトルバーと枠からの戻り",
                      "pass" if not reasons else "fail", reasons,
                      [relative(base, cursor_path), relative(base, probe_path),
                       relative(base, capture("return") or cursor_path)],
                      {"cursor_inside": handles.get("inside"), "cursor_titlebar": handles.get("titlebar"),
                       "cursor_return": handles.get("return")}))

    # Stage 1/2 item 17: the first line right after a start.
    first = facts.get("first_line", {}) or {}
    first_probe = os.path.join(base, first.get("probe", "first/probe.csv"))
    first_events, first_frequency = read_probe_events(first_probe)
    first_path = os.path.join(base, first.get("actions", "first/actions.txt"))
    first_actions = read_action_lines(first_path)
    stroke_start = int(first.get("stroke_start") or action_time(first_actions, "stroke_start") or 0)
    stroke_end = int(first.get("stroke_end") or action_time(first_actions, "stroke_end") or 0)
    begins, paints = _probe_strokes(first_events, stroke_start, stroke_end)
    reasons = []
    picture = None
    first_capture = os.path.join(base, first.get("capture", "first/line.bmp"))
    if os.path.exists(first_capture):
        try:
            picture = display_check.load(first_capture)
        except ValueError:
            picture = None
    if not first_events:
        reasons.append("起動直後の probe.csv がない")
    elif begins < 1 or paints < 1:
        reasons.append("起動直後の最初の線が描かれていない（begin=%d paint=%d）" % (begins, paints))
    if picture is None:
        reasons.append("起動直後の線の画面の取り込みがない")
    elif count_dark(picture) < 1:
        reasons.append("起動直後の線が画面に写っていない")
    items.append(item("pen.first_line", "起動直後の最初の線", "pass" if not reasons else "fail",
                      reasons, [relative(base, first_probe), relative(base, first_capture)],
                      {"stroke_begin": begins, "stroke_paint": paints,
                       "frequency": first_frequency}))
    return items


def judge_run(run_dir, scenario):
    checks = scenario.get("checks", {})
    facts_path = os.path.join(run_dir, "facts.json")
    facts = json.loads(read_text(facts_path)) if os.path.exists(facts_path) else {}
    results = []
    for judge, identifier in (
            (judge_clipboard, "clipboard"),
            (judge_tabs, "tabs"),
            (judge_keyboard, "keyboard"),
            (judge_flicker, "flicker")):
        try:
            results.append(judge(run_dir, facts, checks))
        except Exception as error:  # keep one broken item from hiding the others
            results.append(item(identifier, identifier, "fail", ["judge failed: %s" % error], []))
    try:
        results.extend(judge_pen(run_dir, facts, checks))
    except Exception as error:
        for identifier in PEN_ITEMS:
            results.append(item(identifier, identifier, "fail",
                                ["judge failed: %s" % error], []))
    for result in results:
        result["evidence"] = [path for path in result["evidence"] if path and path != "."]
    return {
        "target": os.path.basename(run_dir),
        "run_dir": run_dir,
        "facts": facts_path if os.path.exists(facts_path) else None,
        "errors": facts.get("errors", []),
        "items": results,
    }


def apply_fork_baseline(results, scenario):
    baseline_name = scenario.get("baseline_target")
    baseline = next((entry for entry in results if entry.get("target") == baseline_name), None)
    if baseline is None:
        baseline = next((entry for entry in results if "fork" in entry.get("target", "").lower()), None)
    if baseline is None:
        return
    base_flicker = next((entry for entry in baseline["items"] if entry["id"] == "flicker"), None)
    if not base_flicker:
        return
    if base_flicker["verdict"] == "check":
        base_flicker["verdict"] = "pass"
        base_flicker["reasons"] = ["基準（フォーク版）"]
    allowance = scenario.get("checks", {}).get("flicker", {}).get("allowance", {})
    for result in results:
        if result is baseline:
            continue
        flicker = next((entry for entry in result["items"] if entry["id"] == "flicker"), None)
        if not flicker or flicker["verdict"] == "fail":
            continue
        reasons = []
        for key, limit in (("blank_frames", allowance.get("blank_frames", 2)),
                           ("stale_frames", allowance.get("stale_frames", 2))):
            value = flicker["metrics"].get(key, 0)
            base = base_flicker["metrics"].get(key, 0)
            if value > base + limit:
                reasons.append("%s がフォーク版より多い（%d > %d + %d）" % (key, value, base, limit))
        flicker["reasons"] = reasons or ["フォーク版と同じ範囲（%s）" % baseline["target"]]
        flicker["verdict"] = "pass" if not reasons else "fail"
        flicker["metrics"]["fork_baseline"] = baseline["target"]
        flicker["metrics"]["fork_blank_frames"] = base_flicker["metrics"].get("blank_frames")
        flicker["metrics"]["fork_stale_frames"] = base_flicker["metrics"].get("stale_frames")


def runner_for(host, target, scenario, out):
    if not host.get("capture_tool"):
        raise SystemExit("host %s: the automatic check needs capture_tool (windows_capture.exe)"
                         % host.get("name", "?"))
    argv = [RUN_REMOTE, host["ssh"], target["runner_target"], "1", target["app"], out]
    env = {
        "CAPTURE_TOOL": host["capture_tool"],
        "RUN_NAME": target["name"],
        "CHECK_JSON": json.dumps(scenario.get("checks", {}), sort_keys=True),
        "PRESENTMON": "off",
        "SURFACE_STATE": "off",
        "SURFACE_STATE_WAIT": "off",
        "THREAD_CPU": "off",
        "POWER": "off",
        "TARGET_ENV": json.dumps(target.get("env", {}), sort_keys=True),
        "TARGET_KRITARC": json.dumps(target.get("kritarc", {}), sort_keys=True),
    }
    return argv, env


def run_checked(host, target, scenario, run_root):
    out = os.path.join(run_root, host["name"], target["name"])
    os.makedirs(out, exist_ok=True)
    argv, env = runner_for(host, target, scenario, out)
    print("$ " + " ".join(argv), flush=True)
    merged = dict(os.environ)
    merged.update(env)
    status = subprocess.run(argv, env=merged).returncode
    dirs = [os.path.join(out, name) for name in sorted(os.listdir(out))
            if os.path.isdir(os.path.join(out, name))]
    with_facts = [path for path in dirs if os.path.exists(os.path.join(path, "facts.json"))]
    if not with_facts:
        return {"target": target["name"], "run_dir": out, "facts": None, "errors": [
            "run-remote.sh returned %d and wrote no facts.json" % status], "items": [
            item(identifier, identifier, "fail", ["no facts.json"], []) for identifier in ITEMS]}
    result = judge_run(with_facts[-1], scenario)
    result["target"] = target["name"]
    result["run_status"] = status
    return result


def table(results):
    lines = ["| target | item | verdict | evidence | notes |",
             "| --- | --- | --- | --- | --- |"]
    for result in results:
        for entry in result["items"]:
            lines.append("| %s | %s | %s | %s | %s |" % (
                result["target"], entry["title"], entry["verdict"],
                ", ".join(entry["evidence"]) or "-", "; ".join(entry["reasons"]) or "-"))
    return "\n".join(lines)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--scenario", required=True)
    parser.add_argument("--hosts", required=True)
    parser.add_argument("--out", default=os.path.join(ROOT, "build", "perf"))
    parser.add_argument("--label", default="q4-e2e")
    parser.add_argument("--only", help="comma-separated host names")
    parser.add_argument("--targets", help="comma-separated target names")
    parser.add_argument("--analyze-only", metavar="RUN_ROOT")
    args = parser.parse_args(argv)

    scenario = json.load(open(args.scenario))
    config = json.load(open(args.hosts))
    hosts = [host for host in config["hosts"] if not args.only or host["name"] in args.only.split(",")]
    wanted = set(args.targets.split(",")) if args.targets else None
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    run_root = args.analyze_only or os.path.join(
        args.out, "%s-%s-%s" % (scenario["name"], args.label, stamp))
    os.makedirs(run_root, exist_ok=True)

    results = []
    if args.analyze_only:
        for host in hosts:
            host_dir = os.path.join(run_root, host["name"])
            if not os.path.isdir(host_dir):
                continue
            for name in sorted(os.listdir(host_dir)):
                path = os.path.join(host_dir, name)
                if os.path.isdir(path) and os.path.exists(os.path.join(path, "facts.json")):
                    results.append(judge_run(path, scenario))
    else:
        for host in hosts:
            for target in host.get("targets", []):
                if wanted and target["name"] not in wanted:
                    continue
                results.append(run_checked(host, target, scenario, run_root))

    apply_fork_baseline(results, scenario)
    summary = {
        "scenario": scenario["name"],
        "label": args.label,
        "date": datetime.date.today().isoformat(),
        "commit": subprocess.run(["git", "-C", ROOT, "rev-parse", "--short=10", "HEAD"],
                                 capture_output=True, text=True).stdout.strip(),
        "targets": results,
        "not_reproducible": scenario.get("not_reproducible", list(NOT_REPRODUCIBLE)),
    }
    destination = os.path.join(run_root, "checks.json")
    with open(destination, "w") as handle:
        json.dump(summary, handle, indent=2, sort_keys=True)
    for result in results:
        name = re.sub(r"[^A-Za-z0-9_.-]+", "-", result["target"])
        with open(os.path.join(run_root, "check-%s.json" % name), "w") as handle:
            json.dump(result, handle, indent=2, sort_keys=True)
    print(table(results))
    print("checks.json: " + destination)
    failed = any(entry["verdict"] != "pass" for result in results for entry in result["items"])
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))

# SPDX-License-Identifier: GPL-3.0-or-later
"""Contract of scripts/perf/ipad-pen/ipad_measure.py."""

import csv
import importlib.util
import json
import os
import tempfile
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SPEC = importlib.util.spec_from_file_location("ipad_measure", os.path.join(ROOT, "scripts", "perf", "ipad-pen", "ipad_measure.py"))
ipad_measure = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ipad_measure)

OFFSET_NS = 5_000_000_000
FRAME_NS = 1e9 / 120


def write_csv(path, header, rows):
    with open(path, "w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(header)
        writer.writerows(rows)


def flash_files(directory, late_every_other=True, leading_change=False):
    """60 changes 13 panel frames apart and a 60 Hz capture that sees them."""
    flashes, frames = [], []
    t_frame = 0
    if leading_change:
        frames.append((OFFSET_NS - 300_000_000, 0, 1718, 1200, 128.0))
        frames.append((OFFSET_NS - 250_000_000, 0, 1718, 1200, 255.0))
    for k in range(60):
        colour = "white" if k % 2 == 0 else "black"
        target_ns = round(1_000_000_000 + k * 13 * FRAME_NS)
        flashes.append((colour, "%.9f" % (target_ns / 1e9), "0"))
        late = late_every_other and k % 2 == 1
        shown = target_ns + OFFSET_NS + (round(FRAME_NS) if late else 0)
        luma = 255.0 if colour == "white" else 0.0
        frames.append((shown, shown + 40_000_000, 1718, 1200, luma))
    frames.sort()
    write_csv(os.path.join(directory, "flash.csv"), ["color", "target_s", "callback_s"], flashes)
    write_csv(os.path.join(directory, "frames.csv"), ["frame_ns", "arrival_ns", "width", "height", "luma"], frames)
    return os.path.join(directory, "frames.csv"), os.path.join(directory, "flash.csv")


class FlashOffsetTest(unittest.TestCase):
    def test_the_on_time_group_gives_the_offset_when_every_other_change_is_late(self):
        with tempfile.TemporaryDirectory() as directory:
            offset, _, pairs, _ = ipad_measure.flash_offset(*flash_files(directory))
        self.assertLess(abs(offset - OFFSET_NS), 1_000)
        # The first colour starts the comparison, so 29 on-time changes remain.
        self.assertEqual(pairs, 29)

    def test_a_screen_change_before_the_flashes_is_skipped(self):
        with tempfile.TemporaryDirectory() as directory:
            offset, _, _, _ = ipad_measure.flash_offset(*flash_files(directory, leading_change=True))
        self.assertLess(abs(offset - OFFSET_NS), 1_000)

    def test_too_few_changes_stop_with_an_anomaly(self):
        with tempfile.TemporaryDirectory() as directory:
            frames, flash = flash_files(directory)
            with self.assertRaises(SystemExit):
                ipad_measure.flash_offset(frames, flash, to_ns=OFFSET_NS + 1_100_000_000)


class CalibrationTest(unittest.TestCase):
    def test_three_taps_give_the_native_to_window_map(self):
        with tempfile.TemporaryDirectory() as directory:
            log = os.path.join(directory, "calibrate.log")
            with open(log, "w") as handle:
                handle.write("PencilProbe app frame 1194x834 orientation 5\n")
                for nx, ny in ((208.5, 298.5), (625.5, 298.5), (208.5, 895.5)):
                    handle.write("PencilProbe CAL point %g,%g composed X: 0 -> done 13 events ended pencil x=%g y=%g force=0\n"
                                 % (nx, ny, 1194 - ny, nx))
            result = ipad_measure.calibration(log)
        self.assertEqual(result["window"], [1194.0, 834.0])
        x, y = ipad_measure.to_native(result, 894.0, 417.0)
        self.assertAlmostEqual(x, 417.0)
        self.assertAlmostEqual(y, 300.0)


class InjectTest(unittest.TestCase):
    def test_each_sample_gets_the_offset_interpolated_by_its_ipad_time(self):
        with tempfile.TemporaryDirectory() as directory:
            plan = os.path.join(directory, "plan.json")
            with open(plan, "w") as handle:
                json.dump({"rate": 240, "scale": 2.0, "strokes": [{"band": "stroke0", "seconds": 0.1, "dwell": 0.0}]}, handle)
            probe = os.path.join(directory, "probe.csv")
            with open(probe, "w") as handle:
                handle.write("# frequency=1000000000\ntime,event,x,y,extra,thread\n")
                for k in range(25):
                    handle.write("0,ios_pencil_sample,%d,100,%d,1\n" % (10 + k, 10_000_000 + k * 4_167))
            calibrations = []
            for name, t, offset in (("before", 9.0, 1_000_000_000), ("after", 11.0, 1_000_200_000)):
                path = os.path.join(directory, name + ".json")
                with open(path, "w") as handle:
                    json.dump({"offset_ns": offset, "ipad_time_s": t}, handle)
                calibrations.append(path)
            out = os.path.join(directory, "inject.csv")
            args = type("Args", (), {"probe": probe, "plan": plan, "calibration": calibrations, "out": out})
            ipad_measure.inject(args)
            with open(out) as handle:
                rows = list(csv.DictReader(handle))
        # At 10.0 s the offset is halfway: 1_000_100_000 ns.
        self.assertEqual(int(rows[0]["time_ns"]), 10_000_000_000 + 1_000_100_000)
        self.assertEqual(rows[0]["x"], "20.00")
        self.assertEqual(rows[-1]["phase"], "release")


if __name__ == "__main__":
    unittest.main()

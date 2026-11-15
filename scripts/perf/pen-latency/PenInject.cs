// SPDX-License-Identifier: GPL-3.0-or-later
//
// Replays a fixed pen trajectory through the Windows synthetic pointer API
// (CreateSyntheticPointerDevice / InjectSyntheticPointerInput, PT_PEN) and
// records the QueryPerformanceCounter time of every injected sample.
//
// The trajectory is placed relative to the client area of a target window:
//   1. hover: a figure-eight with the pen in range and not touching;
//   2. strokes: short curves drawn with a pressure ramp, separated by hover.
// Compiled at run time by Windows PowerShell 5.1 (C# 5), see run-surface.ps1.

using System;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

public static class PenInject
{
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }

    [StructLayout(LayoutKind.Sequential)]
    public struct POINTER_INFO
    {
        public uint pointerType; public uint pointerId; public uint frameId; public uint pointerFlags;
        public IntPtr sourceDevice; public IntPtr hwndTarget;
        public POINT ptPixelLocation; public POINT ptHimetricLocation;
        public POINT ptPixelLocationRaw; public POINT ptHimetricLocationRaw;
        public uint dwTime; public uint historyCount; public int InputData; public uint dwKeyStates;
        public ulong PerformanceCount; public int ButtonChangeType;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct POINTER_PEN_INFO
    {
        public POINTER_INFO pointerInfo;
        public uint penFlags; public uint penMask; public uint pressure; public uint rotation;
        public int tiltX; public int tiltY;
    }

    // POINTER_TYPE_INFO: a 4-byte type followed by a union of the touch and
    // pen structures. The touch structure is the larger member (144 bytes on
    // x64), so the whole structure is 152 bytes.
    [StructLayout(LayoutKind.Explicit, Size = 152)]
    public struct POINTER_TYPE_INFO
    {
        [FieldOffset(0)] public uint type;
        [FieldOffset(8)] public POINTER_PEN_INFO penInfo;
    }

    [DllImport("user32.dll", SetLastError = true)]
    static extern IntPtr CreateSyntheticPointerDevice(uint pointerType, uint maxCount, uint mode);
    [DllImport("user32.dll", SetLastError = true)]
    static extern bool InjectSyntheticPointerInput(IntPtr device, [In] POINTER_TYPE_INFO[] info, uint count);
    [DllImport("user32.dll")] static extern void DestroySyntheticPointerDevice(IntPtr device);
    [DllImport("user32.dll")] static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr hwnd, ref POINT point);
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr hwnd, int command);
    [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr window);
    [DllImport("kernel32.dll")] static extern uint SetThreadExecutionState(uint flags);
    [DllImport("winmm.dll")] static extern uint timeBeginPeriod(uint period);
    [DllImport("winmm.dll")] static extern uint timeEndPeriod(uint period);

    const uint PT_PEN = 3;
    const uint POINTER_FEEDBACK_DEFAULT = 1;
    const uint FLAG_INRANGE = 0x2, FLAG_INCONTACT = 0x4, FLAG_FIRSTBUTTON = 0x10;
    const uint FLAG_DOWN = 0x10000, FLAG_UPDATE = 0x20000, FLAG_UP = 0x40000;
    const uint PEN_MASK_PRESSURE = 0x1;
    const int SW_MAXIMIZE = 3;
    static readonly IntPtr DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = new IntPtr(-4);

    public class Options
    {
        public double RateHz = 240;
        public double HoverSeconds = 10;
        public double HoverPeriodSeconds = 2.5;
        public int Strokes = 6;
        public double StrokeSeconds = 1.5;
        public double GapSeconds = 0.5;
        public double CenterX = 0.42, CenterY = 0.5;   // fraction of the client area
        public double SpanX = 0.16, SpanY = 0.22;      // half extent, fraction of the client area
        public uint PointerId = 0;
    }

    static StringBuilder log;
    static IntPtr device;
    static uint frame;

    // Written after ShowWindow/SetForegroundWindow and before the first
    // injected sample. run-surface.ps1 copies it into meta.txt so a run that
    // lost the foreground can be told apart from one that was still in front
    // when the input began (2026-11-11: the canvas stopped repainting in runs
    // whose input window may not have been in front).
    public static string ForegroundInjectStart = "unavailable";

    static string ForegroundText(IntPtr hwnd, uint targetPid, out bool appInFront)
    {
        IntPtr foreground = GetForegroundWindow();
        uint pid = 0;
        GetWindowThreadProcessId(foreground, out pid);
        string name = "";
        try { name = Process.GetProcessById((int)pid).ProcessName; } catch { }
        appInFront = targetPid != 0 && pid == targetPid;
        return string.Format("pid={0} name={1} visible={2} same_window={3} app={4}",
            pid, name, IsWindowVisible(foreground), foreground == hwnd, appInFront);
    }

    static string EnsureForeground(IntPtr hwnd)
    {
        uint targetPid = 0;
        GetWindowThreadProcessId(hwnd, out targetPid);
        bool appInFront;
        string first = ForegroundText(hwnd, targetPid, out appInFront);
        if (appInFront) return first + " retried=False";
        SetForegroundWindow(hwnd);
        Thread.Sleep(300);
        string second = ForegroundText(hwnd, targetPid, out appInFront);
        return second + " retried=True";
    }

    static void Inject(string phase, uint flags, int x, int y, uint pressure, uint pointerId)
    {
        var info = new POINTER_TYPE_INFO[1];
        info[0].type = PT_PEN;
        info[0].penInfo.pointerInfo.pointerType = PT_PEN;
        info[0].penInfo.pointerInfo.pointerId = pointerId;
        info[0].penInfo.pointerInfo.frameId = ++frame;
        info[0].penInfo.pointerInfo.pointerFlags = flags;
        info[0].penInfo.pointerInfo.ptPixelLocation.X = x;
        info[0].penInfo.pointerInfo.ptPixelLocation.Y = y;
        info[0].penInfo.penMask = PEN_MASK_PRESSURE;
        info[0].penInfo.pressure = pressure;
        long before = Stopwatch.GetTimestamp();
        bool ok = InjectSyntheticPointerInput(device, info, 1);
        int error = ok ? 0 : Marshal.GetLastWin32Error();
        log.AppendFormat("{0},{1},{2},{3},{4},{5},{6},{7}\n", before, phase, flags, x, y, pressure, ok ? 1 : 0, error);
    }

    // Sleeps until the QPC deadline: coarse sleep, then spin for the last 1.5 ms.
    static void WaitUntil(long deadline)
    {
        double f = Stopwatch.Frequency;
        while (true)
        {
            long now = Stopwatch.GetTimestamp();
            double remainingMs = (deadline - now) * 1000.0 / f;
            if (remainingMs <= 0) return;
            if (remainingMs > 1.5) Thread.Sleep(1); else Thread.SpinWait(50);
        }
    }

    public static int Run(IntPtr hwnd, string outPath, Options o)
    {
        SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        SetThreadExecutionState(0x80000000u | 0x2u | 0x1u); // ES_CONTINUOUS | ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED
        log = new StringBuilder();
        log.AppendFormat("# frequency={0}\n", Stopwatch.Frequency);
        log.Append("qpc,phase,flags,x,y,pressure,ok,error\n");

        if (hwnd != IntPtr.Zero)
        {
            ShowWindow(hwnd, SW_MAXIMIZE);
            SetForegroundWindow(hwnd);
            Thread.Sleep(1000);
            ForegroundInjectStart = EnsureForeground(hwnd);
        }
        RECT rect;
        POINT origin = new POINT();
        if (hwnd == IntPtr.Zero || !GetClientRect(hwnd, out rect) || !ClientToScreen(hwnd, ref origin))
            throw new InvalidOperationException("cannot read the target window's client area");
        int width = rect.Right - rect.Left, height = rect.Bottom - rect.Top;
        double cx = origin.X + o.CenterX * width, cy = origin.Y + o.CenterY * height;
        double ax = o.SpanX * width, ay = o.SpanY * height;
        log.AppendFormat("# client={0},{1},{2},{3}\n", origin.X, origin.Y, width, height);

        device = CreateSyntheticPointerDevice(PT_PEN, 1, POINTER_FEEDBACK_DEFAULT);
        if (device == IntPtr.Zero)
            throw new InvalidOperationException("CreateSyntheticPointerDevice failed: " + Marshal.GetLastWin32Error());

        timeBeginPeriod(1);
        try
        {
            long period = (long)(Stopwatch.Frequency / o.RateHz);
            long next = Stopwatch.GetTimestamp();
            uint hover = FLAG_INRANGE | FLAG_UPDATE;

            // Hover: figure-eight (x = sin t, y = sin 2t / 2) at a constant period.
            int hoverSamples = (int)(o.HoverSeconds * o.RateHz);
            for (int i = 0; i < hoverSamples; i++)
            {
                double t = 2 * Math.PI * (i / o.RateHz) / o.HoverPeriodSeconds;
                WaitUntil(next); next += period;
                Inject("hover", hover, (int)Math.Round(cx + ax * Math.Sin(t)), (int)Math.Round(cy + ay * 0.5 * Math.Sin(2 * t)), 0, o.PointerId);
            }

            // Strokes: arcs across the trajectory area, pressure rising then falling.
            int strokeSamples = (int)(o.StrokeSeconds * o.RateHz);
            int gapSamples = (int)(o.GapSeconds * o.RateHz);
            for (int s = 0; s < o.Strokes; s++)
            {
                double row = o.Strokes > 1 ? (double)s / (o.Strokes - 1) * 2 - 1 : 0; // -1 .. 1
                Func<double, int> px = delegate(double u) { return (int)Math.Round(cx + ax * (2 * u - 1)); };
                Func<double, int> py = delegate(double u) { return (int)Math.Round(cy + ay * (0.8 * row + 0.15 * Math.Sin(2 * Math.PI * u))); };
                for (int i = 0; i < gapSamples; i++)
                {
                    WaitUntil(next); next += period;
                    Inject("hover", hover, px(0), py(0), 0, o.PointerId);
                }
                for (int i = 0; i <= strokeSamples; i++)
                {
                    double u = (double)i / strokeSamples;
                    uint pressure = (uint)Math.Round(1024 * (0.2 + 0.6 * Math.Sin(Math.PI * u)));
                    uint flags = FLAG_INRANGE | FLAG_INCONTACT | FLAG_FIRSTBUTTON | (i == 0 ? FLAG_DOWN : FLAG_UPDATE);
                    WaitUntil(next); next += period;
                    Inject("stroke", flags, px(u), py(u), pressure, o.PointerId);
                }
                WaitUntil(next); next += period;
                Inject("stroke", FLAG_INRANGE | FLAG_UP, px(1), py(1), 0, o.PointerId);
            }
            WaitUntil(next); next += period;
            Inject("leave", FLAG_UPDATE, (int)cx, (int)cy, 0, o.PointerId);
        }
        finally
        {
            timeEndPeriod(1);
            DestroySyntheticPointerDevice(device);
            File.WriteAllText(outPath, log.ToString());
        }
        return 0;
    }
}

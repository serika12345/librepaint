// SPDX-License-Identifier: GPL-3.0-or-later
//
// Win32 helpers for the Q4 automatic check (scripts/perf/check.py).
//
// run-surface.ps1 compiles this file at run time (Windows PowerShell 5.1,
// C# 5) and calls the methods to read the clipboard, the window title, the
// keyboard state, the cursor and the popup windows, and to replay short
// synthetic-pen gestures. It only collects facts and evidence files; the
// verdict is made on the development host by scripts/perf/check.py, so the
// pass limits can be tested without Windows.
//
// The pen sequence uses the same synthetic pointer API as PenInject.cs
// (CreateSyntheticPointerDevice / InjectSyntheticPointerInput, PT_PEN). The
// coordinates are fractions of the target window's client area and may leave
// it (for example y = -0.06 is the title bar and x = 1.15 is past the right
// frame), which is what the stage 2/3 non-client checks need.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading;

public static class Q4Check
{
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int Left, Top, Right, Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }

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

    [StructLayout(LayoutKind.Explicit, Size = 152)]
    public struct POINTER_TYPE_INFO
    {
        [FieldOffset(0)] public uint type;
        [FieldOffset(8)] public POINTER_PEN_INFO penInfo;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct CURSORINFO
    {
        public uint cbSize; public uint flags; public IntPtr hCursor; public POINT ptScreenPos;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct MOUSEINPUT
    {
        public int dx; public int dy; public uint mouseData; public uint dwFlags; public uint time;
        public IntPtr dwExtraInfo;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct KEYBDINPUT
    {
        public ushort wVk; public ushort wScan; public uint dwFlags; public uint time;
        public IntPtr dwExtraInfo;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct HARDWAREINPUT
    {
        public uint uMsg; public ushort wParamL; public ushort wParamH;
    }

    [StructLayout(LayoutKind.Explicit)]
    public struct INPUTUNION
    {
        [FieldOffset(0)] public MOUSEINPUT mi;
        [FieldOffset(0)] public KEYBDINPUT ki;
        [FieldOffset(0)] public HARDWAREINPUT hi;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct INPUT
    {
        public uint type; public INPUTUNION u;
    }

    [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr hwnd, int command);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] static extern bool IsWindow(IntPtr hwnd);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hwnd, out uint pid);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetWindowTextW(IntPtr hwnd, StringBuilder text, int count);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetClassNameW(IntPtr hwnd, StringBuilder text, int count);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] static extern bool GetClientRect(IntPtr hwnd, out RECT rect);
    [DllImport("user32.dll")] static extern bool ClientToScreen(IntPtr hwnd, ref POINT point);
    [DllImport("user32.dll")] static extern bool SetWindowPos(IntPtr hwnd, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] static extern int GetSystemMetrics(int index);
    [DllImport("user32.dll")] static extern bool PostMessage(IntPtr hwnd, uint message, IntPtr wParam, IntPtr lParam);
    [DllImport("user32.dll")] static extern short GetKeyState(int key);
    [DllImport("user32.dll")] static extern short GetAsyncKeyState(int key);
    [DllImport("user32.dll")] static extern bool GetCursorInfo(ref CURSORINFO info);
    [DllImport("user32.dll")] static extern uint SendInput(uint count, INPUT[] inputs, int size);
    [DllImport("user32.dll")] static extern bool OpenClipboard(IntPtr owner);
    [DllImport("user32.dll")] static extern bool CloseClipboard();
    [DllImport("user32.dll")] static extern bool EmptyClipboard();
    [DllImport("user32.dll")] static extern uint EnumClipboardFormats(uint format);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetClipboardFormatNameW(uint format, StringBuilder name, int count);
    [DllImport("user32.dll")] static extern IntPtr GetClipboardData(uint format);
    [DllImport("kernel32.dll")] static extern IntPtr GlobalLock(IntPtr memory);
    [DllImport("kernel32.dll")] static extern bool GlobalUnlock(IntPtr memory);
    [DllImport("kernel32.dll")] static extern UIntPtr GlobalSize(IntPtr memory);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumWindowsProc callback, IntPtr param);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern IntPtr LoadKeyboardLayoutW(string name, uint flags);
    [DllImport("user32.dll")] static extern IntPtr ActivateKeyboardLayout(IntPtr layout, uint flags);
    [DllImport("user32.dll")] static extern IntPtr GetKeyboardLayout(uint thread);
    [DllImport("user32.dll")] static extern bool SetThreadDpiAwarenessContext(IntPtr context);
    [DllImport("user32.dll", SetLastError = true)] static extern IntPtr CreateSyntheticPointerDevice(uint type, uint maxCount, uint mode);
    [DllImport("user32.dll", SetLastError = true)] static extern bool InjectSyntheticPointerInput(IntPtr device, [In] POINTER_TYPE_INFO[] info, uint count);
    [DllImport("user32.dll")] static extern void DestroySyntheticPointerDevice(IntPtr device);
    [DllImport("winmm.dll")] static extern uint timeBeginPeriod(uint period);
    [DllImport("winmm.dll")] static extern uint timeEndPeriod(uint period);

    delegate bool EnumWindowsProc(IntPtr hwnd, IntPtr param);

    const uint PT_PEN = 3;
    const uint POINTER_FEEDBACK_DEFAULT = 1;
    const uint FLAG_INRANGE = 0x2, FLAG_INCONTACT = 0x4, FLAG_FIRSTBUTTON = 0x10;
    const uint FLAG_DOWN = 0x10000, FLAG_UPDATE = 0x20000, FLAG_UP = 0x40000;
    const uint PEN_MASK_PRESSURE = 0x1;
    const uint WM_INPUTLANGCHANGEREQUEST = 0x0050;
    const uint WM_CLOSE = 0x0010;
    const uint SWP_NOZORDER = 0x0004, SWP_NOACTIVATE = 0x0010;
    const uint KLF_ACTIVATE = 0x0001;
    const uint KLF_SETFORPROCESS = 0x0100;
    const int SW_MAXIMIZE = 3;
    static readonly IntPtr DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 = new IntPtr(-4);

    static IntPtr penDevice;
    static StreamWriter penLog;
    static uint penFrame;
    static bool penDown;
    static string lastError = "";

    static Q4Check()
    {
        try { SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); }
        catch { }
    }

    static void CheckDpi()
    {
        try { SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2); }
        catch { }
    }

    static string Failure(string what, int error)
    {
        lastError = what + " error=" + error.ToString();
        return lastError;
    }

    public static long NowTicks()
    {
        return Stopwatch.GetTimestamp();
    }

    public static long TickFrequency()
    {
        return Stopwatch.Frequency;
    }

    public static string WindowText(IntPtr hwnd)
    {
        StringBuilder text = new StringBuilder(1024);
        GetWindowTextW(hwnd, text, text.Capacity);
        return text.ToString();
    }

    public static bool ForegroundIs(IntPtr hwnd)
    {
        return GetForegroundWindow() == hwnd;
    }

    public static bool BringToFront(IntPtr hwnd)
    {
        ShowWindow(hwnd, SW_MAXIMIZE);
        SetForegroundWindow(hwnd);
        Thread.Sleep(400);
        return ForegroundIs(hwnd);
    }

    public static string WindowList(uint pid)
    {
        StringBuilder lines = new StringBuilder();
        EnumWindowsProc callback = delegate(IntPtr hwnd, IntPtr param)
        {
            uint owner = 0;
            GetWindowThreadProcessId(hwnd, out owner);
            if (owner != pid || !IsWindowVisible(hwnd)) return true;
            RECT rect; GetWindowRect(hwnd, out rect);
            if (rect.Right - rect.Left < 2 || rect.Bottom - rect.Top < 2) return true;
            StringBuilder name = new StringBuilder(512);
            StringBuilder cls = new StringBuilder(256);
            GetWindowTextW(hwnd, name, name.Capacity);
            GetClassNameW(hwnd, cls, cls.Capacity);
            lines.AppendFormat("{0}|{1}|{2}|{3}|{4}|{5}\n", hwnd.ToInt64(), cls.ToString(),
                name.ToString().Replace("|", " "), rect.Left, rect.Top,
                (rect.Right - rect.Left) + "x" + (rect.Bottom - rect.Top));
            return true;
        };
        EnumWindows(callback, IntPtr.Zero);
        return lines.ToString();
    }

    public static bool CloseWindow(IntPtr hwnd)
    {
        return IsWindow(hwnd) && PostMessage(hwnd, WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
    }

    public static string ResizeWindow(IntPtr hwnd, int width, int height)
    {
        CheckDpi();
        int screenW = GetSystemMetrics(0), screenH = GetSystemMetrics(1);
        if (width <= 0) width = screenW;
        if (height <= 0) height = screenH;
        int x = Math.Max(0, (screenW - width) / 2);
        int y = Math.Max(0, (screenH - height) / 2);
        bool ok = SetWindowPos(hwnd, IntPtr.Zero, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
        Thread.Sleep(250);
        RECT rect; GetWindowRect(hwnd, out rect);
        return string.Format("ok={0} x={1} y={2} w={3} h={4}", ok, rect.Left, rect.Top,
            rect.Right - rect.Left, rect.Bottom - rect.Top);
    }

    public static string ClientRect(IntPtr hwnd)
    {
        CheckDpi();
        RECT rect; POINT origin = new POINT();
        if (!GetClientRect(hwnd, out rect) || !ClientToScreen(hwnd, ref origin))
            return "unavailable";
        return string.Format("{0},{1},{2},{3}", origin.X, origin.Y,
            rect.Right - rect.Left, rect.Bottom - rect.Top);
    }

    public static string CursorInfo()
    {
        CURSORINFO info = new CURSORINFO();
        info.cbSize = (uint)Marshal.SizeOf(typeof(CURSORINFO));
        if (!GetCursorInfo(ref info)) return "unavailable";
        return string.Format("handle={0} x={1} y={2}", info.hCursor.ToInt64(),
            info.ptScreenPos.X, info.ptScreenPos.Y);
    }

    public static string ClearClipboard()
    {
        for (int i = 0; i < 20; i++)
        {
            if (OpenClipboard(IntPtr.Zero))
            {
                bool ok = EmptyClipboard();
                CloseClipboard();
                return ok ? "cleared" : "empty-failed";
            }
            Thread.Sleep(50);
        }
        return "open-failed";
    }

    static string StandardFormatName(uint id)
    {
        switch (id)
        {
            case 1: return "CF_TEXT";
            case 2: return "CF_BITMAP";
            case 3: return "CF_METAFILEPICT";
            case 8: return "CF_DIB";
            case 17: return "CF_DIBV5";
            case 13: return "CF_UNICODETEXT";
            case 15: return "CF_HDROP";
            default: return "";
        }
    }

    static bool LooksLikePng(string name)
    {
        string lower = (name ?? "").ToLowerInvariant();
        return lower == "png" || lower == "image/png" || lower.EndsWith(";value=\"png\"");
    }

    public static string SaveClipboardFormats(string directory, string sourcePath)
    {
        for (int attempt = 0; attempt < 20; attempt++)
        {
            if (OpenClipboard(IntPtr.Zero)) break;
            if (attempt == 19) return "open-failed";
            Thread.Sleep(50);
        }
        StringBuilder rows = new StringBuilder();
        rows.Append("id,name,bytes,path\n");
        int pngSaved = 0;
        try
        {
            uint format = 0;
            while ((format = EnumClipboardFormats(format)) != 0)
            {
                StringBuilder name = new StringBuilder(512);
                GetClipboardFormatNameW(format, name, name.Capacity);
                string label = name.ToString();
                if (label.Length == 0) label = StandardFormatName(format);
                IntPtr handle = GetClipboardData(format);
                ulong size = 0;
                string path = "";
                if (handle != IntPtr.Zero)
                {
                    size = GlobalSize(handle).ToUInt64();
                    if (LooksLikePng(label) && size > 0)
                    {
                        IntPtr memory = GlobalLock(handle);
                        if (memory != IntPtr.Zero)
                        {
                            byte[] bytes = new byte[(int)size];
                            Marshal.Copy(memory, bytes, 0, bytes.Length);
                            GlobalUnlock(handle);
                            path = pngSaved == 0 ? "clipboard-png.png" :
                                ("clipboard-png-" + pngSaved.ToString() + ".png");
                            File.WriteAllBytes(Path.Combine(directory, path), bytes);
                            pngSaved++;
                        }
                    }
                }
                rows.AppendFormat("{0},{1},{2},{3}\n", format,
                    label.Replace(",", " ").Replace("\n", " ").Replace("\r", " "), size, path);
            }
        }
        finally
        {
            CloseClipboard();
        }
        File.WriteAllText(Path.Combine(directory, "clipboard-formats.txt"), rows.ToString());
        if (!string.IsNullOrEmpty(sourcePath) && File.Exists(sourcePath))
        {
            File.Copy(sourcePath, Path.Combine(directory, "clipboard-source.png"), true);
        }
        return "formats=" + (rows.ToString().Split('\n').Length - 2) + " png=" + pngSaved;
    }

    static void SendKey(ushort key, bool up)
    {
        INPUT input = new INPUT();
        input.type = 1; // INPUT_KEYBOARD
        input.u.ki.wVk = key;
        input.u.ki.wScan = 0;
        input.u.ki.dwFlags = up ? 0x0002u : 0u; // KEYEVENTF_KEYUP
        input.u.ki.time = 0;
        input.u.ki.dwExtraInfo = IntPtr.Zero;
        SendInput(1, new INPUT[] { input }, Marshal.SizeOf(typeof(INPUT)));
    }

    public static string SendCtrl(uint virtualKey)
    {
        INPUT[] inputs = new INPUT[4];
        ushort[] keys = new ushort[] { 0x11, (ushort)virtualKey, (ushort)virtualKey, 0x11 };
        bool[] ups = new bool[] { false, false, true, true };
        for (int i = 0; i < 4; i++)
        {
            inputs[i].type = 1;
            inputs[i].u.ki.wVk = keys[i];
            inputs[i].u.ki.dwFlags = ups[i] ? 0x0002u : 0u;
        }
        uint sent = SendInput(4, inputs, Marshal.SizeOf(typeof(INPUT)));
        Thread.Sleep(120);
        return "sent=" + sent;
    }

    static string KeyLine(int key, string name)
    {
        return string.Format("{0}={1};async={2}", name, (int)GetKeyState(key), (int)GetAsyncKeyState(key));
    }

    public static string KeyStates()
    {
        return string.Join(" ", new string[] {
            KeyLine(0x11, "control"), KeyLine(0x12, "menu"), KeyLine(0x10, "shift"),
            KeyLine(0x5B, "lwin"), KeyLine(0x5C, "rwin") });
    }

    public static int ModifierDown(string states)
    {
        // GetKeyState returns a short; a negative value means the key is down.
        foreach (string field in states.Split(' '))
        {
            int equals = field.IndexOf('=');
            if (equals < 0) continue;
            string value = field.Substring(equals + 1);
            int semicolon = value.IndexOf(';');
            if (semicolon >= 0) value = value.Substring(0, semicolon);
            int parsed;
            if (int.TryParse(value, out parsed) && parsed < 0) return 1;
        }
        return 0;
    }

    public static string LoadJapaneseLayout(IntPtr hwnd)
    {
        IntPtr loaded = LoadKeyboardLayoutW("00000411", KLF_ACTIVATE);
        IntPtr active = ActivateKeyboardLayout(loaded, KLF_SETFORPROCESS);
        bool posted = hwnd != IntPtr.Zero && PostMessage(hwnd, WM_INPUTLANGCHANGEREQUEST,
            IntPtr.Zero, loaded);
        Thread.Sleep(300);
        IntPtr current = GetKeyboardLayout(0);
        return string.Format("loaded={0} active={1} posted={2} current={3}",
            loaded.ToInt64(), active.ToInt64(), posted, current.ToInt64());
    }

    static void InjectPen(uint flags, int x, int y, uint pressure, string action, double fx, double fy)
    {
        POINTER_TYPE_INFO[] info = new POINTER_TYPE_INFO[1];
        info[0].type = PT_PEN;
        info[0].penInfo.pointerInfo.pointerType = PT_PEN;
        info[0].penInfo.pointerInfo.frameId = ++penFrame;
        info[0].penInfo.pointerInfo.pointerFlags = flags;
        info[0].penInfo.pointerInfo.ptPixelLocation.X = x;
        info[0].penInfo.pointerInfo.ptPixelLocation.Y = y;
        info[0].penInfo.penMask = PEN_MASK_PRESSURE;
        info[0].penInfo.pressure = pressure;
        long before = NowTicks();
        bool ok = InjectSyntheticPointerInput(penDevice, info, 1);
        int error = ok ? 0 : Marshal.GetLastWin32Error();
        if (penLog != null)
        {
            penLog.WriteLine(string.Format("{0},{1},{2:F6},{3:F6},{4},{5},{6}", before, action, fx, fy,
                flags, ok ? 1 : 0, error));
            penLog.Flush();
        }
    }

    static void ClientPoint(IntPtr hwnd, double fx, double fy, out int x, out int y)
    {
        CheckDpi();
        RECT rect; POINT origin = new POINT();
        if (!GetClientRect(hwnd, out rect) || !ClientToScreen(hwnd, ref origin))
            throw new InvalidOperationException("cannot read the target window's client area");
        x = origin.X + (int)Math.Round(fx * (rect.Right - rect.Left));
        y = origin.Y + (int)Math.Round(fy * (rect.Bottom - rect.Top));
    }

    public static string PenStart(string logPath)
    {
        CheckDpi();
        PenStop();
        penDevice = CreateSyntheticPointerDevice(PT_PEN, 1, POINTER_FEEDBACK_DEFAULT);
        if (penDevice == IntPtr.Zero) return Failure("CreateSyntheticPointerDevice", Marshal.GetLastWin32Error());
        penDown = false;
        penFrame = 0;
        penLog = new StreamWriter(logPath, false, new UTF8Encoding(false));
        penLog.WriteLine("# frequency=" + Stopwatch.Frequency.ToString());
        penLog.WriteLine("qpc,action,fx,fy,flags,ok,error");
        timeBeginPeriod(1);
        return "started";
    }

    public static void PenStop()
    {
        if (penLog != null)
        {
            penLog.Flush();
            penLog.Dispose();
            penLog = null;
        }
        if (penDevice != IntPtr.Zero)
        {
            DestroySyntheticPointerDevice(penDevice);
            penDevice = IntPtr.Zero;
        }
        try { timeEndPeriod(1); } catch { }
        penDown = false;
    }

    public static string PenHover(IntPtr hwnd, double fx, double fy, int milliseconds)
    {
        int x, y; ClientPoint(hwnd, fx, fy, out x, out y);
        int end = Environment.TickCount + milliseconds;
        do
        {
            InjectPen(FLAG_INRANGE | FLAG_UPDATE, x, y, 0, "hover", fx, fy);
            Thread.Sleep(16);
        } while (Environment.TickCount < end);
        return "ok";
    }

    public static string PenLeave(IntPtr hwnd, double fx, double fy)
    {
        int x, y; ClientPoint(hwnd, fx, fy, out x, out y);
        InjectPen(FLAG_UPDATE, x, y, 0, "leave", fx, fy);
        return "ok";
    }

    public static string PenDownAt(IntPtr hwnd, double fx, double fy)
    {
        int x, y; ClientPoint(hwnd, fx, fy, out x, out y);
        InjectPen(FLAG_INRANGE | FLAG_INCONTACT | FLAG_FIRSTBUTTON | FLAG_DOWN, x, y, 512, "down", fx, fy);
        penDown = true;
        return "ok";
    }

    public static string PenHold(IntPtr hwnd, double fx, double fy, int milliseconds)
    {
        int x, y; ClientPoint(hwnd, fx, fy, out x, out y);
        int end = Environment.TickCount + milliseconds;
        do
        {
            InjectPen(FLAG_INRANGE | FLAG_INCONTACT | FLAG_FIRSTBUTTON | FLAG_UPDATE, x, y, 512, "hold", fx, fy);
            Thread.Sleep(16);
        } while (Environment.TickCount < end);
        return "ok";
    }

    public static string PenUpAt(IntPtr hwnd, double fx, double fy)
    {
        int x, y; ClientPoint(hwnd, fx, fy, out x, out y);
        InjectPen(FLAG_INRANGE | FLAG_UP, x, y, 0, "up", fx, fy);
        penDown = false;
        return "ok";
    }

    public static string PenStroke(IntPtr hwnd, double x0, double y0, double x1, double y1, int milliseconds)
    {
        int samples = Math.Max(8, milliseconds / 4);
        int endX, endY; ClientPoint(hwnd, x0, y0, out endX, out endY);
        int xt, yt; ClientPoint(hwnd, x1, y1, out xt, out yt);
        for (int i = 0; i <= samples; i++)
        {
            double u = (double)i / samples;
            double fx = x0 + (x1 - x0) * u, fy = y0 + (y1 - y0) * u;
            int x = (int)Math.Round(endX + (xt - endX) * u);
            int y = (int)Math.Round(endY + (yt - endY) * u);
            uint flags;
            string action;
            if (i == 0) { flags = FLAG_INRANGE | FLAG_INCONTACT | FLAG_FIRSTBUTTON | FLAG_DOWN; action = "stroke-down"; }
            else if (i == samples) { flags = FLAG_INRANGE | FLAG_UP; action = "stroke-up"; }
            else { flags = FLAG_INRANGE | FLAG_INCONTACT | FLAG_FIRSTBUTTON | FLAG_UPDATE; action = "stroke"; }
            InjectPen(flags, x, y, i == 0 || i == samples ? 0u : 512u, action, fx, fy);
            Thread.Sleep(Math.Max(1, milliseconds / samples));
        }
        return "ok";
    }

    public static string PenTap(IntPtr hwnd, double fx, double fy, int holdMs)
    {
        PenDownAt(hwnd, fx, fy);
        PenHold(hwnd, fx, fy, holdMs);
        PenUpAt(hwnd, fx, fy);
        return "ok";
    }
}

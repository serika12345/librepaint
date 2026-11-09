// SPDX-License-Identifier: GPL-3.0-or-later
//
// Minimal Win32 pen receiver used as the lower-bound reference for pen
// latency measurement. A maximized window records every WM_POINTER pen
// message in the probe.csv format of libs/global/KisLatencyProbe
// ("native_pointer" rows), so analyze.py compares it with LibrePaint, and
// draws a marker at the newest pen position with GDI.
// Compiled at run time by Windows PowerShell 5.1 (C# 5), see run-surface.ps1.

using System;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using System.Windows.Forms;

public class PenProbe : Form
{
    [DllImport("user32.dll")] static extern bool GetPointerType(uint pointerId, out uint type);
    [DllImport("user32.dll")] static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);

    const int WM_POINTERUPDATE = 0x0245, WM_POINTERDOWN = 0x0246, WM_POINTERUP = 0x0247, WM_MOUSEMOVE = 0x0200;
    const uint PT_PEN = 3, FLAG_INCONTACT = 0x4;

    readonly StringBuilder csv = new StringBuilder();
    readonly string outPath;
    Rectangle lastMark = Rectangle.Empty;
    Point lastPoint; bool lastContact;

    public PenProbe(string outPath, int seconds)
    {
        this.outPath = outPath;
        csv.AppendFormat("# frequency={0}\n", Stopwatch.Frequency);
        csv.Append("time,event,x,y,extra,thread\n");
        Text = "PenProbe"; BackColor = Color.White; FormBorderStyle = FormBorderStyle.None;
        WindowState = FormWindowState.Maximized; TopMost = true;
        var timer = new Timer();
        timer.Interval = seconds * 1000;
        timer.Tick += delegate { Close(); };
        timer.Start();
    }

    protected override void OnFormClosed(FormClosedEventArgs e)
    {
        File.WriteAllText(outPath, csv.ToString());
        base.OnFormClosed(e);
    }

    protected override void WndProc(ref Message m)
    {
        if (m.Msg == WM_POINTERUPDATE || m.Msg == WM_POINTERDOWN || m.Msg == WM_POINTERUP)
        {
            long recv = Stopwatch.GetTimestamp();
            long wParam = (long)m.WParam, lParam = (long)m.LParam;
            uint id = (uint)(wParam & 0xFFFF);
            uint type;
            if (GetPointerType(id, out type) && type == PT_PEN)
            {
                uint flags = (uint)((wParam >> 16) & 0xFFFF);
                int x = (short)(lParam & 0xFFFF), y = (short)((lParam >> 16) & 0xFFFF);
                csv.AppendFormat("{0},native_pointer,{1},{2},{3},0\n", recv, x, y, ((long)m.Msg << 32) | flags);
                Draw(PointToClient(new Point(x, y)), (flags & FLAG_INCONTACT) != 0);
                return;
            }
            csv.AppendFormat("{0},native_pointer_other,0,0,{1},0\n", recv, type);
        }
        else if (m.Msg == WM_MOUSEMOVE)
        {
            csv.AppendFormat("{0},native_mouse,0,0,0,0\n", Stopwatch.GetTimestamp());
        }
        base.WndProc(ref m);
    }

    // Black marks on white so that the capture-based measurement detects them
    // like the brush outline (hover) and the ink (contact).
    void Draw(Point point, bool contact)
    {
        using (var g = CreateGraphics())
        {
            if (!lastMark.IsEmpty) g.FillRectangle(Brushes.White, lastMark);
            if (contact && lastContact)
                using (var pen = new Pen(Color.Black, 20)) { pen.StartCap = pen.EndCap = System.Drawing.Drawing2D.LineCap.Round; g.DrawLine(pen, lastPoint, point); }
            lastMark = contact ? Rectangle.Empty : new Rectangle(point.X - 10, point.Y - 10, 21, 21);
            if (!contact) g.FillRectangle(Brushes.Black, lastMark);
        }
        lastPoint = point; lastContact = contact;
    }

    static System.Threading.Thread thread;
    static PenProbe instance;

    // Shows the window on its own STA thread and returns its handle once it exists.
    public static IntPtr Start(string outPath, int seconds)
    {
        var ready = new System.Threading.ManualResetEvent(false);
        thread = new System.Threading.Thread(delegate()
        {
            SetThreadDpiAwarenessContext(new IntPtr(-4)); // per-monitor aware v2
            instance = new PenProbe(outPath, seconds);
            instance.Shown += delegate { ready.Set(); };
            Application.Run(instance);
        });
        thread.SetApartmentState(System.Threading.ApartmentState.STA);
        thread.Start();
        if (!ready.WaitOne(10000)) throw new InvalidOperationException("PenProbe window did not appear");
        IntPtr handle = IntPtr.Zero;
        instance.Invoke((MethodInvoker)delegate { handle = instance.Handle; });
        return handle;
    }

    // Closes the window (which writes probe.csv) and waits for the thread.
    public static void Stop()
    {
        if (instance != null && !instance.IsDisposed)
            instance.BeginInvoke((MethodInvoker)delegate { instance.Close(); });
        thread.Join(10000);
    }
}

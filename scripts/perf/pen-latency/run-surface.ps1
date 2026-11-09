# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs pen-latency measurements on a Windows device inside the interactive
# desktop session (started through the scheduled task that setup-surface.ps1
# registers). Parameters come from request.json next to this script:
#
#   { "target": "LibrePaint" | "Krita" | "PenProbe",
#     "runs": 5, "app": "<path to the executable>",
#     "settleSeconds": 12, "inject": { <PenInject.Options fields> },
#     "capture": false, "presentmon": "on" }
#
# With "capture": true, windows_capture.exe replays plan.csv (written by
# trajectory.py) next to this script and records the screen with DXGI
# Desktop Duplication instead of PenInject.cs replaying its own trajectory.
#
# Each run writes <root>\runs\<id>-<n>\ with presentmon.csv, probe.csv
# (LibrePaint and PenProbe), inject.csv, capture.csv (capture mode) and
# meta.txt. The PresentMon session name includes the run, and meta.txt records
# whether its log exists. meta.txt also records the timer resolution, the
# foreground window and the app process settings at the start, after the first
# window and before the capture. status.txt reports progress and ends with
# "done" or "failed: <reason>".

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$status = Join-Path $here 'status.txt'
Set-Content -Path $status -Value 'starting'

function Log([string]$text) { Add-Content -Path $status -Value $text }

# Launch-time facts that can latch the hover state of one run (L1 of Issue
# #108): the system timer resolution, which window is in front, and the app
# process settings. Each is written to meta.txt so a later analysis can tell
# the two states apart without changing the application.
$probeSource = @'
using System;
using System.Runtime.InteropServices;
public static class L1Probe
{
    [DllImport("ntdll.dll")]
    public static extern int NtQueryTimerResolution(out uint minimum, out uint maximum, out uint current);
    [DllImport("user32.dll")]
    public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")]
    public static extern uint GetWindowThreadProcessId(IntPtr window, out uint processId);
    [DllImport("user32.dll")]
    public static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")]
    public static extern bool IsIconic(IntPtr window);
}
'@
try { Add-Type -TypeDefinition $probeSource } catch { Log ("probe type unavailable: " + $_.Exception.Message) }

function Get-TimerResolution {
    try {
        [uint32]$minimum = 0
        [uint32]$maximum = 0
        [uint32]$current = 0
        [void][L1Probe]::NtQueryTimerResolution([ref]$minimum, [ref]$maximum, [ref]$current)
        return "min100ns=$minimum current100ns=$current max100ns=$maximum"
    } catch { return 'unavailable' }
}

function Get-Foreground {
    try {
        $window = [L1Probe]::GetForegroundWindow()
        [uint32]$ownerPid = 0
        [void][L1Probe]::GetWindowThreadProcessId($window, [ref]$ownerPid)
        $name = ''
        try { $name = (Get-Process -Id $ownerPid -ErrorAction Stop).ProcessName } catch { }
        return [pscustomobject]@{
            pid = $ownerPid
            name = $name
            visible = [L1Probe]::IsWindowVisible($window)
            iconic = [L1Probe]::IsIconic($window)
        }
    } catch {
        return [pscustomobject]@{ pid = 0; name = 'unavailable'; visible = $false; iconic = $false }
    }
}

function Write-LaunchFacts($dir, $stage, $app) {
    $foreground = Get-Foreground
    $lines = @(
        "timer_resolution_$stage=$(Get-TimerResolution)",
        "foreground_$stage=pid=$($foreground.pid) name=$($foreground.name) visible=$($foreground.visible) iconic=$($foreground.iconic)"
    )
    if ($app) {
        $lines += "app_foreground_$stage=$($foreground.pid -eq $app.Id)"
        try {
            $lines += "app_priority_$stage=$($app.PriorityClass)"
            $lines += "app_affinity_$stage=$($app.ProcessorAffinity)"
        } catch { }
    }
    Add-Content -Path (Join-Path $dir 'meta.txt') -Value $lines
}

function Wait-MainWindow($process, [int]$seconds) {
    $deadline = (Get-Date).AddSeconds($seconds)
    while ((Get-Date) -lt $deadline) {
        $process.Refresh()
        if ($process.HasExited) { throw "process exited before showing a window" }
        if ($process.MainWindowHandle -ne [IntPtr]::Zero) { return $process.MainWindowHandle }
        Start-Sleep -Milliseconds 250
    }
    throw "no main window after $seconds s"
}

function Write-Meta($dir, $request, $target) {
    $lines = @(
        "target=$target",
        "app=$($request.app)",
        "date=$(Get-Date -Format o)",
        "computer=$((Get-CimInstance Win32_ComputerSystem).Model)",
        "os=$([System.Environment]::OSVersion.VersionString)",
        "presentmon=$(if ($request.presentmon) { $request.presentmon } else { 'on' })"
    )
    $battery = Get-CimInstance Win32_Battery -ErrorAction SilentlyContinue
    if ($battery) { $lines += "battery_status=$($battery.BatteryStatus) charge=$($battery.EstimatedChargeRemaining)" }
    $video = Get-CimInstance Win32_VideoController | Select-Object -First 1
    $lines += "display=$($video.CurrentHorizontalResolution)x$($video.CurrentVerticalResolution)@$($video.CurrentRefreshRate)"
    $kritarc = Join-Path $env:LOCALAPPDATA 'kritarc'
    if (Test-Path $kritarc) {
        $keys = 'newCursorStyle|newOutlineStyle|useWin8PointerInput|fpsLimit|tabletEventsDelay|OpenGLRenderer|rootSurfaceFormat|useOpenGLTextureBuffer|disableVSync'
        $lines += Get-Content $kritarc | Where-Object { $_ -match "^($keys)=" } | ForEach-Object { "kritarc.$_" }
    }
    Set-Content -Path (Join-Path $dir 'meta.txt') -Value $lines
}

try {
    $request = Get-Content (Join-Path $here 'request.json') -Raw | ConvertFrom-Json
    $target = [string]$request.target
    $runs = [int]$request.runs
    $settle = if ($request.settleSeconds) { [int]$request.settleSeconds } else { 12 }

    $capture = [bool]$request.capture
    $captureTool = Join-Path $here 'windows_capture.exe'
    $plan = Join-Path $here 'plan.csv'
    function Invoke-Capture([string]$dir) {
        $log = Join-Path $dir 'capture.log'
        $process = Start-Process -FilePath $captureTool -ArgumentList @('--out', "`"$dir`"", '--plan', "`"$plan`"") `
            -PassThru -Wait -WindowStyle Hidden -RedirectStandardOutput $log -RedirectStandardError (Join-Path $dir 'capture.err')
        if ($process.ExitCode -ne 0) { throw "windows_capture failed: $(Get-Content (Join-Path $dir 'capture.err') -Raw)" }
    }
    Add-Type -Path (Join-Path $here 'PenInject.cs')
    if ($target -eq 'PenProbe') {
        Add-Type -Path (Join-Path $here 'PenProbe.cs') -ReferencedAssemblies System.Windows.Forms, System.Drawing
    }
    $options = New-Object PenInject+Options
    if ($request.inject) {
        foreach ($property in $request.inject.PSObject.Properties) { $options.($property.Name) = $property.Value }
    }
    $presentMon = Join-Path (Split-Path -Parent $here) 'PresentMon.exe'
    $presentMonEnabled = -not ([string]$request.presentmon -eq 'off')
    $runName = if ($request.name) { [string]$request.name } else { $target }
    $id = '{0}-{1}' -f $runName, (Get-Date -Format 'yyyyMMdd-HHmmss')

    for ($n = 1; $n -le $runs; $n++) {
        $dir = Join-Path $here ("runs\{0}-{1}" -f $id, $n)
        New-Item -ItemType Directory -Force $dir | Out-Null
        Log "run $n $dir"
        Write-Meta $dir $request $target
        Write-LaunchFacts $dir 'start' $null

        if ($target -eq 'PenProbe') {
            if ($capture) {
                $planSeconds = [double]((Get-Content $plan -Tail 1).Split(',')[0])
                $hwnd = [PenProbe]::Start((Join-Path $dir 'probe.csv'), [int]$planSeconds + 30)
                Start-Sleep -Seconds 2
                try { Invoke-Capture $dir } finally { [PenProbe]::Stop() }
            } else {
                $seconds = [int]($options.HoverSeconds + $options.Strokes * ($options.StrokeSeconds + $options.GapSeconds) + 8)
                $hwnd = [PenProbe]::Start((Join-Path $dir 'probe.csv'), $seconds)
                Start-Sleep -Seconds 2
                [PenInject]::Run($hwnd, (Join-Path $dir 'inject.csv'), $options) | Out-Null
                [PenProbe]::Stop()
            }
            continue
        }

        $canvas = Join-Path $dir 'canvas.png'
        Add-Type -AssemblyName System.Drawing
        $bitmap = New-Object System.Drawing.Bitmap 2048, 1536
        $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
        $graphics.Clear([System.Drawing.Color]::White)
        $graphics.Dispose()
        $bitmap.Save($canvas, [System.Drawing.Imaging.ImageFormat]::Png)
        $bitmap.Dispose()

        $processName = [System.IO.Path]::GetFileName([string]$request.app)
        # A session name per run keeps a session left by an earlier run from
        # blocking this one. PresentMon 2.x failed to record after it stopped
        # a session with the same name (2026-11-09 runs).
        $sessionName = 'LibrePaintPenLatency-{0}-{1}' -f $id, $n
        $presentMonCsv = Join-Path $dir 'presentmon.csv'
        $pm = $null
        if ($presentMonEnabled) {
            Get-Process -Name PresentMon -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
            Start-Sleep -Milliseconds 500
            $pmArgs = @('--process_name', $processName, '--output_file', $presentMonCsv, '--qpc_time',
                '--no_console_stats', '--stop_existing_session', '--terminate_on_proc_exit', '--session_name', $sessionName)
            $pm = Start-Process -FilePath $presentMon -ArgumentList $pmArgs -PassThru -WindowStyle Hidden `
                -RedirectStandardError (Join-Path $dir 'presentmon.err') -RedirectStandardOutput (Join-Path $dir 'presentmon.out')
        }

        if ($target -eq 'LibrePaint') { $env:LIBREPAINT_LATENCY_LOG = Join-Path $dir 'probe.csv' }
        else { Remove-Item Env:LIBREPAINT_LATENCY_LOG -ErrorAction SilentlyContinue }
        $app = Start-Process -FilePath $request.app -ArgumentList @('--nosplash', "`"$canvas`"") -PassThru
        try {
            $hwnd = Wait-MainWindow $app 120
            Write-LaunchFacts $dir 'window' $app
            Start-Sleep -Seconds $settle
            Write-LaunchFacts $dir 'capture' $app
            if ($capture) { Invoke-Capture $dir }
            else { [PenInject]::Run($hwnd, (Join-Path $dir 'inject.csv'), $options) | Out-Null }
            # The probe writer flushes every 500 ms; leave time for the last batch.
            Start-Sleep -Milliseconds 1500
        }
        finally {
            if (-not $app.HasExited) { Stop-Process -Id $app.Id -Force }
            if ($pm) {
                if (-not $pm.WaitForExit(20000)) {
                    Stop-Process -Id $pm.Id -Force -ErrorAction SilentlyContinue
                    try { & logman stop $sessionName -ets | Out-Null } catch { }
                }
            }
        }
        if ($presentMonEnabled) {
            $recorded = (Test-Path $presentMonCsv) -and ((Get-Item $presentMonCsv).Length -gt 0)
            $presentMonState = if ($recorded) { 'present' } else { 'none' }
            Add-Content -Path (Join-Path $dir 'meta.txt') -Value "presentmon_result=$presentMonState"
            Log "presentmon=$presentMonState"
        }
        Start-Sleep -Seconds 2
    }
    Log 'done'
}
catch {
    Log ("failed: " + $_.Exception.Message)
    exit 1
}

# SPDX-License-Identifier: GPL-3.0-or-later
#
# Runs pen-latency measurements on a Windows device inside the interactive
# desktop session (started through the scheduled task that setup-surface.ps1
# registers). Parameters come from request.json next to this script:
#
#   { "target": "LibrePaint" | "Krita" | "PenProbe",
#     "runs": 5, "app": "<path to the executable>",
#     "settleSeconds": 12, "inject": { <PenInject.Options fields> },
#     "capture": false, "presentmon": "on",
#     "env": { "LIBREPAINT_L1_SWAP_INTERVAL": "0" } }
#
# With "capture": true, windows_capture.exe replays plan.csv (written by
# trajectory.py) next to this script and records the screen with DXGI
# Desktop Duplication instead of PenInject.cs replaying its own trajectory.
#
# Each run writes <root>\runs\<id>-<n>\ with presentmon.csv, probe.csv
# (LibrePaint and PenProbe), inject.csv, capture.csv (capture mode) and
# meta.txt. PresentMon records the app process by its process ID with one
# fixed session name; the runner stops that session before and after the run
# and records whether the session started and how large the log is. meta.txt
# also records the timer resolution, the foreground window and the app process
# settings at the start, after the first window and before the capture.
# status.txt reports progress and ends with "done" or "failed: <reason>".

$ErrorActionPreference = 'Stop'
# Japanese "settings": Windows PowerShell 5.1 reads this file as the ANSI code
# page, so the literal characters would break the parser. Build the string
# from code points to keep the file pure ASCII (2026-11-11).
$jaSettings = [string]::new([char[]]@(0x8A2D, 0x5B9A))
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$status = Join-Path $here 'status.txt'
$script:runStatus = $null
Set-Content -Path $status -Value 'starting'

function Log([string]$text) {
    Add-Content -Path $status -Value $text
    # Keep each run's own progress: the next run overwrites the shared
    # status.txt, which hid the progress lines of an earlier run (2026-11-10).
    if ($script:runStatus) { Add-Content -Path $script:runStatus -Value $text }
}

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
    [DllImport("user32.dll")]
    public static extern bool SetForegroundWindow(IntPtr window);
    [DllImport("user32.dll")]
    public static extern bool ShowWindow(IntPtr window, int command);
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
            hwnd = $window
            pid = $ownerPid
            name = $name
            visible = [L1Probe]::IsWindowVisible($window)
            iconic = [L1Probe]::IsIconic($window)
        }
    } catch {
        return [pscustomobject]@{ hwnd = [IntPtr]::Zero; pid = 0; name = 'unavailable'; visible = $false; iconic = $false }
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

# The device state decides whether two runs can be compared: on 2026-11-11 the
# reference target's ink latency jumped from 17.8-18.1 to 29-33 ms in the
# middle of one session and the runs after the change were not comparable
# (progress.md, q4-focus-gate). surface_state.ps1 samples the temperatures,
# the processor performance, the total CPU use and the busiest processes; this
# writes the numbers a later analysis reads from meta.txt.
function Write-SurfaceStateSummary($dir, $csv) {
    if (-not (Test-Path $csv)) { return }
    $rows = @(Import-Csv $csv)
    $during = @($rows | Where-Object { $_.phase -eq 'during' })
    $lines = @()
    foreach ($phase in @('before', 'after')) {
        $row = $rows | Where-Object { $_.phase -eq $phase } | Select-Object -First 1
        $lines += "surface_state_${phase}_max_temp_c=$(if ($row) { $row.max_temp_c } else { 'unavailable' })"
        $lines += "surface_state_${phase}_processor_performance_pct=$(if ($row) { $row.processor_performance_pct } else { 'unavailable' })"
        $lines += "surface_state_${phase}_total_cpu_pct=$(if ($row) { $row.total_cpu_pct } else { 'unavailable' })"
    }
    $temps = @($during | Where-Object { $_.max_temp_c -match '^-?[0-9]+(\.[0-9]+)?$' } | ForEach-Object { [double]$_.max_temp_c })
    $performance = @($during | Where-Object { $_.processor_performance_pct -match '^[0-9]+$' } | ForEach-Object { [int]$_.processor_performance_pct })
    $cpu = @($during | Where-Object { $_.total_cpu_pct -match '^[0-9]+$' } | ForEach-Object { [int]$_.total_cpu_pct })
    $lines += "surface_state_during_samples=$($during.Count)"
    $lines += "surface_state_during_max_temp_c=$(if ($temps.Count) { ($temps | Measure-Object -Maximum).Maximum } else { 'unavailable' })"
    $lines += "surface_state_during_min_processor_performance_pct=$(if ($performance.Count) { ($performance | Measure-Object -Minimum).Minimum } else { 'unavailable' })"
    $lines += "surface_state_during_max_total_cpu_pct=$(if ($cpu.Count) { ($cpu | Measure-Object -Maximum).Maximum } else { 'unavailable' })"
    $lines += "surface_state_during_mean_total_cpu_pct=$(if ($cpu.Count) { [math]::Round(($cpu | Measure-Object -Average).Average, 1) } else { 'unavailable' })"
    $names = @()
    foreach ($row in $during) {
        if ($row.top_processes -and $row.top_processes -notmatch '^(unavailable|none)$') {
            $names += ($row.top_processes -split ';')[0].Trim()
        }
    }
    $top = if ($names.Count) {
        ($names | Group-Object | Sort-Object -Property Count -Descending | Select-Object -First 1).Name
    } else { 'unavailable' }
    $lines += "surface_state_top_process=$top"
    Add-Content -Path (Join-Path $dir 'meta.txt') -Value $lines
}

function Stop-PresentMonSession([string]$name) {
    # logman returns a non-zero code when the session does not exist. That is
    # the normal case before the first run, so ignore it.
    & logman stop $name -ets 2>&1 | Out-Null
}

function Test-PresentMonSession([string]$name) {
    & logman query $name -ets 2>&1 | Out-Null
    return $LASTEXITCODE -eq 0
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

function Invoke-SurfaceStateWait($surfaceScript, $dir, [int]$timeoutSeconds, [int]$minimumPct) {
    # The wait must never hold the runner. On 2026-11-11 the first wait hung
    # the runner for 36 minutes before the application started: the call did
    # not pass -Out, which used to be a mandatory parameter, so the hidden,
    # non-interactive console waited at a parameter prompt. The wait's own
    # 120 s cap never started. -Out is now optional, and this still passes a
    # path and runs the wait in its own powershell.exe with a wall-clock
    # limit: one blocked WMI read must not hold the runner either.
    $graceSeconds = 15
    $out = Join-Path $dir 'surface-state-wait.out'
    $err = Join-Path $dir 'surface-state-wait.err'
    $waitCsv = Join-Path $dir 'surface-state-wait.csv'
    $started = Get-Date
    $process = Start-Process -FilePath 'powershell.exe' -PassThru -WindowStyle Hidden `
        -ArgumentList @('-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
            '-File', "`"$surfaceScript`"", '-Out', "`"$waitCsv`"", '-WaitForProcessorPerformance',
            '-WaitMinimumPct', [string]$minimumPct, '-WaitTimeoutSeconds', [string]$timeoutSeconds) `
        -RedirectStandardOutput $out -RedirectStandardError $err
    $finished = $process.WaitForExit(($timeoutSeconds + $graceSeconds) * 1000)
    $elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
    if (-not $finished) {
        # Kill the blocked reader so the run can start. The result is recorded
        # as a timeout; the next run tries again.
        try { $process.Kill() } catch { }
        try { [void]$process.WaitForExit(5000) } catch { }
        Log ("surface state wait process stopped after $elapsed s")
    }
    $lines = @()
    if (Test-Path $out) {
        $lines = @(Get-Content -Path $out | Where-Object { $_ -match '^surface_state_wait' })
    }
    if (-not $finished -and -not ($lines | Where-Object { $_ -match '^surface_state_wait=' })) {
        $lines = @(
            'surface_state_wait=timeout',
            "surface_state_wait_seconds=$elapsed",
            'surface_state_wait_samples=0',
            'surface_state_wait_min_pct=unavailable',
            'surface_state_wait_last_pct=unavailable',
            'surface_state_wait_source=wmi',
            'surface_state_wait_timed_out=process'
        )
    }
    if (-not $lines.Count) {
        $message = ''
        if (Test-Path $err) { $message = (@(Get-Content -Path $err -TotalCount 2) -join ' ') }
        $lines = @(
            'surface_state_wait=failed',
            ('surface_state_wait_error=' + $message)
        )
        Log ("surface state wait failed: " + $message)
    }
    return $lines
}

function Set-TargetEnvironment($envMap) {
    # Environment variables from hosts.json reach the measured application.
    # They are applied to this PowerShell process before Start-Process, so no
    # value is ever part of a command line.
    if (-not $envMap) { return }
    foreach ($property in $envMap.PSObject.Properties) {
        $name = [string]$property.Name
        if ($name -notmatch '^[A-Za-z_][A-Za-z0-9_]*$') {
            throw "invalid environment variable name: $name"
        }
        Set-Item -Path ("Env:" + $name) -Value ([string]$property.Value)
    }
}

function Set-TargetKritarc($dir, $map) {
    # Apply root kritarc keys (the empty KConfig group) for one run. The file
    # is backed up next to meta.txt and restored after the application exits.
    # Root keys must go before the first [Group] line, so an existing key is
    # replaced in place and a new key is appended to the root block.
    if (-not $map) { return $null }
    $path = Join-Path $env:LOCALAPPDATA 'kritarc'
    $backup = Join-Path $dir 'kritarc-before.txt'
    $existed = Test-Path $path
    $lines = @()
    if ($existed) {
        Copy-Item -Force $path $backup
        $lines = @([System.IO.File]::ReadAllLines($path))
    }
    $section = $lines.Count
    for ($i = 0; $i -lt $lines.Count; $i++) {
        if ($lines[$i] -match '^\s*\[') { $section = $i; break }
    }
    $root = New-Object System.Collections.Generic.List[string]
    for ($i = 0; $i -lt $section; $i++) { $root.Add($lines[$i]) }
    $rest = New-Object System.Collections.Generic.List[string]
    for ($i = $section; $i -lt $lines.Count; $i++) { $rest.Add($lines[$i]) }

    $meta = @(
        'kritarc_override=applied',
        "kritarc_existed=$existed",
        "kritarc_backup=$backup"
    )
    foreach ($property in $map.PSObject.Properties) {
        $name = [string]$property.Name
        $value = [string]$property.Value
        $pattern = '^\s*' + [regex]::Escape($name) + '\s*=(.*)$'
        $found = $false
        $original = 'absent'
        for ($i = 0; $i -lt $root.Count; $i++) {
            $match = [regex]::Match($root[$i], $pattern)
            if ($match.Success) {
                if (-not $found) { $original = $match.Groups[1].Value.Trim() }
                $root[$i] = "$name=$value"
                $found = $true
            }
        }
        if (-not $found) { $root.Add("$name=$value") }
        $meta += "kritarc_original.$name=$original"
        $meta += "kritarc_override.$name=$value"
    }
    $out = @($root) + @($rest)
    [System.IO.File]::WriteAllLines($path, [string[]]$out, (New-Object System.Text.UTF8Encoding($false)))
    Add-Content -Path (Join-Path $dir 'meta.txt') -Value $meta
    return [pscustomobject]@{ Path = $path; Backup = $backup; Existed = $existed }
}

function Restore-TargetKritarc($dir, $state) {
    if (-not $state) { return }
    $ok = $true
    $message = ''
    try {
        if ($state.Existed) {
            Copy-Item -Force $state.Backup $state.Path
        } elseif (Test-Path $state.Path) {
            Remove-Item -Force $state.Path
        }
    } catch {
        $ok = $false
        $message = $_.Exception.Message
    }
    $lines = @("kritarc_restored=$ok")
    if (-not $ok) { $lines += "kritarc_restore_error=$message" }
    Add-Content -Path (Join-Path $dir 'meta.txt') -Value $lines
}

function Get-KritarcValues($names) {
    # Read root kritarc keys written by the application during the run. The
    # canvas records its OpenGL state there (canvasState), so a run that never
    # repainted can be told apart from one that never started OpenGL.
    $path = Join-Path $env:LOCALAPPDATA 'kritarc'
    $values = @{}
    if (-not (Test-Path $path)) { return $values }
    $section = $true
    foreach ($line in [System.IO.File]::ReadAllLines($path)) {
        if ($line -match '^\s*\[') { $section = $false }
        if (-not $section) { continue }
        foreach ($name in $names) {
            $match = [regex]::Match($line, '^\s*' + [regex]::Escape($name) + '\s*=(.*)$')
            if ($match.Success) { $values[$name] = $match.Groups[1].Value.Trim() }
        }
    }
    return $values
}

function Get-KritarcGroupValues($group, $names) {
    # Read keys of one named group (for example [General]). The scenario's
    # newOutlineStyle and fpsLimit live there, and the run must record the
    # values that were actually in effect, not the ones the scenario expects.
    $path = Join-Path $env:LOCALAPPDATA 'kritarc'
    $values = @{}
    if (-not (Test-Path $path)) { return $values }
    $current = ''
    foreach ($line in [System.IO.File]::ReadAllLines($path)) {
        $header = [regex]::Match($line, '^\s*\[(.*)\]\s*$')
        if ($header.Success) { $current = $header.Groups[1].Value; continue }
        if ($current -ne $group) { continue }
        foreach ($name in $names) {
            $match = [regex]::Match($line, '^\s*' + [regex]::Escape($name) + '\s*=(.*)$')
            if ($match.Success) { $values[$name] = $match.Groups[1].Value.Trim() }
        }
    }
    return $values
}

function Send-ZoomKeys($app, [int]$percent) {
    # Use the application shortcuts (krita_default.shortcuts): Ctrl+0 puts the
    # canvas at 100 %, and Ctrl+= steps through the standard zoom levels
    # 4/3, 2, 8/3, 4. The window must be in front; record whether it was.
    # Maximize the window so the 100 % capture shows most of the test image.
    # The zoom actions keep the image centre in the middle of the canvas, so
    # both captures show the centre of the image (the fixed position).
    [void][L1Probe]::ShowWindow($app.MainWindowHandle, 3)
    [void][L1Probe]::SetForegroundWindow($app.MainWindowHandle)
    Start-Sleep -Milliseconds 1000
    $shell = New-Object -ComObject WScript.Shell
    $shell.SendKeys('^0')
    Start-Sleep -Milliseconds 800
    if ($percent -ge 400) {
        for ($i = 0; $i -lt 4; $i++) {
            $shell.SendKeys('^{=}')
            Start-Sleep -Milliseconds 250
        }
        Start-Sleep -Milliseconds 800
    }
    $foreground = Get-Foreground
    return ($foreground.pid -eq $app.Id)
}

function Invoke-Frame($dir, [string]$name) {
    # Save one full desktop frame through windows_capture.exe. It uses the
    # same DXGI Desktop Duplication clock as the latency capture, so the
    # images contain exactly what the display showed.
    $tool = Join-Path $here 'windows_capture.exe'
    if (-not (Test-Path $tool)) { throw 'windows_capture.exe is missing' }
    $log = Join-Path $dir ("frame-$name.log")
    $err = Join-Path $dir ("frame-$name.err")
    $process = Start-Process -FilePath $tool -ArgumentList @('--out', "`"$dir`"", '--snapshot-frame', $name) `
        -PassThru -Wait -WindowStyle Hidden -RedirectStandardOutput $log -RedirectStandardError $err
    if ($process.ExitCode -ne 0) {
        throw "windows_capture --snapshot-frame failed: $(Get-Content $err -Raw)"
    }
}

function Invoke-DisplayCheck($dir, $app, [string]$image) {
    Add-Content -Path (Join-Path $dir 'meta.txt') -Value @(
        "display_check_image=$image",
        'display_check_captures=100,400',
        'display_check_window=maximized',
        'display_check_anchor=image-center'
    )
    $foreground = Send-ZoomKeys $app 100
    Add-Content -Path (Join-Path $dir 'meta.txt') -Value "display_check_foreground_100=$foreground"
    Invoke-Frame $dir 'display-100'
    $foreground = Send-ZoomKeys $app 400
    Add-Content -Path (Join-Path $dir 'meta.txt') -Value "display_check_foreground_400=$foreground"
    Invoke-Frame $dir 'display-400'
    Log 'display check captured 100 and 400'
}

function Write-Meta($dir, $request, $target, $surfaceStateMode, $surfaceStateWait) {
    $lines = @(
        "target=$target",
        "app=$($request.app)",
        "date=$(Get-Date -Format o)",
        "computer=$((Get-CimInstance Win32_ComputerSystem).Model)",
        "os=$([System.Environment]::OSVersion.VersionString)",
        "presentmon=$(if ($request.presentmon) { $request.presentmon } else { 'on' })",
        "surface_state_mode=$surfaceStateMode",
        "surface_state_wait_mode=$surfaceStateWait"
    )
    if ($request.env) {
        foreach ($property in $request.env.PSObject.Properties) {
            $lines += "env.$($property.Name)=$($property.Value)"
        }
    }
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

function New-CheckImage([string]$path, [int]$width, [int]$height, [switch]$Alpha) {
    Add-Type -AssemblyName System.Drawing
    $bitmap = New-Object System.Drawing.Bitmap $width, $height
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    if ($Alpha) {
        $graphics.Clear([System.Drawing.Color]::Transparent)
        $red = New-Object System.Drawing.SolidBrush -ArgumentList @([System.Drawing.Color]::FromArgb(255, 220, 40, 40))
        $green = New-Object System.Drawing.SolidBrush -ArgumentList @([System.Drawing.Color]::FromArgb(255, 40, 200, 80))
        $graphics.FillRectangle($red, 0, 0, [int]($width / 2), $height)
        $graphics.FillRectangle($green, [int]($width / 2), 0, [int]($width / 2), [int]($height / 2))
        $red.Dispose()
        $green.Dispose()
    } else {
        $graphics.Clear([System.Drawing.Color]::White)
        $graphics.FillRectangle([System.Drawing.Brushes]::Red, 40, 40, 240, 160)
        $graphics.FillRectangle([System.Drawing.Brushes]::Blue, 360, 80, 280, 200)
        $graphics.FillRectangle([System.Drawing.Brushes]::Green, 120, 320, 400, 180)
        $graphics.FillRectangle([System.Drawing.Brushes]::Black, 700, 100, 120, 120)
        $blackPen = New-Object System.Drawing.Pen -ArgumentList @([System.Drawing.Color]::Black, 8)
        $bluePen = New-Object System.Drawing.Pen -ArgumentList @([System.Drawing.Color]::DarkBlue, 6)
        $graphics.DrawLine($blackPen, 30, 600, $width - 30, 600)
        $graphics.DrawLine($bluePen, 60, 60, $width - 60, $height - 60)
        $blackPen.Dispose()
        $bluePen.Dispose()
    }
    $graphics.Dispose()
    $bitmap.Save($path, [System.Drawing.Imaging.ImageFormat]::Png)
    $bitmap.Dispose()
}

function Start-CheckApp([string[]]$images, [string]$probePath, [string]$dir, [string]$name) {
    $env:LIBREPAINT_LATENCY_LOG = $probePath
    $arguments = @('--nosplash')
    foreach ($image in $images) { $arguments += "`"$image`"" }
    $process = Start-Process -FilePath $request.app -ArgumentList $arguments -PassThru `
        -RedirectStandardOutput (Join-Path $dir "$name.out") `
        -RedirectStandardError (Join-Path $dir "$name.err")
    $handle = Wait-MainWindow $process 120
    return [pscustomobject]@{ Process = $process; Handle = $handle }
}

function Stop-CheckApp($app) {
    if (-not $app) { return }
    try {
        $app.Process.Refresh()
        if (-not $app.Process.HasExited) {
            Stop-Process -Id $app.Process.Id -Force
            [void]$app.Process.WaitForExit(5000)
        }
    } catch { }
}

function Wait-CheckTitle($app, [string]$match, [int]$seconds) {
    $deadline = (Get-Date).AddSeconds($seconds)
    $title = ''
    while ((Get-Date) -lt $deadline) {
        try { $title = [Q4Check]::WindowText($app.Handle) } catch { $title = '' }
        if ($match -and $title -match $match) { return $title }
        if (-not $match -and $title) { return $title }
        Start-Sleep -Milliseconds 250
    }
    return $title
}

function Invoke-CheckSnapshot([string]$dir) {
    $tool = Join-Path $here 'windows_capture.exe'
    $process = Start-Process -FilePath $tool -ArgumentList @('--out', "`"$dir`"", '--snapshot') `
        -PassThru -Wait -WindowStyle Hidden -RedirectStandardOutput (Join-Path $dir 'snapshot.out') `
        -RedirectStandardError (Join-Path $dir 'snapshot.err')
    if ($process.ExitCode -ne 0) { return $null }
    $path = Join-Path $dir 'canvas.json'
    if (-not (Test-Path $path)) { return $null }
    try { return (Get-Content $path -Raw | ConvertFrom-Json) } catch { return $null }
}

function Invoke-CheckFrame([string]$dir, [string]$name) {
    Invoke-Frame $dir $name
    return (Join-Path $dir "$name.bmp")
}

function Invoke-SettingsDialog($main, [int]$processId, [string]$dir) {
    $result = [ordered]@{ opened = $false; method = 'none'; handle = 0; title = '' }
    try {
        Add-Type -AssemblyName UIAutomationClient
        Add-Type -AssemblyName UIAutomationTypes
        $root = [System.Windows.Automation.AutomationElement]::FromHandle($main)
        $menuCondition = New-Object System.Windows.Automation.PropertyCondition(
            [System.Windows.Automation.AutomationElement]::ControlTypeProperty,
            [System.Windows.Automation.ControlType]::MenuItem)
        $items = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, $menuCondition)
        $settings = $null
        foreach ($item in $items) {
            if ($item.Current.Name -match ('^(Settings|' + $jaSettings + ')$')) { $settings = $item; break }
        }
        if ($settings) {
            try {
                $expand = $settings.GetCurrentPattern([System.Windows.Automation.ExpandCollapsePattern]::Pattern)
                $expand.Expand()
                Start-Sleep -Milliseconds 500
            } catch { }
            $items = $root.FindAll([System.Windows.Automation.TreeScope]::Descendants, $menuCondition)
            foreach ($item in $items) {
                if ($item.Current.Name -match ('Configure|Preferences|' + $jaSettings)) {
                    try {
                        $invoke = $item.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern)
                        $invoke.Invoke()
                        $result.opened = $true
                        $result.method = 'uia'
                        break
                    } catch { }
                }
            }
        }
    } catch { }
    if (-not $result.opened) {
        try {
            [void][Q4Check]::BringToFront($main)
            $shell = New-Object -ComObject WScript.Shell
            $shell.SendKeys('%s')
            Start-Sleep -Milliseconds 500
            $shell.SendKeys('c')
            Start-Sleep -Milliseconds 1200
            $result.method = 'keys'
        } catch { }
    } else {
        Start-Sleep -Milliseconds 1200
    }
    $windows = [Q4Check]::WindowList($processId)
    Set-Content -Path (Join-Path $dir 'dialog-windows.txt') -Value $windows
    $best = $null
    $bestArea = 0
    foreach ($line in ($windows -split "`r?`n")) {
        if (-not $line) { continue }
        $parts = $line.Split('|')
        if ($parts.Count -lt 6) { continue }
        if ([int64]$parts[0] -eq $main.ToInt64()) { continue }
        if ($parts[1] -match 'tooltips|IME|MSCTF') { continue }
        if (-not $parts[2]) { continue }
        $size = $parts[5].Split('x')
        if ($size.Count -ne 2) { continue }
        $area = [int]$size[0] * [int]$size[1]
        if ($area -gt $bestArea) { $bestArea = $area; $best = $parts }
    }
    if ($best) {
        $result.opened = $true
        if ($result.method -eq 'none') { $result.method = 'window' }
        $result.handle = [int64]$best[0]
        $result.title = $best[2]
    }
    Add-Content -Path (Join-Path $dir 'dialog.txt') -Value ($result.GetEnumerator() | ForEach-Object { "$($_.Key)=$($_.Value)" })
    return $result
}

function Invoke-Q4Check($request, [string]$target) {
    $runName = if ($request.name) { [string]$request.name } else { 'q4-e2e' }
    $id = '{0}-{1}' -f $runName, (Get-Date -Format 'yyyyMMdd-HHmmss')
    $dir = Join-Path $here ("runs\{0}-1" -f $id)
    New-Item -ItemType Directory -Force $dir | Out-Null
    $script:runStatus = Join-Path $dir 'status.txt'
    Set-Content -Path $script:runStatus -Value 'starting'
    Log "check $dir"
    Write-Meta $dir $request $target 'off' 'off'
    $errors = New-Object System.Collections.Generic.List[string]
    $facts = [ordered]@{
        scenario = 'q4-e2e'
        app = $request.app
        date = (Get-Date -Format o)
        computer = (Get-CimInstance Win32_ComputerSystem).Model
        os = [System.Environment]::OSVersion.VersionString
        errors = @()
    }
    Add-Type -Path (Join-Path $here 'Q4Check.cs')

    $docA = Join-Path $dir 'doc-a.png'
    $docB = Join-Path $dir 'doc-b.png'
    $clipSource = Join-Path $dir 'clipboard-source.png'
    New-CheckImage $docA 1024 768
    New-CheckImage $docB 1024 768
    New-CheckImage $clipSource 96 64 -Alpha
    $penDir = Join-Path $dir 'pen'
    New-Item -ItemType Directory -Force $penDir | Out-Null
    $app = $null
    try {
        $app = Start-CheckApp @($docA, $docB) (Join-Path $penDir 'probe.csv') $dir 'app'
        $initialTitle = Wait-CheckTitle $app 'doc-a|doc-b' 30
        [void][Q4Check]::BringToFront($app.Handle)
        Start-Sleep -Seconds 2
        # The second document may be active after the start. Bring document A
        # to the front before the clipboard and tab steps so their order is
        # fixed.
        for ($i = 0; $i -lt 4; $i++) {
            $current = [Q4Check]::WindowText($app.Handle)
            if ($current -match [regex]::Escape([System.IO.Path]::GetFileNameWithoutExtension($docA))) { break }
            [void][Q4Check]::SendCtrl([uint32]0x09)
            Start-Sleep -Milliseconds 500
        }

        # (a) Clipboard: select all and copy the alpha test image.
        try {
            $cleared = [Q4Check]::ClearClipboard()
            [void][Q4Check]::SendCtrl([uint32]0x41)
            Start-Sleep -Milliseconds 400
            [void][Q4Check]::SendCtrl([uint32]0x43)
            Start-Sleep -Milliseconds 1200
            $saved = [Q4Check]::SaveClipboardFormats($dir, $clipSource)
            $facts.clipboard = [ordered]@{ cleared = $cleared; saved = $saved;
                formats = 'clipboard-formats.txt'; png = 'clipboard-png.png';
                source = 'clipboard-source.png' }
        } catch { $errors.Add('clipboard: ' + $_.Exception.Message) }

        # (b) Tab titles: two documents, switch tabs and read the window title.
        try {
            $titleInitial = [Q4Check]::WindowText($app.Handle)
            [void][Q4Check]::SendCtrl([uint32]0x09)
            Start-Sleep -Milliseconds 800
            $titleAfter = [Q4Check]::WindowText($app.Handle)
            [void][Q4Check]::SendCtrl([uint32]0x09)
            Start-Sleep -Milliseconds 800
            $titleBack = [Q4Check]::WindowText($app.Handle)
            $lines = @("initial=$titleInitial", "after=$titleAfter", "back=$titleBack",
                "expected_a=$([System.IO.Path]::GetFileNameWithoutExtension($docA))",
                "expected_b=$([System.IO.Path]::GetFileNameWithoutExtension($docB))")
            Set-Content -Path (Join-Path $dir 'tabs.txt') -Value $lines
            $facts.tabs = [ordered]@{ file = 'tabs.txt'; initial = $titleInitial;
                after = $titleAfter; back = $titleBack }
        } catch { $errors.Add('tabs: ' + $_.Exception.Message) }

        # (c) Japanese layout: load 00000411 and send Ctrl+Tab with SendInput.
        try {
            $layout = [Q4Check]::LoadJapaneseLayout($app.Handle)
            $titleBefore = [Q4Check]::WindowText($app.Handle)
            $sent = [Q4Check]::SendCtrl([uint32]0x09)
            Start-Sleep -Milliseconds 800
            $titleAfter = [Q4Check]::WindowText($app.Handle)
            $states = [Q4Check]::KeyStates()
            $lines = @("layout=$layout", "send=$sent", "title_before=$titleBefore",
                "title_after=$titleAfter", "key_states=$states",
                "modifier_down=$([Q4Check]::ModifierDown($states))")
            Set-Content -Path (Join-Path $dir 'keyboard.txt') -Value $lines
            $facts.keyboard = [ordered]@{ file = 'keyboard.txt'; layout = $layout;
                send = $sent; title_before = $titleBefore; title_after = $titleAfter;
                key_states = $states; modifier_down = [Q4Check]::ModifierDown($states) }
        } catch { $errors.Add('keyboard: ' + $_.Exception.Message) }

        # (d) Flicker: record frames while resizing the window and opening and
        # closing the settings dialog. The development host counts blank and
        # unchanged frames and compares the counts with the fork version.
        try {
            [void][Q4Check]::BringToFront($app.Handle)
            Start-Sleep -Milliseconds 800
            $snapshot = Invoke-CheckSnapshot $dir
            $canvas = @(0, 0, 0, 0)
            if ($snapshot -and $snapshot.canvas) { $canvas = @($snapshot.canvas) }
            $tool = Join-Path $here 'windows_capture.exe'
            $monitorOut = Join-Path $dir 'monitor.out'
            $monitorErr = Join-Path $dir 'monitor.err'
            $monitor = Start-Process -FilePath $tool -ArgumentList @('--monitor', '--out', "`"$dir`"",
                '--seconds', '18', '--window', ("0x{0:X}" -f $app.Handle.ToInt64()),
                '--canvas', ("{0},{1},{2},{3}" -f $canvas[0], $canvas[1], $canvas[2], $canvas[3])) `
                -PassThru -WindowStyle Hidden -RedirectStandardOutput $monitorOut -RedirectStandardError $monitorErr
            Start-Sleep -Milliseconds 800
            $log = New-Object System.Collections.Generic.List[string]
            foreach ($size in @(@(1400, 900), @(1100, 700), @(0, 0), @(1600, 1000), @(0, 0))) {
                $start = [Q4Check]::NowTicks()
                $detail = [Q4Check]::ResizeWindow($app.Handle, [int]$size[0], [int]$size[1])
                Start-Sleep -Milliseconds 500
                $log.Add("action=resize start=$start end=$([Q4Check]::NowTicks()) detail=$detail")
            }
            $dialog = Invoke-SettingsDialog $app.Handle $app.Process.Id $dir
            Start-Sleep -Milliseconds 700
            if ($dialog.handle -ne 0) {
                [void][Q4Check]::CloseWindow([IntPtr]$dialog.handle)
            } else {
                try { (New-Object -ComObject WScript.Shell).SendKeys('{ESC}') } catch { }
            }
            Start-Sleep -Milliseconds 1200
            foreach ($size in @(@(1200, 800), @(0, 0))) {
                $start = [Q4Check]::NowTicks()
                $detail = [Q4Check]::ResizeWindow($app.Handle, [int]$size[0], [int]$size[1])
                Start-Sleep -Milliseconds 500
                $log.Add("action=resize start=$start end=$([Q4Check]::NowTicks()) detail=$detail")
            }
            $log.Add("action=dialog start=$($dialog.opened) end=$([Q4Check]::NowTicks()) detail=$($dialog.title)")
            Set-Content -Path (Join-Path $dir 'flicker-log.txt') -Value $log
            if (-not $monitor.WaitForExit(30000)) {
                try { Stop-Process -Id $monitor.Id -Force } catch { }
                $errors.Add('flicker: monitor did not exit')
            }
            $monitorExit = -1
            try { $monitor.Refresh(); $monitorExit = $monitor.ExitCode } catch { }
            $facts.flicker = [ordered]@{ csv = 'flicker.csv'; log = 'flicker-log.txt';
                monitor_exit = $monitorExit; canvas = $canvas;
                dialog_opened = $dialog.opened; dialog_method = $dialog.method;
                dialog_title = $dialog.title }
        } catch { $errors.Add('flicker: ' + $_.Exception.Message) }

        # (e) Pen: replay the stage 2/3 checks that synthetic input can reach.
        try {
            [void][Q4Check]::BringToFront($app.Handle)
            Start-Sleep -Milliseconds 800
            $penSnapshot = Invoke-CheckSnapshot $penDir
            $penCanvas = @(0, 0, 0, 0)
            if ($penSnapshot -and $penSnapshot.canvas) { $penCanvas = @($penSnapshot.canvas) }
            $started = [Q4Check]::PenStart((Join-Path $penDir 'pen-events.txt'))
            $events = New-Object System.Collections.Generic.List[string]
            $events.Add("action=pen-start qpc=$([Q4Check]::NowTicks()) detail=$started")

            $events.Add("action=enter qpc=$([Q4Check]::NowTicks()) detail=inside")
            [void][Q4Check]::PenHover($app.Handle, 0.5, 0.5, 500)
            Start-Sleep -Milliseconds 300
            Invoke-CheckFrame $penDir 'inside' | Out-Null
            $events.Add("action=leave qpc=$([Q4Check]::NowTicks()) detail=titlebar")
            [void][Q4Check]::PenHover($app.Handle, 0.5, -0.015, 800)
            Start-Sleep -Milliseconds 300
            Invoke-CheckFrame $penDir 'left' | Out-Null
            [void][Q4Check]::PenLeave($app.Handle, 0.5, -0.015)

            $events.Add("action=edge-start qpc=$([Q4Check]::NowTicks()) detail=right-edge")
            [void][Q4Check]::PenStroke($app.Handle, 0.9, 0.45, 1.02, 0.45, 350)
            Start-Sleep -Milliseconds 500
            Invoke-CheckFrame $penDir 'edge' | Out-Null
            $events.Add("action=edge-end qpc=$([Q4Check]::NowTicks()) detail=right-edge")

            $events.Add("action=single-start qpc=$([Q4Check]::NowTicks()) detail=one-line")
            [void][Q4Check]::PenStroke($app.Handle, 0.2, 0.25, 0.8, 0.25, 350)
            Start-Sleep -Milliseconds 500
            Invoke-CheckFrame $penDir 'single' | Out-Null
            $events.Add("action=single-end qpc=$([Q4Check]::NowTicks()) detail=one-line")

            $events.Add("action=hold-start qpc=$([Q4Check]::NowTicks()) detail=long-press")
            [void][Q4Check]::PenDownAt($app.Handle, 0.5, 0.7)
            [void][Q4Check]::PenHold($app.Handle, 0.5, 0.7, 1200)
            $popupLines = @([Q4Check]::WindowList($app.Process.Id) -split "`r?`n" |
                Where-Object { $_ -and ([int64]($_ -split '\|')[0] -ne $app.Handle.ToInt64()) })
            Set-Content -Path (Join-Path $penDir 'popups.txt') -Value $popupLines
            [void][Q4Check]::PenUpAt($app.Handle, 0.5, 0.7)
            Start-Sleep -Milliseconds 500
            Invoke-CheckFrame $penDir 'longpress' | Out-Null
            $events.Add("action=hold-end qpc=$([Q4Check]::NowTicks()) detail=long-press")

            $events.Add("action=dot-start qpc=$([Q4Check]::NowTicks()) detail=one-dot")
            [void][Q4Check]::PenTap($app.Handle, 0.3, 0.6, 120)
            Start-Sleep -Milliseconds 500
            Invoke-CheckFrame $penDir 'dot' | Out-Null
            $events.Add("action=dot-end qpc=$([Q4Check]::NowTicks()) detail=one-dot")

            $events.Add("action=return-start qpc=$([Q4Check]::NowTicks()) detail=inside")
            [void][Q4Check]::PenHover($app.Handle, 0.5, 0.5, 500)
            Start-Sleep -Milliseconds 300
            $cursorInside = [Q4Check]::CursorInfo()
            $events.Add("action=return-title qpc=$([Q4Check]::NowTicks()) detail=titlebar")
            [void][Q4Check]::PenHover($app.Handle, 0.5, -0.015, 600)
            Start-Sleep -Milliseconds 300
            $cursorTitle = [Q4Check]::CursorInfo()
            $events.Add("action=return-inside qpc=$([Q4Check]::NowTicks()) detail=inside")
            [void][Q4Check]::PenHover($app.Handle, 0.5, 0.5, 700)
            Start-Sleep -Milliseconds 400
            $cursorReturn = [Q4Check]::CursorInfo()
            Invoke-CheckFrame $penDir 'return' | Out-Null
            $events.Add("action=return-end qpc=$([Q4Check]::NowTicks()) detail=inside")
            Set-Content -Path (Join-Path $penDir 'cursors.txt') -Value @(
                "inside=$cursorInside", "titlebar=$cursorTitle", "return=$cursorReturn")
            Set-Content -Path (Join-Path $penDir 'actions.txt') -Value $events
            Start-Sleep -Milliseconds 1500
            [Q4Check]::PenStop()
            $facts.pen = [ordered]@{ dir = 'pen'; probe = 'pen/probe.csv';
                events = 'pen/pen-events.txt'; actions = 'pen/actions.txt'; canvas = $penCanvas;
                captures = [ordered]@{ inside = 'pen/inside.bmp'; left = 'pen/left.bmp';
                    edge = 'pen/edge.bmp'; single = 'pen/single.bmp';
                    longpress = 'pen/longpress.bmp'; dot = 'pen/dot.bmp'; return = 'pen/return.bmp' };
                popups = 'pen/popups.txt'; cursors = 'pen/cursors.txt' }
        } catch { $errors.Add('pen: ' + $_.Exception.Message) }
    } catch { $errors.Add('app: ' + $_.Exception.Message) }
    finally { Stop-CheckApp $app }

    # First line right after a fresh start (stage 1/2 item 17).
    try {
        $firstDir = Join-Path $dir 'first'
        New-Item -ItemType Directory -Force $firstDir | Out-Null
        $first = $null
        try {
            $first = Start-CheckApp @($docA) (Join-Path $firstDir 'probe.csv') $firstDir 'first-app'
            Start-Sleep -Milliseconds 500
            [void][Q4Check]::BringToFront($first.Handle)
            $started = [Q4Check]::PenStart((Join-Path $firstDir 'pen-events.txt'))
            $start = [Q4Check]::NowTicks()
            [void][Q4Check]::PenStroke($first.Handle, 0.2, 0.5, 0.8, 0.5, 300)
            $end = [Q4Check]::NowTicks()
            Start-Sleep -Milliseconds 500
            Invoke-CheckFrame $firstDir 'line' | Out-Null
            Start-Sleep -Milliseconds 1500
            [Q4Check]::PenStop()
            Set-Content -Path (Join-Path $firstDir 'actions.txt') -Value @(
                "pen_start=$started", "stroke_start=$start", "stroke_end=$end")
            $facts.first_line = [ordered]@{ dir = 'first'; probe = 'first/probe.csv';
                events = 'first/pen-events.txt'; actions = 'first/actions.txt';
                capture = 'first/line.bmp'; stroke_start = $start; stroke_end = $end }
        } finally { Stop-CheckApp $first }
    } catch { $errors.Add('first-line: ' + $_.Exception.Message) }

    $facts.errors = @($errors)
    $facts | ConvertTo-Json -Depth 8 | Set-Content -Path (Join-Path $dir 'facts.json')
    Set-Content -Path (Join-Path $dir 'meta.txt') -Value @(
        "target=$target", "app=$($request.app)", "date=$(Get-Date -Format o)",
        "check=q4-e2e", "errors=$($errors.Count)",
        "computer=$((Get-CimInstance Win32_ComputerSystem).Model)",
        "os=$([System.Environment]::OSVersion.VersionString)")
    Log "check done errors=$($errors.Count)"
    Log 'done'
}

try {
    $request = Get-Content (Join-Path $here 'request.json') -Raw | ConvertFrom-Json
    $target = [string]$request.target
    $runs = [int]$request.runs
    $settle = if ($request.settleSeconds) { [int]$request.settleSeconds } else { 12 }

    if ($request.check) {
        Invoke-Q4Check $request $target
        return
    }

    $capture = [bool]$request.capture
    $displayCheck = $request.displayCheck
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
    $presentMonEnabled = (-not ([string]$request.presentmon -eq 'off')) -and (-not $displayCheck)
    # Device-state recording is off unless the target asks for it: the
    # per-second WMI samples were suspected of changing the device's power
    # state during a measurement (2026-11-11 05:40).
    $surfaceStateMode = if ($request.surface_state) { [string]$request.surface_state } else { 'off' }
    if ($surfaceStateMode -notin @('off', 'on', 'light')) {
        Log ("unknown surface_state '" + $surfaceStateMode + "', recording nothing")
        $surfaceStateMode = 'off'
    }
    # Waiting for the device to leave a throttled state is a setting of its
    # own: it uses one Get-Counter read per five seconds before the run and
    # changes nothing during it. It stays on when the state recording is off.
    $surfaceStateWait = if ($null -ne $request.surface_state_wait) { [string]$request.surface_state_wait } else { 'on' }
    if ($surfaceStateWait -notin @('off', 'on')) {
        Log ("unknown surface_state_wait '" + $surfaceStateWait + "', waiting")
        $surfaceStateWait = 'on'
    }
    $surfaceScript = Join-Path $here 'surface_state.ps1'
    # Per-thread CPU is a runner setting and off unless the target asks for
    # it, because it adds a sampling process to every run.
    $threadCpuMode = if ($request.thread_cpu) { [string]$request.thread_cpu } else { 'off' }
    if ($threadCpuMode -notin @('off', 'on')) {
        Log ("unknown thread_cpu '" + $threadCpuMode + "', recording nothing")
        $threadCpuMode = 'off'
    }
    $threadScript = Join-Path $here 'thread_cpu.ps1'
    # Battery power is a runner setting and off unless the target asks for it.
    # It samples Win32_Battery and root\wmi once a second, so a latency run
    # without a power comparison does not pay for it.
    $powerMode = if ($request.power) { [string]$request.power } else { 'off' }
    if ($powerMode -notin @('off', 'on')) {
        Log ("unknown power '" + $powerMode + "', recording nothing")
        $powerMode = 'off'
    }
    $powerScript = Join-Path $here 'power.ps1'
    $runName = if ($request.name) { [string]$request.name } else { $target }
    $id = '{0}-{1}' -f $runName, (Get-Date -Format 'yyyyMMdd-HHmmss')

    for ($n = 1; $n -le $runs; $n++) {
        $dir = Join-Path $here ("runs\{0}-{1}" -f $id, $n)
        New-Item -ItemType Directory -Force $dir | Out-Null
        $script:runStatus = Join-Path $dir 'status.txt'
        Set-Content -Path $script:runStatus -Value 'starting'
        Log "run $n $dir"
        Write-Meta $dir $request $target $surfaceStateMode $surfaceStateWait
        Add-Content -Path (Join-Path $dir 'meta.txt') -Value "thread_cpu_mode=$threadCpuMode"
        Add-Content -Path (Join-Path $dir 'meta.txt') -Value "power_mode=$powerMode"
        Write-LaunchFacts $dir 'start' $null
        # Wait for the device to recover before it is measured. The wait runs
        # in a separate process before the sampler and the application start,
        # so a throttled device delays the run instead of changing its values,
        # and a blocked counter read stops at a hard limit instead of holding
        # the runner.
        if ($surfaceStateWait -eq 'on' -and (Test-Path $surfaceScript)) {
            try {
                $waitLines = @(Invoke-SurfaceStateWait $surfaceScript $dir 120 70)
                if ($waitLines.Count) {
                    Add-Content -Path (Join-Path $dir 'meta.txt') -Value $waitLines
                    Log ("surface state wait: " + ($waitLines -join ' '))
                }
            } catch {
                Add-Content -Path (Join-Path $dir 'meta.txt') -Value @(
                    'surface_state_wait=failed',
                    ("surface_state_wait_error=" + $_.Exception.Message)
                )
                Log ("surface state wait failed: " + $_.Exception.Message)
            }
        } else {
            Add-Content -Path (Join-Path $dir 'meta.txt') -Value @(
                'surface_state_wait=off',
                'surface_state_wait_seconds=0'
            )
        }

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

        $sessionName = if ($request.presentmon_session) { [string]$request.presentmon_session } else { 'LibrePaintPenLatency' }
        $presentMonCsv = Join-Path $dir 'presentmon.csv'
        $presentMonErr = Join-Path $dir 'presentmon.err'
        $presentMonOut = Join-Path $dir 'presentmon.out'
        $pm = $null
        $sessionStarted = $false
        # PresentMon can stop consuming events while the application keeps
        # running (2026-11-10: a 1.2 s CSV for a 33 s run). Record whether the
        # process itself exited before the runner stopped it, and when.
        $pmExitedBeforeStop = $false
        $pmStartedAt = $null
        $pmStoppedAt = $null

        Set-TargetEnvironment $request.env
        if ($target -eq 'LibrePaint') { $env:LIBREPAINT_LATENCY_LOG = Join-Path $dir 'probe.csv' }
        else { Remove-Item Env:LIBREPAINT_LATENCY_LOG -ErrorAction SilentlyContinue }
        $kritarcState = Set-TargetKritarc $dir $request.kritarc
        # Record the values that are actually in effect after the target's
        # overrides. The scenario's newOutlineStyle and fpsLimit are not
        # written by the Windows runner, so the run has to state what it read
        # instead of assuming the scenario was applied.
        $effective = Get-KritarcGroupValues 'General' @('newOutlineStyle', 'fpsLimit')
        $effectiveLines = @()
        foreach ($key in @('newOutlineStyle', 'fpsLimit')) {
            $value = if ($effective -and $effective.ContainsKey($key)) { $effective[$key] } else { 'absent' }
            $effectiveLines += "kritarc_effective.$key=$value"
        }
        Add-Content -Path (Join-Path $dir 'meta.txt') -Value $effectiveLines
        # Device state: one sample before the application starts, one per
        # second while the run happens, and one after the application exits.
        # The sampler is a separate process so it keeps sampling while this
        # script waits for the capture tool.
        $surfaceCsv = Join-Path $dir 'surface-state.csv'
        $surfaceStop = Join-Path $dir 'surface-state.stop'
        $surfaceSampler = $null
        if ($surfaceStateMode -ne 'off' -and (Test-Path $surfaceScript)) {
            try {
                & $surfaceScript -Out $surfaceCsv -Phase before -Samples 1
            } catch {
                Log ("surface state before failed: " + $_.Exception.Message)
            }
            try {
                $surfaceSampler = Start-Process -FilePath 'powershell.exe' -PassThru -WindowStyle Hidden `
                    -ArgumentList @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$surfaceScript`"",
                        '-Out', "`"$surfaceCsv`"", '-Phase', 'during', '-IntervalMs', '1000', '-Mode', $surfaceStateMode,
                        '-StopFile', "`"$surfaceStop`"") `
                    -RedirectStandardOutput (Join-Path $dir 'surface-state.out') `
                    -RedirectStandardError (Join-Path $dir 'surface-state.err')
            } catch {
                Log ("surface state sampler did not start: " + $_.Exception.Message)
            }
        }
        $appImage = if ($displayCheck) { [string]$displayCheck.image } else { $canvas }
        $app = $null
        $threadSampler = $null
        $threadStop = Join-Path $dir 'threads.stop'
        $threadCsv = Join-Path $dir 'threads.csv'
        $powerSampler = $null
        $powerStop = Join-Path $dir 'power.stop'
        $powerCsv = Join-Path $dir 'power.csv'
        $appOut = Join-Path $dir 'app.out'
        $appErr = Join-Path $dir 'app.err'
        try {
            # Battery sampling starts before the application so the CSV has a
            # sample at the start as well as one per second through the input
            # window. It is one extra process reading the battery counters;
            # the mode is off by default.
            if ($powerMode -eq 'on' -and (Test-Path $powerScript)) {
                try {
                    $powerSampler = Start-Process -FilePath 'powershell.exe' -PassThru -WindowStyle Hidden `
                        -ArgumentList @('-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
                            '-File', "`"$powerScript`"", '-Out', "`"$powerCsv`"", '-IntervalMs', '1000',
                            '-StopFile', "`"$powerStop`"") `
                        -RedirectStandardOutput (Join-Path $dir 'power.out') `
                        -RedirectStandardError (Join-Path $dir 'power.err')
                } catch {
                    Log ("power sampler did not start: " + $_.Exception.Message)
                }
            }

            # Keep the application's own log: it names the OpenGL mode, failed
            # plugins and the canvas state, which the probe cannot show. A run
            # whose canvas stopped repainting (2026-11-10 22:55) needs these
            # lines to tell a failed GL start from a stalled update path.
            $app = Start-Process -FilePath $request.app -ArgumentList @('--nosplash', "`"$appImage`"") -PassThru `
                -RedirectStandardOutput $appOut -RedirectStandardError $appErr
            $hwnd = Wait-MainWindow $app 120
            Write-LaunchFacts $dir 'window' $app

            # Per-thread CPU sampling starts with the window and runs until
            # the application exits. It is one extra process reading the
            # process's own thread counters; the mode is off by default.
            if ($threadCpuMode -eq 'on' -and (Test-Path $threadScript)) {
                try {
                    $threadSampler = Start-Process -FilePath 'powershell.exe' -PassThru -WindowStyle Hidden `
                        -ArgumentList @('-NoProfile', '-NonInteractive', '-ExecutionPolicy', 'Bypass',
                            '-File', "`"$threadScript`"", '-ProcessId', [string]$app.Id,
                            '-Out', "`"$threadCsv`"", '-IntervalMs', '500',
                            '-StopFile', "`"$threadStop`"") `
                        -RedirectStandardOutput (Join-Path $dir 'thread-cpu.out') `
                        -RedirectStandardError (Join-Path $dir 'thread-cpu.err')
                } catch {
                    Log ("thread CPU sampler did not start: " + $_.Exception.Message)
                }
            }

            # Start PresentMon after the app window exists: --process_id is
            # then valid and --terminate_on_proc_exit sees the process exit.
            # One fixed session name (see presentmon_command.py) is stopped
            # before and after each run so that a killed PresentMon cannot
            # leave a session behind.
            if ($presentMonEnabled) {
                Stop-PresentMonSession $sessionName
                Get-Process -Name 'PresentMon*' -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue
                Start-Sleep -Milliseconds 500
                $argTemplate = if ($request.presentmon_arguments) { @($request.presentmon_arguments) } else {
                    @('--process_id', '{pid}', '--output_file', '{output}', '--qpc_time',
                        '--no_console_stats', '--terminate_on_proc_exit', '--session_name', $sessionName)
                }
                $pmArgs = @($argTemplate | ForEach-Object {
                    ([string]$_).Replace('{pid}', [string]$app.Id).Replace('{output}', $presentMonCsv)
                })
                $pm = Start-Process -FilePath $presentMon -ArgumentList $pmArgs -PassThru -WindowStyle Hidden `
                    -RedirectStandardError $presentMonErr -RedirectStandardOutput $presentMonOut
                $pmStartedAt = Get-Date
                for ($i = 0; $i -lt 40; $i++) {
                    if (Test-PresentMonSession $sessionName) { $sessionStarted = $true; break }
                    if ($pm.HasExited) { break }
                    Start-Sleep -Milliseconds 250
                }
                if (-not $sessionStarted) { Log 'presentmon session did not start' }
            }

            Start-Sleep -Seconds $settle
            Write-LaunchFacts $dir 'capture' $app
            if ($displayCheck) { Invoke-DisplayCheck $dir $app ([string]$displayCheck.image) }
            elseif ($capture) {
                # The capture tool injects the pointer itself (it replays the
                # plan without PenInject.cs), so the foreground has to be read
                # here, right before the tool starts its first sample. Without
                # this, every capture-mode run reported foreground_inject
                # unknown and a run that lost the foreground looked like a tool
                # that dropped the press.
                $injectForeground = Get-Foreground
                Add-Content -Path (Join-Path $dir 'meta.txt') -Value ("foreground_inject_start=pid={0} name={1} visible={2} same_window={3} app={4} retried=False" -f `
                    $injectForeground.pid, $injectForeground.name, $injectForeground.visible,
                    ($injectForeground.hwnd -eq $hwnd), ($injectForeground.pid -eq $app.Id))
                Invoke-Capture $dir
            }
            else {
                [PenInject]::Run($hwnd, (Join-Path $dir 'inject.csv'), $options) | Out-Null
                # PenInject records the window in front right before the first
                # sample (and after one retry), so the run separates "the app
                # was in front when the capture began" from "it was still in
                # front when the input began".
                Add-Content -Path (Join-Path $dir 'meta.txt') `
                    -Value "foreground_inject_start=$([PenInject]::ForegroundInjectStart)"
            }
            # The probe writer flushes every 500 ms; leave time for the last batch.
            Start-Sleep -Milliseconds 1500
        }
        finally {
            if ($app -and -not $app.HasExited) {
                Stop-Process -Id $app.Id -Force
                [void]$app.WaitForExit(5000)
            }
            if ($threadSampler) {
                try { Set-Content -Path $threadStop -Value 'stop' } catch { }
                try {
                    if (-not $threadSampler.WaitForExit(15000)) {
                        Stop-Process -Id $threadSampler.Id -Force -ErrorAction SilentlyContinue
                        [void]$threadSampler.WaitForExit(5000)
                    }
                } catch { }
            }
            if ($powerSampler) {
                try { Set-Content -Path $powerStop -Value 'stop' } catch { }
                try {
                    if (-not $powerSampler.WaitForExit(15000)) {
                        Stop-Process -Id $powerSampler.Id -Force -ErrorAction SilentlyContinue
                        [void]$powerSampler.WaitForExit(5000)
                    }
                } catch { }
            }
            if ($pm) {
                try {
                    $pm.Refresh()
                    $pmExitedBeforeStop = $pm.HasExited
                } catch { }
                # Stop the session first: ProcessTrace then returns, PresentMon
                # flushes the CSV and exits even when it missed the process
                # exit event.
                Stop-PresentMonSession $sessionName
                if (-not $pm.WaitForExit(15000)) {
                    Stop-Process -Id $pm.Id -Force -ErrorAction SilentlyContinue
                    [void]$pm.WaitForExit(5000)
                }
                $pmStoppedAt = Get-Date
            }
            Restore-TargetKritarc $dir $kritarcState
            # Stop the device sampler after the application is gone, so the
            # last samples describe the end of the run.
            if ($surfaceSampler) {
                try { Set-Content -Path $surfaceStop -Value 'stop' } catch { }
                try {
                    if (-not $surfaceSampler.WaitForExit(15000)) {
                        Stop-Process -Id $surfaceSampler.Id -Force -ErrorAction SilentlyContinue
                        [void]$surfaceSampler.WaitForExit(5000)
                    }
                } catch { }
            }
        }
        if ($threadCpuMode -eq 'on') {
            $threadLines = @(
                "thread_cpu_csv_bytes=$(if (Test-Path $threadCsv) { (Get-Item $threadCsv).Length } else { 0 })",
                "thread_cpu_samples=$(if (Test-Path $threadCsv) { [math]::Max(0, @(Get-Content $threadCsv | Where-Object { $_ -and -not $_.StartsWith('#') }).Count - 1) } else { 0 })",
                "thread_cpu_process_exit=$(if ($threadSampler) { $threadSampler.HasExited } else { 'not-started' })"
            )
            Add-Content -Path (Join-Path $dir 'meta.txt') -Value $threadLines
        }
        if ($powerMode -eq 'on') {
            $powerAc = 0
            $powerRows = @()
            if (Test-Path $powerCsv) {
                try {
                    $powerRows = @(Get-Content $powerCsv |
                        Where-Object { $_ -and -not $_.StartsWith('#') } | ConvertFrom-Csv)
                    foreach ($row in $powerRows) {
                        $online = ([string]$row.power_online).Trim()
                        $charging = ([string]$row.charging).Trim()
                        $status = ([string]$row.battery_status).Trim()
                        if ($online -in @('True', '1') -or $charging -in @('True', '1') -or
                            $status -in @('2', '6', '7', '8', '9')) {
                            $powerAc = 1
                        }
                    }
                } catch {
                    Log ("power summary failed: " + $_.Exception.Message)
                }
            }
            $powerLines = @(
                "power_ac=$powerAc",
                "power_csv_bytes=$(if (Test-Path $powerCsv) { (Get-Item $powerCsv).Length } else { 0 })",
                "power_samples=$($powerRows.Count)",
                "power_sampler_exited=$(if ($powerSampler) { $powerSampler.HasExited } else { 'not-started' })"
            )
            Add-Content -Path (Join-Path $dir 'meta.txt') -Value $powerLines
        }
        if ($surfaceStateMode -ne 'off' -and (Test-Path $surfaceScript)) {
            try {
                & $surfaceScript -Out $surfaceCsv -Phase after -Samples 1
            } catch {
                Log ("surface state after failed: " + $_.Exception.Message)
            }
            try {
                Write-SurfaceStateSummary $dir $surfaceCsv
            } catch {
                Log ("surface state summary failed: " + $_.Exception.Message)
            }
        }
        # State the application left in kritarc, after the overrides are
        # restored: the canvas writes canvasState and the chosen renderer.
        $afterKeys = Get-KritarcValues @('canvasState', 'OpenGLRenderer', 'useOpenGL', 'useMaximumFrameLatencyOne')
        $afterLines = @()
        foreach ($key in @('canvasState', 'OpenGLRenderer', 'useOpenGL', 'useMaximumFrameLatencyOne')) {
            $value = if ($afterKeys -and $afterKeys.ContainsKey($key)) { $afterKeys[$key] } else { 'absent' }
            $afterLines += "kritarc_after.$key=$value"
        }
        $afterLines += "app_log_bytes=$(if (Test-Path $appOut) { (Get-Item $appOut).Length } else { 0 })"
        $afterLines += "app_err_bytes=$(if (Test-Path $appErr) { (Get-Item $appErr).Length } else { 0 })"
        Add-Content -Path (Join-Path $dir 'meta.txt') -Value $afterLines
        if ($presentMonEnabled) {
            $recorded = (Test-Path $presentMonCsv) -and ((Get-Item $presentMonCsv).Length -gt 0)
            $presentMonState = if ($recorded) { 'present' } else { 'none' }
            $lines = @(
                "presentmon_result=$presentMonState",
                "presentmon_session=$sessionName",
                "presentmon_session_started=$sessionStarted",
                "presentmon_session_after=$(Test-PresentMonSession $sessionName)",
                "presentmon_csv_bytes=$(if (Test-Path $presentMonCsv) { (Get-Item $presentMonCsv).Length } else { 0 })",
                "presentmon_err_bytes=$(if (Test-Path $presentMonErr) { (Get-Item $presentMonErr).Length } else { 0 })",
                "presentmon_out_bytes=$(if (Test-Path $presentMonOut) { (Get-Item $presentMonOut).Length } else { 0 })"
            )
            if ($pm) {
                $lines += "presentmon_exited_before_stop=$pmExitedBeforeStop"
                if ($pmStartedAt) { $lines += "presentmon_started_at=$($pmStartedAt.ToString('o'))" }
                if ($pmStoppedAt) { $lines += "presentmon_stopped_at=$($pmStoppedAt.ToString('o'))" }
                $exitCode = $null
                try { $exitCode = $pm.ExitCode } catch { }
                if ($null -ne $exitCode) { $lines += "presentmon_exit=$exitCode" }
                else { $lines += "presentmon_exit=unavailable" }
            }
            if ((Test-Path $presentMonErr) -and ((Get-Item $presentMonErr).Length -gt 2)) {
                $errText = ((Get-Content $presentMonErr -Raw) -replace "`r?`n", ' ').Trim()
                if ($errText.Length -gt 300) { $errText = $errText.Substring(0, 300) }
                $lines += "presentmon_error=$errText"
            }
            Add-Content -Path (Join-Path $dir 'meta.txt') -Value $lines
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

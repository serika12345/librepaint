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

function Send-ZoomKeys($app, [int]$percent) {
    # Use the application shortcuts (krita_default.shortcuts): Ctrl+0 puts the
    # canvas at 100 %, and Ctrl+= steps through the standard zoom levels
    # 4/3, 2, 8/3, 4. The window must be in front; record whether it was.
    [void][L1Probe]::ShowWindow($app.MainWindowHandle, 9)
    [void][L1Probe]::SetForegroundWindow($app.MainWindowHandle)
    Start-Sleep -Milliseconds 500
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
        'display_check_captures=100,400'
    )
    $foreground = Send-ZoomKeys $app 100
    Add-Content -Path (Join-Path $dir 'meta.txt') -Value "display_check_foreground_100=$foreground"
    Invoke-Frame $dir 'display-100'
    $foreground = Send-ZoomKeys $app 400
    Add-Content -Path (Join-Path $dir 'meta.txt') -Value "display_check_foreground_400=$foreground"
    Invoke-Frame $dir 'display-400'
    Log 'display check captured 100 and 400'
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

try {
    $request = Get-Content (Join-Path $here 'request.json') -Raw | ConvertFrom-Json
    $target = [string]$request.target
    $runs = [int]$request.runs
    $settle = if ($request.settleSeconds) { [int]$request.settleSeconds } else { 12 }

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
    $runName = if ($request.name) { [string]$request.name } else { $target }
    $id = '{0}-{1}' -f $runName, (Get-Date -Format 'yyyyMMdd-HHmmss')

    for ($n = 1; $n -le $runs; $n++) {
        $dir = Join-Path $here ("runs\{0}-{1}" -f $id, $n)
        New-Item -ItemType Directory -Force $dir | Out-Null
        $script:runStatus = Join-Path $dir 'status.txt'
        Set-Content -Path $script:runStatus -Value 'starting'
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
        $appImage = if ($displayCheck) { [string]$displayCheck.image } else { $canvas }
        $app = $null
        try {
            $app = Start-Process -FilePath $request.app -ArgumentList @('--nosplash', "`"$appImage`"") -PassThru
            $hwnd = Wait-MainWindow $app 120
            Write-LaunchFacts $dir 'window' $app

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
            elseif ($capture) { Invoke-Capture $dir }
            else { [PenInject]::Run($hwnd, (Join-Path $dir 'inject.csv'), $options) | Out-Null }
            # The probe writer flushes every 500 ms; leave time for the last batch.
            Start-Sleep -Milliseconds 1500
        }
        finally {
            if ($app -and -not $app.HasExited) {
                Stop-Process -Id $app.Id -Force
                [void]$app.WaitForExit(5000)
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
        }
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

# SPDX-License-Identifier: GPL-3.0-or-later
#
# Records the state of the Windows device that a measurement run depends on.
#
# Why: on 2026-11-11 the reference target's ink latency jumped from 17.8-18.1
# to 29-33 ms in the middle of one session (progress.md, q4-focus-gate). The
# runs before and after cannot be compared, but nothing recorded why. This
# script samples the thermal zone temperatures, the processor performance
# (how far the CPU is throttled from its nominal clock), the overall CPU use
# and the five busiest processes, so a later analysis can mark the runs that
# ran on a colder, hotter or busier device.
#
# Two modes exist because the sampling itself may perturb the device. A run
# on 2026-11-11 05:04 showed `% Processor Performance` at 42-50 % while the
# sampler ran and a WmiPrvSE process in the busiest five every second, so the
# runner now records nothing unless the target asks for it ("surface_state" in
# hosts.json):
#   on     four WMI queries per second (temperature, processor performance,
#          total CPU, busiest processes), as before.
#   light  one Get-Counter call per five seconds for the CPU counters and one
#          WMI temperature read per call; the busiest processes only before
#          and after.
#   off    nothing (the runner does not start this script).
#
# Why the wait: on 2026-11-21 a one-second record of the temperature and the
# load showed how heat reduces the work done. When the maximum
# HighPrecisionTemperature of Win32_PerfFormattedData_Counters_
# ThermalZoneInformation reached about 50 C, the CPU package power
# (\Energy Meter(RAPL_Package0_PKG)\Power) fell from 25-32 W to 3-9 W. From
# 44 C, a load on every core held 31-34 W and 220 % performance for 28 s and
# dropped once the temperature passed 50 C; started at 49 C, the same load
# stayed near 9 W from the beginning. About six minutes of idle from 49 C
# brought the temperature down to 42 C. MSAcpi_ThermalZoneTemperature stayed
# at 26.9 C on that device and % Performance Limit read 72 % even when
# nothing was wrong, so neither marks a hot device. The wait therefore reads
# the high-precision temperature every five seconds and returns once it is
# below -WaitMaximumC (45 C by default) for two samples in a row, with a hard
# cap so a device that does not cool still produces a run.
#
# Called by run-surface.ps1:
#   - before each run, to wait until the device has cooled (-WaitForCoolDevice):
#     it reads the temperature from Win32_PerfFormattedData_Counters_
#     ThermalZoneInformation every five seconds and returns as soon as the
#     value is below the requested maximum for two samples in a row (the
#     runner asks for 45 C). A device above 50 C limits the CPU package to
#     3-9 W (2026-11-21), so a run started there cannot be compared with a
#     cool one. The wait is capped (900 s from the runner) so a device that
#     does not cool still produces a run, and the result is written to
#     meta.txt.
#     run-surface.ps1 starts this mode in its own powershell.exe process and
#     stops it at a hard wall-clock limit: a single WMI read can block without
#     returning when the performance-counter subsystem is unhealthy, so an
#     in-process deadline is not enough (2026-11-11: the first wait hung the
#     runner for 36 minutes before the app started).
#     The wait reads WMI, not Get-Counter, so it needs neither Perflib nor a
#     localized counter path. $Out is optional so the wait mode never prompts
#     for a missing mandatory parameter: run-surface.ps1 starts this mode in a
#     hidden console, where such a prompt waited for ever (2026-11-11).
#   - one sample before the application starts (-Phase before -Samples 1)
#   - while the run happens (-Phase during -IntervalMs 1000 -Mode on|light
#     -StopFile <path>), in a separate powershell.exe process
#   - one sample after the application exits (-Phase after -Samples 1)
#
# The counters in the "on" mode come from the formatted performance classes
# instead of Get-Counter: the surface runs a Japanese Windows, where the
# English counter paths of Get-Counter do not resolve. The "light" mode needs
# Get-Counter's continuous sampling, so it builds the localized counter path
# from the Perflib index/name lists (English under 009, localized under
# CurrentLanguage) and falls back to the English path. Every field is
# optional; a value the device does not provide is written as "unavailable".
#
# Docs: docs/architecture/DEVELOPMENT.md, the pen input latency section.

param(
    # -Out is only needed by the sampling modes. The wait mode does not write a
    # CSV, and run-surface.ps1 calls it from a hidden, non-interactive console:
    # a mandatory parameter there makes PowerShell prompt for it and blocks for
    # ever before the wait starts (2026-11-11, the first wait hung 36 minutes).
    [string]$Out = '',
    [string]$Phase = 'during',
    [int]$Samples = 1,
    [int]$IntervalMs = 1000,
    [string]$StopFile,
    [ValidateSet('on', 'light')][string]$Mode = 'on',
    [switch]$WaitForCoolDevice,
    [int]$WaitMaximumC = 45,
    [int]$WaitConsecutiveSamples = 2,
    [int]$WaitTimeoutSeconds = 120,
    [int]$WaitSampleSeconds = 5
)

$ErrorActionPreference = 'Stop'

$columns = @('time', 'phase', 'max_temp_c', 'processor_performance_pct', 'total_cpu_pct', 'top_processes')

function Format-Value($value) {
    if ($null -eq $value) { return 'unavailable' }
    return [string]$value
}

function Get-MaxThermalZoneC {
    # Win32_PerfFormattedData_Counters_ThermalZoneInformation reports
    # HighPrecisionTemperature in tenths of a kelvin. The older MSAcpi_
    # ThermalZoneTemperature class stayed at 26.9 C on the measuring device
    # (2026-11-21), so it could not show when the device was hot.
    try {
        $zones = @(Get-CimInstance -ClassName 'Win32_PerfFormattedData_Counters_ThermalZoneInformation' -ErrorAction Stop)
        $values = @()
        foreach ($zone in $zones) {
            if ($null -ne $zone.HighPrecisionTemperature) {
                $values += [math]::Round(([double]$zone.HighPrecisionTemperature / 10.0) - 273.15, 1)
            }
        }
        if ($values.Count -eq 0) { return $null }
        return ($values | Measure-Object -Maximum).Maximum
    } catch {
        return $null
    }
}

function Get-ProcessorInformation {
    # % Processor Performance says how far the CPU is throttled from its
    # nominal clock.
    try {
        $entry = Get-CimInstance -ClassName 'Win32_PerfFormattedData_Counters_ProcessorInformation' `
            -Filter "Name='_Total'" -ErrorAction Stop
        if (-not $entry) { return @{ performance = $null } }
        $performance = if ($null -ne $entry.PercentProcessorPerformance) {
            [int]$entry.PercentProcessorPerformance
        } else { $null }
        return @{ performance = $performance }
    } catch {
        return @{ performance = $null }
    }
}

function Get-TotalCpuPct {
    try {
        $entry = Get-CimInstance -ClassName 'Win32_PerfFormattedData_PerfOS_Processor' `
            -Filter "Name='_Total'" -ErrorAction Stop
        if ($entry -and $null -ne $entry.PercentProcessorTime) {
            return [int]$entry.PercentProcessorTime
        }
        return $null
    } catch {
        return $null
    }
}

function Get-TopProcesses {
    # PercentProcessorTime is per logical processor, so a busy single-threaded
    # process reads about 100 on an eight-thread machine.
    try {
        $processes = Get-CimInstance -ClassName 'Win32_PerfFormattedData_PerfProc_Process' -ErrorAction Stop |
            Where-Object { $_.Name -ne '_Total' -and $_.Name -ne 'Idle' -and $_.PercentProcessorTime -gt 0 } |
            Sort-Object -Property PercentProcessorTime -Descending |
            Select-Object -First 5
        $parts = @()
        foreach ($process in $processes) {
            $name = [string]$process.Name -replace '#\d+$', ''
            $parts += ('{0} {1}%' -f $name, [int]$process.PercentProcessorTime)
        }
        if ($parts.Count -eq 0) { return 'none' }
        return ($parts -join '; ')
    } catch {
        return $null
    }
}

function Add-Row([string]$time, [string]$phase, $temp, $performance, $cpu, $top) {
    $row = [pscustomobject]@{
        time                      = $time
        phase                     = $phase
        max_temp_c                = Format-Value $temp
        processor_performance_pct = Format-Value $performance
        total_cpu_pct             = Format-Value $cpu
        top_processes             = (Format-Value $top) -replace ',', ';'
    }
    # The first call for a run writes the header; the sampler and the final
    # sample append to the same file.
    if (-not (Test-Path $Out)) {
        Set-Content -Path $Out -Value (($columns -join ','))
    }
    Add-Content -Path $Out -Value (($row.PSObject.Properties | ForEach-Object { [string]$_.Value }) -join ',')
}

function Add-Sample {
    $counters = Get-ProcessorInformation
    Add-Row ((Get-Date).ToString('o')) $Phase (Get-MaxThermalZoneC) $counters.performance `
        (Get-TotalCpuPct) (Get-TopProcesses)
}

function Get-CounterNameMap([string]$language) {
    # Perflib keeps (index, name) pairs. The English list is under 009; the
    # localized list under CurrentLanguage has the same indices, which is how
    # a localized counter path can be built without guessing the translation.
    $path = Join-Path 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Perflib' $language
    try {
        $values = @((Get-ItemProperty -Path $path -Name Counter -ErrorAction Stop).Counter)
    } catch {
        return @{}
    }
    $map = @{}
    for ($i = 0; $i + 1 -lt $values.Count; $i += 2) {
        $index = 0
        if ([int]::TryParse([string]$values[$i], [ref]$index)) {
            $map[$index] = [string]$values[$i + 1]
        }
    }
    return $map
}

function Resolve-CounterPath([string]$setName, [string]$counterName) {
    $english = Get-CounterNameMap '009'
    $localized = Get-CounterNameMap 'CurrentLanguage'
    if ($english.Count -eq 0) { return $null }
    $setIndex = $null
    $counterIndex = $null
    foreach ($entry in $english.GetEnumerator()) {
        if ($null -eq $setIndex -and $entry.Value -eq $setName) { $setIndex = $entry.Key }
        if ($null -eq $counterIndex -and $entry.Value -eq $counterName) { $counterIndex = $entry.Key }
    }
    if ($null -eq $setIndex -or $null -eq $counterIndex) { return $null }
    $setLocal = if ($localized.ContainsKey($setIndex)) { $localized[$setIndex] } else { $setName }
    $counterLocal = if ($localized.ContainsKey($counterIndex)) { $localized[$counterIndex] } else { $counterName }
    return ('\{0}(_Total)\{1}' -f $setLocal, $counterLocal)
}

function Wait-ForCoolDevice {
    # Wait for the device to cool before a measurement. Only the final result
    # is printed (key=value lines) so run-surface.ps1 can append it to
    # meta.txt. A reading that cannot be taken resets the run of cool samples
    # and does not block the measurement.
    #
    # The reading comes from the WMI formatted thermal zone class: on a
    # Japanese Windows the English Get-Counter path does not resolve and
    # building the localized path reads the large Perflib lists. Either call
    # can block for ever when the counter subsystem is unhealthy, so
    # run-surface.ps1 also bounds this whole mode with a separate process and
    # a hard timeout.
    $source = 'Win32_PerfFormattedData_Counters_ThermalZoneInformation'
    $started = Get-Date
    $readings = @()
    $consecutive = 0
    $state = 'timeout'
    while ($true) {
        $value = Get-MaxThermalZoneC
        if ($null -eq $value) {
            $consecutive = 0
        } else {
            $readings += $value
            if ($value -lt $WaitMaximumC) { $consecutive++ } else { $consecutive = 0 }
        }
        if ($consecutive -ge [math]::Max(1, $WaitConsecutiveSamples)) { $state = 'ready'; break }
        $elapsed = ((Get-Date) - $started).TotalSeconds
        if ($elapsed -ge $WaitTimeoutSeconds) { break }
        Start-Sleep -Seconds ([math]::Max(1, $WaitSampleSeconds))
    }
    $elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
    $first = if ($readings.Count) { $readings[0] } else { 'unavailable' }
    $last = if ($readings.Count) { $readings[-1] } else { 'unavailable' }
    Write-Output "surface_state_wait=$state"
    Write-Output "surface_state_wait_seconds=$elapsed"
    Write-Output "surface_state_wait_samples=$($readings.Count)"
    Write-Output "surface_state_wait_consecutive_required=$([math]::Max(1, $WaitConsecutiveSamples))"
    Write-Output "surface_state_wait_maximum_c=$WaitMaximumC"
    Write-Output "surface_state_wait_first_c=$first"
    Write-Output "surface_state_wait_last_c=$last"
    Write-Output "surface_state_wait_source=wmi"
    Write-Output "surface_state_wait_path=$source"
}

function Add-LightSamples([string]$stopFile) {
    # One Get-Counter call collects a batch of samples inside the counter
    # subsystem; the loop only writes the rows. The temperature is one WMI
    # read per batch and is written to every row of it. The busiest processes
    # are left to the before and after samples.
    $performancePath = Resolve-CounterPath 'Processor Information' '% Processor Performance'
    $cpuPath = Resolve-CounterPath 'Processor' '% Processor Time'
    if (-not $performancePath) { $performancePath = '\Processor Information(_Total)\% Processor Performance' }
    if (-not $cpuPath) { $cpuPath = '\Processor(_Total)\% Processor Time' }
    try {
        Set-Content -Path ([System.IO.Path]::ChangeExtension($Out, '.paths.txt')) -Value @(
            'mode=light',
            "processor_performance=$performancePath",
            "total_cpu=$cpuPath"
        )
    } catch { }
    $interval = [math]::Max(1, [int]($IntervalMs / 1000))
    $limit = [math]::Max(1, [int](3600 / $interval))
    $written = 0
    $broken = $false
    while ($written -lt $limit) {
        if (Test-Path $stopFile) { break }
        $temperature = Get-MaxThermalZoneC
        if ($broken) {
            Add-Row ((Get-Date).ToString('o')) $Phase $temperature $null $null $null
            $written++
            Start-Sleep -Milliseconds ([math]::Max(100, $IntervalMs))
            continue
        }
        $batch = [int][math]::Min(5, $limit - $written)
        try {
            $sets = @(Get-Counter -Counter @($performancePath, $cpuPath) -MaxSamples $batch `
                -SampleInterval $interval -ErrorAction Stop)
        } catch {
            Add-Row ((Get-Date).ToString('o')) $Phase $temperature $null $null $null
            $written++
            $broken = $true
            continue
        }
        foreach ($set in $sets) {
            $performance = $null
            $cpu = $null
            foreach ($sample in @($set.CounterSamples)) {
                if ($sample.Path -like "*$performancePath") {
                    $performance = [math]::Round([double]$sample.CookedValue)
                } elseif ($sample.Path -like "*$cpuPath") {
                    $cpu = [math]::Round([double]$sample.CookedValue)
                }
            }
            Add-Row ((Get-Date).ToString('o')) $Phase $temperature $performance $cpu $null
            $written++
        }
    }
}

if (-not $WaitForCoolDevice -and -not $Out) {
    throw '-Out is required unless -WaitForCoolDevice is used'
}

if ($WaitForCoolDevice) {
    Wait-ForCoolDevice
} elseif ($StopFile -and $Mode -eq 'light') {
    Add-LightSamples $StopFile
} elseif ($StopFile) {
    # Run until the runner writes the stop file, with a hard cap so a lost
    # runner cannot leave a sampler behind for ever.
    $limit = [math]::Max(1, [int](3600 * 1000 / [math]::Max(100, $IntervalMs)))
    for ($i = 0; $i -lt $limit; $i++) {
        if (Test-Path $StopFile) { break }
        try { Add-Sample } catch { }
        Start-Sleep -Milliseconds $IntervalMs
    }
} else {
    for ($i = 0; $i -lt [math]::Max(1, $Samples); $i++) {
        if ($i -gt 0) { Start-Sleep -Milliseconds $IntervalMs }
        Add-Sample
    }
}

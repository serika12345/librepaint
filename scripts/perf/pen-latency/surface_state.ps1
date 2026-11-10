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
#   light  one Get-Counter call per five seconds for the CPU counters only;
#          temperature and the busiest processes only before and after.
#   off    nothing (the runner does not start this script).
#
# Called by run-surface.ps1:
#   - before each run, to wait until the device has recovered from a throttled
#     state (-WaitForProcessorPerformance): it reads % Processor Performance
#     with one Get-Counter call every five seconds and returns as soon as the
#     value is at least 70 for two samples in a row. A device that took the
#     2026-11-11 measurements stayed at 42-50 % for minutes; starting a run
#     there produced values that cannot be compared (progress.md, q4-cpu).
#     The wait is capped (120 s by default) so a device that does not recover
#     still produces a run, and the result is written to meta.txt.
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
# Docs: docs/architecture/DEVELOPMENT.md, "性能計測（ペン入力から表示まで）".

param(
    [Parameter(Mandatory = $true)][string]$Out,
    [string]$Phase = 'during',
    [int]$Samples = 1,
    [int]$IntervalMs = 1000,
    [string]$StopFile,
    [ValidateSet('on', 'light')][string]$Mode = 'on',
    [switch]$WaitForProcessorPerformance,
    [int]$WaitMinimumPct = 70,
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
    # MSAcpi_ThermalZoneTemperature reports tenths of a kelvin.
    try {
        $zones = @(Get-CimInstance -Namespace 'root/wmi' -ClassName 'MSAcpi_ThermalZoneTemperature' -ErrorAction Stop)
        $values = @()
        foreach ($zone in $zones) {
            if ($null -ne $zone.CurrentTemperature) {
                $values += [math]::Round(([double]$zone.CurrentTemperature / 10.0) - 273.15, 1)
            }
        }
        if ($values.Count -eq 0) { return $null }
        return ($values | Measure-Object -Maximum).Maximum
    } catch {
        return $null
    }
}

function Get-ProcessorPerformancePct {
    try {
        $entry = Get-CimInstance -ClassName 'Win32_PerfFormattedData_Counters_ProcessorInformation' `
            -Filter "Name='_Total'" -ErrorAction Stop
        if ($entry -and $null -ne $entry.PercentProcessorPerformance) {
            return [int]$entry.PercentProcessorPerformance
        }
        return $null
    } catch {
        return $null
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
    Add-Row ((Get-Date).ToString('o')) $Phase (Get-MaxThermalZoneC) (Get-ProcessorPerformancePct) `
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

function Resolve-ProcessorPerformancePath {
    $path = Resolve-CounterPath 'Processor Information' '% Processor Performance'
    if (-not $path) { $path = '\Processor Information(_Total)\% Processor Performance' }
    return $path
}

function Read-ProcessorPerformancePct([string]$path) {
    try {
        $set = Get-Counter -Counter $path -MaxSamples 1 -ErrorAction Stop
        $sample = @($set.CounterSamples)[0]
        if ($null -eq $sample -or $null -eq $sample.CookedValue) { return $null }
        return [int][math]::Round([double]$sample.CookedValue)
    } catch {
        return $null
    }
}

function Wait-ForProcessorPerformance {
    # Wait for the device to leave a throttled state before a measurement.
    # Only the final result is printed (key=value lines) so run-surface.ps1
    # can append it to meta.txt. A counter that cannot be read is recorded as
    # "unavailable" and does not block the measurement.
    $path = Resolve-ProcessorPerformancePath
    $started = Get-Date
    $readings = @()
    $consecutive = 0
    $state = 'timeout'
    while ($true) {
        $value = Read-ProcessorPerformancePct $path
        if ($null -eq $value) {
            $consecutive = 0
        } else {
            $readings += $value
            if ($value -ge $WaitMinimumPct) { $consecutive++ } else { $consecutive = 0 }
        }
        if ($consecutive -ge [math]::Max(1, $WaitConsecutiveSamples)) { $state = 'ready'; break }
        $elapsed = ((Get-Date) - $started).TotalSeconds
        if ($elapsed -ge $WaitTimeoutSeconds) { break }
        Start-Sleep -Seconds ([math]::Max(1, $WaitSampleSeconds))
    }
    $elapsed = [math]::Round(((Get-Date) - $started).TotalSeconds, 1)
    if ($readings.Count -eq 0) { $state = 'unavailable' }
    $minimum = if ($readings.Count) { ($readings | Measure-Object -Minimum).Minimum } else { 'unavailable' }
    $last = if ($readings.Count) { $readings[-1] } else { 'unavailable' }
    Write-Output "surface_state_wait=$state"
    Write-Output "surface_state_wait_seconds=$elapsed"
    Write-Output "surface_state_wait_samples=$($readings.Count)"
    Write-Output "surface_state_wait_consecutive_required=$([math]::Max(1, $WaitConsecutiveSamples))"
    Write-Output "surface_state_wait_minimum_pct=$WaitMinimumPct"
    Write-Output "surface_state_wait_min_pct=$minimum"
    Write-Output "surface_state_wait_last_pct=$last"
    Write-Output "surface_state_wait_path=$path"
}

function Add-LightSamples([string]$stopFile) {
    # One Get-Counter call collects a batch of samples inside the counter
    # subsystem; the loop only writes the rows. The temperature and the
    # busiest processes are left to the before and after samples.
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
        if ($broken) {
            Add-Row ((Get-Date).ToString('o')) $Phase $null $null $null $null
            $written++
            Start-Sleep -Milliseconds ([math]::Max(100, $IntervalMs))
            continue
        }
        $batch = [int][math]::Min(5, $limit - $written)
        try {
            $sets = @(Get-Counter -Counter @($performancePath, $cpuPath) -MaxSamples $batch `
                -SampleInterval $interval -ErrorAction Stop)
        } catch {
            Add-Row ((Get-Date).ToString('o')) $Phase $null $null $null $null
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
            Add-Row ((Get-Date).ToString('o')) $Phase $null $performance $cpu $null
            $written++
        }
    }
}

if ($WaitForProcessorPerformance) {
    Wait-ForProcessorPerformance
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

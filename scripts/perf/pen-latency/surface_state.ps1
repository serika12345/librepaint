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
# Called by run-surface.ps1:
#   - one sample before the application starts (-Phase before -Samples 1)
#   - one sample per second while the run happens (-Phase during -IntervalMs
#     1000 -StopFile <path>), in a separate powershell.exe process
#   - one sample after the application exits (-Phase after -Samples 1)
#
# The counters come from the formatted performance classes instead of
# Get-Counter: the surface runs a Japanese Windows, where the English counter
# paths of Get-Counter do not resolve. Every field is optional; a value the
# device does not provide is written as "unavailable".
#
# Docs: docs/architecture/DEVELOPMENT.md, "性能計測（ペン入力から表示まで）".

param(
    [Parameter(Mandatory = $true)][string]$Out,
    [string]$Phase = 'during',
    [int]$Samples = 1,
    [int]$IntervalMs = 1000,
    [string]$StopFile
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

function Add-Sample {
    $row = [pscustomobject]@{
        time                          = (Get-Date).ToString('o')
        phase                         = $Phase
        max_temp_c                    = Format-Value (Get-MaxThermalZoneC)
        processor_performance_pct     = Format-Value (Get-ProcessorPerformancePct)
        total_cpu_pct                 = Format-Value (Get-TotalCpuPct)
        top_processes                 = (Format-Value (Get-TopProcesses)) -replace ',', ';'
    }
    # The first call for a run writes the header; the sampler and the final
    # sample append to the same file.
    if (-not (Test-Path $Out)) {
        Set-Content -Path $Out -Value (($columns -join ','))
    }
    Add-Content -Path $Out -Value (($row.PSObject.Properties | ForEach-Object { [string]$_.Value }) -join ',')
}

if ($StopFile) {
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

# SPDX-License-Identifier: GPL-3.0-or-later
#
# Samples the battery once a second while a latency run happens. The values
# come from Win32_Battery (charge, battery status, estimated runtime) and,
# when the device exposes it, root\wmi:BatteryStatus (remaining capacity in
# mWh, discharge rate in mW, AC online). The timestamps are
# QueryPerformanceCounter ticks, the clock that PenInject.cs and
# windows_capture.cpp use for inject.csv.
#
# The runner starts one instance per run and writes power.stop when the
# application has exited. power.py turns the samples into the average
# discharge rate and the discharged energy inside the injected input window.
param(
    [Parameter(Mandatory = $true)][string]$Out,
    [int]$IntervalMs = 1000,
    [string]$StopFile = ''
)

$ErrorActionPreference = 'Stop'
$clock = [System.Diagnostics.Stopwatch]
$frequency = $clock::Frequency
$writer = New-Object System.IO.StreamWriter($Out, $false, [System.Text.Encoding]::UTF8)

function Format-Value($value) {
    if ($null -eq $value) { return 'unavailable' }
    return [string]$value
}

function Get-PowerRow([long]$qpc) {
    $battery = $null
    $status = $null
    try {
        $battery = Get-CimInstance Win32_Battery -ErrorAction SilentlyContinue | Select-Object -First 1
    } catch { }
    try {
        $status = Get-CimInstance -Namespace 'root\wmi' -ClassName BatteryStatus -ErrorAction SilentlyContinue | Select-Object -First 1
    } catch { }
    $charge = if ($battery) { $battery.EstimatedChargeRemaining } else { $null }
    $batteryStatus = if ($battery) { $battery.BatteryStatus } else { $null }
    $estimatedRunTime = if ($battery) { $battery.EstimatedRunTime } else { $null }
    $remaining = if ($status) { $status.RemainingCapacity } else { $null }
    $discharge = if ($status) { $status.DischargeRate } else { $null }
    $online = if ($status) { $status.PowerOnline } else { $null }
    $charging = if ($status) { $status.Charging } else { $null }
    $discharging = if ($status) { $status.Discharging } else { $null }
    return ('{0},{1},{2},{3},{4},{5},{6},{7},{8}' -f `
        $qpc, (Format-Value $charge), (Format-Value $batteryStatus), (Format-Value $estimatedRunTime),
        (Format-Value $remaining), (Format-Value $discharge), (Format-Value $online),
        (Format-Value $charging), (Format-Value $discharging))
}

try {
    $writer.WriteLine("# frequency=$frequency")
    $writer.WriteLine("qpc,charge_pct,battery_status,estimated_run_time,remaining_mwh,discharge_mw,power_online,charging,discharging")
    while ($true) {
        if ($StopFile -and (Test-Path $StopFile)) { break }
        $qpc = $clock::GetTimestamp()
        $writer.WriteLine((Get-PowerRow $qpc))
        $writer.Flush()
        Start-Sleep -Milliseconds $IntervalMs
    }
} finally {
    $writer.Flush()
    $writer.Close()
}

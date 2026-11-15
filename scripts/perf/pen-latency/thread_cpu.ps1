# SPDX-License-Identifier: GPL-3.0-or-later
#
# Samples the CPU time of every thread of one process while a latency run
# happens. The values are the cumulative counters behind the Windows Process
# and Thread performance counters (Process.TotalProcessorTime and
# ProcessThread.TotalProcessorTime), so the script does not need localized
# Get-Counter paths. The timestamps are QueryPerformanceCounter ticks, the
# clock that PenInject.cs and windows_capture.cpp use for inject.csv.
#
# The runner starts one instance per run and writes threads.stop when the
# application has exited. thread_cpu.py turns the cumulative values into the
# CPU time inside the injected input window.
param(
    [Parameter(Mandatory = $true)][int]$ProcessId,
    [Parameter(Mandatory = $true)][string]$Out,
    [int]$IntervalMs = 500,
    [string]$StopFile = ''
)

$ErrorActionPreference = 'Stop'
$clock = [System.Diagnostics.Stopwatch]
$frequency = $clock::Frequency
$writer = New-Object System.IO.StreamWriter($Out, $false, [System.Text.Encoding]::UTF8)
try {
    $writer.WriteLine("# frequency=$frequency")
    $writer.WriteLine("qpc,pid,thread_id,cpu_seconds,thread_state,wait_reason,priority,process_cpu_seconds")
    while ($true) {
        if ($StopFile -and (Test-Path $StopFile)) { break }
        $process = Get-Process -Id $ProcessId -ErrorAction SilentlyContinue
        if (-not $process) { break }
        $qpc = $clock::GetTimestamp()
        $processCpu = 'unavailable'
        try {
            $process.Refresh()
            $processCpu = $process.TotalProcessorTime.TotalSeconds
        } catch { }
        $rows = New-Object System.Collections.Generic.List[string]
        try {
            foreach ($thread in $process.Threads) {
                try {
                    $row = "{0},{1},{2},{3},{4},{5},{6},{7}" -f `
                        $qpc, $ProcessId, $thread.Id, $thread.TotalProcessorTime.TotalSeconds,
                        $thread.ThreadState, $thread.WaitReason, $thread.PriorityLevel, $processCpu
                    $rows.Add($row)
                } catch { }
            }
        } catch { }
        foreach ($row in $rows) { $writer.WriteLine($row) }
        $writer.Flush()
        Start-Sleep -Milliseconds $IntervalMs
    }
} finally {
    $writer.Flush()
    $writer.Close()
}

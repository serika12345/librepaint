# SPDX-License-Identifier: GPL-3.0-or-later
#
# Registers the scheduled task that starts run-surface.ps1 in the logged-on
# user's interactive desktop session with the highest privileges (PresentMon
# needs them for ETW tracing). Run once from an administrator shell, for
# example over ssh. The task starts on battery power as well.

$ErrorActionPreference = 'Stop'
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$script = Join-Path $here 'run-surface.ps1'
$action = New-ScheduledTaskAction -Execute 'powershell.exe' `
    -Argument "-NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File `"$script`""
$principal = New-ScheduledTaskPrincipal -UserId $env:USERNAME -LogonType Interactive -RunLevel Highest
$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries `
    -ExecutionTimeLimit (New-TimeSpan -Hours 2) -MultipleInstances IgnoreNew
Register-ScheduledTask -TaskName 'LibrePaintPenLatency' -Action $action -Principal $principal `
    -Settings $settings -Force | Out-Null
'registered LibrePaintPenLatency'

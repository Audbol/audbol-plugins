<#
Runs every case in tools/reaper/cases at one or more sample rates and prints
one line per case plus any failing checks.

  & .\tools\reaper\run_all.ps1 -SampleRates 44100,48000,96000
#>
param([int[]]$SampleRates = @(48000), [string]$Filter = "*")
$total = 0; $failed = 0
foreach ($sr in $SampleRates) {
    "== $sr Hz =="
    foreach ($case in Get-ChildItem "$PSScriptRoot\cases\$Filter.lua" | ForEach-Object { $_.BaseName }) {
        $r = & "$PSScriptRoot\run.ps1" -Case $case -SampleRate $sr -Snapshot "$env:TEMP\suite_$case.png"
        $checks = @($r | Select-String "^(PASS|FAIL)")
        $bad = @($checks | Select-String "^FAIL")
        $done = [bool]($r -match 'DONE')
        $total += $checks.Count; $failed += $bad.Count
        if (-not $done) { $failed++ }
        "{0,-22} {1,2} pass {2} fail{3}" -f $case, ($checks.Count - $bad.Count), $bad.Count, $(if ($done) { "" } else { "  NO DONE" })
        $bad | ForEach-Object { "    $_" }
    }
}
"TOTAL: $total checks, $failed failed"

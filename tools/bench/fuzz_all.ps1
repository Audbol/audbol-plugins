<#
Automation fuzz for every plug-in: random parameters to random values every
block, in random block sizes (0 to 512 samples), noise input from silence to
+9.5 dBFS, at three sample rates (or -Rates). Fails on NaN / inf or runaway
output above +100 dBFS.

The EQ is fuzzed with its gain-producing parameters held, because 8 stacked
bands at +30 dB x 200 % gain scale are a legitimate (if absurd) way past
+100 dBFS; with them held, any runaway would mean filter instability.

  & .\tools\bench\fuzz_all.ps1 [-Build build] [-Seconds 60] [-Rates 22050,44100,48000,96000,192000]
#>
param([string]$Build = "build", [int]$Seconds = 60, [string]$Rates = "44100,48000,96000")
$root = (Resolve-Path "$PSScriptRoot/../..").Path
$exe = if ($env:OS -eq "Windows_NT") { ".exe" } else { "" }
$bench = "$root/$Build/bin/Release/bench$exe"
$failed = 0
foreach ($plugin in Get-ChildItem "$root/$Build/VST3/Release/*.vst3") {
    # .vst3 bundles are folders, whose BaseName keeps the extension
    $name = [IO.Path]::GetFileNameWithoutExtension($plugin.Name)
    $extra = if ($name -eq "AudbolEQ") { @("--exclude", "Gain, Q,Scale") } else { @() }
    $line = & $bench $plugin.FullName $Seconds $Rates --fuzz @extra
    ($line -replace '\s+', ' ')
    if ($line -match "FAIL") { $failed++ }
}
"FUZZ: $failed plug-in(s) failed"
if ($failed) { exit 1 }

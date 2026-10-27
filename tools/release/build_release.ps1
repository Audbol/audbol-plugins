<#
Builds a release of the whole suite from scratch:
  build/release/AudbolPluginSuite-<version>-win64-setup.exe   installer
  build/release/AudbolPluginSuite-<version>-win64-vst3.zip    plug-ins for manual install
  build/release/SHA256SUMS.txt

  & .\tools\release\build_release.ps1
Requires Visual Studio 2022 (C++), CMake and Inno Setup 6.
#>
param([string]$Iscc = "")
$ErrorActionPreference = "Stop"
$root = (Resolve-Path "$PSScriptRoot\..\..").Path
$build = "$root\build-release"
$out = "$root\build\release"

# Version from the top-level project() call
$version = (Select-String -Path "$root\CMakeLists.txt" -Pattern 'VERSION\s+(\d+\.\d+\.\d+)').Matches[0].Groups[1].Value
"Audbol Plugin Suite $version"

if (-not $Iscc) {
    $Iscc = @("$env:LOCALAPPDATA\Programs\Inno Setup 6\ISCC.exe", "${env:ProgramFiles(x86)}\Inno Setup 6\ISCC.exe",
              "$env:ProgramFiles\Inno Setup 6\ISCC.exe") | Where-Object { Test-Path $_ } | Select-Object -First 1
}
if (-not $Iscc) { throw "Inno Setup 6 (ISCC.exe) not found" }

# Clean build in its own directory; the SDK validator runs on every plug-in.
if (Test-Path $build) { Remove-Item $build -Recurse -Force }
cmake -S $root -B $build -G "Visual Studio 17 2022" -A x64 | Out-Null
$log = cmake --build $build --config Release -- /m /v:minimal /nologo 2>&1
$results = $log | Select-String "Result: \d+ tests passed, (\d+) tests failed"
$bad = $results | Where-Object { $_.Matches[0].Groups[1].Value -ne "0" }
if ($LASTEXITCODE -ne 0 -or $results.Count -lt 10 -or $bad) {
    $log | Select-String "error|failed" | Select-Object -First 20
    throw "Build or validation failed"
}
"built and validated $($results.Count) plug-ins"

# Stage only the suite's bundles
$plugins = Get-ChildItem "$build\VST3\Release" -Directory -Filter "Audbol*.vst3"
if ($plugins.Count -ne 10) { throw "expected 10 plug-ins, found $($plugins.Count)" }
New-Item -ItemType Directory -Force $out | Out-Null
Get-ChildItem $out -File | Remove-Item -Force
$stage = "$build\stage"
New-Item -ItemType Directory -Force $stage | Out-Null
$plugins | ForEach-Object { Copy-Item $_.FullName $stage -Recurse }

# Factory presets (.vstpreset), generated from the same tables the plug-ins use
$presets = "$build\presets\Audbol Audio"
& "$build\bin\Release\make_presets.exe" $presets | Select-Object -Last 1
if ($LASTEXITCODE -ne 0) { throw "make_presets failed" }

$zip = "$out\AudbolPluginSuite-$version-win64-vst3.zip"
New-Item -ItemType Directory -Force "$stage\Presets" | Out-Null
Copy-Item $presets "$stage\Presets" -Recurse
Compress-Archive -Path "$stage\*" -DestinationPath $zip -CompressionLevel Optimal
& $Iscc /Q "/DAppVersion=$version" "/DSourceDir=$stage" "/DPresetDir=$presets" "/O$out" "$PSScriptRoot\audbol.iss"
if ($LASTEXITCODE -ne 0) { throw "Inno Setup failed" }

Get-ChildItem $out -File | Where-Object Name -ne "SHA256SUMS.txt" | ForEach-Object {
    "{0}  {1}" -f (Get-FileHash $_.FullName -Algorithm SHA256).Hash.ToLower(), $_.Name
} | Set-Content "$out\SHA256SUMS.txt" -Encoding ascii
Get-ChildItem $out -File | ForEach-Object { "{0,-48} {1,8:N0} KB" -f $_.Name, ($_.Length / 1KB) }

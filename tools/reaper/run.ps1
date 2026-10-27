<#
Runs one plug-in test case in REAPER and captures the plug-in window.

  powershell -File tools\reaper\run.ps1 -Case compressor
  powershell -File tools\reaper\run.ps1 -Case eq -Snapshot eq.png

REAPER is started with Dummy Audio (no sound card needed) and the suite's
build folder as its VST3 path. Your reaper.ini is backed up first and always
restored afterwards. Close REAPER before running.
#>
param(
    [Parameter(Mandatory)] [string]$Case,
    [string]$Snapshot = "",
    [int]$SampleRate = 48000,
    [string]$Reaper = "C:\Program Files\REAPER (x64)\reaper.exe",
    [string]$PluginDir = (Resolve-Path "$PSScriptRoot\..\..\build\VST3\Release").Path
)
$ErrorActionPreference = "Stop"
if (Get-Process reaper -ErrorAction SilentlyContinue) { throw "Close REAPER first." }
if (-not $Snapshot) { $Snapshot = "$PSScriptRoot\$Case.png" }

$signals = "drums", "pink", "impulse", "sine", "bass" | ForEach-Object { "$PSScriptRoot\signals\$_.wav" }
if (($signals | Where-Object { -not (Test-Path $_) }).Count -gt 0) {
    New-Item -ItemType Directory -Force "$PSScriptRoot\signals" | Out-Null
    & "$PSScriptRoot\gensignals.ps1" -OutDir "$PSScriptRoot\signals" | Out-Null
}
Copy-Item "$PSScriptRoot\cases\$Case.lua" "$PSScriptRoot\current_case.lua" -Force

$ini = "$env:APPDATA\REAPER\reaper.ini"
$backup = "$ini.suite-test-backup"
Copy-Item $ini $backup -Force
try {
    # Dummy Audio is audio system 4; prepend the build folder to the VST path.
    $lines = Get-Content $ini | Where-Object { $_ -notmatch '^vstpath64=' }
    $lines = $lines -replace '^mode=\d+$', 'mode=4' `
                    -replace '^dummy_srate=\d+$', "dummy_srate=$SampleRate" `
                    -replace '^dummy_blocksize=\d+$', 'dummy_blocksize=512'
    $i = [Array]::IndexOf($lines, '[reaper]')
    $lines = $lines[0..$i] + "vstpath64=$PluginDir" + $lines[($i + 1)..($lines.Length - 1)]
    Set-Content $ini $lines -Encoding ascii

    Add-Type -AssemblyName System.Drawing
    Add-Type @"
using System; using System.Text; using System.Runtime.InteropServices;
public static class SuiteWin {
  public delegate bool EnumProc(IntPtr h, IntPtr l);
  [DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc p, IntPtr l);
  [DllImport("user32.dll")] public static extern int GetWindowText(IntPtr h, StringBuilder s, int n);
  [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool SetProcessDPIAware();
  [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
  public static IntPtr Find(string part) {
    IntPtr found = IntPtr.Zero;
    EnumWindows((h, l) => { var sb = new StringBuilder(512); GetWindowText(h, sb, 512);
      if (IsWindowVisible(h) && sb.ToString().Contains(part)) { found = h; return false; } return true; }, IntPtr.Zero);
    return found;
  }
}
"@
    [SuiteWin]::SetProcessDPIAware() | Out-Null

    $log = "$env:TEMP\reaper_test.txt"
    Remove-Item $log -ErrorAction SilentlyContinue
    # A previous REAPER may still be releasing its files: retry the launch.
    $p = $null
    for ($try = 0; $try -lt 10 -and -not $p; $try++) {
        try { $p = Start-Process $Reaper -ArgumentList "-nosplash -new `"$PSScriptRoot\test.lua`"" -PassThru }
        catch { Start-Sleep -Seconds 2 }
    }
    if (-not $p) { throw "Could not start REAPER" }
    $deadline = (Get-Date).AddSeconds(60)
    while ((Get-Date) -lt $deadline) {
        Start-Sleep -Milliseconds 500
        if ((Test-Path $log) -and ((Get-Content $log -Raw) -match 'DONE|FAILED')) { break }
    }
    Start-Sleep -Seconds 3 # let the displays fill with the active plug-in

    $h = [SuiteWin]::Find("Audbol")
    if ($h -ne [IntPtr]::Zero) {
        [SuiteWin]::SetForegroundWindow($h) | Out-Null
        Start-Sleep -Milliseconds 600
        $r = New-Object SuiteWin+RECT
        [SuiteWin]::GetWindowRect($h, [ref]$r) | Out-Null
        $bmp = New-Object System.Drawing.Bitmap ($r.R - $r.L), ($r.B - $r.T)
        [System.Drawing.Graphics]::FromImage($bmp).CopyFromScreen($r.L, $r.T, 0, 0, $bmp.Size)
        $bmp.Save($Snapshot, [System.Drawing.Imaging.ImageFormat]::Png)
        "snapshot: $Snapshot"
    } else { "plug-in window not found" }
    if (Test-Path $log) { Get-Content $log }
    Stop-Process $p -Force -ErrorAction SilentlyContinue
    $p | Wait-Process -Timeout 30 -ErrorAction SilentlyContinue
    Start-Sleep -Seconds 1
}
finally {
    Copy-Item $backup $ini -Force
    Remove-Item $backup
    Remove-Item "$PSScriptRoot\current_case.lua" -ErrorAction SilentlyContinue
}

<#
Sample-accurate checks that a host's meters can't make: each case renders a
test signal through a plug-in offline (bench --render) and checks the exact
output peak in time windows.

  & .\tools\bench\offline_tests.ps1 -Rates 44100,48000,96000

Runs under Windows PowerShell and under PowerShell 7 (pwsh) on Linux and macOS.
To check a Linux build from Windows, name the WSL distro and the build
directory inside it; signals and results stay on the Windows side:

  & .\tools\bench\offline_tests.ps1 -Wsl Ubuntu-22.04 -Build /root/audbol/build
#>
param([int[]]$Rates = @(48000), [string]$Filter = "*", [string]$Build = "build", [string]$Wsl = "")
$root = (Resolve-Path "$PSScriptRoot/../..").Path
if ($Wsl) {
  $bench = "$Build/bin/Release/bench"
  $vst = "$Build/VST3/Release"
  # C:\dir\file -> /mnt/c/dir/file
  function HostPath([string]$p) { "/mnt/" + $p.Substring(0, 1).ToLower() + ($p.Substring(2) -replace '\\', '/') }
  function Bench { $a = @($args | ForEach-Object { if ($_ -match '^[A-Za-z]:\\') { HostPath $_ } else { $_ } }); wsl.exe -d $Wsl --exec $bench @a } # --exec: no shell, so ';' in settings survives
} else {
  $exe = if ($env:OS -eq "Windows_NT") { ".exe" } else { "" }
  $bench = "$root/$Build/bin/Release/bench$exe"
  $vst = "$root/$Build/VST3/Release"
  if (-not (Test-Path $bench)) { throw "build the bench target first" }
  function Bench { & $bench @args }
}
$shell = (Get-Process -Id $PID).Path # gensignals runs in this same PowerShell

Add-Type -TypeDefinition @"
using System; using System.IO;
public static class WavPeak {
  // Max |sample| over both channels of a 32-bit float stereo WAV in [from, to) seconds, in dBFS.
  public static double Db(string path, double from, double to) {
    var b = File.ReadAllBytes(path);
    int sr = BitConverter.ToInt32(b, 24), data = 44;
    int frames = (b.Length - data) / 8;
    int i0 = Math.Max(0, (int)(from * sr)), i1 = Math.Min(frames, (int)(to * sr));
    double m = 0;
    for (int i = i0; i < i1; i++)
      for (int c = 0; c < 2; c++) {
        float v = BitConverter.ToSingle(b, data + i * 8 + c * 4);
        if (float.IsNaN(v) || float.IsInfinity(v)) return double.NaN;
        m = Math.Max(m, Math.Abs(v));
      }
    return m > 0 ? 20 * Math.Log10(m) : -200;
  }
}
"@

# Bursts: 1 kHz at -12 dBFS for the first 250 ms of every second, noise peaking
# at -60 dBFS throughout. Hits: 200 Hz hit every 500 ms, peak -12 dBFS,
# decaying 8.7 dB per 125 ms.
$cases = @(
  @{ Name = "gate"; Plugin = "AudbolGate"; Signal = "bursts"; Settings = "-";
     Note = "defaults: the tone passes, the noise between drops by the 40 dB range";
     Windows = @(@(1.01, 1.24, -12.3, -11.8), @(1.70, 1.95, $null, -88), @(2.70, 2.95, $null, -88)) },
  @{ Name = "gate_range"; Plugin = "AudbolGate"; Signal = "bursts"; Settings = "Range=0.25";
     Note = "range 20 dB: once the 150 ms release has run, the -60 dBFS noise sits at about -79";
     Windows = @(@(1.01, 1.24, -12.3, -11.8), @(1.85, 1.98, -81, -78.5)) },
  @{ Name = "gate_expand"; Plugin = "AudbolGate"; Signal = "bursts"; Settings = "Mode=1";
     Note = "1:2 expansion 20 dB below the threshold: another 20 dB down, about -80";
     Windows = @(@(1.01, 1.24, -12.3, -11.8), @(1.85, 1.98, -82, -77), @(2.85, 2.98, -82, -77)) },
  @{ Name = "gate_hold"; Plugin = "AudbolGate"; Signal = "bursts"; Settings = "Hold=1;Release=0";
     Note = "500 ms hold: the noise right after the tone stays open, then a 5 ms release closes it";
     Windows = @(@(1.30, 1.60, -61, -59), @(1.88, 1.98, $null, -95)) },
  @{ Name = "transient_attack"; Plugin = "AudbolTransientShaper"; Signal = "hits"; Settings = "Attack=1";
     Note = "+100 %: the start of each hit up by most of 15 dB; 300 ms in, untouched";
     Windows = @(@(1.00, 1.02, -3.0, 3.5), @(1.30, 1.45, -33.4, -32.4), @(2.00, 2.02, -3.0, 3.5)) },
  @{ Name = "transient_soften"; Plugin = "AudbolTransientShaper"; Signal = "hits"; Settings = "Attack=0";
     Note = "-100 %: the first 4 ms of each hit down by at least 12 dB; the decay untouched";
     Windows = @(@(1.000, 1.004, $null, -24), @(1.30, 1.45, -33.4, -32.4)) },
  @{ Name = "transient_sustain"; Plugin = "AudbolTransientShaper"; Signal = "hits"; Settings = "Sustain=1";
     Note = "+100 %: first peak within 0.5 dB; 300 ms in the hit is 15 dB (the cap) below the 250 ms sustain envelope, which is added back";
     Windows = @(@(1.00, 1.02, -12.4, -11.5), @(1.30, 1.45, -18.6, -17.3)) },
  @{ Name = "transient_tighten"; Plugin = "AudbolTransientShaper"; Signal = "hits"; Settings = "Sustain=0";
     Note = "-100 %: 300 ms in, the tail is 15 dB lower";
     Windows = @(@(1.00, 1.02, -12.4, -11.5), @(1.30, 1.45, -48.6, -47.3)) },
  @{ Name = "transient_mix"; Plugin = "AudbolTransientShaper"; Signal = "hits"; Settings = "Attack=1;Sustain=1;Mix=0";
     Note = "fully dry: untouched";
     Windows = @(@(1.00, 1.02, -12.15, -12.0), @(1.30, 1.45, -33.0, -32.7)) },
  @{ Name = "transient_steady"; Plugin = "AudbolTransientShaper"; Signal = "bass"; Settings = "Attack=1;Sustain=1";
     Note = "a steady 60 Hz tone has no attack or decay: unchanged";
     Windows = @(,@(0.5, 4.0, -12.1, -11.9)) },
  @{ Name = "clipper_ceiling"; Plugin = "AudbolClipper"; Signal = "drums"; Settings = "Input=0.666667;Ceiling=0.958333";
     Note = "+12 dB into a -1 dB ceiling: no sample above it (after the 30 ms parameter glide)";
     Windows = @(,@(0.1, 8.0, -1.3, -1.0)) },
  @{ Name = "clipper_hard"; Plugin = "AudbolClipper"; Signal = "sine"; Settings = "Input=0.666667;Ceiling=0.75;Softness=0";
     Note = "a 0 dBFS sine hard-clipped at -6 dB";
     Windows = @(,@(0.5, 4.0, -6.1, -6.0)) },
  @{ Name = "clipper_soft"; Plugin = "AudbolClipper"; Signal = "sine"; Settings = "Input=0.416667;Ceiling=0.75;Softness=1";
     Note = "a -9 dBFS peak inside the 100 % knee rounds down to -9.27";
     Windows = @(,@(0.5, 4.0, -9.35, -9.2)) }
)

$tmp = Join-Path ([IO.Path]::GetTempPath()) "audbol_offline"
New-Item -ItemType Directory -Force $tmp | Out-Null
$total = 0; $failed = 0
foreach ($sr in $Rates) {
  "== $sr Hz =="
  $sig = Join-Path $tmp "signals_$sr"
  if (-not (Test-Path "$sig/hits.wav")) {
    New-Item -ItemType Directory -Force $sig | Out-Null
    & $shell -NoProfile -File "$root/tools/reaper/gensignals.ps1" -OutDir $sig -SampleRate $sr | Out-Null
  }
  foreach ($c in $cases | Where-Object { $_.Name -like $Filter }) {
    $out = Join-Path $tmp "$($c.Name)_$sr.wav"
    $msg = Bench "$vst/$($c.Plugin).vst3" --render "$sig/$($c.Signal).wav" $out $c.Settings 2>&1
    if ($LASTEXITCODE -ne 0) { "{0,-20} RENDER FAILED: {1}" -f $c.Name, $msg; $failed++; continue }
    $bad = @()
    foreach ($w in $c.Windows) {
      $db = [WavPeak]::Db($out, $w[0], $w[1])
      $ok = -not [double]::IsNaN($db) -and ($null -eq $w[2] -or $db -ge $w[2]) -and ($null -eq $w[3] -or $db -le $w[3] + 0.005)
      $total++
      if (-not $ok) {
        $failed++
        $bad += "    FAIL {0:0.00}-{1:0.00} s: {2:0.00} dBFS (want {3} .. {4})" -f $w[0], $w[1], $db, $w[2], $w[3]
      }
    }
    "{0,-20} {1} / {2} windows  {3}" -f $c.Name, ($c.Windows.Count - $bad.Count), $c.Windows.Count, $c.Note
    $bad
  }
}
"TOTAL: $total checks, $failed failed"
if ($failed) { exit 1 }

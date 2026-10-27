<#
Independent BS.1770-4 reference for the loudness test cases: reads a test
WAV and computes integrated loudness with a plain block list (no histogram),
so it checks both the gating and the plug-in's histogram approximation.

  & .\tools\reaper\loudness_reference.ps1 -Wav tools\reaper\signals\impulse.wav -Seconds 3.8
#>
param([Parameter(Mandatory)] [string]$Wav, [double]$Seconds = 3.8, [double]$OffsetMs = 0)
# OffsetMs shifts the 100 ms block grid against the audio (a host starts the
# meter at an arbitrary point), which matters for very short sounds.

Add-Type @"
using System; using System.IO; using System.Collections.Generic;
public static class Bs1770Ref {
  class Bq { public double b0,b1,b2,a1,a2,z1,z2;
    public double P(double x){ double y=b0*x+z1; z1=b1*x-a1*y+z2; z2=b2*x-a2*y; return y; } }
  public static double Integrated(string path, double seconds, double offsetMs) {
    byte[] d = File.ReadAllBytes(path);
    int sr = BitConverter.ToInt32(d, 24);
    int data = 44, frames = Math.Min((d.Length - data) / 4, (int)(seconds * sr));
    double f0=1681.974450955533, G=3.999843853973347, Q=0.7071752369554196;
    double K=Math.Tan(Math.PI*f0/sr), Vh=Math.Pow(10,G/20), Vb=Math.Pow(Vh,0.4996667741545416), a0=1+K/Q+K*K;
    double f1=38.13547087602444, Q1=0.5003270373238773, K1=Math.Tan(Math.PI*f1/sr), a1=1+K1/Q1+K1*K1;
    var sh = new Bq[2]; var hp = new Bq[2];
    for (int c = 0; c < 2; c++) {
      sh[c] = new Bq{b0=(Vh+Vb*K/Q+K*K)/a0, b1=2*(K*K-Vh)/a0, b2=(Vh-Vb*K/Q+K*K)/a0, a1=2*(K*K-1)/a0, a2=(1-K/Q+K*K)/a0};
      hp[c] = new Bq{b0=1, b1=-2, b2=1, a1=2*(K1*K1-1)/a1, a2=(1-K1/Q1+K1*K1)/a1};
    }
    var z = new double[frames];
    for (int i = 0; i < frames; i++)
      for (int c = 0; c < 2; c++) {
        double x = BitConverter.ToInt16(d, data + i * 4 + c * 2) / 32767.0;
        double k = hp[c].P(sh[c].P(x)); z[i] += k * k;
      }
    // 400 ms blocks every 100 ms
    int block = (int)(0.4 * sr), hop = (int)(0.1 * sr);
    var blocks = new List<double>();
    for (int s = (int)(offsetMs * 0.001 * sr) - block; s + block <= frames; s += hop) {
      if (s < 0) { double e0 = 0; for (int i = 0; i < s + block; i++) e0 += z[i]; blocks.Add(e0 / block); continue; }
      double e = 0; for (int i = s; i < s + block; i++) e += z[i]; blocks.Add(e / block);
    }
    Func<double,double> L = e => -0.691 + 10 * Math.Log10(e);
    var abs = blocks.FindAll(e => e > 0 && L(e) > -70);
    if (abs.Count == 0) return -70;
    double m = 0; foreach (var e in abs) m += e; m /= abs.Count;
    double gate = L(m) - 10;
    var rel = abs.FindAll(e => L(e) >= gate);
    double r = 0; foreach (var e in rel) r += e; r /= rel.Count;
    return L(r);
  }
}
"@
"{0:F2}" -f [Bs1770Ref]::Integrated((Resolve-Path $Wav).Path, $Seconds, $OffsetMs)

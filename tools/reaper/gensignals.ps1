param([string]$OutDir)
Add-Type -TypeDefinition @"
using System; using System.IO;
public static class Gen {
  const int SR = 48000;
  static void Write(string path, float[] l, float[] r) {
    using (var w = new BinaryWriter(File.Create(path))) {
      int n = l.Length, data = n * 4;
      w.Write("RIFF".ToCharArray()); w.Write(36 + data); w.Write("WAVE".ToCharArray());
      w.Write("fmt ".ToCharArray()); w.Write(16); w.Write((short)1); w.Write((short)2);
      w.Write(SR); w.Write(SR * 4); w.Write((short)4); w.Write((short)16);
      w.Write("data".ToCharArray()); w.Write(data);
      for (int i = 0; i < n; i++) {
        w.Write((short)Math.Max(-32767, Math.Min(32767, l[i] * 32767)));
        w.Write((short)Math.Max(-32767, Math.Min(32767, r[i] * 32767)));
      }
    }
  }
  public static void Drums(string path, double seconds) {
    int n = (int)(SR * seconds); var l = new float[n]; var r = new float[n];
    var rng = new Random(1234); double phase = 0, last = 0, beat = 0.5;
    for (int i = 0; i < n; i++) {
      double t = (double)i / SR, inBeat = t % beat, inEighth = t % (beat / 2);
      int bi = (int)(t / beat) % 4;
      double f = 50 + 110 * Math.Exp(-inBeat * 30); phase += 2 * Math.PI * f / SR;
      double kick = 0.9 * Math.Sin(phase) * Math.Exp(-inBeat * 7);
      double w = rng.NextDouble() * 2 - 1;
      double snare = (bi == 1 || bi == 3) ? 0.45 * w * Math.Exp(-inBeat * 18) : 0;
      double w2 = rng.NextDouble() * 2 - 1; double hat = 0.18 * (w2 - last) * Math.Exp(-inEighth * 90); last = w2;
      double pad = 0.06 * (Math.Sin(2 * Math.PI * 220 * t) + Math.Sin(2 * Math.PI * 277.2 * t));
      l[i] = (float)(kick + snare + hat * 0.6 + pad); r[i] = (float)(kick + snare + hat + pad);
    }
    Write(path, l, r);
  }
  // Short percussive hit every 4 s, silence in between (for reverb tails).
  public static void Impulse(string path, double seconds) {
    int n = (int)(SR * seconds); var l = new float[n]; var r = new float[n];
    var rng = new Random(7); int hitLen = SR / 10;
    for (int start = 0; start < n; start += SR * 4)
      for (int i = 0; i < hitLen && start + i < n; i++) {
        double t = (double)i / SR, env = Math.Exp(-t * 60);
        double v = 0.7 * env * (0.6 * (rng.NextDouble() * 2 - 1) + 0.4 * Math.Sin(2 * Math.PI * 200 * t));
        l[start + i] = r[start + i] = (float)v;
      }
    Write(path, l, r);
  }
  // Steady sine at -12 dBFS.
  public static void Sine(string path, double seconds, double freq) {
    int n = (int)(SR * seconds); var l = new float[n]; var r = new float[n];
    double a = Math.Pow(10, -12.0 / 20);
    for (int i = 0; i < n; i++) l[i] = r[i] = (float)(a * Math.Sin(2 * Math.PI * freq * i / SR));
    Write(path, l, r);
  }
  public static void Pink(string path, double seconds) {
    int n = (int)(SR * seconds); var l = new float[n]; var r = new float[n];
    var rng = new Random(99); double b0 = 0, b1 = 0, b2 = 0;
    for (int i = 0; i < n; i++) {
      double w = rng.NextDouble() * 2 - 1;
      b0 = 0.99765 * b0 + w * 0.0990460; b1 = 0.96300 * b1 + w * 0.2965164; b2 = 0.57000 * b2 + w * 1.0526913;
      l[i] = r[i] = (float)((b0 + b1 + b2 + w * 0.1848) * 0.05);
    }
    Write(path, l, r);
  }
}
"@
[Gen]::Drums("$OutDir\drums.wav", 16)
[Gen]::Pink("$OutDir\pink.wav", 16)
[Gen]::Impulse("$OutDir\impulse.wav", 16)
[Gen]::Sine("$OutDir\sine.wav", 16, 1000.0)
[Gen]::Sine("$OutDir\bass.wav", 16, 60.0)
Get-ChildItem "$OutDir\*.wav" | Select-Object Name, Length

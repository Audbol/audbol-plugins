PLUGIN = "Audbol Utility"
SIGNAL = "bass"  -- 60 Hz at -12 dBFS, identical in L and R
-- Invert R to turn the bass into a pure side signal, then Bass Mono at
-- 240 Hz removes side content below it (24 dB/oct, two octaves -> ~48 dB).
SETTINGS = {
  {"Invert Right", 1.0},
  {"Bass Mono", 1.0},
  {"Bass Mono Freq", 0.772},  -- 240 Hz (log 20..500)
}
MEASURE_START = 0.5
EXPECT_MAX_PEAK_DB = -55.0

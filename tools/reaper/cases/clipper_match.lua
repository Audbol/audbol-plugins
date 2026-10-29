PLUGIN = "Audbol Clipper"
SIGNAL = "sine"
MEASURE_START = 0.5
SETTINGS = {
  {"Input", 0.666667},  -- +12 dB
  {"Ceiling", 0.75},    -- -6 dB
  {"Softness", 0.0},
  {"Gain Match", 1.0},  -- output -12 dB
}
-- Clipped at -6 dBFS, then turned down by the input gain: -18 dBFS.
EXPECT_MIN_PEAK_DB = -18.2
EXPECT_MAX_PEAK_DB = -17.9

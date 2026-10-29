PLUGIN = "Audbol Clipper"
SIGNAL = "sine"   -- 1 kHz at -12 dBFS
MEASURE_START = 0.5
SETTINGS = {
  {"Input", 0.666667},  -- +12 dB: the sine reaches 0 dBFS
  {"Ceiling", 0.75},    -- -6 dB
  {"Softness", 0.0},    -- hard
}
-- Hard clipping at -6 dBFS: the output must never exceed the ceiling.
EXPECT_MIN_PEAK_DB = -6.2
EXPECT_MAX_PEAK_DB = -6.0

PLUGIN = "Audbol Utility"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
SETTINGS = {
  {"Gain", 42 / 72},  -- -6 dB (range -48..+24)
}
MEASURE_START = 0.5
EXPECT_MIN_PEAK_DB = -18.15
EXPECT_MAX_PEAK_DB = -17.85

PLUGIN = "Audbol Saturation"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
SETTINGS = {
  {"Model", 0.0},       -- Tape
  {"Drive", 0.5},       -- +18 dB
  {"Auto Gain", 1.0},
}
-- Auto gain matches RMS to the input, so the peak lands near the input's
-- -12 dBFS despite 18 dB of drive (after it settles).
MEASURE_START = 1.0
EXPECT_MIN_PEAK_DB = -15.0
EXPECT_MAX_PEAK_DB = -9.0

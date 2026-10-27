PLUGIN = "Audbol Utility"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS, identical in L and R
-- Invert the right channel, then collapse to mono (width 0 %): the mid
-- signal L + (-R) is zero, so the output must be silent.
SETTINGS = {
  {"Invert Right", 1.0},
  {"Width", 0.0},
}
MEASURE_START = 0.5
EXPECT_MAX_PEAK_DB = -80.0

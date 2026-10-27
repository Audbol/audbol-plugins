PLUGIN = "Audbol Compressor"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
-- Hard knee, -30 dB threshold, 4:1: -30 + 18/4 = -25.5 dBFS (+- detector ripple).
SETTINGS = { {"Threshold", 0.5}, {"Knee", 0.0} }
MEASURE_START = 1.0
EXPECT_MIN_PEAK_DB = -26.5
EXPECT_MAX_PEAK_DB = -24.5

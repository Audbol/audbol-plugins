PLUGIN = "Audbol EQ"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
-- A +12 dB bell centred on the tone raises it by exactly 12 dB.
SETTINGS = { {"Band 5 Freq", 0.57521}, {"Band 5 Gain", 0.7}, {"Band 5 On", 1.0} }
MEASURE_START = 0.5
EXPECT_MIN_PEAK_DB = -0.2
EXPECT_MAX_PEAK_DB = 0.2

PLUGIN = "Audbol EQ"
SIGNAL = "bass"  -- 60 Hz at -12 dBFS
-- Low cut at 240 Hz, 24 dB/oct: two octaves below -> about -48 dB.
SETTINGS = { {"Band 1 Freq", 0.39694}, {"Band 1 On", 1.0} }
MEASURE_START = 0.5
EXPECT_MAX_PEAK_DB = -55.0

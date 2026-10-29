PLUGIN = "Audbol Clipper"
SIGNAL = "sine"
MEASURE_START = 0.5
SETTINGS = {}   -- defaults: ceiling -0.3 dB, softness 30 %
-- Well below the knee the clipper must not touch the signal.
EXPECT_MIN_PEAK_DB = -12.1
EXPECT_MAX_PEAK_DB = -11.9

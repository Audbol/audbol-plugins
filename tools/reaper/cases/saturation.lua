PLUGIN = "Audbol Saturation"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
SETTINGS = {
  {"Model", 0.75},       -- Hard
  {"Drive", 24 / 36},    -- +24 dB
  {"Auto Gain", 0.0},
}
-- +24 dB into a hard clipper pins the output at full scale. The oversampling
-- filter may overshoot a little (Gibbs), so allow up to +1.5 dBFS.
MEASURE_START = 0.5
EXPECT_MIN_PEAK_DB = -1.0
EXPECT_MAX_PEAK_DB = 1.5

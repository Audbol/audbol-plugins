PLUGIN = "Audbol Clipper"
SIGNAL = "sine"
MEASURE_START = 0.5
SETTINGS = {
  {"Input", 0.666667},  -- +12 dB: peak 1.0
  {"Ceiling", 0.75},    -- -6 dB
  {"Softness", 0.0},
  {"Delta", 1.0},
}
-- Delta plays what was cut off: the part of the sine above 0.5, peak about
-- 0.5 (-6 dBFS), give or take the band-limiting ripple.
EXPECT_MIN_PEAK_DB = -7.5
EXPECT_MAX_PEAK_DB = -4.5

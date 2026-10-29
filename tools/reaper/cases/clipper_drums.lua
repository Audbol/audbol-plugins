PLUGIN = "Audbol Clipper"
SIGNAL = "drums"
SETTINGS = {
  {"Input", 0.666667},  -- +12 dB into the clipper
  {"Ceiling", 0.958333},-- -1 dB
}
-- Loud, dense material: the ceiling must hold at every sample.
EXPECT_MAX_PEAK_DB = -1.0
EXPECT_MIN_PEAK_DB = -1.5

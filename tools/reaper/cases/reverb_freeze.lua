PLUGIN = "Audbol Reverb"
SIGNAL = "impulse"
SETTINGS = {
  {"Mix", 0.5},
}
-- Engage Freeze just after the first hit, then measure 3.0-3.8 s later.
-- A normal 2.5 s tail would be far below -60 dBFS by then; a frozen one
-- must still be there, and must not build up.
LATE_SETTINGS = {
  {"Freeze", 1.0},
}
LATE_AT = 0.3
MEASURE_START = 3.0
EXPECT_MIN_PEAK_DB = -60.0
EXPECT_MAX_PEAK_DB = -6.0

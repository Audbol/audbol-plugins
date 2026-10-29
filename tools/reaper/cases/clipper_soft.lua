PLUGIN = "Audbol Clipper"
SIGNAL = "sine"
MEASURE_START = 0.5
SETTINGS = {
  {"Input", 0.416667},  -- +3 dB: peak 0.355
  {"Ceiling", 0.75},    -- -6 dB (0.501)
  {"Softness", 1.0},    -- knee from 0.25 to 0.75
}
-- Inside the knee the peak is rounded down from -9.0 to about -9.3 dBFS
-- (a hard clipper would leave it alone).
EXPECT_MIN_PEAK_DB = -9.55
EXPECT_MAX_PEAK_DB = -9.1

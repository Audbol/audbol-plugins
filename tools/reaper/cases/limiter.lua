PLUGIN = "Audbol Limiter"
SIGNAL = "drums"
SETTINGS = {
  {"Input Gain", 0.4},    -- +12 dB into the limiter
  {"Ceiling", 0.916667},  -- -1.0 dB
}
-- Output (sample) peak must stay at or below the ceiling.
EXPECT_MAX_PEAK_DB = -1.0

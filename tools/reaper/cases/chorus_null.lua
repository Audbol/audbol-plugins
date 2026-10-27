PLUGIN = "Audbol Chorus"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
-- One static voice delayed by half a period (0.5 ms at 1 kHz), mixed 50/50
-- with the dry signal: the two must cancel.
SETTINGS = {
  {"Voices", 0.0},     -- 1
  {"Depth", 0.0},
  {"Delay", 0.0},      -- 0.5 ms
  {"Feedback", 0.5},   -- 0 %
  {"Low Cut", 0.0},    -- off
  {"High Cut", 1.0},   -- off
  {"Mix", 0.5},
}
MEASURE_START = 0.5
EXPECT_MAX_PEAK_DB = -40.0

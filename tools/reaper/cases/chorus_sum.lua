PLUGIN = "Audbol Chorus"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
-- One static voice delayed by a full period (1 ms) is in phase with the dry
-- signal; the linear 50/50 mix must give back exactly the input level.
SETTINGS = {
  {"Voices", 0.0},
  {"Depth", 0.0},
  {"Delay", 0.15818},  -- 1.0 ms (log 0.5..40 ms)
  {"Feedback", 0.5},
  {"Low Cut", 0.0},
  {"High Cut", 1.0},
  {"Mix", 0.5},
}
MEASURE_START = 0.5
EXPECT_MIN_PEAK_DB = -12.3
EXPECT_MAX_PEAK_DB = -11.7

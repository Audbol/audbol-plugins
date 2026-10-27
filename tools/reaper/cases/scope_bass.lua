PLUGIN = "Audbol Scope"
SIGNAL = "bass"  -- 60 Hz at -12 dBFS
-- Long periods (16.7 ms) still measure exactly; a 50 ms window shows three.
SETTINGS = {
  {"Time", 0.5147},  -- 50 ms
}
EXPECT_PARAMS = {
  {"Frequency", 59.95, 60.05, {0, 20000}},
  {"Peak", -12.1, -11.9},
}

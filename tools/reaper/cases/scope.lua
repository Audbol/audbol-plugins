PLUGIN = "Audbol Scope"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
-- Rising-edge trigger on a 3 ms window (three cycles). The frequency
-- measurement must read 1 kHz and the peak -12 dBFS; audio passes through.
SETTINGS = {
  {"Time", 0.1445},  -- 3 ms (log 1..2000 ms)
}
EXPECT_PARAMS = {
  {"Frequency", 999.5, 1000.5, {0, 20000}},  -- raw value: display text rounds to 10 Hz
  {"Peak", -12.1, -11.9},
}
EXPECT_MIN_PEAK_DB = -12.05
EXPECT_MAX_PEAK_DB = -11.95

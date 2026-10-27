PLUGIN = "Audbol Delay"
SIGNAL = "impulse"  -- a 0.1 s hit every 4 s
SETTINGS = {
  {"Sync", 0.0},
  {"Time Left", 0.7493},  -- 500 ms (log 1..4000 ms)
  {"Feedback", 0.5},
  {"Mix", 0.5},
}
-- Echoes must land at 0.5 s and 1.0 s, with silence just before each.
-- (REAPER's track meter takes ~0.3 s to fall below -45 dB after a hit, so
-- silence windows start at least that long after the previous sound.)
WINDOWS = {
  {from = 0.36, to = 0.46, max = -45},
  {from = 0.48, to = 0.65, min = -20},
  {from = 0.86, to = 0.96, max = -45},
  {from = 0.98, to = 1.15, min = -26},
}

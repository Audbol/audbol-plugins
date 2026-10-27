PLUGIN = "Audbol Delay"
SIGNAL = "impulse"
TEMPO = 100  -- 1/8 note = 300 ms
SETTINGS = {
  {"Sync", 1.0},
  {"Note Left", 5 / 13},  -- 1/8
  {"Mode", 0.5},          -- Ping-Pong
  {"Feedback", 0.6},
  {"Mix", 0.5},
}
-- Synced to the project tempo, echoes alternate: left at 0.3 s, right at 0.6 s.
-- Silence windows start >= 0.3 s after the previous sound on that channel
-- (REAPER's meter fall time).
WINDOWS = {
  {from = 0.30, to = 0.42, ch = 0, min = -20},
  {from = 0.42, to = 0.55, ch = 1, max = -45},
  {from = 0.60, to = 0.72, ch = 1, min = -26},
  {from = 0.72, to = 0.85, ch = 0, max = -45},
}

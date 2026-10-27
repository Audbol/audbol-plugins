PLUGIN = "Audbol Loudness"
SIGNAL = "drums"  -- peaks hit full scale
-- Real program material for the display. The loop touches 0 dBFS, so the
-- true peak must read at or above it (and get flagged over -1 dBTP).
SETTINGS = {}
EXPECT_PARAMS = {
  {"True Peak Max", -0.1, 3.0},
}

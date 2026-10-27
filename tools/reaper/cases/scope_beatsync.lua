PLUGIN = "Audbol Scope"
SIGNAL = "drums"  -- 120 BPM loop
TEMPO = 120
-- Beat Sync over one bar: the sweep follows the song position, so each kick
-- lands on a beat line. Drums aren't periodic, so no frequency is reported.
SETTINGS = {
  {"Trigger", 1.0},          -- Beat Sync
  {"Beat Length", 4 / 6},    -- 1 bar
}
EXPECT_PARAMS = {
  {"Frequency", 0, 0, {0, 20000}},
}

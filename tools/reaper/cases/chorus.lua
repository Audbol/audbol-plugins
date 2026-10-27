PLUGIN = "Audbol Chorus"
SIGNAL = "drums"  -- peaks at 0 dBFS, mostly kick (low, voice-correlated)
-- Default chorus with three voices. A chorus must not pump up the level:
-- correlated low end stays at unity, so peaks stay near the dry 0 dBFS.
SETTINGS = {
  {"Voices", 2 / 3},  -- 3
}
EXPECT_MAX_PEAK_DB = 1.0

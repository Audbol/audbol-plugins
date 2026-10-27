PLUGIN = "Audbol Chorus"
SIGNAL = "pink"
-- Flanger extreme: short delay, full depth, +90 % feedback. Resonances may
-- boost peaks, but the output must stay bounded (no runaway) and not mute.
SETTINGS = {
  {"Voices", 0.0},
  {"Delay", 0.15818},  -- 1 ms
  {"Depth", 1.0},
  {"Feedback", 1.0},   -- +90 %
  {"Rate", 0.6},       -- ~1.2 Hz
  {"Mix", 0.5},
}
MEASURE_START = 0.5
EXPECT_MAX_PEAK_DB = 12.0
EXPECT_MIN_PEAK_DB = -40.0

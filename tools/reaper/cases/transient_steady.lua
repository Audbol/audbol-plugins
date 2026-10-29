PLUGIN = "Audbol Transient Shaper"
SIGNAL = "bass"   -- 60 Hz at -12 dBFS
MEASURE_START = 0.5
SETTINGS = {
  {"Attack", 1.0},
  {"Sustain", 1.0},
}
-- A steady tone has no attack and no decay: even at full settings it must
-- pass unchanged (no ripple-driven modulation on low notes).
EXPECT_MIN_PEAK_DB = -12.2
EXPECT_MAX_PEAK_DB = -11.8

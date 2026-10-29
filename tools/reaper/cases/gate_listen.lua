PLUGIN = "Audbol Gate"
SIGNAL = "sine"   -- 1 kHz at -12 dBFS
MEASURE_START = 0.5
SETTINGS = {
  {"Key High-Pass", 1.0}, -- 4 kHz, 12 dB/octave
  {"Key Listen", 1.0},
}
-- Listen plays the key filter: two octaves below the cutoff, about 24 dB
-- down, so about -36 dBFS.
EXPECT_MIN_PEAK_DB = -37.0
EXPECT_MAX_PEAK_DB = -35.2

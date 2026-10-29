PLUGIN = "Audbol De-Esser"
SIGNAL = "sine"           -- 1 kHz sine at -12 dBFS
MEASURE_START = 0.5     -- skip the first cycle, before the detector reacts
SETTINGS = {
  {"Listen", 1.0},
}
-- Listen plays the sidechain: the 6.5 kHz band-pass (Q 1.5) passes 1 kHz
-- about 20.2 dB down, so about -32.2 dBFS.
EXPECT_MIN_PEAK_DB = -33.5
EXPECT_MAX_PEAK_DB = -31.0

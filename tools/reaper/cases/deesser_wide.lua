PLUGIN = "Audbol De-Esser"
SIGNAL = "ess"            -- 7 kHz sine at -12 dBFS
MEASURE_START = 0.5       -- skip the first cycle, before the detector reacts
SETTINGS = {
  {"Range", 0.5},         -- 12 dB
  {"Filter", 1.0},        -- High (above 6.5 kHz)
  {"Mode", 1.0},          -- Wide: the whole signal is turned down
}
-- The high-pass sidechain hears the tone at about -14.4 dBFS, 15.6 dB over
-- the -30 dB threshold, so the full 12 dB range applies: about -24 dBFS.
EXPECT_MIN_PEAK_DB = -25.5
EXPECT_MAX_PEAK_DB = -22.5

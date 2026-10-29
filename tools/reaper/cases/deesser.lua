PLUGIN = "Audbol De-Esser"
SIGNAL = "ess"            -- 7 kHz sine at -12 dBFS: a steady "s"
MEASURE_START = 0.5     -- skip the first cycle, before the detector reacts
SETTINGS = {
  {"Threshold", 0.5},     -- -30 dB
  {"Range", 0.333333},    -- 8 dB
}
-- The tone sits in the detection band (6.5 kHz), well over the threshold:
-- Split mode turns it down by about 7 dB (the tone is a little off the
-- band centre, so the band is not quite in phase with it): about -19 dBFS.
EXPECT_MIN_PEAK_DB = -21.5
EXPECT_MAX_PEAK_DB = -18.0

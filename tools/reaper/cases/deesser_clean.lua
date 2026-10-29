PLUGIN = "Audbol De-Esser"
SIGNAL = "sine"           -- 1 kHz sine at -12 dBFS
MEASURE_START = 0.5     -- skip the first cycle, before the detector reacts
SETTINGS = {
  {"Threshold", 0.5},     -- -30 dB
  {"Range", 0.333333},    -- 8 dB
}
-- Below the detection band the sidechain stays under the threshold, and the
-- split band barely touches 1 kHz: the tone passes unchanged.
EXPECT_MIN_PEAK_DB = -12.5
EXPECT_MAX_PEAK_DB = -11.5

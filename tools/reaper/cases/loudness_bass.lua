PLUGIN = "Audbol Loudness"
SIGNAL = "bass"  -- 60 Hz at -12 dBFS
-- K-weighting rolls off the low end: -2.90 dB at 60 Hz, so the same -12 dBFS
-- sine reads -15.59 LUFS (see loudness_reference.ps1).
SETTINGS = {}
EXPECT_PARAMS = {
  {"Integrated", -15.7, -15.5},
  {"Short-Term", -15.7, -15.5},
}

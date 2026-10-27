PLUGIN = "Audbol Loudness"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS in both channels
-- BS.1770: -0.691 + 20 log10(A) + K-weighting gain at 1 kHz (+0.698 dB)
-- = -11.99 LUFS for momentary, short-term and integrated. A steady tone has
-- no loudness range, and its true peak is its sample peak (-12 dBTP).
-- (The audio passes through unchanged: active and bypassed peaks match.)
SETTINGS = {}
EXPECT_PARAMS = {
  {"Momentary", -12.1, -11.9},
  {"Short-Term", -12.1, -11.9},
  {"Integrated", -12.1, -11.9},
  {"Loudness Range", 0.0, 0.2},
  {"True Peak Max", -12.15, -11.85},
}
EXPECT_MIN_PEAK_DB = -12.05
EXPECT_MAX_PEAK_DB = -11.95

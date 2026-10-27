PLUGIN = "Audbol Saturation"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
SETTINGS = {
  {"Model", 0.25},  -- Tube
  {"Drive", 0.5},   -- +18 dB
  {"Bias", 0.75},   -- +50 %: even harmonics in the display
  {"Mix", 0.0},     -- fully dry: the latency-aligned dry path must be untouched
}
MEASURE_START = 0.5
EXPECT_MIN_PEAK_DB = -12.2
EXPECT_MAX_PEAK_DB = -11.8

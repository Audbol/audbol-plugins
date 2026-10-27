PLUGIN = "Audbol Reverb"
SIGNAL = "impulse"  -- a hit every 4 s, silence in between
SETTINGS = {
  {"Mix", 0.5},        -- 50 %
}
-- Measure 0.6 s after the hit (the dry hit lasts ~0.1 s): only the reverb
-- tail is left there, so the plug-in must still produce sound.
MEASURE_START = 0.6
EXPECT_MIN_PEAK_DB = -60.0

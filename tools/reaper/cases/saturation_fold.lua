PLUGIN = "Audbol Saturation"
SIGNAL = "sine"
-- Wavefolder at +36 dB stays bounded by full scale (plus filter overshoot).
SETTINGS = { {"Model", 1.0}, {"Drive", 1.0}, {"Auto Gain", 0.0} }
MEASURE_START = 0.5
EXPECT_MIN_PEAK_DB = -3.0
EXPECT_MAX_PEAK_DB = 1.5

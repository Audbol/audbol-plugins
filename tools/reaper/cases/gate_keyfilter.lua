PLUGIN = "Audbol Gate"
SIGNAL = "bursts"
SETTINGS = {
  {"Key High-Pass", 1.0}, -- 4 kHz
  {"Threshold", 0.625},   -- -30 dB
}
-- The key filter puts the 1 kHz tone about 24 dB down (-36 dBFS), below the
-- threshold, so the gate never opens and the tone stays 40 dB down. The key
-- filter only feeds the detector; it must not change the audio.
WINDOWS = {
  {from = 1.01, to = 1.24, max = -50},
  {from = 2.01, to = 2.24, max = -50},
}

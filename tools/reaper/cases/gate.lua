PLUGIN = "Audbol Gate"
SIGNAL = "bursts"  -- 1 kHz at -12 dBFS for 250 ms every second, noise at -60 dBFS throughout
SETTINGS = {}      -- defaults: Gate, threshold -40 dB, range 40 dB
-- The tone opens the gate and must pass untouched. (How far the gate closes
-- between the tones is checked sample-accurately by tools/bench/offline_tests.ps1;
-- REAPER's meters fall too slowly to see -100 dBFS here.)
WINDOWS = {
  {from = 1.01, to = 1.24, min = -12.3, max = -11.8},
  {from = 2.01, to = 2.24, min = -12.3, max = -11.8},
}

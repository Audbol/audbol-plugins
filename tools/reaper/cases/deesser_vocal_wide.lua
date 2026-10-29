PLUGIN = "Audbol De-Esser"
SIGNAL = "vocal"  -- 150 Hz buzz at -18 dBFS + a 120 ms "s" burst (6.5 kHz) every second
SETTINGS = {
  {"Threshold", 0.5},  -- -30 dB
  {"Range", 0.333333}, -- 8 dB
  {"Mode", 1.0},
}
-- Untouched (Range 0) the bursts peak at -10.2 / -10.5 dBFS and the buzz
-- alone at -18.00. The bursts must come down; between them the voice must
-- pass unchanged (no pumping, nothing left over from the release). Wide
-- turns the voice down with the "s", so its bursts end up lower than Split's.
WINDOWS = {
  {from = 1.00, to = 1.45, min = -18.1, max = -17.9},
  {from = 1.50, to = 1.62, max = -16.5},
  {from = 2.00, to = 2.45, min = -18.1, max = -17.9},
  {from = 2.50, to = 2.62, max = -16.5},
}

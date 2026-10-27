PLUGIN = "Audbol Utility"
SIGNAL = "sine"  -- 1 kHz at -12 dBFS
-- Balance hard left: the right channel goes silent, the left is untouched.
SETTINGS = {
  {"Pan", 0.0},  -- L 100
}
WINDOWS = {
  {from = 0.5, to = 3.0, ch = 0, min = -12.2, max = -11.8},
  {from = 0.5, to = 3.0, ch = 1, max = -90},
}

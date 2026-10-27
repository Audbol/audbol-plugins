PLUGIN = "Audbol Delay"
SIGNAL = "impulse"
-- Cross feedback, unlinked: L echo at 0.4 s, R echo at 0.7 s; the other side
-- stays silent at those moments.
SETTINGS = { {"Sync", 0.0}, {"Link L/R", 0.0}, {"Time Left", 0.7224}, {"Time Right", 0.7899},
             {"Mode", 1.0}, {"Feedback", 0.5}, {"Mix", 0.5} }
WINDOWS = {
  {from = 0.40, to = 0.50, ch = 0, min = -20},
  {from = 0.40, to = 0.50, ch = 1, max = -45},
  {from = 0.70, to = 0.80, ch = 1, min = -20},
  {from = 0.82, to = 0.95, ch = 0, max = -45},
}

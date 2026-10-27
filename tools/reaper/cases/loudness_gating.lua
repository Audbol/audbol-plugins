PLUGIN = "Audbol Loudness"
SIGNAL = "impulse"  -- one 0.1 s hit, then silence until 4 s
-- Integrated loudness must ignore the silence (absolute gate at -70 LUFS)
-- and blocks more than 10 LU below the mean (relative gate). The independent
-- block-list reference (loudness_reference.ps1) gives -22.04 .. -23.01 LUFS
-- depending on where the 100 ms block grid falls against the 0.1 s hit
-- (a host starts the meter at an arbitrary point); read before the next hit.
SETTINGS = {}
PARAMS_AT = 3.0
EXPECT_PARAMS = {
  {"Integrated", -23.1, -21.9},
}

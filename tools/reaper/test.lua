-- Suite plug-in test, run by run.ps1. Loads the plug-in on a track playing a
-- test signal, logs its parameters, opens its editor and compares the track's
-- output peak with the plug-in active vs. bypassed.
-- The case file (configs/*.lua) defines PLUGIN, SIGNAL and SETTINGS.
SCRIPT_DIR = debug.getinfo(1, "S").source:match("^@(.*[\\/])")
dofile(SCRIPT_DIR .. "current_case.lua")
local WAV = SCRIPT_DIR .. "signals\\" .. SIGNAL .. ".wav"

local log = io.open(os.getenv("TEMP") .. "\\reaper_test.txt", "w")
local function L(...)
  local t = {}
  for i = 1, select("#", ...) do t[#t + 1] = tostring(select(i, ...)) end
  log:write(table.concat(t, " "), "\n")
  log:flush()
end

local _, mode = reaper.GetAudioDeviceInfo("MODE", "")
local _, srate = reaper.GetAudioDeviceInfo("SRATE", "")
L("audio:", mode, srate)

reaper.InsertTrackAtIndex(0, true)
local tr = reaper.GetTrack(0, 0)
reaper.SetOnlyTrackSelected(tr)
reaper.SetEditCurPos(0, false, false)
reaper.InsertMedia(WAV, 0)

local fx = reaper.TrackFX_AddByName(tr, PLUGIN, false, -1)
L("fx index:", fx)
if fx < 0 then L("FAILED: plug-in not found"); return end
local _, fxname = reaper.TrackFX_GetFXName(tr, fx, "")
local n = reaper.TrackFX_GetNumParams(tr, fx)
L("fx name:", fxname, "params:", n)

local byName = {}
for i = 0, n - 1 do
  local _, pn = reaper.TrackFX_GetParamName(tr, fx, i, "")
  local _, fv = reaper.TrackFX_GetFormattedParamValue(tr, fx, i, "")
  if byName[pn] == nil then byName[pn] = i end -- REAPER appends its own "Bypass"
  if i < 40 then L(string.format("  %2d %-22s %s", i, pn, fv)) end
end

local function set(name, normalized)
  local i = byName[name]
  if not i then L("missing param", name) return end
  reaper.TrackFX_SetParamNormalized(tr, fx, i, normalized)
  local _, fv = reaper.TrackFX_GetFormattedParamValue(tr, fx, i, "")
  L("set", name, "->", fv)
end
for _, s in ipairs(SETTINGS) do set(s[1], s[2]) end
local okPdc, pdc = reaper.TrackFX_GetNamedConfigParm(tr, fx, "pdc")
L("reported latency (samples):", okPdc and pdc or "n/a")

L("items on track:", reaper.CountTrackMediaItems(tr), "project length:", reaper.GetProjectLength(0))
reaper.GetSet_LoopTimeRange(true, true, 0, 16, false)
reaper.GetSetRepeat(1)
reaper.TrackFX_Show(tr, fx, 3)

-- Max of the live track peak, sampled on every defer cycle; peak() returns
-- the max since the previous call, in dBFS.
local maxPeak = 0
local function sample()
  maxPeak = math.max(maxPeak, reaper.Track_GetPeakInfo(tr, 0), reaper.Track_GetPeakInfo(tr, 1))
end
local function peak()
  local p = maxPeak
  maxPeak = 0
  return p > 0 and 20 * math.log(p, 10) or -150
end

-- Timed windows (seconds of audible play position, first pass of the loop):
-- WINDOWS = { {from = 0.48, to = 0.62, min = -20, max = nil, ch = nil}, ... }
-- ch = 0 / 1 checks one channel, otherwise the louder of the two.
local windowMax = {}
local function sampleWindows()
  local pos = reaper.GetPlayPosition()
  if not WINDOWS or pos > 15 then return end
  for i, w in ipairs(WINDOWS) do
    if pos >= w.from and pos <= w.to then
      local p = w.ch and reaper.Track_GetPeakInfo(tr, w.ch)
                or math.max(reaper.Track_GetPeakInfo(tr, 0), reaper.Track_GetPeakInfo(tr, 1))
      windowMax[i] = math.max(windowMax[i] or 0, p)
    end
  end
end
local function reportWindows()
  for i, w in ipairs(WINDOWS or {}) do
    local m = windowMax[i]
    local db = (m and m > 0) and 20 * math.log(m, 10) or -150
    local ok = m ~= nil and (not w.min or db >= w.min) and (not w.max or db <= w.max)
    L(string.format("%s window %.2f-%.2f s %s: %.2f dBFS (want %s%s)", ok and "PASS" or "FAIL",
      w.from, w.to, w.ch and ("ch " .. w.ch) or "L/R", db,
      w.min and (">= " .. w.min) or "", w.max and (" <= " .. w.max) or ""))
  end
end

-- Checks on the plug-in's own parameter values as REAPER shows them (e.g.
-- meter outputs): EXPECT_PARAMS = { {"Integrated", min, max}, ... }, read at
-- PARAMS_AT seconds after playback starts (default 3.7).
local function checkParams()
  for _, p in ipairs(EXPECT_PARAMS or {}) do
    local i = byName[p[1]]
    if not i then
      L("FAIL param", p[1], "not found")
    else
      local _, text = reaper.TrackFX_GetFormattedParamValue(tr, fx, i, "")
      local v = text:find("inf") and -150 or tonumber(text:match("[-+]?%d+%.?%d*"))
      if v and text:find("kHz") then v = v * 1000 end
      -- Optional {lo, hi}: read the raw normalized value of a linear
      -- parameter instead, for more precision than the display text has.
      if p[4] then
        v = p[4][1] + reaper.TrackFX_GetParamNormalized(tr, fx, i) * (p[4][2] - p[4][1])
        text = string.format("%.3f", v)
      end
      local ok = v ~= nil and v >= p[2] and v <= p[3]
      L(string.format("%s param %-16s %-14s (want %g .. %g)", ok and "PASS" or "FAIL", p[1], text, p[2], p[3]))
    end
  end
end

if TEMPO then
  reaper.SetCurrentBPM(0, TEMPO, false)
  L("project tempo:", TEMPO)
end

local start = reaper.time_precise()
local stage = 0
local active
local function step()
  local t = reaper.time_precise() - start
  sample()
  if stage == 1 then sampleWindows() end
  if stage == 0 and t > 1 then
    reaper.SetEditCurPos(0, true, false) -- InsertMedia leaves the cursor at the item end
    reaper.OnPlayButton()
    playStart = t
    peak()
    stage = 1
  end
  -- Active measurement window: [MEASURE_START, 3.8 s] after play starts
  -- (the impulse signal hits again at 4 s).
  if stage == 1 and t < playStart + (MEASURE_START or 0) then peak() end
  -- Optional settings applied shortly after playback starts (e.g. Freeze).
  if stage == 1 and LATE_SETTINGS and not lateDone and t > playStart + (LATE_AT or 0.3) then
    lateDone = true
    for _, s in ipairs(LATE_SETTINGS) do set(s[1], s[2]) end
  end
  if stage == 1 and EXPECT_PARAMS and not paramsChecked and t > playStart + (PARAMS_AT or 3.7) then
    paramsChecked = true
    checkParams()
  end
  if stage == 1 and t > 2 and not logged then
    logged = true
    L("play state:", reaper.GetPlayState(), "position:", reaper.GetPlayPosition())
  end
  if stage == 1 and t > playStart + 3.8 then
    active = peak()
    L(string.format("output peak, plug-in active:   %.2f dBFS", active))
    reportWindows()
    if EXPECT_MAX_PEAK_DB then
      L(active <= EXPECT_MAX_PEAK_DB + 0.01 and "PASS" or "FAIL",
        string.format("peak limit %.2f dBFS", EXPECT_MAX_PEAK_DB))
    end
    if EXPECT_MIN_PEAK_DB then
      L(active >= EXPECT_MIN_PEAK_DB and "PASS" or "FAIL",
        string.format("peak at least %.2f dBFS", EXPECT_MIN_PEAK_DB))
    end
    set("Bypass", 1)
    stage = 2
  end
  if stage == 2 and t > 6 then peak(); stage = 3 end
  if stage == 3 and t > 10 then
    local bypassed = peak()
    L(string.format("output peak, plug-in bypassed: %.2f dBFS", bypassed))
    L(string.format("difference: %.2f dB", bypassed - active))
    set("Bypass", 0)
    L("DONE")
    stage = 4
  end
  if stage < 4 then reaper.defer(step) end
end
reaper.defer(step)

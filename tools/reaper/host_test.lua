-- Host-level checks on every plug-in of the suite, run via run.ps1 -Case host:
--  1. State: random values on every parameter, then the track is rebuilt from
--     its saved state (a new instance) and every value must come back.
--  2. Stress: two instances of each plug-in in one chain play drums while
--     parameters change at random and editor windows open and close; the
--     output must stay finite and every editor must still open afterwards.
local log = io.open(os.getenv("TEMP") .. "\\reaper_test.txt", "w")
local function L(s) log:write(s, "\n") log:flush() end

math.randomseed(20261028)
local failures = 0
local function fail(s) failures = failures + 1 L("FAIL " .. s) end

-- Plug-in parameters only: REAPER appends its own Bypass / Wet / Delta.
local function ownParams(tr, fx)
  local n = reaper.TrackFX_GetNumParams(tr, fx)
  local _, last = reaper.TrackFX_GetParamName(tr, fx, n - 1, "")
  if last == "Delta" then n = n - 3 end
  return n
end

-- Runs fn after `seconds` of defer cycles (the audio engine keeps running).
local function after(seconds, fn)
  local t = reaper.time_precise()
  local function wait() if reaper.time_precise() - t < seconds then reaper.defer(wait) else fn() end end
  reaper.defer(wait)
end

-- 1. State round trip --------------------------------------------------------
-- Values are set with the transport stopped, as a user tweaking knobs before
-- saving would; the state is read a moment later.
local stateTracks = {}
for _, name in ipairs(PLUGINS) do
  reaper.InsertTrackAtIndex(reaper.CountTracks(0), true)
  local tr = reaper.GetTrack(0, reaper.CountTracks(0) - 1)
  local fx = reaper.TrackFX_AddByName(tr, name, false, -1)
  if fx < 0 then
    fail(name .. ": not found")
  else
    local n = ownParams(tr, fx)
    for i = 0, n - 1 do
      local _, pn = reaper.TrackFX_GetParamName(tr, fx, i, "")
      if pn ~= "Bypass" then reaper.TrackFX_SetParamNormalized(tr, fx, i, math.random()) end
    end
    stateTracks[#stateTracks + 1] = {tr = tr, name = name, n = n}
  end
end

local function checkState()
  for _, s in ipairs(stateTracks) do
    local want = {}
    for i = 0, s.n - 1 do want[i] = reaper.TrackFX_GetParamNormalized(s.tr, 0, i) end
    local _, chunk = reaper.GetTrackStateChunk(s.tr, "", false)
    reaper.InsertTrackAtIndex(reaper.CountTracks(0), true)
    local copy = reaper.GetTrack(0, reaper.CountTracks(0) - 1)
    reaper.SetTrackStateChunk(copy, chunk, false)
    local wrong = 0
    for i = 0, s.n - 1 do
      local got = reaper.TrackFX_GetParamNormalized(copy, 0, i)
      if math.abs(got - want[i]) > 1e-4 then
        wrong = wrong + 1
        local _, pn = reaper.TrackFX_GetParamName(copy, 0, i, "")
        if wrong <= 3 then L(string.format("  %s %s: want %.5f got %.5f", s.name, pn, want[i], got)) end
      end
    end
    if wrong > 0 then fail(string.format("%s: %d of %d values not restored", s.name, wrong, s.n))
    else L(string.format("PASS state %-20s %d values restored", s.name, s.n)) end
    reaper.DeleteTrack(copy)
    reaper.DeleteTrack(s.tr)
  end
end

local startStress
after(STATE_WAIT or 1.0, function () checkState(); startStress() end)

-- 2. Stress ------------------------------------------------------------------
function startStress()
reaper.InsertTrackAtIndex(0, true)
local tr = reaper.GetTrack(0, 0)
reaper.SetOnlyTrackSelected(tr)
reaper.SetEditCurPos(0, false, false)
reaper.InsertMedia(SCRIPT_DIR .. "signals\\drums.wav", 0)
local chain = {}
for round = 1, 2 do
  for _, name in ipairs(PLUGINS) do
    local fx = reaper.TrackFX_AddByName(tr, name, false, -1)
    if fx >= 0 then chain[#chain + 1] = {fx = fx, name = name, n = ownParams(tr, fx)} end
  end
end
L(string.format("stress chain: %d instances", #chain))
reaper.GetSet_LoopTimeRange(true, true, 0, 16, false)
reaper.GetSetRepeat(1)
reaper.OnPlayButton()

local t0 = reaper.time_precise()
local cycles, toggles, edits, nonFinite = 0, 0, 0, 0
local function finite(v) return v == v and v ~= math.huge and v ~= -math.huge end

local function finish()
  reaper.OnStopButton()
  -- Every editor must still open
  local closed = 0
  for _, c in ipairs(chain) do reaper.TrackFX_Show(tr, c.fx, 2) end
  for _, c in ipairs(chain) do
    reaper.TrackFX_Show(tr, c.fx, 3)
    if not reaper.TrackFX_GetOpen(tr, c.fx) then
      closed = closed + 1
      fail(c.name .. ": editor did not open after stress")
    end
    reaper.TrackFX_Show(tr, c.fx, 2)
  end
  L(string.format("stress: %d cycles, %d window toggles, %d parameter edits, %d non-finite peaks",
    cycles, toggles, edits, nonFinite))
  if nonFinite > 0 then fail("non-finite output during stress") end
  if failures == 0 then L("PASS state restore, stress and editors for every plug-in") end
  L("DONE")
end

local function step()
  cycles = cycles + 1
  for _ = 1, 4 do
    local c = chain[math.random(#chain)]
    local i = math.random(0, c.n - 1)
    local _, pn = reaper.TrackFX_GetParamName(tr, c.fx, i, "")
    if pn ~= "Bypass" then
      reaper.TrackFX_SetParamNormalized(tr, c.fx, i, math.random())
      edits = edits + 1
    end
  end
  local c = chain[math.random(#chain)]
  reaper.TrackFX_Show(tr, c.fx, reaper.TrackFX_GetOpen(tr, c.fx) and 2 or 3)
  toggles = toggles + 1
  for ch = 0, 1 do
    if not finite(reaper.Track_GetPeakInfo(tr, ch)) then nonFinite = nonFinite + 1 end
  end
  if reaper.time_precise() - t0 < 25 then reaper.defer(step) else finish() end
end
reaper.defer(step)
end

-- Loads every factory .vstpreset through REAPER's own preset loader and
-- checks every parameter against the expected value (presets_expected.lua,
-- written by make_presets). Run via run.ps1 -Case presets.
dofile(SCRIPT_DIR .. "presets_expected.lua")

local log = io.open(os.getenv("TEMP") .. "\\reaper_test.txt", "w")
local function L(s) log:write(s, "\n") log:flush() end

local instances = {}
local checked, failed = 0, 0
for _, p in ipairs(PRESETS) do
  local inst = instances[p.plugin]
  if not inst then
    reaper.InsertTrackAtIndex(reaper.CountTracks(0), true)
    local tr = reaper.GetTrack(0, reaper.CountTracks(0) - 1)
    local fx = reaper.TrackFX_AddByName(tr, p.plugin, false, -1)
    local names = {}
    for i = 0, reaper.TrackFX_GetNumParams(tr, fx) - 1 do
      local _, n = reaper.TrackFX_GetParamName(tr, fx, i, "")
      if names[n] == nil then names[n] = i end
    end
    inst = {tr = tr, fx = fx, names = names}
    instances[p.plugin] = inst
    if fx < 0 then L("FAIL plug-in not found: " .. p.plugin) end
  end
  local loaded = inst.fx >= 0 and reaper.TrackFX_SetPreset(inst.tr, inst.fx, p.file)
  local wrong = 0
  for _, v in ipairs(p.values) do
    local i = inst.names[v[1]]
    local got = i and reaper.TrackFX_GetParamNormalized(inst.tr, inst.fx, i)
    checked = checked + 1
    if not got or math.abs(got - v[2]) > 1e-4 then
      wrong = wrong + 1
      if wrong <= 3 then
        L(string.format("  %s / %s: %s want %.5f got %s", p.plugin, p.preset, v[1], v[2], tostring(got)))
      end
    end
  end
  if not loaded or wrong > 0 then
    failed = failed + 1
    L(string.format("FAIL %s / %s (loaded=%s, %d wrong)", p.plugin, p.preset, tostring(loaded), wrong))
  end
end
-- Leave one plug-in open on a loaded preset: its header should name it.
local show = instances["Audbol Reverb"]
if show and show.fx >= 0 then
  for _, p in ipairs(PRESETS) do
    if p.plugin == "Audbol Reverb" and p.preset == "Vocal Plate" then reaper.TrackFX_SetPreset(show.tr, show.fx, p.file) end
  end
  reaper.TrackFX_Show(show.tr, show.fx, 3)
end
L(string.format("presets loaded: %d, parameter values checked: %d, failed presets: %d", #PRESETS, checked, failed))
if failed == 0 then L("PASS every preset loads with exact parameter values") end
L("DONE")

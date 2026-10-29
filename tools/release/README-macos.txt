Audbol Plugin Suite for macOS (VST3, Apple silicon and Intel, macOS 10.15+)

Install
  Copy the .vst3 bundles into ~/Library/Audio/Plug-Ins/VST3 (just for you)
  or /Library/Audio/Plug-Ins/VST3 (all users), then rescan plug-ins in your
  DAW.

  The plug-ins are not notarized by Apple, so macOS blocks files downloaded
  from the internet. After copying, clear the download flag once in
  Terminal:

    xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Audbol*.vst3

Factory presets
  The presets are built into every plug-in's preset browser. To also see
  them in your DAW's own browser, copy the "Audbol Audio" folder from
  Presets into ~/Library/Audio/Presets.

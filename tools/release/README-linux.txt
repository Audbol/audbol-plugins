Audbol Plugin Suite for Linux (VST3, x86_64)

Install
  Copy the .vst3 folders into ~/.vst3 (just for you) or /usr/lib/vst3
  (all users), then rescan plug-ins in your DAW.

    mkdir -p ~/.vst3 && cp -r Audbol*.vst3 ~/.vst3/

Factory presets
  The presets are built into every plug-in's preset browser. To also see
  them in your DAW's own browser, copy the "Audbol Audio" folder:

    mkdir -p ~/.vst3/presets && cp -r "Presets/Audbol Audio" ~/.vst3/presets/

Requirements
  glibc 2.34 and libstdc++ from GCC 11 or newer (Ubuntu 22.04, Debian 12,
  Fedora 35 or later), and an X11 session or XWayland. Most desktops have
  the libraries already; if a plug-in doesn't load, install:

    Debian/Ubuntu:  sudo apt install libxcb-cursor0 libxcb-util1 libxkbcommon-x11-0 libcairo2 libpangocairo-1.0-0
    Fedora:         sudo dnf install xcb-util-cursor xcb-util libxkbcommon-x11 cairo pango
    Arch:           sudo pacman -S xcb-util-cursor xcb-util libxkbcommon-x11 cairo pango

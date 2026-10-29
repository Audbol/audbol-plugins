; Windows installer for the Audbol plug-in suite (Inno Setup 6).
; Built by build_release.ps1, which passes AppVersion and SourceDir.

#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef PresetDir
  #define PresetDir "..\..\build\presets\Audbol Audio"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\build\VST3\Release"
#endif

[Setup]
; Keep AppId fixed forever: it ties upgrades and uninstalls together.
AppId={{EE822ABE-3A1D-4F2E-B811-BB09D4692627}
AppName=Audbol Plugin Suite
AppVersion={#AppVersion}
AppVerName=Audbol Plugin Suite {#AppVersion}
AppPublisher=Audbol Audio
AppPublisherURL=https://github.com/Audbol/audbol-plugins
AppSupportURL=https://github.com/Audbol/audbol-plugins/issues
DefaultDirName={commoncf64}\VST3\Audbol
DisableProgramGroupPage=yes
DisableWelcomePage=no
DirExistsWarning=no
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
MinVersion=10.0
WizardStyle=modern
Compression=lzma2/max
SolidCompression=yes
OutputBaseFilename=AudbolPluginSuite-{#AppVersion}-win64-setup
UninstallDisplayName=Audbol Plugin Suite
UninstallDisplayIcon={app}\AudbolEQ.vst3\PlugIn.ico
SetupIconFile=..\..\resources\brand\audbol.ico
WizardImageFile=..\..\resources\brand\wizard-164x314.bmp,..\..\resources\brand\wizard-328x628.bmp
WizardSmallImageFile=..\..\resources\brand\wizard-small-55.bmp,..\..\resources\brand\wizard-small-110.bmp
WizardImageStretch=no
WizardImageBackColor=$171311
VersionInfoVersion={#AppVersion}
VersionInfoCompany=Audbol Audio
VersionInfoDescription=Audbol Plugin Suite installer
VersionInfoCopyright=Copyright (C) 2026 Audbol Audio
VersionInfoProductName=Audbol Plugin Suite
VersionInfoProductVersion={#AppVersion}

[Messages]
WelcomeLabel1=Welcome to the Audbol Plugin Suite
WelcomeLabel2=This will install Audbol Plugin Suite {#AppVersion} on your computer: fourteen VST3 plug-ins for mixing, sound design and metering.%n%nEQ, Compressor, De-Esser, Gate, Transient Shaper, Clipper, Limiter, Reverb, Delay, Saturation, Chorus, Utility, Loudness and Scope.%n%nClose your DAW before continuing.
SelectDirLabel3=The plug-ins will be installed into the following folder. Most hosts scan Common Files\VST3 (and its subfolders) automatically.
FinishedHeadingLabel=Audbol Plugin Suite is installed
FinishedLabelNoIcons=The plug-ins are ready. Open your DAW and rescan VST3 plug-ins if they don't appear right away; you'll find them under "Audbol Audio".

[Types]
Name: "full"; Description: "All plug-ins"
Name: "custom"; Description: "Choose plug-ins"; Flags: iscustom

[Components]
Name: "eq";         Description: "Audbol EQ - 8-band parametric EQ with analyzer";         Types: full custom
Name: "compressor"; Description: "Audbol Compressor - transfer curve and GR history";        Types: full custom
Name: "deesser";    Description: "Audbol De-Esser - split-band de-esser with listen";      Types: full custom
Name: "gate";       Description: "Audbol Gate - gate / expander with key filter";          Types: full custom
Name: "transient";  Description: "Audbol Transient Shaper - attack and sustain control";  Types: full custom
Name: "clipper";    Description: "Audbol Clipper - soft / hard clipper, 4x oversampled";   Types: full custom
Name: "limiter";    Description: "Audbol Limiter - lookahead brickwall, true peak";         Types: full custom
Name: "reverb";     Description: "Audbol Reverb - algorithmic reverb with freeze";          Types: full custom
Name: "delay";      Description: "Audbol Delay - tempo-synced stereo / ping-pong delay";    Types: full custom
Name: "saturation"; Description: "Audbol Saturation - 5 models, 4x oversampled";           Types: full custom
Name: "chorus";     Description: "Audbol Chorus - 1-4 voice chorus / flanger";             Types: full custom
Name: "utility";    Description: "Audbol Utility - gain, width, bass mono, goniometer";     Types: full custom
Name: "loudness";   Description: "Audbol Loudness - LUFS meter (EBU R128)";                 Types: full custom
Name: "scope";      Description: "Audbol Scope - oscilloscope with beat sync";              Types: full custom

[Files]
Source: "{#SourceDir}\AudbolEQ.vst3\*";         DestDir: "{app}\AudbolEQ.vst3";         Components: eq;         Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolCompressor.vst3\*"; DestDir: "{app}\AudbolCompressor.vst3"; Components: compressor; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolDeEsser.vst3\*";    DestDir: "{app}\AudbolDeEsser.vst3";    Components: deesser;    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolGate.vst3\*";       DestDir: "{app}\AudbolGate.vst3";       Components: gate;       Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolTransientShaper.vst3\*"; DestDir: "{app}\AudbolTransientShaper.vst3"; Components: transient; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolClipper.vst3\*";    DestDir: "{app}\AudbolClipper.vst3";    Components: clipper;    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolLimiter.vst3\*";    DestDir: "{app}\AudbolLimiter.vst3";    Components: limiter;    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolReverb.vst3\*";     DestDir: "{app}\AudbolReverb.vst3";     Components: reverb;     Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolDelay.vst3\*";      DestDir: "{app}\AudbolDelay.vst3";      Components: delay;      Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolSaturation.vst3\*"; DestDir: "{app}\AudbolSaturation.vst3"; Components: saturation; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolChorus.vst3\*";     DestDir: "{app}\AudbolChorus.vst3";     Components: chorus;     Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolUtility.vst3\*";    DestDir: "{app}\AudbolUtility.vst3";    Components: utility;    Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolLoudness.vst3\*";   DestDir: "{app}\AudbolLoudness.vst3";   Components: loudness;   Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\AudbolScope.vst3\*";      DestDir: "{app}\AudbolScope.vst3";      Components: scope;      Flags: ignoreversion recursesubdirs createallsubdirs
; Factory presets, where VST3 hosts' preset browsers look (all users)
Source: "{#PresetDir}\Audbol EQ\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol EQ"; Components: eq; Flags: ignoreversion
Source: "{#PresetDir}\Audbol Compressor\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol Compressor"; Components: compressor; Flags: ignoreversion
Source: "{#PresetDir}\Audbol De-Esser\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol De-Esser"; Components: deesser; Flags: ignoreversion
Source: "{#PresetDir}\Audbol Gate\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol Gate"; Components: gate; Flags: ignoreversion
Source: "{#PresetDir}\Audbol Transient Shaper\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol Transient Shaper"; Components: transient; Flags: ignoreversion
Source: "{#PresetDir}\Audbol Clipper\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol Clipper"; Components: clipper; Flags: ignoreversion
Source: "{#PresetDir}\Audbol Limiter\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol Limiter"; Components: limiter; Flags: ignoreversion
Source: "{#PresetDir}\Audbol Reverb\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol Reverb"; Components: reverb; Flags: ignoreversion
Source: "{#PresetDir}\Audbol Delay\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol Delay"; Components: delay; Flags: ignoreversion
Source: "{#PresetDir}\Audbol Saturation\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol Saturation"; Components: saturation; Flags: ignoreversion
Source: "{#PresetDir}\Audbol Chorus\*.vstpreset"; DestDir: "{commonappdata}\VST3 Presets\Audbol Audio\Audbol Chorus"; Components: chorus; Flags: ignoreversion


; Inno Setup script for the Acoustic Simulation Engine
; Builds a single shareable Setup.exe that installs the app + measurement data.

#define MyAppName "Atomik Acoustic Simulation Engine"
#define MyAppVersion "1.1"
#define MyAppPublisher "Atomik"
#define MyAppExeName "AtomikAcousticSimulationEngine.exe"
#define MyAppIcon "D:\shayam gui\Assets\atomik_icon.ico"

[Setup]
AppId={{B7E1C2A4-9D3F-4A18-9C2E-7F5A6B8C9D01}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=yes
DisableDirPage=no
OutputDir=D:\shayam gui\Installer\Output
OutputBaseFilename=AtomikAcousticSimulationEngine-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
PrivilegesRequired=lowest
UninstallDisplayName={#MyAppName}
; Atomik branding for the installer wizard + Add/Remove Programs entry.
SetupIconFile={#MyAppIcon}
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
; Main executable (built with static CRT -> no runtime redistributable needed)
Source: "D:\shayam gui\Builds\Release\TwoSpeakerExplorer.exe"; DestDir: "{app}"; DestName: "{#MyAppExeName}"; Flags: ignoreversion
; Measurement data — real readings only (no CLIO PNG dependency).
;   Ground Plane -> Factory Readings (open-field .xlsx)
;   Room         -> shyamGuildMeasurements (room .xlsx)
Source: "D:\shayam gui\Factory Readings\Factory_1m\Factory_1m\*.xlsx"; DestDir: "{app}\Factory Readings\Factory_1m\Factory_1m"; Flags: ignoreversion
Source: "D:\shayam gui\shyamGuildMeasurements\*.xlsx"; DestDir: "{app}\shyamGuildMeasurements"; Excludes: "*_2Horizantal.xlsx"; Flags: ignoreversion skipifsourcedoesntexist
; Brand fonts (Montserrat + Space Mono) loaded at runtime from this folder.
Source: "D:\shayam gui\Assets\Fonts\*.ttf"; DestDir: "{app}\Assets\Fonts"; Flags: ignoreversion
Source: "D:\shayam gui\Installer\README.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme
; Atomik icon (also used for Start Menu / desktop shortcuts).
Source: "D:\shayam gui\Assets\atomik_icon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\atomik_icon.ico"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\atomik_icon.ico"; Tasks: desktopicon

; Start every install from a clean slate: remove any leftover user settings
; (theme/units/recent projects) before installing, so no stale recent projects
; survive an uninstall + reinstall.
[InstallDelete]
Type: filesandordirs; Name: "{userappdata}\Atomik"

; Remove the per-user settings folder when uninstalling.
[UninstallDelete]
Type: filesandordirs; Name: "{userappdata}\Atomik"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

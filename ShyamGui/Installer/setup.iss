; Inno Setup script for Atomik Simulation Engine
; Single Setup.exe — app + brand fonts. Q21S measurement CSVs are embedded in the
; EXE (no Excel / MeasurementIntegrationPack shipped).

#define MyAppName "Atomik Simulation Engine"
#define MyAppVersion "1.3.7"
#define MyAppPublisher "Atomik"
#define MyAppExeName "Atomik Simulation Engine.exe"
#define RepoRoot "D:\WORKING_LATESTSHYAM_GUI"
#define ShyamGui RepoRoot + "\ShyamGui"
#define AssetsRoot "D:\shayam gui\Assets"

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
OutputDir={#ShyamGui}\Installer\Output
OutputBaseFilename=AtomikSimulationEngine-Setup-v{#MyAppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
PrivilegesRequired=lowest
UninstallDisplayName={#MyAppName}
SetupIconFile={#AssetsRoot}\atomik_icon.ico
UninstallDisplayIcon={app}\{#MyAppExeName}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Additional icons:"

[Files]
; Main executable — Q21S CSVs are baked in (EmbeddedQ21SData); no sidecar Excel/CSV.
Source: "{#ShyamGui}\Builds\Release\{#MyAppExeName}"; DestDir: "{app}"; DestName: "{#MyAppExeName}"; Flags: ignoreversion
; Brand fonts (Montserrat + Space Mono) loaded at runtime from this folder.
Source: "{#AssetsRoot}\Fonts\*.ttf"; DestDir: "{app}\Assets\Fonts"; Flags: ignoreversion
Source: "{#ShyamGui}\Installer\README.txt"; DestDir: "{app}"; Flags: ignoreversion isreadme
Source: "{#AssetsRoot}\atomik_icon.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\atomik_icon.ico"
Name: "{group}\Uninstall {#MyAppName}"; Filename: "{uninstallexe}"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\atomik_icon.ico"; Tasks: desktopicon

[InstallDelete]
Type: filesandordirs; Name: "{userappdata}\Atomik"

[UninstallDelete]
Type: filesandordirs; Name: "{userappdata}\Atomik"

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Launch {#MyAppName}"; Flags: nowait postinstall skipifsilent

# Start — Atomik Simulation Engine

How to build and run the Windows app from this repo.

## Quick run (already built)

Double-click or from a terminal:

```bat
ShyamGui\Builds\Release\Atomik Simulation Engine.exe
```

Debug build:

```bat
ShyamGui\Builds\Debug\Atomik Simulation Engine.exe
```

Or use the helper (builds Release if missing, then launches):

```bat
ShyamGui\Run-Release.bat
```

## Build (Visual Studio / MSBuild)

Requirements: Visual Studio 2022+ with **Desktop development with C++** (MSBuild + MSVC x64).

From the repo root (PowerShell or cmd):

```bat
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" ShyamGui\TwoSpeakerExplorer.vcxproj /p:Configuration=Release /p:Platform=x64 /v:minimal
```

Use `Configuration=Debug` for a debug build. Output EXE:

- Release → `ShyamGui\Builds\Release\Atomik Simulation Engine.exe`
- Debug → `ShyamGui\Builds\Debug\Atomik Simulation Engine.exe`

You can also open `ShyamGui\TwoSpeakerExplorer.sln` in Visual Studio and press **F5** / **Ctrl+F5**.

## Installer (optional)

1. Build **Release** as above.
2. Compile `ShyamGui\Installer\setup.iss` with [Inno Setup](https://jrsoftware.org/isinfo.php).
3. Setup lands in `ShyamGui\Installer\Output\AtomikSimulationEngine-Setup-v*.exe`.

Q21S measurement data is **embedded** in the EXE — no Excel or `MeasurementIntegrationPack` folder is required next to the app.

## Notes

- Brand fonts load from `Assets\Fonts` when installed via Setup; local Debug/Release runs use the paths configured in the project.
- Mic / plot tools: see `MIC.md`.

# Atomik Polar (Phase 1)

CLIO-style measured polar plots from your Excel readings. Built with JUCE.

## Real data library

All measurement folders are exported into `Data/`:

| Set | Source folder | Role |
|---|---|---|
| **ShyamGuild** (reference) | `shyamGuildMeasurements` | Primary curves for the app |
| Factory | `Factory Readings` | LF factory set |
| 3inch | `3 inch frequency sweep` | HF small-driver set |
| XN18 | `GyltReadings_3july/curves` | CLIO PNG session (1 deg) |

Original Excel files are mirrored under `Data/source/<set>/`.
Curves are plotted from the CSV **exactly** (same numbers as Excel).

## What Phase 1 does

- Loads all polar CSVs from the unified `Data/` pack
- Draws **CLIO-style** 2D directivity: 0 deg at top, clockwise, dB rings, frequency overlay colours
- Defaults to **ShyamGuild @ 0.5 m**
- Normalize: on-axis (CLIO), peak, or absolute (peak-shifted)
- Toggle frequencies to overlay like CLIO

## Build (Windows)

Requirements: Visual Studio 2022/2026 with C++ desktop workload (you already have VS 18 Community).

```bat
cd "d:\prediction software\PolarPlotter"
build.bat
```

Or manually:

```bat
cmake -S . -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

EXE path:

```
build\AtomikPolar_artefacts\Release\Atomik Polar.exe
```

The `Data\` folder is copied next to the EXE automatically.

## Refresh readings from Excel

If you update the `.xlsx` files:

```bat
python tools\export_csv.py
```

Then rebuild (or just copy `Data\` next to the EXE).

## Phase 2 (later)

Heat maps, multi-device interference, measured-directivity engine — same UI direction as Atomik Acoustic Simulation Engine.

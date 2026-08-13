# Cursor prompt — integrate real polar measurements into existing Atomik project

Copy everything below the line into Cursor chat **in your other existing project**, and attach or copy the `MeasurementIntegrationPack` folder into that project root (or point Cursor at it).

---

## Goal

Integrate the real loudspeaker polar measurement library into this existing project (Atomik Acoustic Simulation Engine / prediction software) so that:

1. **Measured directivity** comes from real Excel/CSV readings (not only the piston model).
2. **ShyamGuild is the reference set** — curves must match those files exactly.
3. Polar / directivity views use the **same CLIO conventions** as the measurement campaign.
4. Heat maps and multi-device interference can use measured directivity when the user enables “Use measured directivity”.
5. Do **not** invent or smooth data in a way that changes values at measured angles. Plot and interpolate **through** measured points only.

Work in this existing codebase. Match its architecture, naming, and UI style. Prefer minimal, focused changes. Do not rebuild the whole app from scratch.

## Pack location

All files live in:

`MeasurementIntegrationPack/`

| Path | Purpose |
|---|---|
| `Data/manifest.csv` | Index of every sweep |
| `Data/<Set>_<freq>Hz_<dist>m.csv` | Angle vs dBSPL (ready to load) |
| `Data/source/<Set>/*.xlsx` | Original Excel (byte-identical source) |
| `specs/DATA_SCHEMA.md` | File format and conventions |
| `specs/QUALITY_AND_DEFAULTS.md` | Which files to trust / discard |
| `specs/metrics_summary.json` | Precomputed stats per sweep |
| `CURSOR_PROMPT.md` | This prompt |

If the pack is not yet in the project, copy the whole `MeasurementIntegrationPack` folder into the project root, or set a configurable data path to it.

## Measurement sets (42 sweeps)

| Set ID | Origin folder | Distances | Frequencies | Angle step | Role |
|---|---|---|---|---|---|
| **ShyamGuild** | `shyamGuildMeasurements` | 0.5, 1.0, 2.0 m | 30, 80, 200, 500 Hz | 10° (36 pts) | **REFERENCE — use for measured directivity** |
| Factory | `Factory Readings` | 1.0 m | 30, 60, 100, 150, 200 Hz | 10° | Cross-check LF (skip Factory 30 Hz) |
| 3inch | `3 inch frequency sweep` | 1.0 m | 300–16000 Hz | 10° | Small driver HF only — do not mix into LF model |
| XN18 | `GyltReadings_3july/curves` | 1.0 m | 30–200 Hz step 10 | **1°** (360 pts) | CLIO PNG session; smoother plots |

**Critical:** These sets are **not** interchangeable. Same frequency label ≠ same curve. ShyamGuild vs XN18 at 30 Hz has shape correlation ~0.2. Always key data by `(set, freq_hz, distance_m)`.

## CLIO polar conventions (must match)

- Plane: **Horizontal** only in this pack.
- Angle: **0° = on-axis**, increasing **clockwise**, **0° at top** of polar plot.
- Labels: `0, 30, 60, 90, 120, 150, 180, -150, -120, -90, -60, -30`.
- Radial axis: dB, outer ring **+6**, then **0, -6, -12, -18, -24**, center ~**-30**.
- Normalize each frequency curve to **on-axis (0°)** for CLIO-style overlay: `rel_dB(θ) = SPL(θ) - SPL(0°)`.
- Optional alternate modes: normalize to peak, or show absolute SPL.
- Frequency colours (CLIO-like): 30=red, 80=grey, 200=green, 500=gold.
- Drop duplicate **360°** row if present; use **0°** only (already done in CSVs).

## Data format

Each CSV:

```text
degree,dBSPL
0.0,105.310
10.0,104.880
...
```

`manifest.csv` columns:

```text
set,freq_hz,distance_m,file,source_xlsx,n_points
ShyamGuild,200,0.5,ShyamGuild_200Hz_0p5m.csv,Frequency_200_0.5Horizantal.xlsx,36
```

Distance tags in filenames use `p` for decimal: `0p5` = 0.5 m, `1p0` = 1.0 m, `2p0` = 2.0 m.

Loader requirements:

1. Read `manifest.csv`.
2. Load each listed CSV into a structure: `{ set, freqHz, distanceM, points[{degree, splDb}] }`.
3. Sort points by degree.
4. Provide lookup: `find(set, freqHz, distanceM)`.
5. Provide `relativeToOnAxis()` and optional linear/cubic **interpolation in angle** for field calculations (period 360°), without changing stored measured samples.

## Defaults for the app

When “Use measured directivity” is on:

| Setting | Default |
|---|---|
| Measurement set | **ShyamGuild** |
| Distance | **0.5 m** (best free-field-ish; existing analysis scripts already prefer 0.5 and 1 m) |
| Frequencies for LF model | **80, 200, 500 Hz** (30 Hz is room-mode contaminated in ShyamGuild) |
| Normalize | On-axis (0°) |
| Ignore for free-field DI | All **2.0 m** ShyamGuild files (room gain; distance law broken) |

### Quality flags (do not use for model fitting)

- **ShyamGuild 30 Hz** at all distances — peak off-axis, high roughness, room modes.
- **ShyamGuild * @ 2.0 m** — only −1 to −3 dB loss vs 1 m (expected −6 dB); reverberant.
- **Factory 30 Hz** — corrupt (0° vs 360° differs by ~9.7 dB, roughness 6.5).
- **3inch** — different device; only for HF product path, not XN18/ShyamGuild LF.

### Good reference curves (trust these)

- ShyamGuild **80 / 200 / 500 Hz** at **0.5 m** and **1.0 m**
- Factory **60 / 100 / 150 / 200 Hz** at **1.0 m** (shape at 200 Hz matches ShyamGuild, corr ~0.93, ~7 dB louder)
- XN18 **1°** sweeps for smooth CLIO-like display comparison only

## Integration tasks (do in order)

### 1. Add measurement library module

Create a small module (name to match project style), e.g. `MeasurementLibrary` / `PolarDataStore`, that:

- Loads `MeasurementIntegrationPack/Data` (or a path from settings).
- Exposes set list, distances, frequencies, and sweep lookup.
- Computes on-axis-normalized linear directivity `D(θ) = 10^(rel_dB/20)` with `D(0)=1`.

### 2. Wire “Use measured directivity”

Where the app currently uses piston + rear taper:

- If measured directivity is enabled **and** a sweep exists for current `(set, freq, distance)`, use interpolated measured `D(θ)`.
- Else fall back to existing piston model.
- Default set = ShyamGuild, default distance = 0.5 m.

Angle convention in field code must match measurements: **0° = on-axis (+x or facing direction)**. Confirm existing heat-map code uses the same convention (prior scripts used `phi = atan2(y-sy, x-sx)` with sources facing +x).

### 3. Measured Polar view

Add or fix the **MEASURED POLAR** view mode:

- CLIO-style polar (0° top, clockwise, dB rings).
- Overlay enabled frequencies for the selected set/distance.
- Colours as above.
- Status text shows source xlsx name from manifest (traceability).

### 4. Directivity view

Show measured vs model (optional but valuable):

- Measured curve from ShyamGuild.
- Existing piston+taper model at same frequency (`a ≈ 0.13 m`, `c = 343 m/s` was used in prior verification).

### 5. Do not mix devices

Keep product/device ID separate:

- LF / XN18 path → ShyamGuild (and Factory as optional alternate).
- 3inch path → 3inch set only.
- XN18 set → display / CLIO compare, not blended into ShyamGuild tables.

### 6. Tests / sanity checks

After integration, verify:

1. ShyamGuild 200 Hz @ 0.5 m on-axis SPL = **105.310** dB (absolute).
2. On-axis-normalized value at 0° is always **0 dB**.
3. ShyamGuild 200 Hz @ 0.5 m at 90° relative ≈ **−7.76 dB**.
4. Loading manifest yields **42** sweeps.
5. With measured directivity on, single-device heat map at 200 Hz is not identical to pure omni.

## Physics notes (for field engine)

- Horizontal-only data → 2D directivity in the horizontal plane.
- Directivity index from a single horizontal cut is a **2D approximation**, not full 3D DI.
- At 30 Hz, λ ≈ 11.4 m — room modes dominate; do not treat 30 Hz polars as true speaker DI.
- Free-field distance doubling ≈ −6.02 dB. ShyamGuild 0.5→1 m is close at 500 Hz; 1→2 m is not free-field.

## UI copy suggestions

- Measurement set dropdown: `ShyamGuild (reference)`, `Factory`, `3inch`, `XN18 / Gylt`
- Checkbox: `Use measured directivity`
- Distance: `0.5 m`, `1.0 m`, `2.0 m` (disable or warn on 2.0 m for model use)
- Footer: `Real data: Frequency_200_0.5Horizantal.xlsx`

## Out of scope (unless already present)

- Do not require building a new JUCE app from scratch.
- Do not delete the piston model; keep it as fallback.
- Do not auto-smooth measured points for the model table (interpolation between angles is OK).

## Success criteria

- [ ] App loads all 42 sweeps from the pack.
- [ ] Default measured path = ShyamGuild @ 0.5 m.
- [ ] Measured polar for 80/200/500 Hz matches the CSV curves point-for-point at 10° steps.
- [ ] “Use measured directivity” affects heat map / interference.
- [ ] Factory 30 Hz and ShyamGuild 30 Hz are not used as default model inputs.
- [ ] 3inch data cannot silently replace ShyamGuild in the LF path.

## Reference implementation (optional)

A Phase-1 JUCE polar viewer that already loads this pack lives at:

`d:\prediction software\PolarPlotter\`

Source files to mirror logic from (not to copy blindly — adapt to this project):

- `Source/MeasurementLibrary.h` / `.cpp` — CSV + manifest loader
- `Source/PolarPlotComponent.cpp` — CLIO-style polar drawing
- `Source/MainComponent.cpp` — set / distance / frequency UI
- `tools/export_csv.py` — how Excel was converted (re-run if readings update)

Prior analysis scripts in `shyamGuildMeasurements/`:

- `analyze_directivity.py`, `clio_style.py`, `verify_model.py`, `polar_vs_distance.py`

---

End of prompt. Implement in the existing project; ask only if the data path or device/product mapping is ambiguous.

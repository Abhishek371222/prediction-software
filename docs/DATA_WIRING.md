# Data wiring tracker

Living record of **which datasets and maths files the app / pipeline use**. Update whenever measured CSVs, source xlsx, or maths scripts change.

**Last updated:** 2026-09-12
**Code loader:** `ShyamGui/Source/MeasurementData.h` → `MainComponent::loadMeasurements()`
**Product cabinet name:** **Q21S** (UI labels formerly XN18). A second, fully
isolated measurement source, **15W750**, was added 2026-09-12 (backend data
pipeline + runtime source switch only — cabinet dimensions/dialog branding
stay Q21S; see §1.4).

---

## How the app wires data today

```text
UI: "Measurement set" combo — Q21S (Ground Plane) or 15W750
        │
        ▼
MeasurementData::packDataFolder()
  → …/MeasurementIntegrationPack/Data/   (manifest.csv must exist, shared by both sets)
        │  prefer CSV named <Set>_<Hz>Hz_<dist>m.csv  (Set = "Q21S" or "15W750")
        ▼
buildCurve → on-axis-normalised R = 10^((SPL−SPL₀)/20)
        │
        ▼
buildDirectivityTables() → AcousticEngine (SPL heatmap + array directivity)
RadiationPatternComponent MeasuredPolar view
```

Q21S and 15W750 are two completely separate devices: each keeps its own
frequency catalogue (`AcousticEngine::kQ21SFrequencies` /
`k15W750Frequencies`), its own CSV file prefix, its own `.q21f` field files,
and its own embedded-C++ pack (`EmbeddedQ21SData` / `Embedded15W750Data`).
Selecting a measurement source repoints the active catalogue
(`AcousticEngine::setActiveFrequencyCatalogue`) and rebuilds the Frequency
dropdown from that catalogue only — the two never merge or share a table, so
switching devices can't leak one model's frequencies/readings into the other.

| Role | Active set ID | Display name | Distances | Frequencies |
|------|---------------|--------------|-----------|-------------|
| Product default | `Q21S` | Ground Plane | 0.5 / 1.0 / 2.0 m | Native `BEM_Data_10m/<Hz>Hz.xlsx` only |
| Trusted for model | Q21S | — | 0.5–2.0 m | 20, 29, 52, 81, 98, 153, 198, 256, 309, 352, 400, 401 |
| Secondary device | `15W750` | 15W750 | 0.5 / 1.0 / 2.0 m | Native `BEM_Data_15W750_10m/<Hz>Hz.xlsx` only |
| Trusted for model | 15W750 | — | 0.5–2.0 m | 64, 135, 243, 507, 1057, 1904, 3971, 8280, 17200 |
| Legacy / unused in UI | `Factory`, `ShyamGuild`, `XN18`, `3inch` | — | — | Still on disk in pack |

Engine / UI heatmap colours: Atomik `ColourMaps::sevenColor` (not matplotlib jet).

```text
rel_dB(θ) = SPL(θ) − SPL(on-axis ≈ 0°)
D_linear(θ) = 10^(rel_dB / 20)     # on-axis = 1
```

---

## 1. Measured — currently wired (Q21S BEM polars)

Primary runtime source:  
`ShyamGui/prediction software/MeasurementIntegrationPack/Data/`

### 1.1 Active in app (Q21S)

Derived from maths BEM workbook `Q21S_PolarPlot_Data_10M.xlsx` via  
`docs/q21s_bem_plots/export_q21s_pack_csvs.py` (mirror complete → constant-radius arc → catalogue Hz).

| Pattern | Freqs | Dist | n | Trust |
|---------|-------|------|---|-------|
| `Q21S_<Hz>Hz_0p5m.csv` | native bands | 0.5 m | 360 | Yes |
| `Q21S_<Hz>Hz_1p0m.csv` | native bands | 1.0 m | 360 | Yes |
| `Q21S_<Hz>Hz_2p0m.csv` | native bands | 2.0 m | 360 | Far-field prefer for SPL sim |

Catalogue Hz list = `kQ21SFrequencies` in `AcousticEngine.h` (**xlsx only, extras hidden**):
**20, 29, 52, 81, 98, 153, 198, 256, 309, 352, 400, 401**

| Origin | Detail |
|--------|--------|
| Source field | BEM complex pressure (Akabak-style export) |
| Single sub | Exact far-field polar from native xlsx → 100×100 m sim (1/r + measured D(θ)) |
| Multi sub (2, 3, …) | Same unit pattern on each cabinet; coherent array sum (gain/delay/polarity/placement) |
| Polar | Arc at 0.5 / 1.0 / 2.0 m; 0° = +Z forward |
| Level | `dBSPL = 20·log10(\|p\| / 20 µPa)` |
| Off-grid Hz | None — UI shows native xlsx bands only |

CSV schema: `degree,dBSPL` (0…359 step 1°).

Regenerate native bands:

```bash
python3 docs/q21s_bem_plots/export_q21s_native_hz_pack.py          # all native BEM_Data_10m bands
python3 docs/q21s_bem_plots/export_q21s_native_hz_pack.py 153 198  # subset
```

Bulk catalogue (big workbook, interpolated):

```bash
python3 docs/q21s_bem_plots/export_q21s_pack_csvs.py
```
### 1.2 Legacy pack sets (not selected by UI)

Still under `MeasurementIntegrationPack/Data/` for reference:  
`Factory_*`, `ShyamGuild_*`, `XN18_*`, `3inch_*`.

### 1.4 Active in app (15W750) — isolated second device

Raw BEM per-Hz workbooks live in `BEM_Data_15W750_10m/<Hz>Hz.xlsx` (same
column layout as `BEM_Data_10m`: X1/X3 = X/Z, Pressure real/imag), exported via
`docs/w750_bem_plots/export_15w750_native_hz_pack.py` (mirror of the Q21S
native-band script, retargeted). Writes into the **same**
`MeasurementIntegrationPack/Data/` folder as Q21S, but every filename is
prefixed `15W750_` instead of `Q21S_`, so the two sets can never collide.

| Pattern | Freqs | Dist | n | Trust |
|---------|-------|------|---|-------|
| `15W750_<Hz>Hz_0p5m.csv` | native bands | 0.5 m | 360 | Yes |
| `15W750_<Hz>Hz_1p0m.csv` | native bands | 1.0 m | 360 | Yes |
| `15W750_<Hz>Hz_2p0m.csv` | native bands | 2.0 m | 360 | Far-field prefer for SPL sim |

Catalogue Hz list = `k15W750Frequencies` in `AcousticEngine.h` — completely
separate array from `kQ21SFrequencies`, never merged:
**64, 135, 243, 507, 1057, 1904, 3971, 8280, 17200**

CSV schema: `degree,dBSPL` (0…359 step 1°) — identical schema to Q21S, kept
in the same folder, distinguished only by the `15W750_` filename prefix and
the `15W750` row value in `manifest.csv`'s `set` column.

Regenerate:

```bash
python docs/w750_bem_plots/export_15w750_native_hz_pack.py          # all 9 native bands
python docs/w750_bem_plots/export_15w750_native_hz_pack.py 64 135   # subset
```

Bake into the EXE after regenerating (mirrors `embed_q21s_pack.py`):

```bash
python ShyamGui/Tools/embed_15w750_pack.py
```

Runtime switch: `ControlPanel`'s "Measurement set" combo (`Ground Plane` /
`15W750`) → `MeasurementData::Source` `OpenField(0)` / `W750(2)` →
`AcousticEngine::setActiveFrequencyCatalogue()` repoints the active
frequency catalogue and `ControlPanel::setMeasurementSource()` rebuilds the
Frequency dropdown from it, before `MainComponent::loadMeasurements()`
reloads an entirely fresh, isolated `MeasuredSet` for the newly-selected
device. Cabinet dimensions, dialog titles, and product branding are
unchanged (still Q21S) — only the acoustic dataset driving the Frequency
list / directivity tables / SPL heatmap switches.

### 1.3 Change log (measured)

| Date | Change | Files / set |
|------|--------|-------------|
| 2026-09-12 | **Added 15W750 as a second, fully isolated measurement source** (own CSVs/.q21f/embedded pack/frequency catalogue; runtime switch via "Measurement set" combo) | `15W750_*Hz_*.csv`, `Embedded15W750Data.*`, `AcousticEngine.h`, `MeasurementData.h`, `ControlPanel.*`, `MainComponent.cpp` |
| 2026-08-11 | **Rewired product measured set to Q21S BEM polars** for full catalogue | `Q21S_*Hz_*.csv` |
| 2026-08-11 | UI cabinet / labels **XN18 → Q21S** | `Source/*` |
| *(prior)* | Ground Plane / Factory 5-band set | `Factory_*` |

---

## 2. Maths

### 2.1 Wired into product behaviour

| Item | Location | Role |
|------|----------|------|
| Q21S BEM → polar CSV pipeline | `docs/q21s_bem_plots/export_q21s_pack_csvs.py` | Feeds measured pack |
| 15W750 BEM → polar CSV pipeline | `docs/w750_bem_plots/export_15w750_native_hz_pack.py` | Feeds measured pack (isolated 15W750 set) |
| Preview heatmaps / polars (jet) | `docs/q21s_bem_plots/generate_q21s_plots.py` | Offline check only |
| Baffled piston fallback | `AcousticEngine.cpp` | When measured directivity off |
| Array field compute | `AcousticEngine::compute` | SPL heatmap (Atomik colours) |

### 2.2 Scripts / workbooks

| File | Role |
|------|------|
| `Q21S_PolarPlot_Data_10M.xlsx` | **Canonical Q21S BEM field** (~182k rows) |
| `docs/q21s_bem_plots/export_q21s_pack_csvs.py` | Pack CSV exporter |
| `docs/q21s_bem_plots/generate_q21s_plots.py` | Preview PNGs |
| `BEM_Data_15W750_10m/<Hz>Hz.xlsx` | Raw 15W750 BEM field, 9 native bands |
| `docs/w750_bem_plots/export_15w750_native_hz_pack.py` | 15W750 pack CSV + `.q21f` exporter |
| `ShyamGui/Tools/embed_15w750_pack.py` | Bakes 15W750 CSVs into `Embedded15W750Data.*` |
| `two_speaker_radiation.m` / `shyamGuildMeasurements/*.py` | Legacy maths helpers |

### 2.3 BEM symmetry rule

Always **mirror opposite side → complete** before arcs / maps. See prior analysis in git history / plot README.

### 2.4 Change log (maths)

| Date | Change | Files |
|------|--------|-------|
| 2026-08-13 | UI frequencies = **xlsx only** (hid 25 / 40 / 63 / 125 / 500) | `AcousticEngine.h` |
| 2026-08-13 | Remaining native BEM bands **20 / 29 / 81 / 98 / 256 / 309 / 352 / 400 / 401** wired like 52 Hz (single-sub exact; multi-sub predicted) | `export_q21s_native_hz_pack.py`, `AcousticEngine.h`, pack CSVs |
| 2026-08-12 | Native BEM bands **153 / 198** (replace 160 / 200); single-sub = exact xlsx polars; multi-sub = coherent array prediction | `export_q21s_native_hz_pack.py`, `AcousticEngine.h`, pack CSVs |
| 2026-08-12 | **52 Hz SPL** = full-world simulation from measured polar CSVs (not absolute Q21F stamp island) | `AcousticEngine.cpp`, `Q21S_52Hz_*.csv` |
| 2026-08-11 | Q21S BEM analysis + mirror rule | `Q21S_PolarPlot_Data_10M.xlsx` |
| 2026-08-11 | Exported catalogue polars into MeasurementIntegrationPack | `Q21S_*Hz_*.csv` |
| 2026-08-11 | App loader points OpenField → `Q21S` set | `MeasurementData.h` |

---

## 3. Quick “what drives Measured + heatmap now?”

| Question | Answer |
|----------|--------|
| Product name on cabinets | **Q21S-1**, **Q21S-2**, … |
| Measurement set | **Q21S** |
| Files | `Q21S_{20…200,500}Hz_{0p5\|1p0}m.csv` |
| Default distance preference | **1.0 m** (OpenField) |
| Measured polar view | Q21S BEM-derived polar curves from all native `BEM_Data_10m/<Hz>Hz.xlsx` bands |
| SPL heatmap | Full **100×100 m** sim from **far-field** Q21S polar (≥1 m, prefer **2 m**) + 1/r; **N cabinets** = coherent sum of the same unit pattern |
| Factory / XN18 pack CSVs | On disk only — **not** loaded |

---

## 4. How to update this tracker

1. Add a row under **§1.3** (measured) or **§2.4** (maths) with date + files.
2. If the loader path changes, update the wiring diagram and §3.
3. Keep raw BEM xlsx under **Maths**; pack polar CSVs under **Measured**.

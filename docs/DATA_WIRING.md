# Data wiring tracker

Living record of **which datasets and maths files the app / pipeline use**. Update whenever measured CSVs, source xlsx, or maths scripts change.

**Last updated:** 2026-08-11  
**Code loader:** `ShyamGui/Source/MeasurementData.h` → `MainComponent::loadMeasurements()`  
**Product cabinet name:** **Q21S** (UI labels formerly XN18)

---

## How the app wires data today

```text
UI: Q21S measured set only
        │
        ▼
MeasurementData::packDataFolder()
  → …/MeasurementIntegrationPack/Data/   (manifest.csv must exist)
        │  prefer CSV named Q21S_<Hz>Hz_<dist>m.csv
        ▼
buildCurve → on-axis-normalised R = 10^((SPL−SPL₀)/20)
        │
        ▼
buildDirectivityTables() → AcousticEngine (SPL heatmap + array directivity)
RadiationPatternComponent MeasuredPolar view (Q21S polars)
```

| Role | Active set ID | Display name | Distances | Frequencies |
|------|---------------|--------------|-----------|-------------|
| Product default | `Q21S` | Q21S | 0.5 / 1.0 / 2.0 m | Native `BEM_Data_10m/<Hz>Hz.xlsx` only |
| Trusted for model | Q21S | — | 0.5–2.0 m | 20, 29, 52, 81, 98, 153, 198, 256, 309, 352, 400, 401 |
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

Catalogue Hz list = `kSupportedFrequencies` in `AcousticEngine.h` (**xlsx only, extras hidden**):  
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

### 1.3 Change log (measured)

| Date | Change | Files / set |
|------|--------|-------------|
| 2026-08-11 | **Rewired product measured set to Q21S BEM polars** for full catalogue | `Q21S_*Hz_*.csv` |
| 2026-08-11 | UI cabinet / labels **XN18 → Q21S** | `Source/*` |
| *(prior)* | Ground Plane / Factory 5-band set | `Factory_*` |

---

## 2. Maths

### 2.1 Wired into product behaviour

| Item | Location | Role |
|------|----------|------|
| Q21S BEM → polar CSV pipeline | `docs/q21s_bem_plots/export_q21s_pack_csvs.py` | Feeds measured pack |
| Preview heatmaps / polars (jet) | `docs/q21s_bem_plots/generate_q21s_plots.py` | Offline check only |
| Baffled piston fallback | `AcousticEngine.cpp` | When measured directivity off |
| Array field compute | `AcousticEngine::compute` | SPL heatmap (Atomik colours) |

### 2.2 Scripts / workbooks

| File | Role |
|------|------|
| `Q21S_PolarPlot_Data_10M.xlsx` | **Canonical Q21S BEM field** (~182k rows) |
| `docs/q21s_bem_plots/export_q21s_pack_csvs.py` | Pack CSV exporter |
| `docs/q21s_bem_plots/generate_q21s_plots.py` | Preview PNGs |
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

# BEM_Data_10m — deep analysis

**Folder:** `Prediction Software/BEM_Data_10m/`  
**Analyzed:** 2026-08-12  
**Rule:** every file, every sheet, every data row counted.

---

## Inventory (15 files)

| File | Size | Role |
|------|------|------|
| `Q21S_10m_PolarPlotData.xlsx` | **114 MB** | Full polar/field dump — **48 frequencies × 40 961 mics** |
| `20Hz.xlsx` | 2.9 MB | Single-band heatmap field |
| `29Hz.xlsx` | 2.4 MB | Single-band heatmap field |
| `52Hz.xlsx` | 2.4 MB | Single-band heatmap field |
| `81Hz.xlsx` | 2.4 MB | Single-band heatmap field |
| `98Hz.xlsx` | 2.4 MB | Single-band heatmap field |
| `153Hz.xlsx` | 2.5 MB | Single-band heatmap field |
| `198Hz.xlsx` | 2.5 MB | Single-band heatmap field |
| `256Hz.xlsx` | 2.4 MB | Single-band heatmap field |
| `309Hz.xlsx` | 2.4 MB | Single-band heatmap field |
| `352Hz.xlsx` | 2.5 MB | Single-band heatmap field |
| `400Hz.xlsx` | 2.5 MB | Single-band heatmap field |
| `401Hz.xlsx` | 2.5 MB | **Exact duplicate of `400Hz.xlsx`** (see below) |
| `Heatmap.m` | 548 B | MATLAB plot script (reads `400Hz.xlsx`) |
| `Heatmap.asv` | 326 B | Autosave draft of same script (reads `20Hz.xlsx`, incomplete grid setup) |

**Total data rows (unique physics in polar file):** **1 966 128**  
**Per-Hz copies (12 × 40 961):** 491 532 (subset / convenience extracts)  
**Scripts:** 0 data rows

---

## Common schema (all xlsx data sheets)

No header row inside the numeric sheets (header is documented in `Explanation` on the polar workbook). Columns match `Heatmap.m`:

| Col | Name (Explanation) | Unit | Meaning |
|-----|--------------------|------|---------|
| 1 | Index / Param | — | Sequential mic / sample index |
| 2 | X1 | m | X |
| 3 | X2 | m | Y (**always 0** — vertical mid-plane) |
| 4 | X3 | m | Z |
| 5 | Freq | Hz | Frequency |
| 6 | Pressure real | Pa | Re{p} |
| 7 | Pressure Imaginary | Pa | Im{p} |

Derived (same as `Heatmap.m`):

```text
|p| = hypot(Pr, Pi)
SPL = 20·log10(|p| / 20e-6)     # dB SPL re 20 µPa
```

---

## 1. `Q21S_10m_PolarPlotData.xlsx` (canonical full set)

| Sheet | Data rows | Notes |
|-------|-----------|--------|
| `Data_1` | **1 048 575** | Excel max-row split (params 1 … 1 048 575) |
| `Data_2` | **917 553** | Continuation (params 1 048 576 … 1 966 128) |
| `Explanation` | 8 text rows | Field dictionary (see schema above) |
| **Total** | **1 966 128** | **= 48 × 40 961 exactly** |

### Frequencies (48, log-spaced 20 → 400 Hz)

```text
20.00000000, 21.31628337, 22.71919683, 24.21444188, 25.80809524,
27.50663357, 29.31695978, 31.24643112, 33.30288900, 35.49469094,
37.83074451, 40.32054351, 42.97420655, 45.80251822, 48.81697287,
52.02982135, 55.45412078, 59.10378763, 62.99365426, 67.13952924,
71.55826153, 76.26780901, 81.28731144, 86.63716825, 92.33912144,
98.41634393, 104.89353377, 111.79701448, 119.15484202, 126.99691887,
135.35511549, 144.26339986, 153.75797557, 163.87742888, 174.66288559,
186.15817817, 198.41002387, 211.46821461, 225.38581932, 240.21939960,
256.02923964, 272.87959115, 290.83893454, 309.98025718, 330.38135005,
352.12512389, 375.29994612, 400.00000000
```

Ratio step ≈ **1.0658** (same family as older `Q21S_PolarPlot_Data_10M.xlsx`).

### Geometry (true 10 m span)

| Axis | Range | Unique samples | Step |
|------|-------|----------------|------|
| X (X1) | **−5 … +5 m** | 287 | ≈ 0.034965 m |
| Y (X2) | **0 only** | 1 | — |
| Z (X3) | **−5 … +5 m** | 287 | ≈ 0.034965 m |

- Full rectangle would be 287×287 = **82 369** cells.  
- Present cells per frequency: **40 961** (checkerboard **even** indices only — every other cell).  
- **Front and back are both present** (Z negative and positive). No front↔back mirror is required for this dataset.

`Data_1` holds mostly Z ≤ ~0.38 m; `Data_2` holds Z ≥ ~0.35 m — seam from Excel row split, not a physical cut.

### SPL (all rows in polar file)

| | dB re 20 µPa |
|--|--|
| Data_1 | min 35.55 · max 123.20 · mean 81.07 |
| Data_2 | min 44.54 · max 125.39 · mean 86.65 |

---

## 2. Per-frequency heatmap workbooks (12 files)

Each file: **1 sheet (`Sheet1`) · exactly 40 961 rows · 1 frequency · same XYZ grid**.

| Filename | Actual BEM Hz | Rows | SPL min…max (dB re 20 µPa) |
|----------|---------------|------|----------------------------|
| `20Hz.xlsx` | 20.0000000000 | 40961 | 46.10 … 103.26 |
| `29Hz.xlsx` | 29.3169597850 | 40961 | 63.71 … 110.13 |
| `52Hz.xlsx` | 52.0298213505 | 40961 | 88.25 … 123.31 |
| `81Hz.xlsx` | 81.2873114392 | 40961 | 88.87 … 116.14 |
| `98Hz.xlsx` | 98.4163439331 | 40961 | 82.36 … 117.17 |
| `153Hz.xlsx` | 153.7579755677 | 40961 | 78.16 … 106.65 |
| `198Hz.xlsx` | 198.4100238729 | 40961 | 67.30 … 116.85 |
| `256Hz.xlsx` | 256.0292396403 | 40961 | 43.07 … 112.17 |
| `309Hz.xlsx` | 309.9802571794 | 40961 | 50.65 … 112.67 |
| `352Hz.xlsx` | 352.1251238861 | 40961 | 40.87 … 104.03 |
| `400Hz.xlsx` | 400.0000000000 | 40961 | 53.55 … 119.56 |
| `401Hz.xlsx` | **400.0000000000** | 40961 | 53.55 … 119.56 |

### Integrity checks

| Check | Result |
|-------|--------|
| All 12 share identical XYZ lattice | **Yes** (±5 m, 287×287 checkerboard) |
| Each is exact subset of polar @ that Hz | **Yes** (`81Hz` vs polar: 40961/40961 match, max \|Δp\| = 0) |
| `401Hz.xlsx` vs `400Hz.xlsx` | **Byte-identical physics** — same freq 400 Hz, max \|Δp\| = 0 |

**Do not treat `401Hz.xlsx` as a separate band** — it is a duplicate export of 400 Hz.

### Native front/back (example @ 81 Hz, R ≈ 1 m)

No artificial rear mirror — sampled from file:

| θ (0° = +Z forward) | SPL dB |
|---------------------|--------|
| 0° | 112.98 |
| 90° / 270° | ~104.3 |
| 180° (behind) | **101.16** (~12 dB down vs on-axis) |

L/R already symmetric in the export.

---

## 3. MATLAB scripts

### `Heatmap.m` (complete)

- Reads `400Hz.xlsx`
- Columns: X=col2, Z=col4, Pr=col6, Pi=col7
- Builds SPL re 20 µPa, `griddata` → `pcolor`, jet colormap
- **This is the intended heatmap recipe for the per-Hz files**

### `Heatmap.asv` (incomplete autosave)

- Same math but reads `20Hz.xlsx`
- References `Xm,Zm,SPLm` without building `meshgrid` / `griddata` (will error if run as-is)
- Keep only as draft; use `Heatmap.m` as reference

---

## 4. How this differs from old `Q21S_PolarPlot_Data_10M.xlsx`

| | Old (repo root) | **New `BEM_Data_10m`** |
|--|--|--|
| Size | ~9 MB | Polar alone **114 MB** |
| Span | ~ X ±1.5 m, Z 0…3 m | **X ±5 m, Z ±5 m (true 10 m)** |
| Rear (Z&lt;0) | Not in export | **Included** |
| Freqs | 48 (20–400) | **Same 48** |
| Points / freq | ~3785 | **40 961** |
| Per-Hz splits | No | **12 convenience files** |

---

## 5. Row accounting (nothing missed)

```text
Q21S_10m_PolarPlotData.xlsx
  Data_1:     1,048,575 rows
  Data_2:       917,553 rows
  ─────────────────────────
  Total:      1,966,128 rows = 48 × 40,961

Per-Hz extracts (also counted, subset of polar):
  12 × 40,961 = 491,532 rows
  (401Hz is duplicate of 400Hz → unique bands = 11 + duplicate)

Scripts: Heatmap.m, Heatmap.asv → 0 pressure rows
```

Every numeric row in every sheet was scanned for schema, bounds, uniqueness, and SPL.

---

## 6. Implications for app wiring

1. **Prefer this folder** over the old ~3 m export for Q21S BEM.  
2. **Heatmaps:** use per-Hz files (or polar slice) on ±5 m plane — Atomik colours in-app.  
3. **Directivity polars:** sample arcs from this field; **L↔R mirror only if a half is missing** — rear already exists, do **not** copy front→back.  
4. **Skip `401Hz.xlsx`** as a distinct frequency.  
5. Map UI catalogue Hz → nearest of the **48** BEM frequencies (same as before).

---

## 7. File hashes (integrity)

| File | sha256[:16] |
|------|-------------|
| `20Hz.xlsx` | e44d06bba545f45c |
| `29Hz.xlsx` | 1ad02630d7a77f1e |
| `52Hz.xlsx` | 0708fee5737a9c43 |
| `81Hz.xlsx` | 0ba1633a3404dfd0 |
| `98Hz.xlsx` | d415c2082d7b5953 |
| `153Hz.xlsx` | 7e1bc1f539f67147 |
| `198Hz.xlsx` | a5ec78fa68d5d045 |
| `256Hz.xlsx` | d7986ed66240d098 |
| `309Hz.xlsx` | 21feaf396f6d1488 |
| `352Hz.xlsx` | d4fb60ad062216c8 |
| `400Hz.xlsx` | 0a071c6edfb8f016 |
| `401Hz.xlsx` | 6386dc82aa340190 |
| `Q21S_10m_PolarPlotData.xlsx` | 6beab90d3152ff32 |

# Atomik — Export Reference

What the app can export, where each action lives, and what the output contains.

---

## Where to find export

| Entry | Location | Actions |
|-------|----------|---------|
| **Export** (header) | Top-right **Export** button | PNG, SPL CSV, Directivity CSV, SVG |
| **⋯ More** menu | Header overflow (⋯) | PDF report, Save project, Preferences |

A simulation result must exist for PNG / SPL CSV / SVG / PDF (status: “Nothing to export” / “Run a simulation…” otherwise).  
**Export Directivity** needs loaded measurement data (Ground Plane / Maths / Room).

---

## 1. Save Image (PNG)

**Menu:** Export → Save Image (PNG)  
**Code:** `MainComponent::exportPNG()` → `ReportExport::renderHeatmapSheet()`  
**File:** `*.png`

**What it does**
- Captures the current plot view at **2×** resolution, then builds a **client-ready sheet**.
- Wraps the plot in Atomik branding: logo header, project metadata block, plot, footer.

**Contents of the sheet**
- Header: Atomik logo + “ACOUSTIC SIMULATION ENGINE” + title (e.g. SPL Coverage Heatmap)
- Project details from the open project
- Embedded plot (heatmap / polar / whatever is currently shown)
- Footer with software / data notes

**Requires:** last simulation result (`hasResult_`).

---

## 2. Export SPL (CSV)

**Menu:** Export → Export SPL (CSV)  
**Code:** `MainComponent::exportCSV()`  
**File:** `*.csv`

**What it does**
- Writes the **relative SPL grid** from the last compute (same field as the heatmap), not absolute dB SPL.

**Format**
```text
# Atomik Acoustic Simulation Engine - relative SPL (dB)
# peak=0 dB; negative = quieter; not absolute SPL
# frequency_Hz,…,rows,…,cols,…,worldW_m,…,worldH_m,…,dbFloor_display_only,…
# matrix: row0 = world Y=0 (bottom); col0 = world X=0 (left)
<row0 col0>,<row0 col1>,…
…
```

**Notes**
- Values are **relative**: loudest ≈ **0 dB**, quieter is negative.
- UI **dB floor** is display-only; CSV is **not** clamped to that floor.
- Warns if the grid has no ~0 dB peak (often means speakers not enabled).

---

## 3. Export Directivity (CSV)

**Menu:** Export → Export Directivity  
**Sub-items:**
- **All frequencies…** — pick a folder; one CSV per loaded measured frequency at the current measurement distance  
- **\<N\> Hz** — single file for that band  

**Code:** `exportDirectivityAllFrequencies()` / `exportDirectivityOneFrequency()` → `MeasurementData::exportCurveSheet()` / `writeAtomikDirectivitySheet()`  
**File:** `Atomik_Directivity_<Hz>Hz_<dist>.csv` (e.g. `Atomik_Directivity_50Hz_1p0m.csv`)

**What it does**
- Exports the **in-app measured polar curve** (Ground Plane / Maths / Room set currently loaded), branded as Atomik Prediction Software.
- Does **not** copy third-party VACS source files; writes Atomik sheets from our curve data.

**Format (VACS-style header + CSV data block)**
```text
SourceDesc=Atomik_Data_Text
Version='beta_v1.3.0'
Author='Atomik Prediction Software'

StartString_Data=Data
EndString_Data=Data_End


Data_Format=LeveldB_Phase
Data_Domain=Other
Data_LevelType=SoundPressure
Data_Phase_AngularFormat=degree
Data_AbscUnit=deg
Data_BaseUnit=Pa
Data_Legend='Curve at: 50 Hz'

Data
Angle (deg),Level (dB),Phase (deg)
0.0,112.250000,0.0
…
Data_End
```

| Column | Meaning |
|--------|---------|
| Angle (deg) | Polar angle |
| Level (dB) | Sound pressure level from the measured curve |
| Phase (deg) | Phase; written as `0.0` when not measured in-app |

**Requires:** measurement set loaded and a valid curve at the selected distance.

---

## 4. Export Figure (SVG)

**Menu:** Export → Export Figure (SVG)  
**Code:** `MainComponent::exportSVG()`  
**File:** `*.svg`

**What it does**
- Builds a fixed **700×700** SVG of the current SPL field.
- Colours the grid with the same banded / jet map as the UI (respects dB floor for colour mapping).
- Embeds the field as a **base64 PNG** inside the SVG.
- Draws enabled speakers as white circles with black stroke at world positions.

**Requires:** last simulation result with a non-empty grid.

---

## 5. Export PDF Report

**Menu:** ⋯ More → Export PDF Report  
**Code:** `exportPdfReport()` → `buildAndWriteReport()` → `ReportBuilder::build()`  
**File:** `*.pdf` (suggested name from project, e.g. `My Project - Report.pdf`)

**What it does**
- Builds a multi-page **engineering report** (AFMG-style structure):

| Section | Content |
|---------|---------|
| Cover | Logo, project name, engineer / owner / date / site |
| Project information | Metadata fields |
| Inputs | Simulation / product parameters |
| Results | Summary metrics from last run |
| Heatmaps | One page per measured frequency (+ current Hz if needed), each re-rendered |
| Summary | Closing summary |

**Requires:** simulation result. Generation is deferred so the status bar can show “Generating PDF report…”.

---

## Related (not under Export menu)

| Action | Where | What |
|--------|-------|------|
| **Save Project** | ⋯ More, or dashboard | Writes `*.atmk` project (scene, params, metadata) — not a plot/data export |
| **Import layout** | Workspace tools | Imports reference `png` / `jpg` / `dxf` — import only |

---

## Quick map (code)

| Feature | Primary files |
|---------|----------------|
| Export menu | `Source/MainComponent.cpp` (`showExportMenu`) |
| PNG sheet | `Source/ReportExport.h` |
| PDF report | `Source/ReportBuilder.h`, `Source/PdfDocument.h` |
| SPL CSV / SVG | `Source/MainComponent.cpp` |
| Directivity CSV | `Source/MeasurementData.h` (`writeAtomikDirectivitySheet`) |

---

## Preconditions summary

| Export | Needs result | Needs measurements |
|--------|--------------|--------------------|
| PNG | Yes | No |
| SPL CSV | Yes | No |
| SVG | Yes | No |
| PDF report | Yes | Optional (extra heatmap pages if loaded) |
| Directivity CSV | No | Yes |

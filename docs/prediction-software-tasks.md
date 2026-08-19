# Prediction Software – Execution Plan

## Overview

Atomik Acoustic Simulation Engine predicts low-frequency coverage for subwoofer arrays (UI cabinet label **XN18**; measured data pack remains Q21S BEM). Users place cabinets in a 2D world, simulate relative SPL coverage and far-field / measured polar directivity, and export PNG, CSV, and PDF reports. Simulation uses measured **Ground Plane** directivity, optional 1/3-octave smoothing, and a display-only dB floor for colour mapping.

## Task Categories

- UI/UX Improvements
- DSP / Audio Processing
- Export / File Handling
- PDF / Reporting
- System / Core Logic
- Research / Understanding

## Tasks

### TASK ID: PS-001
**Title:** Logo size on MacBook Pro / smaller screens (−20%)  
**Category:** UI/UX Improvements  
**Status:** ✅ Completed

### TASK ID: PS-002
**Title:** Use 1/3rd octave frequencies  
**Category:** DSP / Audio Processing  
**Status:** ✅ Completed

### TASK ID: PS-003
**Title:** Rename sub as XN18-1 (hyphen)  
**Category:** UI/UX Improvements  
**Status:** ✅ Completed

### TASK ID: PS-004
**Title:** Selected sub outline black  
**Category:** UI/UX Improvements  
**Status:** ✅ Completed

### TASK ID: PS-005
**Title:** Understand dB floor  
**Category:** Research / Understanding  
**Status:** ✅ Completed

### TASK ID: PS-006
**Title:** Improve 1/3rd octave smoothing  
**Category:** DSP / Audio Processing  
**Status:** ✅ Completed

### TASK ID: PS-007
**Title:** Measurement = ground plane (no room mention)  
**Category:** UI/UX Improvements / System / Core Logic  
**Status:** ✅ Completed

### TASK ID: PS-008
**Title:** Remove import layout feature  
**Category:** System / Core Logic  
**Status:** ✅ Completed

### TASK ID: PS-009
**Title:** Remove array presets  
**Category:** UI/UX Improvements  
**Status:** ✅ Completed

### TASK ID: PS-010
**Title:** PDF uses “Directivity Plot” instead of heatmap  
**Category:** PDF / Reporting  
**Status:** ✅ Completed

### TASK ID: PS-011
**Title:** Check date in exported PNG  
**Category:** Export / File Handling  
**Status:** ✅ Completed

### TASK ID: PS-012
**Title:** Legend −6 dB increments  
**Category:** UI/UX Improvements / DSP / Audio Processing  
**Status:** ✅ Completed

### TASK ID: PS-013
**Title:** Increase image detail font size  
**Category:** UI/UX Improvements / Export / File Handling  
**Status:** ✅ Completed

### TASK ID: PS-014
**Title:** PDF footer www.atomikaudio.com  
**Category:** PDF / Reporting  
**Status:** ✅ Completed

### TASK ID: PS-015
**Title:** Improve Atomik Engine logo distinction  
**Category:** UI/UX Improvements  
**Status:** ✅ Completed

### TASK ID: PS-016
**Title:** Ensure CSV exports are consistent  
**Category:** Export / File Handling  
**Status:** ✅ Completed

### TASK ID: PS-017
**Title:** Remove SVG export option  
**Category:** Export / File Handling  
**Status:** ✅ Completed

### TASK ID: PS-018
**Title:** Reset → Set to Default  
**Category:** UI/UX Improvements  
**Status:** ✅ Completed

### TASK ID: PS-019
**Title:** Polar plot 90° on right  
**Category:** UI/UX Improvements / DSP / Audio Processing  
**Status:** ✅ Completed

## Execution Log

### 2026-08-12 — PS-001 ✅ Completed
- Header logo **18 → 14** (−20%); dashboard **24 → 19** (−20%).
- `Scale::minFactor` **1.0 → 0.78** for smaller MacBook windows.
- Files: `UiTextConfig.h`, `BrandTheme.h`, `DashboardComponent.cpp`

### 2026-08-12 — PS-002 ✅ Completed
- IEC 1/3-octave centres: `20, 25, 31, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500`.
- Re-exported Q21S pack CSVs; removed orphan linear-grid files.
- Files: `AcousticEngine.h`, `ControlPanel.cpp`, `MeasurementData.h`, export script, pack Data/

### 2026-08-12 — PS-003…PS-019 batch ✅ Completed
- **PS-003:** User-visible labels `XN18-n` (hyphen) in combo, plot, info, PDF.
- **PS-004:** Selected sub outline → black (soft accent fill retained).
- **PS-005:** `dBfloor` = **display dynamic range only** (default −36 dB). Peak = 0 dB; values clamped at floor for colour / colourbar / CSV (`dbFloor_display_only`). UI −54…−12. Not absolute SPL / noise floor. No behavioural bug.
- **PS-006:** Smoothing M **7→11**; Hamming-weighted band power average.
- **PS-007:** Measurement UI = **Ground Plane** only; `sourceName()` / CSV `Measurement_Set=Ground_Plane`; PDF “World Width/Depth”.
- **PS-008:** Import Layout UI hidden; callbacks disconnected.
- **PS-009:** Array Presets section hidden.
- **PS-010:** PDF section titles / chrome → “Directivity Plot”.
- **PS-011:** PNG Date falls back to `ProjectMeta::today()` when empty.
- **PS-012:** Colourbar ticks on −6 dB grid; floor slider step = 6.
- **PS-013:** Export / polar detail fonts increased in `UiTextConfig` + PNG sheet.
- **PS-014:** PDF footer centre = `www.atomikaudio.com` (project left, date|page right).
- **PS-015:** Export header uses ATOMIK-only logo preference + “ATOMIK ACOUSTIC SIMULATION ENGINE” wording; larger print logo.
- **PS-016:** Shared CSV meta keys (`SourceDesc`, `Version`, `Author`, `Measurement_Set=Ground_Plane`, `Frequency_Hz`, `Level_Type`) across SPL / array / measured exports.
- **PS-017:** SVG export button remains hidden / unwired.
- **PS-018:** Reset label → **Set to Default**.
- **PS-019:** Confirmed CLIO map: 0° top, **90° right** (`sin`/`cos` in `ClioFrame::toXY`) — no change needed.

### 2026-08-19 — v1.3.0 Windows product
- Plot drawing tools + BEM/directivity field math (origin `633fc36`).
- Product name **Atomik Simulation Engine**; ATOMIK-only logo; header **v1.3.0** no longer clips.
- Ctrl+Z / Ctrl+Y undo-redo (Caps Lock safe); header button **Statistics**.
- Release folder: `Atomik Simulation Engine.exe` only.
- Changelog: `ShyamGui/CHANGELOG.md` + `ShyamGui/CHANGELOG.xlsx`.

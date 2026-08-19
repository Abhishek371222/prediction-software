# Changelog - Atomik Simulation Engine

All notable changes to this product are documented here.
This project loosely follows [Keep a Changelog](https://keepachangelog.com/) and
[Semantic Versioning](https://semver.org/).

Dates follow the work that shipped in source history and the Windows builds of
this tree (including 2026-08-19 updates in this workspace).

---

## [1.3.0] - 2026-08-19
*Development window: 2026-08-12 -> 2026-08-19.*

Windows product build of **Atomik Simulation Engine v1.3.0**. Ships Q21S / XN18
coverage prediction, Ground Plane measurements, plot drawing tools, and the
branded Release executable.

### Development timeline (what landed when)

- **2026-08-12** - Prediction-software task batch (PS-001…PS-019): smaller-screen
  logo, IEC 1/3-octave frequencies, XN18-n labels, Ground Plane-only measurement
  wording, legend −6 dB steps, CSV/PDF/PNG polish, Set to Default, polar 90° right.
- **2026-08-13** - Repository published (`prediction-software` initial commit).
- **2026-08-19** - Plot drawing tools (select/pan/pencil/eraser/ruler/line) and
  tighter BEM / directivity field math (`633fc36`).
- **2026-08-19** - Windows Release packaging and UI polish in this workspace:
  product rename, ATOMIK-only wordmark, header version no longer clips,
  Ctrl+Z / Ctrl+Y undo-redo, Statistics header button, single Release exe.

### Added

- **Plot drawing tools** on the SPL heatmap: Select, Pan, Pencil, Eraser, Ruler,
  Line, plus a colour swatch. Annotations stay locked in world metres.
- **Undo / redo** (`Ctrl+Z`, `Ctrl+Y`, `Ctrl+Shift+Z`). Shortcut letter is
  lowercased so Caps Lock does not block it. Covers drawings, cabinet moves,
  add/delete/layout/presets, and control-panel edits. Text fields keep their own undo.
- **Windows Release exe** `Builds\Release\Atomik Simulation Engine.exe` (static
  CRT, signed locally). Installer script and README updated to v1.3.0.

### Changed

- **Product name** `Atomik Acoustic Simulation Engine` -> **Atomik Simulation Engine**.
  Window title, header, PDF/CSV branding, installer, and Windows file version
  resource all show **v1.3.0**.
- **Wordmark** is **ATOMIK** only (AUDIO line stripped). Header uses cropped
  `Atomik_Logo_Dark.png` / `Atomik_Logo_Light.png`.
- Header **Stats** pill renamed **Statistics**, width sized to the full label.
- Measurement UI remains **Ground Plane** only (no Room set in the live UI).
- Colourbar / dB floor use **−6 dB** steps; `dBfloor` is display range only.
- Frequency list is IEC **1/3-octave** centres.

### Fixed

- Header version no longer ellipsizes to `v1...`; `v1.3.0` stays fully visible
  across the window scale range.
- Header title + version layout uses the real logo width instead of a leftover
  200 px reserve.

---

## [1.1.0] - 2026-06-29
*Development window: 2026-06-25 -> 2026-06-29.*

A large feature + UX release. Adds full theming, a project workflow with a
dashboard, professional heatmap/PDF exports, workspace/grid and CAD-layout
tooling, speaker array presets, dual measurement datasets, responsive/scalable
window behaviour, and a branded Windows installer. All existing v1.0 prediction
functionality is preserved.

### Development timeline (what landed when)

- **2026-06-25** - Theming foundation: central theme-aware palette + custom
  `AtomikLookAndFeel` (`BrandTheme.h`).
- **2026-06-26** - The bulk of the release:
  - Preferences + persistent settings (`AppSettings.h`, `PreferencesComponent`).
  - Project workflow: single-file `.atmk` projects + dashboard
    (`ProjectData.h`, `DashboardComponent`).
  - Enhanced heatmap export sheet (`ReportExport.h`).
  - PDF report pipeline (`PdfDocument.h`, `AcousticAnalysis.h`, `GraphRender.h`,
    `ReportBuilder.h`).
  - Workspace grid + CAD/DXF layout import (`LayoutLayer.h`, `DxfImport.h`).
  - Speaker array presets (Cardioid / End-fired).
  - App icon embedding (`AppIcon.rc`) + Inno Setup installer (`setup.iss`).
  - Header responsiveness, version bump to v1.1, and the red -> cyan colour
    overhaul.
- **2026-06-27** - Window scaling: minimum window size, fixed widescreen aspect
  ratio, defensive layout clamps; "fresh install" guarantee and Recent Projects
  hygiene (hide/prune non-existent projects).
- **2026-06-29** - Dual measurement datasets ("Ground Plane" / "Room"),
  selectable and kept separate; final naming.

### Added

#### Theming & Preferences
- **Dark / Light themes** with dynamic, app-wide switching via a central,
  theme-aware palette (`Brand::Palette`) and a custom `AtomikLookAndFeel`.
  Every screen, graph, table, dialog, heatmap chrome and control adapts live.
- **Preferences / Settings tab** to choose theme and **unit system (SI / Imperial)**.
  Displayed values convert dynamically while all internal calculations stay in SI.
- **Persistent user preferences** (theme, units, grid visibility, measurement set,
  recent projects) stored with `juce::PropertiesFile` and broadcast app-wide via
  `ChangeBroadcaster` / `ChangeListener`.

#### Project Workflow & Metadata
- **Project Dashboard** shown at launch (separate window): create a new project,
  open an existing one, or pick from **Recent Projects**. Opening/creating a
  project then opens the main simulation window.
- **Project metadata capture** (Project Name, Engineer, Owner, Address, City,
  Country, Email, Mobile, Date) used for storage, exports and reports.
- **Single-file `.atmk` projects** (self-contained JSON): metadata + acoustic
  scene (speakers) + simulation settings, with save/open.

#### Heatmap Export (enhanced)
- Heatmap image export upgraded to a **professional, client-ready sheet**
  (without touching heatmap generation logic): header (company logo, software
  name, report title), project details block, and heatmap data (frequency,
  scale, units, legend, heatmap).

#### PDF Report Export
- **Multi-page PDF report** (AFMG SoundFlow-style) generated programmatically:
  Cover Page, Project Information, Input Parameters, Simulation Results, Graphs
  (all result graphs), Heatmaps (e.g. 30/50/80/100 Hz), and Summary.
- Custom minimal **PDF 1.4 writer** (`PdfDocument`) - vector text + embedded
  images (no external dependency).
- **Acoustic analysis** module (`AcousticAnalysis`): RT60 (Sabine), absorption,
  transmission loss (mass-law), and frequency response sampled from the engine.
- **Chart rendering** (`GraphRender`): line/bar charts rendered to images for the
  report.

#### Workspace, Grid & Layout (CAD / DXF)
- **Improved grid**: major/minor lines, zoom-adaptive spacing (`niceStep`),
  higher contrast/legibility, with a **show/hide toggle** (persisted).
- **Layout reference import**: bring in floor plans as background references -
  **images (PNG/JPG)** and **DXF** drawings.
- Dependency-free **DXF parser** (`DxfImport`) for 2D entities (LINE, LWPOLYLINE,
  POLYLINE/VERTEX, CIRCLE, ARC).
- Layout **alignment & visibility tools**: move, rotate, scale (width in metres),
  opacity, show/hide, lock, and snap-to-grid; live drag in the workspace.

#### Speaker Array Presets
- **Cardioid** array presets for **2 / 3 / 4** subwoofers.
- **End-fired** array presets for **2 / 3 / 4** subwoofers, with auto-calculated
  spacing, delay, polarity and orientation. Parameters remain manually editable
  after placement.

#### Measurement Datasets (same subwoofer, two environments)
- **Two selectable measurement sets**, kept fully separate (never merged),
  chosen from a "Measurement set" dropdown in the control panel:
  - **Ground Plane** - factory open-field readings (30/60/100/150/200 Hz, 1 m).
  - **Room** - GYLT readings (30/80/200/500 Hz, 0.5/1 m).
- Switching the set reloads only that dataset and rebuilds directivity tables, so
  **heatmaps and polar plots visibly differ** between environments. Each polar is
  normalised to its own on-axis level. Selection is persisted across launches.
- The measured-polar "available frequencies" and legend are now generated
  dynamically from the loaded dataset.

#### Responsive UI & Window Scaling
- **Minimum window size** enforced (main window 1120x686; dashboard 680x537) so
  the layout can never collapse or hide controls.
- **Fixed widescreen aspect ratio** (1340:820) preserved during resize - no
  horizontal/vertical stretching distortion.
- **Defensive layout clamps**: the heatmap workspace stays visible (Priority 1),
  side panels stay usable (Priority 2), and controls keep usable sizes (Priority 3).
- **Scrollable left control panel** (viewport) so dense controls scroll instead of
  shrinking.
- **Responsive header**: app title auto-shrinks to fit and never clips/overlaps;
  the three header actions (Export PDF Report, Save Project, Preferences) collapse
  into a single **"more options" (kebab) menu** at all window sizes.

#### Branding & Installer
- **Atomik application icon** embedded in the executable (Windows `.rc`) and used
  for the installer wizard, Add/Remove Programs entry, and Start Menu / Desktop
  shortcuts. Icon generated from the Atomik wordmark via `make_icon.py`.
- **Windows installer** (Inno Setup, `setup.iss`): single Setup.exe that installs
  the app + both measurement datasets + brand fonts + icon. Built with the
  **static C runtime** (no VC++ redistributable required), per-user install
  (no admin), with shortcuts and an uninstaller.

### Changed
- **Versioning** bumped `1.0.0 -> 1.1.0` (app title shows **v1.1**; installer 1.1).
- **Color system overhaul**: removed all red from the UI.
  - Header title red -> near-white for visibility.
  - Action/active accents red -> neutral grey, then a full **cyan accent** system.
  - Defined a text hierarchy: headings/values `#F5F7FA`, labels `#B8C0CC`,
    muted `#7E8794`; backgrounds `#0D0D12` / panels `#17171D`; accent `#44D9E6`;
    success `#53D769`; warning `#FF6B6B`. Cyan-highlighted controls use dark
    ("on-accent") text for contrast. The PDF frequency-response line changed from
    red to steel-blue.
- **Measured-directivity normalisation** is now data-driven (each curve normalised
  to its own on-axis level) instead of hard-coded reference SPLs.
- Measurement-set options were renamed for clarity: **Open Field -> Ground Plane**
  and **GYLT -> Room** (underlying datasets unchanged).

### Fixed
- Header action buttons no longer overlap the title in narrow/short windows.
- App title no longer clips (e.g. showing "v1." instead of "v1.1").
- **Recent Projects hygiene**: entries whose `.atmk` file no longer exists are
  hidden and pruned automatically, so dead/non-existent projects never appear.
- **Fresh install guarantee**: the installer clears the per-user settings folder
  on install and uninstall, so a reinstall never carries over stale recent
  projects or settings.

### Notes
- Because the installer resets the per-user settings folder on each install,
  reinstalling/upgrading resets theme and unit preferences to defaults (Dark / SI).

### New source modules in this release
`BrandTheme.h`, `AppSettings.h`, `PreferencesComponent.{h,cpp}`, `ProjectData.h`,
`DashboardComponent.{h,cpp}`, `ReportExport.h`, `PdfDocument.h`,
`AcousticAnalysis.h`, `GraphRender.h`, `ReportBuilder.h`, `LayoutLayer.h`,
`DxfImport.h`, `AppIcon.rc`, plus `Installer/setup.iss` and
`Installer/make_icon.py`.

---

## [1.0.0] - 2026-06-22
*Initial baseline. Measured-directivity loading added 2026-06-24.*

The original Acoustic Prediction / Simulation software for dual-subwoofer (XN18)
placement and directivity visualisation in a 30 x 30 m world.

### Core features
- **Acoustic prediction & visualisation** with multiple view modes:
  - **SPL Heatmap**, **Pressure Map**, **Interference**, **Directivity**, and
    **Measured Polar** plots, plus a **Fit View** control.
- **Acoustic metrics**: SPL, Frequency Response, RT60 (reverberation),
  Absorption, Transmission Loss, and acoustic heatmaps.
- **Frequency selector** with 14 steps: 20, 30, 40, 50, 60, 70, 80, 90, 100, 110,
  120, 150, 200, 500 Hz.
- **Measured horizontal directivity** (added 2026-06-24) loaded from `.xlsx`
  readings (`MeasurementData`) and applied to each source when the simulation
  frequency matches a measured frequency; with **1/3-octave smoothing** and
  optional **3 dB contour bands**.
- **Speaker/scene controls**: per-unit X/Y position, gain (dB), delay (ms),
  invert polarity, reverse orientation, enable/disable; add/delete units; and
  **quick device layouts** (1 / 2 / 3 devices, same plane).
- **Simulation settings**: grid resolution and dB floor.
- **Exports**: PNG image, CSV data, and SVG figure.
- **Engine**: monopole summation with frequency-dependent piston directivity
  fallback and fractional-octave band power averaging for realistic SPL maps.
- **Branding & UI**: JUCE desktop application using Montserrat + Space Mono fonts
  and the Atomik visual identity.

### Source modules (baseline)
`Main.cpp`, `MainComponent.{h,cpp}`, `ControlPanel.{h,cpp}`, `InfoPanel.{h,cpp}`,
`RadiationPatternComponent.{h,cpp}`, `AcousticEngine.{h,cpp}`, `ColourMaps.h`,
and `MeasurementData.h` (added 2026-06-24).

# Changelog - Atomik Simulation Engine

All notable changes to this product are documented here.
This project loosely follows [Keep a Changelog](https://keepachangelog.com/) and
[Semantic Versioning](https://semver.org/).

## Version lineage (v1.0.0 → current)

| Version | Date | Role |
|---------|------|------|
| **1.0.0** | 2026-06-22 | Initial Acoustic Prediction baseline (30×30 m, XN18, measured directivity from 2026-06-24) |
| **1.1.0** | 2026-06-29 | Theming, `.atmk` projects, PDF/heatmap export, CAD/DXF, array presets, dual datasets, installer |
| **1.2.x** (beta) | ~2026-07 | UI redesign to Graph colors.pdf / Atomik chrome (signal-red accents, heatmap colours, packaging as `betav1.2.3`) — see §1.2.0 below |
| **1.3.0** | 2026-08-21 | Q21S BEM polar×1/r on 100×100 m, plot tools, undo/redo, version archive, Windows portable ZIP |
| **1.3.5** | 2026-08-26 | Mic receivers + FR window, Show Degrees, embedded Q21S (no Excel on target), Windows Setup |
| **1.3.6** | 2026-08-31 | Mic ring snap restored; shape-edge Snap tak; start.md |

Dates follow the work that shipped in source history and the Windows/mac builds of
this tree (including 2026-08-21 Q21S physics, Windows data-path / portable pack, and version-archive updates).

---

## [1.3.6] - 2026-08-31

**Atomik Simulation Engine v1.3.6** — mic ring snap back to the original cursor-follow
pattern, reliable **tak** when drawings snap edge-to-edge with Snap on, and a repo
`start.md` for build/run.

### Fixed
- **Mic drag / ring snap**: restored first `Drag::Mic` behaviour (follow cursor,
  1/2/4/8 m ring latch + tak). No longer stuck after latching; not driven by
  object/grid SelectionMove snap.
- **Snap tak on shapes**: plays when two drawings newly share an edge (e.g. two
  rectangles meet). Ignores speaker/mic/centre near-misses that caused random clicks.

### Added
- Root **`start.md`** — how to build and run the Windows app.

### Changed
- Version strings / file version resource → **v1.3.6**.

---

## [1.3.5] - 2026-08-26

**Atomik Simulation Engine v1.3.5** — virtual mic probes on the heatmap, a floating
frequency-response window, optional degree labels vs Q21S facing, and a Windows
Setup that ships **without** Excel / MeasurementIntegrationPack (Q21S CSVs are
embedded in the EXE).

### Added
- **Mic tool** (toolbar): Add Mic, Place on ring, Show Degrees.
- Virtual receivers with ring snap (1 / 2 / 4 / 8 m), in-memory **tak** snap sound,
  live relative dB labels, floating Frequency Response window.
- **Show Degrees**: angle vs Q21S facing (0° = front, CCW) before the dB on mic labels.
- **Embedded Q21S CSVs** in the Release EXE — heatmaps/polars work on another PC
  without shipping Excel or a sidecar Data folder.
- **Windows Setup** `AtomikSimulationEngine-Setup-v1.3.5.exe` (EXE + fonts only).

### Changed
- Installer no longer copies Factory / shyamGuild `.xlsx` measurement folders.
- Version strings / file version resource → **v1.3.5**.

---

## [1.3.0] - 2026-08-21
*Development window: 2026-08-12 → 2026-08-21 (Windows portable pack through 2026-08-21 evening).*

**Atomik Simulation Engine v1.3.0** — Q21S coverage prediction on a **100×100 m**
world using BEM polars × spherical spreading, Ground Plane measurements, plot
drawing tools (including shapes), branded Windows/mac builds, a standalone
version archive, and a **Windows portable ZIP** so SPL heatmaps work off-machine.

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
- **2026-08-20…21** - Q21S product physics: coherent BEM polar × 1/r over the
  full grid (no ±5 m stamp), absolute SPL, MATLAB-style cardioid preset, true
  cabinet footprint (750×784×917 mm), single-sub heatmap docs/figures, portable
  pack zip, and MongoDB-backed version archive (source + DMG / Windows downloads).
- **2026-08-21** - Plot **shapes** (rectangle / square / circle) with colour +
  fill opacity; Caps Lock–safe undo retained (`9db9c91`).
- **2026-08-21** - **Windows heatmap data path**: prefer exe/repo
  `MeasurementIntegrationPack` (Q21S CSVs) over legacy `D:\shayam gui\…`;
  archive ships **ZIP = EXE + Data** so other PCs get correct SPL (`windows` branch).

### Added

- **Plot drawing tools** on the SPL heatmap: Select, Pan, Pencil, Eraser, Ruler,
  Line, plus a colour swatch. Annotations stay locked in world metres.
- **Plot shapes**: Rectangle, Square, and Circle tools with fill **opacity**
  slider; undo/redo covers shape edits.
- **Undo / redo** (`Ctrl+Z`, `Ctrl+Y`, `Ctrl+Shift+Z`). Shortcut letter is
  lowercased so Caps Lock does not block it. Covers drawings, cabinet moves,
  add/delete/layout/presets, and control-panel edits. Text fields keep their own undo.
- **Windows Release exe** `Builds\Release\Atomik Simulation Engine.exe` (static
  CRT, signed locally). Installer script and README updated to v1.3.0.
- **Windows portable ZIP** (`Atomik-Windows-v1.3.0*.zip` / `…-portable.zip`):
  EXE **plus** `MeasurementIntegrationPack\Data` (Q21S CSVs). Required when
  sending builds to other machines — the bare EXE alone cannot load heatmaps.
- **Q21S BEM SPL engine**: measured far-field (≈2 m) directivity D(θ) with
  coherent complex superposition and inverse-square spreading (`r_spread` floored
  at cabinet half-depth). Absolute dB SPL calibrated from on-axis BEM level.
- **MATLAB-style cardioid preset** (rear 180°, polarity −1, −6 dB, 3.5 ms,
  ~0.01 m spacing) with coherent sum in the engine.
- **Single-sub heatmap documentation** (`docs/SINGLE_SUB_HEATMAP.md`) plus
  native-Hz figures and generator script; portable
  `dist_packages/Q21S_SingleSub_SPL_Heatmap.zip`.
- **Version archive** (`version-archive/`): standalone HTML + MongoDB GridFS
  listing releases from **v1.3.0** with Download source zip / macOS DMG /
  **Windows ZIP (EXE + data)** (not wired into the JUCE app). Local serve can
  stream from `artifacts/` without MongoDB.

### Changed

- **Product name** `Atomik Acoustic Simulation Engine` -> **Atomik Simulation Engine**.
  Window title, header, PDF/CSV branding, installer, and Windows file version
  resource all show **v1.3.0**.
- **Wordmark** is **ATOMIK** only (AUDIO line stripped). Header uses cropped
  `Atomik_Logo_Dark.png` / `Atomik_Logo_Light.png`.
- Header **Stats** pill renamed **Statistics**, width sized to the full label.
- Measurement UI remains **Ground Plane** only (no Room set in the live UI);
  product cabinet / data set is **Q21S** (UI labels formerly XN18).
- Colourbar / dB floor use **−6 dB** steps; `dBfloor` is display range only.
- Frequency catalogue = **native BEM bands** where Q21S CSVs exist:
  20, 29, 52, 81, 98, 153, 198, 256, 309, 352, 400, 401 Hz (IEC 1/3-octave
  centres elsewhere in the pipeline where applicable).
- **Cabinet geometry** uses Q21S W **750** / H **784** / D **917** mm for plan
  drawing, hit-test, and 1/r singularity floor (depth/2).
- Heatmap / array prediction uses the **2 m** BEM arc (not 0.5 m near-field).
- Simulation **world** is **100×100 m** (was 30×30 m in v1.0).
- Version-archive Windows artifact is a **ZIP with data**, not a lone EXE.

### Fixed

- Header version no longer ellipsizes to `v1...`; `v1.3.0` stays fully visible
  across the window scale range.
- Header title + version layout uses the real logo width instead of a leftover
  200 px reserve.
- SPL field no longer stamps a ±5 m BEM island; full-world map follows polar × 1/r.
- **Windows** no longer prefers an old `D:\shayam gui\…` MeasurementIntegrationPack
  (Factory/ShyamGuild-only) over the repo Q21S pack — that caused broken SPL
  heatmaps vs macOS when the legacy folder existed.
- Packaged / archive EXEs resolve `MeasurementIntegrationPack\Data` (and optional
  `Resources\…`) beside the executable so portable layouts work.

---

## [1.2.0] - 2026-07 (beta series through betav1.2.3)
*UI redesign between v1.1.0 and the v1.3.0 product cut. Acoustic engine behaviour
largely unchanged in this span; chrome, theming, and packaging moved to the
Graph colors.pdf / Atomik mockup language. Deliverable often named `betav1.2.3.exe`.*

### Added / Changed (UI redesign)

- **Brand tokens** centralised in `BrandTheme.h` from Graph colors.pdf
  (charcoal, white, ash, signal red, heatmap cool/hot stops).
- **Accent system** moved toward **Signal Red #ED2227** for interactive controls
  (later product branding in 1.3.0 settled on ATOMIK wordmark + current palette).
- **Heatmap colour map** stops aligned to brand cool/hot ranges; Rel. SPL legend
  chrome refined.
- **Light / dark chrome** for sidebar density, speaker markers, plot chrome, and
  header actions per product mockups.
- **Windows packaging** of the redesign build as a signed Release / `betav1.2.3`
  style artifact (see `Tools/make_ui_changelog_doc.py` / UI changelog docx).

### Notes

- Detailed bullet list of every chrome tweak lives in the UI redesign doc
  (`Atomik_UI_ChangeLog_GraphColors_to_betav1.2.3.docx` / generator script).
- v1.3.0 supersedes beta 1.2.x as the tagged product line for Q21S physics and
  the public `prediction-software` repo.

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

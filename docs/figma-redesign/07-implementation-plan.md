# Figma Redesign — Implementation Plan

Source spec: `00-overview.md` through `06-open-questions.md` in this folder.
Assets: `ShyamGui/Assets/FigmaRedesign/`.

## Decisions locked in with the user

1. **Toolbar scope**: restyle only. Every existing tool/button (Select, Pan, Pencil, Eraser,
   Ruler, Opacity, Ortho, Snap, Fit View, Range, Zoom) keeps working. Figma's icon look/spacing/
   grouping/labels is applied to today's actual tool set — we do not add Save-As/Export-PDF/
   multi-shape/color-palette features that exist in the mock but not the app, and we do not drop
   Opacity/Ortho/Snap/Fit View/Range that exist in the app but not the mock.
2. **Measurement set dropdown**: stays hidden (per earlier explicit request), even though the
   Figma mock shows it.

## Phases

- [x] **Phase 1 — Design tokens** (`BrandTheme.h`, `UiTextConfig.h`) — DONE
  `lightPalette()` rewritten to exact Figma values: `#FFFFFF` base/panel/panelDark, `#F6F6F6`
  btnIn/exportPill/idleViewPill/plotBg, `#0C0C0C` border/controlBorder() (now opaque, was 45%
  alpha), `#656161` plotBorder, `#B4B4B4` plotGrid, black text/heading/ash/muted/axisLabel.
  `Control::cornerRadius` and `cardCornerRadius` both 4→2px / 8→2px. `controlBorderPx` 0.6→1.0.
  Slider thumb (`#D81F1F`) and filled-track (`#313131`) colors were **already** an exact match
  pre-existing in `drawLinearSlider` — not changed. Dark theme untouched except shared structural
  constants (radius/border width), which it inherits automatically.

- [x] **Phase 2 — Left sidebar** — mostly DONE, inherited free
  Numbered chevron section headers, `#F6F6F6`/border/2px input boxes, slider anatomy, checkbox
  anatomy, and the "Set to Default"/"Clear All" bottom buttons were **already implemented**
  pre-existing in `ControlPanel.cpp`/`BrandTheme.h` — the current code already matched this part
  of the Figma spec almost exactly (see `figma_progress_1.png`); Phase 1's token fixes were the
  only changes needed to bring colors/radius fully into line. Verified by screenshot in light
  theme — matches `figma_full_screen_screenshot.png` closely.

- [x] **Phase 3 — Header / toolbar restructure** — DONE (structural part)
  The tool ribbon (`plotHeader_`) now spans the **full body width**, above both the sidebar and
  the canvas, as one continuous strip directly below the title row — matching Figma's two-row,
  full-width header shape instead of a canvas-only ribbon. Changed in `MainComponent.cpp`:
  `resized()` (new `bodyTop2` below the ribbon, ribbon bounds now `(sideX, bodyTop, W-2·pad,
  toolbarStripH)`, sidebar/canvas both start at `bodyTop2`) and `paint()`/`paintOverChildren()`
  (ribbon gets its own full-width rounded frame; canvas gets its own separate frame — previously
  one frame covered both, which would have wrongly spanned the sidebar once the ribbon went
  full-width). Verified at 1920×1080, 1280×720, and the minimum 1024×700 — no overlap/clipping
  at any size.

  **Row 1 redone to match Figma exactly** (follow-up user request): logo + AutoSave now sit
  together on the left (was: AutoSave on the far right next to Stats/Project), title stays
  centred between them and the new "Ready" pill, and a compact "Ready" pill (red dot — matches
  Figma's colour choice exactly, not the green/amber the bottom status strip uses) sits top-right
  where Stats/Project/More used to be. Added `StatusStrip::setCompact()` (`UiChrome.h`) so the
  existing status-tracking code can drive both the full bottom strip and this compact pill from
  one call (`MainComponent::reportStatus()` now wraps every former `statusStrip_.setStatus()`
  call site — 31 of them — so both stay in sync automatically). Info/Help/Settings icons moved
  from row 1 down into a reserved strip at the right edge of row 2's tool ribbon (matching
  Figma's Help cluster living in row 2, not row 1) — `plotHeader_` now stops short of the full
  width to leave room for them. Stats/Project/More are hidden (`setVisible(false)` + zero bounds)
  exactly like Terminal/View Mode — nothing removed from code (`showStatsPopup()`,
  `showProjectMenu()`, `showOverflowMenu()` are all still there, just not wired to a visible
  button right now). Verified at 1920×1080 and 1280×720.

  **Ribbon fully restructured into labeled Figma clusters** (follow-up user request, with
  reference image): `PlotHeaderBar::resized()`/`paint()` in `UiChrome.h` rewritten from a
  right-to-left flat button row into left-to-right labeled clusters — **File | Navigation | View
  | Tools | Shapes | Colours | Help**, plus one trailing **Options** cluster for app-specific
  controls Figma's mock doesn't have (Opacity, SPL probe, Ortho, Snap, Range) — each with an icon
  row, a centred label underneath, and a thin divider rule after it (`dividerX_`, drawn in
  `paint()`).
  - **File** is new: 5 `DrawableButton`s using the already-downloaded Figma PNGs
    (`figma_icon_file_*.png`) on a fixed mid-grey chip (visible on both themes' ribbon fill,
    since the source PNGs are dark-ink-only). Wired to real actions via new callback members
    (`onFileNew/onFileOpen/onFileSave/onFileSaveAs/onFileExport`) — MainComponent should hook
    these to `launchNewProjectInstance()` / `openProjectInNewWindow()` / `saveProject()` /
    `saveProjectAs()` / `exportPdfReport()` (**not yet wired from MainComponent — the buttons
    exist and are styled but currently no-op; hooking the 5 callbacks is a 5-line follow-up**).
  - **Navigation/View/Tools/Shapes/Colours** reuse all existing tool buttons verbatim, just
    regrouped left-to-right instead of right-to-left.
  - **Help** (Info/Settings/Help badge buttons) is **reparented** from MainComponent into
    `plotHeader_` via a new `setHelpIcons()` method, so it lays out as a proper cluster like
    every other group instead of living in a separate reserved strip outside the ribbon (the
    approach used one iteration earlier — superseded by this).
  - Fixed along the way: the same legacy-asset-path bug from the logo investigation (see earlier
    session) bit the new File icons too — `Brand::assetsFolder()` prefers a hardcoded
    `D:\shayam gui\Assets` path that exists on disk but didn't have a `FigmaRedesign`
    subfolder, so the icons silently failed to load. Fixed by copying
    `ShyamGui/Assets/FigmaRedesign/` there too (same fix pattern as the logo bug).
  - Also fixed: a blank/bordered strip appeared under the icons at first — turned out to be
    `promptLabel_` (the dynamic tool-hint label, e.g. "RULER: specify start point") after it was
    given an opaque background to avoid overlapping the new cluster labels; reverted to its
    original transparent styling since the two never actually need to coexist visually.
  - Verified in both themes and at 1920×1080 / 1280×720 / 1024×700 — labels and dividers render
    correctly at every size tested; "Colours" abbreviates to "Col." at the narrowest width
    (cosmetic only, cluster still has its swatch and divider).

- [x] **Follow-up correction — exact match to reference image** — DONE
  Two remaining deviations from the Figma mock, fixed per explicit instruction ("hide anything
  not in the design"):
  - **"Ready" pill moved into the ribbon row itself** (far right, same row as the tool icon
    clusters) — it was in the title row above, which doesn't match the mock (the mock's Ready
    coordinates land in the toolbar's y-band, not the title strip's). New
    `PlotHeaderBar::setReadyPill()` reparents `headerStatus_` the same way `setHelpIcons()`
    already did; `MainComponent::resized()`'s title-centering math no longer reserves space for
    it on the right.
  - **"Options" cluster (Opacity/SPL-probe/Ortho/Snap/Range) hidden** from the ribbon — the
    Figma mock has no slot for these app-specific extras, so per instruction they're hidden
    (`setVisible(false)` + zero bounds, same pattern as Terminal/View Mode/Stats/Project) rather
    than kept visible in a bolted-on extra cluster. Code/logic untouched.
  - Verified again at 1920×1080 and 1280×720 after both changes.

- [x] **Pixel-accuracy pass on the ribbon** (user: "see the spacing and alignment and placing of
  each element") — DONE. The root cause of the mismatch was that the red **"SPL Heatmap | …"
  caption was living inside the ribbon**, eating ~240px on the left and pushing every cluster
  out of position. In the Figma design that caption is on the canvas, and clusters start hard
  left. Fixes:
  - `title_` reparented out of `PlotHeaderBar` (new `getTitleLabel()` accessor) and positioned by
    `MainComponent::resized()` at the canvas's top-left. Every existing `plotHeader_.setTitle()`
    call site keeps working; the font is now set where it's positioned so it still rescales.
  - Ribbon geometry rebuilt from the documented Figma values. Each constant in
    `UiConfig::Layout` is the Figma 1920×1080 pixel value ÷ 1.317 (the scale factor the app
    derives at that window size), so at 1920×1080 they render at exactly the Figma pixels:
    row height 74, icons 24 on a 30px pitch, icons 11px below the row top, caption row 17px tall
    at 44px below the row top, 20px left margin, 12px of air either side of each divider,
    14px colour dots on a 20×16 grid, 60px right margin for Ready. Label font
    `ribbonClusterLabel = 9.0` → 12px, matching Figma.
  - Ribbon is now **full-bleed** (x=0..W, flush under the title row, closed by a hairline on its
    bottom edge) instead of an inset rounded card, and cluster rules run the **full row height**.
  - **Shapes** is now 5 separate icons (Line / Polyline / Circle / Rectangle / Text box) plus the
    fill-colour preview square, as in the mock, instead of one button opening a construction
    menu — each wired through the same `(shapeId, constructionId)` path the menu used.
  - **Colours** is now the Figma palette: 8 dots in a 4×2 grid with the exact documented hexes,
    each setting the draw colour.
  - **View**'s "Fit View" is an icon (`btnFitView_`) rather than a text button; it triggers the
    original `fitBtn_` so MainComponent's wiring is untouched.
  - Grey chips behind the File icons removed (Figma draws plain glyphs).
  - `btnMic_` hidden — the mock has no mic slot. **Note: this removes the only UI entry point
    for placing measurement mics**; code is intact, so re-showing it is a one-line change.
  - Verified at 1920×1080, 1280×720 and 1024×700.

- [x] **Dark theme hidden for this release** (separate follow-up request) — DONE
  `AppSettings::setTheme()` and its settings-load path now force `ThemeMode::Light`
  unconditionally (dark ignored even if stored in the settings file from a previous run);
  `PreferencesComponent`'s "DARK" segmented button is hidden (`LIGHT` takes its slot). All dark
  palette code/enum values are untouched — removing the two forcing lines in `AppSettings.h`
  fully re-enables dark theme.

- [x] **Flush-layout + measured-calibration pass** ("extract exact sizes, spacing, alignment") — DONE
  The app was still a set of floating rounded cards separated by 8px gaps; Figma is flush
  edge-to-edge regions separated by hairlines. Reworked, and then **verified numerically** rather
  than by eye: the app window's *client area* is sized to exactly 1920×1080 and its render is
  pixel-compared against `figma_full_screen_screenshot.png` (helper scripts kept in the session
  scratchpad).
  - `MainComponent::resized()/paint()/paintOverChildren()`: sidebar `x=0..340` running the full
    height, canvas flush beside it, bottom strip spanning the canvas column only, ribbon
    full-bleed under the title row. All gaps removed; regions separated by hairline rules.
  - Full-width status strip deleted from the layout (`statusStripHeight = 0`). `StatusStrip`
    gained a `Mode` enum — `Pill` drives the ribbon's Ready indicator, `RunInfo` draws
    "Last run : …" / "Elapsed : …" as two stacked lines in the bottom strip, per the mock.
  - Sidebar rebuilt to Figma's rhythm (36px rows, 24px headers, 12px row gap, 14px section gap,
    28.8px checkboxes, 101px value boxes). Found and fixed a compounding bug where `sectionBreak`
    added its gap *on top of* each row's trailing gap, so every section drifted further down than
    the last. "Quick Layout"/1-2-3 Devices and the whole "5. WORKSPACE" section are hidden (no
    counterpart in the mock); "Set to Default"/"Clear All" are now filled buttons pinned 36px
    above the window bottom, as in the design.
  - Ribbon clusters now use Figma's **absolute** x-positions (icon start + closing divider per
    cluster) instead of accumulating padding, which had been pushing each cluster further right —
    Help was landing 36px late. Captions centre on the plate between dividers, matching how the
    mock aligns them.
  - **Opacity slot added** between Tools and Shapes per the updated Figma: label top-left,
    percentage top-right, track beneath (`fillAlpha_` un-hidden, text box replaced by a dedicated
    right-aligned readout label).
  - Rel. SPL legend moved onto Figma's geometry (18×750 bar, 32px below the canvas top, 89px in
    from its right edge) and its labels scaled with the window.
  - **Two systemic font bugs found**: (1) fonts assigned in constructors were stuck at base size
    because `Brand::UI::scale` is still 1.0 at construction — they must be set in `resized()`;
    (2) font bases cannot be derived by dividing the design's CSS px by the scale factor, because
    JUCE's `Font` height is the whole line box — every size is now **calibrated against measured
    ink width** in the Figma render.

  Measured result at 1920×1080 (Figma → app): ribbon caption 67→66px, sidebar section header
  155→154px, sidebar field label 83→83px, "Last run" 227→225px, "Elapsed" 106→103px, Navigation
  label plate x=182..272→183..272, Save/Export buttons x=1500..1679→1500..1679.

  Still deliberately divergent: the canvas renders real heatmap data (the mock's canvas is empty
  and light), and the "Measurement set" dropdown stays hidden per decision #2 above.

- [ ] **Phase 4 — Graph canvas + Rel. SPL legend** (`RadiationPatternComponent.cpp`)
  Panel fill `#F6F6F6`, border `#656161` 0.2px, gridline colors `#B4B4B4`/`#C5C5C5`, legend
  gradient exact stops from `01-design-tokens.md`, red caption text style.

- [x] **Phase 5 — Bottom strip** (`MainComponent.cpp`) — DONE
  Bottom panel is now a single row spanning the canvas width: Save Image (PNG) / Export SPL
  (CSV) side by side at the right edge, matching Figma's geometry exactly. Per explicit user
  decision, the View Mode column (SPL/Directivity/Measured Polar tabs — only SPL Heat Map is a
  real view "for now") and the Terminal column are hidden from this layout since Figma's mock
  shows neither. Both are `setVisible(false)` + zeroed bounds only — no members, callbacks,
  timers, or docking logic were removed, so re-showing either later is a one-line flip in
  `resized()`. "Last run"/"Elapsed" run info continues to show in the full-width `statusStrip_`
  at the very bottom of the window (unchanged) rather than being duplicated into the new row.

- [ ] **Phase 6 — Verify & polish**
  Rebuild, screenshot at several window sizes (matching the earlier responsiveness pass — this
  must not regress), compare side-by-side against `figma_full_screen_screenshot.png`, fix drift.

## Explicit non-goals

- No functional/calculation changes.
- No new features beyond what's needed to visually match (per decision #1).
- Dark theme is not being redesigned from the Figma mock (it has none) — only inherits shared
  structural constants (radius, stroke widths, no shadows).

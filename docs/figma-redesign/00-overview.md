# Figma Redesign — Overview

Source: https://www.figma.com/design/vLUiytDkvtyHqkQGIaEiLk/Untitled?node-id=1-2
fileKey: `vLUiytDkvtyHqkQGIaEiLk` · root node: `1:2` ("Home Screen 1.1")

## Screens / frames in the file

The Figma file's single page ("Page 1", node `0:1`) contains **exactly one top-level frame**:

- `1:2` — "Home Screen 1.1", **1920 × 1080**, positioned at (0,0).

There are **no sibling top-level frames** — this redesign covers a single screen (the main
simulation/heatmap workspace). There is no separate "settings screen", "export screen", etc. in
this file. Everything documented here is one screen's worth of UI.

## Canvas size

1920 × 1080 (16:9, standard desktop). All geometry in the other docs is given in absolute pixels
against this canvas plus a percentage of canvas width/height, so it can be re-derived for other
window sizes.

## High-level layout

The screen is a 4-region layout, matching the current JUCE app's overall structure:

1. **Header / ribbon** — `y = 0` to `132` (full width, 1920 wide). Two stacked bars:
   - Row 1 (`y=0..58`, 58px tall): logo, "Auto Save" checkbox+label, centered document title
     ("Atomik Simulation Engine - Atomik"), right-aligned "Statistics" label and a red
     "● Ready" status pill.
   - Row 2 (`y=58..132`, 74px tall): the main toolbar — grouped icon clusters for File,
     Navigation, View, Tools, Shapes, Colours, Help, each with a text label centered below the
     icon row and a vertical divider rule between groups.

2. **Left Panel** — `x=0..340`, `y=132..1080` (340 wide × 948 tall). A numbered, collapsible
   settings sidebar: 1. FREQUENCY (Hz), 2. XN18 Units (device list), 3. SELECTED XN18
   (per-device X/Y/gain/delay sliders + toggles), 4. SIMULATION (grid resolution / dB floor /
   contour bands / measurement set). Two full-width action buttons ("Set to Default", "Clear
   All") anchored at the bottom.

3. **Graph / heatmap canvas** — `x=340..1920`, `y=132..979` (1580 wide × 847 tall). A plain,
   almost-empty light-gray grid (no heatmap data is actually rendered in this mock) with a red
   status caption in the top-left corner ("SPL Heatmap | 2 Device | 52Hz | Measured @ 2.0m") and
   a vertical color-scale legend ("Rel. SPL", 0 db to −36 db, red→blue→black gradient) docked at
   the right edge, from `x=1813` to `x=1920` (approx. inside the 1920 canvas but the legend body
   extends further; see 03-layout-graph-area.md for the discrepancy).

4. **Bottom strip** — `y=979..1080` (101 tall, spans the graph's width, `x=340..1920`). Run
   metadata text ("Last run", "Elapsed") bottom-left, two action buttons bottom-right
   ("SAVE IMAGE (PNG)", "EXPORT SPL (CSV)").

## Screenshot description (plain language)

The screen is a light, mostly white/off-white desktop tool. A thin black-outlined top bar holds
a small angular wordmark logo ("ATOMIK") at far left, an "Auto Save" checkbox next to it, the
app title centered, and a red "Ready" status dot with label at the far right. Below that is a
toolbar row of dark charcoal-gray icon buttons arranged in labeled clusters — File (new/open/
save/save-as/export), Navigation (cursor/hand), View (zoom in/zoom out/fullscreen), Tools
(pencil/eraser/ruler), Shapes (line/polyline/filled circle/rectangle/text box), Colours (8 small
colored dots in a 4×2 grid: red, blue, black/dark, white; and pink, light-blue, gray, green),
and Help (settings gear / info / help "?"), each cluster separated by a thin vertical divider
line and labeled below in gray. The left sidebar is white with four numbered, dropdown-arrow
collapsible section headers in bold, each holding light-gray (`#f6f6f6`) input boxes with thin
dark borders, red-thumbed horizontal sliders, and black-and-white checkboxes; two gray full-width
buttons ("Set to Default" and "Clear All") sit at the very bottom of the sidebar. The main canvas
area is a large, mostly blank light-gray grid — it currently shows no actual heatmap data, just
faint horizontal/vertical gridlines — with a small red caption top-left and a tall vertical
red-to-blue-to-black color scale bar with dB labels docked at the right edge. A thin gray strip
below the canvas shows the last simulation run's timestamp and elapsed time on the left, and two
gray "Save Image" / "Export SPL" buttons on the right.

## Region count summary

- 1 header/toolbar region (two stacked rows)
- 1 left sidebar (4 numbered sub-sections + 2 bottom buttons)
- 1 graph canvas (with a color-scale legend docked at its right edge)
- 1 bottom strip (run info + export buttons)
- The "4 sub-panel groups" mentioned in the original task turned out, on inspection, to be
  **grid-line decoration only** — see `03-layout-graph-area.md` and `06-open-questions.md` for
  the full explanation; they are not 4 distinct chart/heatmap views.

## Files produced

- `00-overview.md` (this file)
- `01-design-tokens.md`
- `02-layout-left-panel.md`
- `03-layout-graph-area.md`
- `04-layout-header-toolbar.md`
- `05-icons-and-assets.md`
- `06-open-questions.md`

Assets: `D:\WORKING_LATESTSHYAM_GUI\ShyamGui\Assets\FigmaRedesign\` (see `05-icons-and-assets.md`
for the full mapping).

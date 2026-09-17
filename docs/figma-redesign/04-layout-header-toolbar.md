# Header / Toolbar — Exact Geometry

A header/toolbar region **does exist** in this design (unlike the graph sub-panels, this is a
real, fully-populated UI region). It occupies the full canvas width and spans `y=0..132`
(12.22% of the 1080px canvas height), as two stacked bars. None of the toolbar icons are wrapped
in a single Figma frame — they are direct children of the root frame `1:2`, positioned by
absolute x/y — so there is no single node to pull one consolidated `get_design_context` call
from; the values below come from the page metadata (exact) cross-checked against the full-page
screenshot (exact bounding boxes, glyph shapes identified from the downloaded icon assets).

## Row 1 — Title / status bar (`y = 0..58`, 58px tall, 5.37% of canvas height)

| Element | x | y | w | h | Notes |
|---|---|---|---|---|---|
| Row background (`1:702` "Rectangle 554") | -9 | 0 | 1938 | 58 | Border: 0.5px solid **black**. Extends 9px past both left/right canvas edges (bleed) |
| Logo area background (`1:693` "Rectangle 578") | 0 | 58 | 182 | 74 | *(Note: this node's y/h place it in Row 2's band, not Row 1 — likely a background plate that logically groups with the logo above it; height 74 matches Row 2)* |
| Logo mark ("Horizontal Logo - Black 1", `1:701`) | 20 | 20 | 94 | 18 | Angular wordmark "ATOMIK•" — see `figma_logo_atomik_horizontal_black.png` |
| "Auto Save" checkbox box (`1:699` "Rectangle 576") | 143 | 19 | 20 | 20 | Unchecked square checkbox |
| "Auto Save" label (`1:698`) | 173 | 19 | 85.17 | 20.39 | |
| Document title (`1:694`) | 815 | 20 | 289 | 20 | "Atomik Simulation Engine - Atomik", centered-ish in the 1920 width (815+289/2 ≈ 959.5, canvas center = 960 — confirmed centered) |
| "Statistics" label (`1:700`) | 1414.74 | 86.375 | 77.97 | 20.39 | Sits at y=86, which is actually inside Row 2's vertical band — likely a label for a toolbar-row element, not Row 1 |
| "Ready" status group (`1:689`/`1:690`) | 1784 | 87 | 76 | 18 | Also in Row 2's band |
| Status dot (ellipse `1:692`) | 1784 | 88 | 16 | 16 | Red filled circle |
| "Ready" text (`1:691`) | 1808 | 87 | 52 | 18 | |

**Note on the Row1/Row2 split:** several nodes with `y` in the high-50s/60s/80s (Statistics,
Ready, and the toolbar icons themselves starting at `y=69`) all sit inside what visually reads
as one continuous 132px-tall header in the screenshot — the screenshot shows a single visual
divide at `y=58` (a horizontal rule) with the logo/Auto-Save/Title in the thin strip above it
and everything else (toolbar icons, Statistics, Ready) below it. Treat `y=0..58` as the "title
strip" and `y=58..132` as the "toolbar strip" per the screenshot, even though a couple of text
nodes (Statistics, Ready) have y-coordinates that land in the 74-132 toolbar band, not the 0-58
title band — this is consistent (they visually belong with the toolbar strip, right-aligned).

## Row 2 — Toolbar strip (`y = 58..132`, 74px tall, 6.85% of canvas height)

| Element | x | y | w | h | Notes |
|---|---|---|---|---|---|
| Row background (`1:703` "Rectangle 577") | -9 | 58 | 1938 | 74 | Border 0.5px, black @ 50% opacity |

Icon buttons are **24×24px**, positioned at `y=69` (i.e. vertically inset ~11px from the row's
top, 41px from the row's bottom — not vertically centered; there's more clearance below the
icon than above, presumably to leave room for the 17px-tall group-label text at ~`y=102-105`).

### File group (`x = 20..173`)

| Icon | x | Asset |
|---|---|---|
| Add New | 29 | `figma_icon_file_add_new.png` |
| Opened Folder | 59 | `figma_icon_file_open_folder.png` |
| Save | 89 | `figma_icon_file_save.png` |
| Save as | 119 | `figma_icon_file_save_as.png` |
| Export Pdf | 149 | `figma_icon_file_export_pdf.png` |

Label "File" at x=88, y=105 (17×17 approx. text box). Group divider rect ("Rectangle 579")
x=182, y=58, w=90, h=74 — this looks like it's actually the **background plate for the next
group** (Navigation) rather than a divider; treat divider lines as the thin vertical rules
visible in the screenshot between clusters (exact divider-node geometry not sampled before the
rate limit — see open questions).

### Navigation group (`x = 190..267` label; icons ~200-254)

| Icon | x | Asset |
|---|---|---|
| Cursor | 200 | `figma_icon_nav_cursor.png` |
| Hand | 230 | `figma_icon_nav_hand.png` |

Label "Navigation" at x=190, y=102, w=77, h=17.

### View group (icons ~278-338)

| Icon | x | Asset |
|---|---|---|
| Zoom In | 278 | `figma_icon_view_zoom_in.png` |
| Zoom Out | 308 | (not downloaded individually — mirror of Zoom In, magnifying glass with "−") |
| Full Screen | 338 | `figma_icon_view_full_screen.png` |

Label "View" at x=303, y=102, w=34, h=17.

### Tools group (icons ~378-438)

| Icon | x | Asset |
|---|---|---|
| Pencil | 378 | `figma_icon_tools_pencil.png` |
| Eraser | 408 | `figma_icon_tools_eraser.png` |
| Ruler | 438 | (not downloaded individually — triangular ruler icon per screenshot) |

Label "Tools" at x=402, y=102, w=36, h=17.

### Shapes group (icons ~478-638, widest cluster)

| Icon | x | Asset |
|---|---|---|
| Line | 478 | `figma_icon_shapes_line.png` |
| Polyline | 508 | (decorative zig-zag "Vector 77" glyph sits at x=538,y=72,w=8,h=18 between Polyline and Filled Circle — `figma_icon_shapes_polyline_vector.svg`) |
| Filled Circle | 552 | |
| Rectangle | 582 | |
| (small fill-preview swatch, "Rectangle 584") | 612 | 20×20 dark square at y=71 — `figma_icon_shapes_fill_preview_dark_square.png`, likely a "current fill color" indicator |
| Text Box | 638 | |

Label "Shapes" at x=556, y=102, w=52, h=17.

### Colours group (8 swatches, `x = 675..749`, `y = 66/82`)

4×2 grid, 14×14px dots, columns at `x = 675, 695, 715, 735`, rows at `y = 66` (top) and `y = 82`
(bottom):

| Node id | Column x | Row y | Hex |
|---|---|---|---|
| Ellipse 66 | 675 | 66 | `#ED2227` (red) |
| Ellipse 70 | 675 | 82 | `#F09A9C` (pink) |
| Ellipse 68 | 715 | 66 | `#313131` (dark) |
| Ellipse 71 | 715 | 82 | `#85A9ED` (light blue) |
| Ellipse 67 | 695 | 66 | `#145FEA` (blue) |
| Ellipse 72 | 695 | 82 | `#9F9F9F` (gray) — *(re-check: metadata lists Ellipse 72 at x=695,y=82 and Ellipse 71 at x=715,y=82; the swatch SVG hex-to-name mapping above in 01-design-tokens.md should be read alongside this x/y table, not assumed — the SVG fill colors are ground truth per node id, this table's x/y is ground truth per node id, join on node id)* |
| Ellipse 69 | 735 | 66 | `#FFFFFF` w/ black 0.3px stroke (white) |
| Ellipse 73 | 735 | 82 | `#99DEA5` (green) |

Label "Colours" at x=686, y=102, w=53, h=17. Container background plate ("Rectangle 583")
x=668, y=58, w=86, h=74.

### Help group (icons ~758-842)

| Icon | x | Asset |
|---|---|---|
| Settings (gear) | 758 | `figma_icon_settings_gear.png` |
| Info | 788 | `figma_icon_info.png` |
| Help ("?") | 818 | `figma_icon_help.png` |

Label "Help" at x=783, y=102, w=34, h=17. All three wrapped in one frame ("Group 698",
`1:747`, x=758, y=69, w=84, h=24).

### Right-aligned header items (not in a labeled toolbar cluster)

| Element | x | y | Notes |
|---|---|---|---|
| "Statistics" label | 1414.74 | 86.375 | |
| "Ready" status pill (red dot + text) | 1784 | 87 | |

### Divider/background plate rectangles between clusters (for reference — exact role unconfirmed)

| Node | x | y | w | h |
|---|---|---|---|---|
| Rectangle 579 | 182 | 58 | 90 | 74 |
| Rectangle 581 | 369 | 58 | 102 | 74 |
| Rectangle 582 | 471 | 58 | 197 | 74 |
| Rectangle 584 (large) | 754 | 58 | 93 | 74 |
| Rectangle 583 | 668 | 58 | 86 | 74 |

These 5 rectangles all share the row's height (74px) and appear to be per-cluster background
plates (possibly hover/pressed-state fills, all likely transparent/white in the resting state
shown in the screenshot) rather than the thin 1px vertical divider rules visible between
clusters in the screenshot. The actual thin divider rules were not individually captured as
separate nodes in the metadata pass — visually confirm against the screenshot
(`figma_full_screen_screenshot.png`) when implementing; they render as simple 1px vertical
gray lines centered in the small gaps between the rectangles above.

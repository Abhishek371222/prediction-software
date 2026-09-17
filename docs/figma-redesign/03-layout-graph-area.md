# Graph / Heatmap Area — Exact Geometry

Canvas reference: **1920 × 1080**.

## Outer container

| Element | x | y | w | h | x% | y% | w% | h% |
|---|---|---|---|---|---|---|---|---|
| "Graph" rounded-rect (`1:3`, the visible panel background) | 340 | 132 | 1580 | 847 | 17.71% | 12.22% | 82.29% | 78.43% |
| "Mask group" (`1:4`, clips everything below to the Graph rect) | 340 | 132 | 1580 | 847 | (same) | | | |

Panel fill: **`#F6F6F6`**, border **`#656161`** at 0.2px (exact values from the panel's own SVG
export, node "Graph", `figma_graph_panel_background.svg`). Corners: not rounded in practice
(node is typed "rounded-rectangle" but no radius value showed in the export path — treat as
square corners, 0px, unless the un-sampled `1:3`/`1:5` node properties say otherwise; not
confirmed before the tool-call limit hit).

## What is inside the "Mask group" (and what the 4 sub-panels actually are)

The Mask group contains **7 child frames**, clipped to the 1580×847 visible window
(`x=340..1920, y=132..979`):

| Node | Name | x | y | w | h | Visible in the 1580×847 window? |
|---|---|---|---|---|---|---|
| `1:6` | Group 686 | 340 | 132 | 640 | 847 | **Fully visible** (x 340–980) |
| `1:88` | Group 688 | 980 | 132 | 640 | 847 | **Fully visible** (x 980–1620) |
| `1:170` | Group 689 | 1620 | 130.09 | 640 | 847 | **Partially visible** — only its left 300px (x 1620–1920) show; the rest (x 1920–2260) is clipped off-canvas |
| `1:252` | Group 685 | 340 | 742.45 | 888 | 610.45 | **Partially visible** — only y 742–979 (≈237 of its 610px height) shows, at the bottom of the canvas |
| `1:334` | Group 690 | 1228 | 742.45 | 888 | 610.45 | **Partially visible / mostly clipped** — starts at x=1228 (inside frame) but extends to x=2116, and only y 742–979 shows |
| `1:416` | Group 687 | 340 | 1352.9 | 888 | 610.45 | **Fully clipped / invisible** — its y-range (1352.9–1963.4) starts below the 979px mask bottom |
| `1:498` | Group 691 | 1228 | 1352.9 | 888 | 610.45 | **Fully clipped / invisible** — same reason as Group 687 |

### What these groups actually contain

`Group 686` / `Group 688` / `Group 689` (the three 640×847 groups) each contain ~80 vertical
`<line>` elements running the full 847px height — a **gridline pattern**, not a chart. They tile
left-to-right to cover the canvas width (340→980→1620→2260), i.e. this looks like a component
that was meant to repeat as many times as needed to fill an arbitrary-width canvas, but with a
fixed 640px tile size that doesn't evenly divide the 1580px visible width (1580/640 = 2.47), so
the 3rd tile (`Group 689`) is cut off mid-tile.

`Group 685` / `Group 690` / `Group 687` / `Group 691` (the four 888×610.45 groups) were fetched
via `get_design_context` (node `1:252`) and turned out to be a **single flattened SVG image**
per group (`figma_graph_gridline_pattern_offcanvas.svg` in the assets folder) containing ~40
`<line>` elements — again a pure gridline pattern, using the same two gray tones as the small
tiles: major lines `#B4B4B4` @ 0.5px, minor lines `#C5C5C5` @ 0.2px. The SVG's own `viewBox` is
`610.65 × 888` (i.e. rotated 90° relative to its 888×610.45 frame box), confirming it's the same
underlying grid asset reused/rotated across all four of these groups.

**Conclusion: none of the "4 sub-panel groups" are separate heatmap charts, tabs, or a 2×2 tile
layout of distinct views.** They are all instances/copies of the same decorative background
gridline pattern, at different scales/rotations, largely stacked on top of / hidden behind each
other and the mask clips out most of them. Two of the four (`Group 687`, `Group 691`) are
**entirely invisible** in the current design — they render below the visible canvas and would
only become visible if the canvas were taller or the mask's bottom edge moved down. This is very
likely leftover/duplicated content from whoever built the mockup (e.g. copy-pasted a "grid"
component multiple times while laying out the frame, or leftover reference copies never
deleted) rather than intentional multi-panel UI. See `06-open-questions.md` for the
recommendation on how to treat this in the JUCE implementation (i.e.: implement a **single**
gridline background for the canvas, not four).

## Visible canvas content (from the screenshot)

What actually renders inside the 1580×847 graph panel, in front of the gridlines:

| Element | x | y | w | h | Notes |
|---|---|---|---|---|---|
| Caption text "SPL Heatmap \| 2 Device \| 52Hz \| Measured @ 2.0m" | 355 | 145 | 360 | 17 | Red text, top-left corner of the canvas, 10px inset from panel top/left |

No actual heatmap/contour rendering, cursor crosshair, or data points are present anywhere in
the canvas in this mock — it is an empty state showing only the background grid.

## Rel. SPL color-scale legend (docked at the right edge)

| Element | x | y | w | h |
|---|---|---|---|---|
| Legend group container (`1:751` "Group 641") | 1813 | 160 | 72 | 786 |
| Color bar (gradient fill, border `#c2c2c2` 0.2px) | 1813 | 163.65 | 18 | 750.66 |
| dB labels column | 1839 | 160 | 46 | 759.2 |
| "Rel. SPL" caption (below the bar) | 1820 | 925.28 | 58 | 20.72 |

Label y-positions (top-aligned text, each ~20.7px tall): `0 db`@160, `-6 db`@283.08,
`-12 db`@406.16, `-18 db`@529.24, `-24 db`@652.32, `-30 db`@775.4, `-36 db`@898.48 — an even
123.08px vertical spacing between labels (750.66 / 6 ≈ 125.1, close enough given rounding).

**Geometry discrepancy to flag:** the legend's right edge is at `1813 + 72 = 1885`, and its bar
right edge at `1813 + 18 = 1831`, both comfortably inside the 1920px canvas — this part is fine.
However the legend sits at `x=1813`, while the Graph panel's own right edge is at `340+1580=1920`
— so the legend is inset ~107px from the panel's right edge, floating near the canvas's right
edge rather than the panel's. Also note the two duplicate "Rel. SPL" text nodes (`1:752` and
`1:762`) are exact duplicates at the identical position — harmless (likely an accidental double
of the same label during editing), safe to render as a single label.

## Bottom strip (below the graph, `y=979..1080`)

| Element | x | y | w | h |
|---|---|---|---|---|
| "Last run : 11 JUL 2026 14:52:31" | 356 | 990 | 230 | 20 |
| "Elapsed : 1.5s" | 356 | 1014 | 108 | 20 |
| "SAVE IMAGE (PNG)" button | 1490 | 1003 | 194 | 34 |
| "EXPORT SPL (CSV)" button | 1691 | 1003 | 194 | 34 |
| Extra duplicate "Elapsed : 1.5s" text | 1809.4 | 1148.0 | 97.17 | 17.99 | Off-canvas (y=1148 > 1080 canvas height) — dead/unused duplicate node, ignore |

Save/Export buttons: 194×34, gray fill, thin border, black centered bold-ish caps text, 2px
gap between the two buttons (1691 − (1490+194) = 7).

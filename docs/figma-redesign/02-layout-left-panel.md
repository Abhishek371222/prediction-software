# Left Panel — Exact Geometry & Content

Canvas reference: **1920 × 1080**. All `%` columns below are `value / 1920` (x/width) or
`value / 1080` (y/height), so the panel can be re-derived proportionally for other window sizes.

## Panel container

| Element | x | y | w | h | x% | y% | w% | h% |
|---|---|---|---|---|---|---|---|---|
| "Left Panel" background rect (`1:580`) | 0 | 132 | 340 | 948 | 0% | 12.22% | 17.71% | 87.78% |
| Top search/menu strip (`1:587` "Rectangle 467") | 0 | 132 | 340 | 36 | 0% | 12.22% | 17.71% | 3.33% |
| Hamburger menu icon (`1:685` "Group 512", 3 horizontal lines) | 311 | 145 | 14.4 | 9.6 | 16.2% | 13.43% | 0.75% | 0.89% |
| Main scrollable content (`1:588` "Group 680") | 12 | 177.99 | 313.09 | 704.18 | 0.63% | 16.48% | 16.31% | 65.2% |

Panel background: white. Content area left/right inner gutter: **12px** from the panel's left
edge to each group's `x`; groups are ~313px wide inside the 340px panel, i.e. 15px right gutter
(340 − 12 − 313.09 ≈ 15).

## Section 1 — "FREQUENCY (Hz)"

| Element | x | y | w | h |
|---|---|---|---|---|
| Section header text ("FREQUENCY (Hz)", numbered "1.") | 41.99 | 180.39 | 172.73 | 20.39 |
| Header chevron/expand icon | 12 | 177.99 | 23.99 | 23.99 |
| Dropdown field box (bg `#f6f6f6`, border `#0c0c0c` 0.3px, radius 2px) | 13.2 | 211.58 | 311.89 | 35.99 |
| Field text "000 Hz" | 37.19 | 221.17 | 51.58 | 17.99 |
| Field chevron icon (inside box, right side) | 277.11 | 217.57 | 23.99 | 23.99 |

Single full-width dropdown, placeholder value "000 Hz" — this is the master frequency selector.

## Section 2 — "XN18 Units" (device list)

| Element | x | y | w | h |
|---|---|---|---|---|
| Section header text ("XN18 Units", numbered "2.") | 41.99 | 263.96 | 116.35 | 20.4 |
| Header chevron | 12 | 261.57 | 23.99 | 23.99 |
| Row container | 13.2 | 295.16 | 311.89 | 35.99 |
| Device dropdown box | 13.2 | 295.16 | 100.77 | 35.99 |
| Device dropdown text "000 Hz" | 20.4 | 303.55 | 52.78 | 17.99 |
| Device dropdown chevron | 83.98 | 301.15 | 23.99 | 23.99 |
| "+ Add" button box | 118.76 | 295.16 | 100.77 | 35.99 |
| "+ Add" text | 148.75 | 303.55 | 41.99 | 17.99 |
| "Delete" button box | 224.33 | 295.16 | 100.77 | 35.99 |
| "Delete" text | 251.92 | 303.55 | 47.98 | 17.99 |

Three equal-width (100.77px) boxes in a row: device dropdown, Add button, Delete button — all
same styling as the frequency dropdown (`#f6f6f6` fill, `#0c0c0c` 0.3px border, 2px radius).

## Section 3 — "SELECTED XN18" (per-device parameters)

| Element | x | y | w | h |
|---|---|---|---|---|
| Section header text ("SELECTED XN18", numbered "3.") | 38.39 | 346.35 | 161.93 | 20.39 |
| Header chevron | 12 | 345.14 | 23.99 | 23.99 |

Four identical label+slider+value rows, each: a 12px label on the left, a horizontal slider
track (94.8–96px wide × 12px tall) in the middle, and a value box (100.77 × 35.99, same
`#f6f6f6`/`#0c0c0c`/2px style, text right-aligned-ish showing "10.0") on the right at
`x=224.33`.

| Row | Label | Label pos (x,y) | Slider pos (x,y,w,h) | Value box y | Value text pos |
|---|---|---|---|---|---|
| X Position (m) | "X Position (m)" | 13.2, 385.93 | 121.16, 390.73, 94.78, 12.0 | 378.73 | 261.52, 388.33 |
| Y Position (m) | "Y Position (m)" | 13.2, 435.12 | 119.96, 438.71, 95.97, 12.0 | 426.72 | 261.52, 436.32 |
| Gain (db) | "Gain (db)" | 13.2, 483.10 | 119.96, 486.70, 95.97, 12.0 | 474.70 | 261.52, 484.30 |
| Delay (ms) | "Delay (ms)" | 13.2, 531.08 | 121.16, 534.68, 95.97, 12.0 | 522.68 | 261.52, 532.28 |

Slider anatomy (from downloaded SVGs, e.g. `figma_slider_track_x_position.svg`): an outlined
"track remainder" pill (black 0.2px stroke, no fill, rounded ends) plus a filled "progress" pill
(`#313131` fill) drawn from the right edge, plus a circular thumb (`#D81F1F`, ~12px diameter) at
the boundary between filled/unfilled — i.e. sliders are drawn right-to-left in the source (each
track's filled rect uses a `matrix(-1 0 0 1 …)` flip), but visually render as a normal
left-filled slider in the screenshot. Row heights ~12px for the track, but the visible/clickable
row height in the screenshot appears larger (~24-29px) — the 12px is just the thin track
graphic; treat the row's hit-target height as matching the adjacent checkbox rows (~28.8px) for
consistency.

Below the 4 parameter rows, 3 toggle rows (checkbox + label), inside `1:639` "Group 659" at
`x=13.2, y=568.27, w=177.54, h=105.56`:

| Toggle | Checkbox pos (x,y,w,h) | Label pos (x,y) | State in mock |
|---|---|---|---|
| Invert Polarity | 13.2, 568.27, 28.8, 28.78 | 46.79, 574.27 | unchecked |
| Reverse Orientation | 13.2, 606.66, 28.79, 28.78 | 46.79, 612.65 | unchecked |
| Enabled | 13.2, 645.05, 28.8, 28.78 | 46.8, 651.05 (Medium weight, bolder than the other two) | **checked** (red fill, white check) |

Checkbox visuals: unchecked = white square, dark charcoal outline (~2-3px), 2px-ish radius;
checked = solid red (`#D81F1F`-family, sampled via `figma_icon_checkbox_checked.png`) square
with white checkmark.

## Section 4 — "SIMULATION"

| Element | x | y | w | h |
|---|---|---|---|---|
| Section header text ("SIMULATION", numbered "4.") | 38.39 | 690.23 | 135.54 | 20.39 |
| Header chevron | 12 | 687.83 | 23.99 | 23.99 |

Two label+slider+value rows, same anatomy as Section 3's sliders but with a **narrower** value
box (91.17 wide instead of 100.77):

| Row | Label pos | Slider pos (x,y,w,h) | Value box (x,y,w,h) | Value text pos |
|---|---|---|---|---|---|
| Grid Resolution | 13.2, 729.82 | 131.96, 733.41, 95.97, 12.0 | 233.92, 721.42, 91.17, 35.99 | 265.11, 729.82 |
| db Floor | 13.2, 770.60 | 131.96, 774.20, 95.97, 12.0 | 233.92, 762.21, 91.17, 35.99 | 265.11, 770.60 |

Then, still within Section 4's visual grouping (per the metadata nesting, "Contour Bands" and
"Measurement set" are siblings placed **above** the SIMULATION header in raw y-order but grouped
with it logically/visually as trailing controls at the bottom of the panel — see note below):

| Element | x | y | w | h |
|---|---|---|---|---|
| "Contour Bands (3b)" checkbox | 13.2 | 807.79 | 28.79 | 28.79 |
| "Contour Bands (3b)" label | 46.79 | 813.79 | 140.37 | 17.99 |
| "Measurement  set" label (double space in source text) | 14.4 | 855.77 | 130.76 | 17.99 |
| Measurement dropdown box (border `rgba(12,12,12,.75)` 0.3px, else same style) | 173.94 | 846.18 | 151.15 | 35.99 |
| Measurement dropdown text "Ground Plane" | 181.14 | 854.57 | 100.77 | 17.99 |
| Measurement dropdown chevron | 293.9 | 852.18 | 23.99 | 23.99 |

Note on ordering: in the Figma layer tree, "Contour Bands" and "Measurement set" (`1:610`
"Group 679") are a **sibling group before** the "SIMULATION" header+sliders group (`1:619`
"Group 678") in document order, but by `y` position they render **below** the sliders
(Contour Bands at y=807, Measurement set at y=846, vs. sliders at y=721–798). Screenshot
confirms visual order top-to-bottom is: SIMULATION header → Grid Resolution slider → db Floor
slider → Contour Bands checkbox → Measurement set dropdown. Implement in that visual order.

## Bottom action buttons (pinned to panel bottom, outside the scroll group)

| Button | x | y | w | h |
|---|---|---|---|---|
| "Set to Default" | 24 | 979 | 295.1 | 28.79 |
| "Clear All" | 24 | 1015 | 295.1 | 28.79 |

Both full-width (295px, panel content width) gray buttons with centered black text, 2px-ish
radius, same border style family as inputs. Gap between them: ~7px (1015 − (979+28.79) ≈ 7.2).
Gap from "Clear All" bottom (1015+28.79=1043.79) to canvas bottom (1080): ~36px.

## Percent-of-canvas quick reference (for responsive re-implementation)

- Panel width: **17.71%** of 1920 (340px)
- Panel top: **12.22%** of 1080 (132px, i.e. below the 132px header)
- Content left inset: 12px ≈ **0.625%** of 1920
- Standard row height (input/button): 35.99px ≈ **3.33%** of 1080
- Checkbox size: 28.8px ≈ **2.67%** of 1080

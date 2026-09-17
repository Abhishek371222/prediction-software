# Design Tokens

`get_variable_defs` on the root node (`1:2`) returned an **empty object** — this Figma file
defines **no Figma Variables / design tokens**. Every color, font, and spacing value below was
read directly off individual node styles via `get_design_context`, or extracted from downloaded
SVG source (exact `fill=`/`stroke=` attributes). Values marked "(from SVG)" are exact; values
marked "(from screenshot)" are visually confident but not pixel-sampled since the corresponding
node's design context could not be pulled (Figma MCP tool call limit reached — see
`06-open-questions.md`).

## Color palette

### Neutrals / structure

| Hex | Usage |
|---|---|
| `#FFFFFF` | Page/panel background (left panel, header, canvas outer area) |
| `#F6F6F6` | Input field fill (all text boxes, dropdowns in left panel); Graph canvas panel fill (from SVG `svg_1`, node "Graph") |
| `#0C0C0C` | Input field border (0.3px), e.g. frequency/device/position/gain/delay boxes |
| `rgba(12,12,12,0.75)` | Border of the "Measurement set" dropdown (slightly lower-opacity variant of the standard border) |
| `#656161` | Graph panel outer border/stroke (0.2px), from SVG `svg_1` |
| `#C2C2C2` | Border of the Rel. SPL color-scale bar (0.2px) |
| `#B4B4B4` | Major gridline color inside the graph canvas (0.5px stroke) |
| `#C5C5C5` | Minor gridline color inside the graph canvas (0.2px stroke) |
| `#313131` | Filled/active portion of horizontal sliders; dark toolbar icon glyph color (approx., from screenshot); "dark" colour-swatch dot |
| `#434343` | Stroke of the small decorative curved "Vector 77" glyph near the Shapes group |
| `black` (`#000000`) | Body text default color; header row-1 top border (0.5px solid black) |
| `rgba(0,0,0,0.5)` | Header row-2 (toolbar row) border color |

### Accent / semantic colors

| Hex | Usage |
|---|---|
| `#D81F1F` | Slider thumb/handle circle (all 5 left-panel sliders) |
| Red (status dot) | "Ready" status indicator dot in header, top-right (exact hex not sampled — see open questions; visually a saturated red matching `#ED2227`-family) |
| Red caption text | "SPL Heatmap \| 2 Device \| 52Hz \| Measured @ 2.0m" caption above the graph, and "1. FREQUENCY", section header carets are black — only the graph caption text renders in red (exact hex not sampled, visually ≈ `#ED2227`/`#D81F1F` family) |

### "Colours" toolbar palette (8 swatches, exact hex from SVG)

These are the 8 small circular dots in the header's "Colours" cluster, laid out as a 4×2 grid
(top row / bottom row), 14×14px each, `cx=cy=7, r=7` (r=6.85 for the white one, which has a
stroke instead of being borderless):

| Swatch | Hex | Notes |
|---|---|---|
| Red | `#ED2227` | node "Ellipse 66" |
| Blue | `#145FEA` | node "Ellipse 67" |
| Dark / near-black | `#313131` | node "Ellipse 68" |
| White | `#FFFFFF` (stroke `#000000` @ 0.3px) | node "Ellipse 69", only swatch with a visible outline |
| Pink | `#F09A9C` | node "Ellipse 70" |
| Light blue / periwinkle | `#85A9ED` | node "Ellipse 71" (positioned as the "light-blue" slot but is a mid periwinkle) |
| Gray | `#9F9F9F` | node "Ellipse 72" |
| Green | `#99DEA5` | node "Ellipse 73" |

Layout of the 8 dots (from metadata, all 14×14): top row at `y=66`, `x = 675, 695, 715, 735`;
bottom row at `y=82`, same 4 x-positions. So the visual grid is:

```
Red(675,66)   Blue(695,66)    Pink(715,66)?  Green(735,66)?      <- verify against screenshot; see note
Pink(675,82)  Gray(695,82)    Dark(715,82)   Lav(735,82)?
```
(Exact left-to-right/top-to-bottom mapping of which ellipse id sits in which visual cell was not
double-checked against fill color per-cell before the tool rate-limit hit — the table above gives
the correct hex-per-node-id; cross-reference `node id → x,y` in `04-layout-header-toolbar.md`
before final implementation. This is flagged again in `06-open-questions.md`.)

### Rel. SPL color-scale gradient (right-edge legend)

Exact CSS gradient pulled from the node (`1:751`, "Group 641"):

```
linear-gradient(180deg,
  rgb(178, 22, 25)  25%,      /* #B21619 */
  rgb(237, 34, 39)  43.75%,   /* #ED2227 */
  rgb(50, 129, 185) 65.385%,  /* #3281B9 */
  rgb(10, 77, 116)  82.692%,  /* #0A4D74 */
  rgb(35, 31, 32)   100%      /* #231F20 */
)
```
Applied top-to-bottom over a bar 18px wide × 750.66px tall at `x=1813, y=163.65`. Labels "0 db"
at the top down to "-36 db" at the bottom, in 6 db increments (0, -6, -12, -18, -24, -30, -36).
Because there is no explicit 0% stop, the color is flat `rgb(178,22,25)` from 0%–25% of the bar
height, then interpolates through the remaining stops.

## Typography

Single font family used everywhere observed: **Montserrat**, in three weights:

| Style token | Weight | Size | Used for |
|---|---|---|---|
| `Montserrat:Medium` | Medium (500) | 14px | Section headers ("FREQUENCY (Hz)", "XN18 Units", "SELECTED XN18", "SIMULATION"), rendered as `<ol>`+numbered list markers (the "1.", "2.", "3.", "4." prefixes are literal ordered-list numbers in the Figma source, not separate text) |
| `Montserrat:Regular` | Regular (400) | 12px | All field labels and values in the left panel ("X Position (m)", "10.0", "000 Hz", "+ Add", "Delete", etc.) |
| `Montserrat:Medium` | Medium (500) | 12px | "Enabled" checkbox label (bold-ish weight vs. its sibling regular labels) |
| `Montserrat:SemiBold` | SemiBold (600) | 14px | Rel. SPL legend labels ("0 db" … "-36 db", "Rel. SPL") |

Text color is `black` / `#000000` (or plain `text-black` Tailwind token, i.e. pure black) for all
sampled left-panel and legend text. Line-height is CSS `normal` (no explicit override) in all
sampled nodes. No letter-spacing overrides were found on any sampled text node.

Toolbar group labels ("File", "Navigation", "View", "Tools", "Shapes", "Colours", "Help") and
the header title/"Auto Save"/"Statistics" text were only seen via screenshot + metadata
bounding boxes (not via `get_design_context`, due to the rate limit) — assume the same
Montserrat family; exact weight/size were not sampled for these specific nodes (see open
questions).

## Spacing / sizing scale

Observed repeating measurements in the left panel (not formal tokens, just recurring values):

- Standard input/box height: **35.99px** (rounds to 36px)
- Small icon box (dropdown chevron / checkbox): **~24px** square (23.99–24px)
- Small slider-track box: **~28.8px** square (checkbox/toggle hit targets)
- Section header row height: **~24px**, with the numbered label baseline offset ~2.4px below the
  row top
- Left/right inner padding inside the sidebar: **12px** (all section groups start at `x=12`
  relative to the panel, panel is 340 wide, giving ~305px usable content width plus the 12px
  gutter on the right implied by `313.09px` group widths)
- Standard corner radius on input fields/buttons: **2px** (`rounded-[2px]`)
- Standard border width: **0.2–0.3px** hairlines (a "1px @ 200% DPI" convention — treat as 1
  device px when implementing in JUCE, since sub-pixel strokes at 0.2–0.3px are Figma's way of
  drawing 1px hairlines that don't double up under scaling)
- Toolbar icon button size: **24×24px**, on a 74px-tall toolbar row, vertically centered-ish
  (icons start at `y=69`, `69+24=93`, toolbar row is `y=58..132`, so icons are roughly centered
  with a bit more headroom above than below — see `04-layout-header-toolbar.md`)
- Toolbar cluster divider rectangles: exact widths vary per cluster, see
  `04-layout-header-toolbar.md`

## Corner radii

- Input boxes / dropdowns / sliders' value boxes / bottom action buttons: **2px**
- Slider track pill: fully rounded (`rx = height/2`, i.e. `rx≈3.5` for a ~7px-tall track — a
  true pill/capsule shape)
- Slider thumb: circular (full ellipse)
- Colour palette swatches: circular

## Stroke widths

- Input field border: 0.3px
- "Measurement set" dropdown border: 0.3px (at 75% black opacity instead of solid `#0c0c0c`)
- Graph panel border: 0.2px
- Rel. SPL bar border: 0.2px
- Major/minor gridlines: 0.5px / 0.2px respectively
- Slider track outline: 0.2px
- Header row-1 border: 0.5px solid black
- Header row-2 (toolbar) border: 0.5px, black at 50% opacity
- Hamburger-menu icon lines (left panel top): 1.5px, black, round caps
- Decorative "Vector 77" curved glyph: 2px stroke, `#434343`, round cap

## Shadows / blur

None observed on any sampled node. No `box-shadow`/`drop-shadow`/blur properties appeared in any
of the `get_design_context` outputs collected. Buttons and panels all appear flat (no elevation).

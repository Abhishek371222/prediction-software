# Open Questions & Confidence Notes

This documents what is **certain** (pulled directly from `get_design_context` code/CSS or exact
SVG source) vs. **inferred** (read from the screenshot + bounding-box metadata only), and lists
gaps caused by hitting the Figma MCP tool-call rate limit ("You've reached the Figma MCP tool
call limit on the Starter plan") partway through extraction.

## Why some data is missing

The Figma MCP server used here is on a **Starter plan** with a hard cap on tool calls. After
~13 `get_design_context`/`download_assets` calls in this session, every further call (including
plain `download_assets`) started failing with a rate-limit error, and did not recover before
this documentation pass had to be finished. All data in `01`–`05` reflects what was fetched
**before** the limit hit, supplemented by the one full-resolution screenshot and the complete
page metadata (exact x/y/w/h for every node — that call succeeded and is trustworthy/exact for
geometry). What's missing is mostly **exact hex codes and CSS for header/toolbar nodes** and a
handful of individual toolbar icon exports.

## High confidence (verified via get_design_context / raw SVG source)

- All Left Panel geometry, colors, fonts (`02-layout-left-panel.md`, `01-design-tokens.md`)
- The Rel. SPL gradient, its exact color stops, and its geometry
- The Graph panel's own fill/border colors (`#F6F6F6` / `#656161`)
- The gridline pattern colors (`#B4B4B4` / `#C5C5C5`) and the finding that all 4 "sub-panel"
  groups are the same decorative gridline asset, mostly clipped/hidden
- The 8 Colours-palette swatch hex values (exact, from SVG `fill=`)
- The slider anatomy (track/fill/thumb colors and shapes)
- All checkbox/logo/most toolbar icon glyphs (downloaded as image assets, so pixel-exact
  regardless of color extraction)
- The absence of any Figma Variables/design tokens (`get_variable_defs` returned `{}`)
- The absence of shadows/blur anywhere sampled

## Medium confidence (screenshot + metadata bounding boxes, not cross-checked against get_design_context CSS)

- **Header row 1 vs row 2 exact styling** — border colors for the two header rows were sampled
  (`1:702` = 0.5px solid black; `1:703` = 0.5px black@50%), but the fill colors of those two
  rows, and of the 5 "cluster background plate" rectangles in the toolbar (`Rectangle 579/581/
  582/583/584`), were **not** sampled. The screenshot shows them all as plain white/off-white
  with no visible fill difference from the page background — assume white (`#FFFFFF`) unless a
  future pass samples them directly.
- **The thin vertical divider rules** between toolbar clusters, visible in the screenshot, do
  not correspond to any obviously-named node in the metadata (the "Rectangle 579/581/582/583/
  584" rects are each ~74-197px wide, i.e. whole-cluster-width, not thin 1px dividers). Either
  these rects themselves render as very subtle borders/backgrounds that read as dividers at
  their edges, or the true divider lines are separate un-named/un-sampled nodes. **Recommend
  re-fetching `get_design_context` on nodes `1:726`–`1:729` and `1:763` once the rate limit
  resets**, to get their exact fill/border before implementing.
- **Toolbar icon glyph fill color** — icons were downloaded as flat PNG/SVG raster art, so their
  colors are baked in and pixel-correct, but no hex was independently confirmed via
  `get_design_context`'s CSS text output for these specific nodes (only visually, they read as
  a dark charcoal, consistent with the `#313131`/`#434343` family seen elsewhere).
- **Which node id occupies which visual cell in the 4×2 Colours grid.** Section
  `04-layout-header-toolbar.md` flags a possible mismatch: the metadata gives each Ellipse's
  x/y position, and the SVG source gives each Ellipse's hex fill — both are exact and keyed by
  node id — but this pass did not re-verify that node-id-to-position join against the rendered
  screenshot pixel-by-pixel. Cross-check before committing to a final palette layout in code.
- **Exact corner radius on the main Graph panel** — the node is typed "rounded-rectangle" in
  Figma's own metadata but the exported SVG path for it shows straight corners (no visible
  curve in the path data). Treat as 0px unless disproven.
- **Header row 1 element vertical alignment** — a few nodes (Statistics label, Ready pill) have
  y-coordinates landing in the "toolbar strip" band even though they read, in the screenshot, as
  vertically-centered-ish within the whole 132px header. Treated pragmatically in
  `04-layout-header-toolbar.md` by keeping their raw metadata y as ground truth.

## Explicitly flagged as ambiguous / likely designer artifacts (not to be replicated as intentional UI)

- **The 4 "sub-panel" groups in the Mask group** (`Group 685/687/690/691`) — confirmed to be
  duplicate/rotated copies of the same gridline decoration, two of which render completely
  off-canvas and invisible. This reads as leftover content from building the mockup (e.g.
  copy-pasting a "grid" component while laying out the frame) rather than intentional 4-panel
  UI. **Recommendation for the JUCE implementation: render a single gridline background for the
  canvas** (matching the visible `#B4B4B4`/`#C5C5C5` pattern), not four separate panels.
- **Duplicate "Rel. SPL" text nodes** (`1:752`/`1:762`) — exact duplicates at the same position.
  Harmless; render once.
- **Duplicate off-canvas "Elapsed : 1.5s" text node** (`1:697`, at y=1148, below the 1080px
  canvas) — dead node, ignore.
- **"Group 689" (3rd gridline tile) is cut off mid-tile** at the canvas's right edge, because
  the 640px tile size doesn't evenly divide the 1580px visible graph width. If reproducing the
  gridline tiling literally, this clipped-tile artifact would carry over; more likely the intent
  is a continuous, non-tiled grid (see `figma_graph_gridline_pattern_offcanvas.svg` for a single
  usable tile to redraw continuously instead).

## States not visible in this static export

This is a single static frame with no interaction states captured. The following are **not
documented anywhere** because no hover/active/disabled/focus variant exists in the file:

- Hover/pressed states for any toolbar icon, button, checkbox, or slider thumb
- What the toolbar looks like with a tool actively selected (e.g. Pencil "on") — is there a
  highlighted background, an outline, a color change? Unknown.
- Any dropdown's open/expanded state (frequency selector, device selector, measurement-set
  selector, the 4 numbered section collapse/expand carets) — only the closed/collapsed-looking
  static state is shown; whether the caret rotates or the section content actually collapses is
  assumed by convention (chevron = expand/collapse control) but not shown.
- What the graph canvas looks like **with actual heatmap/contour data rendered** — the mockup
  only shows the empty grid background; no heatmap gradient, contour bands, or per-device
  markers are shown anywhere in this file, despite Section 4's "Contour Bands (3b)" control and
  the caption text implying real data ("2 Device | 52Hz | Measured @ 2.0m"). The 5-stop Rel. SPL
  gradient in `01-design-tokens.md` is the only real hint at what heatmap coloring should look
  like.
- The exact behavior/appearance of the "Auto Save" checkbox when checked (only unchecked state
  shown).

## Recommended follow-up before implementation

1. If/when the Figma MCP rate limit resets, re-run `get_design_context` on: header divider
   rects (`1:726`–`1:729`, `1:763`), the two header row background fills (`1:702`, `1:703`), and
   the remaining un-downloaded toolbar icons (Zoom Out, Ruler, Polyline, Filled Circle,
   Rectangle, Text Box — listed in `05-icons-and-assets.md`).
2. Confirm the Colours-grid node-id-to-position join visually against
   `figma_full_screen_screenshot.png` before finalizing the palette order in code.
3. Decide how to handle the "4 sub-panel" gridline duplication — recommendation above is to
   collapse it to one continuous background pattern.

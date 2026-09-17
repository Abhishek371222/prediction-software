# Icons & Assets

All files below are saved at:
`D:\WORKING_LATESTSHYAM_GUI\ShyamGui\Assets\FigmaRedesign\`

40 files total (2 full-screen reference images + 38 individual icon/pattern/swatch assets).
PNG icons were exported by Figma at a fixed **90×90px** raster regardless of their on-canvas
display size (they're rendered at 24×24 or smaller in the actual UI — treat the PNGs as
high-res source art to downscale, not native-size assets). SVGs are vector and carry their true
on-canvas size in the `width`/`height` attributes.

| File | Type | Native size | Used for / on-canvas position |
|---|---|---|---|
| `figma_full_screen_screenshot.png` | Reference | 1920×1080 | Full-screen rendered screenshot of the whole mockup, for visual QA |
| `figma_full_screen_export.png` | Reference | 1920×1080 | Full-screen flat PNG export of node `1:2` (alternate render pass, same content) |
| `figma_logo_atomik_horizontal_black.png` | Logo | 4096×1893 (displays at 94×18) | Header row 1, top-left, "ATOMIK•" wordmark, node `1:701` "Horizontal Logo - Black 1" |
| `figma_icon_dropdown_chevron.png` | Icon | 90×90 (displays at 24×24) | "Sort Down" chevron — reused on every dropdown/collapsible-section header in the left panel (frequency box, device box, section carets, measurement-set box) |
| `figma_icon_checkbox_unchecked.png` | Icon | 90×90 (displays at ~29×29) | Empty checkbox state — Invert Polarity, Reverse Orientation, Contour Bands |
| `figma_icon_checkbox_checked.png` | Icon | 90×90 (displays at ~29×29) | Checked checkbox state (red fill + white check) — "Enabled" toggle |
| `figma_slider_track_x_position.svg` | Vector | 94.78×11.99 | X Position slider track+thumb, left panel Section 3 |
| `figma_slider_track_y_position.svg` | Vector | 95.98×12.0 | Y Position slider |
| `figma_slider_track_gain.svg` | Vector | 95.97×12.0 | Gain (db) slider |
| `figma_slider_track_delay.svg` | Vector | 95.97×12.0 | Delay (ms) slider |
| `figma_slider_track_grid_resolution_and_db_floor.svg` | Vector | 95.97×12.0 | Shared track graphic reused for both Grid Resolution and db Floor sliders in Section 4 |
| `figma_graph_panel_background.svg` | Vector | 1580×847 | The main canvas panel's own fill/stroke (`#F6F6F6` fill, `#656161` 0.2px border), node "Graph" `1:5` |
| `figma_graph_gridline_pattern_offcanvas.svg` | Vector | 610.65×888 | The gridline pattern used (and mostly clipped/hidden) inside the graph "Mask group" — see `03-layout-graph-area.md` for why 4 groups share this one pattern |
| `figma_icon_left_panel_hamburger_menu.svg` | Vector | 15.9×11.1 | 3-line "menu" icon, left-panel top strip, node "Group 512" `1:685` |
| `figma_icon_shapes_polyline_vector.svg` | Vector | 10×20 | Small decorative curved glyph ("Vector 77") sitting between the Polyline and Filled Circle toolbar icons |
| `figma_colour_swatch_red.svg` | Vector | 14×14 | Colours palette dot, `#ED2227` |
| `figma_colour_swatch_blue.svg` | Vector | 14×14 | Colours palette dot, `#145FEA` |
| `figma_colour_swatch_dark.svg` | Vector | 14×14 | Colours palette dot, `#313131` |
| `figma_colour_swatch_white.svg` | Vector | 14×14 | Colours palette dot, white fill + black 0.3px stroke |
| `figma_colour_swatch_pink.svg` | Vector | 14×14 | Colours palette dot, `#F09A9C` |
| `figma_colour_swatch_light_blue.svg` | Vector | 14×14 | Colours palette dot, `#85A9ED` |
| `figma_colour_swatch_gray.svg` | Vector | 14×14 | Colours palette dot, `#9F9F9F` |
| `figma_colour_swatch_green.svg` | Vector | 14×14 | Colours palette dot, `#99DEA5` |
| `figma_icon_file_add_new.png` | Icon | 90×90 (24×24) | Toolbar "File" cluster — New |
| `figma_icon_file_open_folder.png` | Icon | 90×90 (24×24) | Toolbar "File" cluster — Open |
| `figma_icon_file_save.png` | Icon | 90×90 (24×24) | Toolbar "File" cluster — Save |
| `figma_icon_file_save_as.png` | Icon | 90×90 (24×24) | Toolbar "File" cluster — Save As (floppy disk + pencil) |
| `figma_icon_file_export_pdf.png` | Icon | 90×90 (24×24) | Toolbar "File" cluster — Export PDF (document + down arrow) |
| `figma_icon_nav_cursor.png` | Icon | 90×90 (24×24) | Toolbar "Navigation" cluster — Select/Cursor tool |
| `figma_icon_nav_hand.png` | Icon | 90×90 (24×24) | Toolbar "Navigation" cluster — Pan/Hand tool |
| `figma_icon_view_zoom_in.png` | Icon | 90×90 (24×24) | Toolbar "View" cluster — Zoom In (magnifying glass +) |
| `figma_icon_view_full_screen.png` | Icon | 90×90 (24×24) | Toolbar "View" cluster — Full Screen (corner brackets) |
| `figma_icon_tools_pencil.png` | Icon | 90×90 (24×24) | Toolbar "Tools" cluster — Pencil/draw tool |
| `figma_icon_tools_eraser.png` | Icon | 90×90 (24×24) | Toolbar "Tools" cluster — Eraser tool |
| `figma_icon_shapes_line.png` | Icon | 90×90 (24×24) | Toolbar "Shapes" cluster — Line tool |
| `figma_icon_shapes_fill_preview_dark_square.png` | Icon | 90×90 (displays at 20×20) | Small solid dark-gray square between the Shapes tools and the Colours group — likely a "current fill/stroke color" preview swatch, node "Rectangle 584" `1:736` |
| `figma_icon_settings_gear.png` | Icon | 90×90 (24×24) | Toolbar "Help" cluster — Settings gear |
| `figma_icon_info.png` | Icon | 90×90 (24×24) | Toolbar "Help" cluster — Info (circle-i) |
| `figma_icon_help.png` | Icon | 90×90 (24×24) | Toolbar "Help" cluster — Help ("?") |

## Icons referenced in the design but **not individually downloaded**

These exist in the toolbar (confirmed via metadata bounding boxes + screenshot) but the Figma
MCP rate limit was hit before each could be pulled as its own asset. They are close visual
siblings of icons already downloaded above (same family/weight), so use the downloaded ones as
a style reference if redrawing:

- **Zoom Out** (View cluster, `x=308`) — mirror of Zoom In, magnifying glass with "−"
- **Ruler** (Tools cluster, `x=438`) — triangular ruler/set-square glyph per the screenshot
- **Polyline** (Shapes cluster, `x=508`) — zig-zag line icon
- **Filled Circle** (Shapes cluster, `x=552`) — solid black circle
- **Rectangle** (Shapes cluster, `x=582`) — outlined rectangle
- **Text Box** (Shapes cluster, `x=638`) — "T" in a box

## Total asset count

38 icon/pattern/swatch files + 2 full-screen reference renders = **40 files** in
`D:\WORKING_LATESTSHYAM_GUI\ShyamGui\Assets\FigmaRedesign\`.

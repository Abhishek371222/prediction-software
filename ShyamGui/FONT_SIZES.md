# Font Sizes — Exact Values Reference

**Source of truth:** `Source/UiTextConfig.h` → `namespace UiConfig::FontSize`  
**Aliases:** `Source/BrandTheme.h` → `namespace Brand::Type`  
**Captured from working tree:** 2026-08-01  
**Baseline comparison:** git initial commit `c6a606b` (`UiTextConfig.h` as first committed)

> Do not edit sizes in `BrandTheme.h`. Change only `UiTextConfig.h`, then rebuild.  
> Runtime size = `basePx * UiConfig::Scale::factor` via `Brand::UI::scaledFont(basePx)`.  
> At default window **1340 × 820**, `Scale::factor = 1.0`, so on-screen px == the constants below.

---

## 1. Current `UiConfig::FontSize` values (exact)

| Variable | Exact value | Used for |
|---|---:|---|
| `appTitle` | **13.0f** | Header: “Atomik Acoustic Simulation Engine” |
| `appVersion` | **9.0f** | Header version / build label (`beta_v…`) |
| `paramChip` | **12.14f** | Top param strip chips (`f = … Hz`, Grid, View, …) |
| `sectionHeader` | **9.0f** | Sidebar / info section headers |
| `fieldLabel` | **9.5f** | Field labels, checkbox text |
| `fieldValue` | **9.5f** | Slider numeric boxes, combo values, info values |
| `infoKey` | **9.5f** | Info-panel row keys (left column) |
| `plotTitle` | **9.0f** | Plot header title above heatmap |
| `plotGridNumber` | **10.0f** | Axis tick numbers on plot edges |
| `plotAxis` | **10.0f** | Secondary plot axis / annotation text |
| `colourBarTick` | **14.0f** | Colour-bar dB tick labels (`0 dB`, `-6 dB`, …) |
| `colourBarTitle` | **15.0f** | Colour-bar caption (`Rel. SPL`) |
| `speakerPolarityBadge` | **9.0f** | `+/-` polarity badge on speaker marker |
| `speakerId` | **11.0f** | Speaker id label (`XN18_1`) |
| `speakerIdSelected` | **11.0f** | Selected speaker id label |
| `button` | **9.0f** | General `TextButton` captions |
| `bottomBarButton` | **9.0f** | Bottom Export / View Mode pill captions |
| `viewTileCaption` | **1.0f** | Legacy view-tile captions |
| `statusBar` | **1.0f** | Status strip (`Ready`, Last run, Elapsed) |
| `bottomSectionTitle` | **7.5f** | `SAVE / EXPORT` / `VIEW MODE` section titles |
| `polarLegend` | **12.0f** | On-screen polar legend |
| `polarLegendMeta` | **10.0f** | Polar legend meta line |
| `polarEmptyMessage` | **15.0f** | Empty measured-polar message |
| `exportBrand` | **16.0f** | PNG/SVG export brand text |
| `exportFrequency` | **18.0f** | PNG/SVG export frequency |
| `exportChartTitle` | **17.0f** | PNG/SVG export chart title |
| `exportSubtitle` | **12.0f** | PNG/SVG export subtitle |
| `exportPolarRing` | **11.0f** | Export polar dB rings & angle labels (mono) |
| `exportPolarCenter` | **13.0f** | Export polar center label |
| `sidebarMainValue` | **= fieldValue → 9.5f** | Sidebar combo values + slider numeric boxes |
| `sidebarSectionTitle` | **= sectionHeader → 9.0f** | Sidebar collapsible section titles |
| `sidebarButtonText` | **= button → 9.0f** | Sidebar buttons / `< >` steppers |
| `sidebarFieldLabel` | **= fieldLabel → 9.5f** | Sidebar field labels / helpers / checkboxes |
| `freqValue` | **= sidebarMainValue → 9.5f** | Frequency card value (alias) |
| `freqSectionTitle` | **= sidebarSectionTitle → 9.0f** | Frequency section title (alias) |
| `freqStepGlyph` | **= sidebarButtonText → 9.0f** | Frequency `< >` glyphs (alias) |
| `dashActionButton` | **18.0f** | Dashboard `NEW PROJECT` / `OPEN EXISTING…` |
| `dashSubtitle` | **19.0f** | Dashboard subtitle |
| `dashRecentHeader` | **18.0f** | `RECENT PROJECTS` header |
| `dashRecentItem` | **18.0f** | Recent project list rows |
| `plotFitButton` | **10.0f** | Plot toolbar `Fit` button |
| `headerStatsButton` | **12.0f** | Header `Stats` dropdown button |
| `prefsTitle` | **24.0f** | Preferences dialog title |
| `prefsSectionHdr` | **14.5f** | Preferences section headers |
| `prefsLabel` | **14.5f** | Preferences field labels |
| `prefsButton` | **13.0f** | Preferences buttons |
| `prefsNote` | **11.5f** | Preferences note text (mono) |

### Alias note (comments in source are stale)

In `UiTextConfig.h`, comments next to aliases still say `// 15.0`, `// 16.0`, `// 13.0`, `// 14.0`.  
**Actual resolved values today are the alias targets above** (`9.5f` / `9.0f`), not those old comment numbers.

---

## 2. Changes vs initial commit (`c6a606b`)

| Variable | Initial | Current | Delta |
|---|---:|---:|---:|
| `appTitle` | 26.0f | **13.0f** | −13.0 |
| `appVersion` | 12.0f | **9.0f** | −3.0 |
| `paramChip` | 18.14f | **12.14f** | −6.0 |
| `sectionHeader` | 14.5f | **9.0f** | −5.5 |
| `fieldLabel` | 14.5f | **9.5f** | −5.0 |
| `fieldValue` | 14.5f | **9.5f** | −5.0 |
| `infoKey` | 14.5f | **9.5f** | −5.0 |
| `plotTitle` | 15.0f | **9.0f** | −6.0 |
| `plotGridNumber` | 11.0f | **10.0f** | −1.0 |
| `plotAxis` | 11.0f | **10.0f** | −1.0 |
| `colourBarTick` | 14.0f | **14.0f** | 0 (unchanged) |
| `colourBarTitle` | 15.0f | **15.0f** | 0 (unchanged) |
| `speakerPolarityBadge` | 10.0f | **9.0f** | −1.0 |
| `speakerId` | 13.0f | **11.0f** | −2.0 |
| `speakerIdSelected` | 14.0f | **11.0f** | −3.0 |
| `button` | 13.5f | **9.0f** | −4.5 |
| `bottomBarButton` | 11.0f | **9.0f** | −2.0 |
| `viewTileCaption` | 11.0f | **1.0f** | −10.0 |
| `statusBar` | 12.0f | **1.0f** | −11.0 |
| `bottomSectionTitle` | 12.5f | **7.5f** | −5.0 |
| `polarLegend` | 12.0f | **12.0f** | 0 |
| `polarLegendMeta` | 10.0f | **10.0f** | 0 |
| `polarEmptyMessage` | 15.0f | **15.0f** | 0 |
| `exportBrand` | 16.0f | **16.0f** | 0 |
| `exportFrequency` | 18.0f | **18.0f** | 0 |
| `exportChartTitle` | 17.0f | **17.0f** | 0 |
| `exportSubtitle` | 12.0f | **12.0f** | 0 |
| `exportPolarRing` | 11.0f | **11.0f** | 0 |
| `exportPolarCenter` | 13.0f | **13.0f** | 0 |
| `dashActionButton` | 18.0f | **18.0f** | 0 |
| `dashSubtitle` | 19.0f | **19.0f** | 0 |
| `dashRecentHeader` | 18.0f | **18.0f** | 0 |
| `dashRecentItem` | 18.0f | **18.0f** | 0 |
| `plotFitButton` | 15.0f | **10.0f** | −5.0 |
| `headerStatsButton` | 15.0f | **12.0f** | −3.0 |
| `prefsTitle` | 24.0f | **24.0f** | 0 |
| `prefsSectionHdr` | 14.5f | **14.5f** | 0 |
| `prefsLabel` | 14.5f | **14.5f** | 0 |
| `prefsButton` | 13.0f | **13.0f** | 0 |
| `prefsNote` | 11.5f | **11.5f** | 0 |

**Cascading aliases (changed because their targets changed):**

| Alias | Resolves to | Initial resolved | Current resolved |
|---|---|---:|---:|
| `sidebarMainValue` | `fieldValue` | 14.5f | **9.5f** |
| `sidebarSectionTitle` | `sectionHeader` | 14.5f | **9.0f** |
| `sidebarButtonText` | `button` | 13.5f | **9.0f** |
| `sidebarFieldLabel` | `fieldLabel` | 14.5f | **9.5f** |
| `freqValue` | `sidebarMainValue` | 14.5f | **9.5f** |
| `freqSectionTitle` | `sidebarSectionTitle` | 14.5f | **9.0f** |
| `freqStepGlyph` | `sidebarButtonText` | 13.5f | **9.0f** |

---

## 3. Related look-and-feel font scales (`UiConfig::Laf`)

These are **not** absolute font sizes; they control how fonts fit inside control heights / title shrink.

| Variable | Initial | Current | Notes |
|---|---:|---:|---|
| `buttonHeightScale` | 0.50f | **0.50f** | `min(button, height * this)` |
| `comboHeightScale` | 0.54f | **0.54f** | ComboBox font vs height |
| `toggleHeightScale` | 0.72f | **0.72f** | Toggle / checkbox label crush |
| `titleShrinkMin` | 12.0f | **12.0f** | Header title auto-shrink floor (px) |
| `versionFromTitle` | 0.95f | **0.75f** | Version height = `max(versionMin, titleH * this)` |
| `versionMin` | 16.0f | **10.0f** | Absolute floor for version font |
| `sidebarButtonHeightScale` | 0.58f | **0.58f** | Sidebar / freq `< >` glyph vs row height |
| `freqStepHeightScale` | = `sidebarButtonHeightScale` | **0.58f** | Alias |

---

## 4. `Brand::Type` alias map (exact)

| `Brand::Type::…` | Equals `UiConfig::FontSize::…` | Current px |
|---|---|---:|
| `appTitle` | `appTitle` | 13.0 |
| `sectionHdr` | `sectionHeader` | 9.0 |
| `panelTitle` | `plotTitle` | 9.0 |
| `label` | `fieldLabel` | 9.5 |
| `input` | `fieldValue` | 9.5 |
| `button` | `button` | 9.0 |
| `bottomBarButton` | `bottomBarButton` | 9.0 |
| `status` | `statusBar` | 1.0 |
| `metadata` | `paramChip` | 12.14 |
| `axis` | `plotAxis` | 10.0 |
| `gridNum` | `plotGridNumber` | 10.0 |
| `viewTileCaption` | `viewTileCaption` | 1.0 |
| `bottomSectionTitle` | `bottomSectionTitle` | 7.5 |
| `colourBarTick` | `colourBarTick` | 14.0 |
| `colourBarTitle` | `colourBarTitle` | 15.0 |
| `speakerPolarityBadge` | `speakerPolarityBadge` | 9.0 |
| `speakerId` | `speakerId` | 11.0 |
| `speakerIdSelected` | `speakerIdSelected` | 11.0 |
| `polarLegend` | `polarLegend` | 12.0 |
| `polarLegendMeta` | `polarLegendMeta` | 10.0 |
| `polarEmptyMessage` | `polarEmptyMessage` | 15.0 |
| `exportBrand` | `exportBrand` | 16.0 |
| `exportFrequency` | `exportFrequency` | 18.0 |
| `exportChartTitle` | `exportChartTitle` | 17.0 |
| `exportSubtitle` | `exportSubtitle` | 12.0 |
| `exportPolarRing` | `exportPolarRing` | 11.0 |
| `exportPolarCenter` | `exportPolarCenter` | 13.0 |
| `sidebarMainValue` | `sidebarMainValue` | 9.5 |
| `sidebarSectionTitle` | `sidebarSectionTitle` | 9.0 |
| `sidebarButtonText` | `sidebarButtonText` | 9.0 |
| `sidebarFieldLabel` | `sidebarFieldLabel` | 9.5 |
| `freqValue` | `freqValue` | 9.5 |
| `freqSectionTitle` | `freqSectionTitle` | 9.0 |
| `freqStepGlyph` | `freqStepGlyph` | 9.0 |
| `dashActionButton` | `dashActionButton` | 18.0 |
| `dashSubtitle` | `dashSubtitle` | 19.0 |
| `dashRecentHeader` | `dashRecentHeader` | 18.0 |
| `dashRecentItem` | `dashRecentItem` | 18.0 |
| `plotFitButton` | `plotFitButton` | 10.0 |
| `headerStatsButton` | `headerStatsButton` | 12.0 |
| `prefsTitle` | `prefsTitle` | 24.0 |
| `prefsSectionHdr` | `prefsSectionHdr` | 14.5 |
| `prefsLabel` | `prefsLabel` | 14.5 |
| `prefsButton` | `prefsButton` | 13.0 |
| `prefsNote` | `prefsNote` | 11.5 |

---

## 5. Derived / special cases (not plain config constants)

| Location | Expression / value | Notes |
|---|---|---|
| `MainComponent.cpp` title shrink | starts at `scaledFont(appTitle)` (= **13.0** at scale 1), may shrink by 0.5 until `titleShrinkMin` (**12.0**) | Width-fit loop |
| `MainComponent.cpp` version height | `max(versionMin, titleFontH * versionFromTitle)` → `max(10.0, titleH * 0.75)` | After title shrink |
| `MainComponent.cpp` coming-soon overlay | `sectionHdr + 6.0f` → **9.0 + 6.0 = 15.0** | Hard-coded offset on config size |
| Bottom-bar button height crush | `min(scaledFont(bottomBarButton), buttonHeight * 0.55f)` | Extra 0.55 factor in LookAndFeel |
| Prefs button height crush | `min(scaledFont(prefsButton), buttonHeight * 0.52f)` | Extra 0.52 factor |

---

## 6. Hardcoded font sizes outside `UiTextConfig.h`

These are **literal floats in component code**, not driven by `FontSize` constants. Documented so nothing is missed.

### `DashboardComponent.cpp`

| Exact size | Usage |
|---:|---|
| **26.0f** | Main dashboard title (`title_`) |
| **20.0f** | Form title (`formTitle_`) |
| **14.0f** | Form field labels |
| **15.0f** | Form text editors |

(Other dashboard text uses `Brand::Type::dashSubtitle` / `dashRecentHeader` / `dashRecentItem` / `dashActionButton`.)

### `GraphRender.h` (graph / report-style render helpers)

| Exact size | Usage |
|---:|---|
| **20.0f** | Chart title |
| **12.0f** | Axis / secondary label |
| **11.0f** | Tick / annotation (multiple sites) |
| **10.5f** | Smaller annotation |
| **10.0f** | Bold small label |

### `ReportExport.h` (export sheet overlay)

| Exact size | Usage |
|---:|---|
| **22.0f** | Export title |
| **15.0f** | Subtitle / body (two sites) |
| **12.0f** | Mono footer / meta |
| **11.0f** | Mono small / section label |

---

## 7. Scale reference (affects all fonts at runtime)

From `UiConfig::Scale`:

| Variable | Exact value |
|---|---:|
| `referenceWidth` | 1340 |
| `referenceHeight` | 820 |
| `minFactor` | 1.0f |
| `maxFactor` | 1.85f |

Formula: `factor = clamp(min(windowW/1340, windowH/820), 1.0, 1.85)`  
Rendered font height ≈ `FontSize::* * factor`.

---

## 8. How to edit safely

1. Change only the constant in `Source/UiTextConfig.h` (`UiConfig::FontSize::…`).
2. If editing an alias (`sidebarMainValue`, etc.), prefer changing the **base** (`fieldValue`, `sectionHeader`, `button`, `fieldLabel`) unless you intentionally want to break the alias.
3. Rebuild Release and relaunch.
4. Update this file’s “Current” column if you change sizes again.

---

## 9. Snapshot of current `FontSize` block (copy from source)

```cpp
constexpr float appTitle            = 13.0f;
constexpr float appVersion          = 9.0f;
constexpr float paramChip           = 12.14f;
constexpr float sectionHeader       = 9.0f;
constexpr float fieldLabel          = 9.5f;
constexpr float fieldValue          = 9.5f;
constexpr float infoKey             = 9.5f;
constexpr float plotTitle           = 9.0f;
constexpr float plotGridNumber      = 10.0f;
constexpr float plotAxis            = 10.0f;
constexpr float colourBarTick       = 14.0f;
constexpr float colourBarTitle      = 15.0f;
constexpr float speakerPolarityBadge = 9.0f;
constexpr float speakerId             = 11.0f;
constexpr float speakerIdSelected     = 11.0f;
constexpr float button              = 9.0f;
constexpr float bottomBarButton       = 9.0f;
constexpr float viewTileCaption       = 1.0f;
constexpr float statusBar             = 1.0f;
constexpr float bottomSectionTitle    = 7.5f;
constexpr float polarLegend           = 12.0f;
constexpr float polarLegendMeta       = 10.0f;
constexpr float polarEmptyMessage     = 15.0f;
constexpr float exportBrand         = 16.0f;
constexpr float exportFrequency     = 18.0f;
constexpr float exportChartTitle    = 17.0f;
constexpr float exportSubtitle      = 12.0f;
constexpr float exportPolarRing     = 11.0f;
constexpr float exportPolarCenter   = 13.0f;
constexpr float sidebarMainValue      = fieldValue;        // → 9.5f
constexpr float sidebarSectionTitle   = sectionHeader;     // → 9.0f
constexpr float sidebarButtonText     = button;            // → 9.0f
constexpr float sidebarFieldLabel     = fieldLabel;        // → 9.5f
constexpr float freqValue             = sidebarMainValue;  // → 9.5f
constexpr float freqSectionTitle      = sidebarSectionTitle; // → 9.0f
constexpr float freqStepGlyph         = sidebarButtonText; // → 9.0f
constexpr float dashActionButton      = 18.0f;
constexpr float dashSubtitle          = 19.0f;
constexpr float dashRecentHeader      = 18.0f;
constexpr float dashRecentItem        = 18.0f;
constexpr float plotFitButton         = 10.0f;
constexpr float headerStatsButton     = 12.0f;
constexpr float prefsTitle            = 24.0f;
constexpr float prefsSectionHdr       = 14.5f;
constexpr float prefsLabel            = 14.5f;
constexpr float prefsButton           = 13.0f;
constexpr float prefsNote             = 11.5f;
```

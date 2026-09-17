#pragma once

// =============================================================================
// UI TEXT & LAYOUT CONTROL PANEL
// =============================================================================
// Change numbers here, rebuild (Release), and the whole app updates.
// This file is constants only — no logic. BrandTheme.h and UI components
// read these values.
// =============================================================================

namespace UiConfig
{
    // -------------------------------------------------------------------------
    // RESPONSIVE SCALE — baseline = default window (1340 x 820, scale 1.0)
    // Smaller MacBook / laptop windows may shrink to minFactor; larger scale up.
    // -------------------------------------------------------------------------
    namespace Scale
    {
        constexpr int   referenceWidth  = 1340;
        constexpr int   referenceHeight = 820;
        constexpr float minFactor       = 0.78f;  // allow shrink on smaller screens
        constexpr float maxFactor       = 1.85f;

        inline float factor = 1.0f;

        inline float compute (int windowW, int windowH)
        {
            const float sx = windowW  / (float) referenceWidth;
            const float sy = windowH / (float) referenceHeight;
            const float raw = sx < sy ? sx : sy;
            if (raw < minFactor) return minFactor;
            if (raw > maxFactor) return maxFactor;
            return raw;
        }

        inline void updateFromWindow (int windowW, int windowH)
        {
            factor = compute (windowW, windowH);
        }

        inline int px (int v)     { return (int) ((float) v * factor + 0.5f); }
        inline float px (float v) { return v * factor; }
    }

    // -------------------------------------------------------------------------
    namespace FontSize
    {
        // --- Top header bar --------------------------------------------------
        // "Atomik Simulation Engine" centred title
        constexpr float appTitle            = 13.0f;
        // Small version / build label beside the title (also used for subtitle ratio)
        constexpr float appVersion          = 9.0f;

        // --- Top parameter strip ---------------------------------------------
        // Chips: "f = 50 Hz", "Grid: 100 x 2", "View: SPL Heatmap", etc. (plain text, no boxes)
        constexpr float paramChip           = 12.14f;   // 13.44 + 35%

        // --- Sidebars (left control + right info) ----------------------------
        // Bumped ~40-45% from the original 9-9.5px mockup baseline: at a 1920x1080
        // desktop the old sizes rendered around 11-12px on screen (base * runtime
        // Scale::factor), which read as illegible. These sizes plus the matching
        // Layout row-height bumps below keep the same compact/CAD look at a size
        // that's actually readable without feeling bulky.
        constexpr float sectionHeader       = 13.0f;
        // Field labels: "X Position (m)", "Grid Resolution", checkbox text, …
        constexpr float fieldLabel          = 13.0f;
        // Slider numeric boxes (mono) and info-panel value column (right side)
        constexpr float fieldValue          = 13.0f;
        // Info-panel row keys (left column) — usually same as fieldLabel
        constexpr float infoKey             = 13.0f;

        // --- Plot chrome -----------------------------------------------------
        // Title above heatmap: "SPL Heatmap | 2 devices | 50 Hz | …"
        constexpr float plotTitle           = 11.5f;
        // Axis tick numbers along plot edges (0 m, 10 m, 20 m …)
        constexpr float plotGridNumber      = 11.5f;
        // Secondary plot axis / annotation text
        constexpr float plotAxis            = 11.5f;
        // Colour-bar dB tick labels and "Rel. SPL" caption
        constexpr float colourBarTick       = 14.0f;   // export sheets (unscaled)
        constexpr float colourBarTitle      = 15.0f;   // export sheets (unscaled)
        // On-screen Rel. SPL legend. Figma renders these at 14px SemiBold;
        // calibrated against its ink width and put through the window scale so
        // they track window size (the export values above deliberately don't).
        constexpr float legendTickOnScreen  = 11.0f;

        // --- Heatmap speaker markers -----------------------------------------
        // +/- polarity badge inside speaker icon
        constexpr float speakerPolarityBadge = 9.0f;
        // "Q21S-1" label above cabinet (mono)
        constexpr float speakerId             = 11.0f;
        constexpr float speakerIdSelected     = 11.0f;

        // --- Bottom bar (v1.1 screenshot baseline) ---------------------------
        // General TextButton captions elsewhere in the app
        constexpr float button              = 12.5f;
        // Bottom-bar Export / View Mode pill captions (compact v1.1)
        constexpr float bottomBarButton       = 11.5f;
        // View tile captions (legacy icon tiles)
        constexpr float viewTileCaption       = 11.0f;
        // Status strip: "Ready", "Last run: …", "Elapsed: …"
        // Bottom strip "Last run : …" / "Elapsed : …" and the ribbon's Ready
        // pill. Calibrated against the Figma render's ink width (see the
        // sidebar note above for why this isn't just the CSS px value).
        constexpr float statusBar             = 18.0f;
        // "SAVE / EXPORT" / "VIEW MODE" section titles
        constexpr float bottomSectionTitle    = 9.5f;

        // --- Directivity / measured polar view (on-screen legend) ------------
        constexpr float polarLegend           = 12.0f;
        constexpr float polarLegendMeta       = 10.0f;
        constexpr float polarEmptyMessage     = 15.0f;

        // --- PNG/SVG export sheet chrome (polar export overlays only) --------
        constexpr float exportBrand         = 18.0f;
        constexpr float exportFrequency     = 20.0f;
        constexpr float exportChartTitle    = 19.0f;
        constexpr float exportSubtitle      = 14.0f;
        constexpr float exportPolarRing     = 13.0f;   // mono — dB rings & angle labels
        constexpr float exportPolarCenter   = 15.0f;

        // --- Left control sidebar -------------------------------------------
        // Figma uses Montserrat 14px Medium for the numbered section headers
        // and 12px Regular for every field label, value and button. These
        // bases are calibrated by measuring rendered ink width against the
        // Figma render, not by dividing the CSS px value — JUCE's Font height
        // is the whole line box, so matching ink needs a larger number.
        constexpr float sidebarMainValue      = 14.3f;  // -> 12px ink  values / combo text
        constexpr float sidebarSectionTitle   = 19.1f;  // -> 14px ink  "1. FREQUENCY (Hz)"
        constexpr float sidebarButtonText     = 14.3f;  // -> 12px ink  + Add / Delete
        constexpr float sidebarFieldLabel     = 14.3f;  // -> 12px ink  labels, checkbox text

        // Aliases (frequency card uses the same sidebar scale)
        constexpr float freqValue             = sidebarMainValue;
        constexpr float freqSectionTitle      = sidebarSectionTitle;
        constexpr float freqStepGlyph         = sidebarButtonText;

        // --- Project Dashboard (startup screen) -------------------------------
        // This window is much smaller than the main editor's 1340x820 reference,
        // so its Scale::factor is clamped to minFactor (0.78) — these bases are
        // picked so the RENDERED size (base * 0.78) is actually legible.
        // "NEW PROJECT" / "OPEN EXISTING PROJECT" big action buttons
        constexpr float dashActionButton      = 22.0f;
        // "Project Dashboard" subtitle under the title
        constexpr float dashSubtitle          = 23.0f;
        // "RECENT PROJECTS" section header
        constexpr float dashRecentHeader      = 23.0f;
        // Recent project list rows ("name - path") + "No recent projects yet."
        constexpr float dashRecentItem        = 22.0f;

        // --- Plot toolbar ----------------------------------------------------
        // "Fit View" button above the heatmap (bold)
        constexpr float plotFitButton         = 12.5f;
        // Header "Statistics" dropdown button (Montserrat SemiBold)
        constexpr float headerStatsButton     = 12.0f;
        // Small toolbar labels: "Opacity", "Gap", and the drawing-tool status
        // prompt ("LINE: click the plot…"). Were hardcoded literals (10px, no
        // scale) — named + scaled here so they track the legibility pass above.
        constexpr float plotToolbarLabel      = 12.5f;
        // Ribbon cluster captions ("File", "Navigation", ...). Calibrated by
        // measuring rendered ink against the Figma render rather than by
        // converting a CSS px value — JUCE's Font height is the whole line box,
        // so it needs a larger number than the design's nominal font-size to
        // put the same amount of ink on screen.
        constexpr float ribbonClusterLabel    = 15.0f;

        // --- Preferences dialog ----------------------------------------------
        constexpr float prefsTitle            = 24.0f;
        constexpr float prefsSectionHdr       = 14.5f;
        constexpr float prefsLabel            = 14.5f;
        constexpr float prefsButton           = 13.0f;
        constexpr float prefsNote             = 11.5f;
    }

    // -------------------------------------------------------------------------
    // LOOK-AND-FEEL SCALING — how fonts fit inside control heights
    // -------------------------------------------------------------------------
    namespace Laf
    {
        // TextButton: font = min(FontSize::button, buttonHeight * this)
        constexpr float buttonHeightScale   = 0.50f;
        // ComboBox dropdown
        constexpr float comboHeightScale    = 0.54f;
        // ToggleButton / checkbox label — use full fieldLabel on sidebar (no crush)
        constexpr float toggleHeightScale   = 0.72f;
        // Header title auto-shrink when window is narrow (minimum px)
        constexpr float titleShrinkMin      = 12.0f;
        // Version label stays visually secondary to the product name.
        constexpr float versionFromTitle    = 0.75f;
        constexpr float versionMin          = 10.0f;
        // Frequency < > buttons: glyph height vs button row (all sidebar buttons)
        constexpr float sidebarButtonHeightScale = 0.58f;
        constexpr float freqStepHeightScale      = sidebarButtonHeightScale;
    }

    // -------------------------------------------------------------------------
    // LAYOUT (pixels) — panel widths, row heights, header chrome
    // -------------------------------------------------------------------------
    namespace Layout
    {
        // Left sidebar — widened + row heights raised to match the larger
        // FontSize::sidebar* values above (legibility pass); same layout shape,
        // just enough room for 13px text instead of 9-9.5px.
        // Figma "Home Screen 1.1": sidebar is 340px of a 1920px canvas, i.e.
        // 258 through the 1.317 scale factor the app derives at that size.
        constexpr int sidebarWidth          = 258;  // -> 340px
        constexpr int sidebarCollapsedWidth = 32;   // rail when sidebar is collapsed
        // Sidebar metrics, all Figma pixels / 1.317 (see 02-layout-left-panel.md):
        //   12px content gutter · 36px input rows · 24px section headers ·
        //   ~48px slider-row pitch · 28.8px checkboxes · 101px value boxes.
        constexpr int sidebarPadding        = 9;    // -> 12px content gutter
        // Right info panel (Scene Summary, Selected Speaker)
        constexpr int infoPanelWidth          = 264;
        constexpr int controlRowHeight        = 27;   // -> 36px  combo / device / action rows
        constexpr int controlRowGap           = 9;    // -> 12px  between rows in a section
        constexpr int sectionGap              = 11;   // -> 14px  last row -> next header
        constexpr int labelColumnWidth        = 74;   // -> 97px  (label + gap = 109px)

        constexpr int sidebarRowHeight          = controlRowHeight;
        constexpr int sidebarStepButtonWidth    = 30;
        constexpr int sidebarActionButtonWidth  = 77;   // -> 101px  + Add / Delete
        constexpr int sidebarPrimaryButtonH       = 27;
        constexpr int sidebarResetRowH            = 22;   // -> 29px  Set to Default / Clear All
        constexpr int sidebarBorderWidth          = 1;
        constexpr int sidebarSliderBoxWidth     = 77;   // -> 101px  value box
        constexpr int sidebarSliderBoxHeight    = 27;   // -> 36px
        constexpr int sidebarEditLabelHeight    = 20;
        constexpr int sidebarSectionHeaderH     = 18;   // -> 24px
        constexpr int sidebarHelperTextH        = 16;
        constexpr int sidebarSectionHeaderGap   = 7;    // -> 9px  header -> first control
        constexpr int sidebarSliderTextGap      = 6;    // -> 8px  track → value box

        // Legacy names
        constexpr int freqRowHeight             = sidebarRowHeight;
        constexpr int freqStepButtonWidth       = sidebarStepButtonWidth;

        // Top param chip strip height
        constexpr int paramBarHeight          = 36;
        // Tool ribbon (Figma "Home Screen 1.1" row 2). Every ribbon constant
        // below is the Figma 1920x1080 pixel value divided by the 1.317 scale
        // factor the app derives at that window size, so at 1920x1080 they
        // render at exactly the documented Figma pixel sizes.
        constexpr int plotHeaderHeight        = 56;   // -> 74px (Figma row 2)
        constexpr int ribbonIconSize          = 18;   // -> 24px icon buttons
        constexpr int ribbonIconGap           = 5;    // -> 6px  (30px pitch)
        constexpr int ribbonIconTop           = 8;    // -> 11px below row top
        constexpr int ribbonLabelTop          = 32;   // -> 42px below row top
        constexpr int ribbonLabelH            = 16;   // -> 21px label row
        constexpr int ribbonEdgePad           = 15;   // -> 20px left margin
        constexpr int ribbonClusterPad        = 9;    // -> 12px each side of a divider
        constexpr int ribbonSwatch            = 11;   // -> 14px colour dot
        constexpr int ribbonSwatchPitchX      = 15;   // -> 20px
        constexpr int ribbonSwatchPitchY      = 12;   // -> 16px
        constexpr int ribbonReadyRightPad     = 45;   // -> 60px right margin
        // Main window title band (logo + centred title)
        constexpr int headerBandHeight        = 44;
        // ATOMIK wordmark inside the header. Was 14px — at the low end of the
        // window-size scale range that rendered the wordmark's fine strokes
        // as an illegible smudge. Raised to use more of the available
        // headerBandHeight (44, minus padY top+bottom) while still leaving
        // clear vertical padding.
        constexpr int headerLogoMaxHeight     = 22;
        constexpr int headerLogoPadX          = 18;
        constexpr int headerLogoPadY          = 10;

        // Help / Settings / More icon buttons (top-right)
        constexpr int headerIconWidth         = 36;
        constexpr int headerIconHeight        = 32;
        constexpr int headerIconEdgeIndent    = 4;

        // Bottom strip (Figma: y=979..1080 under the canvas, x=340..1920).
        // Run info stacked bottom-left, Save/Export buttons bottom-right.
        // Figma pixels / 1.317, as with the ribbon constants above.
        constexpr int bottomPanelHeight       = 77;   // -> 101px
        constexpr int statusStripHeight       = 0;    // Figma has no full-width status strip
        constexpr int bottomRunInfoLeft       = 13;   // -> 17px text inset from canvas left
        constexpr int bottomRunInfoTop        = 8;    // -> 11px first line below strip top
        constexpr int bottomRunInfoLineGap    = 18;   // -> 24px between the two lines
        constexpr int bottomButtonWidth       = 150;  // -> 197px
        constexpr int bottomButtonHeight      = 23;   // -> 30px
        constexpr int bottomButtonGap         = 4;    // -> 5px
        constexpr int bottomButtonRightPad    = 25;   // -> 33px
        constexpr int bottomButtonBaseline    = 20;   // -> 26px button top below strip top
        constexpr int bottomExportWidth       = 180;  // SAVE / EXPORT column (narrower)
        constexpr int bottomSectionGap        = 16;   // gap Export | View Mode block
        constexpr int bottomViewColGap        = 8;    // gap between the two View Mode columns
        constexpr int bottomExportBtnGap      = 4;    // tight vertical stack (v1.1)
        constexpr int bottomViewBtnGap        = 4;
        constexpr int bottomSectionHeaderH    = 17;
        constexpr int bottomContentTopPad     = 3;    // header -> first button row

        // Slider numeric readout box (default / non-sidebar)
        constexpr int sliderBoxWidth          = 72;
        constexpr int sliderBoxHeight         = 28;

        // Preferences overlay (centred modal)
        constexpr int prefsPanelWidth         = 440;
        constexpr int prefsPanelHeight        = 390;
        constexpr int prefsPadding            = 22;
        constexpr int prefsTitleRowH          = 32;
        constexpr int prefsSectionHeaderH     = 22;
        constexpr int prefsRowH               = 32;
        constexpr int prefsLabelColW          = 130;
        constexpr int prefsSegBtnW            = 120;
        constexpr int prefsNoteRowH           = 36;
        constexpr int prefsCloseBtnW          = 120;
        constexpr int prefsCloseBtnH          = 32;
    }

    // -------------------------------------------------------------------------
    // CONTROL CHROME — sliders, checkboxes (not text size)
    // -------------------------------------------------------------------------
    namespace Control
    {
        constexpr float sliderTrackThickness = 2.0f;
        constexpr float sliderThumbDiameter  = 10.0f;
        constexpr float checkboxTickSize     = 18.0f;
        constexpr float cornerRadius         = 2.0f;   // Figma redesign: flat 2px radius
        constexpr float cardCornerRadius     = 2.0f;   // Figma redesign: flat 2px radius
        // Sidebar collapse chevrons, combo dropdown triangles, checkbox ticks
        constexpr float sidebarChevronScale    = 1.0f;
        constexpr float sidebarComboArrowScale = 1.0f;
        constexpr float sidebarTickScale       = 1.0f;
        // Legacy names
        constexpr float freqChevronScale     = sidebarChevronScale;
        constexpr float freqComboArrowScale  = sidebarComboArrowScale;
    }
}

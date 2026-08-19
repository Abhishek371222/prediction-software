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
        // "Atomik Acoustic Simulation Engine" centred title
        constexpr float appTitle            = 13.0f;
        // Small version / build label beside the title (also used for subtitle ratio)
        constexpr float appVersion          = 9.0f;

        // --- Top parameter strip ---------------------------------------------
        // Chips: "f = 50 Hz", "Grid: 100 x 2", "View: SPL Heatmap", etc. (plain text, no boxes)
        constexpr float paramChip           = 12.14f;   // 13.44 + 35%

        // --- Sidebars (left control + right info) ----------------------------
        // All sidebar field / checkbox labels match Selected-Q21S mockup weight
        constexpr float sectionHeader       = 9.0f;
        // Field labels: "X Position (m)", "Grid Resolution", checkbox text, …
        constexpr float fieldLabel          = 9.5f;
        // Slider numeric boxes (mono) and info-panel value column (right side)
        constexpr float fieldValue          = 9.5f;
        // Info-panel row keys (left column) — usually same as fieldLabel
        constexpr float infoKey             = 9.5f;

        // --- Plot chrome -----------------------------------------------------
        // Title above heatmap: "SPL Heatmap | 2 devices | 50 Hz | …"
        constexpr float plotTitle           = 9.0f;
        // Axis tick numbers along plot edges (0 m, 10 m, 20 m …)
        constexpr float plotGridNumber      = 10.0f;
        // Secondary plot axis / annotation text
        constexpr float plotAxis            = 10.0f;
        // Colour-bar dB tick labels and "Rel. SPL" caption
        constexpr float colourBarTick       = 14.0f;
        constexpr float colourBarTitle      = 15.0f;

        // --- Heatmap speaker markers -----------------------------------------
        // +/- polarity badge inside speaker icon
        constexpr float speakerPolarityBadge = 9.0f;
        // "Q21S-1" label above cabinet (mono)
        constexpr float speakerId             = 11.0f;
        constexpr float speakerIdSelected     = 11.0f;

        // --- Bottom bar (v1.1 screenshot baseline) ---------------------------
        // General TextButton captions elsewhere in the app
        constexpr float button              = 9.0f;
        // Bottom-bar Export / View Mode pill captions (compact v1.1)
        constexpr float bottomBarButton       = 9.0f;
        // View tile captions (legacy icon tiles)
        constexpr float viewTileCaption       = 9.0f;
        // Status strip: "Ready", "Last run: …", "Elapsed: …"
        constexpr float statusBar             = 11.0f;
        // "SAVE / EXPORT" / "VIEW MODE" section titles
        constexpr float bottomSectionTitle    = 7.5f;

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

        // --- Left control sidebar (whole pane — v1.1 baseline, no +30/+35% bump)
        // MAIN: combo values + slider numeric boxes
        constexpr float sidebarMainValue      = fieldValue;        // 15.0
        // REST: collapsible section titles
        constexpr float sidebarSectionTitle   = sectionHeader;     // 16.0
        // REST: buttons, < > steppers
        constexpr float sidebarButtonText     = button;            // 13.0
        // REST: field labels, helper lines, checkbox text
        constexpr float sidebarFieldLabel     = fieldLabel;        // 14.0

        // Aliases (frequency card uses the same sidebar scale)
        constexpr float freqValue             = sidebarMainValue;
        constexpr float freqSectionTitle      = sidebarSectionTitle;
        constexpr float freqStepGlyph         = sidebarButtonText;

        // --- Project Dashboard (startup screen) ------------------------------
        // "NEW PROJECT" / "OPEN EXISTING PROJECT" big action buttons
        constexpr float dashActionButton      = 18.0f;
        // "Project Dashboard" subtitle under the title
        constexpr float dashSubtitle          = 19.0f;
        // "RECENT PROJECTS" section header
        constexpr float dashRecentHeader      = 18.0f;
        // Recent project list rows ("name - path") + "No recent projects yet."
        constexpr float dashRecentItem        = 18.0f;

        // --- Plot toolbar ----------------------------------------------------
        // "Fit" button above the heatmap (bold)
        constexpr float plotFitButton         = 10.0f;
        // Header "Stats" dropdown button (Montserrat SemiBold)
        constexpr float headerStatsButton     = 12.0f;

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
        // Left sidebar — measured from mockup (244px panel, ~33px slider row pitch)
        constexpr int sidebarWidth          = 252;
        constexpr int sidebarCollapsedWidth = 32;   // rail when sidebar is collapsed
        constexpr int sidebarPadding        = 14;
        // Right info panel (Scene Summary, Selected Speaker)
        constexpr int infoPanelWidth          = 248;
        constexpr int controlRowHeight        = 27;   // combo / device / action rows
        constexpr int controlRowGap           = 5;
        constexpr int sectionGap              = 10;
        constexpr int labelColumnWidth        = 96;

        constexpr int sidebarRowHeight          = controlRowHeight;
        constexpr int sidebarStepButtonWidth    = 30;
        constexpr int sidebarActionButtonWidth  = 54;   // + Add / Delete
        constexpr int sidebarPrimaryButtonH       = 27;
        constexpr int sidebarResetRowH            = 24;
        constexpr int sidebarBorderWidth          = 1;
        constexpr int sidebarSliderBoxWidth     = 68;
        constexpr int sidebarSliderBoxHeight    = 21;
        constexpr int sidebarEditLabelHeight    = 16;
        constexpr int sidebarSectionHeaderH     = 16;
        constexpr int sidebarHelperTextH        = 13;
        constexpr int sidebarSectionHeaderGap   = 4;
        constexpr int sidebarSliderTextGap      = 6;    // track → value box

        // Legacy names
        constexpr int freqRowHeight             = sidebarRowHeight;
        constexpr int freqStepButtonWidth       = sidebarStepButtonWidth;

        // Top param chip strip height
        constexpr int paramBarHeight          = 36;
        // Plot title bar above heatmap
        constexpr int plotHeaderHeight        = 44;
        // Main window title band (logo + centred title)
        constexpr int headerBandHeight        = 44;
        // ATOMIK wordmark inside the header (−20% vs prior 18 px baseline)
        constexpr int headerLogoMaxHeight     = 14;
        constexpr int headerLogoPadX          = 18;
        constexpr int headerLogoPadY          = 12;

        // Help / Settings / More icon buttons (top-right)
        constexpr int headerIconWidth         = 36;
        constexpr int headerIconHeight        = 32;
        constexpr int headerIconEdgeIndent    = 4;

        // Bottom export + view toolbar (v1.1 screenshot: compact pills, tight gaps)
        constexpr int bottomPanelHeight       = 84;
        constexpr int statusStripHeight       = 26;
        constexpr int bottomExportWidth       = 168;  // SAVE / EXPORT column (narrower)
        constexpr int bottomSectionGap        = 16;   // gap Export | View Mode block
        constexpr int bottomViewColGap        = 8;    // gap between the two View Mode columns
        constexpr int bottomExportBtnGap      = 3;    // tight vertical stack (v1.1)
        constexpr int bottomViewBtnGap        = 3;
        constexpr int bottomSectionHeaderH    = 14;
        constexpr int bottomContentTopPad     = 2;    // header -> first button row

        // Slider numeric readout box (default / non-sidebar)
        constexpr int sliderBoxWidth          = 72;
        constexpr int sliderBoxHeight         = 28;

        // Preferences overlay (centred modal)
        constexpr int prefsPanelWidth         = 440;
        constexpr int prefsPanelHeight        = 360;
        constexpr int prefsPadding            = 22;
        constexpr int prefsTitleRowH          = 32;
        constexpr int prefsSectionHeaderH     = 22;
        constexpr int prefsRowH               = 32;
        constexpr int prefsLabelColW          = 130;
        constexpr int prefsSegBtnW            = 120;
        constexpr int prefsNoteRowH           = 20;
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
        constexpr float cornerRadius         = 4.0f;
        constexpr float cardCornerRadius     = 8.0f;
        // Sidebar collapse chevrons, combo dropdown triangles, checkbox ticks
        constexpr float sidebarChevronScale    = 1.0f;
        constexpr float sidebarComboArrowScale = 1.0f;
        constexpr float sidebarTickScale       = 1.0f;
        // Legacy names
        constexpr float freqChevronScale     = sidebarChevronScale;
        constexpr float freqComboArrowScale  = sidebarComboArrowScale;
    }
}

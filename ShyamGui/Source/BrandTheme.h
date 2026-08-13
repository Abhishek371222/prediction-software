#pragma once
#include <JuceHeader.h>
#include "AppSettings.h"
#include "UiTextConfig.h"

// ===========================================================================
// Atomik brand theme — central palette, fonts (Montserrat + Space Mono) and a
// LookAndFeel that applies them app-wide.
//
// Brand / chrome tokens (product spec):
//   Primary Red #ED2227 | Pure White #FFFFFF | Soft White #FAFAFA
//   Light Grey  #EFE9E9 | Mid Grey   #DFDEDE | Dark Surface #1C1C1C
//   Dark Button #333131 | Tool Bg    #201F1F
// Theme flips chrome only; SPL heatmap colour stops stay identical.
 // ===========================================================================
namespace Brand
{
    // --- Theme palette ----------------------------------------------------
    struct Palette
    {
        juce::Colour base;          // app background
        juce::Colour panel;         // sidebar
        juce::Colour panelDark;     // plot header strip
        juce::Colour btnIn;         // sidebar controls / value boxes
        juce::Colour exportPill;    // SAVE / EXPORT buttons
        juce::Colour idleViewPill;  // inactive VIEW MODE pills
        juce::Colour statsBtn;      // header Stats pill fill
        juce::Colour statsText;     // ink on Stats pill
        juce::Colour plotToolbar;   // plot tool button fills
        juce::Colour border;        // control outline base
        juce::Colour sidebarBorder;
        juce::Colour plotBorder;
        juce::Colour toolbarBorder;
        juce::Colour fitViewBorder;
        juce::Colour text;          // primary chrome text
        juce::Colour heading;       // section headers
        juce::Colour ash;           // body labels
        juce::Colour muted;         // secondary / helper
        juce::Colour onBtn;         // ink on btnIn / control fills
        juce::Colour plotBg;        // plot canvas
        juce::Colour plotGrid;      // major grid
        juce::Colour plotTitle;     // plot title text
        juce::Colour axisLabel;     // 0 m / 10 m ticks
        juce::Colour activeLabel;   // on-accent / active chrome text
        juce::Colour scrollThumb;   // thin scrollbar pill
    };

    inline Palette darkPalette()
    {
        Palette p;
        p.base          = juce::Colour (0xff1c1c1c);
        p.panel         = juce::Colour (0xff1c1c1c);
        p.panelDark     = juce::Colour (0xff201f1f);
        p.btnIn         = juce::Colour (0xffffffff);
        p.exportPill    = juce::Colour (0xffdfdede);
        p.idleViewPill  = juce::Colour (0xffefe9e9);
        p.statsBtn      = juce::Colour (0xffffffff);
        p.statsText     = juce::Colour (0xff000000);
        p.plotToolbar   = juce::Colour (0xff201f1f);
        p.border        = juce::Colour (0xffe6e6e6);   // soft edge on white controls
        p.sidebarBorder = juce::Colour (0x4dffffff);   // #FFFFFF @ 30%
        p.plotBorder    = juce::Colour (0x80ffffff);   // #FFFFFF @ 50%
        p.toolbarBorder = juce::Colour (0xff1f1e1e);   // #1F1E1E
        p.fitViewBorder = juce::Colour (0xff383636);   // #383636
        p.text          = juce::Colour (0xffffffff);
        p.heading       = juce::Colour (0xffffffff);
        p.ash           = juce::Colour (0xfffafafa);
        p.muted         = juce::Colour (0xffb8b8bc);
        p.onBtn         = juce::Colour (0xff000000);
        p.plotBg        = juce::Colour (0xff1c1c1c);
        p.plotGrid      = juce::Colour (0x1affffff);   // #FFFFFF @ ~10% — softer on dark heatmap
        p.plotTitle     = juce::Colour (0xfffafafa);
        p.axisLabel     = juce::Colour (0xffffffff);
        p.activeLabel   = juce::Colour (0xfffffafa);
        p.scrollThumb   = juce::Colour (0xffefe9e9);   // light pill on dark
        return p;
    }

    inline Palette lightPalette()
    {
        Palette p;
        p.base          = juce::Colour (0xffffffff);
        p.panel         = juce::Colour (0xffefe9e9);
        p.panelDark     = juce::Colour (0xffefe9e9);
        p.btnIn         = juce::Colour (0xffdfdede);
        p.exportPill    = juce::Colour (0xffdfdede);
        p.idleViewPill  = juce::Colour (0xffefe9e9);
        p.statsBtn      = juce::Colour (0xff333131);
        p.statsText     = juce::Colour (0xfffef8f8);
        p.plotToolbar   = juce::Colour (0xffdfdede);
        p.border        = juce::Colour (0xffc9c6c6);   // soft edge on #DFDEDE controls
        p.sidebarBorder = juce::Colour (0x801a1919);   // #1A1919 @ 50%
        p.plotBorder    = juce::Colour (0x801a1919);   // #1A1919 @ 50%
        p.toolbarBorder = juce::Colour (0xff1f1e1e);   // #1F1E1E
        p.fitViewBorder = juce::Colour (0xff383636);   // #383636
        p.text          = juce::Colour (0xff000000);
        p.heading       = juce::Colour (0xff000000);
        p.ash           = juce::Colour (0xff000000);
        p.muted         = juce::Colour (0xff333131);
        p.onBtn         = juce::Colour (0xff000000);
        p.plotBg        = juce::Colour (0xffefe9e9);
        p.plotGrid      = juce::Colour (0x66231f20);
        p.plotTitle     = juce::Colour (0xffed2227);
        p.axisLabel     = juce::Colour (0xffffffff);
        p.activeLabel   = juce::Colour (0xfffffafa);
        p.scrollThumb   = juce::Colour (0xff333131);   // dark pill on light
        return p;
    }

    inline Palette& palette()
    {
        static Palette current = darkPalette();
        return current;
    }

    inline void refreshPalette()
    {
        palette() = AppSettings::get().isDark() ? darkPalette() : lightPalette();
    }

    inline juce::Colour charcoal()  { return juce::Colour (0xff231f20); }
    inline juce::Colour white()     { return juce::Colour (0xffffffff); }
    inline juce::Colour red()       { return juce::Colour (0xffed2227); }
    inline juce::Colour accent()    { return red(); }
    inline juce::Colour onAccent()  { return juce::Colour (0xfffffafa); }
    inline juce::Colour success()   { return juce::Colour (0xff46c37b); }
    inline juce::Colour warning()   { return juce::Colour (0xfff0b04f); }
    inline juce::Colour danger()    { return juce::Colour (0xffe95b5b); }
    inline juce::Colour onPlotDim() { return juce::Colour (0xffb8b8bc); }

    inline juce::Colour base()          { return palette().base; }
    inline juce::Colour panel()         { return palette().panel; }
    inline juce::Colour panelDark()     { return palette().panelDark; }
    inline juce::Colour btnIn()         { return palette().btnIn; }
    inline juce::Colour exportPill()    { return palette().exportPill; }
    inline juce::Colour idleViewPill()  { return palette().idleViewPill; }
    inline juce::Colour statsBtn()      { return palette().statsBtn; }
    inline juce::Colour statsText()     { return palette().statsText; }
    inline juce::Colour plotToolbar()   { return palette().plotToolbar; }
    inline juce::Colour onBtnIn()       { return palette().onBtn; }
    inline juce::Colour border()        { return palette().border; }
    inline juce::Colour sidebarBorder() { return palette().sidebarBorder; }
    inline juce::Colour plotBorder()    { return palette().plotBorder; }
    inline juce::Colour toolbarBorder() { return palette().toolbarBorder; }
    inline juce::Colour fitViewBorder() { return palette().fitViewBorder; }
    inline juce::Colour ash()           { return palette().ash; }
    inline juce::Colour muted()         { return palette().muted; }
    inline juce::Colour text()          { return palette().text; }
    inline juce::Colour heading()       { return palette().heading; }
    inline juce::Colour plotBg()        { return palette().plotBg; }
    inline juce::Colour plotGrid()      { return palette().plotGrid; }
    inline juce::Colour plotTitle()     { return palette().plotTitle; }
    inline juce::Colour axisLabel()     { return palette().axisLabel; }
    inline juce::Colour activeLabel()   { return palette().activeLabel; }
    inline juce::Colour scrollThumb()   { return palette().scrollThumb; }
    inline juce::Colour textDim()       { return ash(); }
    inline juce::Colour card()          { return btnIn(); }
    inline juce::Colour disabled()      { return muted(); }

    // Sidebar control outline: thin grey hairline (reads softer than solid black).
    inline juce::Colour controlBorder() { return juce::Colour (0x731a1919); } // #1A1919 @ ~45%
    constexpr float controlBorderPx   = 0.6f;
    constexpr float valueBoxCorner    = 3.0f;   // slider numeric boxes
    constexpr float controlCorner     = 4.0f;   // combos / buttons

    inline void strokeInsideRounded (juce::Graphics& g, juce::Rectangle<float> bounds,
                                     float corner, float strokePx, juce::Colour col,
                                     float alpha = 1.0f)
    {
        if (strokePx <= 0.0f) return;
        // Preserve the colour's own alpha (withAlpha would force opaque black).
        g.setColour (col.withMultipliedAlpha (alpha));
        const float half = strokePx * 0.5f;
        g.drawRoundedRectangle (bounds.reduced (half),
                                juce::jmax (0.0f, corner - half), strokePx);
    }

    // Soft edge kept for non-sidebar chrome.
    inline juce::Colour softEdge (juce::Colour fill)
    {
        return fill.interpolatedWith (juce::Colours::black, 0.12f);
    }
    constexpr float boxStroke = controlBorderPx;
    // Typography — aliases UiTextConfig.h (edit sizes in UiTextConfig.h only).
    namespace Type
    {
        constexpr float appTitle           = UiConfig::FontSize::appTitle;
        constexpr float sectionHdr           = UiConfig::FontSize::sectionHeader;
        constexpr float panelTitle           = UiConfig::FontSize::plotTitle;
        constexpr float label                = UiConfig::FontSize::fieldLabel;
        constexpr float input                = UiConfig::FontSize::fieldValue;
        constexpr float button               = UiConfig::FontSize::button;
        constexpr float bottomBarButton      = UiConfig::FontSize::bottomBarButton;
        constexpr float status               = UiConfig::FontSize::statusBar;
        constexpr float metadata             = UiConfig::FontSize::paramChip;
        constexpr float axis                 = UiConfig::FontSize::plotAxis;
        constexpr float gridNum              = UiConfig::FontSize::plotGridNumber;
        constexpr float viewTileCaption      = UiConfig::FontSize::viewTileCaption;
        constexpr float bottomSectionTitle   = UiConfig::FontSize::bottomSectionTitle;
        constexpr float colourBarTick        = UiConfig::FontSize::colourBarTick;
        constexpr float colourBarTitle       = UiConfig::FontSize::colourBarTitle;
        constexpr float speakerPolarityBadge = UiConfig::FontSize::speakerPolarityBadge;
        constexpr float speakerId            = UiConfig::FontSize::speakerId;
        constexpr float speakerIdSelected    = UiConfig::FontSize::speakerIdSelected;
        constexpr float polarLegend          = UiConfig::FontSize::polarLegend;
        constexpr float polarLegendMeta      = UiConfig::FontSize::polarLegendMeta;
        constexpr float polarEmptyMessage    = UiConfig::FontSize::polarEmptyMessage;
        constexpr float exportBrand          = UiConfig::FontSize::exportBrand;
        constexpr float exportFrequency      = UiConfig::FontSize::exportFrequency;
        constexpr float exportChartTitle     = UiConfig::FontSize::exportChartTitle;
        constexpr float exportSubtitle       = UiConfig::FontSize::exportSubtitle;
        constexpr float exportPolarRing      = UiConfig::FontSize::exportPolarRing;
        constexpr float exportPolarCenter    = UiConfig::FontSize::exportPolarCenter;
        constexpr float sidebarMainValue     = UiConfig::FontSize::sidebarMainValue;
        constexpr float sidebarSectionTitle  = UiConfig::FontSize::sidebarSectionTitle;
        constexpr float sidebarButtonText    = UiConfig::FontSize::sidebarButtonText;
        constexpr float sidebarFieldLabel    = UiConfig::FontSize::sidebarFieldLabel;
        constexpr float freqValue            = UiConfig::FontSize::freqValue;
        constexpr float freqSectionTitle     = UiConfig::FontSize::freqSectionTitle;
        constexpr float freqStepGlyph        = UiConfig::FontSize::freqStepGlyph;
        constexpr float dashActionButton     = UiConfig::FontSize::dashActionButton;
        constexpr float dashSubtitle         = UiConfig::FontSize::dashSubtitle;
        constexpr float dashRecentHeader     = UiConfig::FontSize::dashRecentHeader;
        constexpr float dashRecentItem       = UiConfig::FontSize::dashRecentItem;
        constexpr float plotFitButton      = UiConfig::FontSize::plotFitButton;
        constexpr float headerStatsButton  = UiConfig::FontSize::headerStatsButton;

        // --- Preferences dialog ----------------------------------------------
        constexpr float prefsTitle         = UiConfig::FontSize::prefsTitle;
        constexpr float prefsSectionHdr    = UiConfig::FontSize::prefsSectionHdr;
        constexpr float prefsLabel         = UiConfig::FontSize::prefsLabel;
        constexpr float prefsButton        = UiConfig::FontSize::prefsButton;
        constexpr float prefsNote          = UiConfig::FontSize::prefsNote;
    }

    // Layout metrics — scaled at runtime from UiTextConfig baseline (1340 x 820).
    namespace UI
    {
        inline float scale = 1.0f;

        inline int panelPad = UiConfig::Layout::sidebarPadding;
        inline int cardPad  = 12;
        inline int rowH     = UiConfig::Layout::controlRowHeight;
        inline int rowGap   = UiConfig::Layout::controlRowGap;
        inline int sectionGap = UiConfig::Layout::sectionGap;
        inline int groupGap   = 20;
        inline int labelColW  = UiConfig::Layout::labelColumnWidth;
        inline int btnGap     = 5;
        inline int sliderBoxW = UiConfig::Layout::sliderBoxWidth;
        inline int sliderBoxH = UiConfig::Layout::sliderBoxHeight;
        inline int sidebarW   = UiConfig::Layout::sidebarWidth;
        inline int sidebarCollapsedW = UiConfig::Layout::sidebarCollapsedWidth;
        inline int infoPanelW = UiConfig::Layout::infoPanelWidth;
        inline int paramBarH  = UiConfig::Layout::paramBarHeight;
        inline int plotHeaderH = UiConfig::Layout::plotHeaderHeight;
        inline int headerBandH = UiConfig::Layout::headerBandHeight;
        inline int headerIconW = UiConfig::Layout::headerIconWidth;
        inline int headerIconH = UiConfig::Layout::headerIconHeight;
        inline int headerIconIndent = UiConfig::Layout::headerIconEdgeIndent;
        inline int bottomPanelH = UiConfig::Layout::bottomPanelHeight;
        inline int statusStripH = UiConfig::Layout::statusStripHeight;

        // Bottom toolbar (scaled copies for layout code)
        inline int bottomExportWidth   = UiConfig::Layout::bottomExportWidth;
        inline int bottomSectionGap    = UiConfig::Layout::bottomSectionGap;
        inline int bottomViewColGap    = UiConfig::Layout::bottomViewColGap;
        inline int bottomExportBtnGap  = UiConfig::Layout::bottomExportBtnGap;
        inline int bottomSectionHeaderH = UiConfig::Layout::bottomSectionHeaderH;
        inline int bottomContentTopPad = UiConfig::Layout::bottomContentTopPad;

        // Left sidebar extras
        inline int sidebarSliderBoxW  = UiConfig::Layout::sidebarSliderBoxWidth;
        inline int sidebarSliderBoxH  = UiConfig::Layout::sidebarSliderBoxHeight;
        inline int sidebarSectionHeaderH = UiConfig::Layout::sidebarSectionHeaderH;
        inline int sidebarSectionHeaderGap = UiConfig::Layout::sidebarSectionHeaderGap;
        inline int sidebarActionButtonW = UiConfig::Layout::sidebarActionButtonWidth;
        inline int sidebarHelperTextH = UiConfig::Layout::sidebarHelperTextH;
        inline int sidebarPrimaryButtonH = UiConfig::Layout::sidebarPrimaryButtonH;
        inline int sidebarResetRowH = UiConfig::Layout::sidebarResetRowH;
        inline int sidebarBorderW = UiConfig::Layout::sidebarBorderWidth;

        inline float cornerRadius   = UiConfig::Control::cornerRadius;
        inline float cardRadius     = UiConfig::Control::cardCornerRadius;
        inline float trackThickness = UiConfig::Control::sliderTrackThickness;
        inline float thumbDiameter  = UiConfig::Control::sliderThumbDiameter;
        inline float tickSize       = UiConfig::Control::checkboxTickSize;

        inline void applyWindowScale (int windowW, int windowH)
        {
            UiConfig::Scale::updateFromWindow (windowW, windowH);
            scale = UiConfig::Scale::factor;
            const auto px = [] (int v) { return UiConfig::Scale::px (v); };
            const auto pxf = [] (float v) { return UiConfig::Scale::px (v); };

            panelPad   = px (UiConfig::Layout::sidebarPadding);
            cardPad    = px (12);
            rowH       = px (UiConfig::Layout::controlRowHeight);
            rowGap     = px (UiConfig::Layout::controlRowGap);
            sectionGap = px (UiConfig::Layout::sectionGap);
            groupGap   = px (20);
            labelColW  = px (UiConfig::Layout::labelColumnWidth);
            btnGap     = px (5);
            sliderBoxW = px (UiConfig::Layout::sliderBoxWidth);
            sliderBoxH = px (UiConfig::Layout::sliderBoxHeight);
            sidebarW   = px (UiConfig::Layout::sidebarWidth);
            sidebarCollapsedW = px (UiConfig::Layout::sidebarCollapsedWidth);
            infoPanelW = px (UiConfig::Layout::infoPanelWidth);
            paramBarH  = px (UiConfig::Layout::paramBarHeight);
            plotHeaderH = px (UiConfig::Layout::plotHeaderHeight);
            headerBandH = px (UiConfig::Layout::headerBandHeight);
            headerIconW = px (UiConfig::Layout::headerIconWidth);
            headerIconH = px (UiConfig::Layout::headerIconHeight);
            headerIconIndent = px (UiConfig::Layout::headerIconEdgeIndent);
            bottomPanelH = px (UiConfig::Layout::bottomPanelHeight);
            statusStripH = px (UiConfig::Layout::statusStripHeight);

            bottomExportWidth    = px (UiConfig::Layout::bottomExportWidth);
            bottomSectionGap     = px (UiConfig::Layout::bottomSectionGap);
            bottomViewColGap     = px (UiConfig::Layout::bottomViewColGap);
            bottomExportBtnGap   = px (UiConfig::Layout::bottomExportBtnGap);
            bottomSectionHeaderH = px (UiConfig::Layout::bottomSectionHeaderH);
            bottomContentTopPad  = px (UiConfig::Layout::bottomContentTopPad);

            sidebarSliderBoxW  = px (UiConfig::Layout::sidebarSliderBoxWidth);
            sidebarSliderBoxH  = px (UiConfig::Layout::sidebarSliderBoxHeight);
            sidebarSectionHeaderH = px (UiConfig::Layout::sidebarSectionHeaderH);
            sidebarSectionHeaderGap = px (UiConfig::Layout::sidebarSectionHeaderGap);
            sidebarActionButtonW = px (UiConfig::Layout::sidebarActionButtonWidth);
            sidebarHelperTextH = px (UiConfig::Layout::sidebarHelperTextH);
            sidebarPrimaryButtonH = px (UiConfig::Layout::sidebarPrimaryButtonH);
            sidebarResetRowH = px (UiConfig::Layout::sidebarResetRowH);
            sidebarBorderW = juce::jmax (1, px (UiConfig::Layout::sidebarBorderWidth));

            cornerRadius   = pxf (UiConfig::Control::cornerRadius);
            cardRadius     = pxf (UiConfig::Control::cardCornerRadius);
            trackThickness = pxf (UiConfig::Control::sliderTrackThickness);
            thumbDiameter  = pxf (UiConfig::Control::sliderThumbDiameter);
            tickSize       = pxf (UiConfig::Control::checkboxTickSize);
        }

        inline float scaledFont (float basePx) { return basePx * scale; }
    }
    // Resolve bundled Assets portably (walk up from CWD / exe — works on macOS).
    inline juce::File resolveProjectChild (const juce::String& rel,
                                           const juce::File& legacyDevPath)
    {
        if (legacyDevPath.isDirectory()) return legacyDevPath;

        auto searchFrom = [&] (juce::File root) -> juce::File
        {
            for (int depth = 0; depth < 14; ++depth)
            {
                const juce::File cand = root.getChildFile (rel);
                if (cand.isDirectory()) return cand;
               #if JUCE_MAC
                const juce::File resources = root.getChildFile ("Resources");
                if (resources.isDirectory())
                {
                    const juce::File fromRes = resources.getChildFile (rel);
                    if (fromRes.isDirectory()) return fromRes;
                }
               #endif
                const juce::File parent = root.getParentDirectory();
                if (parent == root) break;
                root = parent;
            }
            return {};
        };

        if (auto f = searchFrom (juce::File::getCurrentWorkingDirectory()); f != juce::File())
            return f;

        const juce::File exeDir = juce::File::getSpecialLocation (
                                     juce::File::currentExecutableFile)
                                     .getParentDirectory();
        if (auto f = searchFrom (exeDir); f != juce::File())
            return f;

        return legacyDevPath;
    }

    inline juce::File assetsFolder()
    {
        return resolveProjectChild ("Assets", juce::File ("D:\\shayam gui\\Assets"));
    }

    inline juce::File fontsFolder()
    {
        const auto underAssets = assetsFolder().getChildFile ("Fonts");
        if (underAssets.isDirectory()) return underAssets;
        return resolveProjectChild ("Assets/Fonts",
                                    juce::File ("D:\\shayam gui\\Assets\\Fonts"));
    }

    inline juce::Typeface::Ptr loadFace (const juce::String& fileName)
    {
        const auto f = fontsFolder().getChildFile (fileName);
        if (! f.existsAsFile()) return nullptr;
        juce::MemoryBlock mb;
        if (! f.loadFileAsData (mb)) return nullptr;
        return juce::Typeface::createSystemTypefaceFor (mb.getData(), mb.getSize());
    }

    // Logical names used when building Font objects below.
    inline const char* monoName() { return "Space Mono"; }

    // Montserrat weight helpers — LookAndFeel maps these names to bundled TTFs.
    inline juce::Font tech (float height, bool bold = false)   // Regular (or Bold if flagged)
    {
        if (bold)
            return juce::Font ("Montserrat Bold", height, juce::Font::bold);
        return juce::Font ("Montserrat", height, juce::Font::plain);
    }
    inline juce::Font techMed (float height)                   // Medium
    {
        return juce::Font ("Montserrat Medium", height, juce::Font::plain);
    }
    inline juce::Font techSemi (float height)                  // SemiBold
    {
        return juce::Font ("Montserrat SemiBold", height, juce::Font::plain);
    }
    inline juce::Font techBold (float height)                  // Bold
    {
        return juce::Font ("Montserrat Bold", height, juce::Font::bold);
    }
    // Slider value boxes: Regular + 20% letter spacing.
    inline juce::Font sliderNumber (float height)
    {
        return tech (height).withExtraKerningFactor (0.20f);
    }
    inline juce::Font mono (float height, bool bold = false)
    {
        return juce::Font (monoName(), height, bold ? juce::Font::bold : juce::Font::plain);
    }

    // --- Logo ---------------------------------------------------------------
    // Asset SVGs wrap a PNG (data URI). Decode that raster for reliable drawing.
    inline juce::Image decodeEmbeddedPngFromSvg (const juce::File& svg)
    {
        if (! svg.existsAsFile())
            return {};

        const auto text = svg.loadFileAsString();
        constexpr char kMark[] = "base64,";
        const int at = text.indexOfIgnoreCase (kMark);
        if (at < 0)
            return {};

        int i = at + (int) (sizeof (kMark) - 1);
        while (i < text.length() && juce::CharacterFunctions::isWhitespace (text[i]))
            ++i;

        const int end = text.indexOfChar (i, '"');
        if (end <= i)
            return {};

        juce::MemoryOutputStream raw;
        if (! juce::Base64::convertFromBase64 (raw, text.substring (i, end)))
            return {};

        return juce::ImageFileFormat::loadFrom (raw.getData(), raw.getDataSize());
    }

    // ATOMIK-only wordmark aspect (from cropped brand assets).
    constexpr float logoAspect = 1926.0f / 308.0f;

    // whiteVariant: dark-mode / dark-tile logo. Otherwise light-mode logo.
    inline juce::Image loadBrandLogoImage (bool whiteVariant)
    {
        const auto assets = assetsFolder();

        // Preferred: ATOMIK-only crops (no "AUDIO" line).
        const juce::String atomikOnly = whiteVariant
            ? "Atomik_Logo_Dark.png"
            : "Atomik_Logo_Light.png";
        if (auto img = juce::ImageFileFormat::loadFrom (assets.getChildFile (atomikOnly));
            img.isValid())
            return img;

        const juce::String svgName = whiteVariant
            ? "Atomik Audio - Horizontal logo ( White) 3.svg"
            : "Atomik Audio - Horizontal logo 1.svg";

        if (auto img = decodeEmbeddedPngFromSvg (assets.getChildFile (svgName)); img.isValid())
            return img;

        const juce::String pngName = whiteVariant
            ? "Atomik_Audio_Logo_Dark.png"
            : "Atomik_Audio_Logo_Light.png";
        const auto png = assets.getChildFile (pngName);
        if (png.existsAsFile())
            return juce::ImageFileFormat::loadFrom (png);

        return {};
    }

    // Wordmark drawable. Light ink colour → white logo; dark ink → black logo
    // (matches Brand::text() on each theme, and Brand::white() for exports/icons).
    inline std::unique_ptr<juce::Drawable> createLogo (juce::Colour colour = juce::Colours::white)
    {
        const bool whiteVariant = colour.getPerceivedBrightness() >= 0.5f;
        auto img = loadBrandLogoImage (whiteVariant);
        if (! img.isValid())
            return {};

        auto d = std::make_unique<juce::DrawableImage>();
        d->setImage (img);
        return d;
    }

    // Pixel box for the header wordmark: vertically centred, left-aligned,
    // height tracks the scaled header so it stays crisp on every screen size.
    inline juce::Rectangle<float> headerLogoBounds (float headerBandH)
    {
        const float padX = (float) UiConfig::Scale::px (UiConfig::Layout::headerLogoPadX);
        const float padY = (float) UiConfig::Scale::px (UiConfig::Layout::headerLogoPadY);
        const float maxH = (float) UiConfig::Scale::px (UiConfig::Layout::headerLogoMaxHeight);
        // Floor tracks scaled max so small windows stay readable without clipping.
        const float minH = juce::jmax (10.0f, maxH * 0.85f);
        const float h = juce::jlimit (minH, maxH, headerBandH - 2.0f * padY);
        const float w = h * logoAspect;
        return { padX, 0.5f * (headerBandH - h), w, h };
    }

    // Right edge reserved for the logo when centering the window title.
    inline int headerLogoRightReserve()
    {
        const auto b = headerLogoBounds ((float) UI::headerBandH);
        return juce::roundToInt (b.getRight()) + UiConfig::Scale::px (12);
    }

    inline void drawLogo (juce::Graphics& g, juce::Drawable* logo, juce::Rectangle<float> bounds)
    {
        if (logo == nullptr || bounds.isEmpty()) return;
        logo->drawWithin (g, bounds,
                          juce::RectanglePlacement::xLeft
                        | juce::RectanglePlacement::yMid
                        | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
    }

    // Square app icon: charcoal tile with the white wordmark (for title bar /
    // taskbar). Set on the window via DocumentWindow::setIcon().
    inline juce::Image createIcon (int size = 256)
    {
        juce::Image img (juce::Image::ARGB, size, size, true);
        juce::Graphics g (img);
        g.fillAll (charcoal());
        if (auto d = createLogo (white()))
        {
            const float h = size * 0.18f;
            const float w = h * logoAspect;
            const float x = 0.5f * ((float) size - w);
            const float y = 0.5f * ((float) size - h);
            d->drawWithin (g, { x, y, w, h }, juce::RectanglePlacement::centred, 1.0f);
        }
        return img;
    }

    // ======================================================================
    // LookAndFeel: maps the default sans typeface to Montserrat and any
    // "Space Mono" request to Space Mono, and sets the dark Atomik scheme.
    // ======================================================================
    class AtomikLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        static bool isSidebarControl (const juce::Component& c)
        {
            const auto id = c.getComponentID();
            return id == "ctrlCombo" || id == "ctrlBtn" || id == "ctrlSlider" || id == "ctrlToggle"
                || id == "freqCombo" || id == "freqStep";
        }

        AtomikLookAndFeel()
        {
            montReg_  = loadFace ("Montserrat-Regular.ttf");
            montMed_  = loadFace ("Montserrat-Medium.ttf");
            montSemi_ = loadFace ("Montserrat-SemiBold.ttf");
            montBold_ = loadFace ("Montserrat-Bold.ttf");
            monoReg_  = loadFace ("SpaceMono-Regular.ttf");
            monoBold_ = loadFace ("SpaceMono-Bold.ttf");

            applyTheme();
        }

        // Re-read the active palette and restyle scheme colours. Call after a
        // theme change, then repaint the UI.
        void applyTheme()
        {
            refreshPalette();

            auto cs = getCurrentColourScheme();
            using S = juce::LookAndFeel_V4::ColourScheme;
            cs.setUIColour (S::windowBackground,  base());
            cs.setUIColour (S::widgetBackground,  panel());
            cs.setUIColour (S::menuBackground,    panel());
            cs.setUIColour (S::menuText,          text());
            cs.setUIColour (S::outline,           border());
            cs.setUIColour (S::defaultText,       text());
            cs.setUIColour (S::defaultFill,       accent());
            cs.setUIColour (S::highlightedFill,   accent());
            cs.setUIColour (S::highlightedText,   onAccent());
            setColourScheme (cs);

            setColour (juce::PopupMenu::backgroundColourId,           panel());
            setColour (juce::PopupMenu::textColourId,                 text());
            setColour (juce::PopupMenu::highlightedBackgroundColourId, accent());
            setColour (juce::PopupMenu::highlightedTextColourId,      onAccent());

            setColour (juce::ToggleButton::textColourId,            text());
            setColour (juce::ToggleButton::tickColourId,            accent());
            setColour (juce::ToggleButton::tickDisabledColourId,   accent().withAlpha (0.55f));
            setColour (juce::TextButton::textColourOffId,          onBtnIn());
            setColour (juce::TextButton::textColourOnId,           onAccent());
            setColour (juce::ComboBox::backgroundColourId,         btnIn());
            setColour (juce::ComboBox::textColourId,               onBtnIn());
            setColour (juce::ComboBox::outlineColourId,            controlBorder());
            setColour (juce::ComboBox::arrowColourId,              onBtnIn());

            // Slider / control value boxes are light fills — ink must stay dark while typing.
            setColour (juce::Slider::textBoxTextColourId,          onBtnIn());
            setColour (juce::Slider::textBoxBackgroundColourId,    btnIn());
            setColour (juce::Slider::textBoxOutlineColourId,       controlBorder());
            setColour (juce::Label::textWhenEditingColourId,       onBtnIn());
            setColour (juce::TextEditor::textColourId,             onBtnIn());
            setColour (juce::TextEditor::backgroundColourId,       btnIn());
            setColour (juce::TextEditor::outlineColourId,          controlBorder());
            setColour (juce::TextEditor::focusedOutlineColourId,   controlBorder());
            setColour (juce::TextEditor::highlightColourId,        accent());
            setColour (juce::TextEditor::highlightedTextColourId,  onAccent());
            setColour (juce::CaretComponent::caretColourId,        onBtnIn());
        }

        juce::Font getLabelFont (juce::Label& l) override
        {
            return l.getFont();
        }

        juce::Font getTextButtonFont (juce::TextButton& btn, int buttonHeight) override
        {
            const float hSide = juce::jmin (UI::scaledFont (Type::sidebarButtonText),
                (float) buttonHeight * UiConfig::Laf::sidebarButtonHeightScale);
            if (isSidebarControl (btn))
                return techMed (hSide);   // sidebar actions = Medium

            const auto id = btn.getComponentID();
            if (id == "bottomBtn")
            {
                const float h = juce::jmin (UI::scaledFont (Type::bottomBarButton),
                                            (float) buttonHeight * 0.55f);
                const auto txt = btn.getButtonText();
                const bool isExport = txt.containsIgnoreCase ("SAVE")
                                   || txt.containsIgnoreCase ("EXPORT");
                if (isExport)
                    return techMed (h);   // Export Button = Medium
                const bool active = btn.findColour (juce::TextButton::buttonColourId) == accent();
                return active ? techMed (h)   // Active View = Medium
                              : tech (h);     // Idle View = Regular
            }
            if (id == "dashAction")
                return techSemi (UI::scaledFont (Type::dashActionButton));
            if (id == "dashRecent")
                return tech (UI::scaledFont (Type::dashRecentItem));
            if (id == "plotFit")
                return techMed (UI::scaledFont (Type::plotFitButton));
            if (id == "headerStats")
                return techSemi (UI::scaledFont (Type::headerStatsButton));
            if (id == "prefsBtn")
                return techMed (juce::jmin (UI::scaledFont (Type::prefsButton),
                    (float) buttonHeight * 0.52f));

            return techMed (juce::jmin (UI::scaledFont (Type::button),
                                        (float) buttonHeight * UiConfig::Laf::buttonHeightScale));
        }

        juce::Font getComboBoxFont (juce::ComboBox& box) override
        {
            // Combo values use Regular (same family as field values).
            if (isSidebarControl (box))
                return tech (UI::scaledFont (Type::sidebarMainValue));

            return tech (juce::jmin (UI::scaledFont (Type::input),
                                     (float) box.getHeight() * UiConfig::Laf::comboHeightScale));
        }

        juce::Font getPopupMenuFont() override { return tech (UI::scaledFont (Type::input)); }

        juce::Font getSliderPopupFont (juce::Slider&) override { return mono (UI::scaledFont (Type::input)); }

        void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                   const juce::Colour& backgroundColour,
                                   bool highlighted, bool down) override
        {
            if (button.getComponentID() == "ctrlResetLink")
                return;

            auto bounds = button.getLocalBounds().toFloat();
            juce::Colour base = backgroundColour;

            if (! button.isEnabled())
                base = btnIn().withMultipliedBrightness (0.82f).withAlpha (0.72f);
            else if (down)
                base = base.brighter (0.06f);
            else if (highlighted)
                base = base.brighter (backgroundColour == accent() ? 0.10f : 0.14f);

            const float corner = isSidebarControl (button) ? controlCorner : UI::cornerRadius;
            g.setColour (base);
            g.fillRoundedRectangle (bounds, corner);

            if (button.getComponentID() == "plotFit")
                strokeInsideRounded (g, bounds, corner, controlBorderPx, fitViewBorder(),
                                     button.isEnabled() ? 1.0f : 0.40f);
            else if (isSidebarControl (button) || button.getComponentID() == "bottomBtn"
                     || button.getComponentID() == "headerStats")
                strokeInsideRounded (g, bounds, corner, controlBorderPx, controlBorder(),
                                     button.isEnabled() ? 1.0f : 0.40f);
            else
                strokeInsideRounded (g, bounds, corner, controlBorderPx, softEdge (base),
                                     button.isEnabled() ? 1.0f : 0.40f);
        }

        void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                               float sliderPos, float minSliderPos, float maxSliderPos,
                               const juce::Slider::SliderStyle style, juce::Slider& slider) override
        {
            if (style != juce::Slider::LinearHorizontal
                && style != juce::Slider::LinearVertical)
            {
                LookAndFeel_V4::drawLinearSlider (g, x, y, width, height,
                    sliderPos, minSliderPos, maxSliderPos, style, slider);
                return;
            }

            juce::ignoreUnused (minSliderPos, maxSliderPos);

            const bool horiz = (style == juce::Slider::LinearHorizontal);
            const float trackH = 2.0f;
            const float thumbD = 10.0f;
            const float half   = thumbD * 0.5f;
            const float trackCorner = 5.0f;
            // Filled track: white on dark theme, dark on light. Thumb #d81f1f.
            const bool darkTheme = AppSettings::get().isDark();
            const auto thumbCol  = juce::Colour (0xffd81f1f);
            const auto filledCol = darkTheme ? juce::Colours::white
                                             : juce::Colour (0xff313131);
            const auto emptyCol  = darkTheme ? juce::Colours::white.withAlpha (0.35f)
                                             : juce::Colours::black;
            const float alpha    = slider.isEnabled() ? 1.0f : 0.45f;

            if (horiz)
            {
                const float cy = (float) y + height * 0.5f;
                const float ty = cy - trackH * 0.5f;
                const float tx = (float) x + half;
                const float tw = juce::jmax (0.0f, (float) width - thumbD);
                const float thumbCx = juce::jlimit (tx, tx + tw, sliderPos);

                juce::Rectangle<float> track (tx, ty, tw, trackH);
                strokeInsideRounded (g, track, trackCorner, 0.2f,
                                     emptyCol.withMultipliedAlpha (alpha));

                const float fillW = juce::jmax (0.0f, thumbCx - tx);
                if (fillW > 0.5f)
                {
                    g.setColour (filledCol.withAlpha (alpha));
                    g.fillRoundedRectangle (tx, ty, fillW, trackH, trackCorner);
                }

                g.setColour (thumbCol.withAlpha (alpha));
                g.fillEllipse (thumbCx - half, cy - half, thumbD, thumbD);
            }
            else
            {
                const float cx = (float) x + width * 0.5f;
                const float tx = cx - trackH * 0.5f;
                const float ty = (float) y + half;
                const float th = juce::jmax (0.0f, (float) height - thumbD);
                const float thumbCy = juce::jlimit (ty, ty + th, sliderPos);

                juce::Rectangle<float> track (tx, ty, trackH, th);
                strokeInsideRounded (g, track, trackCorner, 0.2f,
                                     emptyCol.withMultipliedAlpha (alpha));

                const float fillH = juce::jmax (0.0f, ty + th - thumbCy);
                if (fillH > 0.5f)
                {
                    g.setColour (filledCol.withAlpha (alpha));
                    g.fillRoundedRectangle (tx, thumbCy, trackH, fillH, trackCorner);
                }

                g.setColour (thumbCol.withAlpha (alpha));
                g.fillEllipse (cx - half, thumbCy - half, thumbD, thumbD);
            }
        }

        int getSliderThumbRadius (juce::Slider&) override
        {
            // Keep min/max positions inset so JUCE never drives the thumb
            // centre onto the clipped left/right edge of the slider.
            return juce::jmax (1, (int) (UI::thumbDiameter * 0.5f + 0.5f));
        }

        void drawComboBox (juce::Graphics& g, int width, int height, bool isDown,
                           int buttonX, int buttonY, int buttonW, int buttonH,
                           juce::ComboBox& box) override
        {
            juce::ignoreUnused (isDown, buttonX, buttonY, buttonW, buttonH);
            auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
            g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
            g.fillRoundedRectangle (bounds, controlCorner);
            strokeInsideRounded (g, bounds, controlCorner, controlBorderPx, controlBorder(),
                                 box.isEnabled() ? 1.0f : 0.40f);

            const float arrowScale = (isSidebarControl (box)
                                         ? UiConfig::Control::sidebarComboArrowScale
                                         : 1.0f) * UI::scale;
            const float arrowInset = 14.0f * arrowScale;
            const float arrowHalfW = 4.0f * arrowScale;
            const float arrowTop   = 2.5f * arrowScale;
            const float arrowBot   = 3.0f * arrowScale;

            const float arrowX = bounds.getRight() - arrowInset;
            const float arrowY = bounds.getCentreY();
            juce::Path arrow;
            arrow.addTriangle (arrowX - arrowHalfW, arrowY - arrowTop,
                               arrowX + arrowHalfW, arrowY - arrowTop,
                               arrowX,           arrowY + arrowBot);
            g.setColour (onBtnIn().withAlpha (box.isEnabled() ? 1.0f : 0.45f));
            g.fillPath (arrow);
        }

        void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
        {
            const float arrowScale = (isSidebarControl (box)
                                         ? UiConfig::Control::sidebarComboArrowScale
                                         : 1.0f) * UI::scale;
            const int arrowReserve = juce::roundToInt (22.0f * arrowScale);
            const int textPad      = UiConfig::Scale::px (8);
            label.setBounds (textPad, 1,
                             juce::jmax (1, box.getWidth() - arrowReserve - textPad),
                             box.getHeight() - 2);
            label.setFont (getComboBoxFont (box));
            label.setJustificationType (juce::Justification::centredLeft);
            label.setMinimumHorizontalScale (0.0f);
        }

        juce::Label* createSliderTextBox (juce::Slider& slider) override
        {
            auto* l = LookAndFeel_V4::createSliderTextBox (slider);
            l->setFont (sliderNumber (UI::scaledFont (isSidebarControl (slider) ? Type::sidebarMainValue : Type::input)));
            l->setJustificationType (juce::Justification::centred);
            l->setColour (juce::Label::backgroundColourId,          btnIn());
            l->setColour (juce::Label::outlineColourId,             controlBorder());
            l->setColour (juce::Label::textColourId,                onBtnIn());
            // Active editor inherits these — without them, dark-theme defaultText (white)
            // paints white-on-white inside the value box.
            l->setColour (juce::Label::textWhenEditingColourId,     onBtnIn());
            l->setColour (juce::TextEditor::textColourId,           onBtnIn());
            l->setColour (juce::TextEditor::backgroundColourId,     btnIn());
            l->setColour (juce::TextEditor::highlightColourId,      accent());
            l->setColour (juce::TextEditor::highlightedTextColourId,onAccent());
            l->setColour (juce::CaretComponent::caretColourId,      onBtnIn());
            return l;
        }

        void drawLabel (juce::Graphics& g, juce::Label& label) override
        {
            const auto bg = label.findColour (juce::Label::backgroundColourId);
            const auto outline = label.findColour (juce::Label::outlineColourId);
            const bool boxed = ! bg.isTransparent() || ! outline.isTransparent();
            auto bounds = label.getLocalBounds().toFloat();

            if (! bg.isTransparent())
            {
                g.setColour (bg);
                g.fillRoundedRectangle (bounds, valueBoxCorner);
            }

            if (! label.isBeingEdited() && label.getText().isNotEmpty())
            {
                g.setColour (label.findColour (juce::Label::textColourId));
                g.setFont (getLabelFont (label));
                const auto textArea = getLabelBorderSize (label).subtractedFrom (label.getLocalBounds());
                g.drawFittedText (label.getText(), textArea, label.getJustificationType(),
                                  juce::jmax (1, (int) (textArea.getHeight() / 12.0f)),
                                  label.getMinimumHorizontalScale());
            }

            if (boxed)
                strokeInsideRounded (g, bounds, valueBoxCorner, controlBorderPx, controlBorder());
        }

        juce::Slider::SliderLayout getSliderLayout (juce::Slider& slider) override
        {
            auto layout = LookAndFeel_V4::getSliderLayout (slider);
            if (! isSidebarControl (slider)
                || slider.getTextBoxPosition() != juce::Slider::TextBoxRight)
                return layout;

            const int gap = UiConfig::Scale::px (UiConfig::Layout::sidebarSliderTextGap);
            const int boxW = layout.textBoxBounds.getWidth();
            const int boxH = layout.textBoxBounds.getHeight();
            auto bounds = slider.getLocalBounds();
            layout.textBoxBounds = juce::Rectangle<int> (
                bounds.getRight() - boxW,
                bounds.getCentreY() - boxH / 2,
                boxW, boxH);
            layout.sliderBounds = bounds.withTrimmedRight (boxW + gap);
            return layout;
        }

        void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                               bool highlighted, bool down) override
        {
            const bool sidebar = isSidebarControl (button);
            // Sidebar: fixed solid SemiBold size (do not crush by row height).
            const float fontSize = sidebar
                ? UI::scaledFont (Type::sidebarFieldLabel)
                : juce::jlimit (UI::scaledFont (Type::label),
                                UI::scaledFont (Type::input),
                                (float) button.getHeight() * UiConfig::Laf::toggleHeightScale);
            // Field labels Regular; "Enabled" specifically Medium per typography sheet.
            const bool enabledLbl = sidebar
                && button.getButtonText().equalsIgnoreCase ("Enabled");
            const auto font = enabledLbl ? techMed (fontSize)
                            : sidebar   ? tech (fontSize)
                                        : tech (fontSize);
            // UI::tickSize is already window-scaled — do not multiply by UI::scale again.
            const float tickScale = sidebar ? UiConfig::Control::sidebarTickScale : 1.0f;
            const int maxSide = juce::jmax (1, button.getHeight() - 2);
            const int tickW = juce::jlimit (12, maxSide,
                                           juce::roundToInt (UI::tickSize * tickScale));
            const float tickY = ((float) button.getHeight() - (float) tickW) * 0.5f;

            drawTickBox (g, button, 1.0f, tickY, (float) tickW, (float) tickW,
                         button.getToggleState(), button.isEnabled(), highlighted, down);

            juce::Colour textCol = button.findColour (juce::ToggleButton::textColourId);
            if (textCol.isTransparent() || textCol == juce::Colours::black)
                textCol = text();
            if (! button.isEnabled())
                textCol = text().withAlpha (0.62f);

            g.setColour (textCol); // full opacity when enabled
            g.setFont (font);
            const int tickPad = tickW + (sidebar ? 8 : 12);
            g.drawFittedText (button.getButtonText(),
                              button.getLocalBounds()
                                  .withTrimmedLeft (tickPad)
                                  .withTrimmedRight (4),
                              juce::Justification::centredLeft, 2);
        }

        void drawTickBox (juce::Graphics& g, juce::Component& component,
                          float x, float y, float w, float h,
                          bool ticked, bool isEnabled,
                          bool highlighted, bool down) override
        {
            juce::ignoreUnused (highlighted, down, component);
            // Pixel-aligned square so fill and border share the same edges.
            const int side = juce::jmax (1, juce::roundToInt (juce::jmin (w, h)));
            const auto box = juce::Rectangle<int> (juce::roundToInt (x),
                                                   juce::roundToInt (y),
                                                   side, side).toFloat();
            const float a = isEnabled ? 1.0f : 0.45f;

            if (ticked)
            {
                // Checked: edge-to-edge accent fill, soft-white tick (both themes).
                g.setColour (accent().withAlpha (a));
                g.fillRect (box);
                auto tick = getTickShape (0.6f);
                g.setColour (onAccent().withAlpha (a));
                const float inset = juce::jmax (3.0f, side * 0.22f);
                g.fillPath (tick, tick.getTransformToScaleToFit (
                    box.reduced (inset), true));
            }
            else
            {
                // Unchecked: control surface + hairline on the outer edge.
                g.setColour (btnIn().withAlpha (isEnabled ? 1.0f : 0.55f));
                g.fillRect (box);
                strokeInsideRounded (g, box, 0.0f, controlBorderPx, controlBorder(),
                                     isEnabled ? 1.0f : 0.40f);
            }
        }

        void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                             bool highlighted, bool down) override
        {
            juce::ignoreUnused (highlighted, down);
            const auto font = getTextButtonFont (button, button.getHeight());
            g.setFont (font);

            juce::Colour col = button.findColour (
                button.getToggleState() ? juce::TextButton::textColourOnId
                                        : juce::TextButton::textColourOffId);
            if (! button.isEnabled())
                col = text().withAlpha (0.58f);

            g.setColour (col);

            const int yIndent = juce::jmin (4, button.proportionOfHeight (0.28f));
            const int cornerSize = juce::jmin (button.getHeight(), button.getWidth()) / 2;
            const int fontHeight = juce::roundToInt (font.getHeight() * 0.6f);
            const int leftIndent  = juce::jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnLeft() ? 4 : 2));
            const int rightIndent = juce::jmin (fontHeight, 2 + cornerSize / (button.isConnectedOnRight() ? 4 : 2));
            const int textWidth = button.getWidth() - leftIndent - rightIndent;

            if (textWidth > 0)
                g.drawFittedText (button.getButtonText(),
                                  leftIndent, yIndent, textWidth,
                                  button.getHeight() - yIndent * 2,
                                  juce::Justification::centred, 2);
        }

        juce::Typeface::Ptr getTypefaceForFont (const juce::Font& f) override
        {
            const auto name = f.getTypefaceName();
            const bool bold = f.isBold();

            if (name == monoName() || name.containsIgnoreCase ("mono"))
            {
                if (bold && monoBold_) return monoBold_;
                if (monoReg_)          return monoReg_;
            }
            else
            {
                // Prefer explicit weight names (SemiBold before Bold substring).
                if (name.containsIgnoreCase ("semibold") && montSemi_) return montSemi_;
                if ((bold || name.containsIgnoreCase ("bold")) && montBold_) return montBold_;
                if (name.containsIgnoreCase ("medium") && montMed_) return montMed_;
                if (montReg_) return montReg_;
            }
            return juce::LookAndFeel_V4::getTypefaceForFont (f);
        }

        // Thin floating pill scrollbar (mockup: dark thumb on light, light thumb on dark).
        void drawScrollbar (juce::Graphics& g, juce::ScrollBar& bar, int x, int y, int width, int height,
                            bool isVertical, int thumbStart, int thumbSize,
                            bool isMouseOver, bool isMouseDown) override
        {
            juce::ignoreUnused (bar);
            // Transparent track — pill floats over the panel background.
            if (thumbSize <= 0)
                return;

            const float inset = UiConfig::Scale::px (2.0f);
            juce::Rectangle<float> thumb;
            if (isVertical)
            {
                const float tw = juce::jmin (4.0f * UI::scale, (float) width - inset * 2.0f);
                thumb = { (float) x + ((float) width - tw) * 0.5f,
                          (float) thumbStart + inset * 0.5f,
                          tw,
                          juce::jmax (tw * 2.0f, (float) thumbSize - inset) };
            }
            else
            {
                const float th = juce::jmin (4.0f * UI::scale, (float) height - inset * 2.0f);
                thumb = { (float) thumbStart + inset * 0.5f,
                          (float) y + ((float) height - th) * 0.5f,
                          juce::jmax (th * 2.0f, (float) thumbSize - inset),
                          th };
            }

            auto col = scrollThumb();
            if (isMouseDown)      col = col.brighter (0.08f);
            else if (isMouseOver) col = col.brighter (0.04f);

            g.setColour (col);
            g.fillRoundedRectangle (thumb, thumb.getWidth() * 0.5f);
        }

        bool areScrollbarButtonsVisible() override { return false; }

        int getDefaultScrollbarWidth() override
        {
            return UiConfig::Scale::px (8);
        }

    private:
        juce::Typeface::Ptr montReg_, montMed_, montSemi_, montBold_, monoReg_, monoBold_;
    };
}

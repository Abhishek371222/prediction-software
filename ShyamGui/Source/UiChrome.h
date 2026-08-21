#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"

// ---------------------------------------------------------------------------
// SectionHeader — collapsible sidebar section title with chevron.
// ---------------------------------------------------------------------------
class SectionHeader : public juce::Component
{
public:
    explicit SectionHeader (const juce::String& title) : title_ (title) {}

    std::function<void (bool expanded)> onToggled;

    void setExpanded (bool e)
    {
        if (expanded_ == e) return;
        expanded_ = e;
        repaint();
    }

    bool isExpanded() const { return expanded_; }

    void setTitleFontSize (float px) { titleFontSize_ = px; repaint(); }
    void setChevronScale (float s)   { chevronScale_ = s; repaint(); }

    void mouseUp (const juce::MouseEvent&) override
    {
        setExpanded (! expanded_);
        if (onToggled) onToggled (expanded_);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (Brand::heading());
        const float titleSize = titleFontSize_ > 0.0f ? titleFontSize_ : Brand::Type::sectionHdr;
        // Section Header = Montserrat Medium
        g.setFont (Brand::techMed (titleSize));

        // Mockup: small caret ~6px from content left, title tight after it.
        const float s = chevronScale_ * 0.95f;
        juce::Path chevron;
        const float cx = 6.0f;
        const float cy = getHeight() * 0.5f;
        if (expanded_)
            chevron.addTriangle (cx - 4.0f * s, cy - 2.0f * s, cx + 4.0f * s, cy - 2.0f * s,
                                 cx, cy + 3.0f * s);
        else
            chevron.addTriangle (cx - 2.0f * s, cy - 4.0f * s, cx - 2.0f * s, cy + 4.0f * s,
                                 cx + 3.0f * s, cy);

        g.fillPath (chevron);
        const int textLeft = juce::roundToInt (16.0f * (chevronScale_ > 0 ? chevronScale_ : 1.0f));
        g.drawText (title_, textLeft, 0, getWidth() - textLeft - 4, getHeight(),
                    juce::Justification::centredLeft, true);
    }

private:
    juce::String title_;
    bool expanded_ = true;
    float titleFontSize_ = 0.0f;
    float chevronScale_  = 1.0f;
};

// ---------------------------------------------------------------------------
// ParamBar — plain-text simulation readouts (top bar, no chip boxes).
// ---------------------------------------------------------------------------
class ParamBar : public juce::Component
{
public:
    void setChips (const juce::StringArray& chips) { chips_ = chips; repaint(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());

        const float pad = 12.0f;
        const float gap = 18.0f;
        float x = pad;
        const auto font = Brand::tech (Brand::Type::metadata);
        g.setFont (font);
        g.setColour (Brand::text());

        const auto textArea = getLocalBounds().toFloat().reduced (pad, 0.0f);

        for (const auto& chip : chips_)
        {
            const float tw = font.getStringWidthFloat (chip);
            if (x + tw > textArea.getRight()) break;

            g.drawText (chip,
                        juce::Rectangle<float> (x, textArea.getY(), tw, textArea.getHeight()),
                        juce::Justification::centredLeft, false);
            x += tw + gap;
        }
    }

private:
    juce::StringArray chips_;
};

// ---------------------------------------------------------------------------
// PlotHeaderBar — workspace title + navigation toolbar.
// ---------------------------------------------------------------------------
class PlotHeaderBar : public juce::Component
{
public:
    PlotHeaderBar()
    {
        title_.setFont (Brand::tech (Brand::Type::panelTitle));
        title_.setColour (juce::Label::textColourId, Brand::plotTitle());
        title_.setJustificationType (juce::Justification::centredLeft);
        title_.setMinimumHorizontalScale (1.0f);
        title_.setBorderSize ({});
        addAndMakeVisible (title_);

        auto mk = [] (const char* svg, juce::Colour c) -> std::unique_ptr<juce::Drawable>
        {
            if (auto xml = juce::parseXML (svg))
                if (auto d = juce::Drawable::createFromSVG (*xml))
                {
                    d->replaceColour (juce::Colours::white, c);
                    return d;
                }
            return {};
        };

        // Tool glyphs: dark ink on light toolbar, light ink on dark toolbar fill.
        const auto iconCol = AppSettings::get().isDark() ? Brand::white() : Brand::text();
        const auto iconHi  = Brand::accent();

        auto styleTool = [&] (juce::DrawableButton& b, const char* svg, const juce::String& tip,
                              bool toggle = false)
        {
            b.setTooltip (tip);
            b.setColour (juce::DrawableButton::backgroundColourId,   Brand::plotToolbar());
            b.setColour (juce::DrawableButton::backgroundOnColourId, Brand::accent().withAlpha (0.28f));
            b.setImages (mk (svg, iconCol).get(), mk (svg, iconHi).get(), mk (svg, iconHi).get());
            b.setEdgeIndent (4);
            b.setOpaque (false);
            b.setVisible (true);
            if (toggle)
            {
                b.setClickingTogglesState (true);
                b.setRadioGroupId (44001);
            }
            addAndMakeVisible (b);
        };

        styleTool (btnSelect_,  kSelectSVG,  "Select — click / drag speakers", true);
        styleTool (btnPan_,     kPanSVG,     "Pan — drag to move the view", true);
        styleTool (btnPencil_,  kPencilSVG,  "Pencil — choose colour, then draw freehand", true);
        styleTool (btnEraser_,  kEraserSVG,  "Eraser — scrub to remove drawings", true);
        styleTool (btnRuler_,   kRulerSVG,   "Ruler — click two points to measure distance", true);
        styleTool (btnLine_,    kLineSVG,    "Line — click start, click end to draw", true);
        styleTool (btnRect_,    kRectSVG,    "Rectangle — drag to place a filled rectangle", true);
        styleTool (btnSquare_,  kSquareSVG,  "Square — drag to place a filled square", true);
        styleTool (btnCircle_,  kCircleSVG,  "Circle — drag to place a filled circle", true);
        styleTool (btnZoomIn_,  kZoomInSVG,  "Zoom in");
        styleTool (btnZoomOut_, kZoomOutSVG, "Zoom out");

        btnSelect_.setToggleState (true, juce::dontSendNotification);

        colourSwatch_.setTooltip ("Fill / stroke colour");
        colourSwatch_.setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        colourSwatch_.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        colourSwatch_.setColour (juce::TextButton::textColourOffId,  juce::Colours::transparentBlack);
        colourSwatch_.setButtonText ({});
        addAndMakeVisible (colourSwatch_);

        alphaLabel_.setText ("Opacity", juce::dontSendNotification);
        alphaLabel_.setFont (Brand::tech (10.0f));
        alphaLabel_.setColour (juce::Label::textColourId, Brand::muted());
        alphaLabel_.setJustificationType (juce::Justification::centredRight);
        alphaLabel_.setBorderSize ({});
        addAndMakeVisible (alphaLabel_);

        fillAlpha_.setRange (0.0, 100.0, 1.0);
        fillAlpha_.setValue (35.0, juce::dontSendNotification);
        fillAlpha_.setSliderStyle (juce::Slider::LinearHorizontal);
        fillAlpha_.setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
        fillAlpha_.setTooltip ("Shape fill transparency (0% = clear … 100% = solid)");
        fillAlpha_.setColour (juce::Slider::trackColourId, Brand::border());
        fillAlpha_.setColour (juce::Slider::thumbColourId, Brand::accent());
        fillAlpha_.setColour (juce::Slider::backgroundColourId, Brand::plotToolbar());
        addAndMakeVisible (fillAlpha_);

        fitBtn_.setButtonText ("Fit View");
        fitBtn_.setComponentID ("plotFit");
        fitBtn_.setTooltip ("Fit the full field in view");
        fitBtn_.setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
        fitBtn_.setColour (juce::TextButton::textColourOffId,  AppSettings::get().isDark() ? Brand::white()
                                                                                           : Brand::onBtnIn());
        addAndMakeVisible (fitBtn_);

        rangeBtn_.setButtonText ("Range");
        rangeBtn_.setComponentID ("plotRange");
        rangeBtn_.setTooltip ("Show / hide 2 m · 4 m · 8 m range markers");
        rangeBtn_.setClickingTogglesState (true);
        rangeBtn_.setToggleState (true, juce::dontSendNotification);
        rangeBtn_.setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
        rangeBtn_.setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.35f));
        rangeBtn_.setColour (juce::TextButton::textColourOffId,  AppSettings::get().isDark() ? Brand::white()
                                                                                             : Brand::onBtnIn());
        rangeBtn_.setColour (juce::TextButton::textColourOnId,   AppSettings::get().isDark() ? Brand::white()
                                                                                             : Brand::onBtnIn());
        addAndMakeVisible (rangeBtn_);

        // Keep tool icons above the title so they never get covered.
        for (auto* tb : { &btnSelect_, &btnPan_, &btnPencil_, &btnEraser_,
                          &btnRuler_, &btnLine_, &btnRect_, &btnSquare_, &btnCircle_,
                          &btnZoomIn_, &btnZoomOut_ })
            tb->toFront (false);
        colourSwatch_.toFront (false);
        alphaLabel_.toFront (false);
        fillAlpha_.toFront (false);
        rangeBtn_.toFront (false);
        fitBtn_.toFront (false);
    }

    void setTitle (const juce::String& t) { title_.setText (t, juce::dontSendNotification); }

    void setDrawColour (juce::Colour c)
    {
        drawColour_ = c;
        colourSwatch_.setColour (juce::TextButton::buttonColourId, c);
        colourSwatch_.repaint();
    }

    juce::Colour getDrawColour() const noexcept { return drawColour_; }

    void setFillAlpha01 (float a)
    {
        fillAlpha_.setValue ((double) juce::jlimit (0.0f, 1.0f, a) * 100.0, juce::dontSendNotification);
    }

    float getFillAlpha01() const noexcept
    {
        return (float) (fillAlpha_.getValue() / 100.0);
    }

    enum class ActiveTool { Select, Pan, Pencil, Eraser, Ruler, Line, Rectangle, Square, Circle };
    void setActiveTool (ActiveTool t)
    {
        btnSelect_.setToggleState (t == ActiveTool::Select, juce::dontSendNotification);
        btnPan_.setToggleState    (t == ActiveTool::Pan,    juce::dontSendNotification);
        btnPencil_.setToggleState (t == ActiveTool::Pencil, juce::dontSendNotification);
        btnEraser_.setToggleState (t == ActiveTool::Eraser, juce::dontSendNotification);
        btnRuler_.setToggleState  (t == ActiveTool::Ruler,  juce::dontSendNotification);
        btnLine_.setToggleState   (t == ActiveTool::Line,   juce::dontSendNotification);
        btnRect_.setToggleState   (t == ActiveTool::Rectangle, juce::dontSendNotification);
        btnSquare_.setToggleState (t == ActiveTool::Square, juce::dontSendNotification);
        btnCircle_.setToggleState (t == ActiveTool::Circle, juce::dontSendNotification);
    }

    juce::DrawableButton btnSelect_ { "sel", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnPan_    { "pan", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnPencil_ { "pen", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnEraser_ { "ers", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnRuler_  { "rul", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnLine_   { "lin", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnRect_   { "rect", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnSquare_ { "sq", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnCircle_ { "cir", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnZoomIn_ { "zi",  juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnZoomOut_{ "zo",  juce::DrawableButton::ImageFitted };
    juce::TextButton     colourSwatch_;
    juce::Label          alphaLabel_;
    juce::Slider         fillAlpha_;
    juce::TextButton     fitBtn_;
    juce::TextButton     rangeBtn_;

    void paint (juce::Graphics& g) override
    {
        // No bottom hairline — MainComponent draws a single outer frame around
        // the plot title bar + canvas so the heatmap window has one top border.
        g.fillAll (Brand::panelDark());
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        auto sw = colourSwatch_.getBounds().toFloat();
        if (sw.isEmpty()) return;
        g.setColour (drawColour_);
        g.fillRoundedRectangle (sw, 4.0f);
        g.setColour (Brand::border());
        g.drawRoundedRectangle (sw, 4.0f, 1.0f);
    }

    void resized() override
    {
        title_.setFont (Brand::techSemi (Brand::UI::scaledFont (Brand::Type::panelTitle)));

        auto b = getLocalBounds().reduced (UiConfig::Scale::px (6), UiConfig::Scale::px (4));
        // Keep tools smaller than the header so they never clip vertically.
        const int tool = juce::jlimit (22, b.getHeight(), UiConfig::Scale::px (28));
        const int gap  = UiConfig::Scale::px (4);
        const int fitW = UiConfig::Scale::px (72);
        const int rangeW = UiConfig::Scale::px (60);
        const int swatch = juce::jlimit (14, tool - 4, UiConfig::Scale::px (18));
        const int edgeIndent = juce::jmax (3, tool / 6);
        const int sep = UiConfig::Scale::px (8);

        for (auto* tb : { &btnSelect_, &btnPan_, &btnPencil_, &btnEraser_,
                          &btnRuler_, &btnLine_, &btnRect_, &btnSquare_, &btnCircle_,
                          &btnZoomIn_, &btnZoomOut_ })
            tb->setEdgeIndent (edgeIndent);

        auto placeRight = [&] (juce::Component& c, int w)
        {
            auto r = b.removeFromRight (w);
            c.setBounds (r.withSizeKeepingCentre (w, tool));
        };

        // Right cluster: Fit / Range / Zoom (navigation)
        placeRight (fitBtn_, fitW);
        b.removeFromRight (gap);
        placeRight (rangeBtn_, rangeW);
        b.removeFromRight (sep);
        placeRight (btnZoomOut_, tool);
        b.removeFromRight (gap);
        placeRight (btnZoomIn_, tool);
        b.removeFromRight (sep);

        // Shape tools + opacity
        placeRight (btnCircle_, tool);
        b.removeFromRight (gap);
        placeRight (btnSquare_, tool);
        b.removeFromRight (gap);
        placeRight (btnRect_, tool);
        b.removeFromRight (gap);
        {
            const int alphaW = UiConfig::Scale::px (72);
            const int labW = UiConfig::Scale::px (44);
            auto ar = b.removeFromRight (alphaW);
            fillAlpha_.setBounds (ar.withSizeKeepingCentre (alphaW, juce::jmax (14, tool - 8)));
            b.removeFromRight (gap);
            auto lr = b.removeFromRight (labW);
            alphaLabel_.setBounds (lr.withSizeKeepingCentre (labW, tool));
        }
        b.removeFromRight (sep);

        // Drawing tools (beside select/pan)
        placeRight (btnLine_, tool);
        b.removeFromRight (gap);
        placeRight (btnRuler_, tool);
        b.removeFromRight (gap);
        placeRight (btnEraser_, tool);
        b.removeFromRight (gap);
        placeRight (btnPencil_, tool);
        b.removeFromRight (gap);
        {
            auto sw = b.removeFromRight (swatch + 2);
            colourSwatch_.setBounds (sw.withSizeKeepingCentre (swatch, swatch));
        }
        b.removeFromRight (sep);

        placeRight (btnPan_, tool);
        b.removeFromRight (gap);
        placeRight (btnSelect_, tool);
        b.removeFromRight (UiConfig::Scale::px (8));

        // Title uses leftover width; keep full text unless the bar is genuinely cramped.
        const int titleNeed = juce::roundToInt (
            title_.getFont().getStringWidthFloat (title_.getText()) + 8.0f);
        title_.setMinimumHorizontalScale (b.getWidth() >= titleNeed ? 1.0f : 0.85f);
        title_.setBounds (b);
    }

    void lookAndFeelChanged() override
    {
        title_.setFont (Brand::techSemi (Brand::UI::scaledFont (Brand::Type::panelTitle)));
        title_.setColour (juce::Label::textColourId, Brand::plotTitle());
        fitBtn_.setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
        fitBtn_.setColour (juce::TextButton::textColourOffId,
                           AppSettings::get().isDark() ? Brand::white() : Brand::onBtnIn());
        rangeBtn_.setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
        rangeBtn_.setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.35f));
        rangeBtn_.setColour (juce::TextButton::textColourOffId,
                             AppSettings::get().isDark() ? Brand::white() : Brand::onBtnIn());
        rangeBtn_.setColour (juce::TextButton::textColourOnId,
                             AppSettings::get().isDark() ? Brand::white() : Brand::onBtnIn());
        colourSwatch_.setColour (juce::TextButton::buttonColourId, drawColour_);

        auto mk = [] (const char* svg, juce::Colour c) -> std::unique_ptr<juce::Drawable>
        {
            if (auto xml = juce::parseXML (svg))
                if (auto d = juce::Drawable::createFromSVG (*xml))
                {
                    d->replaceColour (juce::Colours::white, c);
                    return d;
                }
            return {};
        };
        const auto iconCol = AppSettings::get().isDark() ? Brand::white() : Brand::text();
        const auto iconHi  = Brand::accent();
        auto restyle = [&] (juce::DrawableButton& b, const char* svg)
        {
            b.setColour (juce::DrawableButton::backgroundColourId,   Brand::plotToolbar());
            b.setColour (juce::DrawableButton::backgroundOnColourId, Brand::accent().withAlpha (0.28f));
            b.setImages (mk (svg, iconCol).get(), mk (svg, iconHi).get(), mk (svg, iconHi).get());
        };
        restyle (btnSelect_,  kSelectSVG);
        restyle (btnPan_,     kPanSVG);
        restyle (btnPencil_,  kPencilSVG);
        restyle (btnEraser_,  kEraserSVG);
        restyle (btnRuler_,   kRulerSVG);
        restyle (btnLine_,    kLineSVG);
        restyle (btnRect_,    kRectSVG);
        restyle (btnSquare_,  kSquareSVG);
        restyle (btnCircle_,  kCircleSVG);
        restyle (btnZoomIn_,  kZoomInSVG);
        restyle (btnZoomOut_, kZoomOutSVG);
        alphaLabel_.setColour (juce::Label::textColourId, Brand::muted());
        fillAlpha_.setColour (juce::Slider::trackColourId, Brand::border());
        fillAlpha_.setColour (juce::Slider::thumbColourId, Brand::accent());
        fillAlpha_.setColour (juce::Slider::backgroundColourId, Brand::plotToolbar());
    }

private:
    juce::Label title_;
    juce::Colour drawColour_ { 0xffffcc00 };

    static constexpr const char* kSelectSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M5 3l14 9.5-6.2 1.4L16.5 21l-2.2 1.2-3.6-7.2L5 19.5V3z"/></svg>)SVG";
    static constexpr const char* kPanSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M12 2l3.2 3.2h-2.2v4.1h4.1V7.3L20 10.5l-3.2 3.2v-2.2h-4.1v4.1h2.2L12 19l-3.2-3.2h2.2v-4.1H7.1v2.2L4 10.5l3.1-3.2v2.2h4.1V5.2H9L12 2z"/></svg>)SVG";
    // Simple filled glyphs — avoid SVG arcs (JUCE parser is picky).
    static constexpr const char* kPencilSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M3 21l3.8-.1L19.5 8.2l-3.7-3.7L3 17.3V21zm15.2-15.9l1.7 1.7 1.5-1.5-1.7-1.7-1.5 1.5z"/></svg>)SVG";
    static constexpr const char* kEraserSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M16.1 3.5l4.4 4.4c.8.8.8 2 0 2.8l-8.5 8.5H5.9L2.5 16c-.8-.8-.8-2 0-2.8L13.3 3.5c.8-.8 2-.8 2.8 0zM4 20h16v2H4z"/></svg>)SVG";
    static constexpr const char* kRulerSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M2.8 17.6L17.6 2.8l3.6 3.6L6.4 21.2l-3.6-3.6zm3.9-.4l1.2 1.2.9-.9-1.2-1.2-.9.9zm2.4-2.4l1.2 1.2.9-.9-1.2-1.2-.9.9zm2.4-2.4l1.2 1.2.9-.9-1.2-1.2-.9.9zm2.4-2.4l1.2 1.2.9-.9-1.2-1.2-.9.9zm2.4-2.4l1.2 1.2.9-.9-1.2-1.2-.9.9z"/></svg>)SVG";
    static constexpr const char* kLineSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M4 17.6L17.6 4l2.4 2.4L6.4 20 4 17.6z"/><path fill="#fff" d="M3 19h4v2H3zM17 3h4v2h-4z"/></svg>)SVG";
    static constexpr const char* kRectSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M2 8h20v8H2z" opacity=".4"/><path fill="#fff" d="M2 8h20v2H2zm0 6h20v2H2zM2 8h2v8H2zm18 0h2v8h-2z"/></svg>)SVG";
    static constexpr const char* kSquareSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M5 5h14v14H5z" opacity=".4"/><path fill="#fff" d="M5 5h14v2H5zm0 12h14v2H5zM5 5h2v14H5zm12 0h2v14h-2z"/></svg>)SVG";
    static constexpr const char* kCircleSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle cx="12" cy="12" r="8" fill="#fff" opacity=".4"/><circle cx="12" cy="12" r="8" fill="none" stroke="#fff" stroke-width="2"/></svg>)SVG";
    static constexpr const char* kZoomInSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle fill="none" stroke="#fff" stroke-width="2" cx="10.5" cy="10.5" r="6.5"/><path fill="#fff" d="M15.2 15.2l5.3 5.3-1.4 1.4-5.3-5.3z"/><path fill="#fff" d="M9.5 7.5h2v3h3v2h-3v3h-2v-3h-3v-2h3z"/></svg>)SVG";
    static constexpr const char* kZoomOutSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle fill="none" stroke="#fff" stroke-width="2" cx="10.5" cy="10.5" r="6.5"/><path fill="#fff" d="M15.2 15.2l5.3 5.3-1.4 1.4-5.3-5.3z"/><path fill="#fff" d="M7 9.5h7v2H7z"/></svg>)SVG";
};

// View-mode tile icons
namespace ViewIcons
{
    static constexpr const char* kHeatmap = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><rect fill="#fff" x="3" y="3" width="7" height="7" rx="1"/><rect fill="#fff" x="14" y="3" width="7" height="7" rx="1" opacity=".7"/><rect fill="#fff" x="3" y="14" width="7" height="7" rx="1" opacity=".7"/><rect fill="#fff" x="14" y="14" width="7" height="7" rx="1" opacity=".5"/></svg>)SVG";
    static constexpr const char* kDirectivity = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle fill="none" stroke="#fff" stroke-width="1.5" cx="12" cy="12" r="8"/><path fill="#fff" d="M12 4v16M4 12h16"/></svg>)SVG";
    static constexpr const char* kPolar = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle fill="none" stroke="#fff" stroke-width="1.5" cx="12" cy="12" r="8"/><path fill="#fff" d="M12 12L12 4M12 12L18.9 16M12 12L5.1 16"/></svg>)SVG";
    static constexpr const char* kFit = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M4 9V4h5M15 4h5v5M20 15v5h-5M9 20H4v-5"/></svg>)SVG";
    static constexpr const char* kPhase = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="none" stroke="#fff" stroke-width="1.6" stroke-linecap="round" d="M2 12c2.5 0 2.5-7 5-7s2.5 14 5 14 2.5-7 5-7"/></svg>)SVG";
}

// Header icons — filled charcoal glyphs matching mockup Stats row.
namespace HeaderIcons
{
    // Filled badge: disk = #fff (recolour to charcoal), mark = #000 (recolour to white)
    static constexpr const char* kHelp = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle cx="12" cy="12" r="11" fill="#fff"/><path fill="#000" d="M10.2 8.6c.35-1.15 1.3-1.9 2.7-1.9 1.55 0 2.65.95 2.65 2.35 0 .95-.45 1.55-1.35 2.15-.85.55-1.15.95-1.15 1.7v.45h-1.55v-.55c0-1.15.4-1.7 1.3-2.3.7-.45 1-0.85 1-1.4 0-.7-.55-1.15-1.35-1.15-.8 0-1.35.45-1.55 1.2l-1.7-.4zm1.95 7.55c.65 0 1.15-.5 1.15-1.15s-.5-1.15-1.15-1.15-1.15.5-1.15 1.15.5 1.15 1.15 1.15z"/></svg>)SVG";
    static constexpr const char* kGear = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M19.14 12.94c.04-.31.06-.63.06-.94s-.02-.63-.06-.94l2.03-1.58a.5.5 0 00.12-.64l-1.92-3.32a.5.5 0 00-.6-.22l-2.39.96a7.03 7.03 0 00-1.63-.94l-.36-2.54a.5.5 0 00-.5-.42h-3.84a.5.5 0 00-.5.42l-.36 2.54c-.59.24-1.13.55-1.63.94l-2.39-.96a.5.5 0 00-.6.22L2.77 8.84a.5.5 0 00.12.64l2.03 1.58c-.04.31-.07.63-.07.94s.03.63.07.94L2.89 14.5a.5.5 0 00-.12.64l1.92 3.32c.13.22.39.3.6.22l2.39-.96c.5.39 1.04.71 1.63.94l.36 2.54c.05.24.26.42.5.42h3.84c.24 0 .45-.18.5-.42l.36-2.54c.59-.24 1.13-.55 1.63-.94l2.39.96c.22.08.47 0 .6-.22l1.92-3.32a.5.5 0 00-.12-.64l-2.03-1.58zM12 15.6A3.6 3.6 0 1112 8.4a3.6 3.6 0 010 7.2z"/></svg>)SVG";
    static constexpr const char* kMenu = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle cx="12" cy="5" r="2" fill="#fff"/><circle cx="12" cy="12" r="2" fill="#fff"/><circle cx="12" cy="19" r="2" fill="#fff"/></svg>)SVG";
    // Three-line hamburger for sidebar collapse / expand.
    static constexpr const char* kHamburger = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M4 6.5h16v2.2H4zm0 4.65h16v2.2H4zm0 4.65h16v2.2H4z"/></svg>)SVG";
}

// ---------------------------------------------------------------------------
// ViewTileButton — icon + caption tile for view mode switching.
// ---------------------------------------------------------------------------
class ViewTileButton : public juce::Button
{
public:
    ViewTileButton (const juce::String& name, const char* svg)
        : juce::Button (name)
    {
        if (auto xml = juce::parseXML (svg))
            icon_ = juce::Drawable::createFromSVG (*xml);
    }

    void setActive (bool a) { active_ = a; repaint(); }

    void paintButton (juce::Graphics& g, bool highlighted, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        const float radius = 7.0f;

        // Refined active state: slightly brighter card + accent border + glow
        // (no overpowering full-accent fill).
        const auto bg = active_                 ? Brand::btnIn().brighter (0.16f)
                      : (highlighted || down)   ? Brand::btnIn().brighter (0.10f)
                                                : Brand::btnIn();
        g.setColour (bg);
        g.fillRoundedRectangle (r, radius);

        if (active_)
        {
            g.setColour (Brand::accent().withAlpha (0.20f));
            g.drawRoundedRectangle (r.expanded (1.2f), radius + 1.0f, 2.4f);
        }
        g.setColour (active_ ? Brand::accent() : Brand::border().withAlpha (0.5f));
        g.drawRoundedRectangle (r, radius, active_ ? 1.8f : 1.0f);

        // Horizontal layout: [icon] gap [label], the pair centred in the tile.
        const float iconSize = juce::jmin (26.0f, r.getHeight() * 0.5f);
        const float gap      = 10.0f;
        const auto  font     = Brand::tech (Brand::Type::viewTileCaption, active_);
        const float textW    = font.getStringWidthFloat (getButtonText());
        const float groupW   = iconSize + gap + textW;
        const float startX   = r.getCentreX() - groupW * 0.5f;
        const float cy       = r.getCentreY();

        if (icon_ != nullptr)
        {
            auto iconArea = juce::Rectangle<float> (startX, cy - iconSize * 0.5f,
                                                    iconSize, iconSize);
            icon_->replaceColour (juce::Colours::white,
                                  active_ ? Brand::accent() : Brand::text());
            icon_->drawWithin (g, iconArea, juce::RectanglePlacement::centred, 1.0f);
        }

        auto labelArea = juce::Rectangle<float> (startX + iconSize + gap, r.getY(),
                                                 textW + 4.0f, r.getHeight());
        g.setColour (Brand::text());
        g.setFont (font);
        g.drawText (getButtonText(), labelArea.toNearestInt(),
                    juce::Justification::centredLeft, true);
    }

private:
    std::unique_ptr<juce::Drawable> icon_;
    bool active_ = false;
};

// ---------------------------------------------------------------------------
// StatusStrip — STATUS | Ready + last run + elapsed.
// ---------------------------------------------------------------------------
class StatusStrip : public juce::Component
{
public:
    void setStatus (const juce::String& state, bool ready = true)
    {
        state_ = state; ready_ = ready; repaint();
    }
    void setLastRun (const juce::String& t) { lastRun_ = t; repaint(); }
    void setElapsed (const juce::String& t) { elapsed_ = t; repaint(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());
        g.setColour (Brand::border().withAlpha (0.45f));
        g.drawHorizontalLine (0, 0.0f, (float) getWidth());

        const auto font = Brand::techMed (Brand::UI::scaledFont (Brand::Type::status));
        g.setFont (font);

        const int   edgePad = UiConfig::Scale::px (14);
        const int   third   = getWidth() / 3;
        const float dotR    = 3.5f * Brand::UI::scale;
        const float cy      = getHeight() * 0.5f;

        // Left: status dot + state text (vertically centred, common baseline).
        g.setColour (ready_ ? Brand::success() : Brand::warning());
        g.fillEllipse ((float) edgePad, cy - dotR, dotR * 2.0f, dotR * 2.0f);

        const int stateX = edgePad + (int) (dotR * 2.0f) + UiConfig::Scale::px (8);
        g.setColour (Brand::text());
        g.drawText (state_,
                    juce::Rectangle<int> (stateX, 0, third - stateX, getHeight()),
                    juce::Justification::centredLeft, true);

        // Centre: last run — evenly distributed across the middle third.
        g.setColour (Brand::text().withAlpha (0.78f));
        g.drawText (lastRun_,
                    getLocalBounds().withTrimmedLeft (third).withTrimmedRight (third),
                    juce::Justification::centred, true);

        // Right: elapsed — same padding from the right edge as the left.
        g.drawText (elapsed_,
                    getLocalBounds().withTrimmedLeft (2 * third).withTrimmedRight (edgePad),
                    juce::Justification::centredRight, true);
    }

private:
    juce::String state_   { "Ready" };
    juce::String lastRun_ { "Last run: -" };
    juce::String elapsed_ { "Elapsed: -" };
    bool ready_ = true;
};

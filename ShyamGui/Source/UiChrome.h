#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include <functional>
#include <utility>

// ---------------------------------------------------------------------------
// SectionHeader â€” collapsible sidebar section title with chevron.
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

    void setTitle (const juce::String& t) { title_ = t; repaint(); }
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
// ParamBar â€” plain-text simulation readouts (top bar, no chip boxes).
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
// PlotHeaderBar â€” workspace title + navigation toolbar.
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
        title_.setInterceptsMouseClicks (false, false);
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

        // These five ship as reference art in /Icons (flat, pre-coloured glyphs,
        // not recolourable outlines), so load them from file instead of the
        // hand-drawn inline SVG paths used for the rest of the toolbar.
        auto styleToolFile = [&] (juce::DrawableButton& b, const juce::String& fileName,
                                  const juce::String& tip, bool toggle = false)
        {
            b.setTooltip (tip);
            b.setColour (juce::DrawableButton::backgroundColourId,   Brand::plotToolbar());
            b.setColour (juce::DrawableButton::backgroundOnColourId, Brand::accent().withAlpha (0.28f));
            auto icon = loadToolIcon (fileName);
            b.setImages (icon.get());
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

        styleToolFile (btnSelect_, "Cursor",
                       "Select: click a drawing to move it; hover to read SPL; drag empty space to pan", true);
        styleToolFile (btnPan_,     "Pan-nav-hand.svg", "Pan: drag to move the view", true);
        styleToolFile (btnPencil_,  "Pencil.svg",       "Pencil: choose a colour, then draw freehand", true);
        styleToolFile (btnEraser_,  "Eraser.svg",       "Eraser: scrub to remove drawings", true);
        styleToolFile (btnRuler_,   "Ruler.svg",        "Ruler: click two points to measure distance", true);
        styleTool (btnShape_,   kShapeSVG,   "Shape: Line, Polyline, Circle, Arc, Rectangle, Square, or Text Box", true);
        styleTool (btnMic_,     kMicSVG,     "Mic: add virtual receivers on the gradient plot", false);
        styleToolFile (btnZoomIn_,  "Zoom In.svg",  "Zoom in");
        styleToolFile (btnZoomOut_, "Zoom Out.svg", "Zoom out");

        btnSelect_.setToggleState (true, juce::dontSendNotification);

        // File cluster â€” Figma-exported PNGs, on a light chip so the (dark-ink)
        // icons stay visible on both themes' ribbon fill. Swap for in-house
        // vector icons later; wiring is real (Project menu actions) now.
        auto styleFileIcon = [&] (juce::DrawableButton& b, const juce::String& pngName,
                                  const juce::String& tip, std::function<void()>* onClickSlot)
        {
            b.setTooltip (tip);
            // Figma draws these as plain dark glyphs on the ribbon, no chip.
            b.setColour (juce::DrawableButton::backgroundColourId, juce::Colours::transparentBlack);
            b.setColour (juce::DrawableButton::backgroundOnColourId, Brand::accent().withAlpha (0.35f));
            if (auto img = Brand::assetImage ("FigmaRedesign/" + pngName); img.isValid())
            {
                juce::DrawableImage d;
                d.setImage (img);
                b.setImages (&d);
            }
            b.setEdgeIndent (5);
            b.setOpaque (false);
            addAndMakeVisible (b);
            b.onClick = [onClickSlot] { if (onClickSlot != nullptr && *onClickSlot) (*onClickSlot)(); };
        };
        styleFileIcon (btnFileNew_,    "figma_icon_file_add_new.png",     "New Project", &onFileNew);
        styleFileIcon (btnFileOpen_,   "figma_icon_file_open_folder.png", "Open Project", &onFileOpen);
        styleFileIcon (btnFileSave_,   "figma_icon_file_save.png",        "Save Project (Ctrl+S)", &onFileSave);
        styleFileIcon (btnFileSaveAs_, "figma_icon_file_save_as.png",     "Save Project As...", &onFileSaveAs);
        styleFileIcon (btnFileExport_, "figma_icon_file_export_pdf.png",  "Export PDF Report", &onFileExport);

        auto styleClusterLabel = [&] (juce::Label& l, const juce::String& text)
        {
            l.setText (text, juce::dontSendNotification);
            l.setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::ribbonClusterLabel)));
            l.setColour (juce::Label::textColourId, Brand::text());
            l.setJustificationType (juce::Justification::centred);
            l.setBorderSize ({});
            // Opt out of drawFittedText's auto-shrink so the caption renders at
            // the Figma size instead of being squeezed to ~half of it.
            l.setMinimumHorizontalScale (1.0f);
            l.setInterceptsMouseClicks (false, false);
            addAndMakeVisible (l);
        };
        styleClusterLabel (lblFile_,    "File");
        styleClusterLabel (lblNav_,     "Navigation");
        styleClusterLabel (lblView_,    "View");
        styleClusterLabel (lblTools_,   "Tools");
        styleClusterLabel (lblShapes_,  "Shapes");
        styleClusterLabel (lblColours_, "Colours");
        styleClusterLabel (lblHelp_,    "Help");
        styleClusterLabel (lblOptions_, "Options");

        // Colours: the exact Figma palette, top row then bottom row.
        static const juce::uint32 kPalette[kSwatchCount] = {
            0xffed2227, 0xff145fea, 0xff313131, 0xffffffff,   // red, blue, dark, white
            0xfff09a9c, 0xff85a9ed, 0xff9f9f9f, 0xff99dea5    // pink, light blue, grey, green
        };
        for (int i = 0; i < kSwatchCount; ++i)
        {
            auto& s = swatches_[i];
            s.colour   = juce::Colour (kPalette[i]);
            s.outlined = (kPalette[i] == 0xffffffff);
            s.setTooltip ("Draw colour");
            s.onClick = [this, c = s.colour] { if (onSwatchPicked) onSwatchPicked (c); };
            addAndMakeVisible (s);
        }

        // View: icon for Fit View (Figma shows an icon, not a text button).
        // It simply drives the original fitBtn_, which MainComponent wires.
        styleToolFile (btnFitView_, "Full Screen.svg", "Fit the full field in view");
        btnFitView_.onClick = [this] { fitBtn_.triggerClick(); };

        // Shapes: one icon per shape, like the Figma mock, instead of a single
        // button that opens a construction menu. Each picks the shape with its
        // default construction via the same callback the menu used.
        styleToolFile (btnShapeLine_,     "Shape Line",     "Line: click two points",         true);
        styleToolFile (btnShapePolyline_, "Shape Polyline", "Polyline: click point to point", true);
        styleToolFile (btnShapeArc_,      "Shape Arc",      "Arc: click three points",        true);
        styleToolFile (btnShapeCircle_,   "Shape Circle",   "Circle: centre, then radius",    true);
        styleToolFile (btnShapeRect_,     "Shape Rect",     "Rectangle: click two corners",   true);
        styleToolFile (btnShapeText_,     "Shape Text",     "Text box: click to place",       true);

        auto pickShape = [this] (int shapeId, int constructionId)
        {
            if (onShapeChosen) onShapeChosen (shapeId, constructionId);
        };
        btnShapeLine_.onClick     = [pickShape] { pickShape (0, 0); };   // Line, 2 points
        btnShapePolyline_.onClick = [pickShape] { pickShape (1, 2); };   // Polyline, point-to-point
        btnShapeArc_.onClick      = [pickShape] { pickShape (3, 6); };   // Arc, 3 points
        btnShapeCircle_.onClick   = [pickShape] { pickShape (2, 4); };   // Circle, centre+radius
        btnShapeRect_.onClick     = [pickShape] { pickShape (4, 7); };   // Rectangle, 2 corners
        btnShapeText_.onClick     = [pickShape] { pickShape (6, 9); };   // Text box, click

        auto styleMod = [&] (juce::TextButton& b, const juce::String& tip)
        {
            b.setTooltip (tip);
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
            b.setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.35f));
            b.setColour (juce::TextButton::textColourOffId,  AppSettings::get().isDark() ? Brand::white()
                                                                                           : Brand::onBtnIn());
            b.setColour (juce::TextButton::textColourOnId,   AppSettings::get().isDark() ? Brand::white()
                                                                                           : Brand::onBtnIn());
            addAndMakeVisible (b);
        };
        styleMod (btnOrtho_,
                  "Ortho: align selected speakers horizontally or vertically "
                  "with linked centre-to-centre spacing (Figma-style). Off = normal.");
        styleMod (btnSnap_,
                  "Snap: lock to " + juce::String (Units::snapStepLabel())
                      + " steps and to nearby edges/corners "
                        "(other shapes, mics, speakers). Turn on before drawing or moving.");
        styleMod (btnSplProbe_,
                  "SPL: show dB SPL under the cursor on the gradient plot "
                  "(Select tool). Turn off for a cleaner view.");
        btnSplProbe_.setToggleState (false, juce::dontSendNotification);

        auto styleOrthoOpt = [&] (juce::TextButton& b, const juce::String& tip)
        {
            b.setTooltip (tip);
            b.setClickingTogglesState (true);
            b.setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
            b.setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.45f));
            b.setColour (juce::TextButton::textColourOffId,  AppSettings::get().isDark() ? Brand::white()
                                                                                           : Brand::onBtnIn());
            b.setColour (juce::TextButton::textColourOnId,   AppSettings::get().isDark() ? Brand::white()
                                                                                           : Brand::onBtnIn());
            b.setVisible (false);
            addChildComponent (b);
        };
        styleOrthoOpt (btnOrthoHoriz_, "Align selected speakers in a horizontal row");
        styleOrthoOpt (btnOrthoVert_,  "Align selected speakers in a vertical column");
        btnOrthoHoriz_.setToggleState (true, juce::dontSendNotification);

        orthoGapLabel_.setText ("Gap", juce::dontSendNotification);
        orthoGapLabel_.setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::plotToolbarLabel)));
        orthoGapLabel_.setColour (juce::Label::textColourId, Brand::muted());
        orthoGapLabel_.setJustificationType (juce::Justification::centredRight);
        orthoGapLabel_.setBorderSize ({});
        orthoGapLabel_.setInterceptsMouseClicks (false, false);
        orthoGapLabel_.setVisible (false);
        addChildComponent (orthoGapLabel_);

        orthoGapSlider_.setRange (0.5, 20.0, 0.1);
        orthoGapSlider_.setValue (3.0, juce::dontSendNotification);
        orthoGapSlider_.setSliderStyle (juce::Slider::LinearHorizontal);
        orthoGapSlider_.setTextBoxStyle (juce::Slider::TextBoxRight, false, 44, 18);
        orthoGapSlider_.setNumDecimalPlacesToDisplay (1);
        orthoGapSlider_.setTextValueSuffix (" m");
        orthoGapSlider_.textFromValueFunction =
            [] (double metres) { return juce::String (Units::metresToDisplay (metres), 1); };
        orthoGapSlider_.valueFromTextFunction =
            [] (const juce::String& t) { return Units::displayToMetres (t.getDoubleValue()); };
        orthoGapSlider_.setTooltip ("Linked centre-to-centre spacing â€” changing it updates all gaps");
        orthoGapSlider_.setColour (juce::Slider::trackColourId, Brand::border());
        orthoGapSlider_.setColour (juce::Slider::thumbColourId, Brand::accent());
        orthoGapSlider_.setColour (juce::Slider::backgroundColourId, Brand::plotToolbar());
        orthoGapSlider_.setColour (juce::Slider::textBoxTextColourId, Brand::muted());
        orthoGapSlider_.setColour (juce::Slider::textBoxBackgroundColourId, Brand::plotToolbar());
        orthoGapSlider_.setColour (juce::Slider::textBoxOutlineColourId, Brand::border().withAlpha (0.35f));
        orthoGapSlider_.setVisible (false);
        addChildComponent (orthoGapSlider_);

        // H / V are mutually exclusive while Ortho is on
        btnOrthoHoriz_.onClick = [this]
        {
            if (btnOrthoHoriz_.getToggleState())
                btnOrthoVert_.setToggleState (false, juce::dontSendNotification);
            else
                btnOrthoHoriz_.setToggleState (true, juce::dontSendNotification);
            if (onOrthoOptionsChanged) onOrthoOptionsChanged();
        };
        btnOrthoVert_.onClick = [this]
        {
            if (btnOrthoVert_.getToggleState())
                btnOrthoHoriz_.setToggleState (false, juce::dontSendNotification);
            else
                btnOrthoVert_.setToggleState (true, juce::dontSendNotification);
            if (onOrthoOptionsChanged) onOrthoOptionsChanged();
        };
        orthoGapSlider_.onValueChange = [this]
        {
            if (onOrthoOptionsChanged) onOrthoOptionsChanged();
        };

        promptLabel_.setText ({}, juce::dontSendNotification);
        promptLabel_.setFont (Brand::mono (Brand::UI::scaledFont (Brand::Type::plotToolbarLabel)));
        promptLabel_.setColour (juce::Label::textColourId, Brand::accent());
        promptLabel_.setJustificationType (juce::Justification::centred);
        promptLabel_.setBorderSize ({});
        promptLabel_.setMinimumHorizontalScale (0.7f);
        addAndMakeVisible (promptLabel_);

        colourSwatch_.setTooltip ("Fill / stroke colour");
        colourSwatch_.setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        colourSwatch_.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        colourSwatch_.setColour (juce::TextButton::textColourOffId,  juce::Colours::transparentBlack);
        colourSwatch_.setButtonText ({});
        addAndMakeVisible (colourSwatch_);

        // Figma's ribbon puts Opacity in its own slot between Tools and Shapes:
        // "Opacity" top-left, the percentage top-right, the track underneath.
        alphaLabel_.setText ("Opacity", juce::dontSendNotification);
        alphaLabel_.setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::ribbonOpacityLabel)));
        alphaLabel_.setColour (juce::Label::textColourId, Brand::text());
        alphaLabel_.setJustificationType (juce::Justification::centredLeft);
        alphaLabel_.setBorderSize ({});
        alphaLabel_.setMinimumHorizontalScale (1.0f);
        alphaLabel_.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (alphaLabel_);

        alphaValueLabel_.setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::ribbonOpacityLabel)));
        alphaValueLabel_.setColour (juce::Label::textColourId, Brand::text());
        alphaValueLabel_.setJustificationType (juce::Justification::centredRight);
        alphaValueLabel_.setBorderSize ({});
        alphaValueLabel_.setMinimumHorizontalScale (1.0f);
        alphaValueLabel_.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (alphaValueLabel_);

        fillAlpha_.setRange (0.0, 100.0, 1.0);
        fillAlpha_.setValue (35.0, juce::dontSendNotification);
        fillAlpha_.setSliderStyle (juce::Slider::LinearHorizontal);
        // The percentage is its own top-right label in the Figma layout, so the
        // slider itself carries no text box.
        fillAlpha_.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        fillAlpha_.setNumDecimalPlacesToDisplay (0);
        // Owned internally so the percentage always tracks the slider. This
        // used to be the hook MainComponent assigned to, which replaced it and
        // left the readout frozen at its initial value; external code sets
        // onFillAlphaChanged instead.
        fillAlpha_.onValueChange = [this]
        {
            syncAlphaReadout();
            if (onFillAlphaChanged) onFillAlphaChanged();
        };
        fillAlpha_.setTooltip ("Opacity of the selected shape / text-box background "
                               "(or of the next shape you draw if nothing is selected)");
        fillAlpha_.setColour (juce::Slider::trackColourId, Brand::border());
        fillAlpha_.setColour (juce::Slider::thumbColourId, Brand::accent());
        fillAlpha_.setColour (juce::Slider::backgroundColourId, Brand::plotToolbar());
        fillAlpha_.setColour (juce::Slider::textBoxTextColourId, Brand::muted());
        fillAlpha_.setColour (juce::Slider::textBoxBackgroundColourId, Brand::plotToolbar());
        fillAlpha_.setColour (juce::Slider::textBoxOutlineColourId, Brand::border().withAlpha (0.35f));
        addAndMakeVisible (fillAlpha_);
        syncAlphaReadout();

        fitBtn_.setButtonText ("Fit View");
        fitBtn_.setComponentID ("plotFit");
        fitBtn_.setTooltip ("Fit the full field in view");
        fitBtn_.setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
        fitBtn_.setColour (juce::TextButton::textColourOffId,  AppSettings::get().isDark() ? Brand::white()
                                                                                           : Brand::onBtnIn());
        addAndMakeVisible (fitBtn_);

        rangeBtn_.setButtonText ("Range");
        rangeBtn_.setComponentID ("plotRange");
        rangeBtn_.setTooltip ("Show or hide 1 m, 2 m, 4 m, and 8 m range markers");
        rangeBtn_.setClickingTogglesState (true);
        rangeBtn_.setToggleState (false, juce::dontSendNotification);
        rangeBtn_.setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
        rangeBtn_.setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.35f));
        rangeBtn_.setColour (juce::TextButton::textColourOffId,  AppSettings::get().isDark() ? Brand::white()
                                                                                             : Brand::onBtnIn());
        rangeBtn_.setColour (juce::TextButton::textColourOnId,   AppSettings::get().isDark() ? Brand::white()
                                                                                             : Brand::onBtnIn());
        addAndMakeVisible (rangeBtn_);

        for (auto* tb : { &btnSelect_, &btnPan_, &btnPencil_, &btnEraser_,
                          &btnRuler_, &btnShape_, &btnMic_, &btnZoomIn_, &btnZoomOut_ })
            tb->toFront (false);
        for (auto* fb : { &btnFileNew_, &btnFileOpen_, &btnFileSave_, &btnFileSaveAs_, &btnFileExport_ })
            fb->toFront (false);
        btnOrtho_.toFront (false);
        btnOrthoHoriz_.toFront (false);
        btnOrthoVert_.toFront (false);
        orthoGapLabel_.toFront (false);
        orthoGapSlider_.toFront (false);
        btnSnap_.toFront (false);
        btnSplProbe_.toFront (false);
        colourSwatch_.toFront (false);
        alphaLabel_.toFront (false);
        fillAlpha_.toFront (false);
        rangeBtn_.toFront (false);
        fitBtn_.toFront (false);
        promptLabel_.toFront (false);
    }

    void setTitle (const juce::String& t) { title_.setText (t, juce::dontSendNotification); }

    void setDrawPrompt (const juce::String& p)
    {
        promptLabel_.setText (p, juce::dontSendNotification);
    }

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
        syncAlphaReadout();
    }

    float getFillAlpha01() const noexcept
    {
        return (float) (fillAlpha_.getValue() / 100.0);
    }

    enum class ActiveTool { Select, Pan, Pencil, Eraser, Ruler, Shape };
    void setActiveTool (ActiveTool t)
    {
        btnSelect_.setToggleState (t == ActiveTool::Select, juce::dontSendNotification);
        btnPan_.setToggleState    (t == ActiveTool::Pan,    juce::dontSendNotification);
        btnPencil_.setToggleState (t == ActiveTool::Pencil, juce::dontSendNotification);
        btnEraser_.setToggleState (t == ActiveTool::Eraser, juce::dontSendNotification);
        btnRuler_.setToggleState  (t == ActiveTool::Ruler,  juce::dontSendNotification);
        btnShape_.setToggleState  (t == ActiveTool::Shape,  juce::dontSendNotification);
        // Shapes are now five separate buttons: leaving the Shape tool clears
        // them all; entering it leaves whichever shape the user picked lit.
        if (t != ActiveTool::Shape)
            for (auto* sb : shapeButtons())
                sb->setToggleState (false, juce::dontSendNotification);
    }

    void showShapeMenu (std::function<void (int shapeId, int constructionId)> onPick)
    {
        juce::PopupMenu root;
        auto addShape = [&] (const juce::String& name, int shapeId,
                             std::initializer_list<std::pair<const char*, int>> methods)
        {
            juce::PopupMenu sub;
            for (const auto& m : methods)
                sub.addItem (1000 + shapeId * 100 + m.second, m.first);
            root.addSubMenu (name, sub);
        };

        addShape ("Line",      0, { { "2 Points", 0 }, { "Horizontal/Vertical", 1 } });
        addShape ("Polyline",  1, { { "Point-to-Point", 2 }, { "Closed", 3 } });
        addShape ("Circle",    2, { { "Center + Radius", 4 }, { "2 Points (diameter)", 5 } });
        addShape ("Arc",       3, { { "3 Points", 6 } });
        addShape ("Rectangle", 4, { { "2 Corners", 7 } });
        addShape ("Square",    5, { { "2 Corners", 8 } });
        // Text Box: one click â€” no construction submenu.
        root.addItem (1000 + 6 * 100 + 9, "Text Box");

        root.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&btnShape_),
            [onPick] (int result)
            {
                if (result <= 0 || onPick == nullptr) return;
                const int enc = result - 1000;
                onPick (enc / 100, enc % 100);
            });
    }

    void setMicArmed (bool armed)
    {
        btnMic_.setToggleState (armed, juce::dontSendNotification);
        btnMic_.setColour (juce::DrawableButton::backgroundOnColourId,
                           armed ? Brand::accent().withAlpha (0.45f)
                                 : Brand::accent().withAlpha (0.28f));
        btnMic_.setTooltip (armed
            ? "Mic: placement armed â€” click the field to place (Esc cancels)"
            : "Mic: add virtual receivers on the gradient plot");
        btnMic_.repaint();
    }

    void showMicMenu (std::function<void (int itemId)> onPick, bool hasMics,
                      bool showDegrees)
    {
        juce::PopupMenu root;
        root.addItem (1, "Add Mic");
        root.addItem (2, "Place on ring", hasMics);
        root.addItem (3, "Show Degrees", true, showDegrees);
        root.addItem (4, "Show Frequency Response", hasMics);
        root.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&btnMic_),
            [onPick] (int result)
            {
                if (result > 0 && onPick != nullptr)
                    onPick (result);
            });
    }

    juce::DrawableButton btnSelect_ { "sel", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnPan_    { "pan", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnPencil_ { "pen", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnEraser_ { "ers", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnRuler_  { "rul", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnShape_  { "shp", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnMic_    { "mic", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnZoomIn_ { "zi",  juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnZoomOut_{ "zo",  juce::DrawableButton::ImageFitted };
    /** Master switch for the "Options" cluster (Snap / Ortho) that sits after
        Help in the ribbon. The Figma mock has no slot for it, so it is laid
        out but not shown: flip this to true and the cluster appears in its
        proper place, captioned and divided like every other cluster. All the
        behaviour behind it stays live either way -- Ctrl+F, the SNAP / ORTHO
        terminal verbs and MainComponent's handlers drive the same buttons. */
    static constexpr bool kShowOptionsCluster = false;

    /** Re-applies the ribbon's edge indent to the adopted Help glyphs.
        MainComponent::refreshHeaderIcons() restyles them from scratch and
        stamps the header's own inset back on, which leaves them visibly
        smaller than every other glyph in the row. resized() normally corrects
        that, but JUCE only calls it when the bounds actually change -- and a
        units or theme switch changes neither -- so the shrunken icons stuck.
        Call this whenever those buttons are restyled. */
    void refreshHelpIconMetrics()
    {
        for (auto* hc : helpIcons_)
            if (auto* hb = dynamic_cast<juce::DrawableButton*> (hc))
                hb->setEdgeIndent (0);
    }

    void setOrthoExtrasVisible (bool on)
    {
        // H / V / Gap only ever show alongside the cluster they belong to.
        const bool show = on && kShowOptionsCluster;
        btnOrthoHoriz_.setVisible (show);
        btnOrthoVert_.setVisible (show);
        orthoGapLabel_.setVisible (show);
        orthoGapSlider_.setVisible (show);
        resized();
    }

    bool isOrthoHorizontal() const noexcept { return btnOrthoHoriz_.getToggleState(); }
    void setOrthoSpacingM (double m)
    {
        orthoGapSlider_.setValue (m, juce::dontSendNotification);
    }
    double getOrthoSpacingM() const noexcept { return orthoGapSlider_.getValue(); }

    void refreshUnits()
    {
        orthoGapSlider_.textFromValueFunction =
            [] (double metres) { return juce::String (Units::metresToDisplay (metres), 1); };
        orthoGapSlider_.valueFromTextFunction =
            [] (const juce::String& t) { return Units::displayToMetres (t.getDoubleValue()); };
        orthoGapSlider_.setTextValueSuffix (" " + juce::String (Units::lengthUnit()));
        orthoGapSlider_.updateText();
        btnSnap_.setTooltip (
            "Snap: lock to " + juce::String (Units::snapStepLabel())
                + " steps and to nearby edges/corners "
                  "(other shapes, mics, speakers). Turn on before drawing or moving.");
    }

    std::function<void()> onOrthoOptionsChanged;
    /** Fired when the Opacity slider moves. Use this rather than reaching into
        fillAlpha_.onValueChange, which the bar needs for its own readout. */
    std::function<void()> onFillAlphaChanged;

    /** Greys the Opacity slot out when it has nothing to act on, so it never
        looks live while dragging it would do nothing. */
    void setFillAlphaEnabled (bool on)
    {
        if (fillAlphaEnabled_ == on) return;
        fillAlphaEnabled_ = on;
        fillAlpha_.setEnabled (on);
        const float a = on ? 1.0f : 0.40f;
        alphaLabel_.setAlpha (a);
        alphaValueLabel_.setAlpha (a);
        fillAlpha_.setAlpha (on ? 1.0f : 0.55f);
        fillAlpha_.setTooltip (on ? "Opacity of the selected shape / text-box background "
                                    "(or of the next shape you draw)"
                                  : "Opacity applies to filled shapes - select one, "
                                    "or pick Circle / Rectangle / Text box first");
        repaint();
    }
    /** Figma Shapes cluster: (shapeId, constructionId) as used by showShapeMenu. */
    std::function<void (int shapeId, int constructionId)> onShapeChosen;
    /** Figma Colours cluster: one of the 8 palette dots was clicked. */
    std::function<void (juce::Colour)> onSwatchPicked;

    juce::TextButton     btnOrtho_  { "Ortho" };
    juce::TextButton     btnOrthoHoriz_ { "H" };
    juce::TextButton     btnOrthoVert_  { "V" };
    juce::Label          orthoGapLabel_;
    juce::Slider         orthoGapSlider_;
    juce::TextButton     btnSnap_   { "Snap" };
    juce::TextButton     btnSplProbe_ { "SPL" };
    juce::Label          promptLabel_;
    juce::TextButton     colourSwatch_;
    juce::Label          alphaLabel_;
    juce::Label          alphaValueLabel_;   // "52%" â€” top-right of the Opacity slot
    juce::Slider         fillAlpha_;
    bool                 fillAlphaEnabled_ = true;
    void syncAlphaReadout()
    {
        alphaValueLabel_.setText (juce::String ((int) fillAlpha_.getValue()) + "%",
                                  juce::dontSendNotification);
    }
    juce::TextButton     fitBtn_;
    juce::TextButton     rangeBtn_;

    // Figma redesign: File cluster (New/Open/Save/Save As/Export) â€” no existing
    // in-app icons for these, so reuse the Figma-exported PNGs as-is for now.
    juce::DrawableButton btnFileNew_    { "fnew",    juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnFileOpen_   { "fopen",   juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnFileSave_   { "fsave",   juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnFileSaveAs_ { "fsaveas", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnFileExport_ { "fexport", juce::DrawableButton::ImageFitted };
    std::function<void()> onFileNew, onFileOpen, onFileSave, onFileSaveAs, onFileExport;

    // View cluster: Figma shows an icon for "fit / full screen", not a text
    // button. btnFitView_ is that icon; it just triggers the original fitBtn_
    // so MainComponent's existing wiring keeps working untouched.
    juce::DrawableButton btnFitView_ { "fit", juce::DrawableButton::ImageFitted };

    // Shapes cluster: Figma shows the shape tools as separate icons rather
    // than one button that opens a construction menu.
    static constexpr int kShapeButtonCount = 6;
    juce::DrawableButton btnShapeLine_     { "sline", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnShapePolyline_ { "spoly", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnShapeArc_      { "sarc",  juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnShapeCircle_   { "scirc", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnShapeRect_     { "srect", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnShapeText_     { "stext", juce::DrawableButton::ImageFitted };
    std::array<juce::DrawableButton*, kShapeButtonCount> shapeButtons() noexcept
    {
        return { &btnShapeLine_, &btnShapePolyline_, &btnShapeArc_,
                 &btnShapeCircle_, &btnShapeRect_, &btnShapeText_ };
    }

    // Colours cluster: the Figma palette, 8 dots in a 4x2 grid. Hexes are the
    // exact fills read off the Figma nodes (see docs/figma-redesign).
    struct SwatchButton : public juce::Button
    {
        SwatchButton() : juce::Button ({}) {}
        juce::Colour colour { juce::Colours::black };
        bool outlined = false;   // the white dot needs a visible edge
        void paintButton (juce::Graphics& g, bool highlighted, bool down) override
        {
            auto r = getLocalBounds().toFloat();
            if (highlighted || down) r = r.expanded (0.5f);
            g.setColour (colour);
            g.fillEllipse (r);
            if (outlined || highlighted || down)
            {
                g.setColour (outlined ? juce::Colours::black : Brand::accent());
                g.drawEllipse (r.reduced (0.5f), highlighted || down ? 1.5f : 0.6f);
            }
        }
    };
    static constexpr int kSwatchCount = 8;
    SwatchButton swatches_[kSwatchCount];

    // Cluster labels, drawn under each icon group: Figma's File / Navigation /
    // View / Tools / Shapes / Colours / Help, plus one trailing "Options"
    // cluster for app-specific controls Figma's mock doesn't have.
    juce::Label lblFile_, lblNav_, lblView_, lblTools_, lblShapes_, lblColours_, lblHelp_, lblOptions_;
    std::vector<int> dividerX_;

    // Help cluster icons are owned by MainComponent (existing Info/Settings/
    // Help badge buttons, reparented here) but laid out as a proper labeled
    // cluster like every other group â€” set once via setHelpIcons().
    juce::Component* helpIcons_[3] = { nullptr, nullptr, nullptr };

    // "Ready" pill (also owned by MainComponent, reparented) â€” sits at the
    // far right of this same ribbon row, exactly matching the Figma mock
    // (it is NOT in the title row above).
    juce::Component* readyPill_ = nullptr;
    std::function<int()> readyPillWidth_;

public:
    /** Left-to-right, in the Figma mock's order: settings gear, info, help. */
    void setHelpIcons (juce::Component& first, juce::Component& second, juce::Component& third)
    {
        helpIcons_[0] = &first; helpIcons_[1] = &second; helpIcons_[2] = &third;
        for (auto* c : helpIcons_) addAndMakeVisible (*c);
        resized();
    }
    void setReadyPill (juce::Component& pill, std::function<int()> preferredWidth = {})
    {
        readyPill_ = &pill;
        readyPillWidth_ = std::move (preferredWidth);
        addAndMakeVisible (pill);
        resized();
    }

    /** Re-lays the ribbon after the status text changes, so the pill can
        resize to fit it. */
    void refreshStatusLayout() { resized(); }

    /** The red "SPL Heatmap | ... " caption. In the Figma design this sits at
        the canvas's top-left, NOT in the ribbon, so MainComponent reparents it
        and positions it over the plot â€” setTitle() keeps working either way. */
    juce::Label& getTitleLabel() noexcept { return title_; }
private:

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panelDark());

        // Figma: cluster rules run the full height of the row, and the row is
        // closed by a hairline along its bottom edge.
        g.setColour (Brand::border().withAlpha (0.45f));
        for (int x : dividerX_)
            g.drawVerticalLine (x, 0.0f, (float) getHeight());
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
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
        namespace L = UiConfig::Layout;

        // Anything the Figma ribbon has no slot for is hidden (code/logic
        // untouched â€” flip setVisible back on to restore). The red
        // "SPL Heatmap | ..." caption belongs on the canvas, not here, so
        // MainComponent reparents title_ via getTitleLabel().
        for (juce::Component* c : { (juce::Component*) &btnSplProbe_,
                                     (juce::Component*) &rangeBtn_,
                                     (juce::Component*) &btnMic_,
                                     // superseded by their Figma-shaped equivalents
                                     (juce::Component*) &btnShape_,      // -> 6 shape icons
                                     (juce::Component*) &fitBtn_,        // -> btnFitView_ icon
                                     // fill-colour preview swatch: not in the mock
                                     (juce::Component*) &colourSwatch_ })
        {
            c->setVisible (false);
            c->setBounds (0, 0, 0, 0);
        }

        // Figma row 2 geometry: 24px icons on a 30px pitch, 11px below the row
        // top; cluster captions on a 17px row 44px below the row top; clusters
        // start 20px from the left and are separated by a full-height rule with
        // 12px of air either side. Everything scales together, and shrinks
        // proportionally (never below a legible floor) on narrow windows.
        const int fullIcon    = UiConfig::Scale::px (L::ribbonIconSize);
        const int fullGap     = UiConfig::Scale::px (L::ribbonIconGap);
        const int fullPad     = UiConfig::Scale::px (L::ribbonClusterPad);
        const int fullEdge    = UiConfig::Scale::px (L::ribbonEdgePad);
        const int fullSwatch  = UiConfig::Scale::px (L::ribbonSwatch);
        const int fullPitchX  = UiConfig::Scale::px (L::ribbonSwatchPitchX);
        const int fullReadyPad = UiConfig::Scale::px (L::ribbonReadyRightPad);

        const bool helpPresent = helpIcons_[0] != nullptr;

        // Every cluster's icon-start x and its closing divider come straight
        // from the Figma mock (px / 1.317), so the row is exact by construction
        // rather than by accumulating pads â€” flow layout drifted further right
        // with each cluster. `shrink` scales the whole row down together when
        // the window is narrower than the design.
        struct Cluster { int iconX, dividerX; };
        static constexpr Cluster kFile    { 22,  138 };   // Figma  29 / 182
        static constexpr Cluster kNav     { 152, 207 };   //       200 / 272
        static constexpr Cluster kView    { 211, 280 };   //       278 / 369
        static constexpr Cluster kTools   { 287, 358 };   //       378 / 471
        // Opacity needs more room than the Figma mock gave it: at a legible
        // caption size "Opacity" and "100%" do not both fit in the original
        // 56-unit slot, so the caption lost its last letter and the value lost
        // its % sign. Widened by 60 units and everything after it shifted by
        // the same amount -- the row had empty space to spare to the right of
        // Help, so nothing is pushed off the end.
        static constexpr Cluster kOpacity { 366, 488 };   // Opacity slot (Tools|Shapes)
        static constexpr Cluster kShapes  { 494, 638 };   // Figma 478 / 668 + 60
        static constexpr Cluster kColours { 643, 703 };   //       675 / 754 + 60
        static constexpr Cluster kHelp    { 706, 774 };   //       758 / 847 + 60
        // Options (Snap / Ortho) continues the row past Help on the same
        // rhythm the mock uses elsewhere: 14 units of air after the preceding
        // divider, then the controls, then 14 more before the next rule.
        static constexpr Cluster kOptions { 728, 852 };
        static constexpr int kOptionPillW = 50;           // "Snap" / "Ortho" pills
        static constexpr int kOptionPillGap = 6;
        static constexpr int kPlateLeft = 15;             // Figma  20
        static constexpr int kPitch     = 23;             // Figma  30 icon pitch

        const int lastDividerX = kShowOptionsCluster ? kOptions.dividerX : kHelp.dividerX;
        const int designW = UiConfig::Scale::px (lastDividerX + L::ribbonReadyRightPad + 100);
        const float shrink = juce::jlimit (0.45f, 1.0f,
                                           designW > 0 ? (float) getWidth() / (float) designW : 1.0f);
        auto px2 = [&] (int v) { return juce::jmax (1, juce::roundToInt ((float) UiConfig::Scale::px (v) * shrink)); };

        const int tool       = juce::jmax (12, px2 (L::ribbonIconSize));
        const int pitch      = juce::jmax (tool + 1, px2 (kPitch));
        const int iconTop    = juce::jmax (2, px2 (L::ribbonIconTop));
        const int labelTop   = juce::jmax (tool + 2, px2 (L::ribbonLabelTop));
        const int labelH     = juce::jmax (8, px2 (L::ribbonLabelH));
        // Colours palette: 14px dots on a 20x16 grid in the Figma mock.
        const int swatch     = juce::jmax (6, px2 (L::ribbonSwatch));
        const int pitchX     = juce::jmax (swatch + 1, px2 (L::ribbonSwatchPitchX));
        const int pitchY     = juce::jmax (swatch + 1, px2 (L::ribbonSwatchPitchY));
        // Figma's glyphs are 24x24 in a 24px slot â€” no inset, so the artwork
        // fills the button rather than sitting in a shrunken well.
        const int edgeIndent = 0;

        for (auto* tb : { &btnSelect_, &btnPan_, &btnPencil_, &btnEraser_,
                          &btnRuler_, &btnZoomIn_, &btnZoomOut_, &btnFitView_ })
            tb->setEdgeIndent (edgeIndent);
        for (auto* fb : { &btnFileNew_, &btnFileOpen_, &btnFileSave_, &btnFileSaveAs_, &btnFileExport_ })
            fb->setEdgeIndent (edgeIndent);
        for (auto* sb : shapeButtons())
            sb->setEdgeIndent (edgeIndent);
        // Help's gear/info/? are MainComponent's buttons; they arrive with the
        // header's own inset, which would leave them smaller than every other
        // glyph in the row.
        for (auto* hc : helpIcons_)
            if (auto* hb = dynamic_cast<juce::DrawableButton*> (hc))
                hb->setEdgeIndent (edgeIndent);

        dividerX_.clear();

        // Captions are sized here, not in the constructor: at construction time
        // Brand::UI::scale is still 1.0, so a font set there stays at its base
        // size and renders roughly half as large as the design calls for.
        {
            const auto capFont = Brand::tech (Brand::UI::scaledFont (Brand::Type::ribbonClusterLabel));
            for (auto* l : { &lblFile_, &lblNav_, &lblView_, &lblTools_,
                             &lblShapes_, &lblColours_, &lblHelp_, &lblOptions_ })
                l->setFont (capFont);
        }

        // Lays a cluster's icons on the Figma pitch from its own start x, then
        // centres the caption on the plate between the surrounding dividers â€”
        // which is how the mock aligns them (e.g. "Navigation" is centred on
        // 182..272, not on its two icons).
        int plateLeft = px2 (kPlateLeft);
        auto cluster = [&] (const Cluster& c, juce::Label& lbl,
                            std::initializer_list<juce::Component*> items)
        {
            int ix = px2 (c.iconX);
            for (auto* item : items)
            {
                item->setBounds (ix, iconTop, tool, tool);
                ix += pitch;
            }
            const int plateRight = px2 (c.dividerX);
            lbl.setBounds (plateLeft, labelTop, juce::jmax (10, plateRight - plateLeft), labelH);
            dividerX_.push_back (plateRight);
            plateLeft = plateRight;
        };

        cluster (kFile, lblFile_,
                 { &btnFileNew_, &btnFileOpen_, &btnFileSave_, &btnFileSaveAs_, &btnFileExport_ });
        cluster (kNav,   lblNav_,   { &btnSelect_, &btnPan_ });
        cluster (kView,  lblView_,  { &btnZoomIn_, &btnZoomOut_, &btnFitView_ });
        cluster (kTools, lblTools_, { &btnPencil_, &btnEraser_, &btnRuler_ });

        // Opacity: caption + percentage on one line, track beneath. It has no
        // caption in the bottom label row â€” its own label sits up top instead.
        {
            const int x0 = px2 (kOpacity.iconX);
            const int w  = px2 (kOpacity.dividerX - 6) - x0;
            // Set here, not in the constructor: at construction Brand::UI::scale
            // is still 1.0, so a font assigned there stays at its base size.
            const auto opacityFont = Brand::tech (Brand::UI::scaledFont (Brand::Type::ribbonOpacityLabel));
            alphaLabel_.setFont (opacityFont);
            alphaValueLabel_.setFont (opacityFont);

            // Row tall enough for the caption, so drawFittedText does not shrink it.
            const int textH = juce::jmax (10, juce::roundToInt (opacityFont.getHeight()) + px2 (2));

            // Widths measured from the glyphs rather than split by a fixed
            // fraction, so neither label can clip. The value is sized for
            // "100%" -- its widest reading -- so the number does not shift
            // left and right as the slider moves.
            const int valW = juce::roundToInt (opacityFont.getStringWidthFloat ("100%")) + px2 (4);
            const int labW = juce::jmax (10, w - valW - px2 (4));
            alphaLabel_.setBounds      (x0, iconTop, labW, textH);
            alphaValueLabel_.setBounds (x0 + w - valW, iconTop, valW, textH);
            fillAlpha_.setBounds (x0, iconTop + textH + px2 (3), w, juce::jmax (10, tool - textH));

            const int plateRight = px2 (kOpacity.dividerX);
            dividerX_.push_back (plateRight);
            plateLeft = plateRight;
        }

        // Line | Polyline | Arc | Circle | Rectangle | Text box, in the mock's
        // order. The fill-colour preview swatch that used to sit before the
        // text box is gone â€” see the hidden-controls list above.
        cluster (kShapes, lblShapes_,
                 { &btnShapeLine_, &btnShapePolyline_, &btnShapeArc_,
                   &btnShapeCircle_, &btnShapeRect_, &btnShapeText_ });

        // Colours: 8 dots in a 4x2 grid, exactly like the Figma palette.
        {
            const int gridX   = px2 (kColours.iconX);
            const int gridTop = iconTop + juce::jmax (0, (tool - (swatch + pitchY)) / 2);
            for (int i = 0; i < kSwatchCount; ++i)
                swatches_[i].setBounds (gridX + (i % 4) * pitchX,
                                        gridTop + (i / 4) * pitchY, swatch, swatch);

            const int plateRight = px2 (kColours.dividerX);
            lblColours_.setBounds (plateLeft, labelTop,
                                   juce::jmax (10, plateRight - plateLeft), labelH);
            dividerX_.push_back (plateRight);
            plateLeft = plateRight;
        }

        if (helpPresent)
            cluster (kHelp, lblHelp_, { helpIcons_[0], helpIcons_[1], helpIcons_[2] });

        // Options: Snap and Ortho, plus Ortho's H / V / Gap extras once it is
        // on. These are text pills rather than glyphs, so they get their own
        // widths instead of riding the icon pitch.
        if (kShowOptionsCluster)
        {
            const int pillW = juce::jmax (24, px2 (kOptionPillW));
            const int pillGap = juce::jmax (2, px2 (kOptionPillGap));
            int x = px2 (kOptions.iconX);

            btnSnap_.setBounds  (x, iconTop, pillW, tool); x += pillW + pillGap;
            btnOrtho_.setBounds (x, iconTop, pillW, tool); x += pillW + pillGap;

            // H / V / Gap sit on the caption row beneath the two pills so the
            // cluster keeps its width when Ortho is switched on.
            if (btnOrthoHoriz_.isVisible())
            {
                const int sq = juce::jmax (10, labelH);
                int ex = px2 (kOptions.iconX);
                btnOrthoHoriz_.setBounds (ex, labelTop, sq, sq); ex += sq + pillGap;
                btnOrthoVert_.setBounds  (ex, labelTop, sq, sq); ex += sq + pillGap;
                const int gapLblW = juce::jmax (10, px2 (18));
                orthoGapLabel_.setBounds (ex, labelTop, gapLblW, sq); ex += gapLblW + pillGap;
                orthoGapSlider_.setBounds (ex, labelTop,
                                           juce::jmax (20, px2 (kOptions.dividerX) - pillGap - ex), sq);
                lblOptions_.setBounds (0, 0, 0, 0);
            }
            else
            {
                const int plateRight = px2 (kOptions.dividerX);
                lblOptions_.setBounds (plateLeft, labelTop,
                                       juce::jmax (10, plateRight - plateLeft), labelH);
            }

            const int plateRight = px2 (kOptions.dividerX);
            dividerX_.push_back (plateRight);
            plateLeft = plateRight;
        }
        else
        {
            // Hidden, never removed: the Figma ribbon has no Options slot, but
            // Snap / Ortho stay fully functional through the keyboard and the
            // terminal (see kShowOptionsCluster).
            for (juce::Component* c : { (juce::Component*) &btnSnap_,  (juce::Component*) &btnOrtho_,
                                        (juce::Component*) &btnOrthoHoriz_, (juce::Component*) &btnOrthoVert_,
                                        (juce::Component*) &orthoGapLabel_, (juce::Component*) &orthoGapSlider_,
                                        (juce::Component*) &lblOptions_ })
            {
                c->setVisible (false);
                c->setBounds (0, 0, 0, 0);
            }
        }

        // "Ready" pill â€” far right of this same ribbon row (Figma puts it
        // here, not in the title row above).
        if (readyPill_ != nullptr)
        {
            // Grow to fit longer status text, but never past the clusters.
            int readyW = px2 (100);
            if (readyPillWidth_)
                readyW = juce::jmax (readyW, readyPillWidth_());
            const int maxW = juce::jmax (px2 (60), getWidth() - fullReadyPad - plateLeft - px2 (12));
            readyW = juce::jmin (readyW, maxW);
            // Vertically centred in the row, which is also where Figma puts it
            // (y=87 inside the 58..132 band). Sharing the icons' top edge left
            // it riding high with the whole caption row empty beneath it.
            readyPill_->setBounds (juce::jmax (plateLeft, getWidth() - fullReadyPad - readyW),
                                   juce::jmax (0, (getHeight() - tool) / 2), readyW, tool);
        }

        // Draw prompts share the caption row, to the right of the clusters.
        const int promptX = juce::jmax (plateLeft + px2 (12), getWidth() / 2);
        promptLabel_.setBounds (promptX, labelTop,
                                juce::jmax (0, getWidth() - promptX - fullReadyPad), labelH);
        juce::ignoreUnused (fullIcon, fullGap, fullPad, fullEdge, fullSwatch, fullPitchX);
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
        for (auto* b : { &btnOrtho_, &btnOrthoHoriz_, &btnOrthoVert_, &btnSnap_, &btnSplProbe_ })
        {
            b->setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
            b->setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.35f));
            b->setColour (juce::TextButton::textColourOffId,
                          AppSettings::get().isDark() ? Brand::white() : Brand::onBtnIn());
            b->setColour (juce::TextButton::textColourOnId,
                          AppSettings::get().isDark() ? Brand::white() : Brand::onBtnIn());
        }
        colourSwatch_.setColour (juce::TextButton::buttonColourId, drawColour_);
        promptLabel_.setColour (juce::Label::textColourId, Brand::accent());
        promptLabel_.setFont (Brand::mono (Brand::UI::scaledFont (Brand::Type::plotToolbarLabel)));
        for (auto* l : { &lblFile_, &lblNav_, &lblView_, &lblTools_, &lblShapes_, &lblColours_, &lblHelp_, &lblOptions_ })
        {
            l->setColour (juce::Label::textColourId, Brand::text());
            l->setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::ribbonClusterLabel)));
        }

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
        auto restyleFile = [&] (juce::DrawableButton& b, const juce::String& fileName)
        {
            b.setColour (juce::DrawableButton::backgroundColourId,   Brand::plotToolbar());
            b.setColour (juce::DrawableButton::backgroundOnColourId, Brand::accent().withAlpha (0.28f));
            auto icon = loadToolIcon (fileName);
            b.setImages (icon.get());
        };
        restyle (btnSelect_,  kSelectSVG);
        restyleFile (btnPan_,     "Pan-nav-hand.svg");
        restyleFile (btnPencil_,  "Pencil.svg");
        restyleFile (btnEraser_,  "Eraser.svg");
        restyleFile (btnRuler_,   "Ruler.svg");
        restyle (btnShape_,   kShapeSVG);
        restyle (btnMic_,     kMicSVG);
        restyleFile (btnZoomIn_,  "Zoom In.svg");
        restyleFile (btnZoomOut_, "Zoom Out.svg");
        restyleFile (btnFitView_, "Full Screen.svg");
        alphaLabel_.setColour (juce::Label::textColourId, Brand::muted());
        fillAlpha_.setColour (juce::Slider::trackColourId, Brand::border());
        fillAlpha_.setColour (juce::Slider::thumbColourId, Brand::accent());
        fillAlpha_.setColour (juce::Slider::backgroundColourId, Brand::plotToolbar());
        fillAlpha_.setColour (juce::Slider::textBoxTextColourId, Brand::muted());
        fillAlpha_.setColour (juce::Slider::textBoxBackgroundColourId, Brand::plotToolbar());
        fillAlpha_.setColour (juce::Slider::textBoxOutlineColourId, Brand::border().withAlpha (0.35f));
    }

private:
    juce::Label title_;
    juce::Colour drawColour_ { 0xffffcc00 };

    // Pan / Pencil / Eraser / Ruler / Zoom / Full-screen ship as reference art
    // in the project's /Icons folder (flat pre-coloured glyphs) â€” load them
    // from disk rather than hand-drawing equivalents inline.
    /** Loads a toolbar glyph by base name, preferring the bitmap.

        Figma exports these icons as an SVG whose only content is a <pattern>
        fill referencing an embedded base64 PNG. JUCE's SVG parser doesn't
        implement pattern-with-image fills, so createFromSVG returns a shape
        with no drawable content and the button renders as a solid block â€”
        which is what the whole View/Tools/Navigation row was doing. The real
        bitmaps are extracted alongside as .png, so try those first and only
        fall back to SVG for genuine vector art.
    */
    static std::unique_ptr<juce::Drawable> loadToolIcon (const juce::String& fileName)
    {
        // Via Brand::assetBytes, so a downloaded EXE with no Assets/ folder
        // beside it still gets its glyphs from the baked-in copies.
        const auto base = fileName.upToLastOccurrenceOf (".", false, false);

        // PNG first: the Figma SVGs wrap base64 bitmaps in a pattern fill,
        // which JUCE renders as a solid black square.
        if (const auto mb = Brand::assetBytes ("ToolIcons/" + base + ".png");
            mb.getSize() > 0)
        {
            const auto img = juce::ImageFileFormat::loadFrom (mb.getData(), mb.getSize());
            if (img.isValid())
            {
                auto d = std::make_unique<juce::DrawableImage>();
                d->setImage (img);
                return d;
            }
        }

        if (const auto mb = Brand::assetBytes ("ToolIcons/" + base + ".svg");
            mb.getSize() > 0)
            if (auto xml = juce::XmlDocument::parse (
                    juce::String::createStringFromData (mb.getData(), (int) mb.getSize())))
                return juce::Drawable::createFromSVG (*xml);

        return {};
    }

    static constexpr const char* kSelectSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M5 3l14 9.5-6.2 1.4L16.5 21l-2.2 1.2-3.6-7.2L5 19.5V3z"/></svg>)SVG";
    static constexpr const char* kShapeSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M3 5h8v6H3z" opacity=".45"/><path fill="#fff" d="M3 5h8v2H3zm0 4h8v2H3zM3 5h2v6H3zm6 0h2v6H9z"/><path fill="#fff" d="M14 4l6 4-6 4V4z"/><circle cx="17" cy="17" r="4" fill="#fff" opacity=".45"/><circle cx="17" cy="17" r="4" fill="none" stroke="#fff" stroke-width="2"/></svg>)SVG";
    static constexpr const char* kMicSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><rect x="9" y="3" width="6" height="11" rx="3" fill="#fff"/><path fill="none" stroke="#fff" stroke-width="2" d="M6 11a6 6 0 0 0 12 0"/><path fill="#fff" d="M11 17h2v3h-2z"/><path fill="#fff" d="M8 20h8v2H8z"/></svg>)SVG";
    // Figma ribbon icons: the Shapes cluster.
    static constexpr const char* kShapeLineSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M3.6 19l15.4-15.4 1.4 1.4L5 20.4z"/></svg>)SVG";
    static constexpr const char* kShapePolylineSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="none" stroke="#fff" stroke-width="2" stroke-linejoin="round" stroke-linecap="round" d="M3 18l5-8 4 5 3-6 6 9"/></svg>)SVG";
    static constexpr const char* kShapeCircleSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle fill="#fff" cx="12" cy="12" r="8.5"/></svg>)SVG";
    static constexpr const char* kShapeRectSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><rect fill="#fff" x="3.5" y="6" width="17" height="12" rx="1"/></svg>)SVG";
    static constexpr const char* kShapeTextSVG =
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><rect fill="none" stroke="#fff" stroke-width="1.8" x="3.5" y="4.5" width="17" height="15" rx="1.5"/><path fill="#fff" d="M7.5 8h9v1.9h-3.5v6.6h-2V9.9H7.5z"/></svg>)SVG";
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

// Header icons â€” filled charcoal glyphs matching mockup Stats row.
namespace HeaderIcons
{
    // Filled badge: disk = #fff (recolour to charcoal), mark = #000 (recolour to white)
    static constexpr const char* kHelp = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle cx="12" cy="12" r="11" fill="#fff"/><path fill="#000" d="M10.2 8.6c.35-1.15 1.3-1.9 2.7-1.9 1.55 0 2.65.95 2.65 2.35 0 .95-.45 1.55-1.35 2.15-.85.55-1.15.95-1.15 1.7v.45h-1.55v-.55c0-1.15.4-1.7 1.3-2.3.7-.45 1-0.85 1-1.4 0-.7-.55-1.15-1.35-1.15-.8 0-1.35.45-1.55 1.2l-1.7-.4zm1.95 7.55c.65 0 1.15-.5 1.15-1.15s-.5-1.15-1.15-1.15-1.15.5-1.15 1.15.5 1.15 1.15 1.15z"/></svg>)SVG";
    // Filled badge with lowercase â€œiâ€ (info / keyboard shortcuts).
    static constexpr const char* kInfo = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle cx="12" cy="12" r="11" fill="#fff"/><path fill="#000" d="M11.1 10.2h1.8v7.1h-1.8zm0-3.9h1.8V8h-1.8z"/></svg>)SVG";
    static constexpr const char* kGear = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M19.14 12.94c.04-.31.06-.63.06-.94s-.02-.63-.06-.94l2.03-1.58a.5.5 0 00.12-.64l-1.92-3.32a.5.5 0 00-.6-.22l-2.39.96a7.03 7.03 0 00-1.63-.94l-.36-2.54a.5.5 0 00-.5-.42h-3.84a.5.5 0 00-.5.42l-.36 2.54c-.59.24-1.13.55-1.63.94l-2.39-.96a.5.5 0 00-.6.22L2.77 8.84a.5.5 0 00.12.64l2.03 1.58c-.04.31-.07.63-.07.94s.03.63.07.94L2.89 14.5a.5.5 0 00-.12.64l1.92 3.32c.13.22.39.3.6.22l2.39-.96c.5.39 1.04.71 1.63.94l.36 2.54c.05.24.26.42.5.42h3.84c.24 0 .45-.18.5-.42l.36-2.54c.59-.24 1.13-.55 1.63-.94l2.39.96c.22.08.47 0 .6-.22l1.92-3.32a.5.5 0 00-.12-.64l-2.03-1.58zM12 15.6A3.6 3.6 0 1112 8.4a3.6 3.6 0 010 7.2z"/></svg>)SVG";
    static constexpr const char* kMenu = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle cx="12" cy="5" r="2" fill="#fff"/><circle cx="12" cy="12" r="2" fill="#fff"/><circle cx="12" cy="19" r="2" fill="#fff"/></svg>)SVG";
    // Three-line hamburger for sidebar collapse / expand.
    static constexpr const char* kHamburger = R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><path fill="#fff" d="M4 6.5h16v2.2H4zm0 4.65h16v2.2H4zm0 4.65h16v2.2H4z"/></svg>)SVG";
}

// ---------------------------------------------------------------------------
// ViewTileButton â€” icon + caption tile for view mode switching.
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
// StatusStrip â€” STATUS | Ready + last run + elapsed.
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

    /** Which of the Figma design's status surfaces this instance draws.
        Full  â€” the legacy full-width strip (no longer used by the Figma layout).
        Pill  â€” dot + state only, the ribbon's top-right "Ready" pill.
        RunInfo â€” "Last run : â€¦" / "Elapsed : â€¦" stacked on two lines, the
                  bottom strip's left side. */
    enum class Mode { Full, Pill, RunInfo };
    void setMode (Mode m) { mode_ = m; repaint(); }

    /** Width the Pill needs for its current text. Status messages are far
        longer than "Ready" ("Project saved: ...", "Could not open: ..."), and
        since the full-width status strip is gone this pill is the only place
        they surface â€” so the ribbon sizes it to fit instead of ellipsizing. */
    int preferredPillWidth() const
    {
        const auto font = Brand::techMed (Brand::UI::scaledFont (Brand::Type::status));
        const float dotR = 3.5f * Brand::UI::scale;
        return (int) (dotR * 2.0f) + UiConfig::Scale::px (6)
             + juce::roundToInt (font.getStringWidthFloat (state_)) + UiConfig::Scale::px (4);
    }

    void paint (juce::Graphics& g) override
    {
        const auto font = Brand::techMed (Brand::UI::scaledFont (Brand::Type::status));
        g.setFont (font);
        const float dotR = 3.5f * Brand::UI::scale;
        const float cy   = getHeight() * 0.5f;

        if (mode_ == Mode::Pill)
        {
            // Figma redesign: header "Ready" pill's dot is always the brand
            // red accent (not green/success) â€” matches the mock exactly.
            g.setColour (ready_ ? Brand::accent() : Brand::warning());
            g.fillEllipse (0.0f, cy - dotR, dotR * 2.0f, dotR * 2.0f);
            const int stateX = (int) (dotR * 2.0f) + UiConfig::Scale::px (6);
            g.setColour (Brand::text());
            g.drawText (state_, juce::Rectangle<int> (stateX, 0, getWidth() - stateX, getHeight()),
                        juce::Justification::centredLeft, true);
            return;
        }

        if (mode_ == Mode::RunInfo)
        {
            // Two left-aligned lines, as in the Figma bottom strip.
            const int lineGap = UiConfig::Scale::px (UiConfig::Layout::bottomRunInfoLineGap);
            const int lineH   = juce::jmax (1, lineGap);
            g.setColour (Brand::text());
            g.drawText (lastRun_, juce::Rectangle<int> (0, 0, getWidth(), lineH),
                        juce::Justification::centredLeft, true);
            g.drawText (elapsed_, juce::Rectangle<int> (0, lineGap, getWidth(), lineH),
                        juce::Justification::centredLeft, true);
            return;
        }

        g.fillAll (Brand::panel());
        g.setColour (Brand::border().withAlpha (0.45f));
        g.drawHorizontalLine (0, 0.0f, (float) getWidth());

        const int edgePad = UiConfig::Scale::px (14);
        const int third   = getWidth() / 3;

        // Left: status dot + state text (vertically centred, common baseline).
        g.setColour (ready_ ? Brand::success() : Brand::warning());
        g.fillEllipse ((float) edgePad, cy - dotR, dotR * 2.0f, dotR * 2.0f);

        const int stateX = edgePad + (int) (dotR * 2.0f) + UiConfig::Scale::px (8);
        g.setColour (Brand::text());
        g.drawText (state_,
                    juce::Rectangle<int> (stateX, 0, third - stateX, getHeight()),
                    juce::Justification::centredLeft, true);

        // Centre: last run â€” evenly distributed across the middle third.
        g.setColour (Brand::text().withAlpha (0.78f));
        g.drawText (lastRun_,
                    getLocalBounds().withTrimmedLeft (third).withTrimmedRight (third),
                    juce::Justification::centred, true);

        // Right: elapsed â€” same padding from the right edge as the left.
        g.drawText (elapsed_,
                    getLocalBounds().withTrimmedLeft (2 * third).withTrimmedRight (edgePad),
                    juce::Justification::centredRight, true);
    }

private:
    juce::String state_   { "Ready" };
    juce::String lastRun_ { "Last run : -" };
    juce::String elapsed_ { "Elapsed : -" };
    bool ready_ = true;
    Mode mode_ = Mode::Full;
};

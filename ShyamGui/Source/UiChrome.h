#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include <functional>
#include <utility>
#include <algorithm>
#include <vector>

// ---------------------------------------------------------------------------
// SectionHeader - collapsible sidebar section title with chevron.
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
// ParamBar - plain-text simulation readouts (top bar, no chip boxes).
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
// PlotHeaderBar - workspace title + navigation toolbar.
// ---------------------------------------------------------------------------
/** Word-style alignment glyph: four rules, ragged on the side the text is
    not anchored to. Drawn rather than shipped as an SVG so it stays crisp at
    any scale and adds no asset to embed. */
class TextAlignButton : public juce::Button
{
public:
    /** @param vertical true draws the stack anchored top / middle / bottom
               instead of ragged left / centre / right. */
    TextAlignButton (int align, bool vertical = false)
        : juce::Button ("align"), align_ (align), vertical_ (vertical)
    {
        setClickingTogglesState (false);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat().reduced (getWidth() * 0.22f);
        const bool on = getToggleState();
        if (on || over || down)
        {
            g.setColour (on ? Brand::accent().withAlpha (0.16f)
                            : Brand::border().withAlpha (down ? 0.45f : 0.28f));
            g.fillRoundedRectangle (getLocalBounds().toFloat().reduced (1.5f), 3.0f);
        }

        g.setColour (isEnabled() ? (on ? Brand::accent() : Brand::heading())
                                 : Brand::muted().withAlpha (0.5f));
        const float lh = juce::jmax (1.0f, r.getHeight() / 7.0f);

        if (vertical_)
        {
            // Three rules held to one edge, with the box's own outline behind
            // them so "which edge" reads at a glance.
            g.drawRoundedRectangle (r, 1.5f, 1.0f);
            const float blockH = lh * 5.0f;
            float top = r.getY() + lh;
            if (align_ == 1) top = r.getCentreY() - blockH * 0.5f;
            else if (align_ == 2) top = r.getBottom() - lh - blockH;
            for (int i = 0; i < 3; ++i)
                g.fillRoundedRectangle (r.getX() + lh, top + (float) i * lh * 2.0f,
                                        r.getWidth() - lh * 2.0f, lh, lh * 0.5f);
            return;
        }

        const float gap = r.getHeight() / 3.6f;
        for (int i = 0; i < 4; ++i)
        {
            // Long, short, long, short - the short ones carry the ragged edge.
            const float w = (i % 2 == 0) ? r.getWidth() : r.getWidth() * 0.62f;
            float x = r.getX();
            if (align_ == 1) x = r.getCentreX() - w * 0.5f;
            else if (align_ == 2) x = r.getRight() - w;
            g.fillRoundedRectangle (x, r.getY() + (float) i * gap, w, lh, lh * 0.5f);
        }
    }

private:
    int  align_;
    bool vertical_;
};

/** Word's line-spacing glyph: a stack of rules with a two-headed arrow
    beside them, and a caret to say it opens a list. Drawn rather than shipped
    as an SVG for the same reason as TextAlignButton - crisp at any scale, no
    asset to embed. The current value is NOT on the face: like a word
    processor, the button says what it controls and the menu ticks the value. */
class LineSpacingButton : public juce::Button
{
public:
    LineSpacingButton() : juce::Button ("lineSpacing")
    {
        setClickingTogglesState (false);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto full = getLocalBounds().toFloat();
        if (over || down)
        {
            g.setColour (Brand::border().withAlpha (down ? 0.45f : 0.28f));
            g.fillRoundedRectangle (full.reduced (1.0f), 3.0f);
        }

        g.setColour (isEnabled() ? Brand::heading() : Brand::muted().withAlpha (0.5f));

        auto b     = full.reduced (full.getWidth() * 0.10f, full.getHeight() * 0.22f);
        auto caret = b.removeFromRight (b.getWidth() * 0.30f);
        auto arrowCol = b.removeFromLeft (b.getWidth() * 0.38f);
        auto rules = b.withTrimmedLeft (b.getWidth() * 0.12f)
                      .withTrimmedRight (b.getWidth() * 0.14f);   // air before the caret

        // Four rules, the thing whose spacing is being set.
        const float lh = juce::jmax (1.0f, rules.getHeight() / 9.0f);
        for (int i = 0; i < 4; ++i)
            g.fillRoundedRectangle (rules.getX(),
                                    rules.getY() + (float) i * (rules.getHeight() - lh) / 3.0f,
                                    rules.getWidth(), lh, lh * 0.5f);

        // A double-headed arrow spanning them: "this gap".
        const float cx   = arrowCol.getCentreX();
        const float head = juce::jmax (2.0f, arrowCol.getWidth() * 0.45f);
        const float stem = juce::jmax (1.0f, lh * 0.9f);
        g.fillRect (cx - stem * 0.5f, arrowCol.getY() + head,
                    stem, arrowCol.getHeight() - head * 2.0f);
        juce::Path up, dn;
        up.addTriangle (cx, arrowCol.getY(),
                        cx - head, arrowCol.getY() + head,
                        cx + head, arrowCol.getY() + head);
        dn.addTriangle (cx, arrowCol.getBottom(),
                        cx - head, arrowCol.getBottom() - head,
                        cx + head, arrowCol.getBottom() - head);
        g.fillPath (up);
        g.fillPath (dn);

        // Caret, same weight as the combo boxes beside it.
        const float cw = juce::jmax (3.0f, caret.getWidth() * 0.55f);
        const float ch = cw * 0.55f;
        juce::Path v;
        v.addTriangle (caret.getCentreX() - cw * 0.5f, caret.getCentreY() - ch * 0.5f,
                       caret.getCentreX() + cw * 0.5f, caret.getCentreY() - ch * 0.5f,
                       caret.getCentreX(),             caret.getCentreY() + ch * 0.5f);
        g.fillPath (v);
    }
};

/** A ribbon tab, drawn the way Word draws one: a plain word that takes an
    accent rule along the bottom of the strip when it is the live tab. No pill
    and no box - the rule, and the fact that the tab joins the row of controls
    below it, is the whole affordance. */
class RibbonTabButton : public juce::Button
{
public:
    explicit RibbonTabButton (const juce::String& label)
        : juce::Button (label), label_ (label)
    {
        setClickingTogglesState (false);
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        const bool on = getToggleState();
        auto r = getLocalBounds().toFloat();

        if (on || over || down)
        {
            g.setColour (on ? Brand::accent().withAlpha (0.08f)
                            : Brand::border().withAlpha (down ? 0.40f : 0.22f));
            g.fillRoundedRectangle (r.reduced (1.0f, 1.5f), 3.0f);
        }

        g.setColour (on ? Brand::accent() : Brand::muted());
        // Same reference as the ribbon's controls, so a tab is never smaller
        // than the buttons it switches to.
        g.setFont (Brand::tech (juce::jmax (12.5f,
                                juce::jmin (Brand::UI::scaledFont (Brand::Type::ribbonTabLabel),
                                            r.getHeight() * 0.72f)), on));
        g.drawText (label_, getLocalBounds(), juce::Justification::centred);

        if (on)
        {
            g.setColour (Brand::accent());
            g.fillRect (r.getX() + 2.0f, r.getBottom() - 2.0f,
                        r.getWidth() - 4.0f, 2.0f);
        }
    }

private:
    juce::String label_;
};

/** A ribbon toggle that shows the designer's glyph (Assets/RibbonIcons)
    instead of a word, in the three Figma states: grey when disabled (the
    black art at half strength), black when available, and the red art when
    activated. The word is kept as the button text and leads the tooltip, so
    hovering still names the control. Layers sit in a 24-unit slot so every
    glyph keeps its drawn size against the others, and a badge (Lock Rays'
    padlock) can overhang the main glyph the way the design places it. */
class RibbonIconButton : public juce::Button
{
public:
    struct Layer
    {
        const char* file;          // under Assets/RibbonIcons
        const char* activeFile;    // the red art; nullptr keeps `file` (the padlock)
        float x, y, w, h;          // frame in the 24-unit slot
        bool tint = true;          // false keeps the black art's own colours
    };

    RibbonIconButton (const juce::String& label, std::initializer_list<Layer> layers)
        : juce::Button (label)
    {
        setClickingTogglesState (true);
        for (const auto& l : layers)
            load (l);
    }

    /** A caption drawn as a glyph: no hover, no press, no toggle. Its
        "on" state just says whether the control it labels is live. */
    void setGlyphOnly (bool g)
    {
        glyphOnly_ = g;
        setClickingTogglesState (! g);
        setTriggeredOnMouseDown (false);
        repaint();
    }

    juce::String getTooltip() override
    {
        const auto tip = juce::Button::getTooltip();
        if (tip.isEmpty() || tip == getButtonText())
            return getButtonText();
        return getButtonText() + "\n" + tip;
    }

    void paintButton (juce::Graphics& g, bool over, bool down) override
    {
        auto r = getLocalBounds().toFloat();
        if (! glyphOnly_ && isEnabled() && (over || down))
        {
            g.setColour (Brand::border().withAlpha (down ? 0.45f : 0.28f));
            g.fillRoundedRectangle (r.reduced (1.0f), 3.0f);
        }

        // Disabled wins over on: a greyed control shows grey even if its
        // setting is kept (Show Interdistance while Ortho is off).
        // A one-shot button (+ Range, Mic's menu) has no on state of its
        // own, so it shows red while it is held.
        const bool  momentary = ! glyphOnly_ && ! getClickingTogglesState();
        const bool  active = isEnabled() && (getToggleState() || (momentary && down));
        const float alpha  = isEnabled() ? 1.0f : 0.5f;
        const auto  ink = AppSettings::get().isDark() ? juce::Colours::white
                                                      : juce::Colours::black;
        // The slot IS the Figma frame: ribbonIconSize is the 24 px frame
        // on the ribbon's grid, so no inset.
        const float side = juce::jmin (r.getWidth(), r.getHeight());
        if (side <= 2.0f) return;
        const float u = side / 24.0f;
        const auto origin = r.getCentre() - juce::Point<float> (side, side) * 0.5f;
        const float pixelScale = g.getInternalContext().getPhysicalPixelScaleFactor();

        for (auto& l : layers_)
        {
            const juce::Rectangle<float> dest (origin.x + l.frame.getX() * u,
                                               origin.y + l.frame.getY() * u,
                                               l.frame.getWidth() * u,
                                               l.frame.getHeight() * u);
            const bool red = active && l.active.isValid();
            auto& art = red ? l.active : l.normal;
            const bool tinted = ! red && l.tint;

            if (art.svg != nullptr)
            {
                if (tinted && art.inked != ink)
                {
                    for (auto from : { art.inked, juce::Colour (0xff2c2c2c) })
                        art.svg->replaceColour (from, ink);
                    art.inked = ink;
                }
                art.svg->drawWithin (g, dest, juce::RectanglePlacement::stretchToFit, alpha);
            }
            else if (art.png.isValid())
            {
                const auto& img = art.sized (juce::roundToInt (dest.getWidth()  * pixelScale),
                                             juce::roundToInt (dest.getHeight() * pixelScale));
                if (tinted)
                {
                    g.setColour (ink.withAlpha (alpha));
                    g.drawImage (img, dest, juce::RectanglePlacement::stretchToFit, true);
                }
                else
                {
                    g.setOpacity (alpha);
                    g.drawImage (img, dest, juce::RectanglePlacement::stretchToFit, false);
                }
            }
        }
    }

private:
    struct Art
    {
        std::unique_ptr<juce::Drawable> svg;
        juce::Image png, cached;
        juce::Colour inked { juce::Colours::black };

        bool isValid() const noexcept { return svg != nullptr || png.isValid(); }

        /** The 90 px art stepped down by halves to the drawn size: a single
            large jump in the renderer aliases thin strokes into dashes. */
        const juce::Image& sized (int w, int h)
        {
            w = juce::jmax (1, w);
            h = juce::jmax (1, h);
            if (cached.isValid() && cached.getWidth() == w && cached.getHeight() == h)
                return cached;
            auto img = png;
            while (img.getWidth() > w * 2 && img.getHeight() > h * 2)
                img = img.rescaled (img.getWidth() / 2, img.getHeight() / 2,
                                    juce::Graphics::highResamplingQuality);
            cached = img.rescaled (w, h, juce::Graphics::highResamplingQuality);
            return cached;
        }
    };

    struct Loaded
    {
        Art normal, active;
        juce::Rectangle<float> frame;
        bool tint = true;
    };

    static Art loadArt (const juce::String& file)
    {
        Art art;
        const auto mb = Brand::assetBytes ("RibbonIcons/" + file);
        if (mb.getSize() == 0)
            return art;
        if (file.endsWithIgnoreCase (".svg"))
        {
            if (auto xml = juce::XmlDocument::parse (
                    juce::String::createStringFromData (mb.getData(), (int) mb.getSize())))
                art.svg = juce::Drawable::createFromSVG (*xml);
        }
        else
        {
            art.png = juce::ImageFileFormat::loadFrom (mb.getData(), mb.getSize());
        }
        return art;
    }

    void load (const Layer& l)
    {
        Loaded out;
        out.frame  = { l.x, l.y, l.w, l.h };
        out.tint   = l.tint;
        out.normal = loadArt (l.file);
        if (! out.normal.isValid())
            return;
        if (l.activeFile != nullptr)
            out.active = loadArt (l.activeFile);
        layers_.push_back (std::move (out));
    }

    std::vector<Loaded> layers_;
    bool glyphOnly_ = false;
};

/** A ribbon number box that names its axis inside the field ("X 00.0"), as
    the Figma ribbon draws it. Click to type; the mouse wheel steps the value
    by 0.1 a notch, or by 1 with Shift held. Changes notify like typing does. */
class RibbonCoordBox : public juce::Label
{
public:
    explicit RibbonCoordBox (const juce::String& axis) : axis_ (axis)
    {
        setEditable (true, true, false);
        setJustificationType (juce::Justification::centredLeft);
        setColour (backgroundWhenEditingColourId, juce::Colours::transparentBlack);
        setColour (outlineWhenEditingColourId,    juce::Colours::transparentBlack);
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat().reduced (0.5f);
        g.setColour (Brand::exportPill());
        g.fillRect (r);
        g.setColour (Brand::text().withAlpha (0.12f));
        g.drawRect (r, 1.0f);

        const auto ink = Brand::text().withMultipliedAlpha (isEnabled() ? 1.0f : 0.5f);
        g.setFont (getFont());
        g.setColour (ink);
        g.drawText (axis_, getLocalBounds().withTrimmedLeft (pad()).withWidth (captionW()),
                    juce::Justification::centredLeft, false);
        if (! isBeingEdited())
            g.drawText (getText(), getLocalBounds().withTrimmedLeft (valueLeft()).withTrimmedRight (pad()),
                        juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        if (auto* ed = getCurrentTextEditor())
            ed->setBounds (getLocalBounds().withTrimmedLeft (valueLeft() - 2));
    }

    void editorShown (juce::TextEditor* ed) override
    {
        ed->setColour (juce::TextEditor::textColourId, Brand::text());
        ed->setColour (juce::TextEditor::highlightColourId, Brand::accent().withAlpha (0.3f));
        ed->setColour (juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        ed->setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        ed->setBorder ({});
        ed->setIndents (2, 0);
        ed->setJustification (juce::Justification::centredLeft);
        resized();
    }

    void mouseWheelMove (const juce::MouseEvent& e, const juce::MouseWheelDetails& w) override
    {
        if (! isEnabled() || isBeingEdited())
        {
            juce::Label::mouseWheelMove (e, w);
            return;
        }
        // Shift turns a vertical wheel horizontal on some systems.
        float d = w.deltaY != 0.0f ? w.deltaY : w.deltaX;
        if (w.isReversed) d = -d;
        int steps = 0;
        if (w.isSmooth)
        {
            // Trackpads send a stream of small deltas; a step per quarter
            // "notch" keeps a flick from running the value away.
            wheelAcc_ += d;
            steps = (int) (wheelAcc_ / 0.25f);
            wheelAcc_ -= (float) steps * 0.25f;
        }
        else if (d != 0.0f)
        {
            steps = d > 0.0f ? 1 : -1;
        }
        if (steps == 0)
            return;
        const double step = e.mods.isShiftDown() ? 1.0 : 0.1;
        const double v = getText().getDoubleValue() + steps * step;
        setText (juce::String (v, 1), juce::sendNotificationSync);
    }

private:
    int pad() const       { return juce::jmax (3, juce::roundToInt (getHeight() * 0.25f)); }
    int captionW() const  { return juce::roundToInt (getFont().getStringWidthFloat (axis_)) + 1; }
    int valueLeft() const { return pad() + captionW() + juce::jmax (3, getHeight() / 5); }

    juce::String axis_;
    float wheelAcc_ = 0.0f;
};

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
        btnMic_.setClickingTogglesState (false);
        btnMic_.setTooltip ("Add virtual receivers on the gradient plot");
        addAndMakeVisible (btnMic_);

        // X / Y for the selected mic, typed in the ribbon or rolled with the
        // wheel. Each box names its own axis.
        {
            auto styleCoord = [this] (RibbonCoordBox& l, const juce::String& tip)
            {
                l.setTooltip (tip);
                l.onTextChange = [this] { pushMicCoords(); };
                addAndMakeVisible (l);
            };
            styleCoord (micXBox_, "X of the selected mic, in the unit on show. "
                                  "Select a mic on the plot, then type or scroll "
                                  "(Shift for steps of 1).");
            styleCoord (micYBox_, "Y of the selected mic, in the unit on show. "
                                  "Type or scroll (Shift for steps of 1).");
            setMicCoords (false, 0.0f, 0.0f);
        }
        styleToolFile (btnZoomIn_,  "Zoom In.svg",  "Zoom in");
        styleToolFile (btnZoomOut_, "Zoom Out.svg", "Zoom out");
        // Magnifier: a toggle, but deliberately NOT in the tool radio group.
        // It is a way of looking at the plot, not a thing you do to it, so it
        // must not take Select's place the moment you switch it on.
        styleTool (btnMagnify_, kMagnifySVG,
                   "Magnifier: a glass that follows the pointer and enlarges "
                   "what is under it, without changing the view", false);
        btnMagnify_.setClickingTogglesState (true);

        btnSelect_.setToggleState (true, juce::dontSendNotification);

        // File cluster - Figma-exported PNGs, on a light chip so the (dark-ink)
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
            // Auto-shrink was turned off entirely because it squeezed captions
            // to about half their size. Off, the size is capped instead by the
            // longest caption that has to fit its plate - "Navigation",
            // "Mapping Type", "Architectural" - so every other caption in the
            // row is held down by the worst case. A floor of 0.82 gets both:
            // the size can be set for legibility, and only the few long ones
            // give up a little width, which is far less visible than every
            // caption being a size too small.
            l.setMinimumHorizontalScale (0.82f);
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
        styleClusterLabel (lblText_,    "Text Box");

        // Word-style text controls. Live only while a text box is the target,
        // so they never look like they apply to a shape or a speaker.
        btnAlignL_.setTooltip ("Align text left");
        btnAlignC_.setTooltip ("Centre text");
        btnAlignR_.setTooltip ("Align text right");
        for (auto* b : { &btnAlignL_, &btnAlignC_, &btnAlignR_ })
            addAndMakeVisible (*b);
        btnAlignL_.onClick = [this] { if (onTextAlign) onTextAlign (0); };
        btnAlignC_.onClick = [this] { if (onTextAlign) onTextAlign (1); };
        btnAlignR_.onClick = [this] { if (onTextAlign) onTextAlign (2); };

        btnVAlignT_.setTooltip ("Align text to the top of the box");
        btnVAlignM_.setTooltip ("Centre text vertically");
        btnVAlignB_.setTooltip ("Align text to the bottom of the box");
        for (auto* b : { &btnVAlignT_, &btnVAlignM_, &btnVAlignB_ })
            addAndMakeVisible (*b);
        btnVAlignT_.onClick = [this] { if (onTextVAlign) onTextVAlign (0); };
        btnVAlignM_.onClick = [this] { if (onTextVAlign) onTextVAlign (1); };
        btnVAlignB_.onClick = [this] { if (onTextVAlign) onTextVAlign (2); };

        auto styleStyleBtn = [this] (juce::TextButton& b, const juce::String& t,
                                     const juce::String& tip)
        {
            b.setButtonText (t);
            b.setTooltip (tip);
            b.setClickingTogglesState (true);
            b.setComponentID ("ribbonStyle");
            b.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            b.setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.16f));
            b.setColour (juce::TextButton::textColourOffId, Brand::heading());
            b.setColour (juce::TextButton::textColourOnId,  Brand::accent());
            addAndMakeVisible (b);
        };
        styleStyleBtn (btnBold_,   "B", "Bold");
        styleStyleBtn (btnItalic_, "I", "Italic");
        btnBold_.onClick   = [this] { if (onTextBold)   onTextBold   (btnBold_.getToggleState()); };
        btnItalic_.onClick = [this] { if (onTextItalic) onTextItalic (btnItalic_.getToggleState()); };

        fontSizeBox_.setComponentID ("ctrlCombo");
        fontSizeBox_.setTooltip ("Text size. Auto fits the type to the box.");
        fontSizeBox_.addItem ("Auto", 1);
        {
            // The sizes a word processor offers, so the list is familiar.
            const int sizes[] = { 8, 9, 10, 11, 12, 14, 16, 18, 20, 24, 28, 32, 40, 48, 64 };
            int id = 2;
            for (int v : sizes)
                fontSizeBox_.addItem (juce::String (v), id++);
        }
        fontSizeBox_.setSelectedId (1, juce::dontSendNotification);
        fontSizeBox_.onChange = [this]
        {
            if (textUpdating_) return;
            if (! onTextSize) return;
            const auto t = fontSizeBox_.getText();
            onTextSize (t == "Auto" ? 0.0f : (float) t.getIntValue());
        };
        addAndMakeVisible (fontSizeBox_);

        lineHeightBtn_.setTooltip ("Line spacing - the gap between lines, as a "
                                   "multiple of the text size.");
        lineHeightBtn_.onClick = [this] { showLineSpacingMenu(); };
        addAndMakeVisible (lineHeightBtn_);

        // --- tabs ---------------------------------------------------------
        // Only the text cluster is listed. Home is "everything else", worked
        // out live in resized() rather than snapshotted here: several clusters
        // (help icons, shapes, colours, opacity) are added by setters AFTER
        // this constructor runs, so a snapshot taken now would miss them.
        textChildren_ = { (juce::Component*) &lblText_,
                          (juce::Component*) &btnAlignL_,  (juce::Component*) &btnAlignC_,
                          (juce::Component*) &btnAlignR_,  (juce::Component*) &btnVAlignT_,
                          (juce::Component*) &btnVAlignM_, (juce::Component*) &btnVAlignB_,
                          (juce::Component*) &btnBold_,    (juce::Component*) &btnItalic_,
                          (juce::Component*) &fontSizeBox_,
                          (juce::Component*) &lineHeightBtn_ };

        btnTabHome_.setToggleState (true, juce::dontSendNotification);
        btnTabHome_.onClick    = [this] { setRibbonTab (0); };
        btnTabMapping_.onClick = [this] { setRibbonTab (1); };
        addAndMakeVisible (btnTabHome_);
        addAndMakeVisible (btnTabMapping_);

        // --- Mapping tab --------------------------------------------------
        styleClusterLabel (lblMapping_, "Rays");
        addAndMakeVisible (lblMapping_);

        btnShowRays_.setTooltip ("Draw the aiming ray out of every mid and top. "
                                 "Subs have no ray; select one to turn it, or "
                                 "right-click it to show or hide its aim line.");
        btnShowRays_.onClick = [this]
        {
            if (onShowRays) onShowRays (btnShowRays_.getToggleState());
        };
        addAndMakeVisible (btnShowRays_);

        btnLockRays_.setTooltip ("Lock the rays: they stay on screen but cannot be "
                                 "grabbed, so a finished aim cannot be nudged.");
        btnLockRays_.onClick = [this]
        {
            if (onLockRays) onLockRays (btnLockRays_.getToggleState());
        };
        addAndMakeVisible (btnLockRays_);

        styleClusterLabel (lblMic_, "Mic");
        addAndMakeVisible (lblMic_);

        styleClusterLabel (lblPlane_, "Plane Prediction");
        addAndMakeVisible (lblPlane_);

        auto planeBtn = [this] (RibbonIconButton& b, const juce::String& tip)
        {
            b.setTooltip (tip);
            addAndMakeVisible (b);
        };
        planeBtn (btnUsePlane_,
                  "Treat this shape as a surface in the venue rather than a note on it.");
        planeBtn (btnPlaneListen_,
                  "Audience: the surface people are on.");
        planeBtn (btnPlaneVirt_,
                  "A reference surface nobody occupies - a construction line.");
        planeBtn (btnPlaneArch_,
                  "Structure: a wall, a balcony front, the stage edge.");
        planeBtn (btnRangeAdd_,
                  "Draw reference rings at the distances you choose, around every "
                  "speaker. Type one or several at once in metres or feet - "
                  "2, 4, 8 or 5m, 20ft.");
        btnRangeAdd_.setClickingTogglesState (false);
        btnRangeAdd_.onClick = [this] { if (onAddRange) onAddRange(); };

        planeBtn (btnRangeShow_,
                  "Put the rings away without forgetting them. Hidden rings are "
                  "not drawn, and a mic will not snap to them.");
        btnRangeShow_.setToggleState (true, juce::dontSendNotification);
        btnRangeShow_.onClick = [this]
        {
            const bool on = btnRangeShow_.getToggleState();
            btnRangeShow_.setButtonText (on ? "Shown" : "Hidden");
            if (onShowRanges) onShowRanges (on);
        };

        styleClusterLabel (lblRange_, "Range");
        addAndMakeVisible (lblRange_);

        planeBtn (btnPlaneSeated_,
                  "Ears at 1.2 m. Records the listening height on this plane - "
                  "the engine has no vertical directivity, so it does not "
                  "change the predicted levels.");
        planeBtn (btnPlaneStanding_,
                  "Ears at 1.7 m. Records the listening height on this plane - "
                  "the engine has no vertical directivity, so it does not "
                  "change the predicted levels.");

        btnUsePlane_.onClick    = [this] { if (onUsePlane) onUsePlane (btnUsePlane_.getToggleState()); };
        btnPlaneListen_.onClick = [this] { if (onPlaneType) onPlaneType (0); };
        btnPlaneVirt_.onClick   = [this] { if (onPlaneType) onPlaneType (1); };
        btnPlaneArch_.onClick   = [this] { if (onPlaneType) onPlaneType (2); };
        btnPlaneSeated_.onClick   = [this] { if (onPlaneListenHeight) onPlaneListenHeight (0); };
        btnPlaneStanding_.onClick = [this] { if (onPlaneListenHeight) onPlaneListenHeight (1); };

        // One cluster, not two: Distance held a single button and Options
        // the pills it is always used with.
        // The old single "Options" cluster is split in two, as the Figma has it:
        // Ortho is the mode, Ortho Controls are the things it switches on.
        styleClusterLabel (lblOrtho_, "Ortho");
        addAndMakeVisible (lblOrtho_);
        styleClusterLabel (lblDistance_, "Ortho Controls");
        addAndMakeVisible (lblDistance_);

        btnInterdist_.setTooltip ("Show the gap Ortho is linking, between the "
                                  "selected speakers. Needs Ortho on.");
        // On by default, so switching Ortho on reads exactly as it used to.
        btnInterdist_.setToggleState (true, juce::dontSendNotification);
        btnInterdist_.onClick = [this]
        {
            if (onShowInterdistance) onShowInterdistance (btnInterdist_.getToggleState());
        };
        addAndMakeVisible (btnInterdist_);

        btnShowValues_.setTooltip ("Read the level anywhere: the pointer becomes "
                                   "a crosshair and shows the predicted dB at "
                                   "whatever it is over. A colour tells you "
                                   "roughly; a number tells you.");
        btnShowValues_.setEnabled (false);   // no map yet, nothing to label
        btnShowValues_.onClick = [this]
        {
            if (onShowSplValues) onShowSplValues (btnShowValues_.getToggleState());
        };
        addAndMakeVisible (btnShowValues_);

        styleClusterLabel (lblMappingType_, "Mapping Type");
        addAndMakeVisible (lblMappingType_);

        // What the canvas paints under the speakers: one of two, like the
        // combo it replaces. Off by default - the map is opt-in now.
        btnMapSpl_.setTooltip ("Paint the predicted SPL across the field.");
        btnMapOff_.setTooltip ("Paint nothing under the speakers.");
        for (auto* b : { &btnMapSpl_, &btnMapOff_ })
        {
            b->setRadioGroupId (44010);
            addAndMakeVisible (*b);
        }
        btnMapOff_.setToggleState (true, juce::dontSendNotification);
        auto pickMapping = [this] (int type)
        {
            if (type == mappingType_) return;   // re-clicking the live one
            mappingType_ = type;
            setShowValuesAvailable (type == 1);
            if (onMappingType) onMappingType (type);
        };
        btnMapSpl_.onClick = [this, pickMapping] { if (btnMapSpl_.getToggleState()) pickMapping (1); };
        btnMapOff_.onClick = [this, pickMapping] { if (btnMapOff_.getToggleState()) pickMapping (0); };

        setTextControlsEnabled (false);
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
        btnOrtho_.setTooltip ("Align selected speakers horizontally or vertically "
                              "with linked centre-to-centre spacing. Off = normal.");
        btnSnap_.setTooltip ("Lock to " + juce::String (Units::snapStepLabel())
                                 + " steps and to nearby edges/corners "
                                   "(other shapes, mics, speakers). Turn on before drawing or moving.");
        addAndMakeVisible (btnOrtho_);
        addAndMakeVisible (btnSnap_);
        styleMod (btnSplProbe_,
                  "SPL: show dB SPL under the cursor on the gradient plot "
                  "(Select tool). Turn off for a cleaner view.");
        btnSplProbe_.setToggleState (false, juce::dontSendNotification);

        btnOrthoHoriz_.setTooltip ("Align selected speakers in a horizontal row");
        btnOrthoVert_.setTooltip  ("Align selected speakers in a vertical column");
        btnOrthoHoriz_.setToggleState (true, juce::dontSendNotification);
        orthoGapLabel_.setTooltip ("Linked centre-to-centre spacing between the "
                                   "aligned speakers");
        orthoGapLabel_.setGlyphOnly (true);
        for (auto* b : { &btnOrthoHoriz_, &btnOrthoVert_, &orthoGapLabel_ })
        {
            b->setVisible (false);
            addChildComponent (*b);
        }

        orthoGapSlider_.setRange (0.5, 20.0, 0.1);
        orthoGapSlider_.setValue (3.0, juce::dontSendNotification);
        orthoGapSlider_.setSliderStyle (juce::Slider::LinearBar);
        orthoGapSlider_.setVelocityBasedMode (true);
        orthoGapSlider_.setTextBoxStyle (juce::Slider::TextBoxLeft, false, 44, 18);
        for (auto id : { juce::Slider::backgroundColourId, juce::Slider::trackColourId,
                         juce::Slider::thumbColourId })
            orthoGapSlider_.setColour (id, juce::Colours::transparentBlack);
        orthoGapSlider_.setNumDecimalPlacesToDisplay (1);
        orthoGapSlider_.setTextValueSuffix (" m");
        orthoGapSlider_.textFromValueFunction =
            [] (double metres) { return juce::String (Units::metresToDisplay (metres), 1); };
        orthoGapSlider_.valueFromTextFunction =
            [] (const juce::String& t) { return Units::displayToMetres (t.getDoubleValue()); };
        orthoGapSlider_.setTooltip ("Linked centre-to-centre spacing - changing it updates all gaps");
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
        fillAlpha_.setValue (100.0, juce::dontSendNotification);
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
                          &btnRuler_, &btnShape_, &btnZoomIn_, &btnZoomOut_,
                          &btnMagnify_ })
            tb->toFront (false);
        btnMic_.toFront (false);
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
        // Text Box: one click - no construction submenu.
        root.addItem (1000 + 6 * 100 + 9, "Text Box");

        root.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&btnShape_),
            [onPick] (int result)
            {
                if (result <= 0 || onPick == nullptr) return;
                const int enc = result - 1000;
                onPick (enc / 100, enc % 100);
            });
    }

    /** Show the selected mic's position in the ribbon's X / Y boxes, or grey
        them out when nothing is selected. */
    void setMicCoords (bool hasMic, float x, float y)
    {
        micHasSelection_ = hasMic;
        updatingMicCoords_ = true;
        micXBox_.setText (hasMic ? juce::String (Units::metresToDisplay ((double) x), 1)
                                 : juce::String ("00.0"), juce::dontSendNotification);
        micYBox_.setText (hasMic ? juce::String (Units::metresToDisplay ((double) y), 1)
                                 : juce::String ("00.0"), juce::dontSendNotification);
        updatingMicCoords_ = false;
        micXBox_.setEnabled (hasMic);
        micYBox_.setEnabled (hasMic);
    }

    /** Typed X / Y for the selected mic, in METRES. */
    std::function<void (float, float)> onMicCoordsEntered;

    /** Show Values prints readings ON the SPL map, so it can do nothing with
        the map switched off. Greyed rather than silently inert: a toggle that
        lights up and changes nothing on screen reads as a broken feature. */
    void setShowValuesAvailable (bool mapOn)
    {
        btnShowValues_.setEnabled (mapOn);
        btnShowValues_.setTooltip (mapOn
            ? "Read the level anywhere: the pointer becomes a crosshair and "
              "shows the predicted dB at whatever it is over."
            : "Read the level under the pointer - pick SPL Mapping under "
              "Mapping Type first, otherwise there is no field to read.");
    }

    void setMicArmed (bool armed)
    {
        btnMic_.setToggleState (armed, juce::dontSendNotification);
        btnMic_.setTooltip (armed
            ? "Placement armed - click the field to place (Esc cancels)"
            : "Add virtual receivers on the gradient plot");
        btnMic_.repaint();
    }

    void showMicMenu (std::function<void (int itemId)> onPick, bool hasMics,
                      bool showDegrees, bool hasRings = true,
                      bool showCoords = true)
    {
        juce::PopupMenu root;
        root.addItem (1, "Add Mic");
        // Nothing to place onto until a range exists.
        root.addItem (2, "Place on ring", hasMics && hasRings);
        root.addItem (5, "Show Coordinates", true, showCoords);
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
    RibbonIconButton     btnMic_    { "Mic", { { "Mic.png", "Mic Active.png", 2, 2, 20, 20 } } };
    void pushMicCoords()
    {
        if (updatingMicCoords_ || ! micHasSelection_ || ! onMicCoordsEntered) return;
        const double x = Units::displayToMetres (micXBox_.getText().getDoubleValue());
        const double y = Units::displayToMetres (micYBox_.getText().getDoubleValue());
        onMicCoordsEntered ((float) x, (float) y);
    }

    bool                 micHasSelection_   = false;
    bool                 updatingMicCoords_ = false;
    RibbonCoordBox       micXBox_ { "X" }, micYBox_ { "Y" };
    juce::DrawableButton btnMagnify_{ "mag", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnZoomIn_ { "zi",  juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnZoomOut_{ "zo",  juce::DrawableButton::ImageFitted };
    /** Master switch for the "Options" cluster (Snap / Ortho) that sits after
        Help in the ribbon. The Figma mock has no slot for it, so it is laid
        out but not shown: flip this to true and the cluster appears in its
        proper place, captioned and divided like every other cluster. All the
        behaviour behind it stays live either way -- Ctrl+F, the SNAP / ORTHO
        terminal verbs and MainComponent's handlers drive the same buttons. */
    static constexpr bool kShowOptionsCluster = false;

    /** Text cluster: only live while a text box is selected (or the TextBox
        tool is armed), exactly like the Opacity slider. */
    void setTextControlsEnabled (bool on)
    {
        // The cluster is contextual, like Word's Table tools: with nothing to
        // act on it takes no room at all, so the rest of the ribbon keeps the
        // width the Figma mock gives it.
        // Visibility is settled in one place now (resized), because the tab
        // has a say too: these controls need Home AND a text target.
        const bool changed = (textTargetActive_ != on);
        textTargetActive_ = on;
        if (changed) resized();
        repaint();
    }

    void setTextAlignState (int align)
    {
        btnAlignL_.setToggleState (align == 0, juce::dontSendNotification);
        btnAlignC_.setToggleState (align == 1, juce::dontSendNotification);
        btnAlignR_.setToggleState (align == 2, juce::dontSendNotification);
        repaint();
    }

    void setTextVAlignState (int valign)
    {
        btnVAlignT_.setToggleState (valign == 0, juce::dontSendNotification);
        btnVAlignM_.setToggleState (valign == 1, juce::dontSendNotification);
        btnVAlignB_.setToggleState (valign == 2, juce::dontSendNotification);
        repaint();
    }

    void setTextLineHeightState (float mult)
    {
        lineHeight_ = mult;
    }

    void setTextStyleState (bool bold, bool italic)
    {
        btnBold_.setToggleState (bold, juce::dontSendNotification);
        btnItalic_.setToggleState (italic, juce::dontSendNotification);
        repaint();
    }

    void setTextSizeState (float px)
    {
        textUpdating_ = true;
        fontSizeBox_.setText (px <= 0.5f ? juce::String ("Auto")
                                         : juce::String (juce::roundToInt (px)),
                              juce::dontSendNotification);
        textUpdating_ = false;
    }

    std::function<void(int)>   onTextAlign;
    std::function<void(int)>   onTextVAlign;
    std::function<void(float)> onTextSize;
    std::function<void(bool)>  onTextBold;
    std::function<void(bool)>  onTextItalic;
    std::function<void(float)> onTextLineHeight;
    std::function<void(bool)>  onShowRays;
    std::function<void(bool)>  onUsePlane;
    std::function<void(int)>   onPlaneType;
    std::function<void(int)>   onPlaneListenHeight;
    std::function<void()>      onAddRange;
    std::function<void(float)> onDeleteRange;
    std::function<void(bool)>  onShowRanges;

    /** The Plane cluster shares the Text Box slot: a text box can never be a
        plane, so the two are mutually exclusive and only one can ever want
        the space. */
    /** The ranges that exist now, each shown as a chip you can delete. */
    void setRanges (const std::vector<float>& metres,
                    const std::function<juce::String(float)>& label)
    {
        rangeValues_ = metres;
        rangeChips_.clear (true);
        for (float m : rangeValues_)
        {
            auto* b = new juce::TextButton();
            b->setButtonText (label (m) + "   X");
            b->setTooltip ("Delete the " + label (m) + " range ring");
            b->setComponentID ("ribbonStyle");
            b->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
            b->setColour (juce::TextButton::textColourOffId, Brand::heading());
            const float v = m;
            b->onClick = [this, v] { if (onDeleteRange) onDeleteRange (v); };
            addAndMakeVisible (b);
            rangeChips_.add (b);
        }
        resized();
        repaint();
    }

    /** Reflect whether the rings are currently drawn. */
    void setRangesShown (bool on)
    {
        btnRangeShow_.setToggleState (on, juce::dontSendNotification);
        btnRangeShow_.setButtonText (on ? "Shown" : "Hidden");
        repaint();
    }

    void setPlaneControlsEnabled (bool on)
    {
        // The cluster stays put and greys out instead of appearing and
        // vanishing: a control that disappears is one you have to go looking
        // for, and the row jumping width as you select things is worse than a
        // few dim buttons.
        planeTargetActive_ = on;
        btnUsePlane_.setEnabled (on);
        lblPlane_.setAlpha (on ? 1.0f : 0.75f);
        repaint();
    }

    void setPlaneState (bool isPlane, int type, int listenHeight = 0)
    {
        btnUsePlane_.setToggleState (isPlane, juce::dontSendNotification);
        btnPlaneListen_.setToggleState (isPlane && type == 0, juce::dontSendNotification);
        btnPlaneVirt_.setToggleState   (isPlane && type == 1, juce::dontSendNotification);
        btnPlaneArch_.setToggleState   (isPlane && type == 2, juce::dontSendNotification);
        // A type needs something to apply to AND that something has to be a
        // plane already.
        for (auto* b : { &btnPlaneListen_, &btnPlaneVirt_, &btnPlaneArch_ })
            b->setEnabled (planeTargetActive_ && isPlane);

        // Only a listening plane has ears on it, so the heights follow that
        // type rather than the cluster as a whole.
        const bool listening = isPlane && type == 0;
        btnPlaneSeated_.setToggleState   (listening && listenHeight == 0,
                                          juce::dontSendNotification);
        btnPlaneStanding_.setToggleState (listening && listenHeight == 1,
                                          juce::dontSendNotification);
        btnPlaneSeated_.setEnabled   (planeTargetActive_ && listening);
        btnPlaneStanding_.setEnabled (planeTargetActive_ && listening);
        repaint();
    }
    /** 0 = Mapping Off, 1 = SPL Mapping. */
    std::function<void(int)>   onMappingType;
    std::function<void(bool)>  onShowInterdistance;
    std::function<void(bool)>  onShowSplValues;
    std::function<void(bool)>  onLockRays;

    /** Home / Mapping, the way Word switches Home / Insert. Each tab owns its
        own controls; nothing is shared but the Ready pill. */
    void setRibbonTab (int tab)
    {
        if (ribbonTab_ == tab) return;
        ribbonTab_ = tab;
        btnTabHome_.setToggleState (tab == 0, juce::dontSendNotification);
        btnTabMapping_.setToggleState (tab == 1, juce::dontSendNotification);
        resized();
        repaint();
    }

    void setShowRaysState (bool on)
    {
        btnShowRays_.setToggleState (on, juce::dontSendNotification);
    }
    /** The steps the glyph button offers. The current one is ticked, which is
        where the value lives now that the face carries an icon. */
    void showLineSpacingMenu()
    {
        static constexpr float steps[] = { 1.0f, 1.15f, 1.25f, 1.5f, 1.75f, 2.0f };
        juce::PopupMenu m;
        for (int i = 0; i < 6; ++i)
        {
            // Nearest step wins the tick, so a value loaded from a file still
            // shows as something rather than nothing.
            bool on = true;
            for (int j = 0; j < 6; ++j)
                if (std::abs (steps[j] - lineHeight_) < std::abs (steps[i] - lineHeight_))
                    on = false;
            // Spelled out rather than formatted: trimming trailing zeros gave
            // "1x" and "2x" next to "1.5x", which read as a different kind of
            // number.
            static const char* label[] = { "1.0x", "1.15x", "1.25x",
                                           "1.5x", "1.75x", "2.0x" };
            m.addItem (i + 1, label[i], true, on);
        }
        m.showMenuAsync (juce::PopupMenu::Options()
                             .withTargetComponent (&lineHeightBtn_),
                         [this] (int r)
        {
            if (r >= 1 && r <= 6 && onTextLineHeight)
                onTextLineHeight (steps[r - 1]);
        });
    }

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
        // H / V / Gap want Ortho on AND the Mapping tab open, so the decision
        // is made in resized() where the tab is known.
        orthoExtras_ = on;
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
            "Lock to " + juce::String (Units::snapStepLabel())
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

    RibbonIconButton     btnOrtho_       { "Ortho",      { { "Ortho.svg", "Ortho Active.svg", 0, 0, 24, 24 } } };
    RibbonIconButton     btnOrthoHoriz_  { "Horizontal", { { "Align Horizontally.png",
                                                             "Align Horizontally Active.png", 0, 0, 24, 24 } } };
    RibbonIconButton     btnOrthoVert_   { "Vertical",   { { "Align Vertically.png",
                                                             "Align Vertically Active.png", 0, 0, 24, 24 } } };
    RibbonIconButton     orthoGapLabel_  { "Gap",        { { "Gap.svg", "Gap Active.svg", 3, 4.5f, 18, 15 } } };
    juce::Slider         orthoGapSlider_;
    RibbonIconButton     btnSnap_        { "Snap",       { { "Snap.svg", "Snap Active.svg", 2, 2, 20, 20 } } };
    juce::TextButton     btnSplProbe_ { "SPL" };
    juce::Label          promptLabel_;
    juce::TextButton     colourSwatch_;
    juce::Label          alphaLabel_;
    juce::Label          alphaValueLabel_;   // "52%" - top-right of the Opacity slot
    juce::Slider         fillAlpha_;
    bool                 fillAlphaEnabled_ = true;
    void syncAlphaReadout()
    {
        alphaValueLabel_.setText (juce::String ((int) fillAlpha_.getValue()) + "%",
                                  juce::dontSendNotification);
    }
    juce::TextButton     fitBtn_;
    juce::TextButton     rangeBtn_;

    // Figma redesign: File cluster (New/Open/Save/Save As/Export) - no existing
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
    TextAlignButton  btnAlignL_ { 0 }, btnAlignC_ { 1 }, btnAlignR_ { 2 };
    TextAlignButton  btnVAlignT_ { 0, true }, btnVAlignM_ { 1, true }, btnVAlignB_ { 2, true };
    juce::TextButton btnBold_, btnItalic_;
    bool             textTargetActive_ = false;
    juce::ComboBox   fontSizeBox_;
    LineSpacingButton lineHeightBtn_;
    RibbonTabButton  btnTabHome_ { "Home" }, btnTabMapping_ { "Mapping" };
    int              ribbonTab_  = 0;        // 0 Home, 1 Mapping
    int              tabStripH_  = 0;        // resolved in resized(), read by paint()
    RibbonIconButton btnShowRays_ { "Show Rays", { { "Show Rays.png", "Show Rays Active.png",
                                                     0, 0, 24, 24 } } };
    // The padlock overhangs the fan's top-right corner as drawn; the fan
    // shifts down-left so both stay inside the slot at Show Rays' size. The
    // red set keeps the padlock black, so it has no active art.
    RibbonIconButton btnLockRays_ { "Lock Rays", { { "Show Rays.png", "Show Rays Active.png",
                                                     -1.5f, 2.5f, 24, 24 },
                                                   { "Lock Badge.png", nullptr, 13.5f, -1.1f, 12, 12 } } };
    juce::Label      lblMapping_;
    RibbonIconButton btnMapSpl_ { "SPL Mapping", { { "SPL Mapping.svg", "SPL Mapping Active.svg",
                                                     1, 1, 22, 22, false } } };
    RibbonIconButton btnMapOff_ { "Mapping Off", { { "Mapping Off.svg", "Mapping Off Active.svg",
                                                     1, 1, 22, 22 } } };
    int              mappingType_ = 0;       // 0 Off, 1 SPL
    juce::Label      lblMappingType_;
    RibbonIconButton btnInterdist_  { "Show Interdistance", { { "Show Interdistance.png",
                                                                "Show Interdistance Active.png", 0, 0, 24, 24 } } };
    RibbonIconButton btnShowValues_ { "Show Values",        { { "Show Values.png",
                                                                "Show Values Active.png", 0, 0, 24, 24 } } };
    RibbonIconButton btnUsePlane_     { "Use as Plane",  { { "Use as Plane.png", "Use as Plane Active.png",
                                                             0, 0, 24, 24 } } };
    RibbonIconButton btnPlaneVirt_    { "Virtual",       { { "Virtual Plane.png", "Virtual Plane Active.png",
                                                             0, 0, 24, 24 } } };
    RibbonIconButton btnPlaneArch_    { "Architectural", { { "Architectural Plane.png",
                                                             "Architectural Plane Active.png", 0, 0, 24, 24 } } };
    RibbonIconButton btnPlaneListen_  { "Listening",     { { "Listening Plane.png", "Listening Plane Active.png",
                                                             0, 0, 24, 24 } } };
    RibbonIconButton btnPlaneSeated_  { "Seated",        { { "Seated.png", "Seated Active.png", 0, 0, 24, 24 } } };
    RibbonIconButton btnPlaneStanding_ { "Standing",     { { "Standing.png", "Standing Active.png", 0, 0, 24, 24 } } };
    RibbonIconButton btnRangeAdd_     { "+ Range",       { { "Add Range.png", "Add Range Active.png",
                                                             0, 0, 24, 24 } } };
    RibbonIconButton btnRangeShow_    { "Shown",         { { "Show Ranges.png", "Show Ranges Active.png",
                                                             0, 0, 24, 24 } } };
    juce::Label      lblRange_;
    // One chip per range the user has added, each with its own delete.
    juce::OwnedArray<juce::TextButton> rangeChips_;
    std::vector<float>                 rangeValues_;
    juce::Label      lblPlane_;
    juce::Label      lblMic_;
    bool             planeTargetActive_ = false;
    juce::Label      lblDistance_;   // caption: "Ortho Controls"
    juce::Label      lblOrtho_;      // caption: "Ortho"
    bool             orthoExtras_ = false;   // Ortho is on: show H / V / Gap
    // The text cluster only: it is contextual on top of the tab, so it needs
    // its own rule. Home is every other child, decided in resized().
    std::vector<juce::Component*> textChildren_;
    float             lineHeight_ = 1.0f;
    juce::Label      lblText_;
    bool             textUpdating_ = false;

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
    // cluster like every other group - set once via setHelpIcons().
    juce::Component* helpIcons_[3] = { nullptr, nullptr, nullptr };

    // "Ready" pill (also owned by MainComponent, reparented) - sits at the
    // right end of the tab strip, where long status messages have room.
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
        and positions it over the plot - setTitle() keeps working either way. */
    juce::Label& getTitleLabel() noexcept { return title_; }
private:

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panelDark());

        // The tab strip is its own band, closed by a hairline so the live
        // tab's accent rule has an edge to sit on.
        if (tabStripH_ > 0)
        {
            g.setColour (Brand::border().withAlpha (0.35f));
            g.drawHorizontalLine (tabStripH_ - 1, 0.0f, (float) getWidth());
        }

        // Figma: cluster rules run the full height of the row, and the row is
        // closed by a hairline along its bottom edge.
        g.setColour (Brand::border().withAlpha (UiConfig::Ribbon::dividerAlpha));
        for (int x : dividerX_)
            g.drawVerticalLine (x, (float) tabStripH_, (float) getHeight());
        // The row's closing edge keeps its own weight - it separates the
        // ribbon from the canvas, not one cluster from the next.
        g.setColour (Brand::border().withAlpha (0.45f));
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

        // The tab decides what exists at all; the Figma hide-list below then
        // takes its usual bite out of Home.
        const bool homeTab = (ribbonTab_ == 0);
        for (auto* c : getChildren())
        {
            // The strip itself and the other tab's controls are not Home's to
            // hide; title_ is reparented onto the canvas by MainComponent.
            if (c == (juce::Component*) &btnTabHome_
                || c == (juce::Component*) &btnTabMapping_
                || c == (juce::Component*) &btnShowRays_
                || c == (juce::Component*) &lblMapping_
                || c == (juce::Component*) &btnMapSpl_
                || c == (juce::Component*) &btnMapOff_
                || c == (juce::Component*) &lblMappingType_
                || c == (juce::Component*) &btnInterdist_
                || c == (juce::Component*) &btnLockRays_
                || c == (juce::Component*) &lblDistance_
                || c == (juce::Component*) &lblOrtho_
                || c == (juce::Component*) &btnSnap_
                || c == (juce::Component*) &btnOrtho_
                || c == (juce::Component*) &btnOrthoHoriz_
                || c == (juce::Component*) &btnOrthoVert_
                || c == (juce::Component*) &orthoGapLabel_
                || c == (juce::Component*) &orthoGapSlider_
                || c == (juce::Component*) &lblOptions_
                || c == (juce::Component*) &title_
                || c == readyPill_)     // solver status, not a Home control
                continue;

            const bool isText = std::find (textChildren_.begin(), textChildren_.end(), c)
                                    != textChildren_.end();
            if (c == (juce::Component*) &btnMic_ || c == (juce::Component*) &lblMic_)
            {
                c->setVisible (! homeTab);
                continue;
            }

            const bool isPlaneCtl = (c == (juce::Component*) &btnPlaneSeated_
                                  || c == (juce::Component*) &btnPlaneStanding_
                                  || c == (juce::Component*) &btnUsePlane_
                                  || c == (juce::Component*) &btnPlaneListen_
                                  || c == (juce::Component*) &btnPlaneVirt_
                                  || c == (juce::Component*) &btnPlaneArch_
                                  || c == (juce::Component*) &lblPlane_);
            if (isPlaneCtl)      c->setVisible (! homeTab);
            else if (isText)     c->setVisible (homeTab && textTargetActive_);
            else                 c->setVisible (homeTab);
        }
        micXBox_.setVisible         (! homeTab);
        micYBox_.setVisible         (! homeTab);
        btnShowRays_.setVisible     (! homeTab);
        btnLockRays_.setVisible     (! homeTab);
        lblMapping_.setVisible      (! homeTab);
        btnMapSpl_.setVisible       (! homeTab);
        btnMapOff_.setVisible       (! homeTab);
        lblMappingType_.setVisible  (! homeTab);
        btnInterdist_.setVisible    (! homeTab);
        // Nothing to show without Ortho, and a live-looking button that does
        // nothing is worse than a greyed one that explains itself.
        btnInterdist_.setEnabled    (orthoExtras_);
        btnShowValues_.setVisible   (! homeTab);
        lblDistance_.setVisible     (! homeTab);
        lblOrtho_.setVisible        (! homeTab);
        btnSnap_.setVisible         (! homeTab);
        btnOrtho_.setVisible        (! homeTab);
        // The H / V / Gap row stays where it is and greys out instead of
        // vanishing: controls that appear and disappear move everything
        // around them, and you cannot learn a control you have never seen.
        btnOrthoHoriz_.setVisible  (! homeTab);
        btnOrthoVert_.setVisible   (! homeTab);
        orthoGapLabel_.setVisible  (! homeTab);
        orthoGapSlider_.setVisible (! homeTab);
        btnOrthoHoriz_.setEnabled  (orthoExtras_);
        btnOrthoVert_.setEnabled   (orthoExtras_);
        orthoGapSlider_.setEnabled (orthoExtras_);
        orthoGapLabel_.setEnabled  (orthoExtras_);
        orthoGapLabel_.setToggleState (orthoExtras_, juce::dontSendNotification);
        lblOptions_.setVisible     (false);   // merged into the Options cluster
        btnRangeAdd_.setVisible (! homeTab);
        // Nothing to show or hide until there is a range.
        btnRangeShow_.setVisible (! homeTab);
        btnRangeShow_.setEnabled (! rangeChips_.isEmpty());
        lblRange_.setVisible    (! homeTab);
        for (auto* c : rangeChips_) c->setVisible (! homeTab);

        // Anything the Figma ribbon has no slot for is hidden (code/logic
        // untouched - flip setVisible back on to restore). The red
        // "SPL Heatmap | ..." caption belongs on the canvas, not here, so
        // MainComponent reparents title_ via getTitleLabel().
        for (juce::Component* c : { (juce::Component*) &btnSplProbe_,
                                     (juce::Component*) &rangeBtn_,
                                     // btnMic_ is NOT here any more - it has a
                                     // home in the Mapping tab (see below).
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
        // rather than by accumulating pads - flow layout drifted further right
        // with each cluster. `shrink` scales the whole row down together when
        // the window is narrower than the design.
        struct Cluster { int iconX, dividerX; };
        static constexpr Cluster kFile    { 22,  138 };   // Figma  29 / 182
        static constexpr Cluster kNav     { 152, 207 };   //       200 / 272
        // View gained a fourth icon (the magnifier), so it is one pitch wider
        // than the mock and every cluster after it moves right by the same 23
        // units - the row had air to spare past Help, the same way Opacity was
        // widened earlier.
        static constexpr Cluster kView    { 211, 303 };   //       278 / 369 + 23
        static constexpr Cluster kTools   { 310, 381 };   //       378 / 471 + 23
        // Opacity needs more room than the Figma mock gave it: at a legible
        // caption size "Opacity" and "100%" do not both fit in the original
        // 56-unit slot, so the caption lost its last letter and the value lost
        // its % sign. Widened by 60 units and everything after it shifted by
        // the same amount -- the row had empty space to spare to the right of
        // Help, so nothing is pushed off the end.
        static constexpr Cluster kOpacity { 389, 511 };   // Opacity slot (Tools|Shapes)
        static constexpr Cluster kShapes  { 517, 661 };   // Figma 478 / 668 + 60
        static constexpr Cluster kColours { 666, 726 };   //       675 / 754 + 60
        static constexpr Cluster kHelp    { 729, 797 };   //       758 / 847 + 60
        // Options (Snap / Ortho) continues the row past Help on the same
        // rhythm the mock uses elsewhere: 14 units of air after the preceding
        // divider, then the controls, then 14 more before the next rule.
        static constexpr Cluster kOptions { 751, 875 };
        // Text Box carries on past Help on the same rhythm: a little air after
        // the preceding rule, three alignment glyphs on the icon pitch, then
        // the size box, then the closing rule.
        static constexpr Cluster kText    { 811, 1109 };
        static constexpr int kOptionPillGap = 6;
        static constexpr int kPlateLeft = 15;             // Figma  20
        static constexpr int kPitch     = 23;             // Figma  30 icon pitch

        // Mapping carries more clusters than Home, so the row it has to fit in
        // is wider - working `shrink` out from Home's last rule would let the
        // Mapping row run past the Ready pill on a narrow window.
        //
        // This is SUMMED from the same numbers the Mapping layout uses below
        // rather than written down as one figure. A hard-coded width goes
        // stale the moment a cluster is added - which is exactly what happened
        // when Range arrived and the pill got squeezed down to its dot.
        constexpr int kMapGap   = 14;                                   // air each side of a rule

        // --- Compact mode -----------------------------------------------
        // On a laptop the Mapping row asks for more width than the window
        // has, and `shrink` paid for the difference by scaling everything -
        // including the text - until nothing could be read. Shrinking a long
        // word is the worst way to save width: the honest way is to use a
        // shorter word. Below the width where the full labels fit, the row
        // switches to abbreviations and the narrower slots that go with them,
        // which buys back enough that what remains renders near full size.
        // Every Mapping control but the mic's X / Y and the range chips is a
        // glyph on the icon pitch now. `type` is the room the "Mapping Type"
        // caption needs over its two glyphs.
        struct MapWidths
        {
            int type, chip, coord;
        };
        static constexpr MapWidths kFull    { 116, 54, 56 };
        static constexpr MapWidths kCompact {  96, 44, 48 };
        constexpr int kIconW    = L::ribbonIconSize;
        // The Mapping glyphs carry no word beside them, so they are drawn a
        // third larger than Home's tools, on a pitch to match.
        constexpr int kMapIconW = 24;
        constexpr int kMapPitch = 31;
        constexpr int kSubGroup = 8;                                    // Values | Snap

        auto mapDemand = [&] (const MapWidths& w)
        {
            const int rays  = kMapPitch + kMapIconW;
            // Top row of glyphs, or the caption row (Options, H, V, Gap and
            // the spacing slider) - whichever is wider sets the cluster.
            // Ortho is one glyph under its own caption; Ortho Controls is a
            // single row of six glyphs and the gap read-out, with the caption
            // underneath like every other cluster. The caption is the wider
            // of the two things in the Ortho Controls cluster only on very
            // narrow rows, so both are accounted for.
            const int ortho = juce::jmax (kMapIconW, 40);
            const int opts  = juce::jmax (kMapPitch * 5 + kMapIconW + kOptionPillGap + 48,
                                          96);
            const int plane = kMapPitch * 5 + kMapIconW;
            const int mic   = kMapIconW + 6 + (12 + 3 + w.coord + 6) * 2 + 8
                            + kMapPitch + kMapIconW;
            // The range chips share the cluster, so the row grows as ranges
            // are added and has to shrink to match - otherwise adding a third
            // range walks the row back over the pill.
            const int chips = rangeChips_.isEmpty()
                ? 0 : (int) rangeChips_.size() * w.chip
                      + ((int) rangeChips_.size() - 1) * 4;
            return kPlateLeft + rays + kMapGap
                 + kMapGap + juce::jmax (w.type, kMapPitch + kMapIconW) + kMapGap
                 + kMapGap + ortho  + kMapGap
                 + kMapGap + opts   + kMapGap
                 + kMapGap + plane  + kMapGap
                 + kMapGap + juce::jmax (mic, chips) + kMapGap;
        };

        const int fullDemandX = mapDemand (kFull);
        // Compared against the same magnified grid the row is actually laid
        // out on - see gridUp below - or the test would say the full labels
        // fit when they do not.
        const int fullBaseIcon = juce::jmax (1, UiConfig::Scale::px (L::ribbonIconSize));
        const float fullGridUp = juce::jmax (1.0f, 21.0f / (float) fullBaseIcon);
        // The status pill lives in the tab strip, so the row only keeps a
        // right margin to match its left one.
        const int fullDesignW = juce::roundToInt (
            UiConfig::Scale::px (fullDemandX + kPlateLeft) * fullGridUp);
        // Abbreviate only when the full name genuinely cannot be READ, not
        // merely when the row is narrower than its design width. The first
        // version asked the second question and so shortened every label the
        // moment the window was not enormous - at 1493 px "Show Interdistance"
        // still renders near 13 px with room to spare, and there is no reason
        // to call it "Interdist" there. The row shrinks to about 0.62 before
        // the longest label stops fitting its pill at a readable size; below
        // that the choice is a short word or an unreadable one, and a short
        // word wins.
        const float fullShrink = juce::jlimit (0.45f, 1.0f,
            fullDesignW > 0 ? (float) getWidth() / (float) fullDesignW : 1.0f);
        const bool mapCompact = (! homeTab) && (fullShrink < 0.62f);
        const MapWidths& MW = mapCompact ? kCompact : kFull;

        const int mappingDividerX = mapDemand (MW);

        const int lastDividerX = (ribbonTab_ != 0)
            ? mappingDividerX
            : (textTargetActive_ ? kText.dividerX : kHelp.dividerX);
        // The icon buttons have a floor in real pixels (see `tool` below): the
        // global UI factor bottoms out at 0.78 on a laptop, which would leave
        // them 14 px tall and cap their own text at about 10 px. Raising the
        // icon alone would break the row, because every cluster's x comes from
        // a design grid that assumes the icon is px(ribbonIconSize) - the
        // icons simply grew into their neighbours. So the floor is expressed
        // as a magnification of the WHOLE grid, and `shrink` then pulls the
        // magnified row back to the window. Proportions hold either way.
        const int   baseIcon = juce::jmax (1, UiConfig::Scale::px (L::ribbonIconSize));
        const float gridUp   = juce::jmax (1.0f, 21.0f / (float) baseIcon);
        const int designW = juce::roundToInt (
            UiConfig::Scale::px (lastDividerX + kPlateLeft) * gridUp);
        const float shrink = juce::jlimit (0.45f, 1.0f,
                                           designW > 0 ? (float) getWidth() / (float) designW : 1.0f);
        auto px2 = [&] (int v)
        {
            return juce::jmax (1, juce::roundToInt ((float) UiConfig::Scale::px (v)
                                                     * gridUp * shrink));
        };

        // `shrink` is a HORIZONTAL remedy - it exists because the clusters do
        // not fit the window's width. It used to be applied to the vertical
        // metrics as well, which shrank the row's height on a narrow window
        // although the band it sits in is a fixed 80 units tall and had some
        // 45 px going spare. The cost landed on the text: a pill squeezed to
        // 15 px tall can only carry about 10 px of ink however large a font it
        // is asked for, which is why the ribbon read small at any ordinary
        // window size. Heights now stay at full scale and only x moves.
        // `pitch` already takes jmax (tool + 1, ...), so wider icons spread
        // out rather than overlapping when the row is tight.
        // Floors are in real pixels, not design units. The global UI factor
        // bottoms out at 0.78 on a laptop-sized window, which left the icon
        // buttons 14 px tall - and a pill that short caps its own text at
        // about 10 px however large a font it is given. These are the sizes
        // below which the row stops being readable, so they do not scale.
        // Magnified with the grid, then held to the floor: on a wide window
        // gridUp is 1 and this is simply the design size.
        const int tool       = juce::jmax (14, juce::roundToInt (baseIcon * gridUp * shrink));
        const int pitch      = juce::jmax (tool + 1, px2 (kPitch));
        // The controls keep their Figma geometry; the whole row just starts
        // below the tab strip instead of at the top of the band.
        const int tabStrip   = juce::jmax (24, UiConfig::Scale::px (L::ribbonTabStripH));
        tabStripH_ = tabStrip;
        const int iconTop    = tabStrip + juce::jmax (2, UiConfig::Scale::px (L::ribbonIconTop));
        const int labelTop   = tabStrip + juce::jmax (tool + 2, UiConfig::Scale::px (L::ribbonLabelTop));
        const int labelH     = juce::jmax (19, UiConfig::Scale::px (L::ribbonLabelH));
        // Colours palette: 14px dots on a 20x16 grid in the Figma mock.
        const int swatch     = juce::jmax (6, px2 (L::ribbonSwatch));
        const int pitchX     = juce::jmax (swatch + 1, px2 (L::ribbonSwatchPitchX));
        const int pitchY     = juce::jmax (swatch + 1, px2 (L::ribbonSwatchPitchY));
        // Figma's glyphs are 24x24 in a 24px slot - no inset, so the artwork
        // fills the button rather than sitting in a shrunken well.
        const int edgeIndent = 0;

        for (auto* tb : { &btnSelect_, &btnPan_, &btnPencil_, &btnEraser_,
                          &btnRuler_, &btnZoomIn_, &btnZoomOut_, &btnFitView_,
                          &btnMagnify_ })
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

        // The neighbours in View are raster glyphs with their own transparent
        // margin baked in, so at edge indent 0 they draw about 16 px of ink in
        // a 21 px button. A vector has no such margin: ImageFitted scales its
        // content bounds to fill the button, so the magnifier came out a size
        // larger than the row. Scaling the SVG's own coordinates does nothing
        // for the same reason - the fit is to the ink, not the viewBox. The
        // margin has to come from the button, so it is given here.
        btnMagnify_.setEdgeIndent (edgeIndent + juce::jmax (1, tool / 16));

        dividerX_.clear();

        // Tab strip, left-aligned on the same margin the clusters start from.
        int tabsRight = 0;
        {
            int tx = px2 (kPlateLeft);
            const int th = juce::jmax (12, tabStrip - px2 (2));
            struct TabSlot { RibbonTabButton* b; int w; };
            const TabSlot slots[] = { { &btnTabHome_, 58 }, { &btnTabMapping_, 86 } };
            for (const auto& sl : slots)
            {
                const int w = px2 (sl.w);
                sl.b->setBounds (tx, px2 (1), w, th);
                tabsRight = tx + w;
                tx += w + px2 (4);
            }
        }

        // Captions are sized here, not in the constructor: at construction time
        // Brand::UI::scale is still 1.0, so a font set there stays at its base
        // size and renders roughly half as large as the design calls for.
        {
            // The caption row keeps its full height now, so the captions keep
            // their full size: a cluster's name is the thing you read to find
            // the control, and shrinking it was saving width it does not use
            // (captions are centred on a plate, not laid end to end).
            const auto capFont = Brand::tech (juce::jmax (12.5f,
                Brand::UI::scaledFont (Brand::Type::ribbonClusterLabel)));
            const auto coordFont = Brand::tech (juce::jmax (12.0f,
                Brand::UI::scaledFont (Brand::Type::ribbonPillText)));
            micXBox_.setFont (coordFont);
            micYBox_.setFont (coordFont);
            for (auto* l : { &lblFile_, &lblNav_, &lblView_, &lblTools_, &lblShapes_,
                             &lblColours_, &lblHelp_, &lblOptions_, &lblMapping_,
                             &lblMappingType_, &lblDistance_, &lblOrtho_,
                             &lblPlane_, &lblMic_ })
                l->setFont (capFont);
        }

        // Lays a cluster's icons on the Figma pitch from its own start x, then
        // centres the caption on the plate between the surrounding dividers -
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

        // Placed after every other cluster below, once plateLeft has walked
        // across the row - see the Text Box block at the end of resized().
        cluster (kFile, lblFile_,
                 { &btnFileNew_, &btnFileOpen_, &btnFileSave_, &btnFileSaveAs_, &btnFileExport_ });
        cluster (kNav,   lblNav_,   { &btnSelect_, &btnPan_ });
        cluster (kView,  lblView_,  { &btnZoomIn_, &btnZoomOut_, &btnFitView_, &btnMagnify_ });
        cluster (kTools, lblTools_, { &btnPencil_, &btnEraser_, &btnRuler_ });

        // Opacity: caption + percentage on one line, track beneath. It has no
        // caption in the bottom label row - its own label sits up top instead.
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
        // text box is gone - see the hidden-controls list above.
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
        // on, as glyphs on the icon pitch.
        if (kShowOptionsCluster)
        {
            const int pillGap = juce::jmax (2, px2 (kOptionPillGap));
            int x = px2 (kOptions.iconX);

            btnSnap_.setBounds  (x, iconTop, tool, tool); x += pitch;
            btnOrtho_.setBounds (x, iconTop, tool, tool); x += pitch;

            // H / V / Gap sit on the caption row beneath the two pills so the
            // cluster keeps its width when Ortho is switched on.
            if (btnOrthoHoriz_.isVisible())
            {
                const int sq = juce::jmax (10, labelH);
                int ex = px2 (kOptions.iconX);
                btnOrthoHoriz_.setBounds (ex, labelTop, sq, sq); ex += sq + pillGap;
                btnOrthoVert_.setBounds  (ex, labelTop, sq, sq); ex += sq + pillGap;
                orthoGapLabel_.setBounds (ex, labelTop, sq, sq); ex += sq + pillGap;
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
        else if (homeTab)
        {
            // Home has no Options slot in the Figma ribbon, so the pills stay
            // parked here. They are NOT parked on the Mapping tab - that tab
            // lays them out for real further down, and zeroing them here would
            // undo it.
            for (juce::Component* c : { (juce::Component*) &btnSnap_,  (juce::Component*) &btnOrtho_,
                                        (juce::Component*) &btnOrthoHoriz_, (juce::Component*) &btnOrthoVert_,
                                        (juce::Component*) &orthoGapLabel_, (juce::Component*) &orthoGapSlider_,
                                        (juce::Component*) &lblOptions_ })
            {
                c->setVisible (false);
                c->setBounds (0, 0, 0, 0);
            }
        }

        // Text Box: horizontal alignment, vertical alignment, B / I, then the
        // size list. Contextual - laid out only when a text box is the target,
        // so the row is untouched the rest of the time.
        if (textTargetActive_)
        {
            const int grp = juce::jmax (2, px2 (6));   // air between sub-groups
            int tx = px2 (kText.iconX);
            for (auto* b : { (juce::Component*) &btnAlignL_, (juce::Component*) &btnAlignC_,
                             (juce::Component*) &btnAlignR_ })
            {
                b->setBounds (tx, iconTop, tool, tool);
                tx += pitch;
            }
            tx += grp;
            for (auto* b : { (juce::Component*) &btnVAlignT_, (juce::Component*) &btnVAlignM_,
                             (juce::Component*) &btnVAlignB_ })
            {
                b->setBounds (tx, iconTop, tool, tool);
                tx += pitch;
            }
            tx += grp;
            btnBold_.setBounds   (tx, iconTop, tool, tool); tx += pitch;
            btnItalic_.setBounds (tx, iconTop, tool, tool); tx += pitch;

            // Two fixed lists rather than one stretching to the rule, so
            // neither collapses when the ribbon is shrunk.
            const int plateRight = px2 (kText.dividerX);
            tx += grp;
            fontSizeBox_.setBounds (tx, iconTop, px2 (50), tool);
            tx += px2 (50) + grp;
            lineHeightBtn_.setBounds (tx, iconTop, px2 (32), tool);
            lblText_.setBounds (plateLeft, labelTop,
                                juce::jmax (10, plateRight - plateLeft), labelH);
            dividerX_.push_back (plateRight);
            plateLeft = plateRight;
        }

        // Mapping tab. The Home clusters above have already run and set bounds
        // on components that are now hidden, which costs nothing and keeps that
        // block in one piece - but their dividers would still be painted, so
        // the list is rebuilt here for this tab alone.
        if (! homeTab)
        {
            dividerX_.clear();
            // The glyphs are larger than Home's tools but share their centre
            // line with the mic's X / Y boxes, and stop short of the captions.
            const int mIcon  = juce::jlimit (tool, juce::jmax (tool, labelTop - tabStrip - 4),
                                             px2 (kMapIconW));
            const int mPitch = juce::jmax (mIcon + 2, px2 (kMapPitch));
            const int mTop   = juce::jmax (tabStrip + 2, iconTop + tool / 2 - mIcon / 2);

            // Every cluster on this row is built the same way: the plate is
            // as wide as the wider of its glyph row and its caption, the
            // glyphs are CENTRED on it, and the caption is centred under
            // them. The glyphs used to start hard against the plate's left
            // edge while the caption centred itself, so any cluster whose
            // caption was wider than its glyphs carried a block of dead
            // space on its right - Mapping Type worst of all.
            const int plateGap = px2 (14);
            auto capWidthOf = [&] (const juce::Label& l)
            {
                return l.getFont().getStringWidth (l.getText()) + px2 (10);
            };
            // Returns where the glyph row starts; writes the plate's right edge.
            auto placePlate = [&] (int left, int rowW, const juce::Label& cap,
                                   int& plateRightOut)
            {
                const int inner = juce::jmax (rowW, capWidthOf (cap));
                plateRightOut = left + inner + plateGap;
                return left + (inner - rowW) / 2;
            };
            auto captionOn = [&] (juce::Label& l, int left, int plateRightV)
            {
                l.setBounds (left, labelTop, juce::jmax (10, plateRightV - left - plateGap), labelH);
            };

            const int mx = px2 (kPlateLeft);
            int plateRight = 0;
            {
                const int rowW = mPitch + mIcon;
                const int gx = placePlate (mx, rowW, lblMapping_, plateRight);
                btnShowRays_.setBounds (gx, mTop, mIcon, mIcon);
                btnLockRays_.setBounds (gx + mPitch, mTop, mIcon, mIcon);
                captionOn (lblMapping_, mx, plateRight);
            }
            dividerX_.push_back (plateRight);

            // Mapping Type: its own cluster, because it governs the canvas
            // rather than the rays. Two glyphs, one of them always live; the
            // slot keeps the caption's width and the pair starts at its left
            // edge like every other cluster.
            const int mx2 = plateRight + plateGap;
            int plateRight2 = 0;
            {
                const int rowW = mPitch + mIcon;
                const int gx = placePlate (mx2, rowW, lblMappingType_, plateRight2);
                btnMapSpl_.setBounds (gx,          mTop, mIcon, mIcon);
                btnMapOff_.setBounds (gx + mPitch, mTop, mIcon, mIcon);
                captionOn (lblMappingType_, mx2, plateRight2);
            }
            dividerX_.push_back (plateRight2);

            // Ortho: the mode on its own, one glyph under its own caption.
            // It used to sit at the end of the Options row, which made the
            // switch look like one more option rather than the thing the
            // options belong to.
            const int orx0 = plateRight2 + plateGap;
            int plateRightO = 0;
            {
                const int gx = placePlate (orx0, mIcon, lblOrtho_, plateRightO);
                btnOrtho_.setBounds (gx, mTop, mIcon, mIcon);
                captionOn (lblOrtho_, orx0, plateRightO);
            }
            dividerX_.push_back (plateRightO);

            // Ortho Controls: ONE row of glyphs with the caption underneath,
            // the same shape as every other cluster on the row. They used to
            // be split across two rows - four glyphs on top, H / V / Gap down
            // on the caption line - which is why this cluster read differently
            // from its neighbours.
            //
            // Row: Show Interdistance | Show Values | Snap | H | V | Gap | 0.0 m
            // Everything except the first three greys out when Ortho is off;
            // nothing moves, because a control you cannot find again is worse
            // than a greyed one.
            const int pillGap = juce::jmax (2, px2 (kOptionPillGap));
            const int ox0 = plateRightO + plateGap;

            // The read-out is sized HERE, not in the constructor: at
            // construction Brand::UI::scale is still 1.0, so a size set there
            // stays at its base. The floor is in real pixels because the box
            // has to hold "3.0 m" whatever the row's shrink is doing, and a
            // read-out that says "3...." is worse than no read-out.
            const int tbW = juce::jmax (74, px2 (68));   // holds "3.0 m" on one line
            const int subGroup = px2 (kSubGroup);

            // Measure the row first so it can be centred, exactly like the
            // clusters either side of it.
            const int ctlRowW = mPitch * 5 + subGroup + mIcon + pillGap + tbW;
            int plateRight3 = 0;
            const int ctlX = placePlate (ox0, ctlRowW, lblDistance_, plateRight3);

            int ox = ctlX;
            btnInterdist_.setBounds  (ox, mTop, mIcon, mIcon); ox += mPitch;
            btnShowValues_.setBounds (ox, mTop, mIcon, mIcon); ox += mPitch + subGroup;
            btnSnap_.setBounds       (ox, mTop, mIcon, mIcon); ox += mPitch;
            btnOrthoHoriz_.setBounds (ox, mTop, mIcon, mIcon); ox += mPitch;
            btnOrthoVert_.setBounds  (ox, mTop, mIcon, mIcon); ox += mPitch;
            orthoGapLabel_.setBounds (ox, mTop, mIcon, mIcon); ox += mIcon + pillGap;

            // The gap is a boxed read-out beside its glyph, like the mic's
            // X / Y - the slider's track is gone, so the whole cluster fits
            // one row. Typing and the mouse wheel still set it.
            if (orthoGapSlider_.getTextBoxWidth() != tbW
                || orthoGapSlider_.getTextBoxHeight() != mIcon)
                orthoGapSlider_.setTextBoxStyle (juce::Slider::TextBoxLeft,
                                                 false, tbW, mIcon);
            orthoGapSlider_.setBounds (ox, mTop, tbW, mIcon);

            captionOn (lblDistance_, ox0, plateRight3);
            dividerX_.push_back (plateRight3);
            plateLeft = plateRight3;

            // Plane: always present on this tab, greyed until the selection is
            // a shape that could be a surface.
            {
                const int clusterLeft = plateRight3 + plateGap;
                int plateRightP = 0;
                // Figma order: Use as Plane, Virtual, Architectural,
                // Listening, Seated, Standing.
                int px0 = placePlate (clusterLeft, mPitch * 5 + mIcon, lblPlane_, plateRightP);
                for (auto* b : { &btnUsePlane_, &btnPlaneVirt_, &btnPlaneArch_,
                                 &btnPlaneListen_, &btnPlaneSeated_, &btnPlaneStanding_ })
                {
                    b->setBounds (px0, mTop, mIcon, mIcon);
                    px0 += mPitch;
                }
                captionOn (lblPlane_, clusterLeft, plateRightP);
                dividerX_.push_back (plateRightP);
                plateLeft = plateRightP;

                // Mic: virtual receivers. A measurement tool, so it belongs
                // with the mapping controls rather than the drawing ones.
                // Mic and Range share a cluster: both are measurement tools
                // rather than drawing ones, and giving each its own rule cost
                // more width than the row has.
                const int micLeft = plateRightP + plateGap;
                const int coordWp = px2 (12 + 3 + MW.coord);
                const int micRowW = mIcon + px2 (6) + (coordWp + px2 (6)) * 2 + px2 (2)
                                  + mPitch + mIcon;
                int plateRightMcap = 0;
                const int mx0 = placePlate (micLeft, micRowW, lblMic_, plateRightMcap);
                btnMic_.setBounds (mx0, mTop, mIcon, mIcon);

                // The selected mic's X / Y, typed here rather than only in the
                // sidebar: the mic lives in this cluster, so the numbers that
                // place it belong beside it.
                const int coordW = px2 (12 + 3 + MW.coord);   // mapDemand assumes this
                int cxp = mx0 + mIcon + px2 (6);
                micXBox_.setBounds (cxp, iconTop, coordW, tool); cxp += coordW + px2 (6);
                micYBox_.setBounds (cxp, iconTop, coordW, tool); cxp += coordW + px2 (8);

                const int rx0 = cxp;
                btnRangeAdd_.setBounds  (rx0,          mTop, mIcon, mIcon);
                btnRangeShow_.setBounds (rx0 + mPitch, mTop, mIcon, mIcon);

                // The ranges already added sit on the caption row, each one
                // carrying its own delete, so you can see and remove them
                // without going looking.
                const int chipW = px2 (MW.chip);   // mapDemand assumes this
                const int chipGap = juce::jmax (2, px2 (4));
                int cx = mx0;
                for (auto* c : rangeChips_)
                {
                    c->setBounds (cx, labelTop, chipW, juce::jmax (12, labelH));
                    cx += chipW + chipGap;
                }
                const int iconRowRight = rx0 + mPitch + mIcon;
                const int chipRowRight = rangeChips_.isEmpty() ? mx0 : (cx - chipGap);
                const int plateRightM = juce::jmax (juce::jmax (iconRowRight, chipRowRight) + px2 (14),
                                                    plateRightMcap);
                if (rangeChips_.isEmpty())
                {
                    captionOn (lblMic_, micLeft, plateRightM);
                    lblRange_.setBounds (0, 0, 0, 0);
                }
                else
                {
                    lblMic_.setBounds (0, 0, 0, 0);
                    lblRange_.setBounds (0, 0, 0, 0);
                }
                dividerX_.push_back (plateRightM);
                plateLeft = plateRightM;
            }
        }

        // Status pill ("Ready", "Project saved: ...") - right end of the tab
        // strip, between its two hairlines. In the control row it competed
        // with the clusters for width and the longer messages were cut to
        // "Project..."; the strip is empty past the two tabs, so the whole
        // message fits and the clusters keep the row to themselves.
        if (readyPill_ != nullptr)
        {
            const int rightPad = px2 (kPlateLeft);
            int readyW = px2 (100);
            if (readyPillWidth_)
                readyW = juce::jmax (readyW, readyPillWidth_());
            const int maxW = juce::jmax (px2 (60), getWidth() - rightPad - tabsRight - px2 (24));
            readyW = juce::jmin (readyW, maxW);
            readyPill_->setBounds (getWidth() - rightPad - readyW, 1,
                                   readyW, juce::jmax (12, tabStrip - 2));
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
        btnSplProbe_.setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
        btnSplProbe_.setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.35f));
        btnSplProbe_.setColour (juce::TextButton::textColourOffId,
                                AppSettings::get().isDark() ? Brand::white() : Brand::onBtnIn());
        btnSplProbe_.setColour (juce::TextButton::textColourOnId,
                                AppSettings::get().isDark() ? Brand::white() : Brand::onBtnIn());
        colourSwatch_.setColour (juce::TextButton::buttonColourId, drawColour_);
        promptLabel_.setColour (juce::Label::textColourId, Brand::accent());
        promptLabel_.setFont (Brand::mono (Brand::UI::scaledFont (Brand::Type::plotToolbarLabel)));
        for (auto* l : { &lblFile_, &lblNav_, &lblView_, &lblTools_, &lblShapes_,
                         &lblColours_, &lblHelp_, &lblOptions_, &lblMapping_,
                         &lblMappingType_, &lblDistance_, &lblPlane_, &lblMic_ })
        {
            l->setColour (juce::Label::textColourId, Brand::text());
            l->setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::ribbonClusterLabel)));
        }
        // resized() sizes the captions against the row's current shrink; the
        // line above only knows the unshrunk size, so hand it back to layout.
        resized();

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
        restyle (btnMagnify_, kMagnifySVG);
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
    juce::Colour drawColour_ { 0xffa9c7ff };

    // Pan / Pencil / Eraser / Ruler / Zoom / Full-screen ship as reference art
    // in the project's /Icons folder (flat pre-coloured glyphs) - load them
    // from disk rather than hand-drawing equivalents inline.
    /** Loads a toolbar glyph by base name, preferring the bitmap.

        Figma exports these icons as an SVG whose only content is a <pattern>
        fill referencing an embedded base64 PNG. JUCE's SVG parser doesn't
        implement pattern-with-image fills, so createFromSVG returns a shape
        with no drawable content and the button renders as a solid block -
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
    // A loupe, not a third zoom button. Zoom In and Zoom Out already sit in
    // this cluster as a magnifier with a + and a -, so a plain magnifier read
    // as another one of those - and drawn at twice their stroke weight it
    // looked permanently pressed. Same lens and handle as its neighbours at
    // their weight, with a small frame inside it: this one does not change the
    // zoom, it shows you a patch of the plot enlarged where it is.
    static constexpr const char* kMagnifySVG =
        // The group transform is what holds it to the same ink box as its
        // neighbours. Drawn flat it measured 17x18 px against Zoom In's
        // 16x16, which is small enough to be hard to name and plain enough
        // to see - the glyph sat a size larger than the row it is part of.
        R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle cx="10.2" cy="10.2" r="6.2" fill="none" stroke="#fff" stroke-width="1.3"/><circle cx="10.2" cy="10.2" r="2.1" fill="#fff"/><path stroke="#fff" stroke-width="1.6" stroke-linecap="round" d="M14.8 14.8L20 20"/></svg>)SVG";
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

// Header icons - filled charcoal glyphs matching mockup Stats row.
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
// ViewTileButton - icon + caption tile for view mode switching.
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
// StatusStrip - STATUS | Ready + last run + elapsed.
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
        Full  - the legacy full-width strip (no longer used by the Figma layout).
        Pill  - dot + state only, the ribbon's top-right "Ready" pill.
        RunInfo - "Last run : ..." / "Elapsed : ..." stacked on two lines, the
                  bottom strip's left side. */
    enum class Mode { Full, Pill, RunInfo };
    void setMode (Mode m) { mode_ = m; repaint(); }

    /** Width the Pill needs for its current text. Status messages are far
        longer than "Ready" ("Project saved: ...", "Could not open: ..."), and
        since the full-width status strip is gone this pill is the only place
        they surface - so the ribbon sizes it to fit instead of ellipsizing. */
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
            // red accent (not green/success) - matches the mock exactly.
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

        // Centre: last run - evenly distributed across the middle third.
        g.setColour (Brand::text().withAlpha (0.78f));
        g.drawText (lastRun_,
                    getLocalBounds().withTrimmedLeft (third).withTrimmedRight (third),
                    juce::Justification::centred, true);

        // Right: elapsed - same padding from the right edge as the left.
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

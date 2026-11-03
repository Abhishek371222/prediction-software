#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "UiChrome.h"
#include "TopMenuBar.h"   // for the crosshair, which this strip now carries

// The hairlines in this strip and in the pane headers under it. Brand::border
// is the panel edge colour and reads as a hard black rule across a pale band -
// the reference draws these as a soft division, not a frame.
namespace StripSkin
{
    inline juce::Colour rule()  { return juce::Colour (0xffdcdcdc); }
    // The strip's own edges, as opposed to the rules inside a panel.
    inline juce::Colour bound() { return juce::Colour (0xff000000); }
}

// =============================================================================
// ViewHeaderBar - the strip directly under the top menu, split at the sidebar's
// edge exactly as the Figma draws it:
//
//   | Sources            +   v | Top View  [rays] ................    v |
//
// The left cell is the side panel's first section header; the right cell names
// what the canvas is showing. Both are the same grey band, so the two halves
// read as one rule across the window - which is why the split has to land on
// the sidebar's own edge and nowhere else.
// =============================================================================
// A plain drawn glyph - a chevron or a plus - with no button furniture.
class HeaderGlyphButton : public juce::Button
{
public:
    enum class Glyph { ChevronDown, Plus };

    HeaderGlyphButton (const juce::String& name, Glyph g)
        : juce::Button (name), glyph_ (g) {}

    /** A folded section's chevron points right instead of down. */
    void setFolded (bool f) { if (folded_ != f) { folded_ = f; repaint(); } }
    bool isFolded() const noexcept { return folded_; }

    void paintButton (juce::Graphics& g, bool hl, bool down) override
    {
        const auto r = getLocalBounds().toFloat().reduced (getWidth() * 0.26f);
        const auto ink = Brand::text().withMultipliedAlpha (
                             isEnabled() ? (down ? 0.6f : (hl ? 0.85f : 1.0f)) : 0.4f);
        g.setColour (ink);

        if (glyph_ == Glyph::ChevronDown)
        {
            // Solid, as the Figma draws it - a stroked chevron read as a
            // different mark beside the filled triangles in the panel below.
            juce::Path p;
            const float w = r.getWidth() * 0.86f, hgt = w * 0.56f;
            const float cx = r.getCentreX(), cy = r.getCentreY();
            if (folded_)
                p.addTriangle (cx - hgt * 0.5f, cy - w * 0.5f,
                               cx - hgt * 0.5f, cy + w * 0.5f,
                               cx + hgt * 0.5f, cy);
            else
                p.addTriangle (cx - w * 0.5f, cy - hgt * 0.5f,
                               cx + w * 0.5f, cy - hgt * 0.5f,
                               cx,            cy + hgt * 0.5f);
            g.fillPath (p);
        }
        else
        {
            const float t = juce::jmax (1.2f, r.getWidth() * 0.14f);
            g.fillRect (r.getCentreX() - t * 0.5f, r.getY(), t, r.getHeight());
            g.fillRect (r.getX(), r.getCentreY() - t * 0.5f, r.getWidth(), t);
        }
    }

private:
    Glyph glyph_;
    bool  folded_ = false;
};

// A small outlined text button for a pane header's own tools; a toggle fills
// red while it is on.
class HeaderPillButton : public juce::Button
{
public:
    HeaderPillButton (const juce::String& text, bool toggle) : juce::Button (text)
    {
        setButtonText (text);
        setClickingTogglesState (toggle);
    }

    void paintButton (juce::Graphics& g, bool hl, bool down) override
    {
        const auto r = getLocalBounds().toFloat().reduced (1.0f);
        const bool on = getClickingTogglesState() && getToggleState();
        if (on)
        {
            g.setColour (Brand::accent());
            g.fillRoundedRectangle (r, 3.0f);
        }
        else if (hl || down)
        {
            g.setColour (Brand::text().withAlpha (down ? 0.14f : 0.07f));
            g.fillRoundedRectangle (r, 3.0f);
        }
        g.setColour (on ? Brand::accent() : Brand::text().withAlpha (0.55f));
        g.drawRoundedRectangle (r, 3.0f, 1.0f);
        g.setColour (on ? Brand::onAccent() : Brand::text());
        g.setFont (Brand::figMed (12.0f));
        g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);
    }
};

// The header over a stacked pane - "Elevation view", "Distance Vs. dbSPL".
// Same grey band as the strip above, with a fold chevron on the right, so the
// three panes down the middle read as one stack rather than three widgets.
class PaneHeaderBar : public juce::Component
{
public:
    explicit PaneHeaderBar (const juce::String& title) : title_ (title)
    {
        addAndMakeVisible (fold_);
        fold_.onClick = [this]
        {
            folded_ = ! folded_;
            fold_.setFolded (folded_);
            if (onFold) onFold (folded_);
            repaint();
        };
    }

    std::function<void (bool folded)> onFold;
    bool isFolded() const noexcept { return folded_; }

    /** Buttons laid out right to left, left of the fold chevron. */
    void setTools (std::initializer_list<juce::Component*> tools)
    {
        tools_.assign (tools.begin(), tools.end());
        for (auto* t : tools_) addAndMakeVisible (t);
        resized();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (juce::Colour (0xffdfdede));
        g.setColour (Brand::text());
        // Medium, not Bold: the reference sets these headers lighter than
        // the values under them, and larger. Weight carries the hierarchy
        // here, so making them heavier AND bigger shouted twice.
        // Figma V1.1: Medium 16, 24 px in; the band is outlined in black.
        g.setFont (Brand::figMed (16.0f));
        g.drawText (title_, getLocalBounds().withTrimmedLeft (UiConfig::Scale::fig (24.0f)),
                    juce::Justification::centredLeft, true);
        g.setColour (StripSkin::bound());
        g.drawHorizontalLine (0, 0.0f, (float) getWidth());
        g.drawHorizontalLine (getHeight() - 1, 0.0f, (float) getWidth());
    }

    void resized() override
    {
        const int b = UiConfig::Scale::fig (35.0f);
        fold_.setBounds (getWidth() - UiConfig::Scale::fig (21.0f) - b / 2, (getHeight() - b) / 2, b, b);
        const int th = juce::jmin (getHeight() - 6, UiConfig::Scale::fig (22.0f));
        const int gap = UiConfig::Scale::fig (6.0f);
        int right = fold_.getX() - gap;
        for (auto it = tools_.rbegin(); it != tools_.rend(); ++it)
        {
            auto* t = *it;
            const int w = UiConfig::Scale::fig (t->getName().length() > 2 ? 46.0f : 28.0f);
            t->setBounds (right - w, (getHeight() - th) / 2, w, th);
            right -= w + gap;
        }
    }

private:
    juce::String      title_;
    std::vector<juce::Component*> tools_;
    bool              folded_ = false;
    HeaderGlyphButton fold_ { "Fold", HeaderGlyphButton::Glyph::ChevronDown };
};

class ViewHeaderBar : public juce::Component
{
public:
    ViewHeaderBar()
    {
        btnAddSource_.setTooltip ("Add a source");
        btnSourcesFold_.setTooltip ("Fold the sources list");
        btnViewOptions_.setTooltip ("View options");
        for (auto* b : { &btnAddSource_, &btnSourcesFold_, &btnViewOptions_ })
            addAndMakeVisible (*b);

        addAndMakeVisible (btnRays_);
        btnMic_.setClickingTogglesState (false);
        addChildComponent (btnMic_);
        btnLocate_.setMouseCursor (juce::MouseCursor::PointingHandCursor);
        btnLocate_.setTooltip ("Back to the action - frame the rig, mics and venue");
        addAndMakeVisible (btnLocate_);

        lblSources_.setText ("Sources", juce::dontSendNotification);
        lblView_.setText ("Top View", juce::dontSendNotification);
        for (auto* l : { &lblSources_, &lblView_ })
        {
            l->setJustificationType (juce::Justification::centredLeft);
            l->setInterceptsMouseClicks (false, false);
            l->setColour (juce::Label::textColourId, Brand::text());
            addAndMakeVisible (*l);
        }
    }

    void setSidebarWidth (int w) { sideW_ = juce::jmax (0, w); resized(); repaint(); }
    /** The left cell names whatever the panel below it lists - "Sources" on
        the Sources tab, "Elements" on Venue. One strip, two tabs. */
    void setSourcesName (const juce::String& n)
    {
        lblSources_.setText (n, juce::dontSendNotification);
        resized(); repaint();
    }
    /** The aiming-ray toggle. It belongs to the Sources tab - a venue
        drawing has no cabinets to aim - so the Venue tab turns it off
        rather than showing a control that does nothing. Hidden, not
        removed: the button and its wiring are untouched. */
    void setToolsVisible (bool on)
    {
        btnRays_.setVisible (on);
        resized(); repaint();
    }

    /** The Mapping tab's mic glyph, after the rays. */
    void setMicVisible (bool on)
    {
        btnMic_.setVisible (on);
        resized(); repaint();
    }

    void setViewName (const juce::String& n)
    {
        lblView_.setText (n, juce::dontSendNotification);
        repaint();
    }

    // Figma V1.1: 30 px.
    int preferredHeight() const { return juce::jmax (18, UiConfig::Scale::fig (30.0f)); }

    /** The left edge of the fold chevron at the far right. Anything else put
        in this strip measures its own right edge from here, so the gap to the
        chevron stays the same at every window size. */
    int optionsLeft() const noexcept { return btnViewOptions_.getX(); }

    /** The right edge of the view title and its ray glyph - where the strip's
        left-hand furniture stops and free space begins. */
    int toolsRight() const noexcept
    {
        // The crosshair is always last and always present, so it is the end
        // of the strip's left-hand furniture whatever else is showing.
        return btnLocate_.getRight();
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds();
        // ONE band, one grey. The two halves used to be different shades -
        // the sidebar's a touch darker than the canvas's - which made the
        // strip read as two bars that happened to meet. It is the same grey
        // the panel's section headers use, because that is what the left half
        // IS: the Sources header.
        g.setColour (juce::Colour (0xffdfdede));
        g.fillRect (r);

        // This strip is BOUNDED in black: a line along its top, one along its
        // bottom, and the divider between the two halves. Grey was right for
        // the rules inside a panel, but these three are the strip's own edges
        // - they say where the band starts and stops, which is the one thing
        // a soft rule cannot do against a soft fill.
        // No top line here: the menu bar above closes with its own rule, and
        // two adjacent hairlines stack into one 2 px band - which is exactly
        // why this edge read darker than the one below it.
        g.setColour (StripSkin::bound());
        g.drawHorizontalLine (r.getBottom() - 1, 0.0f, (float) r.getRight());
        if (sideW_ > 0)
            g.drawVerticalLine (sideW_ - 1, 0.0f, (float) r.getHeight());
    }

    void resized() override
    {
        using UiConfig::Scale::fig;
        const int h = getHeight();

        const juce::Font f = Brand::figMed (16.0f);
        lblSources_.setFont (f);
        lblView_.setFont (f);
        lblSources_.setBorderSize ({});
        lblView_.setBorderSize ({});

        // Glyph buttons centred on the Figma icon's centre. Box sizes are set
        // so the drawn mark matches the icon's ink: the triangle is ~14 px
        // inside the 24 px "Sort Down" icon, the plus ~15 px.
        auto glyphAt = [&] (juce::Component& b, float figCentreX, float figBox)
        {
            const int box = fig (figBox);
            const int cx  = juce::roundToInt (UiConfig::Scale::figf (figCentreX));
            b.setBounds (cx - box / 2, (h - box) / 2, box, box);
        };

        // --- left cell (Figma V1.1 x 0..400): the title at 14, the plus (20 px
        //     icon) centred on 332, the fold arrow (24 px) on 374.
        lblSources_.setBounds (fig (14.0f), 0, fig (290.0f), h - 1);
        glyphAt (btnAddSource_,   332.0f, 31.0f);
        glyphAt (btnSourcesFold_, 374.0f, 35.0f);

        // --- right cell (x 400..): the title 24 in, the ray icon (24 px)
        //     after it, the fold arrow centred 21 from the end.
        const int rx = sideW_;
        const int titleW = f.getStringWidth (lblView_.getText()) + fig (4.0f);
        lblView_.setBounds (rx + fig (24.0f), 0, titleW, h - 1);

        {
            const int icon = fig (24.0f);
            int right = lblView_.getRight() - fig (8.0f);   // as if a 24 px icon ended here
            if (btnRays_.isVisible())
            {
                btnRays_.setBounds (right + fig (20.0f), (h - icon) / 2, icon, icon);
                right = btnRays_.getRight();
            }
            // MAPPING V1.1: the mic 20 px after the rays.
            if (btnMic_.isVisible())
            {
                btnMic_.setBounds (right + fig (20.0f), (h - icon) / 2, icon, icon);
                right = btnMic_.getRight();
            }
            // The crosshair closes the group, the same 20 px after whatever
            // came last. It acts on what the canvas is showing, so it belongs
            // with the rays and the mic rather than up among the tabs - and
            // unlike those two it is laid out on EVERY tab, because framing
            // the view is something the Venue sheet wants as much as the plan.
            btnLocate_.setBounds (right + fig (20.0f), (h - icon) / 2, icon, icon);
        }

        const int fold = fig (35.0f);
        btnViewOptions_.setBounds (getWidth() - fig (21.0f) - fold / 2, (h - fold) / 2, fold, fold);
    }

    HeaderGlyphButton btnAddSource_   { "Add",   HeaderGlyphButton::Glyph::Plus };
    HeaderGlyphButton btnSourcesFold_ { "Fold",  HeaderGlyphButton::Glyph::ChevronDown };
    HeaderGlyphButton btnViewOptions_ { "View",  HeaderGlyphButton::Glyph::ChevronDown };
    RibbonIconButton btnRays_ { "Show Rays", { { "Show Rays.png", "Show Rays Active.png",
                                                 0, 0, 24, 24 } } };
    RibbonIconButton btnMic_  { "Mic", { { "Mic.png", "Mic Active.png", 2, 2, 20, 20 } } };
    TopMenuBar::Locate btnLocate_;

private:
    juce::Label lblSources_, lblView_;
    int         sideW_ = 240;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewHeaderBar)
};

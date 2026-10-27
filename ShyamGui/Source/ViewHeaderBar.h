#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "UiChrome.h"

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
        fold_.onClick = [this] { folded_ = ! folded_; if (onFold) onFold (folded_); repaint(); };
    }

    std::function<void (bool folded)> onFold;
    bool isFolded() const noexcept { return folded_; }

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
    }

private:
    juce::String      title_;
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
        addAndMakeVisible (btnLockRays_);

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
    /** The aiming-ray tools. They belong to the Sources tab - a venue
        drawing has no cabinets to aim - so the Venue tab turns them off
        rather than showing two controls that do nothing. Hidden, not
        removed: the buttons and their wiring are untouched. */
    void setToolsVisible (bool on)
    {
        btnRays_.setVisible (on);
        btnLockRays_.setVisible (on);
        resized(); repaint();
    }

    void setViewName (const juce::String& n)
    {
        lblView_.setText (n, juce::dontSendNotification);
        repaint();
    }

    // Figma V1.1: 30 px.
    int preferredHeight() const { return juce::jmax (18, UiConfig::Scale::fig (30.0f)); }

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

        // --- right cell (x 400..): the title 24 in, the two ray icons (24 px)
        //     16 px after it, the fold arrow centred 21 from the end.
        const int rx = sideW_;
        const int titleW = f.getStringWidth (lblView_.getText()) + fig (4.0f);
        lblView_.setBounds (rx + fig (24.0f), 0, titleW, h - 1);

        if (btnRays_.isVisible())
        {
            const int icon = fig (24.0f);
            const int ix   = lblView_.getRight() + fig (12.0f);
            btnRays_.setBounds     (ix,              (h - icon) / 2, icon, icon);
            btnLockRays_.setBounds (ix + fig (36.0f), (h - icon) / 2, icon, icon);
        }

        const int fold = fig (35.0f);
        btnViewOptions_.setBounds (getWidth() - fig (21.0f) - fold / 2, (h - fold) / 2, fold, fold);
    }

    HeaderGlyphButton btnAddSource_   { "Add",   HeaderGlyphButton::Glyph::Plus };
    HeaderGlyphButton btnSourcesFold_ { "Fold",  HeaderGlyphButton::Glyph::ChevronDown };
    HeaderGlyphButton btnViewOptions_ { "View",  HeaderGlyphButton::Glyph::ChevronDown };
    RibbonIconButton btnRays_     { "Show Rays", { { "Show Rays.png", "Show Rays Active.png",
                                                     0, 0, 24, 24 } } };
    RibbonIconButton btnLockRays_ { "Lock Rays", { { "Show Rays.png", "Show Rays Active.png",
                                                     -1.5f, 2.5f, 24, 24 },
                                                   { "Lock Badge.png", nullptr, 13.5f, -1.1f, 12, 12 } } };

private:
    juce::Label lblSources_, lblView_;
    int         sideW_ = 240;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViewHeaderBar)
};

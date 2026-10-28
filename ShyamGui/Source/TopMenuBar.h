#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"

// =============================================================================
// TopMenuBar - the application's top strip, as the Figma draws it
// (Source screen - 1.3, "Group 964"):
//
//   [ ATOMIK* ]   File  Venue  Sources  Mapping  Export   ...   (gear) | INTEGRAL.ATOMIK
//                              -------
//
// It replaces the two-row title band + icon ribbon with one strip. The ribbon
// is not deleted - it is hidden, and everything it carried is reachable from
// one of the five menus, so no action is lost in the move.
//
// The menus start on the sidebar's edge, so the first cell lines up with the
// plot below it. The strip is white with black ink in both themes.
// =============================================================================
class TopMenuBar : public juce::Component
{
public:
    enum MenuId { File = 1, Venue, Sources, Mapping, Export, Settings };

    TopMenuBar()
    {
        auto wire = [this] (juce::Button& b, MenuId id)
        {
            b.setTriggeredOnMouseDown (true);
            b.setMouseCursor (juce::MouseCursor::PointingHandCursor);
            b.onClick = [this, id, &b] { if (onMenu) onMenu (id, &b); };
            addAndMakeVisible (b);
        };
        wire (btnFile_,     File);
        wire (btnVenue_,    Venue);
        wire (btnSources_,  Sources);
        wire (btnMapping_,  Mapping);
        wire (btnExport_,   Export);
        wire (btnSettings_, Settings);
        btnSettings_.setTooltip ("Settings");
        btnSources_.setActive (true);

        projectLabel_.setJustificationType (juce::Justification::centred);
        projectLabel_.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (projectLabel_);
    }

    /** Which TAB is showing. File / Mapping / Export drop a menu and are
        gone again, so they never take this - only the two that change what
        the window is displaying. */
    void setActiveTab (MenuId id)
    {
        if (activeTab_ == id) return;
        activeTab_ = id;
        for (auto* t : tabs())
            t->setActive (t == buttonFor (id));
    }

    /** Called with the menu that was clicked and the button it hangs under. */
    std::function<void (MenuId, juce::Component*)> onMenu;

    void setLogo (juce::Drawable* d)                 { logo_ = d; repaint(); }
    void setSidebarWidth (int w)                     { sideW_ = juce::jmax (80, w); resized(); repaint(); }
    void setProjectName (const juce::String& name)
    {
        // The Figma shows it shouted, which also keeps a long file name from
        // reading as a sentence in the corner of the window.
        projectLabel_.setText (name.trim().isEmpty() ? "UNTITLED.ATOMIK"
                                                     : name.trim().toUpperCase(),
                               juce::dontSendNotification);
        repaint();
    }

    // Figma: 58 px.
    int preferredHeight() const { return juce::jmax (26, UiConfig::Scale::fig (58.0f)); }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds();
        g.fillAll (kWhite);

        if (logo_ != nullptr)
        {
            // Figma: the wordmark is 94 x 18 at (20, 20).
            using UiConfig::Scale::figf;
            const juce::Rectangle<float> lb (figf (20.0f), figf (20.0f), figf (94.0f), figf (18.0f));
            logo_->drawWithin (g, lb, juce::RectanglePlacement::centred, 1.0f);
        }

        // Figma: a 1 px black edge round the strip, and a 1 px black rule
        // closing off the project name's cell.
        g.setColour (kInk);
        g.drawHorizontalLine (0, 0.0f, (float) r.getRight());
        g.drawHorizontalLine (r.getBottom() - 1, 0.0f, (float) r.getRight());
        g.drawVerticalLine (projectBox_.getX(), 0.0f, (float) r.getBottom());
    }

    void resized() override
    {
        const int h = getHeight();
        using UiConfig::Scale::fig;

        // Figma: five 120 px cells from the sidebar's edge (x 399), each word
        // centred in its cell, full strip height.
        const int cellW = fig (120.0f);
        int x = sideW_ - 1;
        for (auto* t : tabs())
        {
            t->setBounds (x, 1, cellW, h - 2);
            x += cellW;
        }

        // Figma: the project name's cell is the last 222 px; the 26 px gear
        // sits 8 px before it, 16 px down. The gear's button is the full
        // strip height and centred on the glyph, so it is an easy target.
        const int projW = fig (222.0f);
        projectBox_ = { getWidth() - projW, 0, projW, h };
        const int gearCx = projectBox_.getX() - fig (8.0f) - fig (13.0f);
        const int hitW   = fig (42.0f);
        btnSettings_.setBounds (gearCx - hitW / 2, 1, hitW, h - 2);

        projectLabel_.setFont (Brand::figMed (16.0f));
        projectLabel_.setBorderSize ({});
        projectLabel_.setColour (juce::Label::textColourId, kInk);
        projectLabel_.setBounds (projectBox_.withTrimmedLeft (1).reduced (fig (8.0f), 1));
    }

private:
    // ---- A menu word: Montserrat 16, black, centred; the open tab carries
    //      Figma's red underline (2 px, #ED2227, round ends, 4 px above the
    //      strip's bottom edge). Hover shows the same line in grey.
    class Tab : public juce::Button
    {
    public:
        Tab (const juce::String& text, bool semiBold)
            : juce::Button (text), semiBold_ (semiBold) {}

        void setActive (bool on) { if (active_ != on) { active_ = on; repaint(); } }

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            using UiConfig::Scale::figf;
            const auto b = getLocalBounds().toFloat();
            g.setFont (semiBold_ ? Brand::figSemi (16.0f) : Brand::figMed (16.0f));
            g.setColour (down ? kInk.withAlpha (0.6f) : kInk);
            g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);

            if (active_ || over)
            {
                const float th = juce::jmax (1.5f, figf (2.0f));
                // The strip's bottom edge is 1 px below this button.
                const float y  = b.getBottom() + 1.0f - figf (5.0f);
                juce::Path p;
                p.startNewSubPath (b.getX() + figf (1.0f), y);
                p.lineTo (b.getRight() - figf (1.0f), y);
                g.setColour (active_ ? kAccent : kHover);
                g.strokePath (p, juce::PathStrokeType (th, juce::PathStrokeType::curved,
                                                       juce::PathStrokeType::rounded));
            }
        }

    private:
        bool semiBold_ = false;
        bool active_   = false;
    };

    // ---- The Settings gear: Figma's 26 px glyph, centred in its button.
    class Gear : public juce::Button
    {
    public:
        Gear() : juce::Button ("Settings"), img_ (Brand::assetImage ("ToolIcons/Settings.png")) {}

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            const float s = UiConfig::Scale::figf (26.0f);
            const auto box = juce::Rectangle<float> (s, s).withCentre (getLocalBounds().toFloat().getCentre());
            if (over || down)
            {
                g.setColour (kHover.withAlpha (down ? 0.9f : 0.6f));
                g.fillEllipse (box.expanded (UiConfig::Scale::figf (5.0f)));
            }
            if (img_.isValid())
            {
                g.setOpacity (1.0f);
                g.drawImage (img_, box, juce::RectanglePlacement::centred);
            }
        }

    private:
        juce::Image img_;
    };

    inline static const juce::Colour kWhite  { 0xffffffff };
    inline static const juce::Colour kInk    { 0xff000000 };
    inline static const juce::Colour kAccent { 0xffed2227 };
    inline static const juce::Colour kHover  { 0xffd5d3d3 };

public:
    Tab  btnFile_     { "File", true }, btnVenue_ { "Venue", false }, btnSources_ { "Sources", false },
         btnMapping_  { "Mapping", false }, btnExport_ { "Export", false };
    Gear btnSettings_;

private:
    std::array<Tab*, 5> tabs() { return { &btnFile_, &btnVenue_, &btnSources_, &btnMapping_, &btnExport_ }; }

    Tab* buttonFor (MenuId id)
    {
        switch (id)
        {
            case File:     return &btnFile_;
            case Venue:    return &btnVenue_;
            case Sources:  return &btnSources_;
            case Mapping:  return &btnMapping_;
            case Export:   return &btnExport_;
            case Settings: break;
        }
        return nullptr;
    }

    MenuId activeTab_ = Sources;

    juce::Drawable*      logo_ = nullptr;
    juce::Label          projectLabel_;
    juce::Rectangle<int> projectBox_;
    int                  sideW_ = 240;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TopMenuBar)
};

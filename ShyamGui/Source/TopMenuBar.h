#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"

// =============================================================================
// TopMenuBar - the application's top strip, as the Figma draws it:
//
//   [ ATOMIK* ] | File | Venue | Sources | Mapping | Export | ... | Settings |
//                                                           | INTEGREL.ATOMIK |
//
// It replaces the two-row title band + icon ribbon with one strip. The ribbon
// is not deleted - it is hidden, and everything it carried is reachable from
// one of the five menus, so no action is lost in the move.
//
// The wordmark cell is exactly the sidebar's width, so the rule under it lines
// up with the sidebar's own edge and the menus start where the plot starts.
// =============================================================================
class TopMenuBar : public juce::Component
{
public:
    enum MenuId { File = 1, Venue, Sources, Mapping, Export, Settings };

    TopMenuBar()
    {
        auto cell = [this] (juce::TextButton& b, const juce::String& text, MenuId id)
        {
            b.setButtonText (text);
            b.setComponentID ("topMenu");
            b.setConnectedEdges (juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight
                               | juce::Button::ConnectedOnTop  | juce::Button::ConnectedOnBottom);
            b.setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
            b.setColour (juce::TextButton::buttonOnColourId, Brand::btnIn());
            b.setColour (juce::TextButton::textColourOffId,  Brand::text());
            b.setColour (juce::TextButton::textColourOnId,   Brand::text());
            b.setTriggeredOnMouseDown (true);
            b.onClick = [this, id, &b] { if (onMenu) onMenu (id, &b); };
            addAndMakeVisible (b);
        };

        cell (btnFile_,     "File",     File);
        cell (btnVenue_,    "Venue",    Venue);
        cell (btnSources_,  "Sources",  Sources);
        cell (btnMapping_,  "Mapping",  Mapping);
        cell (btnExport_,   "Export",   Export);
        cell (btnSettings_, "Settings", Settings);

        projectLabel_.setJustificationType (juce::Justification::centred);
        projectLabel_.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (projectLabel_);
    }

    /** Which TAB is showing. File / Mapping / Export drop a menu and are
        gone again, so they never take this - only the two that change what
        the window is displaying. */
    void setActiveTab (MenuId id) { if (activeTab_ != id) { activeTab_ = id; repaint(); } }

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

        // Figma V1.1: every menu cell is outlined #D5D3D3; the open tab is
        // filled #DFDFDF and outlined #2C2929; Settings is outlined #3D3636.
        // The cells are painted UNDER the transparent buttons.
        const auto* active = buttonFor (activeTab_);
        for (auto* b : { &btnFile_, &btnVenue_, &btnSources_, &btnMapping_, &btnExport_ })
        {
            const bool on = b == active;
            if (on)
            {
                g.setColour (kTabActive);
                g.fillRect (b->getBounds());
            }
            g.setColour (on ? kTabEdge : kRule);
            g.drawRect (b->getBounds(), 1);
        }
        g.setColour (kSettingsEdge);
        g.drawRect (btnSettings_.getBounds(), 1);

        g.setColour (kBound);
        g.drawHorizontalLine (r.getBottom() - 1, 0.0f, (float) r.getRight());
    }

    void resized() override
    {
        const int h = getHeight();

        using UiConfig::Scale::fig;
        projectLabel_.setFont (Brand::topMenu());
        projectLabel_.setBorderSize ({});
        projectLabel_.setColour (juce::Label::textColourId, juce::Colours::black);

        // Figma V1.1 cells, from the panel edge (x 400): File at -1 (80 wide),
        // Venue 78 (92), Sources 169 (89), Mapping 257 (87), Export 343 (81).
        // Neighbours share their 1 px edge. The words are centred, as drawn.
        const struct { float x, w; } cells[] = {
            { -1.0f, 80.0f }, { 78.0f, 92.0f }, { 169.0f, 89.0f }, { 257.0f, 87.0f }, { 343.0f, 81.0f } };
        int i = 0;
        for (auto* b : { &btnFile_, &btnVenue_, &btnSources_, &btnMapping_, &btnExport_ })
        {
            b->setLookAndFeel (nullptr);
            const auto& c = cells[i++];
            const int cx = c.x < 0.0f ? sideW_ - 1 : sideW_ + fig (c.x);
            b->setBounds (cx, 0, fig (c.w), h - 1);
        }

        // Settings (80, ending at 1699) and the project name (221) close the
        // right-hand end. A long file name elides inside its cell; the window
        // title has it in full.
        const int projW = fig (221.0f);
        projectBox_ = { getWidth() - projW, 0, projW, h - 1 };
        btnSettings_.setBounds (projectBox_.getX() - fig (80.0f), 0, fig (80.0f), h - 1);
        projectLabel_.setBounds (projectBox_);
    }

    juce::TextButton btnFile_, btnVenue_, btnSources_, btnMapping_, btnExport_, btnSettings_;

private:
    // Figma V1.1 strip colours.
    const juce::Colour kWhite        { juce::Colour (0xffffffff) };
    const juce::Colour kTabActive    { juce::Colour (0xffdfdfdf) };
    const juce::Colour kTabEdge      { juce::Colour (0xff2c2929) };
    const juce::Colour kRule         { juce::Colour (0xffd5d3d3) };
    const juce::Colour kSettingsEdge { juce::Colour (0xff3d3636) };
    const juce::Colour kBound        { juce::Colour (0xff000000) };

    const juce::TextButton* buttonFor (MenuId id) const
    {
        switch (id)
        {
            case Venue:    return &btnVenue_;
            case Sources:  return &btnSources_;
            case File:     return &btnFile_;
            case Mapping:  return &btnMapping_;
            case Export:   return &btnExport_;
            case Settings: return &btnSettings_;
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

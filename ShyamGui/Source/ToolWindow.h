#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"

// ---------------------------------------------------------------------------
// ToolWindow - the small floating windows (Frequency Response, Listen,
// Terminal). Native title bar with minimise / maximise / close, plus a thin
// strip above the content carrying two controls a title bar cannot:
//
//   Keep on top  - pins the window over the main one, so clicking back into
//                  the plot does not bury it; it stays until minimised.
//   Full screen  - fills the monitor it is on, and back again.
// ---------------------------------------------------------------------------
class ToolWindow : public juce::DocumentWindow
{
public:
    explicit ToolWindow (const juce::String& title)
        : DocumentWindow (title, Brand::panelDark(), DocumentWindow::allButtons),
          frame_ (*this)
    {
        setUsingNativeTitleBar (true);
    }

    ~ToolWindow() override
    {
        clearContentComponent();
    }

    /** Call from the derived constructor once its content exists. */
    void setToolContent (juce::Component& content, bool resizeToFitContent)
    {
        frame_.setContent (&content);
        setContentNonOwned (&frame_, resizeToFitContent);
    }

    void setKeepOnTop (bool on)
    {
        setAlwaysOnTop (on);
        frame_.syncButtons();
    }

    void resized() override
    {
        DocumentWindow::resized();
        frame_.syncButtons();   // the native maximise button changes this too
    }

    void lookAndFeelChanged() override
    {
        DocumentWindow::lookAndFeelChanged();
        setBackgroundColour (Brand::panelDark());
        frame_.restyle();
    }

private:
    class Frame : public juce::Component
    {
    public:
        explicit Frame (ToolWindow& w) : window_ (w)
        {
            pinBtn_.setClickingTogglesState (true);
            pinBtn_.setTooltip ("Keep this window over the main window until you "
                                "minimise it or switch this off.");
            pinBtn_.onClick = [this] { window_.setKeepOnTop (pinBtn_.getToggleState()); };

            // Full screen is withheld: the window already carries the system's
            // own maximise button, and two controls for one job is one too
            // many. The button and its handler stay here, unparented, so it
            // can be put back by adding it below.
            fullBtn_.setTooltip ("Fill the screen with this window, or restore it.");
            fullBtn_.onClick = [this]
            {
                window_.setFullScreen (! window_.isFullScreen());
                syncButtons();
            };

            // Sized like the bottom bar's actions, which is the nearest thing
            // to it in the app - without an id these fell through to the
            // generic cap and rendered about 8 px on a laptop.
            pinBtn_.setComponentID ("bottomBtn");
            addAndMakeVisible (pinBtn_);
            restyle();
        }

        void setContent (juce::Component* c)
        {
            content_ = c;
            if (content_ != nullptr)
                addAndMakeVisible (*content_);
            resized();
        }

        void syncButtons()
        {
            pinBtn_.setToggleState (window_.isAlwaysOnTop(), juce::dontSendNotification);
        }

        void restyle()
        {
            for (auto* b : { &pinBtn_ })
            {
                b->setColour (juce::TextButton::buttonColourId,   Brand::plotToolbar());
                b->setColour (juce::TextButton::buttonOnColourId, Brand::accent());
                b->setColour (juce::TextButton::textColourOffId,  Brand::text());
                b->setColour (juce::TextButton::textColourOnId,   Brand::onAccent());
            }
            repaint();
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (Brand::panelDark());
            g.setColour (Brand::plotBorder().withAlpha (0.35f));
            g.fillRect (0, stripH() - 1, getWidth(), 1);
        }

        void resized() override
        {
            const int h = stripH();
            auto strip = getLocalBounds().removeFromTop (h).reduced (4, 3);
            // Floor in real pixels, like the ribbon's and the bottom bar's:
            // the global factor bottoms out at 0.78, which left this strip
            // 22 px tall carrying 8 px of text.
            const int w = juce::jmax (130, juce::roundToInt (Brand::UI::scaledFont (1.0f) * 120.0f));
            pinBtn_.setBounds (strip.removeFromRight (juce::jmin (w, strip.getWidth())));

            if (content_ != nullptr)
                content_->setBounds (getLocalBounds().withTrimmedTop (h));
        }

    private:
        int stripH() const
        {
            return juce::jmax (36, juce::roundToInt (Brand::UI::scaledFont (1.0f) * 34.0f));
        }

        ToolWindow&       window_;
        juce::Component*  content_ = nullptr;
        juce::TextButton  pinBtn_  { "Keep on top" };
        juce::TextButton  fullBtn_ { "Full screen" };
    };

    Frame frame_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ToolWindow)
};

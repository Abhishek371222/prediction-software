#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "MicListener.h"

// ---------------------------------------------------------------------------
// The window behind "Listen here". Small on purpose: a transport, an A/B, and
// the three caveats that stop this being read as an auralisation of a room.
// ---------------------------------------------------------------------------
class MicListenPanel final : public juce::Component
{
public:
    MicListenPanel()
    {
        auto styleBtn = [this] (juce::TextButton& b, const juce::String& t)
        {
            b.setButtonText (t);
            b.setColour (juce::TextButton::buttonColourId, Brand::panelDark());
            b.setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.35f));
            b.setColour (juce::TextButton::textColourOffId, Brand::text());
            b.setColour (juce::TextButton::textColourOnId,  Brand::text());
            addAndMakeVisible (b);
        };
        styleBtn (playBtn_, "Play");
        playBtn_.setClickingTogglesState (true);
        playBtn_.onClick = [this]
        {
            if (playBtn_.getToggleState()) listener_.start();
            else                           listener_.stop();
            playBtn_.setButtonText (playBtn_.getToggleState() ? "Stop" : "Play");
            refresh();
        };

        styleBtn (abBtn_, "Compare: flat");
        abBtn_.setClickingTogglesState (true);
        abBtn_.setTooltip ("Hold the comparison on to hear the same noise without "
                           "the prediction applied.");
        abBtn_.onClick = [this] { listener_.setBypass (abBtn_.getToggleState()); };

        auto styleLbl = [this] (juce::Label& l, float sz, juce::Colour c)
        {
            l.setFont (Brand::tech (Brand::UI::scaledFont (sz)));
            l.setColour (juce::Label::textColourId, c);
            l.setJustificationType (juce::Justification::topLeft);
            l.setBorderSize ({});
            addAndMakeVisible (l);
        };
        styleLbl (titleLbl_,  15.0f, Brand::text());
        styleLbl (levelLbl_,  14.0f, Brand::accent());
        styleLbl (caveatLbl_, 12.0f, Brand::muted());
        caveatLbl_.setJustificationType (juce::Justification::topLeft);
    }

    ~MicListenPanel() override { listener_.stop(); }

    /** @param curve   predicted level per catalogue frequency at this mic
        @param refDb   level that plays at full scale (the map's peak)
        @param absolute true when the figures are real dB SPL
        @param micName what to call this mic in the title */
    void setMic (const juce::String& micName,
                 std::vector<MicListener::Point> curve,
                 float refDb, bool absolute)
    {
        absolute_ = absolute;
        double lo = 0.0, hi = 0.0;
        if (! curve.empty())
        {
            lo = curve.front().hz;
            hi = curve.back().hz;
        }
        listener_.setResponse (std::move (curve), refDb);

        titleLbl_.setText ("Listening at " + micName, juce::dontSendNotification);
        caveatLbl_.setText (
            juce::String ("Rendered band ")
                + juce::String ((int) (lo + 0.5)) + " - " + juce::String ((int) (hi + 0.5))
                + " Hz, the measured catalogue for this device.\n"
                  "Direct sound only: no room, no reflections, no air absorption.\n"
                + (absolute_ ? "Levels are calibrated dB SPL, played relative to the "
                               "loudest point on the map."
                             : "Levels are RELATIVE - this device has no sensitivity "
                               "figure, so absolute loudness is unknown."),
            juce::dontSendNotification);
        refresh();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());
        g.setColour (Brand::border().withAlpha (0.4f));
        g.drawHorizontalLine (titleLbl_.getBottom() + 6, 0.0f, (float) getWidth());
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (UiConfig::Scale::px (14));
        const int rowH = UiConfig::Scale::px (26);
        titleLbl_.setBounds (r.removeFromTop (UiConfig::Scale::px (20)));
        r.removeFromTop (UiConfig::Scale::px (12));
        levelLbl_.setBounds (r.removeFromTop (UiConfig::Scale::px (20)));
        r.removeFromTop (UiConfig::Scale::px (8));
        auto row = r.removeFromTop (rowH);
        const int halfW = (row.getWidth() - UiConfig::Scale::px (8)) / 2;
        playBtn_.setBounds (row.removeFromLeft (halfW));
        row.removeFromLeft (UiConfig::Scale::px (8));
        abBtn_.setBounds (row);
        r.removeFromTop (UiConfig::Scale::px (12));
        caveatLbl_.setBounds (r);
    }

private:
    void refresh()
    {
        const auto err = listener_.getLastError();
        if (err.isNotEmpty())
        {
            levelLbl_.setText ("No audio device: " + err, juce::dontSendNotification);
            return;
        }
        levelLbl_.setText (juce::String (listener_.levelOffsetDb(), 1)
                               + " dB relative to the loudest point on the map",
                           juce::dontSendNotification);
    }

    MicListener      listener_;
    juce::TextButton playBtn_, abBtn_;
    juce::Label      titleLbl_, levelLbl_, caveatLbl_;
    bool             absolute_ = false;
};

class MicListenWindow : public juce::DocumentWindow
{
public:
    MicListenWindow()
        : DocumentWindow ("Listen", Brand::panelDark(), DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        setContentNonOwned (&content, true);
        setResizeLimits (340, 230, 720, 420);
        centreWithSize (420, 260);
    }

    // Closing must silence it - a hidden window still holding the audio device
    // would keep playing with nothing on screen to stop it.
    void closeButtonPressed() override { setVisible (false); }

    MicListenPanel content;
};

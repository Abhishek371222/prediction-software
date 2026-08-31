#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "MicReceiver.h"
#include "MicRingSnap.h"
#include "AcousticEngine.h"

// ---------------------------------------------------------------------------
// Dialog: place / lock a mic onto a 1 / 2 / 4 / 8 m ring around a speaker.
// ---------------------------------------------------------------------------
class MicRefLockDialog : public juce::Component
{
public:
    std::function<void (int micIndex, int speakerIndex, float radiusM)> onApply;

    MicRefLockDialog (const std::vector<MicReceiver>& mics,
                      const std::vector<Speaker>& speakers,
                      int defaultMic)
        : mics_ (mics), speakers_ (speakers)
    {
        title_.setText ("Place on ring", juce::dontSendNotification);
        title_.setFont (Brand::techSemi (Brand::UI::scaledFont (14.0f)));
        title_.setColour (juce::Label::textColourId, Brand::text());
        addAndMakeVisible (title_);

        micLabel_.setText ("Mic", juce::dontSendNotification);
        micLabel_.setColour (juce::Label::textColourId, Brand::muted());
        addAndMakeVisible (micLabel_);
        for (int i = 0; i < (int) mics_.size(); ++i)
            micBox_.addItem (micDisplayName (mics_[(size_t) i]), i + 1);
        micBox_.setSelectedId (juce::jlimit (1, juce::jmax (1, (int) mics_.size()), defaultMic + 1),
                               juce::dontSendNotification);
        addAndMakeVisible (micBox_);

        spkLabel_.setText ("Speaker", juce::dontSendNotification);
        spkLabel_.setColour (juce::Label::textColourId, Brand::muted());
        addAndMakeVisible (spkLabel_);
        int firstEnabled = 0;
        for (int i = 0; i < (int) speakers_.size(); ++i)
        {
            juce::String name = "Q21S_" + juce::String (i + 1);
            if (! speakers_[(size_t) i].enabled) name += " (off)";
            spkBox_.addItem (name, i + 1);
            if (firstEnabled == 0 && speakers_[(size_t) i].enabled)
                firstEnabled = i + 1;
        }
        spkBox_.setSelectedId (juce::jmax (1, firstEnabled), juce::dontSendNotification);
        addAndMakeVisible (spkBox_);

        ringLabel_.setText ("Ring", juce::dontSendNotification);
        ringLabel_.setColour (juce::Label::textColourId, Brand::muted());
        addAndMakeVisible (ringLabel_);
        for (int i = 0; i < MicRingSnap::kNumRings; ++i)
            ringBox_.addItem (juce::String ((int) MicRingSnap::kRingsM[i]) + " m", i + 1);
        ringBox_.setSelectedId (2, juce::dontSendNotification); // 2 m default
        addAndMakeVisible (ringBox_);

        applyBtn_.setButtonText ("Place");
        applyBtn_.setColour (juce::TextButton::buttonColourId, Brand::accent());
        applyBtn_.setColour (juce::TextButton::textColourOffId, Brand::onAccent());
        applyBtn_.onClick = [this]
        {
            if (onApply && micBox_.getSelectedId() > 0 && spkBox_.getSelectedId() > 0
                && ringBox_.getSelectedId() > 0)
            {
                onApply (micBox_.getSelectedId() - 1,
                         spkBox_.getSelectedId() - 1,
                         MicRingSnap::kRingsM[ringBox_.getSelectedId() - 1]);
            }
            if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
                dw->exitModalState (1);
        };
        addAndMakeVisible (applyBtn_);

        setSize (280, 210);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());
        g.setColour (Brand::border());
        g.drawRect (getLocalBounds(), 1);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (12);
        title_.setBounds (r.removeFromTop (24));
        r.removeFromTop (8);
        auto row = [&] (juce::Label& lab, juce::ComboBox& box)
        {
            auto line = r.removeFromTop (26);
            lab.setBounds (line.removeFromLeft (70));
            box.setBounds (line);
            r.removeFromTop (8);
        };
        row (micLabel_, micBox_);
        row (spkLabel_, spkBox_);
        row (ringLabel_, ringBox_);
        applyBtn_.setBounds (r.removeFromBottom (28).withSizeKeepingCentre (100, 28));
    }

private:
    std::vector<MicReceiver> mics_;
    std::vector<Speaker> speakers_;
    juce::Label title_, micLabel_, spkLabel_, ringLabel_;
    juce::ComboBox micBox_, spkBox_, ringBox_;
    juce::TextButton applyBtn_;
};

#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "AppSettings.h"

// ---------------------------------------------------------------------------
// PreferencesComponent - the Preferences / Settings tab. Lets the user pick
// the UI theme (Dark / Light) and the unit system (SI / Imperial). Choices are
// written straight into AppSettings, which persists them and broadcasts the
// change so the whole UI updates live.
// ---------------------------------------------------------------------------
class PreferencesComponent : public juce::Component
{
public:
    PreferencesComponent();

    void paint   (juce::Graphics&) override;
    void resized () override;
    void lookAndFeelChanged() override;

    std::function<void()> onClose;

private:
    void styleSeg  (juce::TextButton&, const juce::String&);
    void applyColours();
    void refreshSelection();
    void applyScaledChrome();

    juce::Label title_, appearanceHdr_, unitsHdr_;
    juce::Label themeLabel_, unitsLabel_, unitsNote_;

    juce::TextButton themeDarkBtn_  { "DARK" },  themeLightBtn_ { "LIGHT" };
    juce::TextButton unitsSiBtn_    { "SI" },    unitsImpBtn_   { "IMPERIAL" };
    juce::TextButton closeBtn_      { "CLOSE" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PreferencesComponent)
};

#pragma once
#include <JuceHeader.h>
#include "AcousticEngine.h"

// ---------------------------------------------------------------------------
// InfoPanel — read-only physics & scene summary (world, frequency/wavelength,
// active-speaker count, and the currently selected speaker's parameters).
// ---------------------------------------------------------------------------
class InfoPanel : public juce::Component
{
public:
    InfoPanel();

    void updateInfo (const SimResult& r, const SimParams& p, int selectedIndex);

    // Total height needed to show both cards (valid after resized()).
    int getContentHeight() const { return contentHeight_; }

    void paint  (juce::Graphics&) override;
    void resized() override;
    void lookAndFeelChanged() override;   // re-apply theme colours

private:
    struct Row { juce::Label key, val; };
    juce::OwnedArray<Row> rows_;

    juce::Label sceneHeader_, speakerHeader_;
    int sceneCardEnd_ = 0;
    int speakerCardY_ = 0;
    int speakerCardEnd_ = 0;
    int contentHeight_ = 0;

    void addRow (const juce::String& key);
    void setRowVal (int index, const juce::String& val);
    void applyColours();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (InfoPanel)
};

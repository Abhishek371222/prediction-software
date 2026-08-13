#pragma once

#include <JuceHeader.h>
#include "MeasurementLibrary.h"
#include "PolarPlotComponent.h"
#include <memory>
#include <vector>

class MainComponent : public juce::Component,
                      private juce::ComboBox::Listener,
                      private juce::Button::Listener
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    void comboBoxChanged (juce::ComboBox* box) override;
    void buttonClicked (juce::Button* button) override;

    void rebuildSetList();
    void rebuildDistanceList();
    void rebuildFrequencyToggles();
    void refreshPlot();
    juce::File findDataFolder() const;
    juce::Colour colourForFrequency (int freqHz) const;

    MeasurementLibrary library;
    PolarPlotComponent polarPlot;

    juce::Label titleLabel;
    juce::Label setLabel;
    juce::ComboBox setBox;
    juce::Label distLabel;
    juce::ComboBox distBox;
    juce::Label normLabel;
    juce::ComboBox normBox;
    juce::Label freqLabel;
    juce::TextButton selectAllBtn { "All" };
    juce::TextButton selectNoneBtn { "None" };
    juce::Label statusLabel;
    juce::Label helpLabel;

    juce::OwnedArray<juce::ToggleButton> freqToggles;
    std::vector<int> currentFreqs;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

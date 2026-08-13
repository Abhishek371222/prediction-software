#pragma once

#include <JuceHeader.h>
#include "MeasurementLibrary.h"
#include <vector>

struct PlotSeries
{
    juce::String label;
    juce::Colour colour;
    std::vector<PolarPoint> points; // degree + relative dB already
};

class PolarPlotComponent : public juce::Component
{
public:
    PolarPlotComponent();

    void setSeries (std::vector<PlotSeries> newSeries);
    void setTitle (const juce::String& titleText);
    void setSubtitle (const juce::String& subtitleText);
    void setDbRange (float minDb, float maxDb);

    void paint (juce::Graphics& g) override;

private:
    juce::Point<float> polarToScreen (float degreeClockwiseFromNorth,
                                      float db,
                                      juce::Point<float> centre,
                                      float radiusPx) const;

    std::vector<PlotSeries> series;
    juce::String title { "2D Directivity Analysis" };
    juce::String subtitle;
    float dbMin = -30.0f;
    float dbMax = 6.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PolarPlotComponent)
};

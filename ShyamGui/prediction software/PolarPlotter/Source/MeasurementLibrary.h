#pragma once

#include <JuceHeader.h>
#include <vector>

struct PolarPoint
{
    float degree = 0.0f;
    float splDb = 0.0f;
};

struct PolarSweep
{
    juce::String setName;
    int freqHz = 0;
    float distanceM = 1.0f;
    juce::String fileName;
    juce::String sourceXlsx;
    int nPoints = 0;
    std::vector<PolarPoint> points;

    float onAxisSpl() const;
    float peakSpl() const;
    /** CLIO-style: dB relative to on-axis (0 deg). */
    std::vector<float> relativeToOnAxis() const;
    /** Peak-normalized dB. */
    std::vector<float> relativeToPeak() const;
};

class MeasurementLibrary
{
public:
    bool loadFromFolder (const juce::File& dataFolder);
    juce::StringArray getSetNames() const;
    std::vector<float> getDistances (const juce::String& setName) const;
    std::vector<int> getFrequencies (const juce::String& setName, float distanceM) const;
    const PolarSweep* find (const juce::String& setName, int freqHz, float distanceM) const;
    juce::String getStatus() const { return status; }
    int getSweepCount() const { return (int) sweeps.size(); }
    int getSetCount() const;

private:
    bool loadCsv (const juce::File& file, PolarSweep& sweep);
    std::vector<PolarSweep> sweeps;
    juce::String status;
};

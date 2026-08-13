#include "MeasurementLibrary.h"
#include <algorithm>
#include <cmath>

float PolarSweep::onAxisSpl() const
{
    if (points.empty())
        return 0.0f;

    float bestDeg = 1.0e9f;
    float bestSpl = points.front().splDb;

    for (const auto& p : points)
    {
        const float wrapped = std::fmod (p.degree + 180.0f, 360.0f) - 180.0f;
        const float ad = std::abs (wrapped);
        if (ad < bestDeg)
        {
            bestDeg = ad;
            bestSpl = p.splDb;
        }
    }

    return bestSpl;
}

float PolarSweep::peakSpl() const
{
    float peak = -1.0e9f;
    for (const auto& p : points)
        peak = juce::jmax (peak, p.splDb);
    return peak;
}

std::vector<float> PolarSweep::relativeToOnAxis() const
{
    const float ref = onAxisSpl();
    std::vector<float> out;
    out.reserve (points.size());
    for (const auto& p : points)
        out.push_back (p.splDb - ref);
    return out;
}

std::vector<float> PolarSweep::relativeToPeak() const
{
    const float ref = peakSpl();
    std::vector<float> out;
    out.reserve (points.size());
    for (const auto& p : points)
        out.push_back (p.splDb - ref);
    return out;
}

bool MeasurementLibrary::loadCsv (const juce::File& file, PolarSweep& sweep)
{
    if (! file.existsAsFile())
        return false;

    juce::StringArray lines;
    file.readLines (lines);

    sweep.points.clear();

    for (int i = 0; i < lines.size(); ++i)
    {
        auto line = lines[i].trim();
        if (line.isEmpty() || line.startsWithIgnoreCase ("degree"))
            continue;

        auto tokens = juce::StringArray::fromTokens (line, ",", "\"");
        if (tokens.size() < 2)
            continue;

        PolarPoint pt;
        pt.degree = tokens[0].getFloatValue();
        pt.splDb = tokens[1].getFloatValue();
        sweep.points.push_back (pt);
    }

    std::sort (sweep.points.begin(), sweep.points.end(),
               [] (const PolarPoint& a, const PolarPoint& b) { return a.degree < b.degree; });

    sweep.nPoints = (int) sweep.points.size();
    return ! sweep.points.empty();
}

bool MeasurementLibrary::loadFromFolder (const juce::File& dataFolder)
{
    sweeps.clear();
    status.clear();

    if (! dataFolder.isDirectory())
    {
        status = "Data folder not found: " + dataFolder.getFullPathName();
        return false;
    }

    const auto manifest = dataFolder.getChildFile ("manifest.csv");
    if (! manifest.existsAsFile())
    {
        status = "manifest.csv missing in " + dataFolder.getFullPathName();
        return false;
    }

    juce::StringArray lines;
    manifest.readLines (lines);

    int loaded = 0;
    for (int i = 0; i < lines.size(); ++i)
    {
        auto line = lines[i].trim();
        if (line.isEmpty() || line.startsWithIgnoreCase ("set,"))
            continue;

        auto tokens = juce::StringArray::fromTokens (line, ",", "\"");
        if (tokens.size() < 4)
            continue;

        PolarSweep sweep;
        sweep.setName = tokens[0].trim();
        sweep.freqHz = tokens[1].getIntValue();
        sweep.distanceM = tokens[2].getFloatValue();
        sweep.fileName = tokens[3].trim();
        if (tokens.size() >= 5)
            sweep.sourceXlsx = tokens[4].trim();

        const auto csv = dataFolder.getChildFile (sweep.fileName);
        if (! loadCsv (csv, sweep))
            continue;

        sweeps.push_back (std::move (sweep));
        ++loaded;
    }

    status = juce::String (loaded) + " real sweeps | "
             + juce::String (getSetCount()) + " sets | "
             + dataFolder.getFullPathName();
    return loaded > 0;
}

int MeasurementLibrary::getSetCount() const
{
    return getSetNames().size();
}

juce::StringArray MeasurementLibrary::getSetNames() const
{
    juce::StringArray names;
    // ShyamGuild first — reference set for the prediction software
    bool hasShyam = false;
    for (const auto& s : sweeps)
        if (s.setName == "ShyamGuild")
            hasShyam = true;
    if (hasShyam)
        names.add ("ShyamGuild");

    for (const auto& s : sweeps)
        if (s.setName != "ShyamGuild")
            names.addIfNotAlreadyThere (s.setName);

    return names;
}

std::vector<float> MeasurementLibrary::getDistances (const juce::String& setName) const
{
    std::vector<float> dists;
    for (const auto& s : sweeps)
    {
        if (s.setName != setName)
            continue;
        if (std::find_if (dists.begin(), dists.end(),
                          [&] (float d) { return std::abs (d - s.distanceM) < 1.0e-4f; })
            == dists.end())
            dists.push_back (s.distanceM);
    }
    std::sort (dists.begin(), dists.end());
    return dists;
}

std::vector<int> MeasurementLibrary::getFrequencies (const juce::String& setName, float distanceM) const
{
    std::vector<int> freqs;
    for (const auto& s : sweeps)
    {
        if (s.setName != setName)
            continue;
        if (std::abs (s.distanceM - distanceM) > 1.0e-4f)
            continue;
        if (std::find (freqs.begin(), freqs.end(), s.freqHz) == freqs.end())
            freqs.push_back (s.freqHz);
    }
    std::sort (freqs.begin(), freqs.end());
    return freqs;
}

const PolarSweep* MeasurementLibrary::find (const juce::String& setName, int freqHz, float distanceM) const
{
    for (const auto& s : sweeps)
    {
        if (s.setName == setName && s.freqHz == freqHz && std::abs (s.distanceM - distanceM) < 1.0e-4f)
            return &s;
    }
    return nullptr;
}

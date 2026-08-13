#include "PolarPlotComponent.h"
#include <cmath>

namespace
{
constexpr float kPi = 3.14159265358979323846f;

float degToRad (float deg)
{
    return deg * kPi / 180.0f;
}
}

PolarPlotComponent::PolarPlotComponent()
{
    setOpaque (true);
}

void PolarPlotComponent::setSeries (std::vector<PlotSeries> newSeries)
{
    series = std::move (newSeries);
    repaint();
}

void PolarPlotComponent::setTitle (const juce::String& titleText)
{
    title = titleText;
    repaint();
}

void PolarPlotComponent::setSubtitle (const juce::String& subtitleText)
{
    subtitle = subtitleText;
    repaint();
}

void PolarPlotComponent::setDbRange (float minDb, float maxDb)
{
    dbMin = minDb;
    dbMax = maxDb;
    repaint();
}

juce::Point<float> PolarPlotComponent::polarToScreen (float degreeClockwiseFromNorth,
                                                      float db,
                                                      juce::Point<float> centre,
                                                      float radiusPx) const
{
    // CLIO: 0 deg at top, clockwise. Screen y grows downward.
    const float theta = degToRad (degreeClockwiseFromNorth);
    const float t = juce::jlimit (0.0f, 1.0f, (db - dbMin) / (dbMax - dbMin));
    const float r = t * radiusPx;
    const float x = centre.x + r * std::sin (theta);
    const float y = centre.y - r * std::cos (theta);
    return { x, y };
}

void PolarPlotComponent::paint (juce::Graphics& g)
{
    juce::Rectangle<float> bounds = getLocalBounds().toFloat();
    g.fillAll (juce::Colour (0xff0b1220));

    // Header
    g.setColour (juce::Colour (0xff7dd3fc));
    g.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
    g.drawText ("ATOMIK", bounds.removeFromTop (28.0f).reduced (16.0f, 0.0f),
                juce::Justification::centredLeft, true);

    g.setColour (juce::Colours::white);
    g.setFont (juce::FontOptions (16.0f).withStyle ("Bold"));
    g.drawText (title, bounds.removeFromTop (24.0f), juce::Justification::centred, true);

    if (subtitle.isNotEmpty())
    {
        g.setColour (juce::Colour (0xff94a3b8));
        g.setFont (juce::FontOptions (13.0f));
        g.drawText (subtitle, bounds.removeFromTop (20.0f), juce::Justification::centredRight, true);
    }

    // Legend
    {
        auto legendArea = bounds.removeFromTop (22.0f).reduced (16.0f, 0.0f);
        float x = legendArea.getX();
        g.setFont (juce::FontOptions (12.0f).withStyle ("Bold"));
        for (const auto& s : series)
        {
            g.setColour (s.colour);
            g.fillRect (juce::Rectangle<float> (x, legendArea.getCentreY() - 4.0f, 14.0f, 8.0f));
            g.drawText (s.label,
                        juce::Rectangle<float> (x + 18.0f, legendArea.getY(), 70.0f, legendArea.getHeight()),
                        juce::Justification::centredLeft, true);
            x += 90.0f;
        }
    }

    const auto plotArea = bounds.reduced (20.0f, 10.0f);
    const float size = juce::jmin (plotArea.getWidth(), plotArea.getHeight()) - 20.0f;
    const juce::Point<float> centre { plotArea.getCentreX(), plotArea.getCentreY() };
    const float radius = size * 0.5f;

    // Plot disc
    g.setColour (juce::Colours::white);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    auto ringRadius = [&] (float db)
    {
        const float t = juce::jlimit (0.0f, 1.0f, (db - dbMin) / (dbMax - dbMin));
        return t * radius;
    };

    // Solid dB rings
    const float solidRings[] = { 6.0f, 0.0f, -6.0f, -12.0f, -18.0f, -24.0f };
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    for (float db : solidRings)
    {
        const float r = ringRadius (db);
        g.drawEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 1.0f);
    }

    // Intermediate dB rings (lighter, like CLIO dashed rings)
    const float dashRings[] = { 3.0f, -3.0f, -9.0f, -15.0f, -21.0f, -27.0f };
    g.setColour (juce::Colours::black.withAlpha (0.28f));
    for (float db : dashRings)
    {
        const float r = ringRadius (db);
        g.drawEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 0.8f);
    }

    // Angle spokes
    for (int a = 0; a < 360; a += 15)
    {
        const bool major = (a % 30) == 0;
        g.setColour (juce::Colours::black.withAlpha (major ? 0.55f : 0.3f));
        const auto outer = polarToScreen ((float) a, dbMax, centre, radius);
        const auto inner = polarToScreen ((float) a, dbMin, centre, radius);
        g.drawLine ({ inner, outer }, major ? 1.0f : 0.7f);
    }

    // dB labels along top spoke
    g.setFont (juce::FontOptions (11.0f));
    g.setColour (juce::Colours::black);
    for (float db : solidRings)
    {
        const auto p = polarToScreen (2.0f, db, centre, radius);
        g.drawText (juce::String ((int) db),
                    juce::Rectangle<float> (p.x + 4.0f, p.y - 8.0f, 28.0f, 16.0f),
                    juce::Justification::centredLeft, false);
    }
    g.drawText ("dB",
                juce::Rectangle<float> (centre.x - 10.0f, centre.y - radius - 18.0f, 24.0f, 14.0f),
                juce::Justification::centred, false);

    // Angle labels (CLIO: 0 at top, clockwise, negative on left)
    const char* labels[] = { "0", "30", "60", "90", "120", "150", "180",
                             "-150", "-120", "-90", "-60", "-30" };
    g.setFont (juce::FontOptions (12.0f));
    for (int i = 0; i < 12; ++i)
    {
        const float a = (float) (i * 30);
        const auto p = polarToScreen (a, dbMax + 2.5f, centre, radius);
        g.setColour (juce::Colour (0xffe2e8f0));
        g.drawText (juce::String (labels[i]) + " deg",
                    juce::Rectangle<float> (p.x - 22.0f, p.y - 8.0f, 44.0f, 16.0f),
                    juce::Justification::centred, false);
    }

    // Centre label
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.setFont (juce::FontOptions (13.0f).withStyle ("Bold"));
    g.drawText ("ATOMIK",
                juce::Rectangle<float> (centre.x - 40.0f, centre.y - 8.0f, 80.0f, 16.0f),
                juce::Justification::centred, false);

    // Curves
    for (const auto& s : series)
    {
        if (s.points.size() < 2)
            continue;

        juce::Path path;
        bool started = false;
        for (const auto& pt : s.points)
        {
            const auto screen = polarToScreen (pt.degree, pt.splDb, centre, radius);
            if (! started)
            {
                path.startNewSubPath (screen);
                started = true;
            }
            else
            {
                path.lineTo (screen);
            }
        }

        // Close to first point for full rotation
        const auto first = polarToScreen (s.points.front().degree, s.points.front().splDb, centre, radius);
        path.lineTo (first);

        g.setColour (s.colour);
        g.strokePath (path, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }

    // Footer
    g.setColour (juce::Colour (0xff64748b));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("dB normalized to on-axis (0 deg)  |  Phase 1: Measured Polar",
                getLocalBounds().removeFromBottom (22).toFloat(),
                juce::Justification::centred, true);
}

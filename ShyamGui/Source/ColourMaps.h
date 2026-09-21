#pragma once
#include <JuceHeader.h>
#include <cmath>

// ---------------------------------------------------------------------------
// ColourMaps — maps a normalised float t in [0,1] to a juce::Colour.
// Primary SPL map: 7-color heatmap (black→blue→cyan→green→yellow→orange→red).
// cmapIndex: 0=seven  1=turbo  2=hot  3=parula  4=gray  5=jet
// ---------------------------------------------------------------------------
namespace ColourMaps
{

// Atomik Rel. SPL legend gradient, straight off the Figma colour picker.
// Legend top->bottom = 0 db -> -36 db, six stops at the designer's positions
// (they are NOT evenly spaced -- the red holds to 30% and the purple crossover
// is pulled up to 47%, which is what gives the new ramp its longer hot band):
//   #FF0103 -> #F65556 -> #CD617C -> #3281B9 -> #003A6C -> #151515
// t = 1 -> peak / 0 db (top of bar); t = 0 -> floor / -36 db (bottom).
inline juce::Colour sevenColor (float t)
{
    // Positions are distance from the TOP of the Rel. SPL bar (0 = 0 db).
    static const float stops[][4] = {
        { 0.00f, 1.000f, 0.004f, 0.012f },  // #FF0103  0 db
        { 0.30f, 0.965f, 0.333f, 0.337f },  // #F65556
        { 0.47f, 0.804f, 0.380f, 0.486f },  // #CD617C
        { 0.67f, 0.196f, 0.506f, 0.725f },  // #3281B9
        { 0.83f, 0.000f, 0.227f, 0.424f },  // #003A6C
        { 1.00f, 0.082f, 0.082f, 0.082f }   // #151515  -36 db
    };
    const float u = 1.0f - juce::jlimit (0.0f, 1.0f, t); // peak -> top of legend
    constexpr int n = 5;   // segments = stops - 1
    int i = 0;
    while (i < n && u > stops[i + 1][0]) ++i;
    const float span = stops[i + 1][0] - stops[i][0];
    const float f = (span > 1.0e-6f) ? (u - stops[i][0]) / span : 0.0f;
    const float r = stops[i][1] + f * (stops[i + 1][1] - stops[i][1]);
    const float g = stops[i][2] + f * (stops[i + 1][2] - stops[i][2]);
    const float b = stops[i][3] + f * (stops[i + 1][3] - stops[i][3]);
    return juce::Colour::fromFloatRGBA (r, g, b, 1.0f);
}

inline juce::Colour jet (float t)
{
    t = juce::jlimit (0.0f, 1.0f, t);
    float r = juce::jlimit (0.0f, 1.0f, 1.5f - std::abs (4.0f * t - 3.0f));
    float g = juce::jlimit (0.0f, 1.0f, 1.5f - std::abs (4.0f * t - 2.0f));
    float b = juce::jlimit (0.0f, 1.0f, 1.5f - std::abs (4.0f * t - 1.0f));
    return juce::Colour::fromFloatRGBA (r, g, b, 1.0f);
}

inline juce::Colour turbo (float t)
{
    // Turbo colourmap polynomial approximation (Google 2019)
    t = juce::jlimit (0.0f, 1.0f, t);
    const float r = (float)(0.1357 + t*(4.5974 + t*(-42.3277 + t*(130.5887 + t*(-150.5799 + t*57.8187)))));
    const float g = (float)(0.0914 + t*(2.1856 + t*(4.8052  + t*(-14.0741 + t*(14.3534  + t*(-6.7919))))));
    const float b = (float)(0.1067 + t*(12.5925+ t*(-60.1097+ t*(109.0745 + t*(-88.5267 + t*26.9797)))));
    return juce::Colour::fromFloatRGBA (
        juce::jlimit(0.0f,1.0f,r),
        juce::jlimit(0.0f,1.0f,g),
        juce::jlimit(0.0f,1.0f,b), 1.0f);
}

inline juce::Colour hot (float t)
{
    t = juce::jlimit (0.0f, 1.0f, t);
    float r = juce::jlimit (0.0f, 1.0f, t * 3.0f);
    float g = juce::jlimit (0.0f, 1.0f, t * 3.0f - 1.0f);
    float b = juce::jlimit (0.0f, 1.0f, t * 3.0f - 2.0f);
    return juce::Colour::fromFloatRGBA (r, g, b, 1.0f);
}

inline juce::Colour parula (float t)
{
    // Sampled 8-stop parula LUT
    static const float lut[][3] = {
        {0.208f, 0.166f, 0.529f},
        {0.211f, 0.357f, 0.750f},
        {0.122f, 0.526f, 0.745f},
        {0.100f, 0.636f, 0.608f},
        {0.283f, 0.710f, 0.430f},
        {0.683f, 0.749f, 0.198f},
        {0.980f, 0.795f, 0.131f},
        {0.976f, 0.984f, 0.133f}
    };
    t = juce::jlimit (0.0f, 1.0f, t) * 7.0f;
    int   i = (int)t;
    float f = t - i;
    if (i >= 7) { i = 6; f = 1.0f; }
    float r = lut[i][0] + f*(lut[i+1][0]-lut[i][0]);
    float g = lut[i][1] + f*(lut[i+1][1]-lut[i][1]);
    float b = lut[i][2] + f*(lut[i+1][2]-lut[i][2]);
    return juce::Colour::fromFloatRGBA (r, g, b, 1.0f);
}

inline juce::Colour gray (float t)
{
    t = juce::jlimit (0.0f, 1.0f, t);
    return juce::Colour::fromFloatRGBA (t, t, t, 1.0f);
}

// ---------------------------------------------------------------------------
// dB floor is display range only — it does NOT stretch colours. A level of
// -6 dB is always the same colour whether the floor is -36 or -54; the floor
// just clips everything below it to the bottom of the scale.
// ---------------------------------------------------------------------------

// Design span of sevenColor / palette (0 … −36 dB).
inline constexpr float kRelSplDesignSpanDB = 36.0f;
inline constexpr float kRelSplStepDB       = 6.0f;

// Continuous map: fixed dB→colour (0 → t=1, −36 → t=0). Floor only clips.
inline float relDbToColourT (float dB, float floorDB) noexcept
{
    if (floorDB < 0.0f && dB <= floorDB)
        return 0.0f;
    return juce::jlimit (0.0f, 1.0f, (dB + kRelSplDesignSpanDB) / kRelSplDesignSpanDB);
}

// Hard-banded colour for a relative SPL value in dB (<= 0).
// Fixed step size (6 dB default, or 3 dB contour bands). Floor clips only.
// Contour mode quantises the SAME gradient the legend draws, rather than
// indexing the 7-entry palette. Indexing only lined up when the step was 6 dB
// (7 entries x 6 dB = the 0..-36 span); at the UI's 3 dB step it ran out of
// entries by -18 dB and flattened everything quieter than that to black.
inline juce::Colour splBand (float dB, float stepDB = 6.0f)
{
    if (stepDB < 0.5f) stepDB = 0.5f;
    const float bandTopDB = -std::floor ((-dB) / stepDB + 1.0e-4f) * stepDB;
    // floorDB = 0 so nothing is clipped here; the caller applies the dB floor.
    return sevenColor (relDbToColourT (bandTopDB, 0.0f));
}

inline juce::Colour splBandForFloor (float dB, float floorDB, float stepDB = 6.0f)
{
    if (floorDB < 0.0f && dB <= floorDB)
        return sevenColor (0.0f);   // bottom of the gradient
    return splBand (dB, stepDB);
}

// Diverging blue-white-red map for signed pressure t in [0,1] (0.5 = zero).
inline juce::Colour diverging (float t)
{
    t = juce::jlimit (0.0f, 1.0f, t);
    if (t < 0.5f)
    {
        float u = t / 0.5f;                       // 0..1  (blue -> white)
        return juce::Colour::fromFloatRGBA (u, u, 1.0f, 1.0f);
    }
    float u = (t - 0.5f) / 0.5f;                  // 0..1  (white -> red)
    return juce::Colour::fromFloatRGBA (1.0f, 1.0f - u, 1.0f - u, 1.0f);
}

inline juce::Colour apply (float t, int index)
{
    switch (index)
    {
        case 0:  return sevenColor (t);
        case 1:  return turbo  (t);
        case 2:  return hot    (t);
        case 3:  return parula (t);
        case 4:  return gray   (t);
        case 5:  return jet    (t);
        default: return sevenColor (t);
    }
}

} // namespace ColourMaps

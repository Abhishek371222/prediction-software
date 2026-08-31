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

// Atomik Rel. SPL legend (mockup + Graph colors.pdf).
// Legend top→bottom = 0 db → -36 db:
//   #530000 → #B21619 → #ED2227 → (magenta bridge) → #3281B9 → #0A4D74 → #231F20
// t = 1 → peak / 0 db (top of bar); t = 0 → floor / -36 db (bottom).
inline juce::Colour sevenColor (float t)
{
    // Positions are distance from the TOP of the Rel. SPL bar (0 = 0 db).
    static const float stops[][4] = {
        { 0.00f, 0.325f, 0.000f, 0.000f },  // #530000  0 db
        { 0.17f, 0.698f, 0.086f, 0.098f },  // #B21619
        { 0.36f, 0.929f, 0.133f, 0.153f },  // #ED2227
        { 0.50f, 0.651f, 0.275f, 0.369f },  // #A6465E  red→blue bridge
        { 0.58f, 0.498f, 0.349f, 0.486f },  // #7F597C
        { 0.68f, 0.196f, 0.506f, 0.725f },  // #3281B9
        { 0.84f, 0.039f, 0.302f, 0.455f },  // #0A4D74
        { 1.00f, 0.137f, 0.122f, 0.125f }   // #231F20 -36 db
    };
    const float u = 1.0f - juce::jlimit (0.0f, 1.0f, t); // peak → top of legend
    constexpr int n = 7;
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
// Discrete bands matching Rel. SPL mockup ticks (0 → −36 db, 6 db steps).
// dB floor is display range only — it does NOT stretch colours. A level of
// −6 dB is always the same palette stop whether floor is −36 or −54.
// ---------------------------------------------------------------------------
inline const juce::Colour* splPalette()
{
    static const juce::Colour bands[7] = {
        juce::Colour (0xff530000),  //  0 db
        juce::Colour (0xffb21619),  // -6 db
        juce::Colour (0xffed2227),  // -12 db
        juce::Colour (0xff7f597c),  // -18 db  magenta bridge
        juce::Colour (0xff3281b9),  // -24 db
        juce::Colour (0xff0a4d74),  // -30 db
        juce::Colour (0xff231f20)   // -36 db (and quieter / below floor)
    };
    return bands;
}

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
inline juce::Colour splBand (float dB, float stepDB = 6.0f)
{
    if (stepDB < 0.5f) stepDB = 0.5f;
    int idx = (int) std::floor ((-dB) / stepDB + 1.0e-4f);
    idx = juce::jlimit (0, 6, idx);
    return splPalette()[idx];
}

inline juce::Colour splBandForFloor (float dB, float floorDB, float stepDB = 6.0f)
{
    if (floorDB < 0.0f && dB <= floorDB)
        return splPalette()[6];
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

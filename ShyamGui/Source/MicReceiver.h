#pragma once
#include <JuceHeader.h>
#include <vector>

// ---------------------------------------------------------------------------
// MicReceiver — virtual probe on the SPL heatmap (does not affect physics).
// ---------------------------------------------------------------------------
struct MicReceiver
{
    int   id = 1;                 // Mic 1, Mic 2, …
    float x = 50.0f;              // world metres
    float y = 50.0f;
    float relDb = 0.0f;           // live relative SPL from heatmap (peak ≈ 0)
    bool  levelOk = false;        // true when heatmap sample succeeded
    bool  ringLocked = false;     // placed / locked onto a distance ring
    float ringRadiusM = 0.0f;     // 1 / 2 / 4 / 8 when ringLocked
    int   ringSpeaker = -1;       // speaker index used for ring (optional)
};

inline juce::String micDisplayName (const MicReceiver& m)
{
    return "Mic " + juce::String (m.id);
}

inline int nextMicId (const std::vector<MicReceiver>& mics)
{
    int best = 0;
    for (const auto& m : mics)
        best = juce::jmax (best, m.id);
    return best + 1;
}

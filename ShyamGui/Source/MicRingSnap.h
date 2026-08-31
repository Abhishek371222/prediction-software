#pragma once
#include "AcousticEngine.h"
#include "MicReceiver.h"
#include <cmath>
#include <vector>
#include <cstring>

#if JUCE_WINDOWS
 #ifndef NOMINMAX
  #define NOMINMAX
 #endif
 #include <windows.h>
 #include <mmsystem.h>
 #ifdef min
  #undef min
 #endif
 #ifdef max
  #undef max
 #endif
 #pragma comment(lib, "winmm.lib")
#endif

// ---------------------------------------------------------------------------
// Ring snap (1 / 2 / 4 / 8 m around enabled speakers) + short "tak" click.
// ---------------------------------------------------------------------------
namespace MicRingSnap
{
    static constexpr float kRingsM[] = { 1.0f, 2.0f, 4.0f, 8.0f };
    static constexpr int   kNumRings = 4;
    static constexpr float kSnapTolM = 0.45f;

    struct SnapResult
    {
        float x = 0.0f, y = 0.0f;
        float radiusM = 0.0f;
        int   speakerIndex = -1;
        bool  snapped = false;
    };

    inline SnapResult snapToRing (float wx, float wy,
                                  const std::vector<Speaker>& speakers,
                                  float tolM = kSnapTolM)
    {
        SnapResult best;
        best.x = wx;
        best.y = wy;
        float bestDist = tolM;

        for (int si = 0; si < (int) speakers.size(); ++si)
        {
            const auto& s = speakers[(size_t) si];
            if (! s.enabled) continue;
            const float dx = wx - s.x;
            const float dy = wy - s.y;
            const float r = std::sqrt (dx * dx + dy * dy);
            if (r < 1.0e-6f) continue;

            for (int i = 0; i < kNumRings; ++i)
            {
                const float ring = kRingsM[i];
                const float err = std::abs (r - ring);
                if (err <= bestDist)
                {
                    bestDist = err;
                    const float scale = ring / r;
                    best.x = s.x + dx * scale;
                    best.y = s.y + dy * scale;
                    best.radiusM = ring;
                    best.speakerIndex = si;
                    best.snapped = true;
                }
            }
        }
        return best;
    }

    /** Force mic onto a specific ring around a speaker. */
    inline SnapResult placeOnRing (const Speaker& s, float radiusM, float hintX, float hintY)
    {
        SnapResult out;
        out.radiusM = radiusM;
        out.snapped = true;
        float dx = hintX - s.x;
        float dy = hintY - s.y;
        float r = std::sqrt (dx * dx + dy * dy);
        if (r < 1.0e-6f) { dx = 1.0f; dy = 0.0f; r = 1.0f; }
        out.x = s.x + dx / r * radiusM;
        out.y = s.y + dy / r * radiusM;
        return out;
    }

    /** Angle of (wx,wy) about speaker, degrees, relative to Q21S facing.
        0° = forward (+X, or −X when reverseOrientation), CCW like the polar / heatmap. */
    inline float angleDegFromSpeaker (float wx, float wy, const Speaker& s) noexcept
    {
        const float dx = wx - s.x;
        const float dy = wy - s.y;
        if (dx * dx + dy * dy < 1.0e-12f)
            return 0.0f;
        const float facing = s.reverseOrientation ? (float) M_PI : 0.0f;
        float deg = (std::atan2 (dy, dx) - facing) * (180.0f / (float) M_PI);
        while (deg < 0.0f)   deg += 360.0f;
        while (deg >= 360.0f) deg -= 360.0f;
        return deg;
    }

    /** Prefer ring-locked speaker; else nearest enabled speaker. -1 if none. */
    inline int referenceSpeakerIndex (float wx, float wy,
                                      const std::vector<Speaker>& speakers,
                                      int preferredIndex = -1) noexcept
    {
        if (preferredIndex >= 0 && preferredIndex < (int) speakers.size()
            && speakers[(size_t) preferredIndex].enabled)
            return preferredIndex;

        int best = -1;
        float bestD2 = 0.0f;
        for (int i = 0; i < (int) speakers.size(); ++i)
        {
            const auto& s = speakers[(size_t) i];
            if (! s.enabled) continue;
            const float dx = wx - s.x;
            const float dy = wy - s.y;
            const float d2 = dx * dx + dy * dy;
            if (best < 0 || d2 < bestD2)
            {
                best = i;
                bestD2 = d2;
            }
        }
        return best;
    }
}

namespace SnapClick
{
    // Build a short in-memory PCM WAV (~18 ms, 22.05 kHz, 8-bit mono) and play it.
    inline void playTak()
    {
       #if JUCE_WINDOWS
        static std::vector<char> wav;
        if (wav.empty())
        {
            constexpr int sampleRate = 22050;
            constexpr int nSamples = 400; // ~18 ms
            const int dataBytes = nSamples;
            const int fileSize = 44 + dataBytes;

            wav.resize ((size_t) fileSize, 0);
            auto* p = (unsigned char*) wav.data();
            auto put32 = [&] (int off, uint32_t v)
            {
                p[off] = (unsigned char) (v & 0xff);
                p[off + 1] = (unsigned char) ((v >> 8) & 0xff);
                p[off + 2] = (unsigned char) ((v >> 16) & 0xff);
                p[off + 3] = (unsigned char) ((v >> 24) & 0xff);
            };
            auto put16 = [&] (int off, uint16_t v)
            {
                p[off] = (unsigned char) (v & 0xff);
                p[off + 1] = (unsigned char) ((v >> 8) & 0xff);
            };

            std::memcpy (p, "RIFF", 4);
            put32 (4, (uint32_t) (fileSize - 8));
            std::memcpy (p + 8, "WAVE", 4);
            std::memcpy (p + 12, "fmt ", 4);
            put32 (16, 16);
            put16 (20, 1);                 // PCM
            put16 (22, 1);                 // mono
            put32 (24, (uint32_t) sampleRate);
            put32 (28, (uint32_t) sampleRate); // byte rate
            put16 (32, 1);                 // block align
            put16 (34, 8);                 // bits
            std::memcpy (p + 36, "data", 4);
            put32 (40, (uint32_t) dataBytes);

            for (int i = 0; i < nSamples; ++i)
            {
                const float t = (float) i / (float) sampleRate;
                const float env = std::exp (-t * 180.0f);
                const float s = env * (0.55f * std::sin (2.0f * 3.14159265f * 1850.0f * t)
                                     + 0.35f * std::sin (2.0f * 3.14159265f * 3100.0f * t));
                const int v = (int) std::lround (128.0f + s * 110.0f);
                p[44 + i] = (unsigned char) (v < 0 ? 0 : (v > 255 ? 255 : v));
            }
        }

        PlaySoundA (wav.data(), nullptr, SND_MEMORY | SND_ASYNC | SND_NODEFAULT);
       #endif
    }
}

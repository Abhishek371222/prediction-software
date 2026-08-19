#pragma once
#include <JuceHeader.h>
#include "AcousticEngine.h"
#include <vector>
#include <cmath>

// ===========================================================================
// AcousticAnalysis - derives the secondary engineering metrics shown in the
// PDF report (RT60, absorption, transmission loss, frequency response).
//
// The core SPL / coverage maps come straight from AcousticEngine. The room
// metrics below are standard analytical estimates (Sabine RT60, mass-law TL)
// from the room geometry and typical surface assumptions; they are labelled as
// model-based estimates in the report. Frequency response is sampled from the
// engine across the supported bands so it reflects the actual array.
// ===========================================================================
namespace AcousticAnalysis
{
    // Standard octave-band centre frequencies for room metrics.
    inline std::vector<double> octaveBands()
    {
        return { 31.5, 63.0, 125.0, 250.0, 500.0, 1000.0, 2000.0, 4000.0 };
    }

    struct RoomModel
    {
        double w = 100.0, d = 100.0, h = 6.0;   // metres (h assumed for the 2D world)
        double surfaceMass = 25.0;            // kg/m^2 (assumed partition, for TL)

        double volume()  const { return w * d * h; }
        double surface() const { return 2.0 * (w * d + w * h + d * h); }
    };

    // Typical broadband-treated room absorption coefficient per octave band.
    inline std::vector<double> absorptionCoefficients()
    {
        return { 0.12, 0.15, 0.20, 0.28, 0.34, 0.40, 0.45, 0.50 };
    }

    // Sabine RT60 per octave band: T = 0.161 * V / (S * a).
    inline std::vector<double> rt60 (const RoomModel& room)
    {
        const auto a = absorptionCoefficients();
        const double V = room.volume(), S = room.surface();
        std::vector<double> out;
        for (double ai : a)
        {
            const double A = juce::jmax (1.0e-3, S * ai);
            out.push_back (0.161 * V / A);
        }
        return out;
    }

    // Mass-law transmission loss per octave band:
    //   TL = 20*log10(m * f) - 47  (dB), floored at 0.
    inline std::vector<double> transmissionLoss (const RoomModel& room)
    {
        std::vector<double> out;
        for (double f : octaveBands())
        {
            const double tl = 20.0 * std::log10 (juce::jmax (1.0, room.surfaceMass * f)) - 47.0;
            out.push_back (juce::jmax (0.0, tl));
        }
        return out;
    }

    // Frequency response sampled from the engine: for each supported band the
    // relative on-axis level at a listening point (array centroid) is taken,
    // then the curve is re-referenced to its own maximum (dB).
    inline void frequencyResponse (const SimParams& base,
                                   const std::vector<DirectivityPattern>& directivity,
                                   const std::vector<double>& freqs,
                                   std::vector<double>& outFreq,
                                   std::vector<double>& outDb)
    {
        outFreq.clear(); outDb.clear();
        if (base.speakers.empty()) return;

        double cx = 0.0, cy = 0.0; int n = 0;
        for (const auto& s : base.speakers) if (s.enabled) { cx += s.x; cy += s.y; ++n; }
        if (n == 0) { cx = base.worldW * 0.5; cy = base.worldH * 0.5; }
        else        { cx /= n; cy /= n; }
        const double ly = juce::jlimit (0.0, base.worldH, cy + 6.0);  // 6 m in front

        std::vector<double> raw;
        for (double f : freqs)
        {
            SimParams p = base;
            p.frequency  = f;
            p.resolution = 120;          // light grid; we only sample one point
            p.directivity = directivity;
            SimResult r = AcousticEngine::compute (p);
            if (r.width <= 0) { raw.push_back (-60.0); continue; }

            const int col = juce::jlimit (0, r.width - 1,
                                (int) std::round (cx / base.worldW * (r.width - 1)));
            const int row = juce::jlimit (0, r.height - 1,
                                (int) std::round (ly / base.worldH * (r.height - 1)));
            const size_t idx = (size_t) row * r.width + col;
            if (r.hasAbsoluteSpl && idx < r.splAbsDB.size())
                raw.push_back ((double) r.splAbsDB[idx]);
            else if (idx < r.splRelDB.size())
                raw.push_back ((double) r.splRelDB[idx]);
            else
                raw.push_back ((double) r.splDB[idx]);
            outFreq.push_back (f);
        }

        double mx = -1.0e9;
        for (double v : raw) mx = juce::jmax (mx, v);
        for (double v : raw) outDb.push_back (v - mx);
    }

    // Coverage uniformity: fraction of the field within `windowDb` of the peak.
    inline double coverageWithin (const SimResult& r, double windowDb = 6.0)
    {
        const auto& field = r.splRelDB.empty() ? r.splDB : r.splRelDB;
        if (field.empty()) return 0.0;
        double mx = -1.0e9;
        for (float v : field) mx = juce::jmax (mx, (double) v);
        size_t inside = 0;
        for (float v : field) if ((double) v >= mx - windowDb) ++inside;
        return 100.0 * (double) inside / (double) field.size();
    }
}

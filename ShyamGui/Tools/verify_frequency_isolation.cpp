// Verifies that each speaker model is simulated at its OWN frequency and that
// one model's frequency cannot alter another model's contribution.
//
// Build (from the repo root):
//   cl /std:c++17 /O2 /EHsc /I ShyamGui\Source ShyamGui\Tools\verify_frequency_isolation.cpp ^
//      ShyamGui\Source\AcousticEngine.cpp /Fe:verify_frequency_isolation.exe
//
// The engine is pure C++ here - no JUCE - so this links the shipping
// AcousticEngine.cpp itself rather than a transcription of it.
//
// What is asserted, for EVERY Q21S frequency (13) against EVERY Q212P
// frequency (6), and for the BEM 2inch catalogue (11) the same way:
//
//   1. Separability. The intensity of a mixed scene equals the sum of the
//      intensities of the same speakers solved one model at a time. Different
//      frequencies cannot interfere, so their energies add - and if that
//      identity holds, each model's contribution in the mixed scene is exactly
//      what it is on its own, whatever the other model is tuned to.
//   2. Invariance. A Q212P's contribution, recovered from the mixed scene by
//      subtraction, is identical across all 13 Q21S frequencies (and the
//      reverse).
//   3. Interference is NOT lost. Two units of the same model at the same
//      frequency must still interfere - separability must fail there, or the
//      fix would have flattened the physics it is meant to preserve.
//   4. The pattern each unit uses is its own model's table at its own
//      frequency, probed through a marker gain planted in the tables.

#include "AcousticEngine.h"

#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace
{
int    checks = 0;
int    failures = 0;
double worstRel = 0.0;

void check (bool ok, const std::string& what)
{
    ++checks;
    if (! ok) { ++failures; std::printf ("  FAIL: %s\n", what.c_str()); }
}

// For quantities recovered by SUBTRACTING two numbers of similar size, where
// the relative error of the result is the relative error of the inputs blown
// up by how much cancelled. The bound is therefore set on the size of the
// inputs, not of the answer.
void checkCloseAbs (double a, double b, double tolAbs, const std::string& what)
{
    ++checks;
    if (! (std::abs (a - b) <= tolAbs))
    {
        ++failures;
        std::printf ("  FAIL: %s  (%.17g vs %.17g, abs %.3g > %.3g)\n",
                     what.c_str(), a, b, std::abs (a - b), tolAbs);
    }
}

void checkClose (double a, double b, double tolRel, const std::string& what)
{
    ++checks;
    const double scale = std::max (std::max (std::abs (a), std::abs (b)), 1e-300);
    const double rel   = std::abs (a - b) / scale;
    worstRel = std::max (worstRel, rel);
    if (! (rel <= tolRel))
    {
        ++failures;
        std::printf ("  FAIL: %s  (%.17g vs %.17g, rel %.3g)\n",
                     what.c_str(), a, b, rel);
    }
}

// A directivity table with a recognisable shape: a cardioid-ish front lobe
// scaled by a per-frequency marker, so a pattern taken from the wrong
// frequency - or the wrong model - shows up as a level error, not a near-miss.
DirectivityPattern makeTable (int hz, double marker, bool absolute)
{
    DirectivityPattern t;
    t.hz = hz;
    t.gain.resize (360);
    for (int d = 0; d < 360; ++d)
    {
        const double th = d * M_PI / 180.0;
        t.gain[(size_t) d] = (float) (marker * (0.35 + 0.65 * (0.5 * (1.0 + std::cos (th)))));
    }
    t.onAxisSplDb  = 100.0f;
    t.refDistanceM = 2.0f;
    t.hasAbsolute  = absolute;
    t.ok           = true;
    return t;
}

// Marker value for a model/frequency pair - distinct for every combination.
double markerFor (int model, double hz) { return 1.0 + 0.001 * model + 1e-6 * hz; }

void fillTables (SimParams& p)
{
    p.directivity.clear();
    p.directivityBEM2inch.clear();
    p.directivityQ212P.clear();
    for (int i = 0; i < kNumQ21SFrequencies; ++i)
        p.directivity.push_back (makeTable ((int) kQ21SFrequencies[i],
                                            markerFor (0, kQ21SFrequencies[i]), false));
    for (int i = 0; i < kNumBEM2inchFrequencies; ++i)
        p.directivityBEM2inch.push_back (makeTable ((int) kBEM2inchFrequencies[i],
                                                    markerFor (2, kBEM2inchFrequencies[i]), false));
    for (int i = 0; i < kNumQ212PFrequencies; ++i)
        p.directivityQ212P.push_back (makeTable ((int) kQ212PFrequencies[i],
                                                 markerFor (3, kQ212PFrequencies[i]), false));
}

SimParams baseScene()
{
    SimParams p;
    p.worldX0 = 0.0; p.worldY0 = 0.0;
    p.worldW  = 40.0; p.worldH = 40.0;
    p.resolution = 64;
    p.octaveSmoothing = true;
    p.viewMode = ViewMode::SPL;
    fillTables (p);
    return p;
}

Speaker unitAt (double x, double y, int model, double rotDeg = 0.0,
                double gainDB = 0.0, double delayMs = 0.0)
{
    Speaker s;
    s.x = (float) x; s.y = (float) y;
    s.model = model;
    s.rotationDeg = (float) rotDeg;
    s.gainDB = (float) gainDB;
    s.delayMs = (float) delayMs;
    s.enabled = true;
    return s;
}

// Un-normalised intensity at a point, in linear power. sampleIntensityAt
// returns 10*log10(I), which is exactly what the probe and the mic read, and
// unlike the heatmap it is not divided by a scene-wide peak.
double intensityAt (const SimParams& p, double x, double y)
{
    float i = 0.0f, a = 0.0f;
    if (! AcousticEngine::sampleIntensityAt (p, (float) x, (float) y, i, a))
        return 0.0;
    return std::pow (10.0, i / 10.0);
}

const double probes[][2] = {
    { 12.0,  9.0 }, { 20.0, 20.0 }, { 31.5, 14.25 }, { 6.0, 27.0 }, { 25.0,  5.0 }
};
constexpr int nProbes = (int) (sizeof (probes) / sizeof (probes[0]));

void catalogueFor (int model, const double*& f, int& n)
{
    frequencyCatalogue (model, f, n);
}
} // namespace

int main()
{
    std::printf ("Frequency isolation verification\n");
    std::printf ("================================\n\n");

    // ---------------------------------------------------------------- 1 & 2
    // Separability and invariance, over every frequency pair of every model
    // pair. otherModel is swept too, so Q21S-vs-Q212P, Q21S-vs-2inch and
    // 2inch-vs-Q212P are all covered.
    const int modelPairs[][2] = { { 0, 3 }, { 0, 2 }, { 2, 3 } };

    for (const auto& pair : modelPairs)
    {
        const int mA = pair[0], mB = pair[1];
        const double* fA; int nA; catalogueFor (mA, fA, nA);
        const double* fB; int nB; catalogueFor (mB, fB, nB);
        std::printf ("Models %d vs %d: %d x %d frequency pairs\n", mA, mB, nA, nB);

        // Per B-frequency, the B-only intensity recovered from each of the
        // A-frequency scenes must be identical.
        for (int jb = 0; jb < nB; ++jb)
        {
            std::vector<std::vector<double>> recovered, scales;

            for (int ja = 0; ja < nA; ++ja)
            {
                SimParams both = baseScene();
                both.frequencyQ21S     = (mA == 0) ? fA[ja] : ((mB == 0) ? fB[jb] : 20.0);
                both.frequencyBEM2inch = (mA == 2) ? fA[ja] : ((mB == 2) ? fB[jb] : 64.0);
                both.frequencyQ212P    = (mA == 3) ? fA[ja] : ((mB == 3) ? fB[jb] : 484.0);
                // The scene frequency is deliberately set to something that
                // belongs to NEITHER unit: if it still reached the propagation
                // maths, every number below would move.
                both.frequency = 1234.0;
                both.speakers.push_back (unitAt (14.0, 18.0, mA, 0.0, -2.0, 1.5));
                both.speakers.push_back (unitAt (26.0, 22.0, mB, 35.0, -5.0, 0.0));

                SimParams onlyA = both; onlyA.speakers = { both.speakers[0] };
                SimParams onlyB = both; onlyB.speakers = { both.speakers[1] };

                std::vector<double> bHere, scaleHere;
                for (int q = 0; q < nProbes; ++q)
                {
                    const double x = probes[q][0], y = probes[q][1];
                    const double iBoth = intensityAt (both,  x, y);
                    const double iA    = intensityAt (onlyA, x, y);
                    const double iB    = intensityAt (onlyB, x, y);

                    // (1) energies add across frequencies
                    // sampleIntensityAt hands back a float dB, so the
                    // round trip through it carries ~1e-7 relative precision.
                    // Anything at that level is the float, not a coupling:
                    // the bug being fixed moved these numbers by whole dB.
                    checkClose (iBoth, iA + iB, 1e-5,
                                "separability m" + std::to_string (mA) + "@"
                                + std::to_string ((long long) fA[ja]) + " + m"
                                + std::to_string (mB) + "@"
                                + std::to_string ((long long) fB[jb]));

                    bHere.push_back (iBoth - iA);   // B's share of the mixed scene
                    scaleHere.push_back (iBoth);    // what the subtraction came from
                }
                recovered.push_back (bHere);
                scales.push_back (scaleHere);
            }

            // (2) B's share does not depend on A's frequency. B's share is a
            // difference of two float dB readings, so the bound is 1e-6 of
            // the mixed intensity it was taken out of - about eight times the
            // float's own resolution, and still at least four orders of
            // magnitude below the coupling this is testing for.
            for (size_t ja = 1; ja < recovered.size(); ++ja)
                for (int q = 0; q < nProbes; ++q)
                    checkCloseAbs (recovered[ja][(size_t) q], recovered[0][(size_t) q],
                                1e-6 * std::max (scales[ja][(size_t) q],
                                                 scales[0][(size_t) q]),
                                "invariance of m" + std::to_string (mB) + "@"
                                + std::to_string ((long long) fB[jb])
                                + " across m" + std::to_string (mA) + " frequencies");
        }
    }

    // -------------------------------------------------------------------- 3
    // Same model, same frequency: interference MUST survive. Two units half a
    // wavelength apart cannot have their energies simply add.
    {
        std::printf ("\nSame-frequency interference preserved\n");
        const double* f; int n; catalogueFor (0, f, n);
        int interfering = 0;
        for (int j = 0; j < n; ++j)
        {
            SimParams both = baseScene();
            both.frequencyQ21S = f[j];
            both.frequency     = f[j];
            const double lambda = 343.0 / f[j];
            both.speakers.push_back (unitAt (20.0, 20.0, 0));
            both.speakers.push_back (unitAt (20.0 + 0.5 * lambda, 20.0, 0));

            SimParams a = both; a.speakers = { both.speakers[0] };
            SimParams b = both; b.speakers = { both.speakers[1] };

            bool anyDifferent = false;
            for (int q = 0; q < nProbes; ++q)
            {
                const double x = probes[q][0], y = probes[q][1];
                const double iBoth = intensityAt (both, x, y);
                const double sum   = intensityAt (a, x, y) + intensityAt (b, x, y);
                const double rel   = std::abs (iBoth - sum) / std::max (sum, 1e-300);
                if (rel > 1e-3) anyDifferent = true;
            }
            if (anyDifferent) ++interfering;
            check (anyDifferent, "two Q21S at " + std::to_string ((long long) f[j])
                                 + " Hz still interfere");
        }
        std::printf ("  %d of %d Q21S frequencies interfere as before\n", interfering, n);
    }

    // -------------------------------------------------------------------- 4
    // Each unit reads its OWN model's table at its OWN frequency. The marker
    // gain scales amplitude, so intensity scales as marker^2: a unit solved
    // alone must land on its own marker, never the other model's.
    {
        std::printf ("\nPattern provenance (marker probe)\n");
        const int models[] = { 0, 2, 3 };
        for (int model : models)
        {
            const double* f; int n; catalogueFor (model, f, n);
            for (int j = 0; j < n; ++j)
            {
                SimParams p = baseScene();
                p.octaveSmoothing = false;      // one band: the marker is exact
                p.frequency = 999.0;            // again, belongs to no unit
                p.frequencyQ21S     = (model == 0) ? f[j] : 20.0;
                p.frequencyBEM2inch = (model == 2) ? f[j] : 64.0;
                p.frequencyQ212P    = (model == 3) ? f[j] : 484.0;
                p.speakers.push_back (unitAt (20.0, 20.0, model));

                // Flat tables would hide a wrong pick, so compare against the
                // same solve with this model's marker set to 1 everywhere.
                SimParams ref = p;
                for (auto& t : (model == 0 ? ref.directivity
                                           : model == 2 ? ref.directivityBEM2inch
                                                        : ref.directivityQ212P))
                    if (t.hz == (int) f[j])
                        for (auto& g : t.gain) g /= (float) markerFor (model, f[j]);

                const double iMark = intensityAt (p,   24.0, 20.0);
                const double iRef  = intensityAt (ref, 24.0, 20.0);
                const double m     = markerFor (model, f[j]);
                checkClose (iMark, iRef * m * m, 1e-5,
                            "model " + std::to_string (model) + " uses its own "
                            + std::to_string ((long long) f[j]) + " Hz table");
            }
        }
    }

    // -------------------------------------------------------------------- 5
    // The full heatmap path (compute(), not the point probe) must separate the
    // same way. splAbsDB falls back to the relative field here, so compare the
    // unfloored relative grids after undoing the scene-wide normalisation:
    // the shape of each model's own region must not move when the other model
    // is retuned.
    {
        std::printf ("\nHeatmap path (compute)\n");
        const double* fq; int nq; catalogueFor (3, fq, nq);
        const double* fs; int ns; catalogueFor (0, fs, ns);

        for (int jq = 0; jq < nq; ++jq)
        {
            std::vector<float> firstDelta;
            for (int js = 0; js < ns; ++js)
            {
                SimParams p = baseScene();
                p.frequency      = 777.0;
                p.frequencyQ21S  = fs[js];
                p.frequencyQ212P = fq[jq];
                p.speakers.push_back (unitAt (14.0, 18.0, 0, 0.0, -2.0, 1.5));
                p.speakers.push_back (unitAt (26.0, 22.0, 3, 35.0, -5.0, 0.0));

                SimParams onlyS = p; onlyS.speakers = { p.speakers[0] };
                SimParams onlyQ = p; onlyQ.speakers = { p.speakers[1] };

                const SimResult rBoth = AcousticEngine::compute (p);
                const SimResult rS    = AcousticEngine::compute (onlyS);
                const SimResult rQ    = AcousticEngine::compute (onlyQ);

                // Relative dB is normalised per solve, so work in the ratio of
                // each cell to its own solve's peak and reconstruct linear
                // power: Iboth/peakBoth = (Is + Iq)/peakBoth. The scene-wide
                // peak differs between solves, so compare the SHAPE: the ratio
                // of the mixed field to the Q212P-only field, cell by cell, in
                // the half of the world nearer the Q212P.
                std::vector<float> delta;
                for (int row = 0; row < rBoth.height; row += 7)
                    for (int col = 0; col < rBoth.width; col += 7)
                    {
                        const size_t i = (size_t) row * rBoth.width + col;
                        const double pb = std::pow (10.0, rBoth.splRelDB[i] / 10.0);
                        const double ps = std::pow (10.0, rS.splRelDB[i] / 10.0);
                        const double pq = std::pow (10.0, rQ.splRelDB[i] / 10.0);
                        // Both fields are normalised to their own unity peak,
                        // and the unity peak of a scene is itself the sum of
                        // the two groups' unity peaks - so the only identity
                        // that survives normalisation is proportionality.
                        // Record the Q212P share of the mixed field.
                        delta.push_back ((float) (pq / std::max (ps + pq, 1e-300)));
                        (void) pb;
                    }

                if (js == 0) firstDelta = delta;
                else
                    for (size_t i = 0; i < delta.size(); ++i)
                        if (std::abs (delta[i] - firstDelta[i]) > 1e-6)
                        {
                            // Expected: the share DOES move, because the Q21S
                            // field it is being shared against changed. What
                            // must not move is the Q212P-only field itself.
                        }

                // The real invariant: the Q212P-only solve must be identical
                // for every Q21S frequency, since nothing about it depends on
                // the Q21S any more.
                static std::vector<float> qRef;
                static int qRefBand = -1;
                if (qRefBand != jq) { qRef = rQ.splRelDB; qRefBand = jq; }
                else
                    for (size_t i = 0; i < qRef.size(); ++i)
                        checkClose (rQ.splRelDB[i], qRef[i], 0.0,
                                    "Q212P-only heatmap unchanged by Q21S frequency");

                // And the mixed heatmap must equal the power sum of the two
                // solo fields, up to the one scene-wide normalisation factor.
                // Each solve normalises its own grid to its own unity peak,
                // so the mixed field is not a single multiple of the sum of
                // the solo fields - it is a fixed LINEAR COMBINATION of them,
                // one coefficient per solo normaliser:
                //     p_both = a * p_q21s + b * p_q212p
                // with a and b the same in every cell. Separability is
                // exactly the claim that two such constants exist, so they
                // are fitted by least squares over the sampled cells and the
                // residual is what gets checked. (Trying to force a single
                // ratio here is what made this read as a failure at first -
                // the test's algebra, not the engine's.)
                double s11 = 0, s12 = 0, s22 = 0, sy1 = 0, sy2 = 0;
                std::vector<double> sb, ss, sq;
                for (int row = 0; row < rBoth.height; row += 5)
                    for (int col = 0; col < rBoth.width; col += 5)
                    {
                        const size_t i = (size_t) row * rBoth.width + col;
                        if (rBoth.splRelDB[i] < -120.0f || rS.splRelDB[i] < -120.0f
                            || rQ.splRelDB[i] < -120.0f) continue;
                        const double pb = std::pow (10.0, rBoth.splRelDB[i] / 10.0);
                        const double ps = std::pow (10.0, rS.splRelDB[i] / 10.0);
                        const double pq = std::pow (10.0, rQ.splRelDB[i] / 10.0);
                        sb.push_back (pb); ss.push_back (ps); sq.push_back (pq);
                        s11 += ps * ps; s12 += ps * pq; s22 += pq * pq;
                        sy1 += pb * ps; sy2 += pb * pq;
                    }
                const double det = s11 * s22 - s12 * s12;
                double worstResid = 0.0;
                if (std::abs (det) > 1e-300 && ! sb.empty())
                {
                    const double ca = (sy1 * s22 - sy2 * s12) / det;
                    const double cb = (sy2 * s11 - sy1 * s12) / det;
                    for (size_t i = 0; i < sb.size(); ++i)
                    {
                        const double fit = ca * ss[i] + cb * sq[i];
                        worstResid = std::max (worstResid,
                            std::abs (fit - sb[i]) / std::max (sb[i], 1e-300));
                    }
                }
                check (worstResid <= 1e-3,
                       "mixed heatmap is a fixed linear combination of the two solo "
                       "fields (Q21S @" + std::to_string ((long long) fs[js])
                       + ", Q212P @" + std::to_string ((long long) fq[jq])
                       + "), worst residual " + std::to_string (worstResid));
            }
        }
    }

    // -------------------------------------------------------------------- 6
    // What the user actually sees. In a mixed scene, the DISPLAYED heatmap
    // over one device's own ground must not move when the other device is
    // retuned. This is stricter than separability: the relative map divides
    // by a peak, and a scene-wide peak made retuning the Q212P shift the
    // Q21S's colours by about 3 dB although its field never moved. Cells
    // where the other model is more than 40 dB down carry under 0.01% of its
    // power, so anything visible there is the normaliser, not physics.
    {
        std::printf ("\nDisplayed heatmap over one device, while the other is retuned\n");
        const double* fq; int nq; catalogueFor (3, fq, nq);
        const double* fs; int ns; catalogueFor (0, fs, ns);

        auto sweep = [&] (bool sweepTop) -> double
        {
            const int nOuter = sweepTop ? ns : nq;
            const int nInner = sweepTop ? nq : ns;
            double worst = 0.0;
            for (int o = 0; o < nOuter; ++o)
            {
                std::vector<float> ref;
                std::vector<char>  owned;
                for (int k = 0; k < nInner; ++k)
                {
                    SimParams p = baseScene();
                    p.frequency      = 777.0;
                    p.frequencyQ21S  = sweepTop ? fs[o] : fs[k];
                    p.frequencyQ212P = sweepTop ? fq[k] : fq[o];
                    p.speakers.push_back (unitAt (14.0, 18.0, 0, 0.0, -2.0, 1.5));
                    p.speakers.push_back (unitAt (26.0, 22.0, 3, 35.0, -5.0, 0.0));

                    const SimResult rBoth = AcousticEngine::compute (p);

                    if (k == 0)
                    {
                        SimParams onlyS = p; onlyS.speakers = { p.speakers[0] };
                        SimParams onlyQ = p; onlyQ.speakers = { p.speakers[1] };
                        const SimResult rS = AcousticEngine::compute (onlyS);
                        const SimResult rQ = AcousticEngine::compute (onlyQ);
                        ref = rBoth.splRelDB;
                        owned.assign (ref.size(), 0);
                        for (size_t i = 0; i < ref.size(); ++i)
                            owned[i] = sweepTop
                                ? (rQ.splRelDB[i] < rS.splRelDB[i] - 40.0f ? 1 : 0)
                                : (rS.splRelDB[i] < rQ.splRelDB[i] - 40.0f ? 1 : 0);
                        continue;
                    }

                    for (size_t i = 0; i < ref.size(); ++i)
                    {
                        if (! owned[i]) continue;
                        worst = std::max (worst, (double) std::abs (rBoth.splRelDB[i] - ref[i]));
                        checkCloseAbs (rBoth.splRelDB[i], ref[i], 0.01,
                                       sweepTop ? "Q21S's own cells unmoved while the Q212P is retuned"
                                                : "Q212P's own cells unmoved while the Q21S is retuned");
                    }
                }
            }
            return worst;
        };

        std::printf ("  worst shift over the Q21S's own cells:  %.4f dB\n", sweep (true));
        std::printf ("  worst shift over the Q212P's own cells: %.4f dB\n", sweep (false));
    }

    std::printf ("\n--------------------------------------------\n");
    std::printf ("checks: %d   failures: %d   worst relative difference: %.3g\n",
                 checks, failures, worstRel);
    return failures == 0 ? 0 : 1;
}

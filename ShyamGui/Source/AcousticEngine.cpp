#include "AcousticEngine.h"
#include <cmath>
#include <algorithm>
#include <thread>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Active frequency catalogue - defaults to Q21S; setActiveFrequencyCatalogue()
// repoints both on a measurement-source switch (see AcousticEngine.h).
const double* kSupportedFrequencies    = kQ21SFrequencies;
int           kNumSupportedFrequencies = kNumQ21SFrequencies;

static constexpr double kSpeedOfSound = 343.0;
static constexpr double kPistonRadius = 0.13;
static constexpr double kOrientationBias = 0.25;

static double modelDirectivity (double facing, double angleToPoint, double k)
{
    const double rel = angleToPoint - facing;
    const double ka  = k * kPistonRadius;
    const double x   = ka * std::sin (rel);
    double D = (std::abs (x) < 1.0e-9) ? 1.0 : 2.0 * ::j1 (x) / x;

    double w = std::fmod (rel + M_PI, 2.0 * M_PI);
    if (w < 0.0) w += 2.0 * M_PI;
    w -= M_PI;
    const double sigma = std::max (1.0, ka);
    const double taper = 1.0 / (1.0 + std::exp (sigma * (std::abs (w) - M_PI / 2.0)));
    return std::abs (D) * taper;
}

double AcousticEngine::orientationGain (double facingAngle, double angleToPoint)
{
    const double delta = angleToPoint - facingAngle;
    return (1.0 - kOrientationBias) + kOrientationBias * std::cos (delta);
}

static const DirectivityPattern* pickPattern (const std::vector<DirectivityPattern>& tables,
                                              double f)
{
    const DirectivityPattern* pat = nullptr;
    const int fi = (int) std::lround (f);
    int bestDist = 100000;
    for (const auto& d : tables)
    {
        if (! d.ok || d.gain.size() < 360) continue;
        const int dist = std::abs (d.hz - fi);
        if (dist < bestDist) { bestDist = dist; pat = &d; }
        if (dist == 0) break;
    }
    return pat;
}

static double dirFactorRaw (const DirectivityPattern* pat, double k,
                            double facing, double theta)
{
    if (pat == nullptr)
        return modelDirectivity (facing, theta, k);

    double deg = (theta - facing) * 180.0 / M_PI;
    deg = std::fmod (deg, 360.0);
    if (deg < 0.0) deg += 360.0;
    const int    d0   = (int) std::floor (deg) % 360;
    const int    d1   = (d0 + 1) % 360;
    const double frac = deg - std::floor (deg);
    return (1.0 - frac) * pat->gain[(size_t) d0] + frac * pat->gain[(size_t) d1];
}

static double dirFactor (const DirectivityPattern* pat, double k,
                         double facing, double theta)
{
    constexpr double eps = 0.5 * M_PI / 180.0;
    return (dirFactorRaw (pat, k, facing, theta - eps)
          + dirFactorRaw (pat, k, facing, theta)
          + dirFactorRaw (pat, k, facing, theta + eps)) / 3.0;
}

// Table to search for a given speaker's own model - never blends devices.
static const std::vector<DirectivityPattern>& tableFor (const SimParams& p, int model)
{
    if (model == 2) return p.directivityBEM2inch;
    if (model == 3) return p.directivityQ212P;
    return p.directivity;
}

// This speaker's OWN model's currently-selected frequency (never the other
// model's, never the shared/displayed p.frequency when it belongs to a
// different model) - see SimParams::frequencyQ21S / frequencyBEM2inch. This is
// what isolates directivity pattern selection per device: changing one
// model's frequency cannot change what this returns for the other model.
static double ownFrequency (const SimParams& p, int model)
{
    if (model == 2) return std::max (1.0, p.frequencyBEM2inch);
    if (model == 3) return std::max (1.0, p.frequencyQ212P);
    return std::max (1.0, p.frequencyQ21S);
}

SimResult AcousticEngine::compute (const SimParams& p)
{
    SimResult res;

    const int N = std::max (16, p.resolution);
    res.width   = N;
    res.height  = N;
    res.worldX0 = p.worldX0;
    res.worldY0 = p.worldY0;
    res.worldW  = p.worldW;
    res.worldH  = p.worldH;
    res.usedBemField = false;

    // The SCENE frequency: what the reports, the banner and the wavelength
    // read-out quote, and the fallback when no speaker is placed. It no longer
    // propagates anything - each speaker carries its own k and omega (see Src
    // below), which is what keeps one device's dropdown off the other's field.
    const double f      = std::max (1.0, p.frequency);
    const double lambda = kSpeedOfSound / f;
    const double k      = 2.0 * M_PI / lambda;

    res.frequency      = f;
    res.lambda         = lambda;
    res.k              = k;
    res.minDisplayedDB = p.dBfloor;

    struct Src
    {
        double x, y, gainLin, facing, delaySec, polPhase;
        SpeakerFilter filter;      // applied per smoothing sub-band
        std::complex<double> hCentre { 1.0, 0.0 };  // H at fOwn: pressure, interference, polar
        double halfExtent = (double) Q21SCabinet::halfExtentM;  // own cabinet's 1/r floor
        const DirectivityPattern* pat = nullptr;   // this speaker's own model, at fOwn
        bool   hasAbs   = false;
        double calAbs   = 0.0;    // 10^(onAxisSplDb/20), this speaker's own calibration
        double refDistM = 2.0;
        // Each speaker is PROPAGATED at its own model's frequency, not at the
        // scene's: its own wavelength, its own phase per metre, its own delay
        // phase and its own filter frequency. Picking the pattern per model
        // was never enough on its own - the wavenumber was shared, so moving
        // one device's frequency re-drew the other device's interference.
        double fOwn  = 52.0;
        double kOwn  = 0.0;
        double wOwn  = 0.0;
        // Sources at the SAME frequency interfere and are summed as complex
        // pressure; sources at different frequencies cannot interfere, so
        // their intensities add instead. group indexes freqGroups below.
        int    group = 0;
    };
    std::vector<Src> srcs;
    srcs.reserve (p.speakers.size());
    // Distinct propagation frequencies present in the scene, in first-seen
    // order. One entry is the overwhelmingly common case and reduces exactly
    // to the previous single-frequency solve.
    std::vector<double> freqGroups;
    for (const auto& s : p.speakers)
    {
        if (! s.enabled) continue;
        Src src;
        src.x        = s.x;
        src.y        = s.y;
        src.gainLin  = std::pow (10.0, s.gainDB / 20.0);
        // Free rotation plus the Reverse flag, which still flips it end for end.
        src.facing   = s.rotationDeg * M_PI / 180.0
                     + (s.reverseOrientation ? M_PI : 0.0);
        src.delaySec = s.delayMs * 1.0e-3;
        src.polPhase = s.polarityInverted ? M_PI : 0.0;
        src.halfExtent = (double) cabinetFor (s.model).halfExtentM;
        src.filter = s.filter;

        src.fOwn = ownFrequency (p, s.model);
        src.kOwn = 2.0 * M_PI * src.fOwn / kSpeedOfSound;
        src.wOwn = 2.0 * M_PI * src.fOwn;
        src.hCentre = filterResponse (src.fOwn, s.filter);

        src.group = -1;
        for (size_t g = 0; g < freqGroups.size(); ++g)
            if (std::abs (freqGroups[g] - src.fOwn) < 1.0e-6) { src.group = (int) g; break; }
        if (src.group < 0)
        {
            freqGroups.push_back (src.fOwn);
            src.group = (int) freqGroups.size() - 1;
        }

        const auto& table = tableFor (p, s.model);
        if (! table.empty())
            src.pat = pickPattern (table, src.fOwn);
        if (src.pat != nullptr && src.pat->hasAbsolute)
        {
            src.hasAbs   = true;
            src.calAbs   = std::pow (10.0, (double) src.pat->onAxisSplDb / 20.0);
            src.refDistM = (src.pat->refDistanceM > 0.05f) ? (double) src.pat->refDistanceM : 2.0;
        }
        srcs.push_back (src);
    }
    res.activeSpeakers = (int) srcs.size();

    // Info-panel summary only; a mixed scene may carry several models/patterns
    // at once - this just reports whichever one was found first.
    res.usedMeasuredDirectivity = false;
    res.measuredDirectivityHz   = 0;
    for (const auto& s : srcs)
        if (s.pat != nullptr) { res.usedMeasuredDirectivity = true; res.measuredDirectivityHz = s.pat->hz; break; }

    // Absolute SPL requires every active speaker to carry its own calibrated
    // pattern - a partially-calibrated mixed scene falls back to relative only
    // rather than showing a misleading absolute number.
    bool hasAbs = ! srcs.empty();
    for (const auto& s : srcs) hasAbs = hasAbs && s.hasAbs;

    constexpr int    Mband   = 7;
    const double     halfOct = 1.0 / 6.0;
    const int        mCount  = p.octaveSmoothing ? Mband : 1;
    const int        gCount  = std::max (1, (int) freqGroups.size());
    // 1/3-octave smoothing is applied around EACH group's own centre
    // frequency, so a Q21S at 52 Hz is smoothed over 52 Hz's neighbours and a
    // Q212P at 484 Hz over its own - not both over one shared band.
    std::vector<std::vector<double>> kBand ((size_t) gCount,
                                            std::vector<double> ((size_t) mCount, 0.0));
    std::vector<std::vector<double>> omegaBand ((size_t) gCount,
                                                std::vector<double> ((size_t) mCount, 0.0));
    for (int g = 0; g < gCount; ++g)
    {
        const double fg = freqGroups.empty() ? f : freqGroups[(size_t) g];
        for (int m = 0; m < mCount; ++m)
        {
            const double frac = (mCount == 1) ? 0.0
                              : (2.0 * m / (mCount - 1) - 1.0) * halfOct;
            const double fm = fg * std::pow (2.0, frac);
            kBand[(size_t) g][(size_t) m]     = 2.0 * M_PI * fm / kSpeedOfSound;
            omegaBand[(size_t) g][(size_t) m] = 2.0 * M_PI * fm;
        }
    }

    // Per speaker, per band: that speaker's own model's pattern, band-offset
    // from THAT model's own frequency (never the shared displayed f, and
    // never the other model's) - srcs is already enabled-only, same order.
    std::vector<std::vector<const DirectivityPattern*>> patBand (
        srcs.size(), std::vector<const DirectivityPattern*> ((size_t) mCount, nullptr));
    // The crossover is evaluated at EVERY sub-band, not once at the centre.
    // A filter is steepest exactly where it is interesting, so a single value
    // taken at the middle of a smoothed band comes out about half a dB wrong
    // near the corner - and wrong in a way that quietly disagrees with the
    // transfer function it claims to be.
    // H is taken at the frequency the wave is propagated at - which is now
    // this speaker's OWN band frequency, the same one that sets its km / wm.
    // Taking it at the scene frequency instead left the filter acting on a
    // signal at one frequency while the wave travelled at another, and made
    // one device's crossover respond to the other device's dropdown.
    std::vector<std::vector<std::complex<double>>> hBand (
        srcs.size(), std::vector<std::complex<double>> ((size_t) mCount,
                                                        std::complex<double> (1.0, 0.0)));
    {
        size_t si = 0;
        for (const auto& s : p.speakers)
        {
            if (! s.enabled) continue;
            const auto& table = tableFor (p, s.model);
            const double fOwn = ownFrequency (p, s.model);
            for (int m = 0; m < mCount; ++m)
            {
                const double frac = (mCount == 1) ? 0.0
                                  : (2.0 * m / (mCount - 1) - 1.0) * halfOct;
                const double fmOwn = fOwn * std::pow (2.0, frac);
                if (! table.empty())
                    patBand[si][(size_t) m] = pickPattern (table, fmOwn);
                hBand[si][(size_t) m] = filterResponse (fmOwn, s.filter);
            }
            ++si;
        }
    }

    res.splDB.assign        ((size_t) N * N, (float) p.dBfloor);
    res.splRelDB.assign     ((size_t) N * N, (float) p.dBfloor);
    res.splAbsDB.assign     ((size_t) N * N, 0.0f);
    res.pressure.assign     ((size_t) N * N, 0.0f);
    res.interference.assign ((size_t) N * N, 0.0f);

    const double dx = res.worldW / (N - 1);
    const double dy = res.worldH / (N - 1);

    using cd = std::complex<double>;

    // The field is kept ONE GRID PER FREQUENCY GROUP, not one grid for the
    // scene. The relative heatmap divides by the scene's unity-gain peak, and
    // a single shared peak is itself a coupling: retuning the Q212P moves the
    // scene's loudest point, so every Q21S cell changes colour although its
    // own field never moved. Measured at about 3 dB across the Q21S's half of
    // a plan. Each group is therefore normalised by ITS OWN unity peak and the
    // normalised fields are added, which leaves one device's map untouched by
    // the other's dropdown. With one frequency in the scene the sum has one
    // term and this is exactly the old arithmetic.
    std::vector<std::vector<float>> gI     ((size_t) gCount, std::vector<float> ((size_t) N * N, 0.0f));
    std::vector<std::vector<float>> gPre   ((size_t) gCount, std::vector<float> ((size_t) N * N, 0.0f));
    std::vector<std::vector<float>> gCohA  ((size_t) gCount, std::vector<float> ((size_t) N * N, 0.0f));
    std::vector<std::vector<float>> gIncA  ((size_t) gCount, std::vector<float> ((size_t) N * N, 0.0f));

    // Per-speaker-calibrated absolute intensity. Absolute dB SPL is not
    // normalised against anything, so it needs no grouping - it was never
    // subject to this coupling and stays one grid.
    std::vector<double> IabsCal ((size_t) N * N, 0.0);

    double maxIAbsBand = 0.0;
    std::vector<double> maxIUnityG ((size_t) gCount, 0.0);
    std::vector<double> maxAbsReUnityG ((size_t) gCount, 0.0);

    // Rows are split across cores. Every cell writes only its own index in the
    // group grids, so the only shared state is the running maxima, which each
    // worker accumulates locally and merges once at the end. The scratch
    // vectors are per-worker for the same reason -- they were shared before,
    // which is exactly what would have made this unsafe.
    const int hw = (int) std::thread::hardware_concurrency();
    const int nThreads = std::min (16, std::max (1, hw > 0 ? hw : 1));

    struct Partial
    {
        double maxIAbsBand = 0;
        std::vector<double> maxIUnity, maxAbsReUnity;
    };
    std::vector<Partial> partials ((size_t) nThreads);
    for (auto& pr : partials)
    {
        pr.maxIUnity.assign ((size_t) gCount, 0.0);
        pr.maxAbsReUnity.assign ((size_t) gCount, 0.0);
    }

    auto runRows = [&] (int rowBegin, int rowEnd, Partial& acc)
    {
        std::vector<double> rGeom ((size_t) srcs.size());
        std::vector<double> rSpread ((size_t) srcs.size());
        std::vector<double> theta ((size_t) srcs.size());
        // Per-frequency-group accumulators, per worker for the same reason the
        // distance scratch is: they are written every cell.
        std::vector<std::complex<double>> gCentre ((size_t) gCount);
        std::vector<std::complex<double>> gCentreUnity ((size_t) gCount);
        std::vector<std::complex<double>> gSm ((size_t) gCount);
        std::vector<std::complex<double>> gSmUnity ((size_t) gCount);
        std::vector<std::complex<double>> gSmAbs ((size_t) gCount);
        // This cell's per-group totals, kept apart all the way to the end so
        // each one can be divided by its own peak.
        std::vector<double> cellI ((size_t) gCount);
        std::vector<double> cellIUnity ((size_t) gCount);
        std::vector<double> cellIncoh ((size_t) gCount);

        for (int row = rowBegin; row < rowEnd; ++row)
        {
        const double Y = res.worldY0 + row * dy;
        for (int col = 0; col < N; ++col)
        {
            const double X = res.worldX0 + col * dx;

            for (size_t i = 0; i < srcs.size(); ++i)
            {
                const auto& s = srcs[i];
                const double rg = std::sqrt ((X - s.x) * (X - s.x) + (Y - s.y) * (Y - s.y));
                rGeom[i]   = rg;
                rSpread[i] = std::max (rg, s.halfExtent);
                theta[i]   = std::atan2 (Y - s.y, X - s.x);
            }

            for (int g = 0; g < gCount; ++g)
            {
                gCentre[(size_t) g] = cd (0.0, 0.0);
                gCentreUnity[(size_t) g] = cd (0.0, 0.0);
                cellIncoh[(size_t) g] = 0.0;
            }

            for (size_t i = 0; i < srcs.size(); ++i)
            {
                const auto& s = srcs[i];
                const double D = dirFactor (s.pat, s.kOwn, s.facing, theta[i]);
                const double ampBase = D / rSpread[i];
                const double amp = s.gainLin * ampBase * std::abs (s.hCentre);
                const double phase = -(s.kOwn * rGeom[i] + s.wOwn * s.delaySec) + s.polPhase
                                   + std::arg (s.hCentre);
                // The instantaneous waveform, the coherence and the
                // incoherent reference are all gathered PER GROUP: each one
                // is normalised by its own group's peak further down, and a
                // coherence ratio only means anything within one frequency
                // anyway.
                gCentre[(size_t) s.group] += std::polar (amp, phase);
                gCentreUnity[(size_t) s.group] += std::polar (ampBase, phase);
                cellIncoh[(size_t) s.group] += amp;
            }

            for (int g = 0; g < gCount; ++g)
            { cellI[(size_t) g] = 0.0; cellIUnity[(size_t) g] = 0.0; }
            double IbandAbs = 0.0;
            for (int m = 0; m < mCount; ++m)
            {
                for (int g = 0; g < gCount; ++g)
                {
                    gSm[(size_t) g] = cd (0.0, 0.0);
                    gSmUnity[(size_t) g] = cd (0.0, 0.0);
                    gSmAbs[(size_t) g] = cd (0.0, 0.0);
                }

                for (size_t i = 0; i < srcs.size(); ++i)
                {
                    const auto& s = srcs[i];
                    // This speaker's own band wavenumber - its group's, taken
                    // around its own model's frequency.
                    const double km = kBand[(size_t) s.group][(size_t) m];
                    const double wm = omegaBand[(size_t) s.group][(size_t) m];
                    const DirectivityPattern* pm = patBand[i][(size_t) m];
                    const double D = dirFactor (pm, km, s.facing, theta[i]);
                    const double ampBase = D / rSpread[i];
                    const auto&  H  = hBand[i][(size_t) m];
                    const double hm = std::abs (H);
                    const double amp = s.gainLin * ampBase * hm;
                    const double phase = -(km * rGeom[i] + wm * s.delaySec) + s.polPhase
                                       + std::arg (H);
                    gSm[(size_t) s.group] += std::polar (amp, phase);
                    gSmUnity[(size_t) s.group] += std::polar (ampBase, phase);
                    if (hasAbs)
                    {
                        // This speaker's own on-axis calibration (dB @ its own
                        // refDistanceM), carried as a per-speaker linear scale
                        // so mixed-sensitivity devices sum correctly.
                        const double ampAbs = s.gainLin * ampBase * hm * s.calAbs * s.refDistM;
                        gSmAbs[(size_t) s.group] += std::polar (ampAbs, phase);
                    }
                }
                // Within a group, pressures interfere; across groups only the
                // energy adds. With a single frequency present this is the
                // same |sum|^2 as before, to the last bit.
                for (int g = 0; g < gCount; ++g)
                {
                    cellI[(size_t) g]      += std::norm (gSm[(size_t) g]);
                    cellIUnity[(size_t) g] += std::norm (gSmUnity[(size_t) g]);
                    if (hasAbs) IbandAbs += std::norm (gSmAbs[(size_t) g]);
                }
            }
            IbandAbs /= (double) mCount;

            const size_t idx = (size_t) row * N + col;
            for (int g = 0; g < gCount; ++g)
            {
                const size_t gi = (size_t) g;
                cellI[gi]      /= (double) mCount;
                cellIUnity[gi] /= (double) mCount;
                gI[gi][idx]    = (float) cellI[gi];
                gPre[gi][idx]  = (float) gCentre[gi].real();
                gCohA[gi][idx] = (float) std::abs (gCentre[gi]);
                gIncA[gi][idx] = (float) cellIncoh[gi];
                acc.maxIUnity[gi] = std::max (acc.maxIUnity[gi], cellIUnity[gi]);
                acc.maxAbsReUnity[gi] = std::max (acc.maxAbsReUnity[gi],
                                                  std::abs (gCentreUnity[gi].real()));
            }
            IabsCal[idx] = IbandAbs;
            acc.maxIAbsBand = std::max (acc.maxIAbsBand, IbandAbs);
        }
        }
    };

    if (nThreads <= 1)
    {
        runRows (0, N, partials[0]);
    }
    else
    {
        std::vector<std::thread> workers;
        workers.reserve ((size_t) nThreads - 1);
        const int chunk = (N + nThreads - 1) / nThreads;
        for (int t = 1; t < nThreads; ++t)
        {
            const int b = std::min (N, t * chunk);
            const int e = std::min (N, b + chunk);
            if (b >= e) break;
            workers.emplace_back ([&, b, e, t] { runRows (b, e, partials[(size_t) t]); });
        }
        runRows (0, std::min (N, chunk), partials[0]);   // this thread takes the first chunk
        for (auto& w : workers) w.join();
    }

    for (const auto& pr : partials)
    {
        maxIAbsBand = std::max (maxIAbsBand, pr.maxIAbsBand);
        for (int g = 0; g < gCount; ++g)
        {
            maxIUnityG[(size_t) g] = std::max (maxIUnityG[(size_t) g],
                                               pr.maxIUnity[(size_t) g]);
            maxAbsReUnityG[(size_t) g] = std::max (maxAbsReUnityG[(size_t) g],
                                                   pr.maxAbsReUnity[(size_t) g]);
        }
    }

    for (int g = 0; g < gCount; ++g)
    {
        if (maxIUnityG[(size_t) g] < 1e-300)     maxIUnityG[(size_t) g] = 1.0;
        if (maxAbsReUnityG[(size_t) g] < 1e-300) maxAbsReUnityG[(size_t) g] = 1.0;
    }

    const double floorDB = p.dBfloor;

    // Each speaker's absolute amplitude already carries its own onAxisSplDb/
    // refDistanceM calibration (see ampAbs above), so the summed intensity
    // converts to dB directly - no separate reference-intensity division
    // needed (this is algebraically identical to the previous single-model
    // onAxisAbs + 10*log10(I/Iref) formula when every speaker shares one
    // model, and generalises correctly when they don't).
    const double peakAbs = hasAbs
        ? (10.0 * std::log10 (std::max (maxIAbsBand, 1e-300)))
        : 0.0;

    const size_t nCells = (size_t) N * (size_t) N;
    for (size_t i = 0; i < nCells; ++i)
    {
        // Each group normalised to its OWN unity-gain peak, then added. Gain,
        // delay and filter changes stay visible exactly as before, because
        // the peak each group is divided by is a unity-gain peak - it carries
        // no per-speaker gain. What it no longer carries is the other
        // device's frequency.
        double relPower = 0.0;
        double pre = 0.0;
        double cohAmp = 0.0, incAmp = 0.0;
        for (int g = 0; g < gCount; ++g)
        {
            const size_t gi = (size_t) g;
            relPower += (double) gI[gi][i] / maxIUnityG[gi];
            pre      += (double) gPre[gi][i] / maxAbsReUnityG[gi];
            // Coherence is a ratio, so numerator and denominator take the
            // same per-group scale and it cancels within a group; across
            // groups the scale is what stops one weighting the other.
            const double amp = std::sqrt (maxIUnityG[gi]);
            cohAmp += (double) gCohA[gi][i] / amp;
            incAmp += (double) gIncA[gi][i] / amp;
        }

        const double rel = 10.0 * std::log10 (std::max (relPower, 1e-300));
        res.splRelDB[i] = (float) rel;
        res.splDB[i]    = (float) std::max (rel, floorDB);

        if (hasAbs)
            res.splAbsDB[i] = (float) (10.0 * std::log10 (std::max (IabsCal[i], 1e-300)));
        else
            res.splAbsDB[i] = (float) rel;

        // Each group's waveform already peaks at 1, so the average keeps the
        // sum inside [-1, 1] and leaves a single-frequency scene untouched.
        res.pressure[i] = (float) (pre / (double) gCount);

        res.interference[i] = (incAmp > 1e-300)
                                ? (float) std::clamp (cohAmp / incAmp, 0.0, 1.0)
                                : 0.0f;
    }
    res.peakAbsDb      = peakAbs;
    res.hasAbsoluteSpl = hasAbs;

    {
        const int nPolar = 720;
        double cx = 0.0, cy = 0.0;
        if (! srcs.empty())
        {
            for (const auto& s : srcs) { cx += s.x; cy += s.y; }
            cx /= (double) srcs.size();
            cy /= (double) srcs.size();
        }
        else
        {
            cx = res.worldW * 0.5;
            cy = res.worldH * 0.5;
        }
        const double rFar = std::max (res.worldW, res.worldH) * 5.0;
        res.polarMag.assign (nPolar, 0.0f);

        // One magnitude per group per angle, so each group can be divided by
        // its own peak before they are added - the polar needs the same
        // treatment as the map, or retuning one device would still re-scale
        // the other's lobe.
        std::vector<std::vector<double>> polarG ((size_t) gCount,
                                                 std::vector<double> ((size_t) nPolar, 0.0));
        std::vector<double> maxMagUnityG ((size_t) gCount, 0.0);
        for (int i = 0; i < nPolar; ++i)
        {
            const double angle = i * (2.0 * M_PI / nPolar);
            const double Xf = cx + rFar * std::cos (angle);
            const double Yf = cy + rFar * std::sin (angle);

            // Grouped exactly as the map is: one lobe pattern per frequency
            // present, added in power, so the Q21S polar does not re-shape
            // when the Q212P is retuned.
            std::vector<cd> sum ((size_t) gCount, cd (0.0, 0.0));
            std::vector<cd> sumUnity ((size_t) gCount, cd (0.0, 0.0));
            for (const auto& s : srcs)
            {
                const double rg = std::sqrt ((Xf - s.x) * (Xf - s.x) + (Yf - s.y) * (Yf - s.y));
                const double rs = std::max (rg, s.halfExtent);
                const double th = std::atan2 (Yf - s.y, Xf - s.x);
                const double D  = dirFactor (s.pat, s.kOwn, s.facing, th);
                const double ampBase = D / rs;
                const double amp = s.gainLin * ampBase * std::abs (s.hCentre);
                const double phase = -(s.kOwn * rg + s.wOwn * s.delaySec) + s.polPhase
                                   + std::arg (s.hCentre);
                sum[(size_t) s.group] += std::polar (amp, phase);
                sumUnity[(size_t) s.group] += std::polar (ampBase, phase);
            }

            for (int g = 0; g < gCount; ++g)
            {
                polarG[(size_t) g][(size_t) i] = std::abs (sum[(size_t) g]);
                maxMagUnityG[(size_t) g] = std::max (maxMagUnityG[(size_t) g],
                                                     std::abs (sumUnity[(size_t) g]));
            }
        }
        for (int i = 0; i < nPolar; ++i)
        {
            double magSq = 0.0;
            for (int g = 0; g < gCount; ++g)
            {
                const double m = maxMagUnityG[(size_t) g];
                if (m <= 1e-300) continue;
                const double v = polarG[(size_t) g][(size_t) i] / m;
                magSq += v * v;
            }
            res.polarMag[i] = (float) std::sqrt (magSq);
        }
    }

    return res;
}

bool AcousticEngine::sampleIntensityAt (const SimParams& p, float x, float y,
                                        float& intensityDb, float& absDb)
{
    intensityDb = 0.0f;
    absDb = 0.0f;

    struct Src
    {
        double x, y, gainLin, facing, delaySec, polPhase;
        SpeakerFilter filter;      // applied per smoothing sub-band
        int    model;
        double halfExtent = (double) Q21SCabinet::halfExtentM;  // own cabinet's 1/r floor
        bool   hasAbs   = false;
        double calAbs   = 0.0;
        double refDistM = 2.0;
        // As in compute(): this speaker's own propagation frequency and the
        // group of same-frequency speakers it may interfere with.
        double fOwn  = 52.0;
        int    group = 0;
    };
    std::vector<Src> srcs;
    std::vector<double> freqGroups;
    for (const auto& s : p.speakers)
    {
        if (! s.enabled) continue;
        Src src;
        src.x = s.x; src.y = s.y;
        src.gainLin = std::pow (10.0, s.gainDB / 20.0);
        src.facing = s.rotationDeg * M_PI / 180.0
                   + (s.reverseOrientation ? M_PI : 0.0);
        src.delaySec = s.delayMs * 1.0e-3;
        src.polPhase = s.polarityInverted ? M_PI : 0.0;
        src.model = s.model;
        src.halfExtent = (double) cabinetFor (s.model).halfExtentM;
        src.filter = s.filter;

        src.fOwn = ownFrequency (p, s.model);
        src.group = -1;
        for (size_t g = 0; g < freqGroups.size(); ++g)
            if (std::abs (freqGroups[g] - src.fOwn) < 1.0e-6) { src.group = (int) g; break; }
        if (src.group < 0)
        {
            freqGroups.push_back (src.fOwn);
            src.group = (int) freqGroups.size() - 1;
        }
        srcs.push_back (src);
    }
    if (srcs.empty()) return false;

    const int gCount = std::max (1, (int) freqGroups.size());

    // hasAbs requires every active speaker's own model to carry a calibrated
    // pattern at ITS OWN frequency - mirrors compute()'s conservative
    // mixed-scene fallback.
    bool hasAbs = true;
    for (auto& s : srcs)
    {
        const auto& table = tableFor (p, s.model);
        const DirectivityPattern* pat = table.empty() ? nullptr
                                       : pickPattern (table, ownFrequency (p, s.model));
        if (pat != nullptr && pat->hasAbsolute)
        {
            s.hasAbs   = true;
            s.calAbs   = std::pow (10.0, (double) pat->onAxisSplDb / 20.0);
            s.refDistM = (pat->refDistanceM > 0.05f) ? (double) pat->refDistanceM : 2.0;
        }
        hasAbs = hasAbs && s.hasAbs;
    }

    constexpr int Mband = 7;
    const double halfOct = 1.0 / 6.0;
    const int mCount = p.octaveSmoothing ? Mband : 1;

    const double X = x, Y = y;
    std::vector<double> rGeom (srcs.size()), rSpread (srcs.size()), theta (srcs.size());
    for (size_t i = 0; i < srcs.size(); ++i)
    {
        const auto& s = srcs[i];
        const double rg = std::sqrt ((X - s.x) * (X - s.x) + (Y - s.y) * (Y - s.y));
        rGeom[i] = rg;
        rSpread[i] = std::max (rg, srcs[i].halfExtent);
        theta[i] = std::atan2 (Y - s.y, X - s.x);
    }

    using cd = std::complex<double>;
    double Iband = 0.0;
    double IbandAbs = 0.0;
    for (int m = 0; m < mCount; ++m)
    {
        const double frac = (mCount == 1) ? 0.0
                          : (2.0 * m / (mCount - 1) - 1.0) * halfOct;

        std::vector<cd> sm ((size_t) gCount, cd (0.0, 0.0));
        std::vector<cd> smAbs ((size_t) gCount, cd (0.0, 0.0));
        for (size_t i = 0; i < srcs.size(); ++i)
        {
            const auto& s = srcs[i];
            const auto& table = tableFor (p, s.model);
            // Everything below is at THIS speaker's own band frequency: its
            // pattern, its wavenumber, its delay phase and its filter.
            const double fmOwn = s.fOwn * std::pow (2.0, frac);
            const double km = 2.0 * M_PI * fmOwn / kSpeedOfSound;
            const double wm = 2.0 * M_PI * fmOwn;
            const DirectivityPattern* pm = table.empty() ? nullptr : pickPattern (table, fmOwn);
            const double D = dirFactor (pm, km, s.facing, theta[i]);
            const double ampBase = D / rSpread[i];
            const auto   H  = filterResponse (fmOwn, s.filter);
            const double hm = std::abs (H);
            const double amp = s.gainLin * ampBase * hm;
            const double phase = -(km * rGeom[i] + wm * s.delaySec) + s.polPhase
                               + std::arg (H);
            sm[(size_t) s.group] += std::polar (amp, phase);
            if (hasAbs)
            {
                const double ampAbs = amp * s.calAbs * s.refDistM;
                smAbs[(size_t) s.group] += std::polar (ampAbs, phase);
            }
        }
        for (int g = 0; g < gCount; ++g)
        {
            Iband += std::norm (sm[(size_t) g]);
            if (hasAbs) IbandAbs += std::norm (smAbs[(size_t) g]);
        }
    }
    Iband /= (double) mCount;
    IbandAbs /= (double) mCount;

    intensityDb = (float) (10.0 * std::log10 (std::max (Iband, 1e-300)));

    if (hasAbs)
        absDb = (float) (10.0 * std::log10 (std::max (IbandAbs, 1e-300)));
    else
        absDb = intensityDb;
    return true;
}

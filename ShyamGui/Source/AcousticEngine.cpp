#include "AcousticEngine.h"
#include <cmath>
#include <algorithm>
#include <thread>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Active frequency catalogue — defaults to Q21S; setActiveFrequencyCatalogue()
// repoints both on a measurement-source switch (see AcousticEngine.h).
const double* kSupportedFrequencies    = kQ21SFrequencies;
int           kNumSupportedFrequencies = kNumQ21SFrequencies;

static constexpr double kSpeedOfSound = 343.0;
static constexpr double kCabHalfW     = (double) Q21SCabinet::halfExtentM;
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

// Table to search for a given speaker's own model — never blends devices.
static const std::vector<DirectivityPattern>& tableFor (const SimParams& p, int model)
{
    return (model == 2) ? p.directivity15W750 : p.directivity;
}

// This speaker's OWN model's currently-selected frequency (never the other
// model's, never the shared/displayed p.frequency when it belongs to a
// different model) — see SimParams::frequencyQ21S / frequency15W750. This is
// what isolates directivity pattern selection per device: changing one
// model's frequency cannot change what this returns for the other model.
static double ownFrequency (const SimParams& p, int model)
{
    return std::max (1.0, (model == 2) ? p.frequency15W750 : p.frequencyQ21S);
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

    const double f      = std::max (1.0, p.frequency);
    const double lambda = kSpeedOfSound / f;
    const double k      = 2.0 * M_PI / lambda;
    const double omega  = 2.0 * M_PI * f;

    res.frequency      = f;
    res.lambda         = lambda;
    res.k              = k;
    res.minDisplayedDB = p.dBfloor;

    struct Src
    {
        double x, y, gainLin, facing, delaySec, polPhase;
        const DirectivityPattern* pat = nullptr;   // this speaker's own model, at f
        bool   hasAbs   = false;
        double calAbs   = 0.0;    // 10^(onAxisSplDb/20), this speaker's own calibration
        double refDistM = 2.0;
    };
    std::vector<Src> srcs;
    srcs.reserve (p.speakers.size());
    for (const auto& s : p.speakers)
    {
        if (! s.enabled) continue;
        Src src;
        src.x        = s.x;
        src.y        = s.y;
        src.gainLin  = std::pow (10.0, s.gainDB / 20.0);
        src.facing   = s.reverseOrientation ? M_PI : 0.0;
        src.delaySec = s.delayMs * 1.0e-3;
        src.polPhase = s.polarityInverted ? M_PI : 0.0;

        const auto& table = tableFor (p, s.model);
        if (! table.empty())
            src.pat = pickPattern (table, ownFrequency (p, s.model));
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
    // at once — this just reports whichever one was found first.
    res.usedMeasuredDirectivity = false;
    res.measuredDirectivityHz   = 0;
    for (const auto& s : srcs)
        if (s.pat != nullptr) { res.usedMeasuredDirectivity = true; res.measuredDirectivityHz = s.pat->hz; break; }

    // Absolute SPL requires every active speaker to carry its own calibrated
    // pattern — a partially-calibrated mixed scene falls back to relative only
    // rather than showing a misleading absolute number.
    bool hasAbs = ! srcs.empty();
    for (const auto& s : srcs) hasAbs = hasAbs && s.hasAbs;

    constexpr int    Mband   = 7;
    const double     halfOct = 1.0 / 6.0;
    const int        mCount  = p.octaveSmoothing ? Mband : 1;
    std::vector<double> kBand ((size_t) mCount), omegaBand ((size_t) mCount);
    for (int m = 0; m < mCount; ++m)
    {
        const double frac = (mCount == 1) ? 0.0
                          : (2.0 * m / (mCount - 1) - 1.0) * halfOct;
        const double fm = f * std::pow (2.0, frac);
        kBand[(size_t) m]     = 2.0 * M_PI * fm / kSpeedOfSound;
        omegaBand[(size_t) m] = 2.0 * M_PI * fm;
    }

    // Per speaker, per band: that speaker's own model's pattern, band-offset
    // from THAT model's own frequency (never the shared displayed f, and
    // never the other model's) — srcs is already enabled-only, same order.
    std::vector<std::vector<const DirectivityPattern*>> patBand (
        srcs.size(), std::vector<const DirectivityPattern*> ((size_t) mCount, nullptr));
    {
        size_t si = 0;
        for (const auto& s : p.speakers)
        {
            if (! s.enabled) continue;
            const auto& table = tableFor (p, s.model);
            const double fOwn = ownFrequency (p, s.model);
            if (! table.empty())
                for (int m = 0; m < mCount; ++m)
                {
                    const double frac = (mCount == 1) ? 0.0
                                      : (2.0 * m / (mCount - 1) - 1.0) * halfOct;
                    patBand[si][(size_t) m] = pickPattern (table, fOwn * std::pow (2.0, frac));
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

    std::vector<cd>     P    ((size_t) N * N);
    std::vector<double> Iavg ((size_t) N * N);
    std::vector<double> Iabs ((size_t) N * N);

    // Per-speaker-calibrated absolute intensity. Like P / Iavg / Iabs this is
    // written once per cell by exactly one worker, so it needs no locking;
    // the rGeom/rSpread/theta scratch is per-worker inside runRows instead.
    std::vector<double> IabsCal ((size_t) N * N, 0.0);

    double maxI = 0.0;
    double maxIUnity = 0.0;
    double maxIAbsBand = 0.0;
    double maxAbsRe = 0.0;
    double maxAbsReUnity = 0.0;

    // Rows are split across cores. Every cell writes only its own index in P /
    // Iavg / Iabs, so the only shared state is the five running maxima, which
    // each worker accumulates locally and merges once at the end. The scratch
    // vectors are per-worker for the same reason -- they were shared before,
    // which is exactly what would have made this unsafe.
    const int hw = (int) std::thread::hardware_concurrency();
    const int nThreads = std::min (16, std::max (1, hw > 0 ? hw : 1));

    struct Partial { double maxI = 0, maxIUnity = 0, maxIAbsBand = 0,
                            maxAbsRe = 0, maxAbsReUnity = 0; };
    std::vector<Partial> partials ((size_t) nThreads);

    auto runRows = [&] (int rowBegin, int rowEnd, Partial& acc)
    {
        std::vector<double> rGeom ((size_t) srcs.size());
        std::vector<double> rSpread ((size_t) srcs.size());
        std::vector<double> theta ((size_t) srcs.size());

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
                rSpread[i] = std::max (rg, kCabHalfW);
                theta[i]   = std::atan2 (Y - s.y, X - s.x);
            }

            cd     centre (0.0, 0.0);
            cd     centreUnity (0.0, 0.0);
            double incoh = 0.0;
            for (size_t i = 0; i < srcs.size(); ++i)
            {
                const auto& s = srcs[i];
                const double D = dirFactor (s.pat, k, s.facing, theta[i]);
                const double ampBase = D / rSpread[i];
                const double amp = s.gainLin * ampBase;
                const double phase = -(k * rGeom[i] + omega * s.delaySec) + s.polPhase;
                centre += std::polar (amp, phase);
                centreUnity += std::polar (ampBase, phase);
                incoh  += amp;
            }

            double Iband = 0.0;
            double IbandUnity = 0.0;
            double IbandAbs = 0.0;
            for (int m = 0; m < mCount; ++m)
            {
                cd sm (0.0, 0.0);
                cd smUnity (0.0, 0.0);
                cd smAbs (0.0, 0.0);
                const double km = kBand[(size_t) m];
                const double wm = omegaBand[(size_t) m];
                for (size_t i = 0; i < srcs.size(); ++i)
                {
                    const auto& s = srcs[i];
                    const DirectivityPattern* pm = patBand[i][(size_t) m];
                    const double D = dirFactor (pm, km, s.facing, theta[i]);
                    const double ampBase = D / rSpread[i];
                    const double amp = s.gainLin * ampBase;
                    const double phase = -(km * rGeom[i] + wm * s.delaySec) + s.polPhase;
                    sm += std::polar (amp, phase);
                    smUnity += std::polar (ampBase, phase);
                    if (hasAbs)
                    {
                        // This speaker's own on-axis calibration (dB @ its own
                        // refDistanceM), carried as a per-speaker linear scale
                        // so mixed-sensitivity devices sum correctly.
                        const double ampAbs = s.gainLin * ampBase * s.calAbs * s.refDistM;
                        smAbs += std::polar (ampAbs, phase);
                    }
                }
                Iband += std::norm (sm);
                IbandUnity += std::norm (smUnity);
                if (hasAbs) IbandAbs += std::norm (smAbs);
            }
            Iband /= (double) mCount;
            IbandUnity /= (double) mCount;
            IbandAbs /= (double) mCount;

            const size_t idx = (size_t) row * N + col;
            P[idx]    = centre;
            Iavg[idx] = Iband;
            Iabs[idx] = incoh;
            IabsCal[idx] = IbandAbs;

            acc.maxI          = std::max (acc.maxI, Iband);
            acc.maxIUnity     = std::max (acc.maxIUnity, IbandUnity);
            acc.maxIAbsBand   = std::max (acc.maxIAbsBand, IbandAbs);
            acc.maxAbsRe      = std::max (acc.maxAbsRe, std::abs (centre.real()));
            acc.maxAbsReUnity = std::max (acc.maxAbsReUnity, std::abs (centreUnity.real()));
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
        maxI          = std::max (maxI, pr.maxI);
        maxIUnity     = std::max (maxIUnity, pr.maxIUnity);
        maxIAbsBand   = std::max (maxIAbsBand, pr.maxIAbsBand);
        maxAbsRe      = std::max (maxAbsRe, pr.maxAbsRe);
        maxAbsReUnity = std::max (maxAbsReUnity, pr.maxAbsReUnity);
    }

    if (maxI < 1e-300)          maxI = 1.0;
    if (maxIUnity < 1e-300)     maxIUnity = 1.0;
    if (maxAbsRe < 1e-300)      maxAbsRe = 1.0;
    if (maxAbsReUnity < 1e-300) maxAbsReUnity = 1.0;

    const double floorDB = p.dBfloor;

    // Each speaker's absolute amplitude already carries its own onAxisSplDb/
    // refDistanceM calibration (see ampAbs above), so the summed intensity
    // converts to dB directly — no separate reference-intensity division
    // needed (this is algebraically identical to the previous single-model
    // onAxisAbs + 10*log10(I/Iref) formula when every speaker shares one
    // model, and generalises correctly when they don't).
    const double peakAbs = hasAbs
        ? (10.0 * std::log10 (std::max (maxIAbsBand, 1e-300)))
        : 0.0;

    for (size_t i = 0; i < P.size(); ++i)
    {
        // Normalise to unity-gain peak so per-speaker gain and level changes are visible.
        const double rel = 10.0 * std::log10 (std::max (Iavg[i], 1e-300) / maxIUnity);
        res.splRelDB[i] = (float) rel;
        res.splDB[i]    = (float) std::max (rel, floorDB);

        if (hasAbs)
            res.splAbsDB[i] = (float) (10.0 * std::log10 (std::max (IabsCal[i], 1e-300)));
        else
            res.splAbsDB[i] = (float) rel;

        res.pressure[i] = (float) (P[i].real() / maxAbsReUnity);

        const double coh = std::abs (P[i]);
        res.interference[i] = (Iabs[i] > 1e-300)
                                ? (float) std::clamp (coh / Iabs[i], 0.0, 1.0)
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

        double maxMagUnity = 0.0;
        for (int i = 0; i < nPolar; ++i)
        {
            const double angle = i * (2.0 * M_PI / nPolar);
            const double Xf = cx + rFar * std::cos (angle);
            const double Yf = cy + rFar * std::sin (angle);

            cd sum (0.0, 0.0);
            cd sumUnity (0.0, 0.0);
            for (const auto& s : srcs)
            {
                const double rg = std::sqrt ((Xf - s.x) * (Xf - s.x) + (Yf - s.y) * (Yf - s.y));
                const double rs = std::max (rg, kCabHalfW);
                const double th = std::atan2 (Yf - s.y, Xf - s.x);
                const double D  = dirFactor (s.pat, k, s.facing, th);
                const double ampBase = D / rs;
                const double amp = s.gainLin * ampBase;
                const double phase = -(k * rg + omega * s.delaySec) + s.polPhase;
                sum += std::polar (amp, phase);
                sumUnity += std::polar (ampBase, phase);
            }

            const double mag = std::abs (sum);
            res.polarMag[i]  = (float) mag;
            maxMagUnity = std::max (maxMagUnity, std::abs (sumUnity));
        }
        if (maxMagUnity > 1e-300)
            for (auto& v : res.polarMag) v = (float) (v / maxMagUnity);
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
        int    model;
        bool   hasAbs   = false;
        double calAbs   = 0.0;
        double refDistM = 2.0;
    };
    std::vector<Src> srcs;
    for (const auto& s : p.speakers)
    {
        if (! s.enabled) continue;
        Src src;
        src.x = s.x; src.y = s.y;
        src.gainLin = std::pow (10.0, s.gainDB / 20.0);
        src.facing = s.reverseOrientation ? M_PI : 0.0;
        src.delaySec = s.delayMs * 1.0e-3;
        src.polPhase = s.polarityInverted ? M_PI : 0.0;
        src.model = s.model;
        srcs.push_back (src);
    }
    if (srcs.empty()) return false;

    const double f = std::max (1.0, p.frequency);

    // hasAbs requires every active speaker's own model to carry a calibrated
    // pattern at ITS OWN frequency — mirrors compute()'s conservative
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
        rSpread[i] = std::max (rg, kCabHalfW);
        theta[i] = std::atan2 (Y - s.y, X - s.x);
    }

    using cd = std::complex<double>;
    double Iband = 0.0;
    double IbandAbs = 0.0;
    for (int m = 0; m < mCount; ++m)
    {
        const double frac = (mCount == 1) ? 0.0
                          : (2.0 * m / (mCount - 1) - 1.0) * halfOct;
        const double fm = f * std::pow (2.0, frac);
        const double km = 2.0 * M_PI * fm / kSpeedOfSound;
        const double wm = 2.0 * M_PI * fm;

        cd sm (0.0, 0.0);
        cd smAbs (0.0, 0.0);
        for (size_t i = 0; i < srcs.size(); ++i)
        {
            const auto& s = srcs[i];
            const auto& table = tableFor (p, s.model);
            const double fmOwn = ownFrequency (p, s.model) * std::pow (2.0, frac);
            const DirectivityPattern* pm = table.empty() ? nullptr : pickPattern (table, fmOwn);
            const double D = dirFactor (pm, km, s.facing, theta[i]);
            const double ampBase = D / rSpread[i];
            const double amp = s.gainLin * ampBase;
            const double phase = -(km * rGeom[i] + wm * s.delaySec) + s.polPhase;
            sm += std::polar (amp, phase);
            if (hasAbs)
            {
                const double ampAbs = amp * s.calAbs * s.refDistM;
                smAbs += std::polar (ampAbs, phase);
            }
        }
        Iband += std::norm (sm);
        if (hasAbs) IbandAbs += std::norm (smAbs);
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

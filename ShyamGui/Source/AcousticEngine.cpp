#include "AcousticEngine.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

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

SimResult AcousticEngine::compute (const SimParams& p)
{
    SimResult res;

    const int N = std::max (16, p.resolution);
    res.width   = N;
    res.height  = N;
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
        srcs.push_back (src);
    }
    res.activeSpeakers = (int) srcs.size();

    const DirectivityPattern* pat = nullptr;
    if (! p.directivity.empty())
        pat = pickPattern (p.directivity, f);

    res.usedMeasuredDirectivity = (pat != nullptr);
    res.measuredDirectivityHz   = pat != nullptr ? pat->hz : 0;

    constexpr int    Mband   = 7;
    const double     halfOct = 1.0 / 6.0;
    const int        mCount  = p.octaveSmoothing ? Mband : 1;
    std::vector<double> kBand ((size_t) mCount), omegaBand ((size_t) mCount);
    std::vector<const DirectivityPattern*> patBand ((size_t) mCount, pat);
    for (int m = 0; m < mCount; ++m)
    {
        const double frac = (mCount == 1) ? 0.0
                          : (2.0 * m / (mCount - 1) - 1.0) * halfOct;
        const double fm = f * std::pow (2.0, frac);
        kBand[(size_t) m]     = 2.0 * M_PI * fm / kSpeedOfSound;
        omegaBand[(size_t) m] = 2.0 * M_PI * fm;
        if (! p.directivity.empty())
            patBand[(size_t) m] = pickPattern (p.directivity, fm);
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

    std::vector<double> rGeom ((size_t) srcs.size());
    std::vector<double> rSpread ((size_t) srcs.size());
    std::vector<double> theta ((size_t) srcs.size());

    double maxI = 0.0;
    double maxIUnity = 0.0;
    double maxAbsRe = 0.0;
    double maxAbsReUnity = 0.0;

    for (int row = 0; row < N; ++row)
    {
        const double Y = row * dy;
        for (int col = 0; col < N; ++col)
        {
            const double X = col * dx;

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
                const double D = dirFactor (pat, k, s.facing, theta[i]);
                const double ampBase = D / rSpread[i];
                const double amp = s.gainLin * ampBase;
                const double phase = -(k * rGeom[i] + omega * s.delaySec) + s.polPhase;
                centre += std::polar (amp, phase);
                centreUnity += std::polar (ampBase, phase);
                incoh  += amp;
            }

            double Iband = 0.0;
            double IbandUnity = 0.0;
            for (int m = 0; m < mCount; ++m)
            {
                cd sm (0.0, 0.0);
                cd smUnity (0.0, 0.0);
                const double km = kBand[(size_t) m];
                const double wm = omegaBand[(size_t) m];
                const DirectivityPattern* pm = patBand[(size_t) m];
                for (size_t i = 0; i < srcs.size(); ++i)
                {
                    const auto& s = srcs[i];
                    const double D = dirFactor (pm, km, s.facing, theta[i]);
                    const double ampBase = D / rSpread[i];
                    const double amp = s.gainLin * ampBase;
                    const double phase = -(km * rGeom[i] + wm * s.delaySec) + s.polPhase;
                    sm += std::polar (amp, phase);
                    smUnity += std::polar (ampBase, phase);
                }
                Iband += std::norm (sm);
                IbandUnity += std::norm (smUnity);
            }
            Iband /= (double) mCount;
            IbandUnity /= (double) mCount;

            const size_t idx = (size_t) row * N + col;
            P[idx]    = centre;
            Iavg[idx] = Iband;
            Iabs[idx] = incoh;

            maxI         = std::max (maxI, Iband);
            maxIUnity    = std::max (maxIUnity, IbandUnity);
            maxAbsRe     = std::max (maxAbsRe, std::abs (centre.real()));
            maxAbsReUnity = std::max (maxAbsReUnity, std::abs (centreUnity.real()));
        }
    }

    if (maxI < 1e-300)          maxI = 1.0;
    if (maxIUnity < 1e-300)     maxIUnity = 1.0;
    if (maxAbsRe < 1e-300)      maxAbsRe = 1.0;
    if (maxAbsReUnity < 1e-300) maxAbsReUnity = 1.0;

    const double floorDB = p.dBfloor;
    const bool hasAbs = (pat != nullptr && pat->hasAbsolute);
    const double Rref = (hasAbs && pat->refDistanceM > 0.05f) ? (double) pat->refDistanceM : 2.0;
    const double Iref = 1.0 / (Rref * Rref);
    const double onAxisAbs = hasAbs ? (double) pat->onAxisSplDb : 0.0;

    const double peakAbs = hasAbs
        ? (onAxisAbs + 10.0 * std::log10 (std::max (maxI, 1e-300) / Iref))
        : 0.0;

    for (size_t i = 0; i < P.size(); ++i)
    {
        // Normalise to unity-gain peak so per-speaker gain and level changes are visible.
        const double rel = 10.0 * std::log10 (std::max (Iavg[i], 1e-300) / maxIUnity);
        res.splRelDB[i] = (float) rel;
        res.splDB[i]    = (float) std::max (rel, floorDB);

        if (hasAbs)
            res.splAbsDB[i] = (float) (onAxisAbs
                                       + 10.0 * std::log10 (std::max (Iavg[i], 1e-300) / Iref));
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
                const double D  = dirFactor (pat, k, s.facing, th);
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
        srcs.push_back (src);
    }
    if (srcs.empty()) return false;

    const double f = std::max (1.0, p.frequency);
    const DirectivityPattern* pat = nullptr;
    if (! p.directivity.empty())
        pat = pickPattern (p.directivity, f);

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
    for (int m = 0; m < mCount; ++m)
    {
        const double frac = (mCount == 1) ? 0.0
                          : (2.0 * m / (mCount - 1) - 1.0) * halfOct;
        const double fm = f * std::pow (2.0, frac);
        const double km = 2.0 * M_PI * fm / kSpeedOfSound;
        const double wm = 2.0 * M_PI * fm;
        const DirectivityPattern* pm = pat;
        if (! p.directivity.empty())
            pm = pickPattern (p.directivity, fm);

        cd sm (0.0, 0.0);
        for (size_t i = 0; i < srcs.size(); ++i)
        {
            const auto& s = srcs[i];
            const double D = dirFactor (pm, km, s.facing, theta[i]);
            const double amp = s.gainLin * D / rSpread[i];
            const double phase = -(km * rGeom[i] + wm * s.delaySec) + s.polPhase;
            sm += std::polar (amp, phase);
        }
        Iband += std::norm (sm);
    }
    Iband /= (double) mCount;

    intensityDb = (float) (10.0 * std::log10 (std::max (Iband, 1e-300)));

    const bool hasAbs = (pat != nullptr && pat->hasAbsolute);
    if (hasAbs)
    {
        const double Rref = pat->refDistanceM > 0.05f ? (double) pat->refDistanceM : 2.0;
        const double Iref = 1.0 / (Rref * Rref);
        absDb = (float) ((double) pat->onAxisSplDb
                         + 10.0 * std::log10 (std::max (Iband, 1e-300) / Iref));
    }
    else
    {
        absDb = intensityDb;
    }
    return true;
}

#include "AcousticEngine.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Speed of sound (Project Config sheet) and a few model constants.
static constexpr double kSpeedOfSound   = 343.0;  // m/s
// 1 m SPL reference distance: the level is referenced to 1 m (standard) and
// clamped inside it, so a source does not produce a singular near-field spike
// that crushes the rest of the map to the noise floor.
static constexpr double kMinDistance    = 1.0;    // m, reference distance
static constexpr double kOrientationBias = 0.25;  // 0 = omni, 1 = strong cardioid
static constexpr double kPistonRadius    = 0.13;  // m, effective driver radius (ka)

// ---------------------------------------------------------------------------
// Frequency-dependent model directivity: a baffled circular piston of radius
// kPistonRadius. D(theta) = 2*J1(ka*sin)/(ka*sin) with a sigmoid rear taper so
// energy rolls off behind the cabinet. ka grows with frequency, so the pattern
// is near-omni at low f and progressively more forward-biased at high f.
// ---------------------------------------------------------------------------
static double modelDirectivity (double facing, double angleToPoint, double k)
{
    const double rel = angleToPoint - facing;          // angle off the forward axis
    const double ka  = k * kPistonRadius;
    const double x   = ka * std::sin (rel);

    // j1 from libm (Apple libc++ lacks std::cyl_bessel_j).
    double D = (std::abs (x) < 1.0e-9) ? 1.0
                                       : 2.0 * ::j1 (x) / x;

    double w = std::fmod (rel + M_PI, 2.0 * M_PI);     // wrap to [-pi, pi]
    if (w < 0.0) w += 2.0 * M_PI;
    w -= M_PI;
    const double sigma = std::max (1.0, ka);
    const double taper = 1.0 / (1.0 + std::exp (sigma * (std::abs (w) - M_PI / 2.0)));

    return std::abs (D) * taper;
}

// ---------------------------------------------------------------------------
// Subtle front/back bias so the Orientation control is meaningful while the
// subwoofer stays essentially omnidirectional. Returns a factor in
// [1 - 2*bias, 1] depending on the angle between the facing direction and the
// direction toward the field point.
// ---------------------------------------------------------------------------
double AcousticEngine::orientationGain (double facingAngle, double angleToPoint)
{
    const double delta = angleToPoint - facingAngle;
    return (1.0 - kOrientationBias) + kOrientationBias * std::cos (delta);
}

// ---------------------------------------------------------------------------
SimResult AcousticEngine::compute (const SimParams& p)
{
    SimResult res;

    const int N = std::max (16, p.resolution);
    res.width   = N;
    res.height  = N;
    res.worldW  = p.worldW;
    res.worldH  = p.worldH;

    const double f      = std::max (1.0, p.frequency);
    const double lambda = kSpeedOfSound / f;
    const double k      = 2.0 * M_PI / lambda;
    const double omega  = 2.0 * M_PI * f;

    res.frequency      = f;
    res.lambda         = lambda;
    res.k              = k;
    res.minDisplayedDB = p.dBfloor;

    // Collect enabled speakers. Geometry / amplitude are frequency-independent;
    // only the phase term depends on frequency, which we exploit for fast
    // fractional-octave band averaging.
    struct Src
    {
        double x, y, gainLin, facing, delaySec, polPhase;
        bool   reverseOrientation;
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
        src.polPhase = s.polarityInverted ? M_PI : 0.0;   // polarity = 180 deg
        src.reverseOrientation = s.reverseOrientation;
        srcs.push_back (src);
    }
    res.activeSpeakers = (int) srcs.size();

    // Select a measured directivity pattern for this frequency. Exact match
    // preferred; otherwise the nearest available reading so the SPL heat map
    // updates for every frequency in the UI (Room / Ground Plane sets).
    const DirectivityPattern* pat = nullptr;
    if (p.useMeasuredDirectivity && ! p.directivity.empty())
    {
        const int fi = (int) std::lround (f);
        int bestDist = 100000;
        for (const auto& d : p.directivity)
        {
            if (! d.ok || d.gain.size() < 360) continue;
            const int dist = std::abs (d.hz - fi);
            if (dist < bestDist) { bestDist = dist; pat = &d; }
            if (dist == 0) break;
        }
    }
    res.usedMeasuredDirectivity = (pat != nullptr);
    res.measuredDirectivityHz   = pat != nullptr ? pat->hz : 0;

    // Note: absolute BEM mid-plane packs (Q21F) are NOT stamped into the world.
    // Stamping the native ±5 m (10×10 m) reading produces a rectangular island
    // on the 100×100 m canvas. The product simulation uses measured polar
    // directivity (pat / dirFactor) with 1/r spreading across the full world.

    // Directivity factor: measured shape when available, else the frequency-
    // dependent piston model (which narrows with frequency).
    auto dirFactorRaw = [pat, k] (double facing, double theta) -> double
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
    };

    // Average ±½° so a single grid row on the forward axis cannot paint a
    // 1-pixel ridge from polar seam / overlap spikes at exactly 0°.
    auto dirFactor = [&] (double facing, double theta) -> double
    {
        constexpr double eps = 0.5 * M_PI / 180.0;
        return (dirFactorRaw (facing, theta - eps)
              + dirFactorRaw (facing, theta)
              + dirFactorRaw (facing, theta + eps)) / 3.0;
    };

    res.splDB.assign        ((size_t) N * N, (float) p.dBfloor);
    res.splRelDB.assign     ((size_t) N * N, (float) p.dBfloor);
    res.splAbsDB.assign     ((size_t) N * N, 0.0f);
    res.pressure.assign     ((size_t) N * N, 0.0f);
    res.interference.assign ((size_t) N * N, 0.0f);

    // Fractional-octave band for power averaging. A single steady-state tone
    // produces mathematically perfect (infinitely deep, razor-thin) nulls that
    // are never observed in real broadband measurements. Averaging |P|^2 over a
    // 1/3-octave band reproduces the smoother, physically realistic SPL maps
    // that tools like EASE show, while still resolving true summation and broad
    // cancellation zones.
    constexpr int    M       = 7;            // sub-frequencies across the band
    const double     halfOct = 1.0 / 6.0;    // +/- 1/6 octave => 1/3 octave total
    const int        mCount  = p.octaveSmoothing ? M : 1;
    std::vector<double> kBand ((size_t) mCount), omegaBand ((size_t) mCount);
    for (int m = 0; m < mCount; ++m)
    {
        const double frac = (mCount == 1) ? 0.0
                          : (2.0 * m / (mCount - 1) - 1.0) * halfOct;
        const double fm = f * std::pow (2.0, frac);
        kBand[(size_t) m]     = 2.0 * M_PI * fm / kSpeedOfSound;
        omegaBand[(size_t) m] = 2.0 * M_PI * fm;
    }

    // Grid spans the full world: X in [0, worldW], Y in [0, worldH].
    const double dx = res.worldW / (N - 1);
    const double dy = res.worldH / (N - 1);

    using cd = std::complex<double>;

    std::vector<cd>     P    ((size_t) N * N);   // centre-frequency complex pressure
    std::vector<double> Iavg ((size_t) N * N);   // band-averaged intensity
    std::vector<double> Iabs ((size_t) N * N);   // incoherent magnitude sum

    // Per-speaker scratch (geometry computed once per grid point, reused across
    // the band's sub-frequencies).
    std::vector<double> rr ((size_t) srcs.size()), amp ((size_t) srcs.size());

    double maxI = 0.0;          // max band intensity for SPL normalisation
    double maxAbsRe = 0.0;      // max |Re(P)| for pressure normalisation

    for (int row = 0; row < N; ++row)
    {
        const double Y = row * dy;
        for (int col = 0; col < N; ++col)
        {
            const double X = col * dx;

            // 1) Frequency-independent geometry & amplitude per speaker.
            for (size_t i = 0; i < srcs.size(); ++i)
            {
                const auto& s = srcs[i];
                double r = std::sqrt ((X - s.x) * (X - s.x) + (Y - s.y) * (Y - s.y));
                r = std::max (r, kMinDistance);
                const double theta = std::atan2 (Y - s.y, X - s.x);
                const double D     = dirFactor (s.facing, theta);
                rr[i]  = r;
                amp[i] = s.gainLin * D / r;          // 1/r spherical spreading
            }

            // 2) Centre-frequency coherent sum (pressure / interference views).
            cd     centre (0.0, 0.0);
            double incoh = 0.0;
            for (size_t i = 0; i < srcs.size(); ++i)
            {
                const double phase = -(k * rr[i] + omega * srcs[i].delaySec) + srcs[i].polPhase;
                centre += std::polar (amp[i], phase);
                incoh  += amp[i];
            }

            // 3) Band-averaged intensity (smooth, realistic SPL heatmap).
            double Iband = 0.0;
            for (int m = 0; m < mCount; ++m)
            {
                cd sm (0.0, 0.0);
                for (size_t i = 0; i < srcs.size(); ++i)
                {
                    const double phase = -(kBand[(size_t) m] * rr[i]
                                           + omegaBand[(size_t) m] * srcs[i].delaySec)
                                         + srcs[i].polPhase;
                    sm += std::polar (amp[i], phase);
                }
                Iband += std::norm (sm);
            }
            Iband /= (double) mCount;

            const size_t idx = (size_t) row * N + col;
            P[idx]    = centre;
            Iavg[idx] = Iband;
            Iabs[idx] = incoh;

            maxI     = std::max (maxI, Iband);
            maxAbsRe = std::max (maxAbsRe, std::abs (centre.real()));
        }
    }

    if (maxI < 1e-300)     maxI = 1.0;
    if (maxAbsRe < 1e-300) maxAbsRe = 1.0;

    const double floorDB = p.dBfloor;

    const bool hasAbs = (pat != nullptr && pat->hasAbsolute);
    const double Rref = (hasAbs && pat->refDistanceM > 0.05f) ? (double) pat->refDistanceM : 2.0;
    const double Iref = 1.0 / (Rref * Rref);   // one unit, on-axis, at the polar radius
    const double onAxisAbs = hasAbs ? (double) pat->onAxisSplDb : 0.0;
    double peakAbs = -1.0e9;

    for (size_t i = 0; i < P.size(); ++i)
    {
        // Relative SPL (max = 0 dB). Colour map uses the display floor;
        // CSV / reports keep the unfloored reading.
        const double rel = 10.0 * std::log10 (std::max (Iavg[i], 1e-300) / maxI);
        res.splRelDB[i] = (float) rel;
        res.splDB[i]    = (float) std::max (rel, floorDB);

        // Absolute dB SPL from the measured on-axis reading (unfloored).
        // I = |P|² with amp = D/r; at r = Rref, D = 1 → I = Iref → SPL = on-axis.
        if (hasAbs)
        {
            const double absDb = onAxisAbs + 10.0 * std::log10 (std::max (Iavg[i], 1e-300) / Iref);
            res.splAbsDB[i] = (float) absDb;
            peakAbs = std::max (peakAbs, absDb);
        }
        else
        {
            res.splAbsDB[i] = (float) rel;
        }

        // Instantaneous real pressure normalised to [-1, 1].
        res.pressure[i] = (float) (P[i].real() / maxAbsRe);

        // Coherence ratio: |sum| / sum|contrib| in [0, 1].
        const double coh = std::abs (P[i]);
        res.interference[i] = (Iabs[i] > 1e-300)
                                ? (float) std::clamp (coh / Iabs[i], 0.0, 1.0)
                                : 0.0f;
    }
    res.peakAbsDb      = hasAbs ? peakAbs : 0.0;
    res.hasAbsoluteSpl = hasAbs;

    // --- Far-field polar pattern of the whole array ------------------------
    // Centre on the array (not the world mid-point) so angle 0 = forward (+X),
    // matching the SPL heatmap speaker facing.
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

        double maxMag = 0.0;
        for (int i = 0; i < nPolar; ++i)
        {
            // Angle 0 = +X (forward), increasing CCW in world (+Y up).
            const double angle = i * (2.0 * M_PI / nPolar);
            const double Xf = cx + rFar * std::cos (angle);
            const double Yf = cy + rFar * std::sin (angle);

            cd sum (0.0, 0.0);
            for (const auto& s : srcs)
            {
                double r = std::sqrt ((Xf - s.x) * (Xf - s.x) + (Yf - s.y) * (Yf - s.y));
                r = std::max (r, kMinDistance);
                const double theta = std::atan2 (Yf - s.y, Xf - s.x);
                const double D     = dirFactor (s.facing, theta);
                const double amp   = s.gainLin * D / r;
                const double phase = -(k * r + omega * s.delaySec) + s.polPhase;
                sum += std::polar (amp, phase);
            }

            const double mag = std::abs (sum);
            res.polarMag[i]  = (float) mag;
            maxMag = std::max (maxMag, mag);
        }
        if (maxMag > 1e-300)
            for (auto& v : res.polarMag) v = (float) (v / maxMag);
    }

    return res;
}

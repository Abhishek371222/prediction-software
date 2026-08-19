#include "AcousticEngine.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static DirectivityPattern makeBemOmni (int hz, float onAxis, float rRef)
{
    DirectivityPattern d;
    d.hz = hz;
    d.gain.assign (360, 1.0f);
    d.onAxisSplDb = onAxis;
    d.refDistanceM = rRef;
    d.hasAbsolute = true;
    d.ok = true;
    return d;
}

static float sampleAbs (const SimResult& r, double x, double y)
{
    const int col = (int) std::lround (x / r.worldW * (r.width - 1));
    const int row = (int) std::lround (y / r.worldH * (r.height - 1));
    return r.splAbsDB[(size_t) row * (size_t) r.width + (size_t) col];
}

int main()
{
    const float bemOnAxis = 104.357840f;   // Q21S_52Hz_2p0m.csv on-axis
    const float rRef = 2.0f;

    SimParams p;
    p.frequency = 52.0;
    p.worldW = 100.0;
    p.worldH = 100.0;
    p.resolution = 401;          // 0.25 m cells so 2 m / 4 m land on grid
    p.dBfloor = -36.0;
    p.octaveSmoothing = false;
    p.useMeasuredDirectivity = true;
    p.directivity.push_back (makeBemOmni (52, bemOnAxis, rRef));

    Speaker s;
    s.x = 50.0f;
    s.y = 50.0f;
    s.gainDB = 0.0f;
    s.enabled = true;
    p.speakers = { s };

    SimResult one = AcousticEngine::compute (p);
    if (! one.hasAbsoluteSpl || one.usedBemField)
    {
        std::fprintf (stderr, "FAIL: expected abs SPL path, no BEM stamp\n");
        return 1;
    }
    if (one.width * one.height != 401 * 401)
    {
        std::fprintf (stderr, "FAIL: expected full 401x401 grid\n");
        return 1;
    }

    const float at2 = sampleAbs (one, 52.0, 50.0);
    const float at4 = sampleAbs (one, 54.0, 50.0);
    const float err2 = std::abs (at2 - bemOnAxis);
    const float drop = at2 - at4;

    std::printf ("1 box @ 2 m on-axis: %.3f dB (BEM %.3f, err %.3f)\n", at2, bemOnAxis, err2);
    std::printf ("1 box @ 4 m on-axis: %.3f dB (drop %.3f, expect ~6)\n", at4, drop);

    if (err2 > 0.15f)
    {
        std::fprintf (stderr, "FAIL: 2 m reading does not match BEM on-axis\n");
        return 1;
    }
    if (std::abs (drop - 6.0f) > 0.2f)
    {
        std::fprintf (stderr, "FAIL: inverse-square drop is not ~6 dB\n");
        return 1;
    }

    p.speakers.push_back (s);   // second coherent box, same pose
    SimResult two = AcousticEngine::compute (p);
    const float twoAt2 = sampleAbs (two, 52.0, 50.0);
    const float sumGain = twoAt2 - at2;
    std::printf ("2 boxes @ 2 m: %.3f dB (vs 1: +%.3f, expect ~+6)\n", twoAt2, sumGain);
    if (std::abs (sumGain - 6.0f) > 0.25f)
    {
        std::fprintf (stderr, "FAIL: coherent sum is not ~+6 dB\n");
        return 1;
    }

    // Corner of the 100 m world must be a real (finite) reading, not empty.
    const float corner = sampleAbs (one, 0.0, 0.0);
    std::printf ("Corner (0,0): %.3f dB SPL\n", corner);
    if (! std::isfinite (corner))
    {
        std::fprintf (stderr, "FAIL: corner SPL is not finite\n");
        return 1;
    }

    std::printf ("OK: full-world BEM superposition + inverse-square\n");
    return 0;
}

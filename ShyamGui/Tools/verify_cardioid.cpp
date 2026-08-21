#include "AcousticEngine.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static bool loadBemPolarCsv (const char* path, DirectivityPattern& out)
{
    std::ifstream in (path);
    if (! in) return false;

    std::string line;
    std::getline (in, line); // header

    std::vector<float> spl (360, 0.0f);
    int n = 0;
    while (std::getline (in, line))
    {
        if (line.empty()) continue;
        std::stringstream ss (line);
        std::string a, s;
        if (! std::getline (ss, a, ',')) continue;
        if (! std::getline (ss, s, ',')) continue;
        const int deg = (int) std::lround (std::stod (a)) % 360;
        if (deg < 0 || deg >= 360) continue;
        spl[(size_t) deg] = (float) std::stod (s);
        ++n;
    }
    if (n < 180) return false;

    const float onAxis = spl[0];
    out.hz = 52;
    out.onAxisSplDb = onAxis;
    out.refDistanceM = 2.0f;
    out.hasAbsolute = true;
    out.ok = true;
    out.gain.assign (360, 1.0f);
    for (int deg = 0; deg < 360; ++deg)
    {
        const float g = (float) std::pow (10.0, (spl[(size_t) deg] - onAxis) / 20.0);
        out.gain[(size_t) deg] = std::clamp (g, 0.0f, 2.0f);
    }
    return true;
}

static float sampleAbs (const SimResult& r, double x, double y)
{
    const int col = (int) std::lround (x / r.worldW * (r.width - 1));
    const int row = (int) std::lround (y / r.worldH * (r.height - 1));
    const int c = std::clamp (col, 0, r.width - 1);
    const int rr = std::clamp (row, 0, r.height - 1);
    return r.splAbsDB[(size_t) rr * (size_t) r.width + (size_t) c];
}

int main (int argc, char** argv)
{
    const char* csv = (argc > 1)
        ? argv[1]
        : "prediction software/MeasurementIntegrationPack/Data/Q21S_52Hz_2p0m.csv";

    DirectivityPattern pat;
    if (! loadBemPolarCsv (csv, pat))
    {
        std::fprintf (stderr, "FAIL: could not load BEM polar CSV: %s\n", csv);
        return 1;
    }

    // MATLAB cardioid recipe at 52 Hz.
    constexpr float d = 0.01f;
    constexpr float delayMs = 3.5f;
    constexpr float rearGainDb = -6.0f;
    constexpr double rCheck = 2.0;
    const float cx = 50.0f, cy = 50.0f;

    Speaker front, rear;
    front.x = cx + 0.5f * d;
    front.y = cy;
    front.gainDB = 0.0f;
    front.delayMs = 0.0f;
    front.polarityInverted = false;
    front.reverseOrientation = false;
    front.enabled = true;

    rear.x = cx - 0.5f * d;
    rear.y = cy;
    rear.gainDB = rearGainDb;
    rear.delayMs = delayMs;
    rear.polarityInverted = true;
    rear.reverseOrientation = true;   // 180 deg physical rotation
    rear.enabled = true;

    SimParams p;
    p.frequency = 52.0;
    p.worldW = 100.0;
    p.worldH = 100.0;
    p.resolution = 401;
    p.dBfloor = -36.0;
    p.octaveSmoothing = false;
    p.useMeasuredDirectivity = true;
    p.directivity.push_back (pat);
    p.speakers = { rear, front };

    SimResult r = AcousticEngine::compute (p);
    if (! r.hasAbsoluteSpl || r.usedBemField)
    {
        std::fprintf (stderr, "FAIL: expected full-world abs SPL, no BEM stamp\n");
        return 1;
    }

    // App forward = +X (MATLAB +Z). Front/back check at r_check.
    const float splFront = sampleAbs (r, cx + rCheck, cy);
    const float splRear  = sampleAbs (r, cx - rCheck, cy);
    const float rejection = splFront - splRear;

    std::printf ("Cardioid 52 Hz | d=%.2f m | rear: 180 rot + %.1f ms + Gain 0.5\n",
                 (double) d, (double) delayMs);
    std::printf ("SPL front (X ~ +%.1f m): %.1f dB\n", rCheck, splFront);
    std::printf ("SPL rear  (X ~ -%.1f m): %.1f dB\n", rCheck, splRear);
    std::printf ("Front-to-back rejection: %.1f dB\n", rejection);

    if (rejection < 3.0f)
    {
        std::fprintf (stderr, "FAIL: expected positive front-to-back rejection (>= 3 dB)\n");
        return 1;
    }

    std::printf ("OK: MATLAB-style cardioid on full 100x100 m SPL heatmap path\n");
    return 0;
}

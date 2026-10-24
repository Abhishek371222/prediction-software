// Loads an .atmk scene with the real measured directivity and reports how it
// covers the audience: level across the seats, the spread front to back and
// side to side, what the delay towers are worth, and what the sub array does
// across the width of the floor.
//
// Build (from the repo root, in a VS developer shell):
//   cl /std:c++17 /O2 /EHsc /I ShyamGui\JuceLibraryCode /I C:\JUCE\modules ^
//      /I ShyamGui\Source ShyamGui\Tools\analyse_scene.cpp ^
//      ShyamGui\Source\AcousticEngine.cpp ShyamGui\Source\EmbeddedQ21SData.cpp ^
//      ShyamGui\Source\EmbeddedBEM2inchData.cpp ShyamGui\Source\EmbeddedQ212PData.cpp ^
//      ShyamGui\JuceLibraryCode\include_juce_core.cpp ... /link /SUBSYSTEM:CONSOLE
//
// It reads the same project file the app opens and the same measured tables
// the app loads, so the numbers it prints are the app's numbers.

#include "MeasurementData.h"
#include "AcousticEngine.h"
#include "AcousticAnalysis.h"

#include <cstdio>
#include <cmath>
#include <vector>
#include <string>

// Normally supplied by juce_gui_basics, which a console tool does not link.
struct tagMSG;
namespace juce { long long juce_offerEventToActiveXControl (tagMSG&) { return -1; } }

namespace
{
struct Scene
{
    std::vector<Speaker> speakers;
    double freqQ21S = 52.0, freqQ212P = 1088.0;
    int resolution = 400;
    double dBfloor = -24.0;
};

Scene loadScene (const juce::File& f)
{
    Scene sc;
    const juce::var v = juce::JSON::parse (f);
    auto* root = v.getDynamicObject();
    if (root == nullptr) { std::printf ("could not parse %s\n", f.getFullPathName().toRawUTF8()); return sc; }

    if (auto mf = root->getProperty ("modelFrequency"); mf.isArray())
    {
        if (mf.size() > 0 && (double) mf[0] > 0) sc.freqQ21S  = (double) mf[0];
        if (mf.size() > 3 && (double) mf[3] > 0) sc.freqQ212P = (double) mf[3];
    }
    if (root->hasProperty ("resolution")) sc.resolution = (int) root->getProperty ("resolution");
    if (root->hasProperty ("dBfloor"))    sc.dBfloor    = (double) root->getProperty ("dBfloor");

    if (auto sv = root->getProperty ("speakers"); sv.isArray())
        for (auto& e : *sv.getArray())
            if (auto* o = e.getDynamicObject())
            {
                Speaker s;
                s.x = (float) (double) o->getProperty ("x");
                s.y = (float) (double) o->getProperty ("y");
                s.gainDB = (float) (double) o->getProperty ("gainDB");
                s.delayMs = (float) (double) o->getProperty ("delayMs");
                s.rotationDeg = (float) (double) o->getProperty ("rotationDeg");
                s.baseHeightM = (float) (double) o->getProperty ("baseHeightM");
                s.tiltDeg = (float) (double) o->getProperty ("tiltDeg");
                s.model = (int) o->getProperty ("model");
                s.enabled = true;
                sc.speakers.push_back (s);
            }
    return sc;
}

// The app's own loader, so the patterns are the ones it draws with.
struct Tables
{
    std::vector<DirectivityPattern> q21s, bem2, q212p;
};

Tables loadTables()
{
    Tables t;
    auto mQ21S  = MeasurementData::loadMeasurements (MeasurementData::folderForSource (MeasurementData::OpenField),
                                                     MeasurementData::OpenField);
    auto mQ212P = MeasurementData::loadMeasurements (MeasurementData::folderForSource (MeasurementData::Q212P),
                                                     MeasurementData::Q212P);
    t.q21s  = MeasurementData::buildDirectivityTables (mQ21S,  MeasurementData::farFieldDirectivityDistance (mQ21S, 1.0f));
    t.q212p = MeasurementData::buildDirectivityTables (mQ212P, MeasurementData::farFieldDirectivityDistance (mQ212P, 1.0f));
    std::printf ("measured tables: Q21S %d bands, Q212P %d bands\n",
                 (int) t.q21s.size(), (int) t.q212p.size());
    return t;
}

SimParams baseParams (const Scene& sc, const Tables& t)
{
    SimParams p;
    p.worldX0 = 0; p.worldY0 = 0; p.worldW = 100; p.worldH = 100;
    p.resolution = sc.resolution;
    p.dBfloor = sc.dBfloor;
    p.octaveSmoothing = true;
    p.viewMode = ViewMode::SPL;
    p.directivity = t.q21s;
    p.directivityQ212P = t.q212p;
    p.frequencyQ21S = sc.freqQ21S;
    p.frequencyQ212P = sc.freqQ212P;
    p.frequency = sc.freqQ212P;
    return p;
}

// Un-normalised level at a point, in dB. Differences between points in the
// same scene are what matter, so an arbitrary reference is fine.
double levelAt (const SimParams& p, double x, double y)
{
    float i = 0, a = 0;
    if (! AcousticEngine::sampleIntensityAt (p, (float) x, (float) y, i, a)) return -999.0;
    return (double) i;
}

// Calibrated dB SPL, when every box in the scene carries a measured
// sensitivity. Returns -999 when the scene is relative-only.
double absAt (const SimParams& p, double x, double y)
{
    float i = 0, a = 0;
    if (! AcousticEngine::sampleIntensityAt (p, (float) x, (float) y, i, a)) return -999.0;
    return (double) a;
}

struct Area { const char* name; double x0, y0, x1, y1; int nx, ny; };

// A listening position has to be a position a listener could stand in.
// Fills and delay towers sit in the audience, so without this the grid
// lands a metre from a cabinet and that point sets the reference.
constexpr double kMinDistToBox = 3.0;
bool isSeat (const SimParams& p, double x, double y)
{
    for (const auto& s : p.speakers)
        if (std::hypot (x - s.x, y - s.y) < kMinDistToBox) return false;
    return true;
}

const Area AREAS[] = {
    { "Floor  (standing)", 20, 14, 80, 52, 13, 9 },
    { "West stand",         8, 14, 18, 52,  3, 9 },
    { "East stand",        82, 14, 92, 52,  3, 9 },
    { "North stand",       20, 54, 80, 64, 13, 3 },
};

struct Stat { double lo = 1e9, hi = -1e9, sum = 0; int n = 0;
              void add (double v) { lo = std::min (lo, v); hi = std::max (hi, v); sum += v; ++n; }
              double mean() const { return n ? sum / n : 0; }
              double spread() const { return hi - lo; } };

Stat survey (const SimParams& p, const Area& a)
{
    Stat st;
    for (int iy = 0; iy < a.ny; ++iy)
        for (int ix = 0; ix < a.nx; ++ix)
        {
            const double x = a.x0 + (a.nx == 1 ? 0 : (a.x1 - a.x0) * ix / (a.nx - 1));
            const double y = a.y0 + (a.ny == 1 ? 0 : (a.y1 - a.y0) * iy / (a.ny - 1));
            if (! isSeat (p, x, y)) continue;
            st.add (levelAt (p, x, y));
        }
    return st;
}

// Coverage over the SEATS, which is the number a designer wants. The field
// version counts the whole 100 x 100 world, and the loudest cell in that
// world sits a metre in front of a cabinet - so it reports almost 0% for
// any real design and says nothing about the audience.
double audienceCoverage (const SimParams& p, double windowDb)
{
    std::vector<double> v;
    for (const auto& a : AREAS)
        for (int iy = 0; iy < a.ny; ++iy)
            for (int ix = 0; ix < a.nx; ++ix)
            {
                const double x = a.x0 + (a.nx == 1 ? 0 : (a.x1 - a.x0) * ix / (a.nx - 1));
                const double y = a.y0 + (a.ny == 1 ? 0 : (a.y1 - a.y0) * iy / (a.ny - 1));
                if (! isSeat (p, x, y)) continue;
                v.push_back (levelAt (p, x, y));
            }
    double mx = -1e9; for (double d : v) mx = std::max (mx, d);
    int in = 0; for (double d : v) if (d >= mx - windowDb) ++in;
    return 100.0 * in / (double) v.size();
}

void report (const char* title, const SimParams& p, double ref)
{
    juce::ignoreUnused (ref);
    std::printf ("\n%s\n", title);
    std::printf ("  %-20s %8s %8s %8s %8s\n", "area", "best", "worst", "mean", "spread");
    for (const auto& a : AREAS)
    {
        const Stat s = survey (p, a);
        std::printf ("  %-20s %+7.1f %+8.1f %+8.1f %7.1f dB\n",
                     a.name, s.hi, s.lo, s.mean(), s.spread());
    }
}
} // namespace

int main (int argc, char** argv)
{
    const juce::File f (argc > 1 ? juce::String (argv[1])
                                 : juce::String ("C:\\Users\\User\\Downloads\\Small_Stadium.atmk"));
    std::printf ("scene: %s\n", f.getFullPathName().toRawUTF8());

    const Tables t = loadTables();
    const Scene sc = loadScene (f);
    std::printf ("speakers: %d   tops @ %.0f Hz   subs @ %.0f Hz\n",
                 (int) sc.speakers.size(), sc.freqQ212P, sc.freqQ21S);
    if (sc.speakers.empty()) return 1;

    auto onlyModel = [&] (int model)
    {
        std::vector<Speaker> v;
        for (const auto& s : sc.speakers) if (s.model == model) v.push_back (s);
        return v;
    };

    // ---------------------------------------------------------------- tops
    {
        SimParams p = baseParams (sc, t);
        p.speakers = onlyModel (3);
        p.frequency = sc.freqQ212P;

        // reference: the loudest seat on the floor, so every figure below
        // reads as "how far down on the best seat in the house"
        double ref = -1e9;
        for (const auto& a : AREAS) ref = std::max (ref, survey (p, a).hi);

        const SimResult r = AcousticEngine::compute (p);
        std::printf ("\n================ TOPS, %d boxes @ %.0f Hz ================\n",
                     (int) p.speakers.size(), sc.freqQ212P);
        std::printf ("coverage over the seats: %.0f %% within 6 dB, %.0f %% within 10 dB\n",
                     audienceCoverage (p, 6.0), audienceCoverage (p, 10.0));
        std::printf ("  (whole-field figure for reference: %.1f %% - its peak is on a cabinet)\n",
                     AcousticAnalysis::coverageWithin (r, 6.0));
        report ("level across the audience, raw dB - comparable between scenes", p, 0.0);

        // delay towers off
        SimParams q = p;
        q.speakers.clear();
        for (const auto& s : p.speakers) if (s.y < 20.0f) q.speakers.push_back (s);
        std::printf ("\n-- with the two delay towers muted (%d boxes) --\n", (int) q.speakers.size());
        report ("level across the audience, same scale", q, 0.0);

        // down the centre line, every 6 m
        std::printf ("\n  down the centre line (x = 50):\n");
        for (double y = 16; y <= 64; y += 6)
            std::printf ("    y = %4.0f m   with towers %+6.1f dB    towers muted %+6.1f dB\n",
                         y, levelAt (p, 50, y), levelAt (q, 50, y));

        // left / right symmetry
        double worstAsym = 0;
        for (double y = 16; y <= 50; y += 4)
            for (double d = 6; d <= 28; d += 4)
                worstAsym = std::max (worstAsym, std::abs (levelAt (p, 50 - d, y) - levelAt (p, 50 + d, y)));
        std::printf ("\n  worst left/right imbalance across the floor: %.2f dB\n", worstAsym);
    }

    // ---------------------------------------------------------------- subs
    {
        SimParams p = baseParams (sc, t);
        p.speakers = onlyModel (0);
        p.frequency = sc.freqQ21S;

        double ref = -1e9;
        for (const auto& a : AREAS) ref = std::max (ref, survey (p, a).hi);

        const SimResult r = AcousticEngine::compute (p);
        std::printf ("\n================ SUBS, %d boxes @ %.0f Hz ================\n",
                     (int) p.speakers.size(), sc.freqQ21S);
        std::printf ("coverage over the seats: %.0f %% within 6 dB, %.0f %% within 10 dB\n",
                     audienceCoverage (p, 6.0), audienceCoverage (p, 10.0));
        std::printf ("  (whole-field figure for reference: %.1f %%)\n",
                     AcousticAnalysis::coverageWithin (r, 6.0));
        report ("level across the audience, raw dB", p, 0.0);

        std::printf ("\n  across the width at y = 30 m (looking for lobes):\n");
        for (double x = 22; x <= 78; x += 4)
            std::printf ("    x = %4.0f m  %+6.1f dB\n", x, levelAt (p, x, 30));

        {
            const SimResult chk = AcousticEngine::compute (p);
            std::printf ("\n  absolute dB SPL (every sub carries its measured sensitivity: %s)\n",
                         chk.hasAbsoluteSpl ? "yes" : "no - relative only");
            if (chk.hasAbsoluteSpl)
                for (const auto& a : AREAS)
                {
                    double lo = 1e9, hi = -1e9, sum = 0; int n = 0;
                    for (int iy = 0; iy < a.ny; ++iy)
                        for (int ix = 0; ix < a.nx; ++ix)
                        {
                            const double x = a.x0 + (a.nx == 1 ? 0 : (a.x1 - a.x0) * ix / (a.nx - 1));
                            const double y = a.y0 + (a.ny == 1 ? 0 : (a.y1 - a.y0) * iy / (a.ny - 1));
                            const double v = absAt (p, x, y);
                            lo = std::min (lo, v); hi = std::max (hi, v); sum += v; ++n;
                        }
                    std::printf ("    %-20s %6.1f .. %5.1f dB SPL   mean %5.1f\n",
                                 a.name, lo, hi, sum / n);
                }
        }
        std::printf ("\n  behind the array, on stage (x = 50):\n");
        for (double y = 10; y >= 4; y -= 2)
            std::printf ("    y = %4.0f m  %+6.1f dB\n", y, levelAt (p, 50, y));
    }

    std::printf ("\ndone\n");
    return 0;
}

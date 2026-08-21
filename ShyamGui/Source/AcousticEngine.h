#pragma once
#include <vector>
#include <complex>

// ---------------------------------------------------------------------------
// Speaker — a single subwoofer in the 2D world (pure data, no JUCE).
// Q21S cabinet: W 750 mm × H 784 mm × D 917 mm (plan footprint = W × D).
// ---------------------------------------------------------------------------
struct Speaker
{
    float x          = 50.0f;   // metres, world X
    float y          = 50.0f;   // metres, world Y
    float gainDB     = 0.0f;    // <= 0 dB, 1 dB steps
    float delayMs    = 0.0f;    // milliseconds
    bool  polarityInverted   = false;   // Normal / Reverse
    bool  reverseOrientation = false;   // Forward (+x) / Reverse (-x)
    bool  enabled            = true;
};

// Q21S product cabinet dimensions (metres). Plan view uses width × depth.
namespace Q21SCabinet
{
    constexpr float widthM  = 0.750f;   // 750 mm — left/right
    constexpr float heightM = 0.784f;   // 784 mm — vertical
    constexpr float depthM  = 0.917f;   // 917 mm — front/back (firing axis)
    constexpr float halfExtentM = depthM * 0.5f;  // singularity floor for 1/r
}

// Measured horizontal directivity for one frequency: linear gain vs angle
// (degree-indexed, 360 entries), normalized so the on-axis (0 deg) gain = 1.
struct DirectivityPattern
{
    int                hz = 0;
    std::vector<float> gain;     // 360 entries, linear gain per integer degree
    float              onAxisSplDb = 0.0f;   // measured on-axis dB SPL at refDistanceM
    float              refDistanceM = 2.0f;  // polar arc used to build this table
    bool               hasAbsolute = false;
    bool               ok = false;
};

// Absolute BEM SPL field on the X–Z mid-plane (MATLAB Heatmap.m formation).
// relDb is peak-normalised (max = 0 dB), row-major [iz * nx + ix].
struct BemFieldPattern
{
    int                hz = 0;
    int                nx = 0;
    int                nz = 0;
    float              xmin = 0.0f, xmax = 0.0f;
    float              zmin = 0.0f, zmax = 0.0f;
    std::vector<float> relDb;
    bool               ok = false;
};

// UI catalogue = native BEM_Data_10m/<Hz>Hz.xlsx only (no interpolated extras).
static constexpr double kSupportedFrequencies[] = {
    20, 29, 52, 81, 98, 153, 198, 256, 309, 352, 400, 401
};
static constexpr int kNumSupportedFrequencies =
    (int) (sizeof (kSupportedFrequencies) / sizeof (kSupportedFrequencies[0]));

// View modes selectable in the UI / renderer.
enum class ViewMode
{
    SPL          = 0,   // relative SPL heatmap (banded contours)
    Pressure     = 1,   // instantaneous real pressure (interference fringes)
    Interference = 2,   // coherent vs incoherent summation ratio
    Directivity  = 3,   // far-field polar pattern of the array
    MeasuredPolar = 4   // measured horizontal polar readings (.xlsx)
};

struct SimParams
{
    double frequency  = 52.0;    // one of kSupportedFrequencies (Hz)
    double worldW     = 100.0;   // world width  (m)
    double worldH     = 100.0;   // world height (m)
    int    resolution = 400;     // grid cells per axis
    double dBfloor    = -36.0;   // lowest displayed dB (display dynamic range)
    int    colourmap  = 0;       // 0 = SPL contour (spec), others = legacy maps
    bool   bandedSPL  = false;   // true = spec 3 dB contour bands, false = smooth
    bool   octaveSmoothing = true; // 1/3-octave power averaging (realistic SPL)
    ViewMode viewMode = ViewMode::SPL;

    std::vector<Speaker> speakers;

    // Measured directivity (optional). When useMeasuredDirectivity is true and a
    // pattern matching the simulation frequency exists, each source radiates with
    // its measured shape instead of the omnidirectional monopole model.
    std::vector<DirectivityPattern> directivity;
    // Absolute BEM mid-plane fields (Heatmap.m / Q21F). Loaded for tooling;
    // the live SPL heatmap uses measured polar directivity across the full world
    // (not a stamped ±5 m island).
    std::vector<BemFieldPattern> bemFields;
    bool useMeasuredDirectivity = true;
};

struct SimResult
{
    int width  = 0;
    int height = 0;

    double worldW = 100.0;
    double worldH = 100.0;

    // Derived physics (info panel)
    double frequency = 0;
    double lambda    = 0;
    double k         = 0;

    // Field grids (row-major [height x width]) ----------------------------
    std::vector<float> splDB;        // relative SPL, colour-clipped at dBfloor only
    std::vector<float> splRelDB;     // relative SPL unfloored (max = 0 dB)
    std::vector<float> splAbsDB;     // absolute dB SPL (unfloored, measured-calibrated)
    std::vector<float> pressure;     // normalised real pressure in [-1, 1]
    std::vector<float> interference; // coherence ratio in [0, 1] (1 = constructive)
    double peakAbsDb = 0.0;          // max absolute SPL on the grid
    bool   hasAbsoluteSpl = false;   // true when splAbsDB is measured-calibrated dB SPL

    // Far-field polar pattern: 720 normalised magnitudes (0..1), 0.5 deg step
    std::vector<float> polarMag;

    int    activeSpeakers = 0;
    double minDisplayedDB = -18.0;
    bool   usedMeasuredDirectivity = false;   // true if a measured pattern was applied
    int    measuredDirectivityHz   = 0;       // which reading frequency was used (0 = none)

    // When true, splDB was built from an absolute BEM mid-plane stamp (unused
    // in the product path — full-world measured-directivity simulation instead).
    bool   usedBemField = false;
    double bemOriginX   = 0.0;
    double bemOriginY   = 0.0;
};

// ---------------------------------------------------------------------------
// AcousticEngine — BEM polar × 1/r over the full world, coherent array sum.
//
// Each enabled Q21S uses the measured BEM directivity D(θ) and on-axis dB SPL
// at R_ref. Pressure spreads as 1/r (inverse-square intensity). Pressures add
// as complex numbers (superposition). The ±5 m BEM field is never stamped.
// ---------------------------------------------------------------------------
class AcousticEngine
{
public:
    static SimResult compute (const SimParams& p);

    // dB thresholds for the spec colour contour (0, -3, ... -18).
    static constexpr double kColourStepDB = 3.0;

private:
    // Orientation directivity: subtle cardioid-like bias toward facing dir.
    static double orientationGain (double facingAngle, double angleToPoint);
};

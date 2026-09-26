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
    // Which device this unit is: 0 = Q21S, 2 = BEM2inch (matches
    // MeasurementData::Source / AcousticEngine::MeasurementSourceId). Each
    // unit is simulated with its OWN model's directivity — a scene can freely
    // mix both; they are never blended into one shared pattern.
    int   model              = 0;
};

// Display name for a Speaker::model / MeasurementData::Source value. Single
// canonical spot so UI labels, dialogs, and reports never disagree.
inline const char* speakerModelName (int model) noexcept
{
    return (model == 2) ? "BEM 2inch" : "Q21S";
}

// Identifier form of the model name, for labelling an individual unit
// ("BEM2inch_1"). speakerModelName() is the prose form used for headings and
// product text and keeps its space; a unit's name is a token, so it does not.
inline const char* speakerModelTag (int model) noexcept
{
    return (model == 2) ? "BEM2inch" : "Q21S";
}

// 1-based position of this unit among units of the SAME model. A mixed scene
// numbers each device from 1 -- placing a BEM unit second overall still makes
// it BEM2inch_1 -- rather than numbering by global placement order.
inline int speakerModelOrdinal (const std::vector<Speaker>& all, int index) noexcept
{
    if (index < 0 || index >= (int) all.size()) return index + 1;
    const int model = all[(size_t) index].model;
    int n = 0;
    for (int i = 0; i <= index; ++i)
        if (all[(size_t) i].model == model) ++n;
    return n;
}

// Q21S product cabinet dimensions (metres). Plan view uses width × depth.
// Q21S enclosure, manufacturer figures: 1546 x 679 x 1024 mm
// (60.86" x 26.73" x 40.31"), read in the pro-audio convention Height x
// Width x Depth. Width 679 mm is a single 21" driver plus baffle either
// side, and 1024 mm of depth suits the horn loading -- both consistent with
// that reading. If the source actually lists W x H x D instead, swap widthM
// and heightM here and nothing else needs touching.
//
// These replace an earlier 750 x 784 x 917 mm set that did not match the
// product. Plan footprint (width x depth) is what the SPL view draws, and
// halfExtentM feeds the engine's 1/r singularity floor, so both the marker
// and the near-field level follow from these numbers.
namespace Q21SCabinet
{
    constexpr float widthM  = 0.679f;   // 679 mm — left/right, across the baffle
    constexpr float heightM = 1.546f;   // 1546 mm — vertical
    constexpr float depthM  = 1.024f;   // 1024 mm — front/back (firing axis)
    constexpr float halfExtentM = depthM * 0.5f;  // singularity floor for 1/r
}

// 2" HF horn enclosure (metres). Much smaller than the Q21S in every axis,
// which matters twice over: the plan marker is drawn to scale against the
// field, and halfExtentM sets how close the 1/r law is allowed to get before
// it is clamped. Using the Q21S's 512 mm floor for a 150 mm-deep horn would
// have flattened its near field over half a metre of empty air.
namespace BEM2inchCabinet
{
    constexpr float widthM  = 0.459f;    // 459 mm — left/right, across the mouth
    constexpr float heightM = 0.2765f;   // 276.5 mm — vertical
    constexpr float depthM  = 0.150f;    // 150 mm — front/back (firing axis)
    constexpr float halfExtentM = depthM * 0.5f;
}

// Physical enclosure of one model, so plan markers, reported dimensions and
// the engine's near-field floor all read from one place.
struct CabinetDims
{
    float widthM = 0.0f, heightM = 0.0f, depthM = 0.0f, halfExtentM = 0.0f;
};

inline CabinetDims cabinetFor (int model) noexcept
{
    if (model == 2)
        return { BEM2inchCabinet::widthM, BEM2inchCabinet::heightM,
                 BEM2inchCabinet::depthM, BEM2inchCabinet::halfExtentM };
    return { Q21SCabinet::widthM, Q21SCabinet::heightM,
             Q21SCabinet::depthM, Q21SCabinet::halfExtentM };
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

// Per-model frequency catalogues — native BEM xlsx bands only (no interpolated
// extras). Q21S and BEM2inch are two completely separate devices: each keeps
// its own array below and the two are never merged or shared.
static constexpr double kQ21SFrequencies[] = {
    20, 29, 52, 81, 98, 153, 198, 256, 309, 352, 400, 401
};
static constexpr int kNumQ21SFrequencies =
    (int) (sizeof (kQ21SFrequencies) / sizeof (kQ21SFrequencies[0]));

static constexpr double kBEM2inchFrequencies[] = {
    64, 135, 243, 507, 1057, 1904, 3971, 8280, 17266
};
static constexpr int kNumBEM2inchFrequencies =
    (int) (sizeof (kBEM2inchFrequencies) / sizeof (kBEM2inchFrequencies[0]));

// Measurement source ids (mirror MeasurementData::Source; duplicated here so
// this header doesn't need to include MeasurementData.h).
enum class MeasurementSourceId { Q21S = 0, Room = 1, BEM2in = 2 };

// Isolated per-model catalogue lookup — never returns a blended list.
inline void frequencyCatalogue (int source, const double*& freqs, int& count) noexcept
{
    if (source == (int) MeasurementSourceId::BEM2in)
    {
        freqs = kBEM2inchFrequencies;
        count = kNumBEM2inchFrequencies;
    }
    else
    {
        freqs = kQ21SFrequencies;
        count = kNumQ21SFrequencies;
    }
}

// UI catalogue = the currently active model's native band list only. Repointed
// by setActiveFrequencyCatalogue() whenever the measurement source changes —
// the previous model's array is fully swapped out, never merged (single
// active pointer, so the two devices' frequencies cannot collide at runtime).
extern const double* kSupportedFrequencies;
extern int           kNumSupportedFrequencies;

inline void setActiveFrequencyCatalogue (int source) noexcept
{
    frequencyCatalogue (source, kSupportedFrequencies, kNumSupportedFrequencies);
}

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
    double frequency  = 52.0;    // active/displayed simulation frequency (Hz)
                                 // -- drives wavelength/phase (shared physical
                                 // basis) and the Frequency dropdown/reports.
    // Each device's OWN currently-selected frequency, always a value native to
    // THAT model's own catalogue -- never borrowed from the other model. Every
    // speaker picks its directivity pattern using its own model's entry here,
    // never the other model's, and never the raw `frequency` above when that
    // belongs to a different model. The two devices are isolated by
    // construction: changing one can never change the other's value or its
    // speakers' rendered pattern. Populated by ControlPanel::getParams().
    double frequencyQ21S   = 20.0;
    double frequencyBEM2inch = 64.0;
    // Region solved for: [worldX0, worldX0+worldW] x [worldY0, worldY0+worldH].
    // The origin exists so the view can pan the solved region around rather
    // than sliding a fixed box that always started at (0, 0).
    double worldX0    = 0.0;     // region origin x (m)
    double worldY0    = 0.0;     // region origin y (m)
    double worldW     = 100.0;   // world width  (m)
    double worldH     = 100.0;   // world height (m)
    int    resolution = 400;     // grid cells per axis
    double dBfloor    = -36.0;   // lowest displayed dB (display dynamic range)
    int    colourmap  = 0;       // 0 = SPL contour (spec), others = legacy maps
    bool   bandedSPL  = false;   // true = spec 3 dB contour bands, false = smooth
    bool   octaveSmoothing = true; // 1/3-octave power averaging (realistic SPL)
    ViewMode viewMode = ViewMode::SPL;

    std::vector<Speaker> speakers;

    // Measured BEM directivity tables, one array per device — always applied
    // (useMeasuredDirectivity is forced on — no UI toggle). Each Speaker picks
    // its table by its own `model` field (directivity = Q21S/source 0,
    // directivityBEM2inch = source 2); the engine never blends the two.
    std::vector<DirectivityPattern> directivity;
    std::vector<DirectivityPattern> directivityBEM2inch;
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

    double worldX0 = 0.0;
    double worldY0 = 0.0;
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
    double peakAbsDb = 0.0;          // absolute SPL at heatmap Rel. SPL = 0 dB
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
// Each enabled speaker uses ITS OWN model's measured BEM directivity D(θ) and
// on-axis dB SPL at R_ref (Q21S and BEM2inch units may coexist in one scene).
// Pressure spreads as 1/r (inverse-square intensity). Pressures add as complex
// numbers (superposition). The ±5 m BEM field is never stamped.
// ---------------------------------------------------------------------------
class AcousticEngine
{
public:
    static SimResult compute (const SimParams& p);

    // Point probe (no full grid). intensityDb = 10·log10(I); absDb when measured.
    // Used for Frequency Response curves across mics / frequencies.
    static bool sampleIntensityAt (const SimParams& p, float x, float y,
                                   float& intensityDb, float& absDb);

    // dB thresholds for the spec colour contour (0, -3, ... -18).
    static constexpr double kColourStepDB = 3.0;

private:
    // Orientation directivity: subtle cardioid-like bias toward facing dir.
    static double orientationGain (double facingAngle, double angleToPoint);
};

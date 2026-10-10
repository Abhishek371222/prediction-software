#pragma once
#include <vector>
#include <complex>

// ---------------------------------------------------------------------------
// SpeakerFilter - a crossover section on one cabinet.
//
// The transfer functions are the analogue prototypes, evaluated at the run
// frequency, exactly as butterworthFilter.m and LRFilter.m define them:
//
//   s = j*2*pi*f        wc = 2*pi*fc
//
//   Butterworth 2   LP  wc^2 / (s^2 + sqrt(2)*wc*s + wc^2)
//                   HP  s^2  / (s^2 + sqrt(2)*wc*s + wc^2)
//   Butterworth 4       the two Q-sections above with a1 / a2, multiplied
//   Linkwitz-Riley 2    a first-order section squared
//   Linkwitz-Riley 4    the 2nd-order Butterworth squared
//
// H is COMPLEX, so it carries phase as well as magnitude. That is the whole
// point: a crossover shifts the phase of what it passes, which moves where
// cabinets sum and cancel. Applying only |H| would change the level and quietly
// leave the interference pattern wrong.
// ---------------------------------------------------------------------------
struct SpeakerFilter
{
    enum class Family { Butterworth = 0, LinkwitzRiley = 1 };
    enum class Type   { Off = 0, LowPass = 1, HighPass = 2, BandPass = 3 };

    Family family = Family::LinkwitzRiley;
    Type   type   = Type::Off;
    int    order  = 4;            // 2 or 4, as the .m files allow
    float  fcHz   = 100.0f;

    // Band pass only: the UPPER corner, where the pass band stops. fcHz is
    // then the lower corner, so the band runs fcHz -> fcHiHz. Each end keeps
    // its own family and order, as BandPass.m allows (lowtype/Nl for the low
    // pass, hightype/Nh for the high pass) - a 24 dB/oct bottom under a
    // 12 dB/oct top is a real thing to want.
    Family familyHi = Family::LinkwitzRiley;
    int    orderHi  = 4;
    float  fcHiHz   = 2000.0f;

    static bool validOrder (int n) noexcept { return n == 2 || n == 4; }

    bool active() const noexcept
    {
        if (type == Type::Off) return false;
        if (! (fcHz > 0.0f && validOrder (order))) return false;
        // A band with its top at or below its bottom passes nothing at all;
        // treating that as "off" beats silencing a cabinet over a typo.
        if (type == Type::BandPass)
            return fcHiHz > fcHz && validOrder (orderHi);
        return true;
    }
};

/** H(f) for ONE section - a single low pass or high pass of the given family
    and order. This is the shared half of every filter shape: a low pass is one
    of these, a high pass is one of these, and a band pass is the product of
    two (BandPass.m: H = HLP .* HHP). */
inline std::complex<double> filterSectionResponse (double f,
                                                   SpeakerFilter::Family family,
                                                   int order, double fcHz,
                                                   bool lowPass) noexcept
{
    using cplx = std::complex<double>;
    constexpr double kPi = 3.14159265358979323846;
    const cplx   s  (0.0, 2.0 * kPi * f);
    const double wc = 2.0 * kPi * fcHz;
    const double wc2 = wc * wc;

    // One 2nd-order section. `a` is the damping term that sets its Q.
    auto section2 = [&] (double a) -> cplx
    {
        const cplx den = s * s + a * wc * s + wc2;
        return lowPass ? (cplx (wc2, 0.0) / den) : ((s * s) / den);
    };

    if (family == SpeakerFilter::Family::Butterworth)
    {
        if (order == 2)
            return section2 (1.41421356237309504880);          // sqrt(2)
        // Order 4 is two sections with the Butterworth Q pair:
        // 2*sin(pi/8) and 2*sin(3*pi/8).
        return section2 (0.76536686473017954) * section2 (1.84775906502257351);
    }

    // Linkwitz-Riley: the Butterworth of half the order, squared.
    if (order == 2)
    {
        const cplx h1 = lowPass ? (cplx (wc, 0.0) / (s + wc)) : (s / (s + wc));
        return h1 * h1;
    }
    const cplx hbw = section2 (1.41421356237309504880);
    return hbw * hbw;
}

/** H(f) for one filter. Returns 1 (no change) when the filter is off. */
inline std::complex<double> filterResponse (double f, const SpeakerFilter& flt) noexcept
{
    using cplx = std::complex<double>;
    if (! flt.active()) return cplx (1.0, 0.0);

    switch (flt.type)
    {
        case SpeakerFilter::Type::LowPass:
            return filterSectionResponse (f, flt.family, flt.order, (double) flt.fcHz, true);

        case SpeakerFilter::Type::HighPass:
            return filterSectionResponse (f, flt.family, flt.order, (double) flt.fcHz, false);

        case SpeakerFilter::Type::BandPass:
            // BandPass.m: the two sections MULTIPLY. A band pass is one signal
            // through both ends in series, which is a different thing from the
            // crossover in Crossover.m, where two signals through one end each
            // are ADDED back together in the air.
            return filterSectionResponse (f, flt.family,   flt.order,   (double) flt.fcHz,   false)
                 * filterSectionResponse (f, flt.familyHi, flt.orderHi, (double) flt.fcHiHz, true);

        case SpeakerFilter::Type::Off:
        default:
            return cplx (1.0, 0.0);
    }
}

// ---------------------------------------------------------------------------
// Speaker - a single subwoofer in the 2D world (pure data, no JUCE).
// Q21S cabinet: W 750 mm x H 784 mm x D 917 mm (plan footprint = W x D).
// ---------------------------------------------------------------------------
struct Speaker
{
    float x          = 50.0f;   // metres, world X
    float y          = 50.0f;   // metres, world Y
    float gainDB     = 0.0f;    // <= 0 dB, 1 dB steps
    float delayMs    = 0.0f;    // milliseconds
    bool  polarityInverted   = false;   // Normal / Reverse
    bool  reverseOrientation = false;   // Forward (+x) / Reverse (-x)
    // Where the cabinet points, in degrees counter-clockwise from +x, so 0
    // fires right and 90 fires up the plan. Snapped to 5 degrees by the
    // rotation handle. Orientation only - the cabinet keeps its size.
    float rotationDeg        = 0.0f;
    bool  enabled            = true;
    // Where the cabinet sits vertically, and how far it is aimed down.
    // These describe the rig for the elevation view and for reports; the SPL
    // prediction is a horizontal plane and does not read them. Saying so
    // plainly matters: the measured data is one horizontal plane per device,
    // so there is no vertical pattern to predict with. Tilt is positive down.
    float baseHeightM = 0.0f;   // underside of the cabinet above the ground
    float tiltDeg     = 0.0f;   // down-tilt, degrees
    // Which device this unit is: 0 = Q21S, 2 = BEM2inch (matches
    // MeasurementData::Source / AcousticEngine::MeasurementSourceId). Each
    // unit is simulated with its OWN model's directivity - a scene can freely
    // mix both; they are never blended into one shared pattern.
    int   model              = 0;
    // Crossover on this cabinet: subs low-passed, tops high-passed, and so on.
    // Applied to the complex pressure before summation, so it moves phase as
    // well as level - see filterResponse().
    SpeakerFilter filter;
    // Draw this unit's aiming line. Display only - the heading is still
    // rotationDeg whether the line is on screen or not. A sub has no ray, so
    // this stalk and knob is the only handle it can be turned by; hiding it is
    // for a plan already crowded with them, not for giving up the aim.
    bool  showAimLine        = true;
};

// Whether a model is a subwoofer. The Q21S is the sub; the 2" horn is not.
//
// Subs carry no aiming ray. A ray says "this box is pointed here", which only
// means something for a box with a front axis - and at sub frequencies a single
// cabinet is close to omnidirectional, so there is no axis to draw. The
// industry tools agree: ArrayCalc and EASE Focus steer low end by array
// geometry and per-box delay and then show the resulting dispersion, rather
// than by pointing a cabinet.
inline bool isSubwooferModel (int model) noexcept
{
    return model != 2;      // 0 = Q21S (sub); 2 = BEM 2inch
}

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

// Q21S product cabinet dimensions (metres). Plan view uses width x depth.
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
    // Top view per the placement sketch: 900 mm across the baffle by 1000 mm
    // front to back. NOTE this disagrees with the spec sheet quoted earlier
    // (W x H x D = 1546 x 679 x 1024 mm, 60.86 x 26.73 x 40.31 in), which is
    // a larger cabinet entirely. The sketch wins here because it is what the
    // placement UI is being drawn against; if the spec sheet is the truth,
    // these three numbers are the only place to change.
    constexpr float widthM  = 0.900f;   // 900 mm - left/right, across the baffle
    constexpr float heightM = 0.679f;   // 679 mm - vertical (unchanged: the
                                        //   sketch gives the top view only)
    constexpr float depthM  = 1.000f;   // 1000 mm - front/back (firing axis)
    constexpr float halfExtentM = depthM * 0.5f;  // singularity floor for 1/r
}

// 2" HF horn enclosure (metres). Much smaller than the Q21S in every axis,
// which matters twice over: the plan marker is drawn to scale against the
// field, and halfExtentM sets how close the 1/r law is allowed to get before
// it is clamped. Using the Q21S's 512 mm floor for a 150 mm-deep horn would
// have flattened its near field over half a metre of empty air.
namespace BEM2inchCabinet
{
    constexpr float widthM  = 0.500f;    // 500 mm - left/right, across the mouth
    constexpr float heightM = 0.2765f;   // 276.5 mm - vertical (unchanged: the
                                         //   sketch gives the top view only)
    constexpr float depthM  = 0.500f;    // 500 mm - front/back (firing axis)
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

// Per-model frequency catalogues - native BEM xlsx bands only (no interpolated
// extras). Q21S and BEM2inch are two completely separate devices: each keeps
// its own array below and the two are never merged or shared.
static constexpr double kQ21SFrequencies[] = {
    20, 29, 52, 60, 81, 98, 153, 198, 256, 309, 352, 400, 401
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

// Isolated per-model catalogue lookup - never returns a blended list.
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
// by setActiveFrequencyCatalogue() whenever the measurement source changes -
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
    MeasuredPolar = 4,  // measured horizontal polar readings (.xlsx)
    // Elevation views are DRAWINGS of the rig, not predictions. The measured
    // data is one horizontal plane per device, so there is no vertical
    // directivity to solve with; these show cabinet heights and tilt to
    // scale and deliberately carry no heatmap.
    ElevationFront = 5, // looking along the firing axis: width x height
    ElevationSide  = 6  // looking from the side: depth x height
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

    // Measured BEM directivity tables, one array per device - always applied
    // (useMeasuredDirectivity is forced on - no UI toggle). Each Speaker picks
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
    // in the product path - full-world measured-directivity simulation instead).
    bool   usedBemField = false;
    double bemOriginX   = 0.0;
    double bemOriginY   = 0.0;
};

// ---------------------------------------------------------------------------
// AcousticEngine - BEM polar x 1/r over the full world, coherent array sum.
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

    // Point probe (no full grid). intensityDb = 10-log10(I); absDb when measured.
    // Used for Frequency Response curves across mics / frequencies.
    static bool sampleIntensityAt (const SimParams& p, float x, float y,
                                   float& intensityDb, float& absDb);

    // dB thresholds for the spec colour contour (0, -6, ... -36).
    // The step the renderer actually draws is ColourMaps::kContourStepDB.
    static constexpr double kColourStepDB = 6.0;

private:
    // Orientation directivity: subtle cardioid-like bias toward facing dir.
    static double orientationGain (double facingAngle, double angleToPoint);
};

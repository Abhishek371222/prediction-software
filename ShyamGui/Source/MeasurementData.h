#pragma once
#include <JuceHeader.h>
#include "AcousticEngine.h"
#include "EmbeddedQ21SData.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>

#ifndef M_PI
 #define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// MeasurementData - loads Q21S BEM-derived horizontal polar readings from
// MeasurementIntegrationPack (CSV), or from EmbeddedQ21SData baked into the EXE
// when no sidecar Data/ folder is present. Polars are exported from
// BEM_Data_10m/Q21S_10m_PolarPlotData.xlsx (true ±5 m field, native rear).
// Converts to on-axis-normalised linear gain:
//     R = 10^((SPL - SPL_onAxis)/20)
// Header-only (inline) so it can be included by more than one translation
// unit without an ODR violation.
// ---------------------------------------------------------------------------

struct MeasuredCurve
{
    std::vector<float> angleDeg;   // sorted ascending [0, 360)
    std::vector<float> splAbs;     // absolute dB from the reading file
    std::vector<float> R;          // linear gain, on-axis normalised (engine)
    float onAxisR   = 0.0f;        // = 1 when ok
    float onAxisSpl = 0.0f;        // absolute dB at on-axis
    float peakSpl   = 0.0f;        // absolute dB at peak
    float beamwidthDeg = 0.0f;     // full -6 dB beamwidth (deg), 0 if unknown
    bool  trustForModel = true;    // false for room-mode / corrupt sweeps
    juce::String sourceFile;       // CSV or xlsx name (traceability)
    bool  ok = false;
};

struct MeasuredFreq
{
    int           hz = 0;
    MeasuredCurve d05;             // 0.5 m
    MeasuredCurve d1;              // 1.0 m
    MeasuredCurve d2;              // 2.0 m (Room / ShyamGuild only)
    float         rMax = 1.0f;     // ceil(max R over curves) for ring scaling
    float         beamwidthDeg = 0.0f;
    bool          trustForModel = true;
    juce::String  sourceFile;
    bool          ok = false;
};

struct MeasuredSet
{
    std::vector<MeasuredFreq> freqs;
    bool ok = false;
    int          source = 0;          // OpenField / Gylt
    juce::String sourceName;          // "Q21S" / "Room"
    juce::String packPath;            // MeasurementIntegrationPack/Data if used
};

namespace MeasurementData
{
    // Raw degree / dB-SPL pair list parsed from one sheet.
    struct RawSweep
    {
        std::vector<float> deg;
        std::vector<float> spl;
        bool ok = false;
    };

    inline int columnIndexFromCellRef (const juce::String& cellRef)
    {
        // "B12" -> column 'B'. Returns 0 for A, 1 for B, -1 if none.
        if (cellRef.isEmpty()) return -1;
        const juce::juce_wchar c = juce::CharacterFunctions::toUpperCase (cellRef[0]);
        if (c < 'A' || c > 'Z') return -1;
        return (int) (c - 'A');
    }

    inline RawSweep parseSheetXml (const juce::String& xml)
    {
        RawSweep out;
        std::unique_ptr<juce::XmlElement> root (juce::XmlDocument::parse (xml));
        if (root == nullptr) return out;

        juce::XmlElement* sheetData = root->getChildByName ("sheetData");
        if (sheetData == nullptr) return out;

        for (auto* row = sheetData->getChildByName ("row"); row != nullptr;
             row = row->getNextElementWithTagName ("row"))
        {
            const int rowIndex = row->getIntAttribute ("r", -1);
            if (rowIndex == 1) continue;   // header row (Degree / dBSPL)

            float a = std::numeric_limits<float>::quiet_NaN();
            float b = std::numeric_limits<float>::quiet_NaN();

            for (auto* c = row->getChildByName ("c"); c != nullptr;
                 c = c->getNextElementWithTagName ("c"))
            {
                // Skip shared-string cells (text); we only want numeric values.
                if (c->getStringAttribute ("t") == "s") continue;

                auto* v = c->getChildByName ("v");
                if (v == nullptr) continue;

                const int col = columnIndexFromCellRef (c->getStringAttribute ("r"));
                const float val = (float) v->getAllSubText().getDoubleValue();
                if      (col == 0) a = val;
                else if (col == 1) b = val;
            }

            if (! std::isnan (a) && ! std::isnan (b))
            {
                out.deg.push_back (a);
                out.spl.push_back (b);
            }
        }

        out.ok = ! out.deg.empty();
        return out;
    }

    inline RawSweep loadXlsxSweep (const juce::File& file)
    {
        RawSweep out;
        if (! file.existsAsFile()) return out;

        juce::ZipFile zip (file);
        const int idx = zip.getIndexOfFileName ("xl/worksheets/sheet1.xml");
        if (idx < 0) return out;

        std::unique_ptr<juce::InputStream> in (zip.createStreamForEntry (idx));
        if (in == nullptr) return out;

        const juce::String xml = in->readEntireStreamAsString();
        return parseSheetXml (xml);
    }

    inline float energyAverageDB (const std::vector<float>& a, const std::vector<float>& b)
    {
        double acc = 0.0; size_t n = 0;
        for (float v : a) { acc += std::pow (10.0, v / 10.0); ++n; }
        for (float v : b) { acc += std::pow (10.0, v / 10.0); ++n; }
        if (n == 0) return 0.0f;
        return (float) (10.0 * std::log10 (acc / (double) n));
    }

    // SPL nearest the on-axis (0 deg) sample in a raw sweep, or NaN if empty.
    inline float onAxisSPL (const RawSweep& s)
    {
        float best = std::numeric_limits<float>::quiet_NaN();
        float bestAbs = 1.0e9f;
        for (size_t i = 0; i < s.deg.size(); ++i)
        {
            float wrapped = std::fmod (s.deg[i] + 180.0f, 360.0f) - 180.0f; // -> [-180,180)
            if (std::abs (wrapped) < bestAbs) { bestAbs = std::abs (wrapped); best = s.spl[i]; }
        }
        return best;
    }

    // Build one curve from one sweep only — never shares a reference with
    // another distance (that was mixing 0.5 m and 1 m levels).
    inline MeasuredCurve buildCurve (const RawSweep& sweep)
    {
        MeasuredCurve c;
        if (! sweep.ok) return c;

        std::vector<size_t> order (sweep.deg.size());
        for (size_t i = 0; i < order.size(); ++i) order[i] = i;
        std::sort (order.begin(), order.end(),
                   [&] (size_t l, size_t r) { return sweep.deg[l] < sweep.deg[r]; });

        bool hasZero = false;
        for (size_t k : order)
        {
            float d = sweep.deg[k];
            while (d < 0.0f)    d += 360.0f;
            while (d >= 360.0f) d -= 360.0f;
            if (d < 0.5f || d > 359.5f) hasZero = true;
        }

        c.angleDeg.reserve (order.size());
        c.splAbs.reserve (order.size());
        c.R.reserve (order.size());

        float bestAbs = 1.0e9f;
        float onAxisSpl = 0.0f;
        float peakSpl = -1.0e9f;

        for (size_t k : order)
        {
            float d = sweep.deg[k];
            while (d < 0.0f)    d += 360.0f;
            while (d >= 360.0f) d -= 360.0f;
            if (hasZero && d > 359.5f) continue;   // drop bad 360 deg seam

            const float spl = sweep.spl[k];
            c.angleDeg.push_back (d);
            c.splAbs.push_back (spl);
            peakSpl = std::max (peakSpl, spl);

            float wrapped = std::fmod (d + 180.0f, 360.0f) - 180.0f;
            if (std::abs (wrapped) < bestAbs) { bestAbs = std::abs (wrapped); onAxisSpl = spl; }
        }

        if (c.angleDeg.size() < 2) return c;

        c.onAxisSpl = onAxisSpl;
        c.peakSpl   = peakSpl;
        // Engine: on-axis gain = 1 (each curve uses its own on-axis level).
        for (float spl : c.splAbs)
            c.R.push_back ((float) std::pow (10.0, (spl - onAxisSpl) / 20.0));
        c.onAxisR = 1.0f;
        c.ok = true;
        return c;
    }

    // Full -6 dB beamwidth from on-axis (walk both sides until rel < -6 dB).
    inline float computeBeamwidthDeg (const MeasuredCurve& c)
    {
        if (! c.ok || c.angleDeg.size() < 2) return 0.0f;

        auto relInterp = [&] (float deg) -> float
        {
            while (deg < 0.0f)    deg += 360.0f;
            while (deg >= 360.0f) deg -= 360.0f;
            const auto& A = c.angleDeg;
            const auto& S = c.splAbs;
            if (deg <= A.front() || deg >= A.back())
            {
                const float a0 = A.back(), a1 = A.front() + 360.0f;
                const float d  = deg < A.front() ? deg + 360.0f : deg;
                const float t  = (d - a0) / std::max (1.0e-6f, a1 - a0);
                return (S.back() + t * (S.front() - S.back())) - c.onAxisSpl;
            }
            for (size_t i = 1; i < A.size(); ++i)
                if (deg <= A[i])
                {
                    const float t = (deg - A[i - 1]) / std::max (1.0e-6f, A[i] - A[i - 1]);
                    return (S[i - 1] + t * (S[i] - S[i - 1])) - c.onAxisSpl;
                }
            return S.back() - c.onAxisSpl;
        };

        float left = 0.0f, right = 0.0f;
        for (float a = 0.0f; a <= 180.0f; a += 0.5f)
            if (relInterp (a) < -6.0f) { left = a; break; }
            else left = a;
        for (float a = 0.0f; a <= 180.0f; a += 0.5f)
            if (relInterp (360.0f - a) < -6.0f) { right = a; break; }
            else right = a;
        return left + right;
    }

    inline void finalizeCurve (MeasuredCurve& c, bool trust, const juce::String& sourceFile)
    {
        if (! c.ok) return;
        c.trustForModel = trust;
        c.sourceFile    = sourceFile;
        c.beamwidthDeg  = computeBeamwidthDeg (c);
    }

    inline juce::File fileFor (const juce::File& folder, int hz, const juce::String& dist)
    {
        return folder.getChildFile ("Frequency_" + juce::String (hz) + "_"
                                    + dist + "Horizantal.xlsx");
    }

    // CLIO 2D Directivity Analysis PNG (Room readings).
    //
    // WHY spikes happened before:
    //  1) The vertical axis (0 deg / 180 deg) is solid black from dB *labels*,
    //     not the curve — ray-casting there produced wild radii.
    //  2) Grid ticks and the outer frame were sometimes picked as the curve.
    //  3) Connecting those bad samples made "peak / starburst" plots.
    //
    // Fix: ignore the label axis, accept only a thin black stroke with white
    // just outside it, interpolate gaps, then median-filter + smooth.
    namespace detail
    {
        inline bool isDark (const juce::Image& img, int x, int y)
        {
            if (x < 0 || y < 0 || x >= img.getWidth() || y >= img.getHeight()) return false;
            return img.getPixelAt (x, y).getBrightness() < 0.16f;   // pure black stroke
        }

        inline bool isWhite (const juce::Image& img, int x, int y)
        {
            if (x < 0 || y < 0 || x >= img.getWidth() || y >= img.getHeight()) return false;
            return img.getPixelAt (x, y).getBrightness() > 0.82f;
        }

        // Vertical label axis is unusable (dB numbers are solid black).
        inline bool isLabelAxis (int deg)
        {
            const int d = ((deg % 360) + 360) % 360;
            return d <= 12 || d >= 348 || (d >= 168 && d <= 192);
        }

        // Outermost thin curve stroke along one ray, or -1 if none.
        inline int curveRadiusAt (const juce::Image& img, float cx, float cy, float deg)
        {
            const float rad  = (deg - 90.0f) * (float) M_PI / 180.0f;
            const float cosA = std::cos (rad);
            const float sinA = std::sin (rad);

            int best = -1;
            bool inB = false;
            int s = 0;
            // Curve lives between ~-12 dB and +6 dB rings on CLIO plots.
            for (int r = 170; r <= 255; ++r)
            {
                const int x = (int) std::lround (cx + (float) r * cosA);
                const int y = (int) std::lround (cy + (float) r * sinA);
                const bool b = isDark (img, x, y);
                if (b && ! inB) { s = r; inB = true; }
                if (! b && inB)
                {
                    const int span = r - s;
                    // Curve stroke is 2–5 px; long runs are labels/artifacts.
                    if (span >= 2 && span <= 5)
                    {
                        const int xo = (int) std::lround (cx + (float) (r + 4) * cosA);
                        const int yo = (int) std::lround (cy + (float) (r + 4) * sinA);
                        if (isWhite (img, xo, yo))
                            best = (s + r - 1) / 2;
                    }
                    inB = false;
                }
            }
            return best;
        }

        inline float clioRadiusToDb (float r)
        {
            constexpr float rMin = 58.0f, rMax = 278.0f;
            return -24.0f + (r - rMin) * 30.0f / (rMax - rMin);
        }

        inline void medianFilterCircular (std::vector<float>& a, int halfWin)
        {
            const int n = (int) a.size();
            if (n == 0) return;
            std::vector<float> out (a.size());
            std::vector<float> win;
            win.reserve ((size_t) halfWin * 2 + 1);
            for (int i = 0; i < n; ++i)
            {
                win.clear();
                for (int k = -halfWin; k <= halfWin; ++k)
                    win.push_back (a[(size_t) ((i + k + n * 8) % n)]);
                std::nth_element (win.begin(), win.begin() + (int) win.size() / 2, win.end());
                out[(size_t) i] = win[win.size() / 2];
            }
            a.swap (out);
        }

        inline void smoothCircular (std::vector<float>& a, int halfWin)
        {
            const int n = (int) a.size();
            if (n == 0) return;
            std::vector<float> out (a.size());
            const float inv = 1.0f / (float) (2 * halfWin + 1);
            for (int i = 0; i < n; ++i)
            {
                float s = 0.0f;
                for (int k = -halfWin; k <= halfWin; ++k)
                    s += a[(size_t) ((i + k + n * 8) % n)];
                out[(size_t) i] = s * inv;
            }
            a.swap (out);
        }
    }

    // Measurement environments. Product default is Q21S BEM polars (OpenField).
    // Legacy Room (Gylt) maps to ShyamGuild CSVs — kept for pack compatibility.
    enum Source { OpenField = 0, Gylt = 1 };   // Q21S / Room (legacy)

    inline const char* packSetName (int source)
    {
        return source == Gylt ? "ShyamGuild" : "Q21S";
    }

    // Quality flags: Q21S BEM arcs @ 0.5/1.0/2.0 m for 1/3-oct catalogue (20–200).
    // >200 Hz / 500 Hz: display OK; weaker for model (BEM ceiling / extrapolation).
    inline bool isTrustedForModel (int source, int hz, float distanceM = 0.5f)
    {
        if (source == Gylt)
        {
            if (hz == 30) return false;
            if (distanceM > 1.5f) return false;           // room gain at 2 m
            return hz == 80 || hz == 200 || hz == 500;
        }
        // Q21S BEM 10 m field — native xlsx bands only (20…401).
        for (int i = 0; i < kNumSupportedFrequencies; ++i)
            if ((int) std::lround (kSupportedFrequencies[i]) == hz)
                return true;
        return false;
    }

    inline juce::String csvFileName (const juce::String& setName, int hz, float distanceM)
    {
        juce::String distTag = "1p0";
        if (std::abs (distanceM - 0.5f) < 1.0e-3f)      distTag = "0p5";
        else if (std::abs (distanceM - 2.0f) < 1.0e-3f) distTag = "2p0";
        return setName + "_" + juce::String (hz) + "Hz_" + distTag + "m.csv";
    }

    inline RawSweep loadCsvSweepText (const juce::String& text)
    {
        RawSweep out;
        juce::StringArray lines;
        lines.addLines (text);
        for (int i = 0; i < lines.size(); ++i)
        {
            auto line = lines[i].trim();
            if (line.isEmpty() || line.startsWithIgnoreCase ("degree"))
                continue;
            auto tokens = juce::StringArray::fromTokens (line, ",", "\"");
            if (tokens.size() < 2) continue;
            out.deg.push_back (tokens[0].getFloatValue());
            out.spl.push_back (tokens[1].getFloatValue());
        }
        out.ok = out.deg.size() >= 2;
        return out;
    }

    inline RawSweep loadCsvSweep (const juce::File& file)
    {
        if (! file.existsAsFile()) return {};
        return loadCsvSweepText (file.loadFileAsString());
    }

    inline RawSweep loadCsvSweepEmbedded (const char* fileName)
    {
        if (auto* e = EmbeddedQ21S::find (fileName))
            return loadCsvSweepText (juce::String::fromUTF8 (e->data, e->size));
        return {};
    }

    inline bool hasCsvOnDiskOrEmbedded (const juce::File& packDir,
                                        const juce::String& csvName)
    {
        if (packDir.isDirectory() && packDir.getChildFile (csvName).existsAsFile())
            return true;
        return EmbeddedQ21S::hasFile (csvName.toRawUTF8());
    }

    // Prefer MeasurementIntegrationPack CSV (disk or baked into EXE);
    // fall back to legacy .xlsx when neither is present.
    inline MeasuredCurve loadCurvePreferPack (const juce::File& packDir,
                                              const juce::File& xlsxFolder,
                                              int source, int hz, float distanceM)
    {
        MeasuredCurve c;
        const juce::String setName = packSetName (source);
        const juce::String csvName = csvFileName (setName, hz, distanceM);
        const juce::File csv = packDir.getChildFile (csvName);

        juce::String srcName;
        if (csv.existsAsFile())
        {
            c = buildCurve (loadCsvSweep (csv));
            srcName = csvName;
        }
        else if (auto emb = loadCsvSweepEmbedded (csvName.toRawUTF8()); emb.ok)
        {
            c = buildCurve (emb);
            srcName = csvName + " (embedded)";
        }
        else
        {
            const juce::String dist = (std::abs (distanceM - 0.5f) < 1.0e-3f) ? "0.5"
                                    : (std::abs (distanceM - 2.0f) < 1.0e-3f) ? "2" : "1";
            const juce::File xlsx = fileFor (xlsxFolder, hz, dist);
            c = buildCurve (loadXlsxSweep (xlsx));
            srcName = xlsx.getFileName();
        }

        finalizeCurve (c, isTrustedForModel (source, hz, distanceM), srcName);
        return c;
    }

    inline int parseHzFromName (const juce::String& name)
    {
        for (int i = 0; i < name.length(); ++i)
        {
            juce::String digits;
            while (i < name.length() && juce::CharacterFunctions::isDigit (name[i]))
                digits += name[i++];
            if (digits.isNotEmpty() && i < name.length()
                && name.substring (i).startsWithIgnoreCase ("Hz"))
                return digits.getIntValue();
        }
        return -1;
    }

    inline juce::File pngFileFor (const juce::File& folder, int hz)
    {
        // "XN 18 SUB Horizontal 40Hzs.png" (note occasional double-space before 30).
        // Match exact Hz (avoid "30Hz" matching inside "130Hz").
        auto matches = folder.findChildFiles (juce::File::findFiles, false, "*.png");
        for (const auto& f : matches)
            if (parseHzFromName (f.getFileName()) == hz)
                return f;
        return {};
    }

    // Q21S = BEM polars for every UI catalogue frequency present as CSV.
    // Legacy Gylt = ShyamGuild room set. Plots come from pack CSVs (not PNGs).
    inline std::vector<int> discoverFrequencies (const juce::File& packDir,
                                                 const juce::File& xlsxFolder,
                                                 int source)
    {
        std::vector<int> out;
        const juce::String setName = packSetName (source);

        auto hasSweep = [&] (int hz) -> bool
        {
            const bool hasCsv =
                   hasCsvOnDiskOrEmbedded (packDir, csvFileName (setName, hz, 0.5f))
                || hasCsvOnDiskOrEmbedded (packDir, csvFileName (setName, hz, 1.0f))
                || hasCsvOnDiskOrEmbedded (packDir, csvFileName (setName, hz, 2.0f));
            const bool hasXlsx = fileFor (xlsxFolder, hz, "1").existsAsFile()
                              || fileFor (xlsxFolder, hz, "0.5").existsAsFile()
                              || fileFor (xlsxFolder, hz, "2").existsAsFile();
            return hasCsv || hasXlsx;
        };

        if (source == Gylt)
        {
            const int roomFreqs[] = { 30, 80, 200, 500 };
            for (int hz : roomFreqs)
                if (hasSweep (hz))
                    out.push_back (hz);
            return out;
        }

        // Q21S / OpenField — full software catalogue
        for (int i = 0; i < kNumSupportedFrequencies; ++i)
        {
            const int hz = (int) std::lround (kSupportedFrequencies[i]);
            if (hasSweep (hz))
                out.push_back (hz);
        }
        return out;
    }

    inline RawSweep loadClioPolarPng (const juce::File& file)
    {
        RawSweep out;
        if (! file.existsAsFile()) return out;

        juce::Image img = juce::ImageFileFormat::loadFrom (file);
        if (! img.isValid()) return out;

        constexpr float cx = 512.0f, cy = 395.0f;
        std::vector<float> radius (360, -1.0f);

        for (int deg = 0; deg < 360; ++deg)
        {
            if (detail::isLabelAxis (deg)) continue;   // dB labels, not the curve
            const int r = detail::curveRadiusAt (img, cx, cy, (float) deg);
            if (r > 0) radius[(size_t) deg] = (float) r;
        }

        // Fill gaps (label axis + missed samples) by circular linear interp.
        for (int i = 0; i < 360; ++i)
        {
            if (radius[(size_t) i] > 0.0f) continue;
            int prev = -1, next = -1;
            for (int k = 1; k < 180; ++k)
                if (radius[(size_t) ((i - k + 360) % 360)] > 0.0f) { prev = (i - k + 360) % 360; break; }
            for (int k = 1; k < 180; ++k)
                if (radius[(size_t) ((i + k) % 360)] > 0.0f) { next = (i + k) % 360; break; }
            if (prev >= 0 && next >= 0)
            {
                const int d0 = (i - prev + 360) % 360;
                const int d1 = (next - i + 360) % 360;
                const float t = (float) d0 / (float) juce::jmax (1, d0 + d1);
                radius[(size_t) i] = radius[(size_t) prev] * (1.0f - t)
                                   + radius[(size_t) next] * t;
            }
            else if (prev >= 0) radius[(size_t) i] = radius[(size_t) prev];
            else if (next >= 0) radius[(size_t) i] = radius[(size_t) next];
        }

        int valid = 0;
        for (float r : radius) if (r > 0.0f) ++valid;
        if (valid < 90) return out;

        detail::medianFilterCircular (radius, 4);
        detail::smoothCircular (radius, 5);

        // Relative dB with peak = 0 (CLIO convention).
        std::vector<float> db (360);
        float peak = -1.0e9f;
        for (int i = 0; i < 360; ++i)
        {
            db[(size_t) i] = detail::clioRadiusToDb (radius[(size_t) i]);
            peak = std::max (peak, db[(size_t) i]);
        }

        out.deg.reserve (360);
        out.spl.reserve (360);
        for (int i = 0; i < 360; ++i)
        {
            out.deg.push_back ((float) i);
            out.spl.push_back (db[(size_t) i] - peak);   // peak -> 0 dB
        }
        out.ok = true;
        return out;
    }

    inline const char* sourceName (int source)
    {
        juce::ignoreUnused (source);
        // Product UI: Ground Plane measured set (Room removed from UI).
        return "Ground Plane";
    }

    // Resolve a dataset folder portably (Windows/macOS).
    // Prefer paths near the running exe / CWD (the cloned repo), then fall back
    // to a legacy absolute Windows path. Checking legacy first is wrong on
    // machines that still have an old D:\shayam gui tree without Q21S CSVs —
    // that made Windows heatmaps diverge from macOS.
    // Optional markerFile must exist inside a candidate before it is accepted.
    inline juce::File resolveSourceFolder (const juce::File& legacyDevPath,
                                           const std::vector<juce::String>& rels,
                                           const juce::String& markerFile = {})
    {
        auto accept = [&] (const juce::File& cand) -> bool
        {
            if (! cand.isDirectory()) return false;
            return markerFile.isEmpty() || cand.getChildFile (markerFile).existsAsFile();
        };

        auto searchFrom = [&] (juce::File root) -> juce::File
        {
            for (int depth = 0; depth < 14; ++depth)
            {
                for (const auto& rel : rels)
                {
                    const juce::File cand = root.getChildFile (rel);
                    if (accept (cand)) return cand;
                }

               #if JUCE_MAC || JUCE_WINDOWS
                // Packaged builds: data may live under Resources next to the exe/.app
                const juce::File resources = root.getChildFile ("Resources");
                if (resources.isDirectory())
                {
                    for (const auto& rel : rels)
                    {
                        const juce::File cand = resources.getChildFile (rel);
                        if (accept (cand)) return cand;
                    }
                }
               #endif

                const juce::File parent = root.getParentDirectory();
                if (parent == root) break;
                root = parent;
            }
            return {};
        };

        // Prefer exe location over CWD: `start` / Explorer launches often have
        // an unrelated working directory on Windows.
        const juce::File exeDir = juce::File::getSpecialLocation (
                                     juce::File::currentExecutableFile)
                                     .getParentDirectory();
        if (auto f = searchFrom (exeDir); f != juce::File())
            return f;

        if (auto f = searchFrom (juce::File::getCurrentWorkingDirectory()); f != juce::File())
            return f;

        if (accept (legacyDevPath))
            return legacyDevPath;

        return {};   // missing — never return a folder that failed the marker check
    }

    // MeasurementIntegrationPack/Data — primary source of real CSV readings.
    // Marker is a native Q21S far-field polar so old Factory/ShyamGuild-only
    // packs (which also have manifest.csv) are never selected.
    // Include ShyamGui/… so an EXE under version-archive/artifacts still finds
    // the repo pack when walking up to the clone root.
    inline juce::File packDataFolder()
    {
        return resolveSourceFolder (
            juce::File ("D:\\shayam gui\\prediction software\\MeasurementIntegrationPack\\Data"),
            { "ShyamGui/prediction software/MeasurementIntegrationPack/Data",
              "prediction software/MeasurementIntegrationPack/Data",
              "MeasurementIntegrationPack/Data",
              "Data" },
            "Q21S_52Hz_2p0m.csv");
    }

    // Legacy .xlsx folders (fallback when a pack CSV is missing).
    inline juce::File folderForSource (int source)
    {
        const juce::File pack = packDataFolder();
        if (pack.isDirectory()
            && pack.getChildFile ("Q21S_52Hz_2p0m.csv").existsAsFile())
            return pack;

        // Room = real guild measurements (.xlsx).
        if (source == Gylt)
            return resolveSourceFolder (
                juce::File ("D:\\shayam gui\\shyamGuildMeasurements"),
                { "shyamGuildMeasurements",
                  "ShyamGui/shyamGuildMeasurements" });

        // Q21S OpenField — pack CSVs only (no legacy Factory xlsx required).
        return resolveSourceFolder (
            juce::File ("D:\\shayam gui\\prediction software\\MeasurementIntegrationPack\\Data"),
            { "ShyamGui/prediction software/MeasurementIntegrationPack/Data",
              "prediction software/MeasurementIntegrationPack/Data",
              "MeasurementIntegrationPack/Data",
              "Data" },
            "Q21S_52Hz_2p0m.csv");
    }

    inline juce::File xlsxFolderForSource (int source)
    {
        if (source == Gylt)
            return resolveSourceFolder (
                juce::File ("D:\\shayam gui\\shyamGuildMeasurements"),
                { "shyamGuildMeasurements" });

        // Q21S: canonical BEM 10 m workbook folder (per-Hz xlsx pack).
        return resolveSourceFolder (
            juce::File ("D:\\shayam gui\\BEM_Data_10m"),
            { "BEM_Data_10m", "../BEM_Data_10m", "ShyamGui/../BEM_Data_10m" },
            "52Hz.xlsx");
    }

    // Backwards-compatible default (Open Field).
    inline juce::File resolveFolder() { return folderForSource (OpenField); }

    // Linear-interpolate R at an arbitrary degree from a (sorted) curve.
    // Handles full-circle wrap so 359° -> 0° is continuous.
    inline float interpCurve (const MeasuredCurve& cv, float deg)
    {
        const auto& A = cv.angleDeg;
        const auto& R = cv.R;
        if (A.empty()) return 0.0f;

        while (deg < 0.0f)    deg += 360.0f;
        while (deg >= 360.0f) deg -= 360.0f;

        if (A.size() == 1) return R.front();

        // Wrap segment between last and first sample.
        if (deg <= A.front() || deg >= A.back())
        {
            const float a0 = A.back(), a1 = A.front() + 360.0f;
            const float d  = deg < A.front() ? deg + 360.0f : deg;
            const float t  = (d - a0) / std::max (1.0e-6f, a1 - a0);
            return R.back() + t * (R.front() - R.back());
        }

        for (size_t i = 1; i < A.size(); ++i)
            if (deg <= A[i])
            {
                const float t = (deg - A[i - 1]) / std::max (1.0e-6f, A[i] - A[i - 1]);
                return R[i - 1] + t * (R[i] - R[i - 1]);
            }
        return R.back();
    }

    // Interpolate absolute SPL (dB) vs angle — CLIO-style (dB domain, not linear R).
    inline float interpSplDb (const MeasuredCurve& cv, float deg)
    {
        const auto& A = cv.angleDeg;
        const auto& S = cv.splAbs;
        if (A.empty() || S.size() != A.size()) return 0.0f;

        while (deg < 0.0f)    deg += 360.0f;
        while (deg >= 360.0f) deg -= 360.0f;

        if (A.size() == 1) return S.front();

        auto sampleAt = [&] (float a0, float s0, float a1, float s1, float d) -> float
        {
            const float span = std::max (1.0e-6f, a1 - a0);
            float t = (d - a0) / span;
            t = t * t * (3.0f - 2.0f * t);   // smoothstep between samples
            return s0 + t * (s1 - s0);
        };

        if (deg <= A.front() || deg >= A.back())
        {
            const float a0 = A.back(), a1 = A.front() + 360.0f;
            const float d  = deg < A.front() ? deg + 360.0f : deg;
            return sampleAt (a0, S.back(), a1, S.front(), d);
        }

        for (size_t i = 1; i < A.size(); ++i)
            if (deg <= A[i])
                return sampleAt (A[i - 1], S[i - 1], A[i], S[i], deg);
        return S.back();
    }

    // Exact measured points, on-axis normalised (CLIO): rel_dB = SPL - SPL(0°).
    // Never invents angles — plot/interpolate through these only.
    inline void measuredPointsDb (const MeasuredCurve& cv,
                                  std::vector<float>& degs, std::vector<float>& dbs)
    {
        degs.clear(); dbs.clear();
        if (! cv.ok || cv.splAbs.size() < 2) return;

        degs.reserve (cv.angleDeg.size());
        dbs.reserve (cv.splAbs.size());
        for (size_t i = 0; i < cv.angleDeg.size(); ++i)
        {
            degs.push_back (cv.angleDeg[i]);
            dbs.push_back (cv.splAbs[i] - cv.onAxisSpl);
        }
    }

    // Upsample one curve to 1 deg for engine / dense stroke fallback.
    // On-axis = 0 dB (CLIO convention). Single curve only — never merges distances.
    inline void sampleCurveDb (const MeasuredCurve& cv,
                               std::vector<float>& degs, std::vector<float>& dbs)
    {
        degs.clear(); dbs.clear();
        if (! cv.ok || cv.splAbs.size() < 2) return;

        degs.reserve (360);
        dbs.reserve (360);
        for (int deg = 0; deg < 360; ++deg)
        {
            const float spl = interpSplDb (cv, (float) deg);
            degs.push_back ((float) deg);
            dbs.push_back (spl - cv.onAxisSpl);
        }
    }

    // Smooth closed polar for Directivity / Measured Polar stroke (not a polyline).
    // Median + circular average in dB, then 0.5° samples so the plot is a curve.
    inline void polarStrokeSamples (const MeasuredCurve& cv,
                                    std::vector<float>& degs, std::vector<float>& dbs)
    {
        sampleCurveDb (cv, degs, dbs);
        if (dbs.size() < 8) return;

        detail::medianFilterCircular (dbs, 2);
        detail::smoothCircular (dbs, 6);   // ~13° window — CLIO-style smooth lobe

        const int n = (int) dbs.size();
        std::vector<float> aOut, dOut;
        aOut.reserve ((size_t) n * 2);
        dOut.reserve ((size_t) n * 2);
        for (int i = 0; i < n; ++i)
        {
            const int j = (i + 1) % n;
            aOut.push_back (degs[(size_t) i]);
            dOut.push_back (dbs[(size_t) i]);
            float midA = 0.5f * (degs[(size_t) i] + degs[(size_t) j]);
            if (j == 0) midA = 0.5f * (degs[(size_t) i] + degs[(size_t) j] + 360.0f);
            if (midA >= 360.0f) midA -= 360.0f;
            aOut.push_back (midA);
            dOut.push_back (0.5f * (dbs[(size_t) i] + dbs[(size_t) j]));
        }
        degs.swap (aOut);
        dbs.swap (dOut);
    }

    // Curve at a requested measurement distance. Falls back to nearest
    // available distance (never blends distances).
    inline const MeasuredCurve* curveAtDistance (const MeasuredFreq& mf, float distanceM)
    {
        const MeasuredCurve* exact = nullptr;
        if (std::abs (distanceM - 0.5f) < 1.0e-3f)      exact = mf.d05.ok ? &mf.d05 : nullptr;
        else if (std::abs (distanceM - 2.0f) < 1.0e-3f) exact = mf.d2.ok  ? &mf.d2  : nullptr;
        else                                             exact = mf.d1.ok  ? &mf.d1  : nullptr;
        if (exact != nullptr) return exact;

        // Far-field requests prefer larger arcs; near-field prefers closer.
        if (distanceM + 1.0e-3f >= 1.0f)
        {
            if (mf.d2.ok)  return &mf.d2;
            if (mf.d1.ok)  return &mf.d1;
            if (mf.d05.ok) return &mf.d05;
        }
        else
        {
            if (mf.d05.ok) return &mf.d05;
            if (mf.d1.ok)  return &mf.d1;
            if (mf.d2.ok)  return &mf.d2;
        }
        return nullptr;
    }

    // Default primary: prefer 0.5 m, else 1 m, else 2 m.
    inline const MeasuredCurve* primaryCurve (const MeasuredFreq& mf)
    {
        return curveAtDistance (mf, 0.5f);
    }

    // Distances that have at least one loaded sweep in this set.
    inline std::vector<float> availableDistances (const MeasuredSet& set)
    {
        bool has05 = false, has1 = false, has2 = false;
        for (const auto& mf : set.freqs)
        {
            if (mf.d05.ok) has05 = true;
            if (mf.d1.ok)  has1  = true;
            if (mf.d2.ok)  has2  = true;
        }
        std::vector<float> out;
        if (has05) out.push_back (0.5f);
        if (has1)  out.push_back (1.0f);
        if (has2)  out.push_back (2.0f);
        return out;
    }

    // Far-field arc for 100×100 m SPL prediction. Near-field (0.5 m) BEM arcs
    // embed cabinet geometry as fake beams; prefer the largest available radius
    // ≥ 1.0 m (typically 2.0 m for Q21S / 52 Hz).
    inline float farFieldDirectivityDistance (const MeasuredSet& set, float fallback = 1.0f)
    {
        const auto dists = availableDistances (set);
        float best = -1.0f;
        for (float d : dists)
            if (d + 1.0e-3f >= 1.0f && d > best)
                best = d;
        if (best > 0.0f) return best;
        for (float d : dists)
            if (d > best) best = d;
        return best > 0.0f ? best : fallback;
    }

    // Directivity at any UI frequency: exact lab curve when available, otherwise
    // log-frequency blend of the nearest measured anchors (same distance).
    struct CurveAtFrequency
    {
        MeasuredCurve curve;
        bool fromMeasurement = false;   // true = exact lab reading at this Hz
        bool ok = false;
    };

    inline CurveAtFrequency curveForFrequency (const MeasuredSet& set, int hz, float distanceM)
    {
        CurveAtFrequency out;
        if (! set.ok || hz <= 0) return out;

        struct Anchor { int hz; const MeasuredCurve* c; };
        std::vector<Anchor> anchors;
        anchors.reserve (set.freqs.size());

        for (const auto& mf : set.freqs)
        {
            const MeasuredCurve* cv = curveAtDistance (mf, distanceM);
            if (cv == nullptr || ! cv->ok || cv->angleDeg.size() < 2) continue;
            if (mf.hz == hz)
            {
                out.curve = *cv;
                out.fromMeasurement = true;
                out.ok = true;
                return out;
            }
            anchors.push_back ({ mf.hz, cv });
        }

        if (anchors.empty()) return out;
        std::sort (anchors.begin(), anchors.end(),
                   [] (const Anchor& a, const Anchor& b) { return a.hz < b.hz; });

        const MeasuredCurve* c0 = nullptr;
        const MeasuredCurve* c1 = nullptr;
        int f0 = 0, f1 = 0;
        float t = 0.0f;

        if (hz <= anchors.front().hz)
        {
            c0 = anchors.front().c;
            out.curve = *c0;
            out.fromMeasurement = (hz == anchors.front().hz);
            out.ok = true;
            return out;
        }
        if (hz >= anchors.back().hz)
        {
            c0 = anchors.back().c;
            out.curve = *c0;
            out.fromMeasurement = (hz == anchors.back().hz);
            out.ok = true;
            return out;
        }

        for (size_t i = 1; i < anchors.size(); ++i)
        {
            if (hz <= anchors[i].hz)
            {
                f0 = anchors[i - 1].hz;
                f1 = anchors[i].hz;
                c0 = anchors[i - 1].c;
                c1 = anchors[i].c;
                const double a = std::log ((double) juce::jmax (1, f0));
                const double b = std::log ((double) juce::jmax (1, f1));
                const double x = std::log ((double) juce::jmax (1, hz));
                t = (float) juce::jlimit (0.0, 1.0, (x - a) / std::max (1.0e-9, b - a));
                break;
            }
        }

        if (c0 == nullptr || c1 == nullptr) return out;

        MeasuredCurve& c = out.curve;
        c.angleDeg.reserve (360);
        c.splAbs.reserve (360);
        c.R.reserve (360);

        const float onAxis = (1.0f - t) * c0->onAxisSpl + t * c1->onAxisSpl;
        float peak = -1.0e9f;

        for (int deg = 0; deg < 360; ++deg)
        {
            const float rel0 = interpSplDb (*c0, (float) deg) - c0->onAxisSpl;
            const float rel1 = interpSplDb (*c1, (float) deg) - c1->onAxisSpl;
            const float rel  = (1.0f - t) * rel0 + t * rel1;
            const float spl  = onAxis + rel;
            c.angleDeg.push_back ((float) deg);
            c.splAbs.push_back (spl);
            c.R.push_back ((float) std::pow (10.0, rel / 20.0));
            peak = std::max (peak, spl);
        }

        c.onAxisSpl = onAxis;
        c.onAxisR   = 1.0f;
        c.peakSpl   = peak;
        c.trustForModel = c0->trustForModel && c1->trustForModel;
        c.sourceFile = "interp_" + juce::String (f0) + "_" + juce::String (f1) + "Hz";
        c.ok = true;
        c.beamwidthDeg = computeBeamwidthDeg (c);
        out.fromMeasurement = false;
        out.ok = true;
        return out;
    }

    // Directivity tables for the engine at the selected measurement distance.
    // One pattern per UI frequency: exact lab when present, else interpolated.
    // On-axis gain = 1. Never merges distances or sets.
    //
    // Near-field BEM arcs (esp. 0.5 m) carry an L↔R mirror / checkerboard seam
    // around 0° that paints a bright horizontal ridge on the SPL heatmap.
    // Symmetrise + median/smooth the gain before the engine samples it.
    inline void sanitizeDirectivityGain (std::vector<float>& gain)
    {
        if (gain.size() != 360) return;

        // Mid-plane physics is L/R symmetric — average out mirror-overlap errors.
        for (int deg = 1; deg < 180; ++deg)
        {
            const float a = 0.5f * (gain[(size_t) deg] + gain[(size_t) (360 - deg)]);
            gain[(size_t) deg] = a;
            gain[(size_t) (360 - deg)] = a;
        }

        detail::medianFilterCircular (gain, 3);
        detail::smoothCircular (gain, 2);

        const float on = std::max (1.0e-6f, gain[0]);
        for (float& g : gain)
            g = juce::jlimit (0.0f, 2.0f, g / on);
    }

    inline std::vector<DirectivityPattern> buildDirectivityTables (const MeasuredSet& set,
                                                                   float distanceM = 0.5f)
    {
        std::vector<DirectivityPattern> out;
        if (! set.ok) return out;

        for (int i = 0; i < kNumSupportedFrequencies; ++i)
        {
            const int hz = (int) std::lround (kSupportedFrequencies[i]);
            const auto syn = curveForFrequency (set, hz, distanceM);
            if (! syn.ok || ! syn.curve.ok || syn.curve.angleDeg.size() < 2) continue;

            DirectivityPattern dp;
            dp.hz = hz;
            dp.gain.assign (360, 1.0f);
            for (int deg = 0; deg < 360; ++deg)
            {
                const float spl = interpSplDb (syn.curve, (float) deg);
                const float g = (float) std::pow (10.0, (spl - syn.curve.onAxisSpl) / 20.0);
                dp.gain[(size_t) deg] = juce::jlimit (0.0f, 2.0f, g);
            }
            sanitizeDirectivityGain (dp.gain);
            dp.onAxisSplDb = syn.curve.onAxisSpl;
            dp.refDistanceM = distanceM > 0.05f ? distanceM : 2.0f;
            dp.hasAbsolute = syn.curve.onAxisSpl > 20.0f;
            dp.ok = true;
            out.push_back (std::move (dp));
        }
        return out;
    }

    // -----------------------------------------------------------------------
    // Load Q21S BEM absolute-field heatmaps (MATLAB Heatmap.m formation).
    // Files: Q21S_Field_<Hz>Hz.q21f  magic "Q21F"
    // -----------------------------------------------------------------------
    inline BemFieldPattern loadBemFieldFile (const juce::File& f)
    {
        BemFieldPattern out;
        juce::FileInputStream in (f);
        if (! in.openedOk()) return out;

        char mag[4] = {};
        if (in.read (mag, 4) != 4) return out;
        if (std::memcmp (mag, "Q21F", 4) != 0) return out;

        const int ver  = in.readInt();
        const int hz   = in.readInt();
        const int nx   = in.readInt();
        const int nz   = in.readInt();
        const float xmin = in.readFloat();
        const float xmax = in.readFloat();
        const float zmin = in.readFloat();
        const float zmax = in.readFloat();
        if (ver != 1 || nx < 2 || nz < 2 || nx > 2048 || nz > 2048) return out;

        out.hz = hz;
        out.nx = nx;
        out.nz = nz;
        out.xmin = xmin; out.xmax = xmax;
        out.zmin = zmin; out.zmax = zmax;
        out.relDb.resize ((size_t) nx * (size_t) nz);
        for (size_t i = 0; i < out.relDb.size(); ++i)
            out.relDb[i] = in.readFloat();
        out.ok = true;
        return out;
    }

    inline std::vector<BemFieldPattern> loadBemFieldTables (const MeasuredSet& /*set*/)
    {
        std::vector<BemFieldPattern> out;
        const juce::File packDir = packDataFolder();
        if (! packDir.isDirectory()) return out;

        for (size_t i = 0; i < kNumSupportedFrequencies; ++i)
        {
            const int hz = (int) std::lround (kSupportedFrequencies[i]);
            const auto f = packDir.getChildFile ("Q21S_Field_" + juce::String (hz) + "Hz.q21f");
            if (! f.existsAsFile()) continue;
            auto bp = loadBemFieldFile (f);
            if (bp.ok) out.push_back (std::move (bp));
        }
        return out;
    }

    inline MeasuredSet loadMeasurements (const juce::File& /*folder*/, int source = OpenField)
    {
        MeasuredSet set;
        set.source     = source;
        set.sourceName = sourceName (source);

        const juce::File packDir    = packDataFolder();
        const juce::File xlsxFolder = xlsxFolderForSource (source);
        if (packDir.isDirectory())
            set.packPath = packDir.getFullPathName();
        else if (EmbeddedQ21S::numFiles > 0)
            set.packPath = "embedded://Q21S";   // baked into EXE — no sidecar Data/

        const auto freqList = discoverFrequencies (packDir, xlsxFolder, source);

        for (int hz : freqList)
        {
            MeasuredFreq mf;
            mf.hz = hz;

            // Each distance is built independently (no shared reference level).
            mf.d05 = loadCurvePreferPack (packDir, xlsxFolder, source, hz, 0.5f);
            mf.d1  = loadCurvePreferPack (packDir, xlsxFolder, source, hz, 1.0f);
            mf.d2  = loadCurvePreferPack (packDir, xlsxFolder, source, hz, 2.0f);
            mf.ok  = mf.d05.ok || mf.d1.ok || mf.d2.ok;

            if (const MeasuredCurve* p = primaryCurve (mf))
            {
                mf.beamwidthDeg  = p->beamwidthDeg;
                mf.trustForModel = p->trustForModel;
                mf.sourceFile    = p->sourceFile;
            }

            if (mf.ok) { set.ok = true; set.freqs.push_back (std::move (mf)); }
        }

        return set;
    }

    // Filename distance tag: 0.5 → 0p5m, 1.0 → 1p0m, 2.0 → 2p0m
    inline juce::String distanceFileTag (float distanceM)
    {
        if (std::abs (distanceM - 0.5f) < 0.05f) return "0p5m";
        if (std::abs (distanceM - 2.0f) < 0.05f) return "2p0m";
        return "1p0m";
    }

    inline juce::String suggestedDirectivityFileName (int hz, float distanceM)
    {
        return "Atomik_Directivity_" + juce::String (hz) + "Hz_"
             + distanceFileTag (distanceM) + "(PredictionSoftware).csv";
    }

    // VACS-style Atomik Prediction Software sheet (matches Export_Reference /
    // Atomik_Directivity_* sample). Writes absolute measured SPL at native
    // angles — values differ per frequency / distance from the loaded Excel/CSV.
    inline bool writeAtomikDirectivitySheet (const juce::File& file,
                                             const MeasuredCurve& cv,
                                             int hz,
                                             float /*distanceM*/)
    {
        if (! cv.ok || cv.angleDeg.size() < 2 || cv.splAbs.size() != cv.angleDeg.size())
            return false;

        juce::FileOutputStream fos (file);
        if (! fos.openedOk()) return false;

        auto line = [&] (const juce::String& s)
        {
            fos.writeText (s + "\n", false, false, nullptr);
        };

        line ("SourceDesc=Atomik_Data_Text");
        line ("Version='v1.3.7'");
        line ("Author='Atomik Prediction Software'");
        line ("");
        line ("Frequency_Hz=" + juce::String (hz));
        line ("Measurement_Set=Ground_Plane");
        line ("Level_Type=Absolute_SPL_dB");
        line ("");
        line ("StartString_Data=Data");
        line ("EndString_Data=Data_End");
        line ("");
        line ("Data_Format=LeveldB_Phase");
        line ("Data_Domain=Other");
        line ("Data_LevelType=SoundPressure");
        line ("Data_Phase_AngularFormat=degree");
        line ("Data_AbscUnit=deg");
        line ("Data_BaseUnit=Pa");
        line ("Data_Legend='Curve at: " + juce::String (hz) + " Hz | Ground Plane'");
        line ("");
        line ("Data");
        line ("Angle (deg),Level (dB),Phase (deg)");

        auto formatAngle = [] (float a) -> juce::String
        {
            const double r = std::round ((double) a * 10.0) / 10.0;
            if (std::abs (r - std::round (r)) < 1.0e-6)
                return juce::String ((int) std::lround (r)) + ".0";
            return juce::String (r, 1);
        };

        // CLIO-style order: ascending angles with on-axis (≈0°) last when present.
        std::vector<size_t> order (cv.angleDeg.size());
        for (size_t i = 0; i < order.size(); ++i) order[i] = i;
        std::sort (order.begin(), order.end(),
                   [&] (size_t L, size_t R) { return cv.angleDeg[L] < cv.angleDeg[R]; });

        size_t zeroIdx = (size_t) -1;
        for (size_t k : order)
        {
            const float a = cv.angleDeg[k];
            if (a < 0.05f || a > 359.95f) { zeroIdx = k; break; }
        }

        auto writePoint = [&] (size_t k)
        {
            float a = cv.angleDeg[k];
            if (a > 359.95f) a = 0.0f;
            line (formatAngle (a) + ","
                  + juce::String (cv.splAbs[k], 6) + ",0.0");
        };

        for (size_t k : order)
            if (k != zeroIdx) writePoint (k);
        if (zeroIdx != (size_t) -1) writePoint (zeroIdx);

        line ("Data_End");
        return true;
    }

    inline bool exportCurveSheet (const juce::File& file,
                                  const MeasuredFreq& mf,
                                  float distanceM)
    {
        const MeasuredCurve* cv = curveAtDistance (mf, distanceM);
        if (cv == nullptr || ! cv->ok) return false;
        return writeAtomikDirectivitySheet (file, *cv, mf.hz, distanceM);
    }

    inline bool exportCurveAtFrequency (const juce::File& file,
                                        const MeasuredSet& set,
                                        int hz,
                                        float distanceM)
    {
        const auto syn = curveForFrequency (set, hz, distanceM);
        if (! syn.ok || ! syn.curve.ok) return false;
        return writeAtomikDirectivitySheet (file, syn.curve, hz, distanceM);
    }
}

#pragma once
#include <JuceHeader.h>
#include <vector>
#include <utility>
#include <cmath>

// ===========================================================================
// DxfImport - a small, dependency-free DXF reader that converts the common
// 2D entity types (LINE, LWPOLYLINE, POLYLINE/VERTEX, CIRCLE, ARC) found in
// architectural floor plans into a single juce::Path for use as a reference
// underlay. DXF is an ASCII tagged format: alternating (group-code, value)
// lines. We only read the ENTITIES section.
//
// Output path is in source drawing units with y pointing DOWN (DXF y is up,
// so it is negated) to match the raster-image convention used by LayoutLayer.
// ===========================================================================
namespace DxfImport
{
    namespace detail
    {
        struct Group
        {
            juce::String type;
            std::vector<std::pair<int, juce::String>> attrs;

            juce::String str (int code, const juce::String& def = {}) const
            {
                for (auto& a : attrs) if (a.first == code) return a.second;
                return def;
            }
            double num (int code, double def = 0.0) const
            {
                for (auto& a : attrs) if (a.first == code) return a.second.getDoubleValue();
                return def;
            }
            int flags (int code) const { return (int) num (code, 0.0); }
        };

        inline void addArcSegments (juce::Path& p, float cx, float cy, float r,
                                    float a0deg, float a1deg)
        {
            // DXF arcs sweep counter-clockwise from a0 to a1 (degrees, +x = 0).
            float a1 = a1deg;
            while (a1 < a0deg) a1 += 360.0f;
            const float sweep = a1 - a0deg;
            const int steps = juce::jlimit (2, 180, (int) std::ceil (sweep / 3.0f));
            for (int i = 0; i <= steps; ++i)
            {
                const float a = juce::degreesToRadians (a0deg + sweep * (float) i / (float) steps);
                const float x = cx + r * std::cos (a);
                const float y = -(cy + r * std::sin (a));   // negate y (y-down)
                if (i == 0) p.startNewSubPath (x, y);
                else        p.lineTo (x, y);
            }
        }
    }

    inline bool load (const juce::File& f, juce::Path& outPath, juce::Rectangle<float>& outBounds)
    {
        using namespace detail;
        outPath.clear();

        const juce::String text = f.loadFileAsString();
        if (text.isEmpty()) return false;

        juce::StringArray lines;
        lines.addLines (text);

        // Tokenise into (code, value) pairs.
        std::vector<std::pair<int, juce::String>> toks;
        toks.reserve ((size_t) lines.size() / 2 + 1);
        for (int i = 0; i + 1 < lines.size(); i += 2)
            toks.push_back ({ lines[i].trim().getIntValue(), lines[i + 1].trim() });

        // Split into entity groups at each code-0 marker.
        std::vector<Group> groups;
        Group cur; bool started = false;
        for (auto& t : toks)
        {
            if (t.first == 0)
            {
                if (started) groups.push_back (cur);
                cur = Group(); cur.type = t.second; started = true;
            }
            else if (started) cur.attrs.push_back (t);
        }
        if (started) groups.push_back (cur);

        juce::String section;
        std::vector<juce::Point<float>> polyVerts;
        bool inPoly = false, polyClosed = false;

        auto flushPoly = [&]
        {
            if (polyVerts.size() >= 2)
            {
                outPath.startNewSubPath (polyVerts[0]);
                for (size_t i = 1; i < polyVerts.size(); ++i) outPath.lineTo (polyVerts[i]);
                if (polyClosed) outPath.closeSubPath();
            }
            polyVerts.clear();
            inPoly = false; polyClosed = false;
        };

        for (auto& g : groups)
        {
            if (g.type == "SECTION") { section = g.str (2); continue; }
            if (g.type == "ENDSEC")  { if (inPoly) flushPoly(); section = {}; continue; }
            if (section != "ENTITIES") continue;

            if (g.type == "LINE")
            {
                const float x1 = (float) g.num (10), y1 = (float) g.num (20);
                const float x2 = (float) g.num (11), y2 = (float) g.num (21);
                outPath.startNewSubPath (x1, -y1);
                outPath.lineTo (x2, -y2);
            }
            else if (g.type == "LWPOLYLINE")
            {
                std::vector<juce::Point<float>> vs;
                float pendingX = 0.0f; bool haveX = false;
                for (auto& a : g.attrs)
                {
                    if (a.first == 10) { pendingX = a.second.getFloatValue(); haveX = true; }
                    else if (a.first == 20 && haveX)
                    {
                        vs.push_back ({ pendingX, -a.second.getFloatValue() });
                        haveX = false;
                    }
                }
                const bool closed = (g.flags (70) & 1) != 0;
                if (vs.size() >= 2)
                {
                    outPath.startNewSubPath (vs[0]);
                    for (size_t i = 1; i < vs.size(); ++i) outPath.lineTo (vs[i]);
                    if (closed) outPath.closeSubPath();
                }
            }
            else if (g.type == "POLYLINE")
            {
                if (inPoly) flushPoly();
                inPoly = true;
                polyClosed = (g.flags (70) & 1) != 0;
            }
            else if (g.type == "VERTEX")
            {
                if (inPoly)
                    polyVerts.push_back ({ (float) g.num (10), -(float) g.num (20) });
            }
            else if (g.type == "SEQEND")
            {
                if (inPoly) flushPoly();
            }
            else if (g.type == "CIRCLE")
            {
                const float cx = (float) g.num (10), cy = (float) g.num (20), r = (float) g.num (40);
                if (r > 0.0f) outPath.addEllipse (cx - r, -cy - r, 2.0f * r, 2.0f * r);
            }
            else if (g.type == "ARC")
            {
                const float cx = (float) g.num (10), cy = (float) g.num (20), r = (float) g.num (40);
                if (r > 0.0f) addArcSegments (outPath, cx, cy, r, (float) g.num (50), (float) g.num (51));
            }
        }
        if (inPoly) flushPoly();

        if (outPath.isEmpty()) return false;
        outBounds = outPath.getBounds();
        return outBounds.getWidth() > 0.0f && outBounds.getHeight() > 0.0f;
    }
}

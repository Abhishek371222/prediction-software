#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include <cmath>
#include "BrandTheme.h"
#include "AppSettings.h"
#include "AcousticEngine.h"

// ---------------------------------------------------------------------------
// Speaker3DView - the cabinet as a solid, drawn to its real proportions and
// draggable to turn.
//
// Painted rather than rendered: a box is six quads, so a rotation matrix, a
// depth sort and JUCE's own Path fills give a true 3D view with no OpenGL
// context, no shaders and nothing extra to ship. The faces carry brand colour,
// with the baffle in signal red so which way the cabinet fires is never in
// doubt - the thing a plan view cannot tell you.
//
// Dimensions come from cabinetFor(), the same place the plan marker and the
// Properties rows read, so the solid can never disagree with the numbers
// printed beside it.
// ---------------------------------------------------------------------------
class Speaker3DView : public juce::Component
{
public:
    // juce::Vector3D lives in a module this build does not include, and three
    // floats plus a cross product is the whole of what is needed here.
    struct Vec3
    {
        float x = 0.0f, y = 0.0f, z = 0.0f;
        Vec3 operator- (const Vec3& o) const { return { x - o.x, y - o.y, z - o.z }; }
        Vec3 cross (const Vec3& o) const
        {
            return { y * o.z - z * o.y, z * o.x - x * o.z, x * o.y - y * o.x };
        }
        float length() const { return std::sqrt (x * x + y * y + z * z); }
    };

    explicit Speaker3DView (int model) : model_ (model)
    {
        setSize (420, 380);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());

        const auto cab = cabinetFor (model_);
        const float W = cab.widthM, H = cab.heightM, D = cab.depthM;
        if (W <= 0.0f || H <= 0.0f || D <= 0.0f) return;

        // Box centred on the origin. +x right (width), +y up (height),
        // +z towards the viewer (depth); the baffle is the +z face.
        const float hx = W * 0.5f, hy = H * 0.5f, hz = D * 0.5f;
        const std::array<Vec3, 8> v = {{
            { -hx, -hy,  hz }, {  hx, -hy,  hz }, {  hx,  hy,  hz }, { -hx,  hy,  hz },
            { -hx, -hy, -hz }, {  hx, -hy, -hz }, {  hx,  hy, -hz }, { -hx,  hy, -hz }
        }};

        // Rotate: yaw about the vertical, then pitch about the horizontal.
        const float cy = std::cos (yaw_),   sy = std::sin (yaw_);
        const float cp = std::cos (pitch_), sp = std::sin (pitch_);
        auto rot = [&] (Vec3 p)
        {
            const float x1 =  p.x * cy + p.z * sy;
            const float z1 = -p.x * sy + p.z * cy;
            const float y2 =  p.y * cp - z1 * sp;
            const float z2 =  p.y * sp + z1 * cp;
            return Vec3 { x1, y2, z2 };
        };

        std::array<Vec3, 8> r;
        for (size_t i = 0; i < 8; ++i) r[i] = rot (v[i]);

        // Fit: one scale for every axis, or the proportions are a lie.
        float maxAbs = 0.0f;
        for (const auto& p : r)
            maxAbs = juce::jmax (maxAbs, std::abs (p.x), std::abs (p.y));
        const auto box = getLocalBounds().reduced (56, 52).toFloat();
        const float scale = (maxAbs > 1.0e-6f)
                          ? juce::jmin (box.getWidth(), box.getHeight()) * 0.5f / maxAbs
                          : 1.0f;
        const float ox = box.getCentreX(), oy = box.getCentreY();
        auto proj = [&] (Vec3 p)
        {
            return juce::Point<float> (ox + p.x * scale, oy - p.y * scale);
        };

        // Six faces, wound so the cross product points outwards.
        struct Face { int a, b, c, d; juce::Colour col; const char* tag; };
        const juce::Colour body = Brand::charcoal();
        const Face faces[6] = {
            { 0, 1, 2, 3, Brand::accent(), "BAFFLE" },   // +z, fires this way
            { 5, 4, 7, 6, body,            nullptr  },   // -z  back
            { 1, 5, 6, 2, body,            nullptr  },   // +x  side
            { 4, 0, 3, 7, body,            nullptr  },   // -x  side
            { 3, 2, 6, 7, body,            nullptr  },   // +y  top
            { 4, 5, 1, 0, body,            nullptr  }    // -y  bottom
        };

        // Painter's algorithm: furthest first. Six quads makes sorting cheaper
        // than any cleverer method, and a convex box never self-intersects.
        std::array<int, 6> order = { 0, 1, 2, 3, 4, 5 };
        auto depthOf = [&] (const Face& f)
        {
            return (r[(size_t) f.a].z + r[(size_t) f.b].z
                  + r[(size_t) f.c].z + r[(size_t) f.d].z) * 0.25f;
        };
        std::sort (order.begin(), order.end(),
                   [&] (int A, int B) { return depthOf (faces[A]) < depthOf (faces[B]); });

        for (int fi : order)
        {
            const auto& f = faces[fi];
            const auto pa = proj (r[(size_t) f.a]), pb = proj (r[(size_t) f.b]);
            const auto pc = proj (r[(size_t) f.c]), pd = proj (r[(size_t) f.d]);

            // Outward normal, for both culling and shading.
            const auto e1 = r[(size_t) f.b] - r[(size_t) f.a];
            const auto e2 = r[(size_t) f.d] - r[(size_t) f.a];
            auto n = e1.cross (e2);
            const float nl = n.length();
            if (nl < 1.0e-9f) continue;
            n = Vec3 { n.x / nl, n.y / nl, n.z / nl };
            if (n.z <= 0.0f) continue;                 // facing away

            // Light from above and slightly toward the viewer, so turning the
            // box reads as turning rather than flickering.
            const float lit = juce::jlimit (0.0f, 1.0f,
                                            0.45f + 0.55f * (0.55f * n.y + 0.45f * n.z));

            juce::Path quad;
            quad.startNewSubPath (pa); quad.lineTo (pb);
            quad.lineTo (pc); quad.lineTo (pd); quad.closeSubPath();

            g.setColour (f.col.withMultipliedBrightness (0.55f + 0.85f * lit));
            g.fillPath (quad);
            g.setColour (Brand::charcoal().withAlpha (0.55f));
            g.strokePath (quad, juce::PathStrokeType (1.0f));

            if (f.tag != nullptr)
            {
                g.setColour (Brand::onAccent().withAlpha (0.92f));
                g.setFont (Brand::tech (juce::jmax (9.0f, 10.0f * Brand::UI::scale), true));
                const auto ctr = (pa + pb + pc + pd) * 0.25f;
                g.drawText (f.tag, juce::roundToInt (ctr.x) - 50,
                            juce::roundToInt (ctr.y) - 7, 100, 14,
                            juce::Justification::centred);
            }
        }

        drawCallouts (g, cab);
    }

    void mouseDown (const juce::MouseEvent& e) override { last_ = e.position; }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        const auto d = e.position - last_;
        last_ = e.position;
        yaw_ += d.x * 0.012f;
        pitch_ = juce::jlimit (-1.35f, 1.35f, pitch_ + d.y * 0.012f);
        repaint();
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        yaw_ = 0.62f; pitch_ = 0.42f;      // back to the opening three-quarter
        repaint();
    }

private:
    void drawCallouts (juce::Graphics& g, const CabinetDims& cab)
    {
        g.setFont (Brand::mono (juce::jmax (9.0f, 10.5f * Brand::UI::scale), true));
        g.setColour (Brand::text());
        const int lh = 15;
        int y = 10;
        auto line = [&] (const juce::String& s, juce::Colour c)
        {
            g.setColour (c);
            g.drawText (s, 12, y, getWidth() - 24, lh, juce::Justification::centredLeft);
            y += lh;
        };
        line ("W  " + Units::dim (cab.widthM  * 1000.0), Brand::text());
        line ("H  " + Units::dim (cab.heightM * 1000.0), Brand::text());
        line ("D  " + Units::dim (cab.depthM  * 1000.0), Brand::text());

        g.setFont (Brand::tech (juce::jmax (8.5f, 9.5f * Brand::UI::scale)));
        g.setColour (Brand::muted());
        g.drawText ("drag to turn  -  double-click to reset",
                    12, getHeight() - 20, getWidth() - 24, 14,
                    juce::Justification::centredLeft);
        g.setColour (Brand::accent());
        g.drawText ("red face = baffle",
                    12, getHeight() - 34, getWidth() - 24, 14,
                    juce::Justification::centredLeft);
    }

    int   model_ = 0;
    float yaw_   = 0.62f;
    float pitch_ = 0.42f;
    juce::Point<float> last_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Speaker3DView)
};

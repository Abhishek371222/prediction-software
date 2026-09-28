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
        setSize (470, 430);
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
        const auto box = getLocalBounds().reduced (96, 92).toFloat();
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

        // --- dimensions, on the edges they measure ------------------------
        // Drawn after the solid so they are never buried by it, and offset
        // outwards in 3D so they turn with the box instead of floating in
        // screen space. This is the whole point of the view: the numbers sit
        // against the edges, not in a corner where they mean nothing.
        {
            const float sp = 0.16f * juce::jmax (W, juce::jmax (H, D));
            const juce::Colour ink = Brand::text();

            auto arrow2 = [&] (juce::Point<float> tip, juce::Point<float> from)
            {
                const auto d = tip - from;
                const float len = juce::jmax (0.001f, d.getDistanceFromOrigin());
                const juce::Point<float> u (d.x / len, d.y / len), n (-u.y, u.x);
                const float ah = 7.0f, aw = 3.0f;
                juce::Path t;
                t.addTriangle (tip.x, tip.y,
                               tip.x - u.x * ah + n.x * aw, tip.y - u.y * ah + n.y * aw,
                               tip.x - u.x * ah - n.x * aw, tip.y - u.y * ah - n.y * aw);
                g.fillPath (t);
            };

            auto dim3 = [&] (Vec3 a, Vec3 b, Vec3 off, const juce::String& label)
            {
                const Vec3 a2 { a.x + off.x, a.y + off.y, a.z + off.z };
                const Vec3 b2 { b.x + off.x, b.y + off.y, b.z + off.z };
                const auto pa = proj (rot (a)),  pb = proj (rot (b));
                const auto qa = proj (rot (a2)), qb = proj (rot (b2));

                g.setColour (ink.withAlpha (0.40f));          // extension lines
                g.drawLine (pa.x, pa.y, qa.x, qa.y, 1.0f);
                g.drawLine (pb.x, pb.y, qb.x, qb.y, 1.0f);

                g.setColour (ink);                            // dimension line
                g.drawLine (qa.x, qa.y, qb.x, qb.y, 1.4f);
                arrow2 (qa, qb);
                arrow2 (qb, qa);

                // Label on a panel-coloured pill: at some angles the line
                // passes behind the text and the two become unreadable.
                const auto mid = (qa + qb) * 0.5f;
                g.setFont (Brand::mono (juce::jmax (9.0f, 10.0f * Brand::UI::scale), true));
                const int tw = juce::jmax (52, g.getCurrentFont().getStringWidth (label) + 10);
                juce::Rectangle<float> pill (mid.x - tw * 0.5f, mid.y - 9.0f,
                                             (float) tw, 18.0f);
                g.setColour (Brand::panel().withAlpha (0.88f));
                g.fillRoundedRectangle (pill, 3.0f);
                g.setColour (ink);
                g.drawText (label, pill.toNearestInt(), juce::Justification::centred);
            };

            // Width along the baffle's bottom edge, pushed down and forward.
            dim3 ({ -hx, -hy, hz }, { hx, -hy, hz }, { 0.0f, -sp, sp * 0.35f },
                  "W " + Units::dim (cab.widthM * 1000.0));

            // Height up the baffle's right edge, pushed out to the right.
            dim3 ({ hx, -hy, hz }, { hx, hy, hz }, { sp, 0.0f, sp * 0.35f },
                  "H " + Units::dim (cab.heightM * 1000.0));

            // Depth along the bottom-right edge running back from the baffle.
            dim3 ({ hx, -hy, hz }, { hx, -hy, -hz }, { sp * 0.35f, -sp, 0.0f },
                  "D " + Units::dim (cab.depthM * 1000.0));
        }

        drawHints (g);
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
    void drawHints (juce::Graphics& g)
    {
        g.setFont (Brand::tech (juce::jmax (8.5f, 9.5f * Brand::UI::scale)));
        g.setColour (Brand::accent());
        g.drawText ("red face = baffle (fires this way)",
                    12, getHeight() - 34, getWidth() - 24, 14,
                    juce::Justification::centredLeft);
        g.setColour (Brand::muted());
        g.drawText ("drag to turn  -  double-click to reset",
                    12, getHeight() - 20, getWidth() - 24, 14,
                    juce::Justification::centredLeft);
    }

    int   model_ = 0;
    float yaw_   = 0.62f;
    float pitch_ = 0.42f;
    juce::Point<float> last_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Speaker3DView)
};

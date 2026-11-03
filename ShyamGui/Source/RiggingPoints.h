#pragma once
#include <JuceHeader.h>
#include <cmath>

// =============================================================================
// The rigging lever, as the supplied drawing and table define it.
//
// Source: ShyamGui/Source/Rigging point.xlsx - a three-column table of
// TILT ANGLE / LEVER CONFIGURATION / POINT, with the bracket drawing beside it.
// The numbers below ARE that table; nothing here is inferred.
//
//      TILT   CONFIG   POINT          The bracket carries two sets of holes:
//      -15      B        5
//      -12      B        4            CONFIGURATION A - the near-vertical
//       -9      B        3            column up the front edge, holes 0..5,
//       -6      B        2            tilting the box UP  (0, +3 .. +15).
//       -3      B        1
//        0      A        0            CONFIGURATION B - the horizontal row
//       +3      A        1            along the bottom arm, holes 1..5,
//       +6      A        2            tilting the box DOWN (-3 .. -15).
//       +9      A        3
//      +12      A        4            Hole 0 exists only on A: it is the
//      +15      A        5            level setting, shared by both.
//
// So a point is worth exactly kDegPerPoint of tilt, and the configuration is
// just which way it leans. Keeping that as a table rather than as arithmetic
// scattered through the panel means a bracket re-drilled to a different pitch
// is one number here.
// =============================================================================
namespace Rigging
{
    /** Which set of holes the pin goes through. */
    enum class Lever { A = 0, B = 1 };

    /** Degrees of tilt per hole, from the drawing. */
    inline constexpr float kDegPerPoint = 3.0f;
    /** Highest hole number on either set. */
    inline constexpr int   kMaxPoint    = 5;

    inline const char* leverName (Lever l) noexcept { return l == Lever::A ? "A" : "B"; }

    /** One row of the table: a hole, and which set it is in. */
    struct Pick
    {
        Lever lever = Lever::A;
        int   point = 0;

        bool operator== (const Pick& o) const noexcept
        { return lever == o.lever && point == o.point; }
    };

    /** The tilt a pin in this hole produces. A leans up, B leans down. */
    inline float tiltForPick (Pick p) noexcept
    {
        const float mag = (float) juce::jlimit (0, kMaxPoint, p.point) * kDegPerPoint;
        return p.lever == Lever::A ? mag : -mag;
    }

    /** The hole that gives this tilt, to the nearest hole the bracket has.
        Tilt beyond +/-15 degrees cannot be rigged on this lever, so it clamps
        to the end hole rather than inventing one. */
    inline Pick pickForTilt (float tiltDeg) noexcept
    {
        const int n = juce::jlimit (0, kMaxPoint,
                                    (int) std::lround (std::abs (tiltDeg) / kDegPerPoint));
        return { tiltDeg < 0.0f ? Lever::B : Lever::A, n };
    }

    /** True when the angle lands on a hole rather than between two. A rigger
        can only pin a hole, so an angle typed in between is reported rather
        than quietly rounded. */
    inline bool isOnAHole (float tiltDeg) noexcept
    {
        return std::abs (tiltDeg - tiltForPick (pickForTilt (tiltDeg))) < 0.05f;
    }

    /** "A0", "B3" - what a rigger writes on the sheet. */
    inline juce::String label (Pick p)
    {
        return juce::String (leverName (p.lever)) + juce::String (p.point);
    }

    /** Every hole the bracket has, in table order: B5..B1 then A0..A5. */
    inline juce::Array<Pick> allPicks()
    {
        juce::Array<Pick> out;
        for (int n = kMaxPoint; n >= 1; --n) out.add ({ Lever::B, n });
        for (int n = 0; n <= kMaxPoint; ++n) out.add ({ Lever::A, n });
        return out;
    }

    // ---- the two ropes -----------------------------------------------------
    //
    // A flown box hangs on two lines - the FRONT pick and the REAR pick - and
    // the difference in their lengths is what holds the box at its tilt. The
    // reference sheet puts it the same way: the chains hang vertically and
    // raising the rear point above the front tilts the bumper down.
    //
    // Given the two pick points on the bumper and the tilt, the rear line is
    // longer than the front by the rise across the span between them.

    /** How much longer the rear line is than the front, for this tilt over a
        bumper span of @p spanM metres. Positive means the rear hangs lower. */
    inline float riseBetweenPicks (float tiltDeg, float spanM) noexcept
    {
        return spanM * std::tan (juce::degreesToRadians (tiltDeg));
    }

    /** The angle a line makes with vertical when its top is @p offsetM to one
        side of its bottom over a drop of @p dropM. Zero is plumb. */
    inline float lineAngleFromVertical (float offsetM, float dropM) noexcept
    {
        if (dropM <= 1.0e-4f) return 0.0f;
        return juce::radiansToDegrees (std::atan2 (offsetM, dropM));
    }
}

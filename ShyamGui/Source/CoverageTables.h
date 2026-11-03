#pragma once
#include <JuceHeader.h>
#include <cmath>
#include <vector>

// =============================================================================
// Measured coverage, band by band, per cabinet.
//
// Each table is a supplied FREQUENCY / DEGREE list - the full -6 dB beamwidth
// the cabinet actually has at each measured band. These are the published
// numbers for the device, so they stand ahead of anything the app derives from
// a polar sweep: a beamwidth read off a curve is an estimate of this, and
// where the two disagree the measurement wins.
//
// WHICH CABINETS DRAW ANGLE LINES
//
//   Q212P  - yes, at the fixed kQ212PEdgeDeg below; its table is kept for
//            reference.
//   Q21S   - NO. The sub draws no angle lines at all; it carries only the
//            short stub off its front face that says which way it points.
//            Its table is kept here because the numbers are real and were
//            supplied, not because anything currently reads them.
//
// One angle per cabinet was never going to be right for either of these: the
// pattern moves by more than fifty degrees across the measured range, which is
// why the lines have to be read at the solve frequency.
// =============================================================================
namespace Coverage
{
    struct Band { float hz; float deg; };

    /** The Q212P, and the only table that draws anything. */
    inline const std::vector<Band>& q212p()
    {
        static const std::vector<Band> t {
            {   484.0f, 168.49f }, {  1088.0f, 116.98f }, {  2078.0f,  94.71f },
            {  3969.0f,  76.04f }, {  8909.0f,  45.47f }, { 17013.0f,  21.41f },
        };
        return t;
    }

    /** The Q21S. Retained, deliberately unused - the sub shows no angle
        lines. See the note above before wiring this to anything. */
    inline const std::vector<Band>& q21s()
    {
        static const std::vector<Band> t {
            {  20.0f, 75.08f }, {  29.0f, 79.37f }, {  52.0f, 86.89f },
            {  60.0f, 88.27f }, {  81.0f, 90.80f }, {  98.0f, 91.82f },
            { 153.0f, 85.05f }, { 198.0f, 64.47f }, { 256.0f, 65.23f },
            { 309.0f, 49.77f }, { 352.0f, 50.43f }, { 400.0f, 39.80f },
        };
        return t;
    }

    /** The band @p hz belongs to, by nearest measured frequency.

        Nearest rather than interpolated, because each row IS a measurement and
        the frequencies between them were never measured - 2078 Hz at 94.71 and
        3969 Hz at 76.04 are two readings, not two points on a line anyone
        took. Outside the measured range it holds the end band rather than
        extrapolating a number nobody stood behind. */
    inline const Band& bandFor (const std::vector<Band>& t, double hz) noexcept
    {
        const Band* best = &t.front();
        for (const auto& b : t)
            if (std::abs ((double) b.hz - hz) < std::abs ((double) best->hz - hz))
                best = &b;
        return *best;
    }

    /** The Q212P's full -6 dB beamwidth at @p hz, in degrees. */
    inline float q212pDegAt (double hz) noexcept { return bandFor (q212p(), hz).deg; }

    /** The angle the plan draws between the Q212P's two coverage edges. A
        fixed figure, not read off the table above at the solve frequency. */
    inline constexpr float kQ212PEdgeDeg = 50.36f;
}

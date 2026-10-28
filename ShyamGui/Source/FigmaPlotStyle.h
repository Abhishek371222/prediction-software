#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "AppSettings.h"

// =============================================================================
// The Venue tab's drawing style, shared by the three stacked Sources panes
// (Top View, Elevation view, Distance Vs. dbSPL) so all four read as one set.
//
// Figma "Source screen - 1.3.1" (node 207:1269):
//
//   field     #E4E4E4 sheet, 1 px black frame
//   grid      #D9D9D9 on a 2 m step (5 ft in Imperial), no finer rule
//   scale     48 px per 2 m in the 1920-wide frame
//   labels    Montserrat SemiBold 12, black, outside the field: along the top
//             and down the left, zero named for its axis ("0(x)", "0(y)")
//   x = 0     a dotted blue rule through the origin in every pane
//
// Every pane takes the SAME horizontal mapping from the plan, so a metre
// sits in the same column in all three.
// =============================================================================
namespace FigPlot
{
    inline juce::Colour field()     { return juce::Colour (0xffe4e4e4); }
    inline juce::Colour grid()      { return juce::Colour (0xffd9d9d9); }
    inline juce::Colour frame()     { return juce::Colour (0xff000000); }
    inline juce::Colour tick()      { return juce::Colour (0xff000000); }
    inline juce::Colour mark()      { return juce::Colour (0xffed2227); }
    inline juce::Colour freq1()     { return juce::Colour (0xffed2227); }
    inline juce::Colour freq2()     { return juce::Colour (0xff145fea); }
    /** The x = 0 rule: blue, drawn down the sheet. */
    inline juce::Colour originAxis() { return juce::Colour (0xff145fea).withAlpha (0.75f); }
    /** The y = 0 rule: red, drawn across it. Two colours because the two
        lines answer different questions - one is 'where is the stage', the
        other 'where is the centre line' - and a single colour for both
        leaves you counting rules to tell which is which. */
    inline juce::Colour originAxisY() { return juce::Colour (0xffed2227).withAlpha (0.75f); }
    inline juce::Colour cabinet()   { return juce::Colour (0x80d3def1); }   // rgba(211,222,241,0.5)

    /** Figma px per metre at the default framing: 48 px per 2 m. */
    constexpr float kDefaultPxPerM = 24.0f;

    /** Frame insets in Figma px. The plan's right inset includes the colour
        bar's strip, which sits INSIDE the frame but outside the grid. */
    struct Insets { float l, t, r, b; };
    constexpr Insets kPlan      { 66.0f, 53.0f, 32.0f, 27.0f };
    constexpr Insets kPane      { 66.0f, 41.0f, 32.0f, 44.0f };

    inline juce::Rectangle<int> inset (juce::Rectangle<int> r, Insets i)
    {
        using UiConfig::Scale::fig;
        return r.withTrimmedLeft (fig (i.l)).withTrimmedTop (fig (i.t))
                .withTrimmedRight (fig (i.r)).withTrimmedBottom (fig (i.b));
    }

    /** The base rule, in metres: 2 m, or five feet so Imperial lands on whole feet. */
    inline double baseStepM() { return Units::imperial() ? 5.0 * 0.3048 : 2.0; }

    /** Grid step for this zoom: the base step, coarsened or refined in 1-2-5
        steps so rules stay at least @p wantPx apart without going sparse. */
    inline double gridStepM (double pxPerM, double wantPx)
    {
        static constexpr double mult[] = { 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0,
                                           10.0, 25.0, 50.0, 100.0, 250.0 };
        for (double k : mult)
            if (baseStepM() * k * pxPerM >= wantPx)
                return baseStepM() * k;
        return baseStepM() * 250.0;
    }

    /** The opening scale for a frame @p framePx wide: as near 48 px per 2 m as
        lands a whole number of cells across it, so both side edges sit on a
        rule and the corners carry a figure instead of a part cell. */
    inline float snappedPxPerM (int framePx)
    {
        const double cell = baseStepM() * UiConfig::Scale::figf (kDefaultPxPerM);
        const double n = juce::jmax (1.0, std::round ((double) framePx / cell));
        return (float) ((double) framePx / (n * baseStepM()));
    }

    /** The left edge the panes open on: three cells left of the origin. */
    inline double openingLeftM() { return -3.0 * baseStepM(); }

    /** Label step: a whole multiple of the grid step, so every number sits on a rule. */
    inline double labelStepM (double gridM, double pxPerM, double wantPx)
    {
        for (double n : { 1.0, 2.0, 5.0, 10.0, 20.0 })
            if (gridM * n * pxPerM >= wantPx)
                return gridM * n;
        return gridM * 50.0;
    }

    /** "-12m", "0(x)"; one decimal once the step drops below a whole unit. */
    inline juce::String tick (double m, double stepM, const char* axis)
    {
        if (std::abs (m) < stepM * 1.0e-3) return juce::String ("0(") + axis + ")";
        const double v = Units::metresToDisplay (m);
        const bool frac = Units::metresToDisplay (stepM) < 0.999;
        return (frac ? juce::String (v, 1) : juce::String ((int) std::lround (v)))
             + Units::lengthUnit();
    }

    template <typename Fn>
    inline void forSteps (double lo, double hi, double step, Fn&& fn)
    {
        if (step <= 0.0 || hi < lo) return;
        const long i0 = (long) std::ceil  ((lo - 1.0e-9) / step);
        const long i1 = (long) std::floor ((hi + 1.0e-9) / step);
        for (long i = i0; i <= i1; ++i) fn ((double) i * step);
    }

    /** Vertical rules across @p f for a horizontal mapping px = x0 + m * ppm. */
    inline void drawXGrid (juce::Graphics& g, juce::Rectangle<int> f,
                           float x0, float ppm, double stepM)
    {
        if (ppm <= 0.0f) return;
        const double lo = (f.getX() - x0) / ppm, hi = (f.getRight() - x0) / ppm;
        forSteps (lo, hi, stepM, [&] (double m)
        {
            const int x = juce::roundToInt (x0 + m * ppm);
            if (x > f.getX() && x < f.getRight())
                g.drawVerticalLine (x, (float) f.getY(), (float) f.getBottom());
        });
    }

    /** Dotted blue rule at x = 0, inside @p f only. */
    inline void drawOriginAxis (juce::Graphics& g, juce::Rectangle<int> f, float x0)
    {
        if (x0 <= f.getX() || x0 >= f.getRight()) return;
        const float dashes[] = { 2.0f, 3.0f };
        g.setColour (originAxis());
        g.drawDashedLine ({ x0, (float) f.getY(), x0, (float) f.getBottom() }, dashes, 2, 1.0f);
    }

    /** Dotted red rule at y = 0, inside @p f only. Same weight and dash as the
        blue one, so the pair reads as one crosshair on the origin. */
    inline void drawOriginAxisY (juce::Graphics& g, juce::Rectangle<int> f, float y0)
    {
        if (y0 <= f.getY() || y0 >= f.getBottom()) return;
        const float dashes[] = { 2.0f, 3.0f };
        g.setColour (originAxisY());
        g.drawDashedLine ({ (float) f.getX(), y0, (float) f.getRight(), y0 }, dashes, 2, 1.0f);
    }

    /** The distance row above a field, centred on each rule including the two
        on the frame's edges. A figure that would run past either end of the
        frame is pulled back inside its span, so nothing sticks out at the
        corners into the height column; one that still does not fit is skipped. */
    inline void drawXLabels (juce::Graphics& g, juce::Rectangle<int> f, float x0, float ppm,
                             double stepM, float figTop, int paneW, const char* axis = "x")
    {
        if (ppm <= 0.0f) return;
        using UiConfig::Scale::fig;
        const auto font = Brand::figSemi (12.0f);
        g.setFont (font);
        g.setColour (tick());
        const int h = (int) std::ceil (font.getHeight());
        const double lo = (f.getX() - 1 - x0) / ppm, hi = (f.getRight() + 1 - x0) / ppm;
        forSteps (lo, hi, stepM, [&] (double m)
        {
            const int x = juce::roundToInt (x0 + m * ppm);
            const auto txt = FigPlot::tick (m, stepM, axis);
            const int tw = font.getStringWidth (txt) + 2;
            if (x < f.getX() - 1 || x > f.getRight() + 1 || tw > f.getWidth() || tw > paneW) return;
            const int lx = juce::jlimit (f.getX(), f.getRight() - tw, x - tw / 2);

            // An end figure pulled inside the frame is dropped if the pull
            // crowds it against the next figure in: it must keep three
            // quarters of the usual spacing and clear the neighbour.
            if (lx != x - tw / 2)
            {
                const double inward = lx > x - tw / 2 ? stepM : -stepM;
                const int nx  = juce::roundToInt (x0 + (m + inward) * ppm);
                const int ntw = font.getStringWidth (FigPlot::tick (m + inward, stepM, axis)) + 2;
                const int gap = fig (4.0f);
                const bool overlaps = inward > 0 ? lx + tw + gap > nx - ntw / 2
                                                 : lx - gap < nx + ntw / 2;
                if (overlaps || std::abs (lx + tw / 2 - nx) < 0.75 * stepM * ppm)
                    return;
            }
            g.drawText (txt, lx, fig (figTop), tw, h, juce::Justification::centred, false);
        });
    }

    /** One figure down the left of a field, left-aligned at @p figLeft and
        centred on the rule at @p y. Dropped if it would hang off the frame. */
    inline void drawYLabel (juce::Graphics& g, juce::Rectangle<int> f, int compH, int y,
                            const juce::String& s, float figLeft, bool atEdges,
                            float stepPx = 0.0f)
    {
        // With @p atEdges the figures on the frame's top and bottom rules are
        // kept, pulled inside the frame's height so they never rise into the
        // distance row or drop below the frame; otherwise they are left out.
        // @p stepPx is the spacing to the next figure: a pulled-in figure that
        // would then sit on top of it is left out instead.
        const auto& font = g.getCurrentFont();
        const int h = (int) std::ceil (font.getHeight());
        if (y < f.getY() - 1 || y > f.getBottom() + 1 || h > f.getHeight() || h > compH) return;
        if (! atEdges && (y - h / 2 < f.getY() || y + h / 2 > f.getBottom())) return;
        const int top = juce::jlimit (f.getY(), f.getBottom() - h, y - h / 2);
        const int pulled = std::abs (top - (y - h / 2));
        if (pulled > 0 && stepPx > 0.0f
            && (stepPx - (float) pulled < (float) h + 2.0f || (float) pulled > 0.25f * stepPx))
            return;
        const int right = f.getX() - 2;
        const int x = juce::jmax (0, juce::jmin (UiConfig::Scale::fig (figLeft),
                                                 right - 2 - font.getStringWidth (s)));
        g.drawText (s, x, top, juce::jmax (1, right - x), h,
                    juce::Justification::centredLeft, false);
    }

    inline void drawMark (juce::Graphics& g, float x, float y, float sizePx)
    {
        g.setColour (mark());
        g.fillRect (x - sizePx * 0.5f, y - sizePx * 0.5f, sizePx, sizePx);
    }
}

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
//   field     white sheet, 1 px black frame (inverted per "MAPPING V1.1",
//             node 280:2019: white grid, grey boxes)
//   grid      #D9D9D9 on a 2 m step (5 ft in Imperial), no finer rule
//   venue     boxes #E4E4E4 on screen, #A0A0A0 hairline; red only when selected
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
    inline juce::Colour field()     { return juce::Colour (0xffffffff); }
    /** Venue element body; drawn at 75%, so it lands on #E4E4E4 over the field. */
    inline juce::Colour venueFill() { return juce::Colour (0xffdbdbdb); }
    inline juce::Colour venueEdge() { return juce::Colour (0xffa0a0a0); }
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
        // A grid resolution set in Settings: draw exactly that spacing while
        // its lines are at least ~8 px apart; zoomed further out, draw every
        // 2nd / 5th / 10th... line instead (CAD "adaptive grid"), always a
        // whole multiple so every rule still sits on the chosen spacing.
        if (const double userM = Units::gridResolutionM(); userM > 0.0)
        {
            const double minPx = juce::jmin (wantPx, (double) UiConfig::Scale::figf (8.0f));
            for (double k : { 1.0, 2.0, 4.0, 5.0, 10.0, 20.0, 40.0, 50.0, 100.0, 200.0, 500.0, 1000.0 })
            {
                const double st = userM * k;
                // 4x / 40x only where they stay round (0.5 ft x 4 = 2 ft).
                const double disp = Units::metresToDisplay (st);
                if ((k == 4.0 || k == 40.0) && std::abs (disp - std::round (disp)) > 1.0e-6) continue;
                if (st * pxPerM >= minPx) return st;
            }
            return userM * 1000.0;
        }
        // Metric: 0.1 .. 500 m off the 2 m base. Imperial: 3 in, 6 in, 1, 2,
        // 5, 10, 20, 50 ... ft off the 5 ft base - never 1.25 ft or 12.5 ft,
        // which no tape reads and which rounded to the wrong whole foot.
        static constexpr double multSI[] = { 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0,
                                             10.0, 25.0, 50.0, 100.0, 250.0 };
        static constexpr double multFt[] = { 0.05, 0.1, 0.2, 0.4, 1.0, 2.0, 4.0,
                                             10.0, 20.0, 40.0, 100.0, 200.0 };
        const bool imp = Units::imperial();
        const double* mult = imp ? multFt : multSI;
        for (int i = 0; i < 12; ++i)
            if (baseStepM() * mult[i] * pxPerM >= wantPx)
                return baseStepM() * mult[i];
        return baseStepM() * mult[11];
    }

    /** Decimals that print every multiple of @p stepDisplay exactly: 0 for
        whole steps, 1 for tenths, else 2 (a 0.25 ft rule). */
    inline int decimalsFor (double stepDisplay)
    {
        auto whole = [] (double v) { return std::abs (v - std::round (v)) < 1.0e-6; };
        if (whole (stepDisplay))          return 0;
        if (whole (stepDisplay * 10.0))   return 1;
        return 2;
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
        // With a grid resolution set, the numbers follow the GRID: every
        // line carries its figure as soon as the figures fit side by side
        // (spacing = the width of a figure like "-88.5ft", not a fixed
        // 40 px), else every 2nd / 5th / 10th line - always on a line.
        if (Units::gridResolutionM() > 0.0 && pxPerM > 0.0)
        {
            const int dp = decimalsFor (Units::metresToDisplay (gridM));
            const juce::String sample = juce::String (-88.0 - (dp > 0 ? 0.25 : 0.0), dp)
                                      + Units::lengthUnit();
            const double need = juce::jmin (wantPx,
                (double) Brand::figSemi (12.0f).getStringWidth (sample) + UiConfig::Scale::figf (8.0f));
            for (double k : { 1.0, 2.0, 4.0, 5.0, 10.0, 20.0, 40.0, 50.0, 100.0, 200.0, 500.0, 1000.0 })
            {
                const double st = gridM * k;
                const double disp = Units::metresToDisplay (st);
                const bool round = std::abs (disp - std::round (disp)) < 1.0e-6;
                if (k > 1.0 && disp >= 1.0 && ! round) continue;          // never 2.5 ft
                if ((k == 4.0 || k == 40.0) && ! round) continue;
                if (st * pxPerM >= need) return st;
            }
            return gridM * 1000.0;
        }
        for (double n : { 1.0, 2.0, 5.0, 10.0, 20.0 })
        {
            // Past one whole unit, number only whole ones: a 0.5 ft grid is
            // labelled 0.5, 1, 5, 10 ft - never 2.5 ft.
            const double disp = Units::metresToDisplay (gridM * n);
            if (n > 1.0 && disp >= 1.0 && std::abs (disp - std::round (disp)) > 1.0e-6) continue;
            if (gridM * n * pxPerM >= wantPx)
                return gridM * n;
        }
        return gridM * 50.0;
    }

    /** "-12m", "0(x)"; one decimal once the step drops below a whole unit. */
    inline juce::String tick (double m, double stepM, const char* axis)
    {
        if (std::abs (m) < stepM * 1.0e-3) return juce::String ("0(") + axis + ")";
        const double v = Units::metresToDisplay (m);
        const int dp = decimalsFor (Units::metresToDisplay (stepM));
        return (dp > 0 ? juce::String (v, dp) : juce::String ((int) std::lround (v)))
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

#pragma once
#include <JuceHeader.h>
#include "FigmaPlotStyle.h"

// =============================================================================
// The third Sources pane: level against distance along the centre line, read
// straight off the plan's last solve. Same sheet, grid and labels as the plan
// and the elevation, and the same horizontal mapping, so a distance here sits
// in the same column as on the two panes above.
//
// Figma V1.1 (207:1558): a 1200 x 272 field, the distance row along the top,
// "NNdb" down the left on a 6 dB step, Frequency 1 in red.
// =============================================================================
class DistanceSplPlot : public juce::Component
{
public:
    DistanceSplPlot() { setInterceptsMouseClicks (false, false); }

    void setXMapping (float pxAtZero, float pxPerM)
    {
        if (pxAtZero == x0_ && pxPerM == ppm_) return;
        x0_ = pxAtZero; ppm_ = pxPerM;
        repaint();
    }

    /** (x metres, dB) along the line; @p absolute picks the axis wording. */
    void setCurve (std::vector<juce::Point<float>> pts, bool absolute, double hz, float alongYm)
    {
        pts_ = std::move (pts); absolute_ = absolute; hz_ = hz; alongY_ = alongYm;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using UiConfig::Scale::fig;
        using UiConfig::Scale::figf;

        g.fillAll (Brand::panel());
        const auto f = FigPlot::inset (getLocalBounds(), FigPlot::kPane);
        if (f.getWidth() < 40 || f.getHeight() < 30) return;

        float x0 = x0_, ppm = ppm_;
        if (ppm <= 1.0e-6f)
        {
            ppm = figf (FigPlot::kDefaultPxPerM);
            x0  = (float) f.getX() + 6.0f * ppm;
        }

        // The dB span: the curve's own range, padded and snapped to the 6 dB
        // step, never less than 30 dB so a flat line does not fill the pane.
        constexpr double kStepDb = 6.0;
        double lo = absolute_ ? 70.0 : -36.0, hi = absolute_ ? 106.0 : 0.0;
        if (! pts_.empty())
        {
            float mn = pts_.front().y, mx = mn;
            for (const auto& p : pts_) { mn = juce::jmin (mn, p.y); mx = juce::jmax (mx, p.y); }
            lo = std::floor ((mn - 3.0) / kStepDb) * kStepDb;
            hi = std::ceil  ((mx + 3.0) / kStepDb) * kStepDb;
            if (hi - lo < 30.0) lo = hi - 30.0;
        }
        auto dy = [&] (double db)
        { return (float) (f.getBottom() - (db - lo) / (hi - lo) * f.getHeight()); };

        g.setColour (FigPlot::field());
        g.fillRect (f);

        const double gridX  = FigPlot::gridStepM (ppm, figf (30.0f));
        const double labelX = FigPlot::labelStepM (gridX, ppm, figf (40.0f));
        g.setColour (FigPlot::grid());
        FigPlot::forSteps (lo, hi, kStepDb, [&] (double db)
        {
            const int y = juce::roundToInt (dy (db));
            if (y > f.getY() && y < f.getBottom())
                g.drawHorizontalLine (y, (float) f.getX(), (float) f.getRight());
        });
        FigPlot::drawXGrid (g, f, x0, ppm, gridX);
        g.setColour (FigPlot::frame());
        g.drawRect (f, 1);

        FigPlot::drawXLabels (g, f, x0, ppm, labelX, 20.0f, getWidth());
        g.setFont (Brand::figSemi (12.0f));
        g.setColour (FigPlot::tick());
        const double labelDb = (f.getHeight() / (hi - lo)) * kStepDb >= figf (14.0f) ? kStepDb : kStepDb * 2.0;
        FigPlot::forSteps (lo, hi, labelDb, [&] (double db)
        {
            FigPlot::drawYLabel (g, f, getHeight(), juce::roundToInt (dy (db)),
                                 juce::String ((int) std::lround (db)) + "db", 24.0f, true,
                                 (float) (labelDb * f.getHeight() / (hi - lo)));
        });

        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (f.reduced (1));
        FigPlot::drawOriginAxis (g, f, x0);

        if (pts_.size() >= 2)
        {
            juce::Path p;
            for (size_t i = 0; i < pts_.size(); ++i)
            {
                const float px = x0 + pts_[i].x * ppm, py = dy (pts_[i].y);
                if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
            g.setColour (FigPlot::freq1());
            g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }

        // What the line is: the solve's frequency, and where it was read.
        const juce::String key = (hz_ > 0.0 ? juce::String (juce::roundToInt (hz_)) + " Hz" : juce::String ("-"))
                               + "  |  along y = " + FigPlot::tick (alongY_, 1.0, "y")
                               + (absolute_ ? juce::String() : juce::String ("  |  rel. dB"));
        const auto kf = Brand::figSemi (10.0f);
        g.setFont (kf);
        const int kw = kf.getStringWidth (key) + fig (22.0f);
        const int kh = (int) std::ceil (kf.getHeight()) + fig (6.0f);
        const juce::Rectangle<int> box (f.getRight() - kw - fig (8.0f), f.getY() + fig (6.0f), kw, kh);
        g.setColour (FigPlot::field().withAlpha (0.85f));
        g.fillRect (box);
        g.setColour (FigPlot::freq1());
        g.fillRect (box.getX() + fig (4.0f), box.getCentreY() - fig (5.0f), fig (8.0f), fig (10.0f));
        g.setColour (FigPlot::tick());
        g.drawText (key, box.withTrimmedLeft (fig (16.0f)), juce::Justification::centredLeft, false);

        if (pts_.empty())
        {
            g.setColour (FigPlot::tick().withAlpha (0.55f));
            g.setFont (Brand::figSemi (12.0f));
            g.drawText ("Run a prediction to see level against distance", f, juce::Justification::centred);
        }
    }

private:
    float  x0_ = 0.0f, ppm_ = 0.0f;
    std::vector<juce::Point<float>> pts_;
    bool   absolute_ = true;
    double hz_ = 0.0;
    float  alongY_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DistanceSplPlot)
};

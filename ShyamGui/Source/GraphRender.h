#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include <vector>
#include <cmath>

// ---------------------------------------------------------------------------
// GraphRender - draws clean, light-themed engineering charts to juce::Images
// for embedding in the PDF report (line charts for response curves, bar charts
// for per-band metrics). Self-contained; no UI component needed.
// ---------------------------------------------------------------------------
namespace GraphRender
{
    inline const juce::Colour kBg    { 0xffffffff };
    inline const juce::Colour kAxis  { 0xff3a3f47 };
    inline const juce::Colour kGrid  { 0xffe1e4ea };
    inline const juce::Colour kText  { 0xff20242a };
    inline const juce::Colour kSub   { 0xff5b6068 };

    inline void niceRange (double lo, double hi, double& outLo, double& outHi, double& step)
    {
        if (hi <= lo) hi = lo + 1.0;
        const double range = hi - lo;
        const double rough = range / 5.0;
        const double mag   = std::pow (10.0, std::floor (std::log10 (rough)));
        const double norm  = rough / mag;
        step = (norm < 1.5 ? 1.0 : norm < 3.0 ? 2.0 : norm < 7.0 ? 5.0 : 10.0) * mag;
        outLo = std::floor (lo / step) * step;
        outHi = std::ceil  (hi / step) * step;
    }

    // Generic chart frame. Returns the plot rectangle for the caller to fill.
    inline juce::Rectangle<int> frame (juce::Graphics& g, juce::Rectangle<int> bounds,
                                       const juce::String& title,
                                       const juce::String& xLabel, const juce::String& yLabel)
    {
        g.setColour (kText);
        g.setFont (Brand::tech (20.0f, true));
        g.drawText (title, bounds.getX(), bounds.getY() + 6, bounds.getWidth(), 26,
                    juce::Justification::centredLeft);

        auto plot = bounds.reduced (66, 40).withTrimmedTop (24).withTrimmedBottom (26);

        g.setColour (kAxis);
        g.drawRect (plot, 1);

        g.setColour (kSub);
        g.setFont (Brand::tech (12.0f));
        g.drawText (xLabel, plot.getX(), plot.getBottom() + 24, plot.getWidth(), 16,
                    juce::Justification::centred);

        // Rotated y-axis label
        {
            juce::Graphics::ScopedSaveState ss (g);
            g.addTransform (juce::AffineTransform::rotation (-juce::MathConstants<float>::halfPi,
                            (float) (bounds.getX() + 18), (float) plot.getCentreY()));
            g.drawText (yLabel, bounds.getX() + 18 - 90, plot.getCentreY() - 8, 180, 16,
                        juce::Justification::centred);
        }
        return plot;
    }

    // Line chart of (xs, ys). xLog plots the x-axis on a log scale (for Hz).
    inline juce::Image lineChart (const juce::String& title,
                                  const std::vector<double>& xs, const std::vector<double>& ys,
                                  const juce::String& xLabel, const juce::String& yLabel,
                                  juce::Colour lineCol = Brand::red(),
                                  bool xLog = true,
                                  int W = 940, int H = 440)
    {
        juce::Image img (juce::Image::RGB, W, H, true);
        juce::Graphics g (img);
        g.fillAll (kBg);
        auto plot = frame (g, img.getBounds(), title, xLabel, yLabel);
        if (xs.size() < 2) return img;

        double ylo, yhi, ystep;
        double dlo = *std::min_element (ys.begin(), ys.end());
        double dhi = *std::max_element (ys.begin(), ys.end());
        niceRange (dlo, dhi, ylo, yhi, ystep);

        const double xlo = xs.front(), xhi = xs.back();
        auto sx = [&] (double x)
        {
            const double t = xLog ? (std::log10 (x) - std::log10 (xlo)) / (std::log10 (xhi) - std::log10 (xlo))
                                  : (x - xlo) / (xhi - xlo);
            return plot.getX() + t * plot.getWidth();
        };
        auto sy = [&] (double v) { return plot.getBottom() - (v - ylo) / (yhi - ylo) * plot.getHeight(); };

        // y grid + labels
        g.setFont (Brand::tech (11.0f));
        for (double v = ylo; v <= yhi + 1e-6; v += ystep)
        {
            const float yy = (float) sy (v);
            g.setColour (kGrid); g.drawLine ((float) plot.getX(), yy, (float) plot.getRight(), yy, 1.0f);
            g.setColour (kSub);
            g.drawText (juce::String (v, (ystep < 1.0 ? 1 : 0)), plot.getX() - 56, (int) yy - 8, 50, 16,
                        juce::Justification::centredRight);
        }
        // x ticks
        for (double x : xs)
        {
            const float xx = (float) sx (x);
            g.setColour (kGrid); g.drawLine (xx, (float) plot.getY(), xx, (float) plot.getBottom(), 1.0f);
            g.setColour (kSub);
            g.drawText (juce::String ((int) x), (int) xx - 24, plot.getBottom() + 4, 48, 14,
                        juce::Justification::centred);
        }

        juce::Path path;
        for (size_t i = 0; i < xs.size(); ++i)
        {
            const float px = (float) sx (xs[i]);
            const float py = (float) sy (ys[i]);
            if (i == 0) path.startNewSubPath (px, py); else path.lineTo (px, py);
        }
        g.setColour (lineCol);
        g.strokePath (path, juce::PathStrokeType (2.5f));
        for (size_t i = 0; i < xs.size(); ++i)
        {
            g.setColour (lineCol);
            g.fillEllipse ((float) sx (xs[i]) - 3.0f, (float) sy (ys[i]) - 3.0f, 6.0f, 6.0f);
        }
        return img;
    }

    inline juce::Image barChart (const juce::String& title,
                                 const juce::StringArray& cats, const std::vector<double>& vals,
                                 const juce::String& xLabel, const juce::String& yLabel,
                                 juce::Colour barCol = Brand::red(),
                                 int W = 940, int H = 440)
    {
        juce::Image img (juce::Image::RGB, W, H, true);
        juce::Graphics g (img);
        g.fillAll (kBg);
        auto plot = frame (g, img.getBounds(), title, xLabel, yLabel);
        if (vals.empty()) return img;

        double ylo, yhi, ystep;
        double dhi = *std::max_element (vals.begin(), vals.end());
        niceRange (0.0, dhi, ylo, yhi, ystep);
        ylo = 0.0;

        auto sy = [&] (double v) { return plot.getBottom() - (v - ylo) / (yhi - ylo) * plot.getHeight(); };

        g.setFont (Brand::tech (11.0f));
        for (double v = ylo; v <= yhi + 1e-6; v += ystep)
        {
            const float yy = (float) sy (v);
            g.setColour (kGrid); g.drawLine ((float) plot.getX(), yy, (float) plot.getRight(), yy, 1.0f);
            g.setColour (kSub);
            g.drawText (juce::String (v, (ystep < 1.0 ? 2 : (ystep < 10 ? 1 : 0))),
                        plot.getX() - 56, (int) yy - 8, 50, 16, juce::Justification::centredRight);
        }

        const int n = (int) vals.size();
        const double slot = (double) plot.getWidth() / n;
        const double bw = slot * 0.58;
        for (int i = 0; i < n; ++i)
        {
            const double cx = plot.getX() + slot * (i + 0.5);
            const float top = (float) sy (vals[(size_t) i]);
            juce::Rectangle<float> bar ((float) (cx - bw * 0.5), top, (float) bw,
                                        (float) plot.getBottom() - top);
            g.setColour (barCol.withAlpha (0.85f));
            g.fillRect (bar);
            g.setColour (kSub);
            g.setFont (Brand::tech (10.5f));
            if (i < cats.size())
                g.drawText (cats[i], (int) (cx - slot * 0.5), plot.getBottom() + 4, (int) slot, 14,
                            juce::Justification::centred);
            g.setColour (kText);
            g.setFont (Brand::tech (10.0f, true));
            g.drawText (juce::String (vals[(size_t) i], (vals[(size_t) i] < 10 ? 2 : 1)),
                        (int) (cx - slot * 0.5), (int) top - 16, (int) slot, 14,
                        juce::Justification::centred);
        }
        return img;
    }
}

#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "MicReceiver.h"
#include "MicRefLevelEditor.h"
#include "AcousticEngine.h"
#include "ToolWindow.h"
#include <vector>
#include <cmath>

// ---------------------------------------------------------------------------
// Frequency Response plot - one curve per mic across kSupportedFrequencies.
// Levels are calibrated dB SPL where the device carries a sensitivity.
// Hosted in a small floating window (MicFrequencyResponseWindow).
//
// Every measurement here comes from the component's own size and the UI
// scale: gutters are measured from the text that has to fit in them, ticks
// thin out until their labels stop colliding, and the legend steps aside
// when the window is too narrow to carry it. A layout written in fixed
// pixels only looks right at the one size it was written at.
// ---------------------------------------------------------------------------
class FrequencyResponseComponent : public juce::Component
{
public:
    FrequencyResponseComponent()
    {
        title_.setText ("Frequency Response", juce::dontSendNotification);
        title_.setColour (juce::Label::textColourId, Brand::text());
        title_.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (title_);
        addAndMakeVisible (legend_);
        legend_.onReferenceChanged = [this] (int idx)
        {
            refIndex_ = idx;
            if (onReferenceChanged) onReferenceChanged (idx);
            repaint();
        };
    }

    std::function<void (int)> onReferenceChanged;

    void setCurves (const std::vector<MicReceiver>& mics,
                    const std::vector<std::vector<float>>& dbByMicByHz,
                    int refIndex, bool absolute)
    {
        absolute_ = absolute;
        mics_ = mics;
        curves_ = dbByMicByHz;
        refIndex_ = (refIndex >= 0 && refIndex < (int) mics_.size()) ? refIndex : 0;
        legend_.setMics (mics_, refIndex_);
        resized();
        repaint();
    }

    int getReferenceIndex() const noexcept { return legend_.getReferenceIndex(); }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panelDark());
        g.setColour (Brand::border());
        g.drawRect (getLocalBounds(), 1);

        const float sc     = uiScale();
        const float axisSz = juce::jmax (8.0f, 10.0f * sc);
        const float noteSz = juce::jmax (9.0f, 11.0f * sc);
        const juce::Font axisFont = Brand::tech (axisSz);

        auto plot = plotArea_;
        if (plot.getWidth() < 8 || plot.getHeight() < 8) return;

        g.setColour (Brand::plotBg());
        g.fillRect (plot);
        g.setColour (Brand::border().withAlpha (0.5f));
        g.drawRect (plot, 1);

        if (mics_.empty() || curves_.empty())
        {
            g.setColour (Brand::muted());
            g.setFont (Brand::tech (noteSz));
            g.drawFittedText ("Add mics to see frequency response", plot,
                              juce::Justification::centred, 2);
            return;
        }

        // Real levels, on a real axis. This plot used to subtract either the
        // reference mic or the curve's own band average, which made it
        // impossible to see what a filter, a gain or a delay had done: the
        // shape survived and the level - the thing you were changing -
        // normalised itself away. It shows calibrated dB SPL now.
        const int nHz = kNumSupportedFrequencies;
        // A probe that could not be taken comes back at this floor; drawing it
        // would invent a cliff that is not in the prediction.
        auto valid = [] (float db) { return db > -119.0f; };

        float peak = -1.0e9f, trough = 1.0e9f;
        for (const auto& cv : curves_)
            for (int hi = 0; hi < nHz && hi < (int) cv.size(); ++hi)
            {
                const float v = cv[(size_t) hi];
                if (! valid (v)) continue;
                peak   = juce::jmax (peak, v);
                trough = juce::jmin (trough, v);
            }

        if (peak < -1.0e8f)
        {
            g.setColour (Brand::muted());
            g.setFont (Brand::tech (noteSz));
            g.drawFittedText ("No level at these positions - run the simulation first",
                              plot, juce::Justification::centred, 2);
            return;
        }

        // The top of the window is anchored the first time data arrives and
        // only ever grows: a scale that re-centres on every solve would hide
        // the very change you are looking for. The SPAN grows to hold the
        // quietest point and no further, so a 14 dB curve fills the plot
        // instead of hugging the ceiling of a fixed 60 dB grid.
        const float wantTop = std::ceil (peak / 6.0f) * 6.0f;
        if (! axisSet_ || wantTop > axisTop_) { axisTop_ = wantTop; axisSet_ = true; }

        const float needSpan = std::ceil ((axisTop_ - trough) / 6.0f) * 6.0f + 6.0f;
        axisSpan_ = juce::jlimit (18.0f, 96.0f, juce::jmax (axisSpan_, needSpan));

        const float spanDb  = axisSpan_;
        const float axisBot = axisTop_ - spanDb;

        auto yFor = [&] (float db) -> float
        {
            const float t = juce::jlimit (axisBot, axisTop_, db);
            return plot.getBottom() - ((t - axisBot) / spanDb) * plot.getHeight();
        };

        // One gridline per readable slice of height, on a round dB step.
        const float minRowPx = juce::jmax (14.0f, axisSz * 1.7f);
        float stepDb = 3.0f;
        for (float cand : { 3.0f, 6.0f, 12.0f, 24.0f })
        {
            stepDb = cand;
            if ((cand / spanDb) * (float) plot.getHeight() >= minRowPx) break;
        }

        g.setFont (axisFont);
        const int lblW = juce::jmax (12, gutterW_ - (int) (4.0f * sc));
        for (float db = std::ceil (axisBot / stepDb) * stepDb; db <= axisTop_ + 0.01f; db += stepDb)
        {
            const float y = yFor (db);
            g.setColour (Brand::border().withAlpha (0.35f));
            g.drawHorizontalLine ((int) y, (float) plot.getX(), (float) plot.getRight());
            g.setColour (Brand::muted());
            g.drawText (juce::String ((int) std::round (db)),
                        plot.getX() - gutterW_, (int) y - (int) (axisSz * 0.8f),
                        lblW, (int) (axisSz * 1.6f),
                        juce::Justification::centredRight, false);
        }

        static const juce::uint32 kCols[] = {
            0xffffcc00, 0xff34c759, 0xff007aff, 0xffff9500,
            0xffaf52de, 0xffff3b30, 0xffffffff, 0xff5ac8fa
        };

        bool anyDrawn = false;
        for (int mi = 0; mi < (int) curves_.size(); ++mi)
        {
            const auto& cv = curves_[(size_t) mi];
            if ((int) cv.size() < nHz) continue;

            juce::Path path;
            bool started = false;
            for (int hi = 0; hi < nHz; ++hi)
            {
                const float here = cv[(size_t) hi];
                if (! valid (here)) { started = false; continue; }   // break the line
                const float x = xFor (hi, plot, nHz);
                const float y = yFor (here);
                if (! started) { path.startNewSubPath (x, y); started = true; }
                else path.lineTo (x, y);
                anyDrawn = true;
            }
            g.setColour (juce::Colour (kCols[mi % 8]));
            g.strokePath (path, juce::PathStrokeType ((mi == refIndex_ ? 2.2f : 1.6f) * sc));
        }

        if (! anyDrawn)
        {
            g.setColour (Brand::muted());
            g.setFont (Brand::tech (noteSz));
            g.drawFittedText ("No level at this position - run the simulation first",
                              plot, juce::Justification::centred, 2);
        }

        // Say what the numbers are, so nobody has to guess.
        g.setColour (Brand::muted());
        g.setFont (axisFont);
        g.drawText (absolute_ ? "dB SPL at the mic" : "dB (relative - device not calibrated)",
                    plot.getX(), plot.getY() - capH_, plot.getWidth(), capH_,
                    juce::Justification::centredLeft, false);

        // Frequency ticks, thinned until their labels stop touching.
        int maxLblW = 0;
        for (int hi = 0; hi < nHz; ++hi)
            maxLblW = juce::jmax (maxLblW,
                                  axisFont.getStringWidth (juce::String ((int) kSupportedFrequencies[hi])));
        const int slotW = maxLblW + (int) (10.0f * sc);
        const int step  = juce::jmax (1, (int) std::ceil ((double) (nHz * slotW)
                                                          / (double) juce::jmax (1, plot.getWidth())));
        g.setColour (Brand::muted());
        for (int hi = 0; hi < nHz; hi += step)
        {
            const float x = xFor (hi, plot, nHz);
            g.drawText (juce::String ((int) kSupportedFrequencies[hi]),
                        (int) x - slotW / 2, plot.getBottom() + (int) (3.0f * sc),
                        slotW, tickH_, juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        const float sc     = uiScale();
        const int   pad    = (int) (6.0f * sc);
        const float axisSz = juce::jmax (8.0f, 10.0f * sc);
        const juce::Font axisFont = Brand::tech (axisSz);

        title_.setFont (Brand::techSemi (juce::jmax (10.0f, 12.0f * sc)));
        legend_.setUiScaleBoost (sc / juce::jmax (0.5f, Brand::UI::scale));

        auto r = getLocalBounds().reduced (pad);
        title_.setBounds (r.removeFromTop ((int) (18.0f * sc)));
        r.removeFromTop ((int) (2.0f * sc));

        // The legend earns its width only while there is room for the plot
        // beside it; below that it would squeeze the curves into a sliver.
        const int legendW = juce::jlimit ((int) (84.0f * sc),
                                          (int) (170.0f * sc),
                                          r.getWidth() / 4);
        const bool showLegend = (! mics_.empty())
                              && r.getWidth() - legendW > (int) (190.0f * sc);
        legend_.setVisible (showLegend);
        if (showLegend)
        {
            auto strip = r.removeFromRight (legendW);
            legend_.setBounds (strip.withHeight (juce::jmin (strip.getHeight(),
                                                             legend_.preferredHeight())));
            r.removeFromRight ((int) (4.0f * sc));
        }

        // Gutters measured from the text that has to live in them.
        gutterW_ = axisFont.getStringWidth ("-120") + (int) (8.0f * sc);
        tickH_   = (int) std::ceil (axisFont.getHeight()) + (int) (2.0f * sc);
        capH_    = (int) std::ceil (axisFont.getHeight()) + (int) (2.0f * sc);

        plotArea_ = r.withTrimmedLeft (gutterW_)
                     .withTrimmedBottom (tickH_ + (int) (3.0f * sc))
                     .withTrimmedTop (capH_);
    }

private:
    // The UI scale, nudged up a little on a big window. Text pinned to the
    // global scale alone reads fine at 360 px tall and like fine print at
    // 980; it is capped so the plot never turns into four huge numbers.
    float uiScale() const noexcept
    {
        const float base = juce::jmax (0.5f, Brand::UI::scale);
        const float grow = juce::jlimit (1.0f, 1.6f, (float) getHeight() / 360.0f);
        return base * grow;
    }

    static float xFor (int hzIndex, juce::Rectangle<int> plot, int nHz)
    {
        return plot.getX()
             + ((float) hzIndex / (float) juce::jmax (1, nHz - 1)) * plot.getWidth();
    }

    juce::Label title_;
    MicRefLevelEditor legend_;
    juce::Rectangle<int> plotArea_;
    int   gutterW_ = 30;
    int   tickH_   = 12;
    int   capH_    = 12;
    std::vector<MicReceiver> mics_;
    std::vector<std::vector<float>> curves_;   // [mic][hzIndex] calibrated dB SPL
    int   refIndex_ = 0;
    bool  absolute_ = false;
    // The dB window is anchored on first data and only grows, so a change in
    // level is visible instead of being re-centred away.
    float axisTop_  = 0.0f;
    float axisSpan_ = 18.0f;
    bool  axisSet_  = false;
};

// ---------------------------------------------------------------------------
// Small floating FR window - shown while mics exist; does not resize the plot.
// ---------------------------------------------------------------------------
class MicFrequencyResponseWindow : public ToolWindow
{
public:
    MicFrequencyResponseWindow()
        : ToolWindow ("Frequency Response")
    {
        setResizable (true, false);
        setToolContent (content, false);
        // Small enough to tuck into a corner; no practical ceiling, so
        // maximise and Full screen fill any monitor.
        const float sc = juce::jmax (0.5f, Brand::UI::scale);
        setResizeLimits ((int) (300 * sc), (int) (180 * sc), 10000, 10000);
        centreWithSize ((int) (620 * sc), (int) (360 * sc));
    }

    void closeButtonPressed() override { setVisible (false); }

    FrequencyResponseComponent content;
};

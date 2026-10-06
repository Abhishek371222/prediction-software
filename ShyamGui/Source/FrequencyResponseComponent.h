#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "MicReceiver.h"
#include "MicRefLevelEditor.h"
#include "AcousticEngine.h"
#include <vector>
#include <cmath>

// ---------------------------------------------------------------------------
// Frequency Response plot - one curve per mic across kSupportedFrequencies.
// Curves are relative to the reference mic (*). Hosted in a small floating
// window (MicFrequencyResponseWindow) so the main UI layout is unchanged.
// ---------------------------------------------------------------------------
class FrequencyResponseComponent : public juce::Component
{
public:
    FrequencyResponseComponent()
    {
        title_.setText ("Frequency Response", juce::dontSendNotification);
        title_.setFont (Brand::techSemi (Brand::UI::scaledFont (12.0f)));
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
                    int refIndex)
    {
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

        auto plot = plotBounds();
        g.setColour (Brand::plotBg());
        g.fillRect (plot);
        g.setColour (Brand::border().withAlpha (0.5f));
        g.drawRect (plot, 1);

        if (mics_.empty() || curves_.empty())
        {
            g.setColour (Brand::muted());
            g.setFont (Brand::tech (11.0f));
            g.drawText ("Add mics to see frequency response", plot,
                        juce::Justification::centred);
            return;
        }

        // With a single mic there is nothing to compare against: subtracting
        // the reference from itself drew one dead flat line and the window
        // looked broken. Alone, a mic shows its OWN shape instead - the curve
        // against its average across the band, which is what "what does this
        // seat sound like" actually means.
        const bool comparing = (curves_.size() >= 2);

        // Axes: 0 dB at mid, ±24 dB span.
        constexpr float spanDb = 24.0f;
        auto yFor = [&] (float relDb) -> float
        {
            const float t = juce::jlimit (-spanDb, spanDb, relDb);
            return plot.getBottom() - ((t + spanDb) / (2.0f * spanDb)) * plot.getHeight();
        };

        g.setFont (Brand::tech (9.0f));
        for (int db = -24; db <= 24; db += 6)
        {
            const float y = yFor ((float) db);
            g.setColour (Brand::border().withAlpha (0.35f));
            g.drawHorizontalLine ((int) y, (float) plot.getX(), (float) plot.getRight());
            // Numbers on the scale: a grid you cannot read a value off is
            // decoration.
            g.setColour (Brand::muted());
            g.drawText (juce::String (db > 0 ? "+" : "") + juce::String (db),
                        plot.getX() - 30, (int) y - 6, 26, 12,
                        juce::Justification::centredRight);
        }
        g.setColour (Brand::accent().withAlpha (0.45f));
        g.drawHorizontalLine ((int) yFor (0.0f), (float) plot.getX(), (float) plot.getRight());

        static const juce::uint32 kCols[] = {
            0xffffcc00, 0xff34c759, 0xff007aff, 0xffff9500,
            0xffaf52de, 0xffff3b30, 0xffffffff, 0xff5ac8fa
        };

        const int nHz = kNumSupportedFrequencies;
        // A probe that could not be taken comes back at this floor; drawing it
        // would invent a cliff that is not in the prediction.
        auto valid = [] (float db) { return db > -119.0f; };

        bool anyDrawn = false;
        for (int mi = 0; mi < (int) curves_.size(); ++mi)
        {
            const auto& cv = curves_[(size_t) mi];
            if ((int) cv.size() < nHz) continue;

            // What zero means on the axis: the reference mic when there is one
            // to compare with, otherwise this curve's own band average.
            float zero = 0.0f;
            if (! comparing)
            {
                double sum = 0.0; int n = 0;
                for (int hi = 0; hi < nHz; ++hi)
                    if (valid (cv[(size_t) hi])) { sum += cv[(size_t) hi]; ++n; }
                if (n == 0) continue;
                zero = (float) (sum / n);
            }

            juce::Path path;
            bool started = false;
            for (int hi = 0; hi < nHz; ++hi)
            {
                const float here = cv[(size_t) hi];
                if (! valid (here)) { started = false; continue; }   // break the line

                float rel = here - zero;
                if (comparing && refIndex_ >= 0 && refIndex_ < (int) curves_.size()
                    && (int) curves_[(size_t) refIndex_].size() > hi)
                {
                    const float r = curves_[(size_t) refIndex_][(size_t) hi];
                    if (! valid (r)) { started = false; continue; }
                    rel = here - r;
                }

                const float x = plot.getX()
                    + ((float) hi / (float) juce::jmax (1, nHz - 1)) * plot.getWidth();
                const float y = yFor (rel);
                if (! started) { path.startNewSubPath (x, y); started = true; }
                else path.lineTo (x, y);
                anyDrawn = true;
            }
            g.setColour (juce::Colour (kCols[mi % 8]));
            g.strokePath (path, juce::PathStrokeType (
                (comparing && mi == refIndex_) ? 2.2f : 1.6f));
        }

        if (! anyDrawn)
        {
            g.setColour (Brand::muted());
            g.setFont (Brand::tech (11.0f));
            g.drawText ("No level at this position - run the simulation first",
                        plot, juce::Justification::centred);
        }

        // Say which zero this is, so a flat trace is never ambiguous.
        g.setColour (Brand::muted());
        g.setFont (Brand::tech (9.0f));
        g.drawText (comparing ? "dB re. reference mic" : "dB re. this mic's band average",
                    plot.getX(), plot.getY() - 12, plot.getWidth(), 12,
                    juce::Justification::topLeft);

        // Frequency ticks
        g.setFont (Brand::tech (9.0f));
        g.setColour (Brand::muted());
        for (int hi = 0; hi < nHz; hi += 2)
        {
            const float x = plot.getX()
                + ((float) hi / (float) juce::jmax (1, nHz - 1)) * plot.getWidth();
            g.drawText (juce::String ((int) kSupportedFrequencies[hi]),
                        (int) x - 16, plot.getBottom() + 2, 32, 12,
                        juce::Justification::centred);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (6);
        title_.setBounds (r.removeFromTop (18));
        r.removeFromTop (2);
        const int legendW = 110;
        legend_.setBounds (r.removeFromRight (legendW));
        r.removeFromRight (4);
        plotArea_ = r.withTrimmedBottom (14).withTrimmedLeft (30).withTrimmedTop (12);
        legend_.setBounds (legend_.getBounds().withHeight (legend_.preferredHeight()));
    }

private:
    juce::Rectangle<int> plotBounds() const { return plotArea_; }

    juce::Label title_;
    MicRefLevelEditor legend_;
    juce::Rectangle<int> plotArea_;
    std::vector<MicReceiver> mics_;
    std::vector<std::vector<float>> curves_; // [mic][hzIndex] absolute-ish intensity dB
    int refIndex_ = 0;
};

// ---------------------------------------------------------------------------
// Small floating FR window - shown while mics exist; does not resize the plot.
// ---------------------------------------------------------------------------
class MicFrequencyResponseWindow : public juce::DocumentWindow
{
public:
    MicFrequencyResponseWindow()
        : DocumentWindow ("Frequency Response",
                          Brand::panelDark(),
                          DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        setContentNonOwned (&content, true);
        setResizeLimits (360, 200, 900, 520);
        centreWithSize (520, 280);
    }

    void closeButtonPressed() override { setVisible (false); }

    FrequencyResponseComponent content;
};

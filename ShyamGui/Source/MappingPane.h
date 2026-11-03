#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "MicReceiver.h"
#include "AcousticEngine.h"
#include "SourcePanel.h"
#include <cmath>
#include <vector>

// =============================================================================
// The Mapping tab's furniture, from the MAPPING V1.1 frame: the Frequency
// Response pane under the plan, and the Calculate footer pinned to the bottom
// of the side panel. Every number is a Figma pixel, mapped through fig().
// =============================================================================
namespace MappingSkin
{
    inline juce::Colour blue()  { return juce::Colour (0xff3281b9); }
    inline juce::Colour grid()  { return juce::Colour (0xffd9d9d9); }

    /** Microphone n's colour, the same in the legend and on the curve. */
    inline juce::Colour micColour (int i)
    {
        static const juce::uint32 cols[] = { 0xffff0404, 0xff3281b9, 0xff00b54c, 0xffff9500,
                                             0xffaf52de, 0xff5ac8fa, 0xffffcc00, 0xff8e8e93 };
        return juce::Colour (cols[(size_t) juce::jmax (0, i) % 8]);
    }
}

// -----------------------------------------------------------------------------
// One curve per mic over 20 Hz - 20 kHz on a log axis, levels relative to the
// loudest point on any curve, 0 to -36 dB - the same span as the plan's bar.
// -----------------------------------------------------------------------------
class MappingResponsePlot : public juce::Component
{
public:
    MappingResponsePlot()
    {
        micIcon_ = Brand::assetImage ("RibbonIcons/Mic.png");
    }

    void setCurves (const std::vector<MicReceiver>& mics,
                    const std::vector<std::vector<float>>& dbByMicByHz)
    {
        mics_   = mics;
        curves_ = dbByMicByHz;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        using UiConfig::Scale::fig;
        using UiConfig::Scale::figf;
        g.fillAll (Brand::white());

        const auto plot = plotArea();
        g.setColour (Brand::white());
        g.fillRect (plot);

        // Third-octave-ish vertical rules on every 1-2-5 step, decades darker.
        for (double dec = 10.0; dec <= 10000.0; dec *= 10.0)
            for (int k = 1; k <= 9; ++k)
            {
                const double f = dec * k;
                if (f < kLoHz || f > kHiHz * 1.4) continue;
                g.setColour (k == 1 ? MappingSkin::grid().darker (0.15f) : MappingSkin::grid());
                g.drawVerticalLine (juce::roundToInt (xFor (f, plot)), plot.getY(), plot.getBottom());
            }
        for (int db = 0; db >= -36; db -= 6)
        {
            g.setColour (MappingSkin::grid());
            g.drawHorizontalLine (juce::roundToInt (yFor ((float) db, plot)), plot.getX(), plot.getRight());
        }
        g.setColour (juce::Colour (0xff000000));
        g.drawRect (plot.toNearestInt(), 1);

        // dB down the left, right-aligned 13 px short of the plot.
        g.setColour (Brand::text());
        g.setFont (Brand::figMed (12.0f));
        for (int db = 0; db >= -36; db -= 6)
        {
            const float y = yFor ((float) db, plot);
            g.drawText (juce::String (db) + " db",
                        juce::Rectangle<float> (0.0f, y - figf (8.0f), plot.getX() - figf (13.0f), figf (16.0f)),
                        juce::Justification::centredRight, false);
        }
        g.setFont (Brand::figSemi (10.0f));
        g.drawText ("Rel. SPL", juce::Rectangle<float> (figf (12.0f), plot.getBottom() - figf (24.0f),
                                                        plot.getX() - figf (14.0f), figf (14.0f)),
                    juce::Justification::centredLeft, false);
        // The curves arrive referenced to one metre from the nearest cabinet,
        // not to each mic's own distance, so a mic further back no longer
        // simply sits lower. ArrayCalc marks its equivalent the same way.
        // It goes in the legend column, clear of the frequency ticks - at the
        // plot's bottom right it sat on top of "20K".
        g.setColour (Brand::text().withAlpha (0.55f));
        g.setFont (Brand::figMed (10.0f));
        g.drawFittedText ("(distance-\ncompensated)",
                          juce::Rectangle<float> (plot.getRight() + figf (12.0f),
                                                  (float) getHeight() - figf (34.0f),
                                                  figf (120.0f), figf (28.0f)).toNearestInt(),
                          juce::Justification::bottomLeft, 2);
        g.setColour (Brand::text());

        // Frequencies along the bottom.
        g.setFont (Brand::figSemi (14.0f));
        const struct { double hz; const char* text; } ticks[] = {
            { 20, "20" }, { 50, "50" }, { 100, "100" }, { 200, "200" }, { 500, "500" },
            { 1000, "1K" }, { 2000, "2K" }, { 5000, "5K" }, { 10000, "10K" }, { 20000, "20K" } };
        for (const auto& t : ticks)
        {
            const float x = xFor (t.hz, plot);
            g.drawText (t.text, juce::Rectangle<float> (x - figf (24.0f), plot.getBottom() + figf (10.0f),
                                                        figf (48.0f), figf (18.0f)),
                        juce::Justification::centred, false);
        }

        drawCurves (g, plot);
        drawLegend (g, plot);
    }

private:
    static constexpr double kLoHz = 20.0, kHiHz = 20000.0;

    juce::Rectangle<float> plotArea() const
    {
        using UiConfig::Scale::figf;
        // Figma: the plot runs x 69..1386 of the pane and fills its height
        // bar 15 px above and the tick row below; the legend takes the rest.
        const float right = (float) getWidth() - figf (135.0f);
        return { figf (69.0f), figf (15.0f),
                 juce::jmax (40.0f, right - figf (69.0f)),
                 juce::jmax (40.0f, (float) getHeight() - figf (15.0f) - figf (39.0f)) };
    }

    /** 20 Hz sits on the left edge; 20 kHz 92% across, as the Figma sets it. */
    static float xFor (double hz, juce::Rectangle<float> plot)
    {
        const double t = std::log10 (juce::jmax (1.0, hz) / kLoHz) / std::log10 (kHiHz / kLoHz);
        return plot.getX() + plot.getWidth() * (0.004f + 0.913f * (float) t);
    }

    /** 0 dB 13% down, -36 dB 84% down - the Figma's margins. */
    static float yFor (float db, juce::Rectangle<float> plot)
    {
        const float t = juce::jlimit (-36.0f, 0.0f, db) / -36.0f;
        return plot.getY() + plot.getHeight() * (0.133f + 0.708f * t);
    }

    void drawCurves (juce::Graphics& g, juce::Rectangle<float> plot)
    {
        auto valid = [] (float db) { return db > -119.0f; };
        float peak = -1.0e9f;
        for (const auto& cv : curves_)
            for (float v : cv)
                if (valid (v)) peak = juce::jmax (peak, v);

        if (mics_.empty() || peak < -1.0e8f)
        {
            g.setColour (Brand::text().withAlpha (0.55f));
            g.setFont (Brand::figMed (12.0f));
            g.drawText (mics_.empty() ? "Place microphones on the plan to see their frequency response"
                                      : "No level at the microphones - run Calculate",
                        plot.toNearestInt(), juce::Justification::centred, true);
            return;
        }

        const int n = juce::jmin (kNumSupportedFrequencies, (int) (curves_.empty() ? 0 : curves_[0].size()));
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (plot.toNearestInt());
        for (int mi = 0; mi < (int) curves_.size(); ++mi)
        {
            const auto& cv = curves_[(size_t) mi];
            juce::Path p;
            bool started = false;
            for (int hi = 0; hi < n && hi < (int) cv.size(); ++hi)
            {
                if (! valid (cv[(size_t) hi])) { started = false; continue; }
                const float x = xFor (kSupportedFrequencies[hi], plot);
                const float y = yFor (cv[(size_t) hi] - peak, plot);
                if (! started) { p.startNewSubPath (x, y); started = true; }
                else p.lineTo (x, y);
            }
            g.setColour (MappingSkin::micColour (mi));
            g.strokePath (p, juce::PathStrokeType (juce::jmax (1.5f, UiConfig::Scale::figf (2.0f)),
                                                   juce::PathStrokeType::curved,
                                                   juce::PathStrokeType::rounded));
        }
    }

    // Figma: rows 39 apart from the top; the mic at 50%, a 5 x 19 colour bar
    // after it, then "Microphone n" in Medium 12.
    void drawLegend (juce::Graphics& g, juce::Rectangle<float> plot)
    {
        using UiConfig::Scale::figf;
        const float x0 = plot.getRight() + figf (12.0f);
        float y = figf (1.0f);
        g.setFont (Brand::figMed (12.0f));
        for (int i = 0; i < (int) mics_.size(); ++i)
        {
            const float icon = figf (21.3f);
            if (micIcon_.isValid())
            {
                g.setOpacity (0.5f);
                g.drawImage (micIcon_, juce::Rectangle<float> (x0, y, icon, icon),
                             juce::RectanglePlacement::centred);
                g.setOpacity (1.0f);
            }
            g.setColour (MappingSkin::micColour (i));
            g.fillRect (x0 + figf (21.3f), y + figf (1.8f), figf (5.3f), figf (18.7f));
            g.setColour (Brand::text());
            g.drawText ("Microphone " + juce::String (mics_[(size_t) i].id),
                        juce::Rectangle<float> (x0 + figf (32.0f), y, (float) getWidth() - x0 - figf (32.0f), icon),
                        juce::Justification::centredLeft, true);
            y += figf (39.0f);
            if (y + icon > (float) getHeight()) break;
        }
    }

    juce::Image micIcon_;
    std::vector<MicReceiver> mics_;
    std::vector<std::vector<float>> curves_;
};

// -----------------------------------------------------------------------------
// The footer: "Frequency to mapping" on a blue strip, then Calculate and
// Auto - Calculate. White, with a soft shadow cast up over the panel.
// -----------------------------------------------------------------------------
class MappingFooter : public juce::Component
{
public:
    MappingFooter()
    {
        freq_.setLookAndFeel (&lnf_);
        freq_.setColour (juce::ComboBox::backgroundColourId, juce::Colours::white.withAlpha (0.4f));
        freq_.onChange = [this] { if (! building_ && onFrequency) onFrequency (freq_.getSelectedItemIndex()); };
        addAndMakeVisible (freq_);

        calc_.onClick = [this] { if (onCalculate) onCalculate(); };
        auto_.setClickingTogglesState (true);
        auto_.setToggleState (true, juce::dontSendNotification);
        auto_.onClick = [this] { if (onAutoCalculate) onAutoCalculate (auto_.getToggleState()); };
        addAndMakeVisible (calc_);
        addAndMakeVisible (auto_);
    }

    ~MappingFooter() override { freq_.setLookAndFeel (nullptr); }

    std::function<void (int index)> onFrequency;
    std::function<void()>           onCalculate;
    std::function<void (bool on)>   onAutoCalculate;

    bool isAutoCalculate() const { return auto_.getToggleState(); }

    /** Figma px the footer takes, shadow excluded. */
    static constexpr float kFigHeight = 116.0f;
    static int shadowHeight() { return UiConfig::Scale::fig (10.0f); }

    void setFrequencies (const juce::StringArray& items, int selectedIndex)
    {
        building_ = true;
        bool same = items.size() == freq_.getNumItems();
        for (int i = 0; same && i < items.size(); ++i)
            same = freq_.getItemText (i) == items[i].removeCharacters (" ");
        if (! same)
        {
            freq_.clear (juce::dontSendNotification);
            for (int i = 0; i < items.size(); ++i)
                freq_.addItem (items[i].removeCharacters (" "), i + 1);
        }
        freq_.setSelectedItemIndex (selectedIndex, juce::dontSendNotification);
        building_ = false;
    }

    void paint (juce::Graphics& g) override
    {
        using UiConfig::Scale::fig;
        const int sh = shadowHeight();
        juce::ColourGradient shade (juce::Colours::black.withAlpha (0.0f), 0.0f, 0.0f,
                                    juce::Colours::black.withAlpha (0.18f), 0.0f, (float) sh, false);
        g.setGradientFill (shade);
        g.fillRect (0, 0, getWidth(), sh);

        auto body = getLocalBounds().withTrimmedTop (sh);
        g.setColour (Brand::white());
        g.fillRect (body);

        const auto strip = body.withHeight (fig (44.0f));
        g.setColour (MappingSkin::blue());
        g.fillRect (strip);
        g.setColour (juce::Colours::black);
        g.drawRect (strip, 1);
        g.setColour (Brand::white());
        g.setFont (Brand::figMed (12.0f));
        g.drawText ("Frequency to mapping", strip.withTrimmedLeft (fig (14.0f)),
                    juce::Justification::centredLeft, true);
    }

    void resized() override
    {
        using UiConfig::Scale::fig;
        const int top = shadowHeight();
        freq_.setBounds (fig (282.0f), top + fig (7.0f), fig (102.0f), fig (28.0f));
        calc_.setBounds (fig (24.0f),    top + fig (58.0f), fig (161.0f), fig (44.0f));
        auto_.setBounds (fig (217.6f),   top + fig (58.0f), fig (161.0f), fig (44.0f));
    }

private:
    // The combo on the blue strip: translucent white, white text and arrow.
    struct StripLnF : public SourcePanel::PanelLnF
    {
        void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int,
                           juce::ComboBox& box) override
        {
            g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
            g.fillRoundedRectangle (0.0f, 0.0f, (float) w, (float) h, UiConfig::Scale::figf (2.0f));
            const float hw = UiConfig::Scale::figf (5.6f), hh = UiConfig::Scale::figf (2.9f);
            const float cx = (float) w - UiConfig::Scale::figf (17.4f), cy = (float) h * 0.5f;
            juce::Path t;
            t.addTriangle (cx - hw, cy - hh, cx + hw, cy - hh, cx, cy + hh);
            g.setColour (Brand::white());
            g.fillPath (t);
        }

        void positionComboBoxText (juce::ComboBox& box, juce::Label& l) override
        {
            SourcePanel::PanelLnF::positionComboBoxText (box, l);
            l.setBounds (UiConfig::Scale::fig (9.0f), 0,
                         box.getWidth() - UiConfig::Scale::fig (37.0f), box.getHeight());
            l.setColour (juce::Label::textColourId, Brand::white());
        }
    };

    // Figma: #3281B9, 2 px corners, white Medium 16. Auto - Calculate shows
    // its off state as the outline of the same button.
    struct BlueButton : public juce::Button
    {
        explicit BlueButton (const juce::String& t) : juce::Button (t) {}

        void paintButton (juce::Graphics& g, bool over, bool down) override
        {
            const auto r = getLocalBounds().toFloat().reduced (0.5f);
            const float rad = UiConfig::Scale::figf (2.0f);
            const bool solid = ! getClickingTogglesState() || getToggleState();
            auto fill = MappingSkin::blue();
            if (down) fill = fill.darker (0.2f);
            else if (over) fill = fill.brighter (0.1f);
            if (solid)
            {
                g.setColour (fill);
                g.fillRoundedRectangle (r, rad);
            }
            else
            {
                g.setColour (Brand::white());
                g.fillRoundedRectangle (r, rad);
                g.setColour (fill);
                g.drawRoundedRectangle (r, rad, juce::jmax (1.0f, UiConfig::Scale::figf (1.5f)));
            }
            g.setColour (solid ? Brand::white() : fill);
            g.setFont (Brand::figMed (16.0f));
            g.drawText (getButtonText(), getLocalBounds(), juce::Justification::centred, false);
        }
    };

    StripLnF   lnf_;
    juce::ComboBox freq_;
    BlueButton calc_ { "Calculate" };
    BlueButton auto_ { "Auto - Calculate" };
    bool building_ = false;
};

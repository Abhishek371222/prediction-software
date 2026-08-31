#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "MicReceiver.h"
#include <vector>

// ---------------------------------------------------------------------------
// FR legend: left-click a mic row to set it as the reference (*).
// ---------------------------------------------------------------------------
class MicRefLevelEditor : public juce::Component
{
public:
    std::function<void (int refMicIndex)> onReferenceChanged;

    void setMics (const std::vector<MicReceiver>& mics, int refIndex)
    {
        mics_ = mics;
        refIndex_ = (refIndex >= 0 && refIndex < (int) mics_.size()) ? refIndex : 0;
        repaint();
    }

    int getReferenceIndex() const noexcept { return refIndex_; }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panelDark());
        g.setFont (Brand::tech (Brand::UI::scaledFont (11.0f)));
        int y = 4;
        for (int i = 0; i < (int) mics_.size(); ++i)
        {
            const bool isRef = (i == refIndex_);
            g.setColour (isRef ? Brand::accent() : Brand::text());
            juce::String line = (isRef ? "* " : "  ") + micDisplayName (mics_[(size_t) i]);
            if (mics_[(size_t) i].levelOk)
                line += "  " + juce::String (mics_[(size_t) i].relDb, 1) + " dB";
            g.drawText (line, 8, y, getWidth() - 16, 16, juce::Justification::centredLeft, false);
            y += 18;
        }
        if (mics_.empty())
        {
            g.setColour (Brand::muted());
            g.drawText ("No mics", getLocalBounds(), juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int row = (e.y - 4) / 18;
        if (row < 0 || row >= (int) mics_.size()) return;
        if (refIndex_ == row) return;
        refIndex_ = row;
        if (onReferenceChanged) onReferenceChanged (refIndex_);
        repaint();
    }

    int preferredHeight() const noexcept
    {
        return juce::jmax (24, 8 + 18 * juce::jmax (1, (int) mics_.size()));
    }

private:
    std::vector<MicReceiver> mics_;
    int refIndex_ = 0;
};

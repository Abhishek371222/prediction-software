#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "MicReceiver.h"
#include <vector>
#include <cmath>

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
        g.setFont (Brand::tech (rowFontSize()));
        const int rowH = rowHeight();
        const int inset = sidePad();
        int y = topPad();
        for (int i = 0; i < (int) mics_.size(); ++i)
        {
            const bool isRef = (i == refIndex_);
            g.setColour (isRef ? Brand::accent() : Brand::text());
            juce::String line = (isRef ? "* " : "  ") + micDisplayName (mics_[(size_t) i]);
            if (mics_[(size_t) i].levelOk)
                line += "  " + juce::String (mics_[(size_t) i].relDb, 1) + " dB";
            g.drawText (line, inset, y, juce::jmax (8, getWidth() - 2 * inset), rowH,
                        juce::Justification::centredLeft, false);
            y += rowH;
        }
        if (mics_.empty())
        {
            g.setColour (Brand::muted());
            g.drawText ("No mics", getLocalBounds(), juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const int row = (e.y - topPad()) / rowHeight();
        if (row < 0 || row >= (int) mics_.size()) return;
        if (refIndex_ == row) return;
        refIndex_ = row;
        if (onReferenceChanged) onReferenceChanged (refIndex_);
        repaint();
    }

    // The host plot grows its own text on a large window; the legend beside
    // it has to grow with it or the two stop looking like one panel.
    void setUiScaleBoost (float boost)
    {
        boost = juce::jlimit (1.0f, 2.0f, boost);
        if (std::abs (boost - uiScaleBoost_) < 0.01f) return;
        uiScaleBoost_ = boost;
        repaint();
    }

    int preferredHeight() const noexcept
    {
        return juce::jmax (rowHeight() + 2 * topPad(),
                           2 * topPad() + rowHeight() * juce::jmax (1, (int) mics_.size()));
    }

private:
    // Row metrics follow the UI scale, like the text in them. Written as
    // fixed pixels, the hit test and the drawn rows drift apart the moment
    // the scale moves off 1.0, and clicking a mic selects the one above it.
    float scale() const noexcept
    {
        return juce::jmax (0.5f, Brand::UI::scale) * uiScaleBoost_;
    }
    float rowFontSize() const noexcept { return juce::jmax (9.0f, 11.0f * scale()); }
    int   rowHeight()   const noexcept { return (int) (18.0f * scale()); }
    int   topPad()      const noexcept { return (int) (4.0f * scale()); }
    int   sidePad()     const noexcept { return (int) (8.0f * scale()); }

    float uiScaleBoost_ = 1.0f;
    std::vector<MicReceiver> mics_;
    int refIndex_ = 0;
};

#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "AppSettings.h"
#include "AcousticEngine.h"

// ---------------------------------------------------------------------------
// Read-only speaker Properties panel (name, cabinet size, position, DSP).
// ---------------------------------------------------------------------------
class SpeakerPropertiesDialog : public juce::Component
{
public:
    SpeakerPropertiesDialog (const Speaker& spk, int index)
    {
        title_.setText ("Speaker Properties", juce::dontSendNotification);
        title_.setFont (Brand::techSemi (Brand::UI::scaledFont (15.0f)));
        title_.setColour (juce::Label::textColourId, Brand::heading());
        addAndMakeVisible (title_);

        const juce::String modelName = speakerModelName (spk.model);
        const juce::String name = modelName + "-" + juce::String (index + 1);
        const juce::String u = Units::lengthUnit();

        addRow ("Name", name);
        addRow ("Product", "Atomik " + modelName);
        // Cabinet dimensions are only specified for Q21S; 15W750's physical
        // enclosure isn't modelled yet, so don't show Q21S's numbers under
        // its name.
        if (spk.model != 2)
        {
            addRow ("Dimensions (W × H × D)",
                   Units::mm (Q21SCabinet::widthM * 1000.0, 0) + " × "
                   + Units::mm (Q21SCabinet::heightM * 1000.0, 0) + " × "
                   + Units::mm (Q21SCabinet::depthM * 1000.0, 0));
            addRow ("Plan footprint (W × D)",
                   Units::mm (Q21SCabinet::widthM * 1000.0, 0) + " × "
                   + Units::mm (Q21SCabinet::depthM * 1000.0, 0));
        }
        addRow ("Position",
               "(" + juce::String (Units::metresToDisplay (spk.x), 2) + ", "
               + juce::String (Units::metresToDisplay (spk.y), 2) + ") " + u);
        addRow ("Gain", juce::String (spk.gainDB, 0) + " dB");
        addRow ("Delay", juce::String (spk.delayMs, 1) + " ms");
        addRow ("Polarity", spk.polarityInverted ? "Reverse" : "Normal");
        addRow ("Orientation", spk.reverseOrientation ? "Reverse (−x)" : "Forward (+x)");
        addRow ("Enabled", spk.enabled ? "Yes" : "No");

        setSize (360, 44 + (int) rows_.size() * 26 + 16);
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());
        g.setColour (Brand::border().withAlpha (0.55f));
        g.drawRect (getLocalBounds(), 1);
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (14, 12);
        title_.setBounds (r.removeFromTop (22));
        r.removeFromTop (8);
        for (auto* row : rows_)
        {
            auto line = r.removeFromTop (24);
            row->key.setBounds (line.removeFromLeft (line.getWidth() / 2));
            row->val.setBounds (line);
            r.removeFromTop (2);
        }
    }

private:
    struct Row
    {
        juce::Label key, val;
    };

    void addRow (const juce::String& key, const juce::String& val)
    {
        auto* row = rows_.add (new Row());
        row->key.setText (key, juce::dontSendNotification);
        row->key.setFont (Brand::tech (Brand::UI::scaledFont (12.0f)));
        row->key.setColour (juce::Label::textColourId, Brand::muted());
        row->key.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (row->key);

        row->val.setText (val, juce::dontSendNotification);
        row->val.setFont (Brand::mono (Brand::UI::scaledFont (12.0f), true));
        row->val.setColour (juce::Label::textColourId, Brand::text());
        row->val.setJustificationType (juce::Justification::centredRight);
        addAndMakeVisible (row->val);
    }

    juce::Label title_;
    juce::OwnedArray<Row> rows_;
};

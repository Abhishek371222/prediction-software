#pragma once
#include <JuceHeader.h>
#include "AcousticEngine.h"
#include "BrandTheme.h"
#include "ProjectData.h"
#include "LayoutLayer.h"
#include "UiChrome.h"
#include <vector>
#include <functional>

// ---------------------------------------------------------------------------
// ControlPanel — owns the editable scene (speaker list + global settings) and
// is the single source of truth for SimParams. Any edit fires onChanged so
// MainComponent can recompute; speaker drags in the renderer are pushed back
// here via setSpeakerPosition().
// ---------------------------------------------------------------------------
class ControlPanel : public juce::Component
{
public:
    ControlPanel();
    ~ControlPanel() override;

    std::function<void()> onWillEdit;          // before a user-visible mutation
    std::function<void()>    onChanged;            // any parameter changed
    std::function<void()>    onRunClicked;         // explicit recompute
    std::function<void()>    onClearAll;           // clear SPL heatmap drawings / lines
    std::function<void(int)> onSelectionChanged;   // selected speaker index
    std::function<void()>    onSectionsChanged;    // a section expanded/collapsed (re-size viewport)
    /** "+ Add" — request click-to-place on the plot (MainComponent arms the renderer). */
    std::function<void()>    onAddSpeakerRequest;

    // 0=Q21S, 1=GYLT, 2=15W750. Fires when section 2's Speaker Model picker
    // changes, to update the Measured Polar reference view — it no longer
    // gates the engine, since every placed speaker simulates with its own
    // model regardless (there's no separate "Measurement set" selector).
    std::function<void(int)> onMeasurementSourceChanged;
    std::function<void(float)> onMeasurementDistanceChanged;     // metres

    /** Sync section 2 (Speaker Model picker + new-unit tag) and the Frequency
        dropdown to a model: repoints the active catalogue and rebuilds
        freqBox_ from scratch (Q21S / 15W750 are fully isolated — the two
        devices' frequencies can never mix). Frequency is a single global
        scene parameter, so this always leaves a valid selection behind. */
    void setMeasurementSource (int idx);

    // Populate distance choices from loaded measurement set (0.5 / 1.0 / 2.0 m).
    void setAvailableDistances (const std::vector<float>& distancesM, float preferM = 0.5f);
    float getMeasurementDistance() const;

    // Workspace / layout (Phase 5 & 6) -------------------------------------
    std::function<void(bool)> onGridToggled;       // show/hide grid
    std::function<void()>     onImportLayout;      // request layout import
    std::function<void()>     onRemoveLayout;      // remove imported layout
    std::function<void()>     onLayoutSettingsChanged;  // visible/lock/edit/snap/transform

    void setLayoutLayer (LayoutLayer* l) { layout_ = l; }
    void setGridToggleState (bool b) { gridToggle_.setToggleState (b, juce::dontSendNotification); }
    void refreshLayoutControls();          // sync controls to current layer state
    bool layoutEditMode() const;
    bool layoutSnap() const;

    int  getContentHeight() const { return contentHeight_; }

    SimParams getParams() const;
    /** Model whose catalogue the Frequency dropdown currently shows (0=Q21S, 2=15W750). */
    int  getBrowsedModel() const { return currentMeasSource_; }
    const std::vector<Speaker>& getSpeakers() const { return speakers_; }
    int  getSelectedIndex() const { return selected_; }

    void applyProject (const ProjectData& p);   // load scene from a project

    void setSpeakerPosition (int index, float x, float y);
    void selectSpeaker (int index);
    /** Sync plot multi-select into the panel (primary drives the editor values). */
    void setSelectedSpeakers (const std::vector<int>& indices, int primaryIndex);
    /** Append speakers (e.g. paste); returns new indices. Does not fire onSelectionChanged. */
    std::vector<int> appendSpeakers (const std::vector<Speaker>& added);
    /** Place a new Q21S at world metres (click-to-place). */
    void addSpeakerAt (float x, float y);
    /** Remove speakers by index (highest first). */
    void removeSpeakers (const std::vector<int>& indices);
    void resetToDefaults();
    void refreshUnits();    // update position labels/readouts for current unit system

    void paint (juce::Graphics&) override;
    void resized() override;
    void lookAndFeelChanged() override;   // re-apply theme colours

private:
    /** The frequency to use for a given model's speakers (0=Q21S, 2=15W750),
        completely independent of which model the Frequency dropdown is
        currently showing: live value when it IS the browsed model, else that
        model's own remembered value, else its own lowest prescribed
        frequency. Never derived from or blended with the other model. */
    double resolvedFrequencyFor (int model) const;

    void applyColours();
    void rebuildSpeakerBox();
    void refreshEditors();
    void addSpeaker();
    void deleteSpeaker();
    void applyDeviceLayout (int count);   // 1/2/3 devices, same plane, 3 m apart
    void pushEdit();          // commit editor values into selected speaker
    void pushPositionEdit();  // X/Y only — avoids quantizing snapped positions
    void pushSharedEdit();    // gain/delay/polarity/orientation/enabled (multi-select)
    void notifyChanged();
    void willEdit();

    enum class PresetKind { Cardioid, EndFired };
    void applyArrayPreset (PresetKind kind, int count);
    void pushLayoutTransform();   // commit width/rot/opacity sliders to the layer

    // Scene state
    std::vector<Speaker> speakers_;
    int  selected_ = -1;
    std::vector<int> selectedSpeakers_; // multi-select from plot; empty → use selected_
    bool updatingUI_ = false;

    // Per-source frequency memory: each model keeps its own last-selected Hz.
    // -1 = never visited yet (first switch uses nearest-frequency matching).
    // currentMeasSource_ = section 2's Speaker Model picker: drives the
    // Frequency dropdown's catalogue AND the model tag new "+Add" units get.
    int    currentMeasSource_ = 0;
    double savedFreqHz_[3]    = { -1.0, -1.0, -1.0 };

    // Frequency
    SectionHeader    freqHdr_     { "1. FREQUENCY (Hz)" };
    juce::ComboBox   freqBox_;
    juce::TextButton freqPrevBtn_ { "<" };
    juce::TextButton freqNextBtn_ { ">" };
    void stepFrequency (int delta);

    // Speaker selector
    SectionHeader    speakersHdr_ { "2. Q21S Units" };
    juce::Label      speakerModelLabel_;
    juce::ComboBox   speakerModelBox_;   // Q21S / 15W750 model selector
    juce::ComboBox   speakerBox_;
    juce::TextButton addBtn_, deleteBtn_;

    // Device-layout presets (same plane)
    juce::Label      layoutLabel_;
    juce::TextButton layout1Btn_, layout2Btn_, layout3Btn_;

    // Per-speaker editors
    SectionHeader editHdr_ { "3. SELECTED Q21S" };
    juce::Label  xLabel_,  yLabel_,  gainLabel_,  delayLabel_;
    juce::Slider xSlider_, ySlider_, gainSlider_, delaySlider_;
    juce::ToggleButton polarityToggle_, orientationToggle_, enabledToggle_;

    // Global
    SectionHeader globalHdr_ { "4. SIMULATION" };
    juce::Label  resLabel_;
    juce::Slider resSlider_;
    juce::Label  floorLabel_;
    juce::Slider floorSlider_;
    juce::ToggleButton bandsToggle_;
    // Measured directivity is always on (Q21S BEM) — no UI toggle.

    // Measurement distance (kept separate per set) — currently hidden from UI
    juce::Label    measDistLabel_;
    juce::ComboBox measDistBox_;
    std::vector<float> measDistances_;

    // Workspace (grid + imported layout)
    SectionHeader      workspaceHdr_ { "5. WORKSPACE" };
    juce::ToggleButton gridToggle_;
    juce::TextButton   importLayoutBtn_, removeLayoutBtn_;
    juce::ToggleButton layoutVisibleToggle_, layoutLockToggle_, layoutEditToggle_, layoutSnapToggle_;
    juce::Label        layoutWidthLabel_, layoutRotLabel_, layoutOpacityLabel_;
    juce::Slider       layoutWidthSlider_, layoutRotSlider_, layoutOpacitySlider_;

    // Array presets
    SectionHeader      presetHdr_ { "6. ARRAY PRESETS" };
    juce::ComboBox     presetBox_;
    juce::TextButton   applyPresetBtn_;

    LayoutLayer*       layout_ = nullptr;   // not owned
    int                contentHeight_ = 0;
    std::vector<juce::Rectangle<int>> sectionCards_;
    std::vector<int>   sectionDividerYs_;
    bool secFreqOpen_       = true;
    bool secSpeakersOpen_   = true;
    bool secEditOpen_       = true;
    bool secSimOpen_        = true;
    bool secWorkspaceOpen_  = false;
    bool secPresetOpen_     = false;

    void setSectionVisible (const std::initializer_list<juce::Component*>& items, bool vis);

    // Actions
    juce::TextButton runBtn_, resetBtn_, clearAllBtn_;

    static juce::Colour kHdr()    { return Brand::heading(); }
    static juce::Colour kText()   { return Brand::ash(); }
    static juce::Colour kBtnAct() { return Brand::accent(); }
    static juce::Colour kBtnIn()  { return Brand::btnIn(); }
    static juce::Colour kBorder() { return Brand::border(); }

    juce::String activeModelName() const;   // "Q21S" or "15W750"
    void updateModelDependentLabels();      // headers, tooltip, speaker list prefix

    void styleSlider (juce::Slider&, double lo, double hi, double step, double val);
    void styleToggle (juce::ToggleButton&, const juce::String&);
    void updateScaledChrome();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPanel)
};

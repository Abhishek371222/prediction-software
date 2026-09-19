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

    std::function<void(int)> onMeasurementSourceChanged;  // 0 = Open Field, 1 = GYLT
    std::function<void(float)> onMeasurementDistanceChanged;  // metres

    /** @returns the index actually selected, which may differ from the one
        asked for. Selecting an id the box does not contain leaves ComboBox
        showing NOTHING, with no error -- a stale "measurementSource" of 1 in
        the settings file blanked this control and gave no clue why. Fall back
        to the first item instead and let the caller know. */
    int setMeasurementSource (int idx)
    {
        const int wanted = idx + 1;
        const bool exists = measSetBox_.indexOfItemId (wanted) >= 0;
        const int useId = exists ? wanted : measSetBox_.getItemId (0);
        measSetBox_.setSelectedId (useId, juce::dontSendNotification);
        return juce::jmax (0, useId - 1);
    }

    // Populate distance choices from loaded measurement set (0.5 / 1.0 / 2.0 m).
    void setAvailableDistances (const std::vector<float>& distancesM, float preferM = 0.5f);
    float getMeasurementDistance() const;

    /** Sets the world region the next run should cover, in metres. Driven by
        the plot's visible extent (see RadiationPatternComponent::
        onViewRegionChanged) so the simulation follows the view. */
    void setWorldExtent (double w, double h)
    {
        worldW_ = juce::jmax (1.0, w);
        worldH_ = juce::jmax (1.0, h);
        // The X / Y position sliders span the world, so they have to grow with
        // it -- they were fixed at 0..100 m, which silently clamped every unit
        // to the old box no matter how far the view had been zoomed out.
        syncPositionRanges();
    }
    /** Origin of the region the next run should cover, in metres. Moved by
        panning the plot (see RadiationPatternComponent::onViewRegionChanged). */
    void setWorldOrigin (double x, double y)
    {
        worldX0_ = x; worldY0_ = y;
        syncPositionRanges();
    }

    double worldExtentW() const noexcept { return worldW_; }
    double worldExtentH() const noexcept { return worldH_; }

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
    /** Hard cap on Q21S units. Enforced in addSpeakerAt, which every add path
        funnels through (the + Add button, click-to-place, and the terminal's
        ADDSPEAKER), so there is no way round it. */
    static constexpr int kMaxSpeakers = 8;
    bool canAddSpeaker() const noexcept { return (int) speakers_.size() < kMaxSpeakers; }

    /** @returns false when the cap is already reached and nothing was added. */
    bool addSpeakerAt (float x, float y);
    /** Remove speakers by index (highest first). */
    void removeSpeakers (const std::vector<int>& indices);
    void resetToDefaults();
    void refreshUnits();    // update position labels/readouts for current unit system

    void paint (juce::Graphics&) override;
    void resized() override;
    void lookAndFeelChanged() override;   // re-apply theme colours

private:
    void applyColours();
    void rebuildSpeakerBox();
    void refreshEditors();
    void addSpeaker();
    void deleteSpeaker();
    void applyDeviceLayout (int count);   // 1/2/3 devices, same plane, 3 m apart
    void pushEdit();          // commit editor values into selected speaker
    void pushPositionEdit();  // X/Y only — avoids quantizing snapped positions
    void syncPositionRanges();   // X/Y slider spans follow the world extent
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

    // Frequency
    SectionHeader    freqHdr_     { "1. FREQUENCY (Hz)" };
    juce::ComboBox   freqBox_;
    juce::TextButton freqPrevBtn_ { "<" };
    juce::TextButton freqNextBtn_ { ">" };
    void stepFrequency (int delta);

    // Speaker selector
    SectionHeader    speakersHdr_ { "2. Q21S Units" };
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

    // Measurement dataset + distance (kept separate per set)
    // Region the next simulation covers. Replaces a hard-coded 100 x 100 m.
    double worldW_ = 100.0, worldH_ = 100.0;
    double worldX0_ = 0.0, worldY0_ = 0.0;

    juce::Label    measSetLabel_;
    juce::ComboBox measSetBox_;
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

    void styleSlider (juce::Slider&, double lo, double hi, double step, double val);
    void styleToggle (juce::ToggleButton&, const juce::String&);
    void updateScaledChrome();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ControlPanel)
};

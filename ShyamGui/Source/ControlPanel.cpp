#include "ControlPanel.h"
#include "UiChrome.h"
#include "AcousticEngine.h"
#include <iterator>
#include <cmath>

// ---------------------------------------------------------------------------
void ControlPanel::styleSlider (juce::Slider& s, double lo, double hi, double step, double val)
{
    s.setRange (lo, hi, step);
    s.setValue (val, juce::dontSendNotification);
    s.setSliderStyle (juce::Slider::LinearHorizontal);
    s.setTextBoxStyle (juce::Slider::TextBoxRight, false,
                       Brand::UI::sidebarSliderBoxW,
                       Brand::UI::sidebarSliderBoxH);
    s.setComponentID ("ctrlSlider");
    s.setColour (juce::Slider::trackColourId,      juce::Colour (0xff313131));
    s.setColour (juce::Slider::thumbColourId,      juce::Colour (0xffd81f1f));
    s.setColour (juce::Slider::backgroundColourId, juce::Colours::transparentBlack);
    s.setColour (juce::Slider::textBoxTextColourId, Brand::onBtnIn());
    s.setColour (juce::Slider::textBoxBackgroundColourId, Brand::btnIn());
    s.setColour (juce::Slider::textBoxOutlineColourId, Brand::controlBorder());
    addAndMakeVisible (s);
}

void ControlPanel::styleToggle (juce::ToggleButton& t, const juce::String& txt)
{
    t.setButtonText (txt);
    t.setComponentID ("ctrlToggle");
    t.setColour (juce::ToggleButton::textColourId, Brand::text());
    t.setColour (juce::ToggleButton::tickColourId, kBtnAct());
    t.setColour (juce::ToggleButton::tickDisabledColourId, Brand::ash().withAlpha (0.7f));
    addAndMakeVisible (t);
}

// ---------------------------------------------------------------------------
ControlPanel::ControlPanel()
{
    auto configTxt = [&] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (Brand::tech (Brand::Type::sidebarFieldLabel));
        l.setColour (juce::Label::textColourId, Brand::text());
        l.setJustificationType (juce::Justification::centredLeft);
        l.setMinimumHorizontalScale (0.85f);
        addAndMakeVisible (l);
    };
    auto styleBtn = [&] (juce::TextButton& b, const juce::String& txt, bool big = false)
    {
        b.setButtonText (txt);
        b.setComponentID ("ctrlBtn");
        b.setColour (juce::TextButton::buttonColourId,   big ? kBtnAct() : kBtnIn());
        b.setColour (juce::TextButton::buttonOnColourId, kBtnAct());
        b.setColour (juce::TextButton::textColourOffId,  big ? Brand::onAccent() : Brand::onBtnIn());
        b.setColour (juce::TextButton::textColourOnId,   Brand::onAccent());
        addAndMakeVisible (b);
    };

    // Default scene — two units near the centre of the 100 m world.
    speakers_.push_back ({ 45.0f, 50.0f, 0.0f, 0.0f, false, false, true });
    speakers_.push_back ({ 55.0f, 50.0f, 0.0f, 0.0f, false, false, true });

    auto addSection = [&] (SectionHeader& h, bool& open)
    {
        h.setTitleFontSize (UiConfig::FontSize::sidebarSectionTitle);
        h.setChevronScale (UiConfig::Control::sidebarChevronScale);
        h.setExpanded (open);
        h.onToggled = [&open, this] (bool e)
        {
            open = e;
            resized();   // refresh contentHeight_ for the new open/closed state
            // Then let the host re-run its two-pass viewport sizing so the
            // scrollbar range tracks the new content height.
            if (onSectionsChanged) onSectionsChanged();
        };
        addAndMakeVisible (h);
    };

    // --- Frequency ---------------------------------------------------------
    addSection (freqHdr_, secFreqOpen_);
    for (int i = 0; i < (int) kNumSupportedFrequencies; ++i)
        freqBox_.addItem (juce::String (kSupportedFrequencies[i]) + " Hz", i + 1);
    freqBox_.setComponentID ("ctrlCombo");
    {
        int defId = 1;
        for (int i = 0; i < kNumSupportedFrequencies; ++i)
            if (kSupportedFrequencies[i] == 52) { defId = i + 1; break; }
        freqBox_.setSelectedId (defId, juce::dontSendNotification);   // 52 Hz
    }
    freqBox_.setColour (juce::ComboBox::backgroundColourId, kBtnIn());
    freqBox_.setColour (juce::ComboBox::textColourId,       Brand::onBtnIn());
    freqBox_.onChange = [this] { willEdit(); notifyChanged(); };
    addAndMakeVisible (freqBox_);

    // Frequency stepping via < > removed — v1.1 uses a full-width dropdown only.
    freqPrevBtn_.setVisible (false);
    freqNextBtn_.setVisible (false);

    // --- Speaker selector --------------------------------------------------
    addSection (speakersHdr_, secSpeakersOpen_);
    speakerBox_.setComponentID ("ctrlCombo");
    speakerBox_.setColour (juce::ComboBox::backgroundColourId, kBtnIn());
    speakerBox_.setColour (juce::ComboBox::textColourId,       Brand::onBtnIn());
    speakerBox_.onChange = [this]
    {
        if (updatingUI_) return;
        selected_ = speakerBox_.getSelectedId() - 1;
        selectedSpeakers_.clear();
        if (selected_ >= 0)
            selectedSpeakers_.push_back (selected_);
        refreshEditors();
        if (onSelectionChanged) onSelectionChanged (selected_);
    };
    addAndMakeVisible (speakerBox_);
    styleBtn (addBtn_,    "+ Add");
    styleBtn (deleteBtn_, "Delete");
    addBtn_.onClick    = [this] { willEdit(); addSpeaker(); };
    deleteBtn_.onClick = [this] { willEdit(); deleteSpeaker(); };

    // --- Device-layout presets (same plane) -------------------------------
    configTxt (layoutLabel_, "Quick Layout (same plane)");
    layoutLabel_.setColour (juce::Label::textColourId, Brand::text());
    styleBtn (layout1Btn_, "1 Device");
    styleBtn (layout2Btn_, "2 Devices");
    styleBtn (layout3Btn_, "3 Devices");
    layout1Btn_.onClick = [this] { willEdit(); applyDeviceLayout (1); };
    layout2Btn_.onClick = [this] { willEdit(); applyDeviceLayout (2); };
    layout3Btn_.onClick = [this] { willEdit(); applyDeviceLayout (3); };

    // --- Per-speaker editors ----------------------------------------------
    addSection (editHdr_, secEditOpen_);
    configTxt (xLabel_,     "X Position (m)");
    configTxt (yLabel_,     "Y Position (m)");
    configTxt (gainLabel_,  "Gain (db)");
    configTxt (delayLabel_, "Delay (ms)");
    styleSlider (xSlider_,     0.0, 100.0, 0.1, 50.0);
    styleSlider (ySlider_,     0.0, 100.0, 0.1, 50.0);
    styleSlider (gainSlider_, -40.0, 0.0, 1.0,  0.0);
    styleSlider (delaySlider_, 0.0, 50.0, 0.1,  0.0);
    xSlider_.onValueChange     = [this] { pushPositionEdit(); };
    ySlider_.onValueChange     = [this] { pushPositionEdit(); };
    gainSlider_.onValueChange  = [this] { pushSharedEdit(); };
    delaySlider_.onValueChange = [this] { pushSharedEdit(); };
    xSlider_.onDragStart       = [this] { willEdit(); };
    ySlider_.onDragStart       = [this] { willEdit(); };
    gainSlider_.onDragStart    = [this] { willEdit(); };
    delaySlider_.onDragStart   = [this] { willEdit(); };

    styleToggle (polarityToggle_,    "Invert Polarity");
    styleToggle (orientationToggle_, "Reverse Orientation");
    styleToggle (enabledToggle_,     "Enabled");
    polarityToggle_.onClick    = [this] { willEdit(); pushSharedEdit(); };
    orientationToggle_.onClick = [this] { willEdit(); pushSharedEdit(); };
    enabledToggle_.onClick     = [this] { willEdit(); pushSharedEdit(); };

    // --- Global settings ---------------------------------------------------
    addSection (globalHdr_, secSimOpen_);
    configTxt (resLabel_,   "Grid Resolution");
    configTxt (floorLabel_, "db Floor");
    styleSlider (resSlider_,   100.0, 700.0, 20.0, 400.0);
    styleSlider (floorSlider_, -54.0, -12.0, 6.0, -36.0);
    resSlider_.onValueChange   = [this] { notifyChanged(); };
    floorSlider_.onValueChange = [this] { notifyChanged(); };
    resSlider_.onDragStart     = [this] { willEdit(); };
    floorSlider_.onDragStart   = [this] { willEdit(); };
    styleToggle (bandsToggle_, "Contour bands (3 dB)");
    bandsToggle_.setToggleState (false, juce::dontSendNotification); // continuous 7-color by default
    bandsToggle_.onClick = [this] { willEdit(); notifyChanged(); };

    configTxt (measSetLabel_, "Measurement set");
    measSetBox_.addItem ("Ground Plane", 1);
    measSetBox_.setComponentID ("ctrlCombo");
    measSetBox_.setSelectedId (1, juce::dontSendNotification);
    measSetBox_.setEnabled (false); // Ground Plane only
    measSetBox_.setColour (juce::ComboBox::backgroundColourId, kBtnIn());
    measSetBox_.setColour (juce::ComboBox::textColourId,       Brand::onBtnIn());
    measSetBox_.onChange = [this]
    {
        if (updatingUI_) return;
        // Always Ground Plane (Open Field).
        if (onMeasurementSourceChanged)
            onMeasurementSourceChanged (0);
    };
    addAndMakeVisible (measSetBox_);

    configTxt (measDistLabel_, "Distance");
    measDistBox_.setComponentID ("ctrlCombo");
    measDistBox_.setColour (juce::ComboBox::backgroundColourId, kBtnIn());
    measDistBox_.setColour (juce::ComboBox::textColourId,       Brand::onBtnIn());
    measDistBox_.onChange = [this]
    {
        if (updatingUI_) return;
        willEdit();
        if (onMeasurementDistanceChanged)
            onMeasurementDistanceChanged (getMeasurementDistance());
    };
    addAndMakeVisible (measDistBox_);
    // Placeholder until measurements load (Ground Plane defaults to 0.5 m).
    setAvailableDistances ({ 0.5f, 1.0f, 2.0f }, 0.5f);

    // --- Workspace (grid + imported reference layout) ----------------------
    addSection (workspaceHdr_, secWorkspaceOpen_);
    styleToggle (gridToggle_, "Show grid");
    gridToggle_.setToggleState (true, juce::dontSendNotification);
    gridToggle_.onClick = [this] { if (onGridToggled) onGridToggled (gridToggle_.getToggleState()); };

    styleBtn (importLayoutBtn_, "Import Layout");
    styleBtn (removeLayoutBtn_, "Remove");
    importLayoutBtn_.onClick = nullptr;
    removeLayoutBtn_.onClick = nullptr;

    styleToggle (layoutVisibleToggle_, "Show layout");
    styleToggle (layoutLockToggle_,    "Lock");
    styleToggle (layoutEditToggle_,    "Move");
    styleToggle (layoutSnapToggle_,    "Snap to grid");
    layoutVisibleToggle_.setToggleState (true, juce::dontSendNotification);
    auto layoutToggleCb = [this]
    {
        if (layout_ != nullptr)
        {
            layout_->visible = layoutVisibleToggle_.getToggleState();
            layout_->locked  = layoutLockToggle_.getToggleState();
        }
        if (onLayoutSettingsChanged) onLayoutSettingsChanged();
    };
    layoutVisibleToggle_.onClick = layoutToggleCb;
    layoutLockToggle_.onClick    = layoutToggleCb;
    layoutEditToggle_.onClick    = [this] { if (onLayoutSettingsChanged) onLayoutSettingsChanged(); };
    layoutSnapToggle_.onClick    = [this] { if (onLayoutSettingsChanged) onLayoutSettingsChanged(); };

    configTxt (layoutWidthLabel_,   "Width (m)");
    configTxt (layoutRotLabel_,     "Rotation");
    configTxt (layoutOpacityLabel_, "Opacity");
    styleSlider (layoutWidthSlider_,   1.0, 60.0, 0.1, 12.0);
    styleSlider (layoutRotSlider_,  -180.0, 180.0, 1.0, 0.0);
    styleSlider (layoutOpacitySlider_, 0.0, 1.0, 0.01, 0.65);
    layoutWidthSlider_.onValueChange   = [this] { pushLayoutTransform(); };
    layoutRotSlider_.onValueChange     = [this] { pushLayoutTransform(); };
    layoutOpacitySlider_.onValueChange = [this] { pushLayoutTransform(); };

    // --- Array presets -----------------------------------------------------
    addSection (presetHdr_, secPresetOpen_);
    presetBox_.setComponentID ("ctrlCombo");
    presetBox_.setColour (juce::ComboBox::backgroundColourId, kBtnIn());
    presetBox_.setColour (juce::ComboBox::textColourId,       Brand::onBtnIn());
    presetBox_.addItem ("Cardioid - 2 sub", 1);
    presetBox_.addItem ("Cardioid - 3 sub", 2);
    presetBox_.addItem ("Cardioid - 4 sub", 3);
    presetBox_.addItem ("End-fired - 2 sub", 4);
    presetBox_.addItem ("End-fired - 3 sub", 5);
    presetBox_.addItem ("End-fired - 4 sub", 6);
    presetBox_.setSelectedId (1, juce::dontSendNotification);
    addAndMakeVisible (presetBox_);
    styleBtn (applyPresetBtn_, "Place Array", true);
    applyPresetBtn_.onClick = [this]
    {
        const int id = presetBox_.getSelectedId();
        const PresetKind kind = (id <= 3) ? PresetKind::Cardioid : PresetKind::EndFired;
        const int count = (id <= 3) ? (id + 1) : (id - 2);   // 2,3,4
        willEdit();
        applyArrayPreset (kind, count);
    };

    // RESET / CLEAR — centred text links at the bottom (no Run Simulation in v1.1).
    auto styleTextLink = [] (juce::TextButton& b, const juce::String& id)
    {
        b.setComponentID (id);
        b.setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        b.setColour (juce::TextButton::textColourOffId,  Brand::ash());
        b.setColour (juce::TextButton::textColourOnId,   Brand::text());
    };

    resetBtn_.setButtonText ("Set to Default");
    styleTextLink (resetBtn_, "ctrlResetLink");
    resetBtn_.onClick = [this] { willEdit(); resetToDefaults(); };
    addAndMakeVisible (resetBtn_);

    clearAllBtn_.setButtonText ("Clear All");
    styleTextLink (clearAllBtn_, "ctrlClearAllLink");
    clearAllBtn_.setTooltip ("Remove all drawings, lines, rulers, and shapes from the SPL heatmap");
    clearAllBtn_.onClick = [this]
    {
        willEdit();
        if (onClearAll) onClearAll();
    };
    addAndMakeVisible (clearAllBtn_);

    rebuildSpeakerBox();
    refreshEditors();
    refreshUnits();
    refreshLayoutControls();
}

ControlPanel::~ControlPanel() {}

// ---------------------------------------------------------------------------
// Unit system: the X/Y position sliders store metres internally (so the
// acoustic engine is unaffected); only their displayed text + labels convert.
void ControlPanel::refreshUnits()
{
    const juce::String u = Units::lengthUnit();
    xLabel_.setText ("X Position (" + u + ")", juce::dontSendNotification);
    yLabel_.setText ("Y Position (" + u + ")", juce::dontSendNotification);

    std::function<juce::String (double)> toText =
        [] (double metres) { return juce::String (Units::metresToDisplay (metres), 1); };
    std::function<double (const juce::String&)> fromText =
        [] (const juce::String& t) { return Units::displayToMetres (t.getDoubleValue()); };

    for (auto* s : { &xSlider_, &ySlider_ })
    {
        s->textFromValueFunction = toText;
        s->valueFromTextFunction = fromText;
        s->updateText();
    }
}

// ---------------------------------------------------------------------------
void ControlPanel::rebuildSpeakerBox()
{
    updatingUI_ = true;
    speakerBox_.clear (juce::dontSendNotification);
    for (int i = 0; i < (int) speakers_.size(); ++i)
        speakerBox_.addItem ("Q21S-" + juce::String (i + 1), i + 1);
    if (selected_ >= (int) speakers_.size()) selected_ = (int) speakers_.size() - 1;
    if (selected_ < 0 && ! speakers_.empty()) selected_ = 0;
    if (selected_ >= 0) speakerBox_.setSelectedId (selected_ + 1, juce::dontSendNotification);
    updatingUI_ = false;
}

void ControlPanel::refreshEditors()
{
    const bool has = (selected_ >= 0 && selected_ < (int) speakers_.size());
    updatingUI_ = true;
    if (has)
    {
        const auto& s = speakers_[(size_t) selected_];
        xSlider_.setValue     (s.x,       juce::dontSendNotification);
        ySlider_.setValue     (s.y,       juce::dontSendNotification);
        gainSlider_.setValue  (s.gainDB,  juce::dontSendNotification);
        delaySlider_.setValue (s.delayMs, juce::dontSendNotification);
        polarityToggle_.setToggleState    (s.polarityInverted,   juce::dontSendNotification);
        orientationToggle_.setToggleState (s.reverseOrientation, juce::dontSendNotification);
        enabledToggle_.setToggleState     (s.enabled,            juce::dontSendNotification);
    }
    xSlider_.setEnabled (has); ySlider_.setEnabled (has);
    gainSlider_.setEnabled (has); delaySlider_.setEnabled (has);
    polarityToggle_.setEnabled (has); orientationToggle_.setEnabled (has);
    enabledToggle_.setEnabled (has); deleteBtn_.setEnabled (has);
    updatingUI_ = false;
}

void ControlPanel::addSpeaker()
{
    Speaker s;
    s.x = (float) (50.0 + speakers_.size());
    s.y = 50.0f;
    speakers_.push_back (s);
    selected_ = (int) speakers_.size() - 1;
    rebuildSpeakerBox();
    refreshEditors();
    if (onSelectionChanged) onSelectionChanged (selected_);
    notifyChanged();
}

void ControlPanel::deleteSpeaker()
{
    if (selected_ < 0 || selected_ >= (int) speakers_.size()) return;
    speakers_.erase (speakers_.begin() + selected_);
    if (selected_ >= (int) speakers_.size()) selected_ = (int) speakers_.size() - 1;
    rebuildSpeakerBox();
    refreshEditors();
    if (onSelectionChanged) onSelectionChanged (selected_);
    notifyChanged();
}

void ControlPanel::applyDeviceLayout (int count)
{
    count = juce::jlimit (1, 3, count);
    const float spacing = 3.0f;                 // metres, same plane
    const float planeY  = 50.0f;                // centre of the 100 m world
    const float centreX = 50.0f;

    speakers_.clear();
    const float start = centreX - spacing * (float) (count - 1) * 0.5f;
    for (int i = 0; i < count; ++i)
    {
        Speaker s;
        s.x = start + spacing * (float) i;
        s.y = planeY;
        s.gainDB = 0.0f;
        s.delayMs = 0.0f;
        s.polarityInverted   = false;
        s.reverseOrientation = false;
        s.enabled            = true;
        speakers_.push_back (s);
    }
    selected_ = 0;
    rebuildSpeakerBox();
    refreshEditors();
    if (onSelectionChanged) onSelectionChanged (selected_);
    notifyChanged();
}

// ---------------------------------------------------------------------------
// Array presets: lay out a cardioid or end-fired sub array along the firing
// axis (+x). Cardioid matches the MATLAB BEM cardioid recipe:
//   rear physically rotated 180 deg, polarity -1, Gain 0.5, delay 3.5 ms,
//   spacing d = 0.01 m; then coherent pressure sum (superposition) in the engine.
// End-fired uses ~lambda/4 spacing with progressive delay. Editable afterwards.
void ControlPanel::applyArrayPreset (PresetKind kind, int count)
{
    count = juce::jlimit (2, 4, count);
    const double c = 343.0;                            // speed of sound (m/s)
    const int fi = juce::jlimit (0, (int) kNumSupportedFrequencies - 1,
                                 freqBox_.getSelectedId() - 1);
    const double f = kSupportedFrequencies[fi];
    const double lambda = c / juce::jmax (1.0, f);

    // MATLAB cardioid DSP on the rear cabinet.
    constexpr float kCardioidPairD       = 0.01f;   // m, front–rear spacing
    constexpr float kCardioidRearDelayMs = 3.5f;
    constexpr float kCardioidRearGainDb  = -6.0f;   // 20*log10(0.5)

    const float centreX = 50.0f, centreY = 50.0f;

    speakers_.clear();

    if (kind == PresetKind::Cardioid)
    {
        // 2-sub cardioid pair at the array centre; any extra units sit ahead
        // along +x at ~lambda/4 so the rear pair keeps the MATLAB geometry.
        const float extraS = (float) juce::jlimit (0.6, 1.5, lambda * 0.25);
        const float pairSpan = kCardioidPairD + extraS * (float) juce::jmax (0, count - 2);
        const float rearX = centreX - 0.5f * pairSpan;

        for (int i = 0; i < count; ++i)
        {
            Speaker sp;
            sp.y = centreY;
            sp.enabled = true;

            if (i == 0)
            {
                // Rear: rotated 180, inverted, Gain 0.5, 3.5 ms delay.
                sp.x = rearX;
                sp.gainDB = kCardioidRearGainDb;
                sp.reverseOrientation = true;
                sp.polarityInverted = true;
                sp.delayMs = kCardioidRearDelayMs;
            }
            else if (i == 1)
            {
                // Front of the cardioid pair: faces +x, unity gain, no delay.
                sp.x = rearX + kCardioidPairD;
                sp.gainDB = 0.0f;
                sp.reverseOrientation = false;
                sp.polarityInverted = false;
                sp.delayMs = 0.0f;
            }
            else
            {
                // Additional forward units ahead of the pair.
                sp.x = rearX + kCardioidPairD + extraS * (float) (i - 1);
                sp.gainDB = 0.0f;
                sp.reverseOrientation = false;
                sp.polarityInverted = false;
                sp.delayMs = 0.0f;
            }
            speakers_.push_back (sp);
        }
    }
    else // EndFired
    {
        const float s = (float) juce::jlimit (0.6, 1.5, lambda * 0.25);
        const float start = centreX - s * (float) (count - 1) * 0.5f;

        for (int i = 0; i < count; ++i)
        {
            Speaker sp;
            sp.x = start + s * (float) i;
            sp.y = centreY;
            sp.gainDB = 0.0f;
            sp.reverseOrientation = false;
            sp.polarityInverted = false;
            sp.delayMs = (float) ((double) i * (double) s / c * 1000.0);
            sp.enabled = true;
            speakers_.push_back (sp);
        }
    }

    selected_ = 0;
    rebuildSpeakerBox();
    refreshEditors();
    if (onSelectionChanged) onSelectionChanged (selected_);
    notifyChanged();
}

// ---------------------------------------------------------------------------
bool ControlPanel::layoutEditMode() const { return layoutEditToggle_.getToggleState(); }
bool ControlPanel::layoutSnap()     const { return layoutSnapToggle_.getToggleState(); }

void ControlPanel::pushLayoutTransform()
{
    if (updatingUI_) return;
    if (layout_ != nullptr)
    {
        layout_->widthM      = (float) layoutWidthSlider_.getValue();
        layout_->rotationDeg = (float) layoutRotSlider_.getValue();
        layout_->opacity     = (float) layoutOpacitySlider_.getValue();
    }
    if (onLayoutSettingsChanged) onLayoutSettingsChanged();
}

void ControlPanel::refreshLayoutControls()
{
    const bool has = (layout_ != nullptr && layout_->valid());
    updatingUI_ = true;
    if (has)
    {
        layoutVisibleToggle_.setToggleState (layout_->visible, juce::dontSendNotification);
        layoutLockToggle_.setToggleState    (layout_->locked,  juce::dontSendNotification);
        layoutWidthSlider_.setValue   (layout_->widthM,      juce::dontSendNotification);
        layoutRotSlider_.setValue     (layout_->rotationDeg, juce::dontSendNotification);
        layoutOpacitySlider_.setValue (layout_->opacity,     juce::dontSendNotification);
    }
    updatingUI_ = false;

    juce::Component* layoutCtrls[] = {
        &removeLayoutBtn_, &layoutVisibleToggle_, &layoutLockToggle_,
        &layoutEditToggle_, &layoutSnapToggle_, &layoutWidthSlider_,
        &layoutRotSlider_, &layoutOpacitySlider_ };
    for (auto* c : layoutCtrls) c->setEnabled (has);
    if (! has) layoutEditToggle_.setToggleState (false, juce::dontSendNotification);
}

void ControlPanel::pushEdit()
{
    // Legacy entry: apply position + shared params (e.g. programmatic commits).
    pushPositionEdit();
    pushSharedEdit();
}

void ControlPanel::pushPositionEdit()
{
    if (updatingUI_) return;
    if (selected_ < 0 || selected_ >= (int) speakers_.size()) return;

    auto& primary = speakers_[(size_t) selected_];
    primary.x = (float) xSlider_.getValue();
    primary.y = (float) ySlider_.getValue();
    notifyChanged();
}

void ControlPanel::pushSharedEdit()
{
    if (updatingUI_) return;
    if (selected_ < 0 || selected_ >= (int) speakers_.size()) return;

    const float gainDB  = (float) gainSlider_.getValue();
    const float delayMs = (float) delaySlider_.getValue();
    const bool polarity = polarityToggle_.getToggleState();
    const bool reverse  = orientationToggle_.getToggleState();
    const bool enabled  = enabledToggle_.getToggleState();

    // Shared acoustic params apply to every speaker currently selected on the plot.
    // Do not touch X/Y here — slider step (0.1 m) would break object/grid snaps.
    std::vector<int> targets = selectedSpeakers_;
    if (targets.empty())
        targets.push_back (selected_);

    for (int idx : targets)
    {
        if (idx < 0 || idx >= (int) speakers_.size()) continue;
        auto& s = speakers_[(size_t) idx];
        s.gainDB  = gainDB;
        s.delayMs = delayMs;
        s.polarityInverted   = polarity;
        s.reverseOrientation = reverse;
        s.enabled            = enabled;
    }

    notifyChanged();
}

void ControlPanel::willEdit()
{
    if (updatingUI_) return;
    if (onWillEdit) onWillEdit();
}

void ControlPanel::notifyChanged()
{
    if (updatingUI_) return;
    if (onChanged) onChanged();
}

// ---------------------------------------------------------------------------
void ControlPanel::setSpeakerPosition (int index, float x, float y)
{
    if (index < 0 || index >= (int) speakers_.size()) return;
    speakers_[(size_t) index].x = x;
    speakers_[(size_t) index].y = y;
    if (index == selected_)
    {
        updatingUI_ = true;
        xSlider_.setValue (x, juce::dontSendNotification);
        ySlider_.setValue (y, juce::dontSendNotification);
        updatingUI_ = false;
    }
}

void ControlPanel::selectSpeaker (int index)
{
    if (index < -1 || index >= (int) speakers_.size()) return;
    selected_ = index;
    selectedSpeakers_.clear();
    if (index >= 0)
        selectedSpeakers_.push_back (index);
    updatingUI_ = true;
    if (index >= 0) speakerBox_.setSelectedId (index + 1, juce::dontSendNotification);
    updatingUI_ = false;
    refreshEditors();
}

void ControlPanel::setSelectedSpeakers (const std::vector<int>& indices, int primaryIndex)
{
    selectedSpeakers_.clear();
    for (int idx : indices)
        if (idx >= 0 && idx < (int) speakers_.size())
            selectedSpeakers_.push_back (idx);

    if (primaryIndex >= 0 && primaryIndex < (int) speakers_.size())
        selected_ = primaryIndex;
    else if (! selectedSpeakers_.empty())
        selected_ = selectedSpeakers_.back();
    else
        selected_ = speakers_.empty() ? -1 : juce::jlimit (0, (int) speakers_.size() - 1, selected_);

    updatingUI_ = true;
    if (selected_ >= 0)
        speakerBox_.setSelectedId (selected_ + 1, juce::dontSendNotification);
    updatingUI_ = false;
    refreshEditors();
}

std::vector<int> ControlPanel::appendSpeakers (const std::vector<Speaker>& added)
{
    std::vector<int> idxs;
    if (added.empty()) return idxs;

    for (const auto& s : added)
    {
        idxs.push_back ((int) speakers_.size());
        speakers_.push_back (s);
    }

    selectedSpeakers_ = idxs;
    selected_ = idxs.back();
    rebuildSpeakerBox();
    refreshEditors();
    notifyChanged();
    return idxs;
}

void ControlPanel::removeSpeakers (const std::vector<int>& indices)
{
    if (indices.empty()) return;

    std::vector<int> order = indices;
    std::sort (order.begin(), order.end(), std::greater<int>());
    int last = -1;
    for (int idx : order)
    {
        if (idx == last) continue;
        last = idx;
        if (idx < 0 || idx >= (int) speakers_.size()) continue;
        speakers_.erase (speakers_.begin() + idx);
    }

    selectedSpeakers_.clear();
    if (selected_ >= (int) speakers_.size())
        selected_ = (int) speakers_.size() - 1;
    if (selected_ >= 0)
        selectedSpeakers_.push_back (selected_);

    rebuildSpeakerBox();
    refreshEditors();
    if (onSelectionChanged) onSelectionChanged (selected_);
    notifyChanged();
}

void ControlPanel::resetToDefaults()
{
    speakers_.clear();
    speakers_.push_back ({ 45.0f, 50.0f, 0.0f, 0.0f, false, false, true });
    speakers_.push_back ({ 55.0f, 50.0f, 0.0f, 0.0f, false, false, true });
    selected_ = 0;
    updatingUI_ = true;
    {
        int defId = 1;
        for (int i = 0; i < kNumSupportedFrequencies; ++i)
            if (kSupportedFrequencies[i] == 52) { defId = i + 1; break; }
        freqBox_.setSelectedId (defId, juce::dontSendNotification);   // 52 Hz
    }
    measSetBox_.setSelectedId (1, juce::dontSendNotification); // Ground Plane
    setAvailableDistances ({ 0.5f, 1.0f, 2.0f }, 0.5f);
    resSlider_.setValue (400.0, juce::dontSendNotification);
    floorSlider_.setValue (-36.0, juce::dontSendNotification);
    bandsToggle_.setToggleState (false, juce::dontSendNotification);
    updatingUI_ = false;
    rebuildSpeakerBox();
    refreshEditors();
    if (onMeasurementSourceChanged) onMeasurementSourceChanged (0); // Ground Plane
    if (onSelectionChanged) onSelectionChanged (selected_);
    notifyChanged();
}

// ---------------------------------------------------------------------------
void ControlPanel::applyProject (const ProjectData& p)
{
    speakers_ = p.speakers;
    if (speakers_.empty())
        speakers_.push_back ({ 50.0f, 50.0f, 0.0f, 0.0f, false, false, true });
    selected_ = 0;

    updatingUI_ = true;
    int freqId = 4;
    for (int i = 0; i < (int) kNumSupportedFrequencies; ++i)
        if (kSupportedFrequencies[i] == (int) std::lround (p.frequency)) { freqId = i + 1; break; }
    freqBox_.setSelectedId (freqId, juce::dontSendNotification);
    resSlider_.setValue   ((double) p.resolution, juce::dontSendNotification);
    floorSlider_.setValue (p.dBfloor,             juce::dontSendNotification);
    bandsToggle_.setToggleState       (p.bandedSPL,           juce::dontSendNotification);
    updatingUI_ = false;

    rebuildSpeakerBox();
    refreshEditors();
    refreshUnits();
    if (onSelectionChanged) onSelectionChanged (selected_);
    notifyChanged();
}

// ---------------------------------------------------------------------------
SimParams ControlPanel::getParams() const
{
    SimParams p;
    const int fi = juce::jlimit (0, (int) kNumSupportedFrequencies - 1,
                                 freqBox_.getSelectedId() - 1);
    p.frequency  = kSupportedFrequencies[fi];
    p.worldW     = 100.0;
    p.worldH     = 100.0;
    p.resolution = (int) resSlider_.getValue();
    p.dBfloor    = floorSlider_.getValue();
    p.colourmap  = 0;
    p.bandedSPL  = bandsToggle_.getToggleState();
    p.octaveSmoothing = true; // always on (UI control removed)
    p.useMeasuredDirectivity = true; // always on — Q21S BEM polars drive heatmap + Directivity
    p.speakers   = speakers_;
    return p;
}

// ---------------------------------------------------------------------------
void ControlPanel::stepFrequency (int delta)
{
    const int n = (int) kNumSupportedFrequencies;
    int id = juce::jlimit (1, n, freqBox_.getSelectedId() + delta);
    if (id == freqBox_.getSelectedId()) return;
    freqBox_.setSelectedId (id, juce::sendNotificationSync);
}

// ---------------------------------------------------------------------------
void ControlPanel::setAvailableDistances (const std::vector<float>& distancesM, float preferM)
{
    measDistances_ = distancesM;
    const float prev = getMeasurementDistance();

    updatingUI_ = true;
    measDistBox_.clear (juce::dontSendNotification);
    int preferId = 1;
    bool keptPrev = false;
    for (int i = 0; i < (int) distancesM.size(); ++i)
    {
        const float d = distancesM[(size_t) i];
        measDistBox_.addItem (juce::String (d, 1) + " m", i + 1);
        if (std::abs (d - prev) < 1.0e-3f) { preferId = i + 1; keptPrev = true; }
    }
    if (! keptPrev)
        for (int i = 0; i < (int) distancesM.size(); ++i)
            if (std::abs (distancesM[(size_t) i] - preferM) < 1.0e-3f)
                preferId = i + 1;
    if (! distancesM.empty())
        measDistBox_.setSelectedId (preferId, juce::dontSendNotification);
    updatingUI_ = false;
}

float ControlPanel::getMeasurementDistance() const
{
    const int id = measDistBox_.getSelectedId();
    if (id <= 0 || id > (int) measDistances_.size())
        return measDistances_.empty() ? 1.0f : measDistances_.front();
    return measDistances_[(size_t) (id - 1)];
}

// ---------------------------------------------------------------------------
void ControlPanel::paint (juce::Graphics& g)
{
    g.fillAll (Brand::panel());
}

// ---------------------------------------------------------------------------
void ControlPanel::setSectionVisible (const std::initializer_list<juce::Component*>& items, bool vis)
{
    for (auto* c : items) c->setVisible (vis);
}

void ControlPanel::applyColours()
{
    auto txt = { &xLabel_, &yLabel_, &gainLabel_, &delayLabel_,
                 &resLabel_, &floorLabel_, &measSetLabel_, &measDistLabel_,
                 &layoutWidthLabel_, &layoutRotLabel_, &layoutOpacityLabel_ };
    for (auto* l : txt) l->setColour (juce::Label::textColourId, Brand::text());
    layoutLabel_.setColour (juce::Label::textColourId, Brand::text());

    for (auto* b : { &freqBox_, &speakerBox_, &presetBox_, &measSetBox_, &measDistBox_ })
    {
        b->setColour (juce::ComboBox::backgroundColourId, kBtnIn());
        b->setColour (juce::ComboBox::textColourId,       Brand::onBtnIn());
        b->setColour (juce::ComboBox::outlineColourId,    Brand::controlBorder());
        b->setColour (juce::ComboBox::arrowColourId,      Brand::onBtnIn());
    }

    auto styleBtnC = [] (juce::TextButton& b, bool big)
    {
        b.setColour (juce::TextButton::buttonColourId,   big ? kBtnAct() : kBtnIn());
        b.setColour (juce::TextButton::buttonOnColourId, kBtnAct());
        b.setColour (juce::TextButton::textColourOffId,  big ? Brand::onAccent() : Brand::onBtnIn());
        b.setColour (juce::TextButton::textColourOnId,   Brand::onAccent());
    };
    for (auto* b : { &addBtn_, &deleteBtn_, &layout1Btn_, &layout2Btn_, &layout3Btn_,
                     &importLayoutBtn_, &removeLayoutBtn_,
                     &freqPrevBtn_, &freqNextBtn_ })
        styleBtnC (*b, false);
    styleBtnC (resetBtn_, false);
    resetBtn_.setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
    resetBtn_.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    resetBtn_.setColour (juce::TextButton::textColourOffId,  Brand::ash());
    styleBtnC (clearAllBtn_, false);
    clearAllBtn_.setColour (juce::TextButton::buttonColourId,   juce::Colours::transparentBlack);
    clearAllBtn_.setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
    clearAllBtn_.setColour (juce::TextButton::textColourOffId,  Brand::ash());
    styleBtnC (applyPresetBtn_, true);

    for (auto* s : { &xSlider_, &ySlider_, &gainSlider_, &delaySlider_,
                     &resSlider_, &floorSlider_,
                     &layoutWidthSlider_, &layoutRotSlider_, &layoutOpacitySlider_ })
    {
        s->setColour (juce::Slider::trackColourId,           juce::Colour (0xff313131));
        s->setColour (juce::Slider::thumbColourId,           juce::Colour (0xffd81f1f));
        s->setColour (juce::Slider::backgroundColourId,      juce::Colours::transparentBlack);
        s->setColour (juce::Slider::textBoxTextColourId,       Brand::onBtnIn());
        s->setColour (juce::Slider::textBoxBackgroundColourId, Brand::btnIn());
        s->setColour (juce::Slider::textBoxOutlineColourId,    Brand::controlBorder());
        // Force rebuilt text boxes to pick up dark-ink-on-light-fill editing colours.
        s->lookAndFeelChanged();
    }

    for (auto* t : { &polarityToggle_, &orientationToggle_, &enabledToggle_,
                     &bandsToggle_,
                     &gridToggle_, &layoutVisibleToggle_, &layoutLockToggle_,
                     &layoutEditToggle_, &layoutSnapToggle_ })
    {
        t->setColour (juce::ToggleButton::textColourId,         Brand::text());
        t->setColour (juce::ToggleButton::tickColourId,         kBtnAct());
        t->setColour (juce::ToggleButton::tickDisabledColourId, Brand::accent().withAlpha (0.45f));
    }
}

void ControlPanel::lookAndFeelChanged()
{
    applyColours();
    repaint();
}

// ---------------------------------------------------------------------------
void ControlPanel::updateScaledChrome()
{
    const float secTitle = Brand::UI::scaledFont (UiConfig::FontSize::sidebarSectionTitle);
    const float labelSz  = Brand::UI::scaledFont (Brand::Type::sidebarFieldLabel);
    const float chevron  = UiConfig::Control::sidebarChevronScale * Brand::UI::scale;

    for (auto* h : { &freqHdr_, &speakersHdr_, &editHdr_, &globalHdr_, &workspaceHdr_, &presetHdr_ })
    {
        h->setTitleFontSize (secTitle);
        h->setChevronScale (chevron);
    }

    for (auto* l : { &xLabel_, &yLabel_, &gainLabel_, &delayLabel_,
                     &resLabel_, &floorLabel_, &measSetLabel_, &measDistLabel_,
                     &layoutWidthLabel_, &layoutRotLabel_, &layoutOpacityLabel_ })
        l->setFont (Brand::tech (labelSz));

    layoutLabel_.setFont (Brand::tech (labelSz));

    auto fixSlider = [] (juce::Slider& s)
    {
        s.setTextBoxStyle (juce::Slider::TextBoxRight, false,
                           Brand::UI::sidebarSliderBoxW,
                           Brand::UI::sidebarSliderBoxH);
    };
    for (auto* s : { &xSlider_, &ySlider_, &gainSlider_, &delaySlider_,
                     &resSlider_, &floorSlider_,
                     &layoutWidthSlider_, &layoutRotSlider_, &layoutOpacitySlider_ })
        fixSlider (*s);
}

// ---------------------------------------------------------------------------
void ControlPanel::resized()
{
    updateScaledChrome();

    const int pad    = Brand::UI::panelPad;
    const int W      = getWidth() - 2 * pad;
    int       y      = pad;
    const int rowH   = Brand::UI::rowH;
    const int gap    = Brand::UI::rowGap;
    const int secGap = Brand::UI::sectionGap;
    const int labelW = Brand::UI::labelColW;
    const int btnGap = Brand::UI::btnGap;

    auto section = [&] (SectionHeader& h, bool& open)
    {
        const int hdrH = Brand::UI::sidebarSectionHeaderH;
        h.setBounds (pad, y, W, hdrH);
        h.setExpanded (open);
        y += hdrH + Brand::UI::sidebarSectionHeaderGap;
    };
    auto ifOpen = [&] (bool open, auto&& fn)
    {
        if (open) fn();
    };
    auto fullRow = [&] (juce::Component& c)
    {
        c.setBounds (pad, y, W, rowH);
        y += rowH + gap;
    };
    // v1.1: label left | control right on the same row.
    auto editRow = [&] (juce::Component& lbl, juce::Component& ctl)
    {
        const int lblGap = UiConfig::Scale::px (8);
        lbl.setBounds (pad, y, labelW, rowH);
        ctl.setBounds (pad + labelW + lblGap, y, W - labelW - lblGap, rowH);
        y += rowH + gap;
    };
    auto equalPair = [&] (juce::Component& a, juce::Component& b)
    {
        const int bw = (W - btnGap) / 2;
        a.setBounds (pad, y, bw, rowH);
        b.setBounds (pad + bw + btnGap, y, bw, rowH);
        y += rowH + gap;
    };
    auto checkboxGrid = [&] (juce::ToggleButton& a, juce::ToggleButton& b,
                             juce::ToggleButton& c, juce::ToggleButton& d)
    {
        const int cw = (W - btnGap) / 2;
        a.setBounds (pad,           y, cw, rowH);
        b.setBounds (pad + cw + btnGap, y, cw, rowH);
        y += rowH + gap;
        c.setBounds (pad,           y, cw, rowH);
        d.setBounds (pad + cw + btnGap, y, cw, rowH);
        y += rowH + gap;
    };
    auto equalTriple = [&] (juce::Component& a, juce::Component& b, juce::Component& c)
    {
        const int bw = (W - 2 * btnGap) / 3;
        a.setBounds (pad, y, bw, rowH);
        b.setBounds (pad + bw + btnGap, y, bw, rowH);
        c.setBounds (pad + 2 * (bw + btnGap), y, bw, rowH);
        y += rowH + gap;
    };
    auto speakerUnitRow = [&]
    {
        const int actW = Brand::UI::sidebarActionButtonW;
        speakerBox_.setBounds (pad, y, W - 2 * actW - 2 * btnGap, rowH);
        addBtn_.setBounds     (pad + W - 2 * actW - btnGap, y, actW, rowH);
        deleteBtn_.setBounds  (pad + W - actW, y, actW, rowH);
        y += rowH + gap;
    };
    auto sectionBreak = [&] () { y += secGap; };

    // 1. Frequency (Hz)
    section (freqHdr_, secFreqOpen_);
    ifOpen (secFreqOpen_, [&] { fullRow (freqBox_); });
    setSectionVisible ({ &freqBox_ }, secFreqOpen_);
    sectionBreak();

    // 2. Q21S Units
    section (speakersHdr_, secSpeakersOpen_);
    ifOpen (secSpeakersOpen_, [&]
    {
        speakerUnitRow();
        const int helperH = Brand::UI::sidebarHelperTextH;
        layoutLabel_.setBounds (pad, y, W, helperH);
        y += helperH + UiConfig::Scale::px (2);
        equalTriple (layout1Btn_, layout2Btn_, layout3Btn_);
    });
    setSectionVisible ({ &speakerBox_, &addBtn_, &deleteBtn_, &layoutLabel_,
                         &layout1Btn_, &layout2Btn_, &layout3Btn_ }, secSpeakersOpen_);
    sectionBreak();

    // 3. Selected Q21S
    section (editHdr_, secEditOpen_);
    ifOpen (secEditOpen_, [&]
    {
        editRow (xLabel_,     xSlider_);
        editRow (yLabel_,     ySlider_);
        editRow (gainLabel_,  gainSlider_);
        editRow (delayLabel_, delaySlider_);
        // Compact stacked checkboxes — tall enough for 14.5 label
        const int chkH = UiConfig::Scale::px (26);
        const int chkGap = UiConfig::Scale::px (4);
        polarityToggle_.setBounds (pad, y, W, chkH); y += chkH + chkGap;
        orientationToggle_.setBounds (pad, y, W, chkH); y += chkH + chkGap;
        enabledToggle_.setBounds (pad, y, W, chkH); y += chkH + gap;
    });
    setSectionVisible ({ &xLabel_, &yLabel_, &gainLabel_, &delayLabel_,
                         &xSlider_, &ySlider_, &gainSlider_, &delaySlider_,
                         &polarityToggle_, &orientationToggle_, &enabledToggle_ },
                       secEditOpen_);
    sectionBreak();

    // 4. Simulation
    section (globalHdr_, secSimOpen_);
    ifOpen (secSimOpen_, [&]
    {
        editRow (resLabel_,   resSlider_);
        editRow (floorLabel_, floorSlider_);
        fullRow (bandsToggle_);
        editRow (measSetLabel_, measSetBox_);
    });
    setSectionVisible ({ &resLabel_, &floorLabel_, &resSlider_, &floorSlider_,
                         &bandsToggle_,
                         &measSetLabel_, &measSetBox_ },
                       secSimOpen_);
    measDistLabel_.setVisible (false);
    measDistBox_.setVisible (false);
    sectionBreak();

    // 5. Workspace — grid only (Import Layout removed)
    section (workspaceHdr_, secWorkspaceOpen_);
    ifOpen (secWorkspaceOpen_, [&] { fullRow (gridToggle_); });
    setSectionVisible ({ &gridToggle_ }, secWorkspaceOpen_);
    importLayoutBtn_.setVisible (false);
    removeLayoutBtn_.setVisible (false);
    layoutVisibleToggle_.setVisible (false);
    layoutLockToggle_.setVisible (false);
    layoutEditToggle_.setVisible (false);
    layoutSnapToggle_.setVisible (false);
    layoutWidthLabel_.setVisible (false);
    layoutRotLabel_.setVisible (false);
    layoutOpacityLabel_.setVisible (false);
    layoutWidthSlider_.setVisible (false);
    layoutRotSlider_.setVisible (false);
    layoutOpacitySlider_.setVisible (false);
    sectionBreak();

    // 6. Array Presets — removed from UI
    presetHdr_.setVisible (false);
    presetBox_.setVisible (false);
    applyPresetBtn_.setVisible (false);

    // RESET / CLEAR ALL — centred text links at the bottom.
    const int resetH       = Brand::UI::sidebarResetRowH;
    const int linkGap      = juce::jmax (2, gap / 2);
    const int actionsBlock = resetH * 2 + linkGap + pad;
    const int panelBottom  = getHeight() - pad;
    int actionsTop = y;
    if (y + actionsBlock <= panelBottom)
        actionsTop = panelBottom - actionsBlock;

    resetBtn_.setBounds (pad, actionsTop, W, resetH);
    clearAllBtn_.setBounds (pad, actionsTop + resetH + linkGap, W, resetH);
    runBtn_.setVisible (false);

    contentHeight_ = juce::jmax (getHeight(), actionsTop + actionsBlock);
}

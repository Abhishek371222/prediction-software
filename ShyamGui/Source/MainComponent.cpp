#include "MainComponent.h"
#include "PreferencesComponent.h"
#include "ReportExport.h"
#include "ReportBuilder.h"
#include "AcousticAnalysis.h"
#include "GraphRender.h"
#include "DxfImport.h"
#include <algorithm>

static void styleActionBtn (juce::TextButton& b, const juce::String& txt,
                            juce::Colour bg = Brand::btnIn(), bool active = false)
{
    b.setButtonText (txt);
    b.setComponentID ("bottomBtn");
    const auto fill = active ? Brand::accent() : bg;
    b.setColour (juce::TextButton::buttonColourId,   fill);
    b.setColour (juce::TextButton::buttonOnColourId, Brand::accent());
    b.setColour (juce::TextButton::textColourOffId,
                 active ? Brand::onAccent() : Brand::onBtnIn());
    b.setColour (juce::TextButton::textColourOnId,   Brand::onAccent());
}

// ---------------------------------------------------------------------------
MainComponent::MainComponent (ProjectData project)
    : juce::Thread ("AcousticWorker"), project_ (std::move (project))
{
    logo_ = Brand::createLogo (Brand::text());

    titleLabel_.setText ("Atomik Simulation Engine",
                         juce::dontSendNotification);
    titleLabel_.setMinimumHorizontalScale (1.0f);
    titleLabel_.setBorderSize ({});
    titleLabel_.setFont (Brand::techSemi (Brand::Type::appTitle));
    titleLabel_.setColour (juce::Label::textColourId, Brand::text());
    titleLabel_.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (titleLabel_);

    versionLabel_.setText ("v1.3.7", juce::dontSendNotification);
    versionLabel_.setMinimumHorizontalScale (1.0f);
    versionLabel_.setBorderSize ({});
    versionLabel_.setFont (Brand::techSemi (UiConfig::FontSize::appVersion));
    versionLabel_.setColour (juce::Label::textColourId, Brand::text().darker (0.10f));
    versionLabel_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (versionLabel_);

    paramBar_.setVisible (false);   // stats now live in the Help (?) popup

    controlViewport_.setViewedComponent (&controlPanel_, false);
    controlViewport_.setScrollBarsShown (true, false);
    controlViewport_.setScrollBarThickness (8);
    addAndMakeVisible (controlViewport_);

    btnSidebarToggle_.setTooltip ("Hide controls");
    btnSidebarToggle_.onClick = [this] { toggleSidebar(); };
    btnSidebarToggle_.setColour (juce::DrawableButton::backgroundColourId,   juce::Colours::transparentBlack);
    btnSidebarToggle_.setColour (juce::DrawableButton::backgroundOnColourId, Brand::btnIn().withAlpha (0.35f));
    addAndMakeVisible (btnSidebarToggle_);
    syncSidebarToggleChrome();

    addAndMakeVisible (plotHeader_);
    addAndMakeVisible (patternComp_);

    auto configHdr = [&] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (Brand::techSemi (Brand::Type::bottomSectionTitle));
        l.setColour (juce::Label::textColourId, Brand::heading());
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    };
    configHdr (exportHeader_,   "SAVE / EXPORT");
    configHdr (viewHeader_,     "VIEW MODE");
    configHdr (terminalHeader_, "TERMINAL");

    styleActionBtn (btnExportPNG_, "SAVE IMAGE (PNG)", Brand::exportPill());
    styleActionBtn (btnExportCSV_, "EXPORT SPL (CSV)", Brand::exportPill());
    addAndMakeVisible (btnExportPNG_);
    addAndMakeVisible (btnExportCSV_);
    btnExportPNG_.onClick = [this] { exportPNG(); };
    btnExportCSV_.onClick = [this] { exportCSV(); };

    styleActionBtn (btnViewSPL_,         "SPL HEAT MAP", Brand::idleViewPill());
    styleActionBtn (btnViewDirectivity_, "DIRECTIVITY", Brand::idleViewPill());
    styleActionBtn (btnViewMeasured_,    "MEASURED POLAR", Brand::idleViewPill());
    for (auto* b : { &btnViewSPL_, &btnViewDirectivity_, &btnViewMeasured_ })
        addAndMakeVisible (*b);
    btnViewSPL_.onClick         = [this] { setViewMode (ViewMode::SPL); };
    btnViewDirectivity_.onClick = [this] { setViewMode (ViewMode::Directivity); };
    btnViewMeasured_.onClick    = [this] { setViewMode (ViewMode::MeasuredPolar); };

    addAndMakeVisible (commandTerminal_);

    btnStats_.setComponentID ("headerStats");
    btnStats_.setButtonText ("Statistics");
    btnStats_.setTooltip ("Scene Summary & Selected Speaker");
    btnStats_.setColour (juce::TextButton::buttonColourId,   Brand::statsBtn());
    btnStats_.setColour (juce::TextButton::textColourOffId,  Brand::statsText());
    btnStats_.onClick = [this] { showStatsPopup(); };
    addAndMakeVisible (btnStats_);

    btnProject_.setComponentID ("headerNewProject");
    btnProject_.setButtonText (juce::String ("Project") + juce::String::fromUTF8 (" \xe2\x96\xbe"));
    btnProject_.setTooltip ("Open or create a project");
    btnProject_.setColour (juce::TextButton::buttonColourId,   Brand::statsBtn());
    btnProject_.setColour (juce::TextButton::textColourOffId,  Brand::statsText());
    btnProject_.onClick = [this] { showProjectMenu(); };
    addAndMakeVisible (btnProject_);

    btnHelp_.setTooltip ("Help");
    btnPrefsIcon_.setTooltip ("Preferences");
    btnMore_.setTooltip ("More options");
    addAndMakeVisible (btnHelp_);
    addAndMakeVisible (btnPrefsIcon_);
    addAndMakeVisible (btnMore_);
    btnPrefsIcon_.onClick = [this] { openPreferences(); };
    btnMore_.onClick      = [this] { showOverflowMenu(); };
    btnHelp_.onClick      = [this]
    {
        const juce::String stats = statChips_.isEmpty()
            ? juce::String ("No simulation stats yet. Run a simulation to see live values.")
            : statChips_.joinIntoString ("\n");
        juce::AlertWindow::showMessageBoxAsync (juce::AlertWindow::InfoIcon,
            "Simulation Stats", stats);
    };
    refreshHeaderIcons();

    plotHeader_.fitBtn_.onClick      = [this] { patternComp_.resetView(); };
    plotHeader_.rangeBtn_.onClick    = [this]
    {
        patternComp_.setShowDistanceRings (plotHeader_.rangeBtn_.getToggleState());
    };
    plotHeader_.btnZoomIn_.onClick   = [this] { patternComp_.zoomIn(); };
    plotHeader_.btnZoomOut_.onClick  = [this] { patternComp_.zoomOut(); };
    plotHeader_.btnSelect_.onClick   = [this]
    {
        if (plotHeader_.btnSelect_.getToggleState())
            applyPlotTool (RadiationPatternComponent::Tool::Select);
    };
    plotHeader_.btnPan_.onClick      = [this]
    {
        if (plotHeader_.btnPan_.getToggleState())
            applyPlotTool (RadiationPatternComponent::Tool::Pan);
    };
    plotHeader_.btnPencil_.onClick   = [this]
    {
        // Radio-group untoggles also fire onClick — only act when Pencil turns ON.
        if (! plotHeader_.btnPencil_.getToggleState())
            return;
        applyPlotTool (RadiationPatternComponent::Tool::Pencil, true);
    };
    plotHeader_.btnEraser_.onClick   = [this]
    {
        if (plotHeader_.btnEraser_.getToggleState())
            applyPlotTool (RadiationPatternComponent::Tool::Eraser);
    };
    plotHeader_.btnRuler_.onClick    = [this]
    {
        if (plotHeader_.btnRuler_.getToggleState())
            applyPlotTool (RadiationPatternComponent::Tool::Ruler);
    };
    plotHeader_.btnShape_.onClick = [this]
    {
        // Radio-group untoggles also fire onClick (e.g. clicking Select). Only
        // open the construction menu when Shape is being turned ON / re-clicked.
        if (! plotHeader_.btnShape_.getToggleState())
            return;

        plotHeader_.showShapeMenu ([this] (int shapeId, int constructionId)
        {
            using DS = RadiationPatternComponent::DrawShape;
            using C  = RadiationPatternComponent::Construction;
            static const DS shapes[] = {
                DS::Line, DS::Polyline, DS::Circle, DS::Arc, DS::Rectangle, DS::Square, DS::TextBox
            };
            if (shapeId < 0 || shapeId >= (int) (sizeof (shapes) / sizeof (shapes[0])))
                return;
            if (constructionId < 0 || constructionId > (int) C::TextBoxTwoCorners)
                return;
            patternComp_.setDrawShape (shapes[shapeId], (C) constructionId);
            patternComp_.setAddMicArmed (false);
            plotHeader_.setActiveTool (PlotHeaderBar::ActiveTool::Shape);
            plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
            patternComp_.grabKeyboardFocus();
        });
    };
    plotHeader_.btnMic_.onClick = [this]
    {
        const bool hasMics = ! patternComp_.getMics().empty();
        plotHeader_.showMicMenu ([this] (int itemId)
        {
            if (itemId == 1)
            {
                patternComp_.setAddMicArmed (true);
                plotHeader_.setMicArmed (true);
                plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
                patternComp_.grabKeyboardFocus();
            }
            else if (itemId == 2)
            {
                showMicPlaceOnRingDialog();
            }
            else if (itemId == 3)
            {
                patternComp_.setShowMicDegrees (! patternComp_.showMicDegrees());
            }
            else if (itemId == 4)
            {
                showFrequencyResponseWindow();
            }
        }, hasMics, patternComp_.showMicDegrees());
    };
    patternComp_.onAddMicArmedChanged = [this]
    {
        plotHeader_.setMicArmed (patternComp_.isAddMicArmed());
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
    };
    patternComp_.onMicsChanged = [this]
    {
        refreshFrequencyResponse();
    };
    plotHeader_.btnOrtho_.onClick = [this]
    {
        const bool on = plotHeader_.btnOrtho_.getToggleState();
        plotHeader_.setOrthoExtrasVisible (on);
        patternComp_.setOrtho (on);
        if (on)
        {
            patternComp_.setOrthoAlign (plotHeader_.isOrthoHorizontal()
                ? RadiationPatternComponent::OrthoAlign::Horizontal
                : RadiationPatternComponent::OrthoAlign::Vertical);
            plotHeader_.setOrthoSpacingM ((double) patternComp_.getOrthoSpacingM());
        }
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
    };
    plotHeader_.onOrthoOptionsChanged = [this]
    {
        if (! plotHeader_.btnOrtho_.getToggleState())
            return;
        patternComp_.setOrthoAlign (plotHeader_.isOrthoHorizontal()
            ? RadiationPatternComponent::OrthoAlign::Horizontal
            : RadiationPatternComponent::OrthoAlign::Vertical);
        patternComp_.setOrthoSpacingM ((float) plotHeader_.getOrthoSpacingM());
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
    };
    plotHeader_.btnSnap_.onClick = [this]
    {
        patternComp_.setDrawGridSnap (plotHeader_.btnSnap_.getToggleState());
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
    };
    plotHeader_.btnSplProbe_.onClick = [this]
    {
        patternComp_.setShowSplProbe (plotHeader_.btnSplProbe_.getToggleState());
    };
    plotHeader_.colourSwatch_.onClick = [this] { showDrawColourPicker(); };
    plotHeader_.setDrawColour (patternComp_.getDrawColour());
    plotHeader_.setFillAlpha01 (patternComp_.getDrawFillAlpha());
    plotHeader_.fillAlpha_.onValueChange = [this]
    {
        patternComp_.setDrawFillAlpha (plotHeader_.getFillAlpha01());
        plotHeader_.repaint();
    };
    patternComp_.onAnnotSelectionChanged = [this]
    {
        // Swatch + opacity follow the selected shape (or the draw brush if none).
        plotHeader_.setFillAlpha01 (patternComp_.getActiveFillAlpha());
        plotHeader_.setDrawColour (patternComp_.getActiveDrawColour());
        plotHeader_.repaint();
    };
    patternComp_.onToolChanged = [this] (RadiationPatternComponent::Tool t)
    {
        using T = RadiationPatternComponent::Tool;
        using A = PlotHeaderBar::ActiveTool;
        plotHeader_.setActiveTool (t == T::Select ? A::Select
                                  : t == T::Pan    ? A::Pan
                                  : t == T::Pencil ? A::Pencil
                                  : t == T::Eraser ? A::Eraser
                                  : t == T::Ruler  ? A::Ruler
                                                   : A::Shape);
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
    };
    patternComp_.onDrawPromptChanged = [this]
    {
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
    };

    addAndMakeVisible (statusStrip_);
    statusStrip_.setStatus ("Ready", true);

    // Wire panels -----------------------------------------------------------
    controlPanel_.onWillEdit = [this] { willEdit(); };
    controlPanel_.onClearAll = [this]
    {
        patternComp_.clearAnnotations();
        patternComp_.clearMics();
        commitEdit();
        refreshFrequencyResponse();
    };
    controlPanel_.onChanged     = [this]
    {
        commitEdit();
        // Always push the live speaker list (delete / enable / layout) so markers
        // and active counts update immediately, then recompute field views.
        syncRenderer();
        const auto live = controlPanel_.getParams();
        const int hz = (int) (live.frequency + 0.5);
        patternComp_.setMeasuredFrequency (hz);
        patternComp_.setMeasuredData (measured_);

        if (currentView_ == ViewMode::MeasuredPolar
            || currentView_ == ViewMode::Directivity)
        {
            // Unit directivity is from readings; still recompute so the array
            // overlay (2+ subs) tracks delete / enable. Measured Polar also
            // recomputes so a native BEM field (e.g. 52 Hz) paints as heatmap.
            updateSettingsBar();
            patternComp_.repaint();
            scheduleRecompute();
        }
        else
        {
            // Display-only params (db Floor / contour bands): recolour immediately
            // and skip a full physics recompute — the relative field is unchanged.
            const bool displayOnly =
                std::abs (live.dBfloor - lastParams_.dBfloor) > 1.0e-6
                || live.bandedSPL != lastParams_.bandedSPL
                || live.colourmap != lastParams_.colourmap;

            const bool physicsChanged =
                std::abs (live.frequency - lastParams_.frequency) > 1.0e-6
                || live.resolution != lastParams_.resolution
                || live.octaveSmoothing != lastParams_.octaveSmoothing
                || live.useMeasuredDirectivity != lastParams_.useMeasuredDirectivity
                || live.speakers.size() != lastParams_.speakers.size();

            // Speakers compared lightly — full recompute still scheduled when unsure.
            bool speakersSame = ! physicsChanged
                && live.speakers.size() == lastParams_.speakers.size();
            if (speakersSame)
            {
                for (size_t i = 0; i < live.speakers.size(); ++i)
                {
                    const auto& a = live.speakers[i];
                    const auto& b = lastParams_.speakers[i];
                    if (a.enabled != b.enabled
                        || a.polarityInverted != b.polarityInverted
                        || a.reverseOrientation != b.reverseOrientation
                        || std::abs (a.x - b.x) > 1.0e-4f
                        || std::abs (a.y - b.y) > 1.0e-4f
                        || std::abs (a.gainDB - b.gainDB) > 1.0e-4f
                        || std::abs (a.delayMs - b.delayMs) > 1.0e-4f)
                    {
                        speakersSame = false;
                        break;
                    }
                }
            }

            {
                juce::ScopedLock sl (resultLock_);
                if (hasResult_)
                {
                    lastParams_.dBfloor   = live.dBfloor;
                    lastParams_.bandedSPL = live.bandedSPL;
                    lastParams_.colourmap = live.colourmap;
                    patternComp_.updateData (lastResult_, lastParams_);
                }
            }

            if (! hasResult_ || physicsChanged || ! speakersSame)
                scheduleRecompute();
            else if (! displayOnly)
                scheduleRecompute(); // e.g. other sim flags without speaker/freq delta
            else
                updateSettingsBar(); // floor / bands only — image already recoloured
        }
    };
    controlPanel_.onRunClicked  = [this] { runSimulation(); };
    controlPanel_.onSectionsChanged = [this] { resized(); };   // re-fit sidebar viewport
    controlPanel_.onSelectionChanged = [this] (int idx)
    {
        patternComp_.setSpeakers (controlPanel_.getSpeakers(), idx);
        patternComp_.selectOnlySpeaker (idx);
        patternComp_.repaint();
    };

    patternComp_.onSpeakerSelected = [this] (int idx)
    {
        controlPanel_.setSelectedSpeakers (patternComp_.getSelectedSpeakers(), idx);
        if (plotHeader_.btnOrtho_.getToggleState())
        {
            patternComp_.syncOrthoSpacingFromSelection();
            plotHeader_.setOrthoSpacingM ((double) patternComp_.getOrthoSpacingM());
            plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
        }
    };
    patternComp_.onPasteSpeakers = [this] (std::vector<Speaker> added)
    {
        return controlPanel_.appendSpeakers (added);
    };
    patternComp_.onDeleteSpeakers = [this] (std::vector<int> idxs)
    {
        controlPanel_.removeSpeakers (idxs);
    };
    patternComp_.onSpeakerMoved = [this] (int idx, float x, float y)
    {
        controlPanel_.setSpeakerPosition (idx, x, y);
        scheduleRecompute();
    };
    patternComp_.onWillEdit = [this] { willEdit(); };
    patternComp_.onEditCommitted = [this] { commitEdit(); };
    patternComp_.onKeyPressed = [this] (const juce::KeyPress& k) { return handleEditShortcut (k); };
    patternComp_.onLayoutMoved = [this] { patternComp_.repaint(); };

    // Workspace / layout wiring --------------------------------------------
    patternComp_.setLayoutLayer (&layout_);
    controlPanel_.setLayoutLayer (&layout_);
    controlPanel_.onGridToggled = [this] (bool on)
    {
        AppSettings::get().setShowGrid (on);   // persists + broadcasts -> applyGridPref
    };
    controlPanel_.onImportLayout        = [this] { importLayout(); };
    controlPanel_.onRemoveLayout        = [this] { removeLayout(); };
    controlPanel_.onLayoutSettingsChanged = [this] { applyLayoutSettings(); };
    controlPanel_.refreshLayoutControls();

    // Measured polar data: initial load + ~1 s live refresh ----------------
    measSource_ = AppSettings::get().measurementSource();
    measDir_    = MeasurementData::folderForSource (measSource_);
    controlPanel_.setMeasurementSource (measSource_);
    controlPanel_.onMeasurementSourceChanged = [this] (int s) { setMeasurementSource (s); };
    controlPanel_.onMeasurementDistanceChanged = [this] (float d) { setMeasurementDistance (d); };
    loadMeasurements();
    measPoll_.fn = [this] { pollMeasurements(); };
    measPoll_.startTimer (1000);

    highlightViewBtn (currentView_);
    updatePlotChrome();

    // Load the project's scene (may be empty — clean slate for new projects).
    controlPanel_.applyProject (project_);

    AppSettings::get().addChangeListener (this);
    applyGridPref();

    setSize (1340, 820);   // after all child components exist (setSize calls resized)
    setWantsKeyboardFocus (true);
    addKeyListener (this); // hear keys while a child (toolbar / plot) has focus
    grabKeyboardFocus();
    editBaseline_ = takeEditSnapshot();

    // Only compute when the scene has units; empty projects stay a blank plot.
    if (! controlPanel_.getSpeakers().empty())
        runSimulation();
    else
        syncRenderer();
}

MainComponent::~MainComponent()
{
    removeKeyListener (this);
    if (keyHost_ != nullptr)
        keyHost_->removeKeyListener (this);
    AppSettings::get().removeChangeListener (this);
    measPoll_.stopTimer();
    stopTimer();
    stopThread (3000);
    frWindow_.reset();
}

// ---------------------------------------------------------------------------
void MainComponent::openPreferences()
{
    if (prefsPanel_ == nullptr)
    {
        prefsPanel_ = std::make_unique<PreferencesComponent>();
        prefsPanel_->onClose = [this] { if (prefsPanel_ != nullptr) prefsPanel_->setVisible (false); };
        addChildComponent (*prefsPanel_);
    }

    prefsPanel_->setVisible (true);
    prefsPanel_->toFront (true);
    layoutPrefsPanel();
}

void MainComponent::layoutPrefsPanel()
{
    if (prefsPanel_ == nullptr || ! prefsPanel_->isVisible()) return;
    const int w = UiConfig::Scale::px (UiConfig::Layout::prefsPanelWidth);
    const int h = UiConfig::Scale::px (UiConfig::Layout::prefsPanelHeight);
    prefsPanel_->setBounds ((getWidth()  - w) / 2,
                            (getHeight() - h) / 2, w, h);
}

// ---------------------------------------------------------------------------
ProjectData MainComponent::currentProject() const
{
    ProjectData p = project_;                      // keep metadata + backing file
    const SimParams sp = controlPanel_.getParams();
    p.speakers              = sp.speakers;
    p.frequency             = sp.frequency;
    p.resolution            = sp.resolution;
    p.dBfloor               = sp.dBfloor;
    p.bandedSPL             = sp.bandedSPL;
    p.octaveSmoothing       = sp.octaveSmoothing;
    p.useMeasuredDirectivity = sp.useMeasuredDirectivity;
    return p;
}

juce::juce_wchar MainComponent::shortcutLetter (const juce::KeyPress& key)
{
    // Caps Lock must not break Ctrl/Cmd+Z/Y. Prefer the physical key code, then
    // the text character, then Ctrl letter codes (1=A … 26=Z) that some OSes
    // report when a modifier is held. Always compare in lowercase.
    auto fromCtrlCode = [] (int v) -> juce::juce_wchar
    {
        if (v >= 1 && v <= 26)
            return (juce::juce_wchar) ('a' + (v - 1));
        return 0;
    };

    juce::juce_wchar ch = 0;
    const int raw = key.getKeyCode();
    if ((raw >= 'A' && raw <= 'Z') || (raw >= 'a' && raw <= 'z'))
        ch = (juce::juce_wchar) raw;
    else if (auto c = fromCtrlCode (raw); c != 0)
        ch = c;

    if (ch == 0)
    {
        const auto t = (int) key.getTextCharacter();
        if ((t >= 'A' && t <= 'Z') || (t >= 'a' && t <= 'z'))
            ch = (juce::juce_wchar) t;
        else if (auto c = fromCtrlCode (t); c != 0)
            ch = c;
    }

    return juce::CharacterFunctions::toLowerCase (ch);
}

MainComponent::EditSnapshot MainComponent::takeEditSnapshot() const
{
    EditSnapshot s;
    s.scene = currentProject();
    s.drawings = patternComp_.getAnnotations();
    s.mics = patternComp_.getMics();
    return s;
}

void MainComponent::applyEditSnapshot (const EditSnapshot& s)
{
    restoringEdit_ = true;
    controlPanel_.applyProject (s.scene);
    patternComp_.setAnnotations (s.drawings);
    patternComp_.setMics (s.mics);
    patternComp_.setSpeakers (controlPanel_.getSpeakers(),
                              controlPanel_.getSelectedIndex());
    restoringEdit_ = false;
    editBaseline_ = s;
    refreshFrequencyResponse();
    resized();
}

void MainComponent::willEdit()
{
    if (restoringEdit_) return;
    undoStack_.push_back (editBaseline_);
    constexpr int kMax = 80;
    if ((int) undoStack_.size() > kMax)
        undoStack_.erase (undoStack_.begin(),
                          undoStack_.begin() + ((int) undoStack_.size() - kMax));
    redoStack_.clear();
}

void MainComponent::commitEdit()
{
    if (restoringEdit_) return;
    editBaseline_ = takeEditSnapshot();
}

void MainComponent::undoEdit()
{
    if (undoStack_.empty()) return;
    redoStack_.push_back (takeEditSnapshot());
    auto s = undoStack_.back();
    undoStack_.pop_back();
    applyEditSnapshot (s);
}

void MainComponent::redoEdit()
{
    if (redoStack_.empty()) return;
    undoStack_.push_back (takeEditSnapshot());
    auto s = redoStack_.back();
    redoStack_.pop_back();
    applyEditSnapshot (s);
}

void MainComponent::parentHierarchyChanged()
{
    if (keyHost_ != nullptr)
        keyHost_->removeKeyListener (this);
    keyHost_ = getTopLevelComponent();
    // Avoid double-registering if top-level is ourselves.
    if (keyHost_ != nullptr && keyHost_ != this)
        keyHost_->addKeyListener (this);
}

bool MainComponent::handleEditShortcut (const juce::KeyPress& key)
{
    // Prefer live modifiers — Caps Lock / drawing-tool focus can leave KeyPress mods stale.
    const auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
    const bool chord = mods.isCommandDown() || mods.isCtrlDown();
    if (! chord || mods.isAltDown())
        return false;

    if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        if (dynamic_cast<juce::TextEditor*> (focused) != nullptr)
            return false; // leave text-field native undo alone

    const auto letter = shortcutLetter (key);
    if (letter == 'z')
    {
        if (mods.isShiftDown())
            redoEdit();
        else
            undoEdit();
        return true;
    }
    if (letter == 'y')
    {
        redoEdit();
        return true;
    }
    if (letter == 'c')
    {
        patternComp_.copySelection();
        return true;
    }
    if (letter == 'v')
    {
        patternComp_.pasteClipboard();
        return true;
    }
    return false;
}

bool MainComponent::keyPressed (const juce::KeyPress& key)
{
    return handleEditShortcut (key);
}

bool MainComponent::keyPressed (const juce::KeyPress& key, juce::Component*)
{
    return handleEditShortcut (key);
}

void MainComponent::saveProject()
{
    if (project_.file == juce::File()) { saveProjectAs(); return; }
    ProjectData p = currentProject();
    if (p.saveToFile (project_.file))
    {
        project_ = p;
        AppSettings::get().addRecentProject (project_.file);
        statusStrip_.setStatus ("Project saved: " + project_.file.getFileName(), true);
    }
    else
        statusStrip_.setStatus ("Could not save project.", false);
}

void MainComponent::saveProjectAs()
{
    const auto suggested = (project_.file != juce::File())
        ? project_.file
        : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
              .getChildFile (juce::File::createLegalFileName (project_.displayName()) + ".atmk");

    fileChooser_ = std::make_unique<juce::FileChooser> ("Save project", suggested, "*.atmk");
    fileChooser_->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            f = f.withFileExtension ("atmk");
            ProjectData p = currentProject();
            if (p.saveToFile (f))
            {
                project_ = p;
                AppSettings::get().addRecentProject (f);
                statusStrip_.setStatus ("Project saved: " + f.getFileName(), true);
            }
        });
}

// AppSettings broadcast: theme and/or unit system changed.
void MainComponent::changeListenerCallback (juce::ChangeBroadcaster*)
{
    if (auto* lf = dynamic_cast<Brand::AtomikLookAndFeel*> (&getLookAndFeel()))
        lf->applyTheme();
    else
        Brand::refreshPalette();

    if (auto* w = dynamic_cast<juce::DocumentWindow*> (getTopLevelComponent()))
        w->setBackgroundColour (Brand::base());

    sendLookAndFeelChange();   // recursively notify children (restyles panels)

    // Unit-system change: re-render the read-out panels with converted values.
    {
        controlPanel_.refreshUnits();
        updateSettingsBar();
    }

    applyGridPref();   // grid show/hide may have changed
    resized();         // sidebar collapse may have changed
    repaint();
}

void MainComponent::toggleSidebar()
{
    AppSettings::get().setSidebarCollapsed (! AppSettings::get().sidebarCollapsed());
    resized();
    repaint();
}

int MainComponent::effectiveSidebarWidth() const
{
    return AppSettings::get().sidebarCollapsed()
               ? Brand::UI::sidebarCollapsedW
               : Brand::UI::sidebarW;
}

void MainComponent::syncSidebarToggleChrome()
{
    const bool collapsed = AppSettings::get().sidebarCollapsed();
    const bool dark = AppSettings::get().isDark();
    const auto ink   = dark ? Brand::white() : Brand::text();
    const auto inkHi = Brand::accent();

    auto mk = [] (const char* svg, juce::Colour c) -> std::unique_ptr<juce::Drawable>
    {
        if (auto xml = juce::parseXML (svg))
            if (auto d = juce::Drawable::createFromSVG (*xml))
            {
                d->replaceColour (juce::Colours::white, c);
                d->replaceColour (juce::Colour (0xffffffff), c);
                return d;
            }
        return {};
    };

    const auto n = mk (HeaderIcons::kHamburger, ink);
    const auto h = mk (HeaderIcons::kHamburger, inkHi);
    btnSidebarToggle_.setEdgeIndent (UiConfig::Scale::px (6));
    btnSidebarToggle_.setImages (n.get(), h.get(), h.get());
    btnSidebarToggle_.setColour (juce::DrawableButton::backgroundColourId,   juce::Colours::transparentBlack);
    btnSidebarToggle_.setColour (juce::DrawableButton::backgroundOnColourId, Brand::btnIn().withAlpha (0.35f));
    btnSidebarToggle_.setTooltip (collapsed ? "Show controls" : "Hide controls");
}

void MainComponent::lookAndFeelChanged()
{
    titleLabel_.setColour   (juce::Label::textColourId, Brand::text());
    versionLabel_.setColour (juce::Label::textColourId, Brand::muted());
    exportHeader_.setColour (juce::Label::textColourId, Brand::heading());
    viewHeader_.setColour   (juce::Label::textColourId, Brand::heading());
    terminalHeader_.setColour (juce::Label::textColourId, Brand::heading());

    btnStats_.setColour (juce::TextButton::buttonColourId,   Brand::statsBtn());
    btnStats_.setColour (juce::TextButton::textColourOffId,  Brand::statsText());
    btnProject_.setColour (juce::TextButton::buttonColourId,   Brand::statsBtn());
    btnProject_.setColour (juce::TextButton::textColourOffId,  Brand::statsText());

    auto restyle = [] (juce::TextButton& b)
    {
        const bool isExport = b.getButtonText().containsIgnoreCase ("SAVE")
                           || b.getButtonText().containsIgnoreCase ("EXPORT");
        styleActionBtn (b, b.getButtonText(),
                        isExport ? Brand::exportPill() : Brand::idleViewPill(), false);
    };
    for (auto* b : { &btnExportPNG_, &btnExportCSV_,
                     &btnViewSPL_, &btnViewDirectivity_, &btnViewMeasured_ })
        restyle (*b);
    updateViewButtonHighlights();
    commandTerminal_.lookAndFeelChanged();
    refreshHeaderIcons(); // also restyles Stats for light/dark mockup
    syncSidebarToggleChrome();
    logo_ = Brand::createLogo (Brand::text());
    repaint();
}

// ---------------------------------------------------------------------------
namespace
{
    // Cover square child corners so panel chrome reads as a rounded card.
    void maskRoundedCard (juce::Graphics& g, juce::Rectangle<float> r, float radius,
                          juce::Colour outside)
    {
        if (r.getWidth() < 2.0f || r.getHeight() < 2.0f)
            return;

        juce::Path p;
        p.setUsingNonZeroWinding (false);
        p.addRectangle (r.expanded (1.0f));
        p.addRoundedRectangle (r, radius);
        g.setColour (outside);
        g.fillPath (p);
    }

    void strokeRoundedCard (juce::Graphics& g, juce::Rectangle<float> r, float radius,
                            float stroke, juce::Colour col)
    {
        if (r.getWidth() < 2.0f || r.getHeight() < 2.0f || stroke <= 0.0f)
            return;
        Brand::strokeInsideRounded (g, r, radius, stroke, col);
    }
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (kBg());

    const int W = getWidth();
    const int H = getHeight();
    const int pad = UiConfig::Scale::px (8);
    const float rad = Brand::UI::cardRadius;
    const int headerH = Brand::UI::headerBandH;
    const int statusH = Brand::UI::statusStripH;
    const int bottomH = Brand::UI::bottomPanelH;
    const int sideW = effectiveSidebarWidth();
    const int sideX = pad;
    const int plotX = sideX + sideW + pad;
    const int centreW = juce::jmax (0, W - plotX - pad);
    const int bodyTop = headerH + pad;
    const int bottomTop = H - statusH - pad - bottomH;
    const int sidebarBottom = H - statusH - pad;

    // Rounded sidebar shell (same top as heatmap).
    g.setColour (Brand::panel());
    if (sideW > 0)
        g.fillRoundedRectangle ((float) sideX, (float) bodyTop,
                                (float) sideW, (float) juce::jmax (0, sidebarBottom - bodyTop),
                                rad);

    // Bottom export / view — same fill as the general app background.
    if (bottomH > 0 && centreW > 0)
    {
        g.setColour (Brand::base());
        g.fillRoundedRectangle ((float) plotX, (float) bottomTop,
                                (float) centreW, (float) bottomH, rad);
    }

    // Atomik wordmark, top-left of the header band (ATOMIK only).
    Brand::drawLogo (g, logo_.get(), Brand::headerLogoBounds ((float) headerH));
}

void MainComponent::paintOverChildren (juce::Graphics& g)
{
    const int W = getWidth();
    const int H = getHeight();
    if (W <= 0 || H <= 0)
        return;

    const int pad = UiConfig::Scale::px (8);
    const float rad = Brand::UI::cardRadius;
    const float stroke = juce::jmax (0.75f, 0.75f * Brand::UI::scale);
    const auto outside = kBg();
    const auto edge = Brand::sidebarBorder();
    const int headerH = Brand::UI::headerBandH;
    const int statusH = Brand::UI::statusStripH;
    const int bottomH = Brand::UI::bottomPanelH;
    const int sideW = effectiveSidebarWidth();
    const int sideX = pad;
    const int plotX = sideX + sideW + pad;
    const int centreW = juce::jmax (0, W - plotX - pad);
    const int bodyTop = headerH + pad;
    const int bottomTop = H - statusH - pad - bottomH;
    const int sidebarBottom = H - statusH - pad;

    // Sidebar card
    if (sideW > 0)
    {
        const auto rf = juce::Rectangle<float> ((float) sideX, (float) bodyTop,
                                               (float) sideW,
                                               (float) juce::jmax (0, sidebarBottom - bodyTop));
        maskRoundedCard (g, rf, rad, outside);
        strokeRoundedCard (g, rf, rad, stroke, edge);
    }

    // Heatmap window: one rounded frame around title bar + canvas (no extra top rule).
    {
        const auto plotFrame = plotHeader_.getBounds().getUnion (patternComp_.getBounds());
        if (! plotFrame.isEmpty())
        {
            const auto rf = plotFrame.toFloat();
            maskRoundedCard (g, rf, rad, outside);
            strokeRoundedCard (g, rf, rad, stroke, Brand::plotBorder());
        }
    }

    // Bottom toolbar — rounded fill only (no outer border stroke).
    if (bottomH > 0 && centreW > 0)
    {
        const auto rf = juce::Rectangle<float> ((float) plotX, (float) bottomTop,
                                               (float) centreW, (float) bottomH);
        maskRoundedCard (g, rf, rad, outside);
    }
}

void MainComponent::resized()
{
    const int W = getWidth();
    const int H = getHeight();
    Brand::UI::applyWindowScale (W, H);

    exportHeader_.setFont (Brand::techSemi (Brand::UI::scaledFont (Brand::Type::bottomSectionTitle)));
    viewHeader_.setFont (exportHeader_.getFont());
    terminalHeader_.setFont (exportHeader_.getFont());

    const int titleH    = Brand::UI::headerBandH;
    const int paramH    = Brand::UI::paramBarH;
    const int plotHdrH  = Brand::UI::plotHeaderH;
    const int bottomH   = Brand::UI::bottomPanelH;
    const int statusH   = Brand::UI::statusStripH;
    const int pad       = UiConfig::Scale::px (8);   // gap between panel cards
    const int innerPad  = UiConfig::Scale::px (4);   // inset inside rounded shells

    const int sideW = effectiveSidebarWidth();
    const int sideX = pad;
    const int plotX = sideX + sideW + pad;
    const int centreW = juce::jmax (280, W - plotX - pad);

    // Header: logo area left, title centre, icon cluster right
    const int rightPad = UiConfig::Scale::px (12);
    const int iconW = Brand::UI::headerIconW;
    const int iconGap = UiConfig::Scale::px (6);
    const int headerBtnTop = UiConfig::Scale::px (8);
    int rx = W - rightPad;
    btnMore_.setBounds       (rx - iconW, headerBtnTop, iconW, Brand::UI::headerIconH); rx -= iconW + iconGap;
    btnPrefsIcon_.setBounds  (rx - iconW, headerBtnTop, iconW, Brand::UI::headerIconH); rx -= iconW + iconGap;
    btnHelp_.setBounds       (rx - iconW, headerBtnTop, iconW, Brand::UI::headerIconH); rx -= iconW + iconGap * 2;

    const auto statsFont = Brand::techSemi (Brand::UI::scaledFont (Brand::Type::headerStatsButton));
    const int headerBtnGap = UiConfig::Scale::px (8);
    auto headerTextBtnW = [&] (const juce::TextButton& b) -> int
    {
        return juce::jmax (UiConfig::Scale::px (96),
                           juce::roundToInt (statsFont.getStringWidthFloat (b.getButtonText()) + 20.0f));
    };
    const int statsW = headerTextBtnW (btnStats_);
    const int projectW = headerTextBtnW (btnProject_);
    btnStats_.setBounds      (rx - statsW, headerBtnTop, statsW, Brand::UI::headerIconH); rx -= statsW + headerBtnGap;
    btnProject_.setBounds (rx - projectW, headerBtnTop, projectW, Brand::UI::headerIconH); rx -= projectW;

    {
        // Logo | title + version | flexible space | header buttons.
        // Version is never ellipsized: it keeps its full glyph width at every scale.
        const int logoRight = Brand::headerLogoRightReserve();
        const int regionR   = rx - UiConfig::Scale::px (12);
        const int regionW   = juce::jmax (1, regionR - logoRight);
        const int pairGap   = UiConfig::Scale::px (10);

        auto glyphW = [] (const juce::Font& font, const juce::String& text) -> int
        {
            return juce::roundToInt (font.getStringWidthFloat (text) + 8.0f);
        };

        juce::Font titleFont = Brand::techSemi (Brand::UI::scaledFont (Brand::Type::appTitle));
        float fontH = titleFont.getHeight();
        juce::Font versionFont = Brand::techSemi (juce::jmax (UiConfig::Laf::versionMin,
                                                              fontH * UiConfig::Laf::versionFromTitle));
        versionLabel_.setFont (versionFont);

        const juce::String titleText = titleLabel_.getText();
        const juce::String verText   = versionLabel_.getText();
        int vw = glyphW (versionFont, verText);
        int tw = glyphW (titleFont, titleText);

        while (fontH > UiConfig::Laf::titleShrinkMin
               && tw + pairGap + vw > regionW)
        {
            fontH -= 0.5f;
            titleFont = titleFont.withHeight (fontH);
            tw = glyphW (titleFont, titleText);
        }
        titleLabel_.setFont (titleFont);

        if (tw + pairGap + vw > regionW)
            tw = juce::jmax (48, regionW - pairGap - vw);

        const int pairW = tw + pairGap + vw;
        float pairX = 0.5f * (float) W - 0.5f * (float) pairW;
        pairX = juce::jlimit ((float) logoRight,
                              (float) juce::jmax (logoRight, regionR - pairW),
                              pairX);

        const int titleTop = UiConfig::Scale::px (6);
        const int labelH   = juce::jmax (16, titleH - UiConfig::Scale::px (8));
        titleLabel_.setBounds ((int) pairX, titleTop, tw, labelH);
        versionLabel_.setBounds ((int) pairX + tw + pairGap, titleTop, vw, labelH);
    }

    // Param stats live in Help; keep bar out of the layout.
    juce::ignoreUnused (paramH);
    paramBar_.setBounds (0, 0, 0, 0);

    // Sidebar + heatmap share the same top (aligned under header with panel gap).
    const int bodyTop    = titleH + pad;
    const int bottomTop  = H - statusH - pad - bottomH;
    const int bodyBottom = bottomTop - pad;
    const int bodyH      = juce::jmax (80, bodyBottom - bodyTop);

    // Tall left card sits beside both plot and bottom toolbar, ending above status.
    const int sidebarBottom = H - statusH - pad;
    const int sidebarH = juce::jmax (80, sidebarBottom - bodyTop);
    const bool collapsed = AppSettings::get().sidebarCollapsed();
    const int railW = Brand::UI::sidebarCollapsedW;
    const int burger = UiConfig::Scale::px (28);
    const int burgerPad = UiConfig::Scale::px (6);

    syncSidebarToggleChrome();

    if (collapsed)
    {
        controlViewport_.setVisible (false);
        controlViewport_.setBounds (0, 0, 0, 0);
        // Hamburger at top of the collapsed rail (expand).
        btnSidebarToggle_.setBounds (sideX + (railW - burger) / 2,
                                     bodyTop + burgerPad,
                                     burger, burger);
    }
    else
    {
        controlViewport_.setVisible (true);
        controlViewport_.setScrollBarThickness (UiConfig::Scale::px (8));
        // Content starts at the top again; hamburger overlays the top-right corner.
        const int panelH = juce::jmax (40, sidebarH - innerPad * 2);
        controlViewport_.setBounds (sideX, bodyTop + innerPad, sideW, panelH);
        const int innerW = sideW - controlViewport_.getScrollBarThickness();
        controlPanel_.setSize (innerW, panelH);
        if (controlPanel_.getContentHeight() > panelH)
            controlPanel_.setSize (innerW, controlPanel_.getContentHeight());

        btnSidebarToggle_.setBounds (sideX + sideW - burger - burgerPad,
                                     bodyTop + burgerPad,
                                     burger, burger);
    }
    btnSidebarToggle_.toFront (false);

    // Heatmap window — same bodyTop as sidebar (FR lives in its own floating window).
    plotHeader_.setBounds (plotX, bodyTop, centreW, plotHdrH);
    patternComp_.setBounds (plotX, bodyTop + plotHdrH, centreW,
                            juce::jmax (40, bodyH - plotHdrH));

    // Bottom panel — Export | View Mode | Terminal (VS Code–style cmd panel).
    const int secHdrH    = Brand::UI::bottomSectionHeaderH;
    const int sectionGap = Brand::UI::bottomSectionGap;
    const int viewColGap = Brand::UI::bottomViewColGap;
    const int btnGap     = Brand::UI::bottomExportBtnGap;
    const int exportW    = Brand::UI::bottomExportWidth;
    const int contentTop = bottomTop + secHdrH + Brand::UI::bottomContentTopPad;
    const int contentH   = bottomH - secHdrH - Brand::UI::bottomContentTopPad - UiConfig::Scale::px (2);
    const int btnH       = (contentH - 2 * btnGap) / 3;
    const int exportBtnH = (contentH - btnGap) / 2;

    int bx = plotX;
    exportHeader_.setBounds (bx, bottomTop, exportW, secHdrH);
    int ey = contentTop;
    btnExportPNG_.setBounds (bx, ey, exportW, exportBtnH); ey += exportBtnH + btnGap;
    btnExportCSV_.setBounds (bx, ey, exportW, exportBtnH);
    bx += exportW + sectionGap;

    // Left: view-mode buttons. Right: command terminal (replaces Phase/Arrival/STI).
    const int remainW  = centreW - exportW - sectionGap;
    const int viewColW = (remainW - viewColGap) / 2;
    const int termW    = remainW - viewColW - viewColGap;

    viewHeader_.setBounds (bx, bottomTop, viewColW, secHdrH);
    int vy = contentTop;
    btnViewSPL_.setBounds         (bx, vy, viewColW, btnH); vy += btnH + btnGap;
    btnViewDirectivity_.setBounds (bx, vy, viewColW, btnH); vy += btnH + btnGap;
    btnViewMeasured_.setBounds    (bx, vy, viewColW, btnH);
    bx += viewColW + viewColGap;

    terminalHeader_.setBounds (bx, bottomTop, termW, secHdrH);
    commandTerminal_.setBounds (bx, contentTop, termW, contentH);

    statusStrip_.setBounds (pad, H - statusH, juce::jmax (0, W - pad * 2), statusH);
    layoutPrefsPanel();
}

// ---------------------------------------------------------------------------
void MainComponent::scheduleRecompute()
{
    startTimer (60);   // debounce bursts of edits / drags
}

void MainComponent::timerCallback()
{
    stopTimer();
    runSimulation();
}

void MainComponent::runSimulation()
{
    if (isThreadRunning()) { startTimer (60); return; }   // retry shortly
    statusStrip_.setStatus ("Computing...", false);
    startThread();
}

void MainComponent::run()
{
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    SimParams p = controlPanel_.getParams();
    p.viewMode  = currentView_;
    {
        juce::ScopedLock sl (measLock_);
        p.directivity = directivityTables_;
        p.bemFields   = bemFieldTables_;
    }

    auto result = AcousticEngine::compute (p);
    lastElapsedSec_ = (juce::Time::getMillisecondCounterHiRes() - t0) / 1000.0;

    {
        juce::ScopedLock sl (resultLock_);
        lastResult_ = std::move (result);
        lastParams_ = p;
        hasResult_  = true;
    }

    juce::MessageManager::callAsync ([this]
    {
        SimResult r; SimParams p;
        {
            juce::ScopedLock sl (resultLock_);
            r = lastResult_;
            p = lastParams_;
        }
        applyResult (r);
    });
}

void MainComponent::applyResult (const SimResult& r)
{
    patternComp_.updateData (r, lastParams_);
    syncRenderer();
    updateSettingsBar();
    refreshFrequencyResponse();
    statusStrip_.setStatus ("Ready", true);
    statusStrip_.setLastRun ("Last run: "
        + juce::Time::getCurrentTime().formatted ("%d %b %Y  %H:%M:%S"));
    statusStrip_.setElapsed ("Elapsed: " + juce::String (lastElapsedSec_, 1) + " s");
}

void MainComponent::syncRenderer()
{
    patternComp_.setSpeakers (controlPanel_.getSpeakers(),
                              controlPanel_.getSelectedIndex());
}

void MainComponent::updatePlotChrome()
{
    const auto& p = lastParams_;
    const char* vmName = (currentView_ == ViewMode::SPL)         ? "SPL Heatmap"
                       : (currentView_ == ViewMode::Directivity) ? "Directivity"
                                                               : "Measured Polar";

    juce::String title = vmName;
    title += "  |  " + juce::String (lastResult_.activeSpeakers) + " devices";
    title += "  |  " + juce::String ((int) p.frequency) + " Hz";
    if (currentView_ == ViewMode::MeasuredPolar)
    {
        title += "  |  Measured @ " + juce::String (measDistanceM_, 1) + " m";
    }
    else if (lastResult_.usedMeasuredDirectivity)
    {
        const float simDist = MeasurementData::farFieldDirectivityDistance (measured_, measDistanceM_);
        title += "  |  Measured @ " + juce::String (simDist, 1) + " m";
    }
    plotHeader_.setTitle (title);
}

void MainComponent::applyPlotTool (RadiationPatternComponent::Tool tool, bool openColourPicker)
{
    patternComp_.setTool (tool);
    using T = RadiationPatternComponent::Tool;
    using A = PlotHeaderBar::ActiveTool;
    plotHeader_.setActiveTool (tool == T::Select ? A::Select
                              : tool == T::Pan    ? A::Pan
                              : tool == T::Pencil ? A::Pencil
                              : tool == T::Eraser ? A::Eraser
                              : tool == T::Ruler  ? A::Ruler
                                                 : A::Shape);
    plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
    patternComp_.grabKeyboardFocus();
    if (openColourPicker)
        showDrawColourPicker();
}

void MainComponent::showDrawColourPicker()
{
    struct ColourPicker : public juce::Component
    {
        std::function<void (juce::Colour)> onPick;
        juce::Colour selected;

        explicit ColourPicker (juce::Colour current) : selected (current)
        {
            setSize (188, 96);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (Brand::panel());
            g.setColour (Brand::border());
            g.drawRect (getLocalBounds(), 1);

            static const juce::uint32 kCols[] = {
                0xffffcc00, 0xffffffff, 0xffff3b30, 0xffff9500,
                0xff34c759, 0xff007aff, 0xffaf52de, 0xff000000
            };

            const int pad = 10, gap = 8, cell = 34;
            for (int i = 0; i < 8; ++i)
            {
                const int col = i % 4;
                const int row = i / 4;
                auto r = juce::Rectangle<float> ((float) (pad + col * (cell + gap)),
                                                 (float) (pad + row * (cell + gap)),
                                                 (float) cell, (float) cell);
                const auto c = juce::Colour (kCols[i]);
                g.setColour (c);
                g.fillRoundedRectangle (r, 5.0f);
                if (c == selected || (c.getARGB() == selected.getARGB()))
                {
                    g.setColour (Brand::accent());
                    g.drawRoundedRectangle (r.expanded (1.5f), 6.0f, 2.0f);
                }
                else
                {
                    g.setColour (Brand::border());
                    g.drawRoundedRectangle (r, 5.0f, 1.0f);
                }
            }
        }

        void mouseUp (const juce::MouseEvent& e) override
        {
            static const juce::uint32 kCols[] = {
                0xffffcc00, 0xffffffff, 0xffff3b30, 0xffff9500,
                0xff34c759, 0xff007aff, 0xffaf52de, 0xff000000
            };
            const int pad = 10, gap = 8, cell = 34;
            for (int i = 0; i < 8; ++i)
            {
                const int col = i % 4;
                const int row = i / 4;
                auto r = juce::Rectangle<int> (pad + col * (cell + gap),
                                               pad + row * (cell + gap),
                                               cell, cell);
                if (r.contains (e.getPosition()))
                {
                    selected = juce::Colour (kCols[i]);
                    if (onPick) onPick (selected);
                    if (auto* box = findParentComponentOfClass<juce::CallOutBox>())
                        box->dismiss();
                    return;
                }
            }
        }
    };

    auto* picker = new ColourPicker (patternComp_.getActiveDrawColour());
    picker->onPick = [this] (juce::Colour c)
    {
        patternComp_.setDrawColour (c);
        plotHeader_.setDrawColour (c);
    };

    juce::CallOutBox::launchAsynchronously (std::unique_ptr<juce::Component> (picker),
                                            plotHeader_.colourSwatch_.getScreenBounds(),
                                            nullptr);
}

void MainComponent::updateSettingsBar()
{
    const auto& p = lastParams_;
    juce::StringArray chips;

    if (currentView_ == ViewMode::MeasuredPolar)
    {
        const int hz = (int) (controlPanel_.getParams().frequency + 0.5);
        chips.add ("f = " + juce::String (hz) + " Hz");
        chips.add (MeasurementData::sourceName (measSource_));
        chips.add (juce::String (measDistanceM_, 1) + " m");
        chips.add ("View: Measured Polar");
    }
    else
    {
        const char* vm = (currentView_ == ViewMode::SPL) ? "SPL Heatmap" : "Directivity";
        chips.add ("f = " + juce::String ((int) p.frequency) + " Hz");
        chips.add ("lambda = " + juce::String (Units::metresToDisplay (lastResult_.lambda), 2)
                     + " " + Units::lengthUnit());
        chips.add ("Q21S = " + juce::String (lastResult_.activeSpeakers)
                     + " / " + juce::String ((int) p.speakers.size()));
        chips.add ("View: " + juce::String (vm));
        chips.add ("Grid: " + juce::String (p.resolution) + " x " + juce::String (p.resolution));
        if (lastResult_.usedMeasuredDirectivity)
        {
            const float simDist = MeasurementData::farFieldDirectivityDistance (measured_, measDistanceM_);
            juce::String m = "Measured @ " + juce::String (simDist, 1) + " m";
            if (lastResult_.measuredDirectivityHz > 0
                && lastResult_.measuredDirectivityHz != (int) p.frequency)
                m += " (" + juce::String (lastResult_.measuredDirectivityHz) + " Hz pattern)";
            chips.add (m);
        }
    }

    statChips_ = chips;   // shown via the Help (?) popup
    updatePlotChrome();
}

// ---------------------------------------------------------------------------
void MainComponent::setViewMode (ViewMode mode)
{
    currentView_ = mode;
    updateViewButtonHighlights();

    // Measured Polar: push readings, then recompute so a native BEM mid-plane
    // (Q21F, e.g. 52 Hz from 52Hz.xlsx) can render as the Heatmap.m field.
    if (mode == ViewMode::MeasuredPolar)
    {
        lastParams_.viewMode = mode;
        patternComp_.setMeasuredData (measured_);
        patternComp_.setMeasuredFrequency ((int) (controlPanel_.getParams().frequency + 0.5));
        SimResult r; { juce::ScopedLock sl (resultLock_); r = lastResult_; }
        patternComp_.updateData (r, lastParams_);
        updateSettingsBar();
        scheduleRecompute();
        return;
    }

    // View switching only re-colours the cached result (no recompute needed).
    SimResult r; bool ready;
    {
        juce::ScopedLock sl (resultLock_);
        r = lastResult_;
        ready = hasResult_;
    }
    if (ready)
    {
        lastParams_.viewMode = mode;
        patternComp_.updateData (r, lastParams_);
        syncRenderer();
        updateSettingsBar();
    }
}

void MainComponent::refreshHeaderIcons()
{
    btnStats_.setColour (juce::TextButton::buttonColourId,  Brand::statsBtn());
    btnStats_.setColour (juce::TextButton::textColourOffId, Brand::statsText());
    btnStats_.setColour (juce::TextButton::textColourOnId,  Brand::statsText());
    btnProject_.setColour (juce::TextButton::buttonColourId,  Brand::statsBtn());
    btnProject_.setColour (juce::TextButton::textColourOffId, Brand::statsText());
    btnProject_.setColour (juce::TextButton::textColourOnId,  Brand::statsText());

    auto hexRgb = [] (juce::Colour c) -> juce::String
    {
        return juce::String::formatted ("%02X%02X%02X",
                                        (int) c.getRed(), (int) c.getGreen(), (int) c.getBlue());
    };

    auto colourise = [] (const char* svg, juce::Colour ink, juce::Colour contrast)
        -> std::unique_ptr<juce::Drawable>
    {
        if (auto xml = juce::parseXML (svg))
            if (auto d = juce::Drawable::createFromSVG (*xml))
            {
                d->replaceColour (juce::Colours::white, ink);
                d->replaceColour (juce::Colour (0xffffffff), ink);
                d->replaceColour (juce::Colour (0x00ffffff), ink);
                d->replaceColour (juce::Colours::black, contrast);
                d->replaceColour (juce::Colour (0xff000000), contrast);
                return d;
            }
        return {};
    };

    // Help badge: bake disk/mark colours into SVG so the idle fill always shows
    // (replaceColour on SVG #fff can leave a white disk = invisible on light header).
    auto makeHelp = [&] (juce::Colour disk, juce::Colour mark) -> std::unique_ptr<juce::Drawable>
    {
        const auto svg = juce::String()
            + R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle cx="12" cy="12" r="11" fill="#)SVG"
            + hexRgb (disk)
            + R"SVG("/><path fill="#)SVG"
            + hexRgb (mark)
            + R"SVG(" d="M10.2 8.6c.35-1.15 1.3-1.9 2.7-1.9 1.55 0 2.65.95 2.65 2.35 0 .95-.45 1.55-1.35 2.15-.85.55-1.15.95-1.15 1.7v.45h-1.55v-.55c0-1.15.4-1.7 1.3-2.3.7-.45 1-0.85 1-1.4 0-.7-.55-1.15-1.35-1.15-.8 0-1.35.45-1.55 1.2l-1.7-.4zm1.95 7.55c.65 0 1.14-.5 1.15-1.15s-.5-1.15-1.15-1.15-1.15.5-1.15 1.15.5 1.15 1.15 1.15z"/></svg>)SVG";
        if (auto xml = juce::parseXML (svg))
            return juce::Drawable::createFromSVG (*xml);
        return {};
    };

    const bool dark = AppSettings::get().isDark();
    const auto ink    = dark ? Brand::white() : Brand::text();
    const auto inkHi  = Brand::accent();
    // Always-visible help disk: charcoal on light header, white on dark.
    const auto helpDisk = dark ? Brand::white() : juce::Colour (0xff333131);
    const auto helpMark = dark ? Brand::charcoal() : Brand::white();

    auto style = [&] (juce::DrawableButton& b, const char* svg, bool twoTone)
    {
        b.setEdgeIndent (Brand::UI::headerIconIndent);
        const auto c0 = colourise (svg, ink,   twoTone ? Brand::white() : ink);
        const auto c1 = colourise (svg, inkHi, twoTone ? Brand::white() : inkHi);
        b.setImages (c0.get(), c1.get(), c1.get());
        b.setColour (juce::DrawableButton::backgroundColourId,   juce::Colours::transparentBlack);
        b.setColour (juce::DrawableButton::backgroundOnColourId, Brand::btnIn().withAlpha (0.35f));
    };

    {
        btnHelp_.setEdgeIndent (Brand::UI::headerIconIndent);
        const auto c0 = makeHelp (helpDisk, helpMark);
        const auto c1 = makeHelp (Brand::accent(), Brand::white());
        btnHelp_.setImages (c0.get(), c1.get(), c1.get());
        btnHelp_.setColour (juce::DrawableButton::backgroundColourId,   juce::Colours::transparentBlack);
        btnHelp_.setColour (juce::DrawableButton::backgroundOnColourId, Brand::btnIn().withAlpha (0.35f));
    }
    style (btnPrefsIcon_, HeaderIcons::kGear, false);
    style (btnMore_,      HeaderIcons::kMenu, false);
}

void MainComponent::showOverflowMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Export PDF Report");
    m.addItem (2, "Save Project");
    m.addItem (3, "Preferences");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (btnMore_),
        [this] (int r)
        {
            if      (r == 1) exportPdfReport();
            else if (r == 2) saveProject();
            else if (r == 3) openPreferences();
        });
}

void MainComponent::showStatsPopup()
{
    // Build the same read-outs the old right pane showed, from the latest data.
    SimResult snap;
    { juce::ScopedLock sl (resultLock_); snap = lastResult_; }

    const auto live = controlPanel_.getParams();
    int nOn = 0;
    for (const auto& s : live.speakers) if (s.enabled) ++nOn;
    snap.activeSpeakers = nOn;
    snap.frequency      = live.frequency;

    auto content = std::make_unique<InfoPanel>();
    content->updateInfo (snap, live, controlPanel_.getSelectedIndex());

    // Two-step size: lay out once, then trim to the exact content height.
    const int popupW = Brand::UI::infoPanelW;
    content->setSize (popupW, 600);
    content->setSize (popupW, content->getContentHeight());

    juce::CallOutBox::launchAsynchronously (std::move (content),
                                            btnStats_.getScreenBounds(),
                                            nullptr);
}

void MainComponent::showMicPlaceOnRingDialog()
{
    const auto mics = patternComp_.getMics();
    if (mics.empty()) return;
    const auto speakers = controlPanel_.getSpeakers();
    const int defMic = juce::jmax (0, patternComp_.getSelectedMic());

    auto* body = new MicRefLockDialog (mics, speakers, defMic);
    body->onApply = [this] (int micIndex, int speakerIndex, float radiusM)
    {
        patternComp_.placeMicOnRing (micIndex, speakerIndex, radiusM);
        refreshFrequencyResponse();
    };

    juce::DialogWindow::LaunchOptions opts;
    opts.content.setOwned (body);
    opts.dialogTitle = "Place on ring";
    opts.dialogBackgroundColour = Brand::panel();
    opts.escapeKeyTriggersCloseButton = true;
    opts.useNativeTitleBar = true;
    opts.resizable = false;
    opts.launchAsync();
}

void MainComponent::ensureFrequencyResponseWindow()
{
    if (frWindow_ != nullptr)
        return;

    frWindow_ = std::make_unique<MicFrequencyResponseWindow>();
    frWindow_->content.onReferenceChanged = [this] (int idx)
    {
        frRefMic_ = idx;
        refreshFrequencyResponse();
    };

    if (auto* top = getTopLevelComponent())
    {
        frWindow_->setTopLeftPosition (top->getRight() - frWindow_->getWidth() - 24,
                                       top->getY() + 72);
    }

    frWindow_->setVisible (true);
    frWindow_->toFront (false);
}

void MainComponent::showFrequencyResponseWindow()
{
    if (patternComp_.getMics().empty())
        return;

    refreshFrequencyResponse();
    if (frWindow_ == nullptr)
        return;

    frWindow_->setVisible (true);
    frWindow_->toFront (true);
}

void MainComponent::refreshFrequencyResponse()
{
    const auto mics = patternComp_.getMics();
    if (mics.empty())
    {
        frWindow_.reset();
        return;
    }

    ensureFrequencyResponseWindow();

    if (frRefMic_ < 0 || frRefMic_ >= (int) mics.size())
        frRefMic_ = 0;

    SimParams base;
    {
        juce::ScopedLock sl (measLock_);
        base = controlPanel_.getParams();
        base.directivity = directivityTables_;
        base.bemFields = bemFieldTables_;
    }

    std::vector<std::vector<float>> curves ((size_t) mics.size());
    for (size_t mi = 0; mi < mics.size(); ++mi)
    {
        curves[mi].resize ((size_t) kNumSupportedFrequencies, 0.0f);
        for (int hi = 0; hi < kNumSupportedFrequencies; ++hi)
        {
            SimParams p = base;
            p.frequency = kSupportedFrequencies[hi];
            float intensityDb = 0.0f, absDb = 0.0f;
            if (AcousticEngine::sampleIntensityAt (p, mics[mi].x, mics[mi].y,
                                                   intensityDb, absDb))
                curves[mi][(size_t) hi] = intensityDb;
            else
                curves[mi][(size_t) hi] = -120.0f;
        }
    }

    frWindow_->content.setCurves (mics, curves, frRefMic_);
}

void MainComponent::showProjectMenu()
{
    juce::PopupMenu m;
    m.addItem (1, "Open Project (New Window)");
    m.addItem (2, "Open Project (Current Window)");
    m.addSeparator();
    m.addItem (3, "New Project");
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (btnProject_),
        [this] (int r)
        {
            if      (r == 1) openProjectInNewWindow();
            else if (r == 2) openProjectInCurrentWindow();
            else if (r == 3) launchNewProjectInstance();
        });
}

bool MainComponent::launchAppInstance (const juce::String& args)
{
    auto target = juce::File::getSpecialLocation (juce::File::currentExecutableFile);

   #if JUCE_MAC
    const auto bundle = target.getParentDirectory()
                              .getParentDirectory()
                              .getParentDirectory();
    if (bundle.hasFileExtension (".app") && bundle.isDirectory())
        target = bundle;
   #endif

    if (target.hasFileExtension (".app"))
        return juce::Process::openDocument (target.getFullPathName(), args);

    return juce::Process::openDocument (target.getFullPathName(), args);
}

void MainComponent::launchNewProjectInstance()
{
    if (! launchAppInstance ({}))
    {
        juce::AlertWindow::showMessageBoxAsync (
            juce::AlertWindow::WarningIcon,
            "New Project",
            "Could not start a new Atomik instance.");
    }
}

void MainComponent::openProjectInNewWindow()
{
    fileChooser_ = std::make_unique<juce::FileChooser> (
        "Open project in new window",
        juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.atmk");
    fileChooser_->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f == juce::File()) return;

            if (! launchAppInstance (f.getFullPathName().quoted()))
            {
                juce::AlertWindow::showMessageBoxAsync (
                    juce::AlertWindow::WarningIcon,
                    "Open Project",
                    "Could not open the project in a new window.\n\n"
                    + f.getFullPathName());
            }
        });
}

void MainComponent::openProjectInCurrentWindow()
{
    fileChooser_ = std::make_unique<juce::FileChooser> (
        "Open project in this window",
        project_.file != juce::File() ? project_.file.getParentDirectory()
            : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
        "*.atmk");
    fileChooser_->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f != juce::File())
                loadProjectFile (f);
        });
}

void MainComponent::loadProjectFile (const juce::File& f)
{
    ProjectData loaded;
    if (! ProjectData::loadFromFile (f, loaded))
    {
        statusStrip_.setStatus ("Could not open: " + f.getFileName(), false);
        return;
    }

    project_ = std::move (loaded);
    controlPanel_.applyProject (project_);
    patternComp_.clearAnnotations();
    patternComp_.clearMics();
    patternComp_.setSpeakers (controlPanel_.getSpeakers(),
                              controlPanel_.getSelectedIndex());
    undoStack_.clear();
    redoStack_.clear();
    editBaseline_ = takeEditSnapshot();
    refreshFrequencyResponse();
    resized();

    AppSettings::get().addRecentProject (project_.file);
    updateSettingsBar();
    updatePlotChrome();
    scheduleRecompute();
    statusStrip_.setStatus ("Opened: " + project_.displayName(), true);

    if (auto* w = dynamic_cast<juce::DocumentWindow*> (getTopLevelComponent()))
        w->setName ("Atomik Simulation Engine - " + project_.displayName());
}

void MainComponent::updateViewButtonHighlights()
{
    auto styleView = [] (juce::TextButton& b, bool on)
    {
        styleActionBtn (b, b.getButtonText(), Brand::idleViewPill(), on);
    };
    auto styleExport = [] (juce::TextButton& b)
    {
        styleActionBtn (b, b.getButtonText(), Brand::exportPill(), false);
    };
    styleExport (btnExportPNG_);
    styleExport (btnExportCSV_);

    styleView (btnViewSPL_,         currentView_ == ViewMode::SPL);
    styleView (btnViewDirectivity_, currentView_ == ViewMode::Directivity);
    styleView (btnViewMeasured_,    currentView_ == ViewMode::MeasuredPolar);
}

void MainComponent::highlightViewBtn (ViewMode mode)
{
    juce::ignoreUnused (mode);
    updateViewButtonHighlights();
}

// ---------------------------------------------------------------------------
void MainComponent::exportPNG()
{
    if (! hasResult_) { statusStrip_.setStatus ("Nothing to export.", false); return; }

    fileChooser_ = std::make_unique<juce::FileChooser> ("Save PNG image", juce::File{}, "*.png");
    fileChooser_->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult().withFileExtension ("png");
            if (f == juce::File{}) return;
            juce::FileOutputStream fos (f);
            if (fos.openedOk())
            {
                // 2x supersampled plot for client-ready image quality.
                const int pw = juce::jmax (1, patternComp_.getWidth());
                const int ph = juce::jmax (1, patternComp_.getHeight());
                juce::Image plotHi (juce::Image::RGB, pw * 2, ph * 2, true);
                {
                    juce::Graphics pg (plotHi);
                    pg.addTransform (juce::AffineTransform::scale (2.0f));
                    patternComp_.paintEntireComponent (pg, false);
                }
                juce::Image plot = plotHi.rescaled (pw * 2, ph * 2,
                                                    juce::Graphics::highResamplingQuality);

                SimResult r; SimParams pr;
                { juce::ScopedLock sl (resultLock_); r = lastResult_; pr = lastParams_; }

                juce::Image sheet = ReportExport::renderHeatmapSheet (plot, project_, pr, r);
                juce::PNGImageFormat fmt;
                fmt.writeImageToStream (sheet, fos);
                statusStrip_.setStatus ("Saved: " + f.getFileName(), true);
            }
        });
}

void MainComponent::exportPdfReport()
{
    if (! hasResult_) { statusStrip_.setStatus ("Run a simulation before exporting a report.", false); return; }

    const auto suggested = (project_.file != juce::File())
        ? project_.file.withFileExtension ("pdf")
        : juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
              .getChildFile (juce::File::createLegalFileName (project_.displayName()) + " - Report.pdf");

    fileChooser_ = std::make_unique<juce::FileChooser> ("Export PDF report", suggested, "*.pdf");
    fileChooser_->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;
            f = f.withFileExtension ("pdf");

            statusStrip_.setStatus ("Generating PDF report...", false);
            // Defer the heavy work so the status line repaints first.
            juce::MessageManager::callAsync ([this, f] { buildAndWriteReport (f); });
        });
}

void MainComponent::buildAndWriteReport (const juce::File& f)
{
    ReportBuilder::ReportInputs in;
    in.project = project_;
    if (in.project.meta.date.isEmpty()) in.project.meta.date = ProjectMeta::today();

    // Live scene + last result.
    SimParams base = controlPanel_.getParams();
    { juce::ScopedLock sl (measLock_); base.directivity = directivityTables_; base.bemFields = bemFieldTables_; }
    { juce::ScopedLock sl (resultLock_); in.result = lastResult_; }
    base.viewMode = ViewMode::SPL;
    in.params = base;

    in.room = {};
    in.room.w = base.worldW;
    in.room.d = base.worldH;

    // SPL heatmaps at every measured frequency (and current selection).
    std::vector<int> reportHz;
    for (const auto& mf : measured_.freqs)
        if (mf.ok) reportHz.push_back (mf.hz);
    const int curHz = (int) (base.frequency + 0.5);
    if (std::find (reportHz.begin(), reportHz.end(), curHz) == reportHz.end())
        reportHz.push_back (curHz);
    std::sort (reportHz.begin(), reportHz.end());

    for (int hz : reportHz)
    {
        ReportBuilder::HeatmapEntry e;
        e.image = renderHeatmapImage ((double) hz, base, e.coveragePct,
                                      &e.peakAbsDb, &e.hasAbsoluteSpl);
        e.hz = hz;
        in.heatmaps.push_back (std::move (e));
    }

    PdfDocument pdf;
    ReportBuilder::build (pdf, in);

    if (pdf.writeToFile (f))
    {
        AppSettings::get().addRecentProject (project_.file != juce::File() ? project_.file : f);
        statusStrip_.setStatus ("Report exported: " + f.getFileName(), true);
    }
    else
        statusStrip_.setStatus ("Could not write PDF report.", false);
}

// ---------------------------------------------------------------------------
// Workspace / layout (Phase 5 & 6)
void MainComponent::applyGridPref()
{
    const bool on = AppSettings::get().showGrid();
    patternComp_.setShowGrid (on);
    controlPanel_.setGridToggleState (on);
}

void MainComponent::importLayout()
{
    fileChooser_ = std::make_unique<juce::FileChooser> (
        "Import reference layout", juce::File{}, "*.png;*.jpg;*.jpeg;*.dxf");
    fileChooser_->launchAsync (
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            const auto f = fc.getResult();
            if (f == juce::File()) return;

            LayoutLayer L;
            L.name = f.getFileName();
            const auto ext = f.getFileExtension().toLowerCase();

            if (ext == ".dxf")
            {
                juce::Path p; juce::Rectangle<float> b;
                if (! DxfImport::load (f, p, b))
                {
                    statusStrip_.setStatus ("Could not read DXF: " + f.getFileName(), false);
                    return;
                }
                L.kind = LayoutLayer::Kind::Dxf;
                L.path = p;
                L.srcBounds = b;
            }
            else
            {
                juce::Image img = juce::ImageFileFormat::loadFrom (f);
                if (! img.isValid())
                {
                    statusStrip_.setStatus ("Could not read image: " + f.getFileName(), false);
                    return;
                }
                L.kind = LayoutLayer::Kind::Image;
                L.image = img;
                L.srcBounds = juce::Rectangle<float> (0.0f, 0.0f,
                                  (float) img.getWidth(), (float) img.getHeight());
            }

            // Default placement: centre the layer in the world, ~20 m wide.
            L.widthM   = 20.0f;
            L.originM  = { (float) (15.0 - L.widthM * 0.5),
                           (float) (15.0 - L.heightM() * 0.5) };
            L.opacity  = 0.65f;
            L.visible  = true;
            L.locked   = false;

            layout_ = L;
            controlPanel_.refreshLayoutControls();
            applyLayoutSettings();
            statusStrip_.setStatus ("Layout imported: " + f.getFileName(), true);
        });
}

void MainComponent::removeLayout()
{
    layout_.clear();
    controlPanel_.refreshLayoutControls();
    patternComp_.repaint();
    statusStrip_.setStatus ("Layout removed.", true);
}

void MainComponent::applyLayoutSettings()
{
    patternComp_.setLayoutEditMode (controlPanel_.layoutEditMode());
    patternComp_.setLayoutSnap (controlPanel_.layoutSnap());
    patternComp_.repaint();
}

juce::Image MainComponent::renderHeatmapImage (double freq, const SimParams& base, double& coverageOut,
                                               double* peakAbsOut, bool* hasAbsOut)
{
    SimParams p = base;
    p.frequency = freq;
    p.viewMode  = ViewMode::SPL;

    SimResult r = AcousticEngine::compute (p);
    coverageOut = AcousticAnalysis::coverageWithin (r, 6.0);
    if (peakAbsOut != nullptr) *peakAbsOut = r.peakAbsDb;
    if (hasAbsOut  != nullptr) *hasAbsOut  = r.hasAbsoluteSpl;

    RadiationPatternComponent comp;
    comp.setBounds (0, 0, 980, 760);
    comp.setMeasuredData (measured_);
    comp.updateData (r, p);
    comp.setSpeakers (p.speakers, -1);
    comp.resetView();

    juce::Image img (juce::Image::RGB, comp.getWidth(), comp.getHeight(), true);
    juce::Graphics g (img);
    comp.paint (g);
    return img;
}

void MainComponent::exportCSV()
{
    if (! hasResult_) { statusStrip_.setStatus ("Nothing to export.", false); return; }

    fileChooser_ = std::make_unique<juce::FileChooser> ("Export SPL CSV", juce::File{}, "*.csv");
    fileChooser_->launchAsync (
        juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult().withFileExtension ("csv");
            if (f == juce::File{}) return;
            juce::FileOutputStream fos (f);
            if (! fos.openedOk()) return;

            juce::ScopedLock sl (resultLock_);
            const auto& r = lastResult_;
            const auto& pr = lastParams_;
            const int W = r.width, H = r.height;
            if (W <= 0 || H <= 0) return;

            const auto now = juce::Time::getCurrentTime();
            int nDev = 0;
            for (const auto& s : pr.speakers) if (s.enabled) ++nDev;

            const bool absOk = r.hasAbsoluteSpl
                && r.splAbsDB.size() == (size_t) W * (size_t) H;
            const bool relOk = r.splRelDB.size() == (size_t) W * (size_t) H;

            auto line = [&] (const juce::String& s)
            {
                fos.writeText (s + "\n", false, false, nullptr);
            };

            line ("# Atomik Simulation Engine v1.3.7");
            line ("# Product,Q21S");
            line ("# www.atomikaudio.com");
            line ("# Generated," + now.formatted ("%d %b %Y") + "," + now.formatted ("%H:%M:%S"));
            line ("# Frequency_Hz," + juce::String (r.frequency, 1));
            line ("# Units," + juce::String (nDev));
            line ("# World_m," + juce::String (r.worldW, 3) + " x " + juce::String (r.worldH, 3));
            line ("# Grid," + juce::String (W) + " x " + juce::String (H));
            if (absOk)
                line ("# Peak_dB_SPL," + juce::String (r.peakAbsDb, 2));
            line ("# Directivity," + juce::String (r.usedMeasuredDirectivity
                    ? "measured" : "model"));
            line ("# spl_dB is "
                  + juce::String (absOk
                        ? "absolute calibrated dB SPL (unfloored)"
                        : "relative dB (0 = peak, unfloored)"));
            line ("# rel_dB is relative to the map peak (0 = loudest, unfloored)");
            line ("x_m,y_m,rel_dB,spl_dB");

            const double dx = (W > 1) ? r.worldW / (W - 1) : 0.0;
            const double dy = (H > 1) ? r.worldH / (H - 1) : 0.0;
            juce::MemoryOutputStream mos;
            mos.preallocate ((size_t) W * (size_t) H * 36);

            for (int row = 0; row < H; ++row)
            {
                const double y = row * dy;
                for (int col = 0; col < W; ++col)
                {
                    const size_t i = (size_t) row * (size_t) W + (size_t) col;
                    const float rel = relOk ? r.splRelDB[i]
                                            : (absOk ? (r.splAbsDB[i] - (float) r.peakAbsDb)
                                                     : r.splDB[i]);
                    const float spl = absOk ? r.splAbsDB[i] : rel;
                    mos << juce::String (col * dx, 4) << ","
                        << juce::String (y, 4) << ","
                        << juce::String (rel, 2) << ","
                        << juce::String (spl, 2) << "\n";
                }
            }
            fos.write (mos.getData(), mos.getDataSize());
            statusStrip_.setStatus ("Saved: " + f.getFileName(), true);
        });
}

// ---------------------------------------------------------------------------
// Measured polar data: load + live auto-refresh
// ---------------------------------------------------------------------------
void MainComponent::rebuildDirectivityTables()
{
    {
        juce::ScopedLock sl (measLock_);
        // Heatmap / array sim: far-field BEM arc (not 0.5 m near-field lobes).
        const float simDist = MeasurementData::farFieldDirectivityDistance (measured_, measDistanceM_);
        directivityTables_ = MeasurementData::buildDirectivityTables (measured_, simDist);
        bemFieldTables_    = MeasurementData::loadBemFieldTables (measured_);
    }
    patternComp_.setMeasuredDistance (measDistanceM_);
    patternComp_.setMeasuredData (measured_);
}

void MainComponent::loadMeasurements()
{
    measured_      = MeasurementData::loadMeasurements (measDir_, measSource_);
    measSignature_ = measurementsSignature();

    // Distance choices depend on which sweeps exist for this set.
    const auto dists = MeasurementData::availableDistances (measured_);
    const float prefer = (measSource_ == MeasurementData::Gylt) ? 0.5f : 1.0f;
    controlPanel_.setAvailableDistances (dists, prefer);
    measDistanceM_ = controlPanel_.getMeasurementDistance();

    rebuildDirectivityTables();
    patternComp_.setMeasuredFrequency ((int) (controlPanel_.getParams().frequency + 0.5));
    updateSettingsBar();
}

void MainComponent::setMeasurementDistance (float distanceM)
{
    if (std::abs (measDistanceM_ - distanceM) < 1.0e-4f) return;
    measDistanceM_ = distanceM;
    patternComp_.setMeasuredDistance (measDistanceM_);
    patternComp_.setMeasuredData (measured_);

    statusStrip_.setStatus ("Polar distance: " + juce::String (measDistanceM_, 1) + " m", true);
    updateSettingsBar();

    // UI distance drives Measured Polar only. SPL prediction keeps the far-field
    // (≈2 m) pattern — no need to rebuild engine tables or recompute.
    if (currentView_ == ViewMode::MeasuredPolar)
        patternComp_.repaint();
}

void MainComponent::setMeasurementSource (int src)
{
    src = juce::jlimit (0, 1, src);
    if (src == measSource_) return;

    measSource_ = src;
    AppSettings::get().setMeasurementSource (src);
    measDir_ = MeasurementData::folderForSource (src);

    loadMeasurements();                 // rebuilds measured_ + directivity tables
    patternComp_.setMeasuredFrequency ((int) (controlPanel_.getParams().frequency + 0.5));

    statusStrip_.setStatus (juce::String ("Measurement set: ")
                        + MeasurementData::sourceName (src), true);
    updateSettingsBar();

    if (currentView_ == ViewMode::MeasuredPolar)
        patternComp_.repaint();
    else
        scheduleRecompute();            // re-run so the heat map uses the new directivity
}

juce::int64 MainComponent::measurementsSignature() const
{
    juce::int64 sig = 0;
    // Prefer MeasurementIntegrationPack CSVs; also watch legacy .xlsx.
    const juce::File pack = MeasurementData::packDataFolder();
    const juce::File xlsx = MeasurementData::xlsxFolderForSource (measSource_);
    const int roomFreqs[] = { 30, 80, 200, 500 };
    const int gpFreqs[]   = { 30, 60, 100, 150, 200 };
    const int* freqList = (measSource_ == MeasurementData::Gylt) ? roomFreqs : gpFreqs;
    const int  nFreq    = (measSource_ == MeasurementData::Gylt) ? 4 : 5;
    const float dists[] = { 0.5f, 1.0f, 2.0f };
    const juce::String setName = MeasurementData::packSetName (measSource_);

    for (int i = 0; i < nFreq; ++i)
        for (float d : dists)
        {
            const juce::File csv = pack.getChildFile (
                MeasurementData::csvFileName (setName, freqList[i], d));
            if (csv.existsAsFile())
                sig += csv.getLastModificationTime().toMilliseconds() + csv.getSize();

            const juce::String distTag = (std::abs (d - 0.5f) < 1.0e-3f) ? "0.5"
                                      : (std::abs (d - 2.0f) < 1.0e-3f) ? "2" : "1";
            const juce::File xf = MeasurementData::fileFor (xlsx, freqList[i], distTag);
            if (xf.existsAsFile())
                sig += xf.getLastModificationTime().toMilliseconds() + xf.getSize();
        }
    return sig;
}

void MainComponent::pollMeasurements()
{
    const juce::int64 sig = measurementsSignature();
    if (sig == measSignature_) return;   // unchanged

    loadMeasurements();
    statusStrip_.setStatus ("Measurements refreshed: "
                        + juce::Time::getCurrentTime().formatted ("%H:%M:%S"), true);
    if (currentView_ == ViewMode::MeasuredPolar)
        patternComp_.repaint();
    else
        scheduleRecompute();   // field views: re-run so heat map reflects new directivity
}

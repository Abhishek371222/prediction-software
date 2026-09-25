#include "MainComponent.h"
#include "PreferencesComponent.h"
#include "InfoDialogComponent.h"
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
    titleLabel_.setFont (Brand::tech (Brand::Type::appTitle));
    titleLabel_.setColour (juce::Label::textColourId, Brand::text());
    // Centred inside its own box: resized() sizes that box to the measured
    // string plus a small slack, so right-justifying would push the ink off
    // the window's centre line by exactly that slack.
    titleLabel_.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel_);

    versionLabel_.setText ("v1.4.0.5", juce::dontSendNotification);
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

    btnTerminalDock_.setButtonText ("Undock");
    btnTerminalDock_.setTooltip ("Undock terminal to a floating window");
    btnTerminalDock_.setColour (juce::TextButton::buttonColourId, Brand::plotToolbar());
    btnTerminalDock_.setColour (juce::TextButton::textColourOffId,
                                AppSettings::get().isDark() ? Brand::white() : Brand::text());
    btnTerminalDock_.onClick = [this]
    {
        if (isTerminalDocked())
            undockTerminal();
        else
            dockTerminal();
    };
    addAndMakeVisible (btnTerminalDock_);

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
    commandTerminal_.onExecuteCommand = [this] (const juce::String& verb, const juce::String& args)
    {
        return handleTerminalCommand (verb, args);
    };
    commandTerminal_.onSessionInput = [this] (const juce::String& line)
    {
        return handleTerminalSessionLine (line);
    };
    commandTerminal_.onSessionCancel = [this]
    {
        cancelCurrentCommand (false);
        preferTerminalFocus();
    };

    btnStats_.setComponentID ("headerStats");
    btnStats_.setButtonText ("Statistics");
    btnStats_.setTooltip ("Scene Summary & Selected Speaker");
    btnStats_.setColour (juce::TextButton::buttonColourId,   Brand::statsBtn());
    btnStats_.setColour (juce::TextButton::textColourOffId,  Brand::statsText());
    btnStats_.onClick = [this] { showStatsPopup(); };
    addAndMakeVisible (btnStats_);

    btnProject_.setComponentID ("headerNewProject");
    btnProject_.setButtonText (juce::String ("Project") + juce::String::fromUTF8 (" \xe2\x96\xbe"));
    btnProject_.setTooltip ("Project: save (Ctrl+S), open, or create");
    btnProject_.setColour (juce::TextButton::buttonColourId,   Brand::statsBtn());
    btnProject_.setColour (juce::TextButton::textColourOffId,  Brand::statsText());
    btnProject_.onClick = [this] { showProjectMenu(); };
    addAndMakeVisible (btnProject_);

    toggleAutosave_.setButtonText ("Auto Save");   // Figma row 1 spells it as two words
    toggleAutosave_.setComponentID ("headerToggle");
    toggleAutosave_.setTooltip ("AutoSave on/off (writes automatically when dirty)");
    toggleAutosave_.setToggleState (AppSettings::get().autosaveEnabled(),
                                    juce::dontSendNotification);
    toggleAutosave_.onClick = [this]
    {
        const bool on = toggleAutosave_.getToggleState();
        AppSettings::get().setAutosaveEnabled (on);
        // The pill's wording depends on this, so refresh it now rather than
        // leaving a stale "Unsaved changes" until the next edit.
        updateSaveIndicator();
        if (on)
        {
            if (! autoSaveTimer_.isTimerRunning())
                autoSaveTimer_.startTimer (kAutosaveTickMs);
        }
        else
        {
            autoSaveTimer_.stopTimer();
        }
    };
    addAndMakeVisible (toggleAutosave_);

    btnInfo_.setTooltip ("Keyboard shortcuts");
    btnHelp_.setTooltip ("Help");
    btnPrefsIcon_.setTooltip ("Preferences");
    btnMore_.setTooltip ("More options");
    // Figma redesign: Info/Help/Settings form row 2's "Help" cluster —
    // reparented into plotHeader_ so it can lay them out like every other
    // labeled cluster (icons + label + divider). btnMore_ has no Figma slot
    // and stays hidden (see resized()), so it keeps its MainComponent parent.
    plotHeader_.setHelpIcons (btnPrefsIcon_, btnInfo_, btnHelp_);   // gear, info, ? — Figma order
    addAndMakeVisible (btnMore_);
    btnPrefsIcon_.onClick = [this] { openPreferences(); };
    btnMore_.onClick      = [this] { showOverflowMenu(); };
    btnInfo_.onClick      = [this] { showKeyboardShortcuts(); };
    btnHelp_.onClick      = [this]
    {
        const juce::String stats = statChips_.isEmpty()
            ? juce::String ("No simulation stats yet. Run a simulation to see live values.")
            : statChips_.joinIntoString ("\n");
        showInfoPanel ("Simulation Stats", {}, stats);
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
    auto chooseShape = [this] (int shapeId, int constructionId)
    {
        using DS = RadiationPatternComponent::DrawShape;
        using C  = RadiationPatternComponent::Construction;
        static const DS shapes[] = {
            DS::Line, DS::Polyline, DS::Circle, DS::Arc, DS::Rectangle, DS::Square, DS::TextBox
        };
        if (shapeId < 0 || shapeId >= (int) (sizeof (shapes) / sizeof (shapes[0])))
            return;
        if (constructionId < 0 || constructionId > (int) C::TextBoxClick)
            return;
        patternComp_.setDrawShape (shapes[shapeId], (C) constructionId);
        patternComp_.setAddMicArmed (false);
        patternComp_.setAddSpeakerArmed (false);
        applyPlotTool (RadiationPatternComponent::Tool::Shape, false, true);
        plotHeader_.setActiveTool (PlotHeaderBar::ActiveTool::Shape);
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
        plotHeader_.setFillAlphaEnabled (patternComp_.hasFillTarget());
        patternComp_.grabKeyboardFocus();
    };

    // Figma Shapes cluster: one icon per shape (Line / Polyline / Circle /
    // Rectangle / Text box) instead of one button opening a menu.
    plotHeader_.onShapeChosen = chooseShape;

    // Figma Colours cluster: the 8 palette dots set the draw colour directly.
    plotHeader_.onSwatchPicked = [this] (juce::Colour c)
    {
        patternComp_.setDrawColour (c);
        plotHeader_.setDrawColour (c);
    };

    // Figma File cluster. Tooltips are set alongside the icons in UiChrome.h
    // ("New Project", "Save Project (Ctrl+S)", ...), these are the actions.
    plotHeader_.onFileNew    = [this] { launchNewProjectInstance(); };
    plotHeader_.onFileOpen   = [this] { openProjectInCurrentWindow(); };
    plotHeader_.onFileSave   = [this] { saveProject(); };
    plotHeader_.onFileSaveAs = [this] { saveProjectAs(); };
    plotHeader_.onFileExport = [this] { exportPdfReport(); };

    plotHeader_.btnShape_.onClick = [this, chooseShape]
    {
        // Retained for the old menu path (button itself is hidden now).
        if (! plotHeader_.btnShape_.getToggleState())
            return;
        plotHeader_.showShapeMenu (chooseShape);
    };
    plotHeader_.btnMic_.onClick = [this]
    {
        const bool hasMics = ! patternComp_.getMics().empty();
        plotHeader_.showMicMenu ([this] (int itemId)
        {
            if (itemId == 1)
            {
                patternComp_.setAddSpeakerArmed (false);
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
    // The plot drives the simulated region: zooming changes how many metres
    // are solved for, not how much of a fixed box is visible, so the field
    // always covers the canvas and the reachable range is unbounded.
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
    plotHeader_.onFillAlphaChanged = [this]
    {
        patternComp_.setDrawFillAlpha (plotHeader_.getFillAlpha01());
        plotHeader_.repaint();
    };
    patternComp_.onAnnotSelectionChanged = [this]
    {
        // Swatch + opacity follow the selected shape (or the draw brush if none).
        plotHeader_.setFillAlpha01 (patternComp_.getActiveFillAlpha());
        plotHeader_.setDrawColour (patternComp_.getActiveDrawColour());
        plotHeader_.setFillAlphaEnabled (patternComp_.hasFillTarget());
        plotHeader_.repaint();
    };
    patternComp_.onToolChanged = [this] (RadiationPatternComponent::Tool t)
    {
        plotHeader_.setFillAlphaEnabled (patternComp_.hasFillTarget());
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
    headerStatus_.setMode (StatusStrip::Mode::Pill);
    statusStrip_.setMode (StatusStrip::Mode::RunInfo);
    // Figma redesign: "Ready" lives in row 2's ribbon (far right, same row
    // as the tool icons), not the title row — reparent into plotHeader_.
    plotHeader_.setReadyPill (headerStatus_,
                              [this] { return headerStatus_.preferredPillWidth(); });
    // ...and the red "SPL Heatmap | ..." caption belongs on the canvas's
    // top-left, not inside the ribbon. Reparent it here; plotHeader_.setTitle()
    // still drives its text from every existing call site.
    addAndMakeVisible (plotHeader_.getTitleLabel());
    reportStatus ("Ready", true);

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
    controlPanel_.onAddSpeakerRequest = [this]
    {
        if (! controlPanel_.canAddSpeaker())
        {
            reportStatus ("Limit reached: "
                          + juce::String (ControlPanel::kMaxSpeakers)
                          + " Q21S units max — delete one to add another", false);
            return;
        }
        patternComp_.setAddSpeakerArmed (true);
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
        reportStatus ("Click the plot to place a " + controlPanel_.activeModelName(), true);
        patternComp_.grabKeyboardFocus();
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
    patternComp_.onPlaceSpeakerAt = [this] (float x, float y)
    {
        if (! controlPanel_.addSpeakerAt (x, y))
        {
            // At the cap: stop placement mode rather than silently swallowing
            // clicks, so it is obvious why nothing is appearing.
            patternComp_.setAddSpeakerArmed (false);
            plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
            reportStatus ("Limit reached: "
                          + juce::String (ControlPanel::kMaxSpeakers)
                          + " Q21S units max — delete one to add another", false);
            return;
        }

        willEdit();
        syncRenderer();
        scheduleRecompute();
        commitEdit();
        const int n = (int) controlPanel_.getParams().speakers.size();
        const juce::String model = controlPanel_.activeModelName();
        reportStatus (n >= ControlPanel::kMaxSpeakers
                          ? model + " placed — limit of "
                                + juce::String (ControlPanel::kMaxSpeakers) + " reached"
                          : model + " placed — click again to add another (Esc cancels)",
                      true);
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
    };
    patternComp_.onAddSpeakerArmedChanged = [this]
    {
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
        if (! patternComp_.isAddSpeakerArmed())
            updateSaveIndicator();
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
    patternComp_.onRequestCancelCurrentTool = [this] { cancelCurrentCommand(); };

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
    // Both Q21S and BEM2inch are loaded unconditionally (reloadAllMeasurements)
    // so mixed-model scenes work from the start; measSource_ only selects
    // which one the Measured Polar reference view shows.
    measSource_ = AppSettings::get().measurementSource();

    // The legacy Room / GYLT set reads a sidecar shyamGuildMeasurements folder
    // that shipped builds do not carry, so a stale setting pointing at it left
    // the combo blank AND the engine with an empty directivity table -- which
    // silently degrades every prediction to omnidirectional instead of the
    // measured pattern. Fall back unless that data is genuinely present.
    if (measSource_ != MeasurementData::OpenField
        && measSource_ != MeasurementData::BEM2in
        && ! MeasurementData::folderForSource (measSource_).isDirectory())
    {
        measSource_ = MeasurementData::OpenField;
        AppSettings::get().setMeasurementSource (measSource_);
    }

    measDir_ = MeasurementData::folderForSource (measSource_);
    controlPanel_.setMeasurementSource (measSource_);
    controlPanel_.onMeasurementSourceChanged   = [this] (int s) { setMeasurementSource (s); };
    controlPanel_.onMeasurementDistanceChanged = [this] (float d) { setMeasurementDistance (d); };
    reloadAllMeasurements();
    measPoll_.fn = [this] { pollMeasurements(); };
    measPoll_.startTimer (1000);

    autoSaveTimer_.fn = [this] { autosaveIfNeeded(); };
    if (AppSettings::get().autosaveEnabled())
        autoSaveTimer_.startTimer (kAutosaveTickMs);   // writes shortly after an edit

    highlightViewBtn (currentView_);
    updatePlotChrome();

    // Load the project's scene (may be empty — clean slate for new projects).
    controlPanel_.applyProject (project_);

    AppSettings::get().addChangeListener (this);
    applyGridPref();
    plotHeader_.refreshUnits();

    setSize (1340, 820);   // after all child components exist (setSize calls resized)
    setWantsKeyboardFocus (true);
    addKeyListener (this); // hear keys while a child (toolbar / plot) has focus
    grabKeyboardFocus();
    editBaseline_ = takeEditSnapshot();

    // Only compute when the scene has units; empty projects show the world grid.
    if (! controlPanel_.getSpeakers().empty())
        runSimulation();
    else
    {
        lastParams_ = controlPanel_.getParams();
        patternComp_.updateData (SimResult{}, lastParams_);
        syncRenderer();
    }

    // Restore floating terminal if the user left it undocked last session.
    if (AppSettings::get().terminalUndocked())
        undockTerminal();
    else
        syncTerminalDockChrome();

    // A project that has just been opened is, by definition, unmodified.
    // Wiring up the panels during construction runs through the same change
    // hooks a real edit does, which left a freshly opened file already
    // reporting "Unsaved changes". State the invariant instead of trying to
    // suppress each hook individually.
    projectDirty_   = false;
    lastAutosaveMs_ = 0;
    updateSaveIndicator();

    // Opacity starts greyed: the Select tool with nothing selected has no fill
    // to act on. setFillAlphaEnabled is the only thing that flips it.
    plotHeader_.setFillAlphaEnabled (patternComp_.hasFillTarget());
}

MainComponent::~MainComponent()
{
    removeKeyListener (this);
    if (keyHost_ != nullptr)
        keyHost_->removeKeyListener (this);
    AppSettings::get().removeChangeListener (this);
    measPoll_.stopTimer();
    autoSaveTimer_.stopTimer();
    stopTimer();
    stopThread (3000);
    // Re-parent terminal onto this component before windows die.
    if (terminalFloat_ != nullptr)
    {
        terminalFloat_->clearContentComponent();
        terminalFloat_.reset();
        addChildComponent (commandTerminal_);
    }
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

void MainComponent::showInfoPanel (const juce::String& heading,
                                   const juce::String& subtitle,
                                   const juce::String& body)
{
    if (infoPanel_ == nullptr)
    {
        infoPanel_ = std::make_unique<InfoDialogComponent>();
        infoPanel_->onClose = [this] { if (infoPanel_ != nullptr) infoPanel_->setVisible (false); };
        addChildComponent (*infoPanel_);
    }

    infoPanel_->setContent (heading, subtitle, body);
    infoPanel_->setVisible (true);
    infoPanel_->toFront (true);
    layoutInfoPanel();
}

void MainComponent::layoutInfoPanel()
{
    if (infoPanel_ == nullptr || ! infoPanel_->isVisible()) return;
    const int w = UiConfig::Scale::px (UiConfig::Layout::prefsPanelWidth);
    const int h = juce::jmin (getHeight() - UiConfig::Scale::px (40),
                              infoPanel_->preferredHeight (w));
    infoPanel_->setBounds ((getWidth()  - w) / 2,
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
    markProjectDirty();
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
    markProjectDirty();
}

void MainComponent::markProjectDirty()
{
    projectDirty_ = true;
    updateSaveIndicator();
}

void MainComponent::updateSaveIndicator()
{
    if (! projectDirty_)
    {
        reportStatus ("Ready", true);
        return;
    }

    // With autosave on the edit is about to be written, so say that rather than
    // "Unsaved changes" -- which read as a warning and, with the old timing,
    // could sit there for the best part of a minute while autosave was in fact
    // working. "Unsaved changes" is now reserved for when nothing will write.
    const bool autoOn = AppSettings::get().autosaveEnabled();
    reportStatus (autoOn ? "Saving..." : "Unsaved changes", autoOn);
}

juce::File MainComponent::autosaveFileForProject() const
{
    auto dir = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                   .getChildFile ("Atomik")
                   .getChildFile ("Autosave");
    dir.createDirectory();
    const auto base = juce::File::createLegalFileName (
        project_.displayName().isNotEmpty() ? project_.displayName() : "Untitled");
    return dir.getChildFile (base + ".atmk");
}

bool MainComponent::writeProjectToFile (const juce::File& f, bool quiet)
{
    if (f == juce::File()) return false;
    ProjectData p = currentProject();
    if (! p.saveToFile (f))
    {
        if (! quiet)
            reportStatus ("Could not save project.", false);
        return false;
    }
    project_ = p;
    project_.file = f;
    projectDirty_ = false;
    AppSettings::get().addRecentProject (f);
    if (! quiet)
        reportStatus ("Project saved: " + f.getFileName(), true);
    else
        reportStatus ("Autosaved: " + f.getFileName(), true);
    return true;
}

void MainComponent::autosaveIfNeeded()
{
    if (! AppSettings::get().autosaveEnabled())
        return;
    if (! projectDirty_) return;
    // Don't interrupt an open file dialog / modal.
    if (juce::ModalComponentManager::getInstance()->getNumModalComponents() > 0)
        return;

    // The timer already paces this; a second, much longer throttle on top of
    // it was the reason autosave felt broken. The timer ran every 15 s but a
    // write was refused unless 30 s had passed, so a dirty project could sit
    // unsaved for up to 45 s with "Unsaved changes" showing the whole time.
    // Keep only a short floor so a continuous drag does not write every tick.
    const auto now = juce::Time::currentTimeMillis();
    if (lastAutosaveMs_ > 0 && (now - lastAutosaveMs_) < kAutosaveMinGapMs)
        return;

    juce::File target = project_.file;
    if (target == juce::File())
        target = autosaveFileForProject();

    if (writeProjectToFile (target, true))
        lastAutosaveMs_ = now;
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
    // Esc: always return to the cursor tool from Shape / Pencil / Ruler / etc.
    // (also works when focus is on the toolbar or sidebar, not only the plot).
    if (key.isKeyCode (juce::KeyPress::escapeKey))
    {
        if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
            if (dynamic_cast<juce::TextEditor*> (focused) != nullptr)
                return false; // leave terminal / text fields alone

        if (prefsPanel_ != nullptr && prefsPanel_->isVisible())
        {
            prefsPanel_->setVisible (false);
            return true;
        }

        cancelCurrentCommand();
        return true;
    }

    // Prefer live modifiers — Caps Lock / drawing-tool focus can leave KeyPress mods stale.
    const auto mods = juce::ModifierKeys::getCurrentModifiersRealtime();
    const bool chord = mods.isCommandDown() || mods.isCtrlDown();
    if (! chord || mods.isAltDown())
        return false;

    const int code = key.getKeyCode();
    const auto textCh = key.getTextCharacter();

    // Ctrl+[ or Ctrl+\ — cancel current command (same as Esc for tools).
    if (code == '[' || textCh == '[' || code == '\\' || textCh == '\\')
    {
        cancelCurrentCommand();
        return true;
    }

    const auto letter = shortcutLetter (key);

    // Ctrl/Cmd+S — save (works even while editing a text box).
    if (letter == 's')
    {
        saveProject();
        return true;
    }

    if (auto* focused = juce::Component::getCurrentlyFocusedComponent())
        if (dynamic_cast<juce::TextEditor*> (focused) != nullptr)
            return false; // leave text-field native undo / copy alone

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
    if (letter == 'x')
    {
        patternComp_.cutSelection();
        return true;
    }
    if (letter == 'v')
    {
        patternComp_.pasteClipboard();
        return true;
    }
    if (letter == 'd')
    {
        // Toggle coordinate / SPL readout under the cursor.
        const bool on = ! plotHeader_.btnSplProbe_.getToggleState();
        plotHeader_.btnSplProbe_.setToggleState (on, juce::dontSendNotification);
        patternComp_.setShowSplProbe (on);
        reportStatus (on ? "Coordinate display on" : "Coordinate display off", true);
        return true;
    }
    if (letter == 'g')
    {
        const bool on = ! AppSettings::get().showGrid();
        AppSettings::get().setShowGrid (on); // persists + applyGridPref via broadcast
        reportStatus (on ? "Grid on" : "Grid off", true);
        return true;
    }
    if (letter == 'f')
    {
        const bool on = ! plotHeader_.btnSnap_.getToggleState();
        plotHeader_.btnSnap_.setToggleState (on, juce::dontSendNotification);
        patternComp_.setDrawGridSnap (on);
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
        reportStatus (on ? "Snap on" : "Snap off", true);
        return true;
    }
    return false;
}

void MainComponent::cancelCurrentCommand (bool focusPlot)
{
    terminalSessionKind_ = TerminalSessionKind::None;
    if (patternComp_.isAddMicArmed())
        patternComp_.setAddMicArmed (false);
    if (patternComp_.isAddSpeakerArmed())
        patternComp_.setAddSpeakerArmed (false);
    patternComp_.cancelDrawSession();
    patternComp_.clearPlotSelection();
    applyPlotTool (RadiationPatternComponent::Tool::Select, false, focusPlot);
}

void MainComponent::reportStatus (const juce::String& state, bool ready)
{
    statusStrip_.setStatus (state, ready);
    headerStatus_.setStatus (state, ready);
    // The pill sizes itself to its text, so the ribbon has to re-lay-out when
    // the message changes — otherwise longer statuses stay clipped.
    plotHeader_.refreshStatusLayout();
}

void MainComponent::preferTerminalFocus()
{
    if (terminalFloat_ != nullptr)
    {
        terminalFloat_->setMinimised (false);
        terminalFloat_->toFront (true);
    }
    commandTerminal_.focusInput();
}

void MainComponent::syncTerminalDockChrome()
{
    const bool docked = isTerminalDocked();
    btnTerminalDock_.setButtonText (docked ? "Undock" : "Dock");
    btnTerminalDock_.setTooltip (docked
        ? "Undock terminal to a floating window"
        : "Dock terminal back into the main window");
    btnTerminalDock_.setColour (juce::TextButton::buttonColourId, Brand::plotToolbar());
    btnTerminalDock_.setColour (juce::TextButton::textColourOffId,
                                AppSettings::get().isDark() ? Brand::white() : Brand::text());
    terminalHeader_.setText (docked ? "TERMINAL" : "TERMINAL (floating)",
                             juce::dontSendNotification);
}

void MainComponent::undockTerminal()
{
    if (terminalFloat_ != nullptr)
        return;

    removeChildComponent (&commandTerminal_);

    terminalFloat_ = std::make_unique<TerminalFloatWindow> (
        commandTerminal_,
        [this] { dockTerminal(); });

    if (auto* top = getTopLevelComponent())
    {
        const int margin = 24;
        terminalFloat_->setTopLeftPosition (
            top->getX() + juce::jmax (margin, top->getWidth() - terminalFloat_->getWidth() - margin),
            top->getY() + juce::jmax (margin, top->getHeight() - terminalFloat_->getHeight() - margin));
    }

    terminalFloat_->setVisible (true);
    terminalFloat_->toFront (true);
    AppSettings::get().setTerminalUndocked (true);
    syncTerminalDockChrome();
    resized();
    commandTerminal_.focusInput();
}

void MainComponent::dockTerminal()
{
    if (terminalFloat_ == nullptr)
    {
        // Already docked — still refresh chrome.
        if (! commandTerminal_.getParentComponent())
            addAndMakeVisible (commandTerminal_);
        AppSettings::get().setTerminalUndocked (false);
        syncTerminalDockChrome();
        resized();
        return;
    }

    terminalFloat_->clearContentComponent();
    terminalFloat_.reset();
    addAndMakeVisible (commandTerminal_);
    AppSettings::get().setTerminalUndocked (false);
    syncTerminalDockChrome();
    resized();
    commandTerminal_.focusInput();
}

void MainComponent::showKeyboardShortcuts()
{
    // "Manage workflow" is the panel's subtitle now, not the first body line.
    const juce::String body =
        "Ctrl+C    Copy object\n"
        "Ctrl+X    Cut object\n"
        "Ctrl+V    Paste object\n"
        "\n"
        "Ctrl+Z    Undo last action\n"
        "Ctrl+Y    Redo last action\n"
        "Ctrl+[    Cancel current command (or Ctrl+\\)\n"
        "Esc       Cancel current command\n"
        "Ctrl+D    Toggle coordinate display\n"
        "Ctrl+G    Toggle Grid\n"
        "Ctrl+F    Toggle Snap\n"
        "\n"
        "Ctrl+S    Save project";

    showInfoPanel ("Keyboard Shortcuts", "Manage workflow", body);
}

bool MainComponent::parseAnnotPoint (const juce::String& text, juce::Point<float>& out)
{
    auto s = text.trim();
    if (s.isEmpty()) return false;

    // Accept "x,y" or "x y" or "x;y"
    s = s.replaceCharacter (';', ' ').replaceCharacter (',', ' ');
    juce::StringArray parts;
    parts.addTokens (s, " \t", "");
    parts.removeEmptyStrings();
    if (parts.size() < 2) return false;

    const float x = (float) parts[0].getDoubleValue();
    const float y = (float) parts[1].getDoubleValue();
    if (! std::isfinite (x) || ! std::isfinite (y)) return false;
    out = { x, y };
    return true;
}

MainComponent::TerminalResult MainComponent::armDrawCommand (const juce::String& verb)
{
    using DS = RadiationPatternComponent::DrawShape;
    using C  = RadiationPatternComponent::Construction;
    using T  = RadiationPatternComponent::Tool;

    patternComp_.setAddMicArmed (false);
    patternComp_.setAddSpeakerArmed (false);
    terminalSessionKind_ = TerminalSessionKind::Draw;

    auto startSession = [this] (const juce::String& firstPrompt) -> TerminalResult
    {
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
        preferTerminalFocus();
        return TerminalResult::continueSession (firstPrompt);
    };

    if (verb == "pencil")
    {
        terminalSessionKind_ = TerminalSessionKind::None;
        applyPlotTool (T::Pencil, false, false);
        preferTerminalFocus();
        return TerminalResult::ok ("PENCIL — draw freehand on the plot (Esc cancels).");
    }
    if (verb == "eraser")
    {
        terminalSessionKind_ = TerminalSessionKind::None;
        applyPlotTool (T::Eraser, false, false);
        preferTerminalFocus();
        return TerminalResult::ok ("ERASER — drag on shapes to erase (Esc cancels).");
    }
    if (verb == "dist" || verb == "ruler")
    {
        terminalSessionKind_ = TerminalSessionKind::Dist;
        applyPlotTool (T::Ruler, false, false);
        return startSession ("Specify first point:");
    }
    if (verb == "line")
    {
        patternComp_.setDrawShape (DS::Line, C::LineTwoPoints);
        applyPlotTool (T::Shape, false, false);
        return startSession ("Specify first point:");
    }
    if (verb == "pline")
    {
        patternComp_.setDrawShape (DS::Polyline, C::PolylinePoints);
        applyPlotTool (T::Shape, false, false);
        return startSession ("Specify start point:");
    }
    if (verb == "circle")
    {
        patternComp_.setDrawShape (DS::Circle, C::CircleCenterRadius);
        applyPlotTool (T::Shape, false, false);
        return startSession ("Specify center point:");
    }
    if (verb == "arc")
    {
        patternComp_.setDrawShape (DS::Arc, C::ArcThreePoints);
        applyPlotTool (T::Shape, false, false);
        return startSession ("Specify start point of arc:");
    }
    if (verb == "rectang")
    {
        patternComp_.setDrawShape (DS::Rectangle, C::RectTwoCorners);
        applyPlotTool (T::Shape, false, false);
        return startSession ("Specify first corner:");
    }
    if (verb == "square")
    {
        patternComp_.setDrawShape (DS::Square, C::SquareTwoCorners);
        applyPlotTool (T::Shape, false, false);
        return startSession ("Specify first corner:");
    }
    if (verb == "text")
    {
        patternComp_.setDrawShape (DS::TextBox, C::TextBoxClick);
        applyPlotTool (T::Shape, false, false);
        return startSession ("Specify insertion point:");
    }

    terminalSessionKind_ = TerminalSessionKind::None;
    return TerminalResult::fail ("Unknown draw command.");
}

MainComponent::TerminalResult MainComponent::beginMoveCommand()
{
    if (! patternComp_.hasCopyableSelection())
        return TerminalResult::fail ("Nothing selected. Select objects on the plot, then type MOVE.");

    terminalSessionKind_ = TerminalSessionKind::MoveBase;
    return TerminalResult::continueSession ("Specify base point:");
}

MainComponent::TerminalResult MainComponent::beginZoomCommand (const juce::String& args)
{
    const auto a = args.trim().toLowerCase();
    if (a == "e" || a == "extents" || a == "all" || a == "fit")
    {
        patternComp_.resetView();
        return TerminalResult::ok ("Zoom Extents.");
    }
    if (a == "i" || a == "in" || a == "+")
    {
        patternComp_.zoomIn();
        return TerminalResult::ok ("Zoom In.");
    }
    if (a == "o" || a == "out" || a == "-")
    {
        patternComp_.zoomOut();
        return TerminalResult::ok ("Zoom Out.");
    }

    terminalSessionKind_ = TerminalSessionKind::Zoom;
    return TerminalResult::continueSession ("Enter an option [Extents/In/Out]:");
}

MainComponent::TerminalResult MainComponent::handleTerminalSessionLine (const juce::String& line)
{
    auto trimmed = line.trim();
    const auto lower = trimmed.toLowerCase();

    if (lower == "cancel" || lower == "esc")
    {
        terminalSessionKind_ = TerminalSessionKind::None;
        cancelCurrentCommand (false);
        preferTerminalFocus();
        return TerminalResult::endSession ("Command canceled.");
    }

    if (terminalSessionKind_ == TerminalSessionKind::Zoom)
    {
        const auto r = beginZoomCommand (trimmed);
        if (r.kind == TerminalResult::Kind::ContinueSession)
            return TerminalResult::fail ("Invalid option. Use E (Extents), I (In), or O (Out).");
        terminalSessionKind_ = TerminalSessionKind::None;
        preferTerminalFocus();
        return TerminalResult::endSession (r.message.isNotEmpty() ? r.message : juce::String ("Zoom."));
    }

    if (terminalSessionKind_ == TerminalSessionKind::MoveBase)
    {
        juce::Point<float> pt;
        if (! parseAnnotPoint (trimmed, pt))
            return TerminalResult::fail ("Invalid point. Use x,y (e.g. 10,20).");
        terminalMoveBase_ = pt;
        terminalSessionKind_ = TerminalSessionKind::MoveSecond;
        return TerminalResult::continueSession ("Specify second point:");
    }

    if (terminalSessionKind_ == TerminalSessionKind::MoveSecond)
    {
        juce::Point<float> pt;
        if (! parseAnnotPoint (trimmed, pt))
            return TerminalResult::fail ("Invalid point. Use x,y (e.g. 10,20).");
        const auto d = pt - terminalMoveBase_;
        if (patternComp_.onWillEdit) patternComp_.onWillEdit();
        // World and annot space share metres on SPL view; polar uses normalized — still apply same delta in annot space.
        patternComp_.moveSelectionBy (d, d);
        if (patternComp_.onEditCommitted) patternComp_.onEditCommitted();
        patternComp_.repaint();
        terminalSessionKind_ = TerminalSessionKind::None;
        preferTerminalFocus();
        return TerminalResult::endSession ("Move completed.");
    }

    // Draw / Dist sessions
    if (patternComp_.getTool() == RadiationPatternComponent::Tool::Shape
        && patternComp_.getDrawShape() == RadiationPatternComponent::DrawShape::Polyline)
    {
        if (trimmed.isEmpty() || lower == "finish" || lower == "done" || lower == "enter")
        {
            if (patternComp_.finishPolylineCommand (false))
            {
                plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
                applyPlotTool (RadiationPatternComponent::Tool::Select, false, false);
                terminalSessionKind_ = TerminalSessionKind::None;
                preferTerminalFocus();
                return TerminalResult::endSession ("Polyline created.");
            }
            return TerminalResult::fail ("Need at least 2 points before FINISH.");
        }
        // Close option — only while PLINE session (not CIRCLE alias)
        if (lower == "close" || (lower == "c" && terminalSessionKind_ == TerminalSessionKind::Draw))
        {
            if (patternComp_.finishPolylineCommand (true))
            {
                plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
                applyPlotTool (RadiationPatternComponent::Tool::Select, false, false);
                terminalSessionKind_ = TerminalSessionKind::None;
                preferTerminalFocus();
                return TerminalResult::endSession ("Closed polyline created.");
            }
            return TerminalResult::fail ("Need at least 2 points before CLOSE.");
        }
    }

    if (trimmed.isEmpty()
        && patternComp_.getDrawShape() == RadiationPatternComponent::DrawShape::Line
        && patternComp_.drawSessionPointCount() >= 2)
    {
        // LINE: Enter after 2+ points — already committed by feed; treat as finish
        applyPlotTool (RadiationPatternComponent::Tool::Select, false, false);
        terminalSessionKind_ = TerminalSessionKind::None;
        preferTerminalFocus();
        return TerminalResult::endSession ("Line created.");
    }

    juce::Point<float> pt;
    if (! parseAnnotPoint (trimmed, pt))
        return TerminalResult::fail ("Invalid point. Use x,y (e.g. 10,20).");

    const auto shapeBefore = patternComp_.getDrawShape();
    const auto toolBefore = patternComp_.getTool();
    const int ptsBefore = patternComp_.drawSessionPointCount();

    if (! patternComp_.feedAnnotPoint (pt))
        return TerminalResult::fail ("Could not accept point.");

    plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());

    const bool stillActive = patternComp_.isDrawSessionActive();
    if (! stillActive)
    {
        juce::String done = "Done.";
        if (toolBefore == RadiationPatternComponent::Tool::Ruler)
            done = "Distance measured.";
        else if (shapeBefore == RadiationPatternComponent::DrawShape::Line)
            done = "Line created.";
        else if (shapeBefore == RadiationPatternComponent::DrawShape::Circle)
            done = "Circle created.";
        else if (shapeBefore == RadiationPatternComponent::DrawShape::Arc)
            done = "Arc created.";
        else if (shapeBefore == RadiationPatternComponent::DrawShape::Rectangle)
            done = "Rectangle created.";
        else if (shapeBefore == RadiationPatternComponent::DrawShape::Square)
            done = "Square created.";
        else if (shapeBefore == RadiationPatternComponent::DrawShape::TextBox)
            done = "Text box placed — type in the plot.";
        else if (shapeBefore == RadiationPatternComponent::DrawShape::Polyline)
            done = "Polyline created.";

        applyPlotTool (RadiationPatternComponent::Tool::Select, false, false);
        terminalSessionKind_ = TerminalSessionKind::None;
        preferTerminalFocus();
        return TerminalResult::endSession (done);
    }

    // Still collecting points — AutoCAD-style next prompts
    juce::String next = "Specify next point:";
    if (shapeBefore == RadiationPatternComponent::DrawShape::Line)
        next = "Specify next point:";
    else if (shapeBefore == RadiationPatternComponent::DrawShape::Polyline)
        next = "Specify next point or [Close/Undo]:";
    else if (shapeBefore == RadiationPatternComponent::DrawShape::Circle && ptsBefore == 0)
        next = "Specify radius point:";
    else if (shapeBefore == RadiationPatternComponent::DrawShape::Circle)
        next = "Specify radius point:";
    else if (shapeBefore == RadiationPatternComponent::DrawShape::Arc)
        next = ptsBefore == 0 ? "Specify second point of arc:"
             : ptsBefore == 1 ? "Specify end point of arc:"
                              : "Specify next point:";
    else if (shapeBefore == RadiationPatternComponent::DrawShape::Rectangle
          || shapeBefore == RadiationPatternComponent::DrawShape::Square)
        next = "Specify other corner:";
    else if (toolBefore == RadiationPatternComponent::Tool::Ruler)
        next = "Specify second point:";

    const auto prompt = patternComp_.getDrawPrompt();
    return TerminalResult::continueSession (next.isNotEmpty() ? next : prompt);
}

MainComponent::TerminalResult MainComponent::handleTerminalCommand (const juce::String& verb,
                                                                   const juce::String& args)
{
    auto toggleFlag = [&args] (bool currentlyOn) -> bool
    {
        const auto a = args.trim().toLowerCase();
        if (a == "on" || a == "1" || a == "true")  return true;
        if (a == "off" || a == "0" || a == "false") return false;
        return ! currentlyOn;
    };

    if (verb == "help")
        return TerminalResult::ok (CommandTerminal::builtinHelpText());

    if (verb == "cancel")
    {
        terminalSessionKind_ = TerminalSessionKind::None;
        cancelCurrentCommand (false);
        preferTerminalFocus();
        return TerminalResult::ok ("Command canceled.");
    }

    if (verb == "undo")
    {
        undoEdit();
        return TerminalResult::ok ("Undo.");
    }
    if (verb == "redo")
    {
        redoEdit();
        return TerminalResult::ok ("Redo.");
    }

    if (verb == "copy")
    {
        if (! patternComp_.copySelection())
            return TerminalResult::fail ("Nothing selected to copy.");
        return TerminalResult::ok ("Copied to clipboard.");
    }
    if (verb == "cut")
    {
        if (! patternComp_.cutSelection())
            return TerminalResult::fail ("Nothing selected to cut.");
        return TerminalResult::ok ("Cut to clipboard.");
    }
    if (verb == "paste")
    {
        if (! patternComp_.pasteClipboard())
            return TerminalResult::fail ("Clipboard is empty.");
        return TerminalResult::ok ("Pasted.");
    }
    if (verb == "erase")
    {
        if (! patternComp_.deleteSelection())
            return TerminalResult::fail ("Nothing selected to erase.");
        return TerminalResult::ok ("Erased.");
    }

    if (verb == "move")
        return beginMoveCommand();

    if (verb == "zoom")
        return beginZoomCommand (args);

    if (verb == "grid")
    {
        const bool on = toggleFlag (AppSettings::get().showGrid());
        AppSettings::get().setShowGrid (on);
        return TerminalResult::ok (on ? "Grid on." : "Grid off.");
    }
    if (verb == "snap")
    {
        const bool on = toggleFlag (plotHeader_.btnSnap_.getToggleState());
        plotHeader_.btnSnap_.setToggleState (on, juce::dontSendNotification);
        patternComp_.setDrawGridSnap (on);
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
        return TerminalResult::ok (on ? "Snap on." : "Snap off.");
    }
    if (verb == "coords")
    {
        const bool on = toggleFlag (plotHeader_.btnSplProbe_.getToggleState());
        plotHeader_.btnSplProbe_.setToggleState (on, juce::dontSendNotification);
        patternComp_.setShowSplProbe (on);
        return TerminalResult::ok (on ? "Coordinate display on." : "Coordinate display off.");
    }
    if (verb == "ortho")
    {
        const bool on = toggleFlag (plotHeader_.btnOrtho_.getToggleState());
        plotHeader_.btnOrtho_.setToggleState (on, juce::dontSendNotification);
        patternComp_.setOrtho (on);
        if (on)
            patternComp_.applyOrthoSpeakerLayout();
        plotHeader_.setDrawPrompt (patternComp_.getDrawPrompt());
        return TerminalResult::ok (on ? "Ortho on." : "Ortho off.");
    }

    if (verb == "save")
    {
        saveProject();
        return TerminalResult::ok ("Save requested.");
    }
    if (verb == "saveas")
    {
        saveProjectAs();
        return TerminalResult::ok ("Save As…");
    }
    if (verb == "open")
    {
        openProjectInCurrentWindow();
        return TerminalResult::ok ("Open project…");
    }

    if (verb == "run")
    {
        runSimulation();
        return TerminalResult::ok ("Simulation started.");
    }

    if (verb == "fit")
    {
        patternComp_.resetView();
        return TerminalResult::ok ("Zoom Extents.");
    }
    if (verb == "zoomin")
    {
        patternComp_.zoomIn();
        return TerminalResult::ok ("Zoom In.");
    }
    if (verb == "zoomout")
    {
        patternComp_.zoomOut();
        return TerminalResult::ok ("Zoom Out.");
    }

    if (verb == "select")
    {
        applyPlotTool (RadiationPatternComponent::Tool::Select, false, false);
        preferTerminalFocus();
        return TerminalResult::ok ("Select tool.");
    }
    if (verb == "pan")
    {
        applyPlotTool (RadiationPatternComponent::Tool::Pan, false, false);
        preferTerminalFocus();
        return TerminalResult::ok ("Pan tool.");
    }

    if (verb == "viewspl")
    {
        setViewMode (ViewMode::SPL);
        return TerminalResult::ok ("View: SPL Gradient Plot.");
    }
    if (verb == "viewdir")
    {
        setViewMode (ViewMode::Directivity);
        return TerminalResult::ok ("View: Directivity.");
    }
    if (verb == "viewmeas")
    {
        setViewMode (ViewMode::MeasuredPolar);
        return TerminalResult::ok ("View: Measured Polar.");
    }

    if (verb == "color" || verb == "colour")
    {
        showDrawColourPicker();
        return TerminalResult::ok ("Colour picker opened.");
    }
    if (verb == "opacity")
    {
        const auto a = args.trim();
        if (a.isEmpty())
            return TerminalResult::fail ("Usage: OPACITY <0-100>");
        const float pct = (float) a.getDoubleValue();
        if (! std::isfinite (pct))
            return TerminalResult::fail ("Invalid opacity.");
        patternComp_.setDrawFillAlpha (juce::jlimit (0.0f, 1.0f, pct / 100.0f));
        plotHeader_.setFillAlpha01 (patternComp_.getDrawFillAlpha());
        return TerminalResult::ok ("Opacity set to " + juce::String ((int) std::lround (pct)) + "%.");
    }

    if (verb == "addmic")
    {
        if (args.trim().isEmpty())
        {
            patternComp_.setAddSpeakerArmed (false);
            patternComp_.setAddMicArmed (true);
            applyPlotTool (RadiationPatternComponent::Tool::Select, false, false);
            preferTerminalFocus();
            return TerminalResult::ok ("ADDMIC — click the plot to place a mic (Esc cancels).");
        }
        juce::Point<float> pt;
        if (! parseAnnotPoint (args, pt))
            return TerminalResult::fail ("Usage: MIC  or  MIC x,y");
        if (! patternComp_.placeMicAtWorld (pt.x, pt.y))
            return TerminalResult::fail ("Could not place mic.");
        return TerminalResult::ok ("Mic placed.");
    }
    if (verb == "addspeaker")
    {
        if (args.trim().isEmpty())
        {
            patternComp_.setAddMicArmed (false);
            patternComp_.setAddSpeakerArmed (true);
            applyPlotTool (RadiationPatternComponent::Tool::Select, false, false);
            preferTerminalFocus();
            return TerminalResult::ok ("ADDSPEAKER — click the plot to place a speaker (Esc cancels).");
        }
        juce::Point<float> pt;
        if (! parseAnnotPoint (args, pt))
            return TerminalResult::fail ("Usage: SPK  or  SPK x,y");
        if (! controlPanel_.addSpeakerAt (pt.x, pt.y))
            return TerminalResult::fail ("Limit reached: "
                                         + juce::String (ControlPanel::kMaxSpeakers)
                                         + " Q21S units max.");
        return TerminalResult::ok ("Speaker placed.");
    }

    if (verb == "line" || verb == "pline" || verb == "circle" || verb == "arc"
        || verb == "rectang" || verb == "square" || verb == "text"
        || verb == "dist" || verb == "ruler"
        || verb == "pencil" || verb == "eraser")
    {
        return armDrawCommand (verb);
    }

    return TerminalResult::fail ("Unknown command \"" + verb.toUpperCase() + "\".");
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
    writeProjectToFile (project_.file, false);
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
            writeProjectToFile (f, false);
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
        plotHeader_.refreshUnits();
        patternComp_.refreshUnits();
        updateSettingsBar();
    }

    applyGridPref();   // grid show/hide may have changed
    // Autosave on/off may have changed via header toggle.
    {
        const bool on = AppSettings::get().autosaveEnabled();
        toggleAutosave_.setToggleState (on, juce::dontSendNotification);
        if (on)
        {
            if (! autoSaveTimer_.isTimerRunning())
                autoSaveTimer_.startTimer (kAutosaveTickMs);
        }
        else
            autoSaveTimer_.stopTimer();
    }
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
    toggleAutosave_.setColour (juce::ToggleButton::textColourId, Brand::text());
    toggleAutosave_.setColour (juce::ToggleButton::tickColourId, Brand::accent());

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
    if (terminalFloat_ != nullptr)
        terminalFloat_->lookAndFeelChanged();
    syncTerminalDockChrome();
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
    const int headerH = Brand::UI::headerBandH;
    const int bottomH = Brand::UI::bottomPanelH;
    const int sideW = effectiveSidebarWidth();
    const int plotX = sideW;
    const int centreW = juce::jmax (0, W - plotX);
    const int bodyTop2 = headerH + Brand::UI::plotHeaderH;
    const int bottomTop = H - bottomH;

    // Figma: flat regions, flush to each other — no rounded cards and no gaps.
    // The sidebar is a light-grey tray; the bottom strip stays white.
    g.setColour (Brand::sidebarBg());
    if (sideW > 0)
        g.fillRect (0, bodyTop2, sideW, juce::jmax (0, H - bodyTop2));

    if (bottomH > 0 && centreW > 0)
    {
        g.setColour (Brand::panel());
        g.fillRect (plotX, bottomTop, centreW, bottomH);
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

    const int headerH = Brand::UI::headerBandH;
    const int bottomH = Brand::UI::bottomPanelH;
    const int sideW = effectiveSidebarWidth();
    const int plotX = sideW;
    const int bodyTop2 = headerH + Brand::UI::plotHeaderH;
    const int bottomTop = H - bottomH;

    // Figma separates regions with hairline rules, not card outlines:
    //   • under the title row     • sidebar's right edge (full height)
    //   • under the canvas (top of the bottom strip)
    // The ribbon draws its own bottom rule in PlotHeaderBar::paint().
    g.setColour (Brand::border().withAlpha (0.45f));
    g.drawHorizontalLine (headerH - 1, 0.0f, (float) W);

    if (sideW > 0)
        g.drawVerticalLine (sideW - 1, (float) bodyTop2, (float) H);

    if (bottomH > 0)
        g.drawHorizontalLine (bottomTop, (float) plotX, (float) W);
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

    // Figma's regions are flush edge-to-edge, separated by hairlines rather
    // than floated as inset rounded cards: sidebar x=0..340, canvas x=340..1920,
    // both starting immediately under the 132px header with no gap.
    const int sideW = effectiveSidebarWidth();
    const int sideX = 0;
    const int plotX = sideW;
    const int centreW = juce::jmax (280, W - plotX);

    // Header row 1 (Figma redesign): logo + AutoSave on the left, title
    // centred — nothing on the right. Stats/Project/More have no Figma slot;
    // hidden but fully intact in code (see reportStatus / showStatsPopup /
    // showProjectMenu / showOverflowMenu). Info/Help/Settings AND the
    // "Ready" pill all live in row 2's ribbon now (see setHelpIcons /
    // setReadyPill in the constructor) — matching the Figma mock, where
    // Ready sits in the same row as the tool icons, not the title row.
    const int rightPad = UiConfig::Scale::px (12);
    // Figma centres the 20px "Auto Save" checkbox on y=28.5 inside the 58px
    // row; the button is headerIconH tall, so its top sits half that above.
    const int headerBtnTop = UiConfig::Scale::px (6);

    for (juce::Component* c : { (juce::Component*) &btnStats_, (juce::Component*) &btnProject_,
                                 (juce::Component*) &btnMore_ })
    {
        c->setVisible (false);
        c->setBounds (0, 0, 0, 0);
    }

    {
        // Figma row 1 (node 39-2): logo at x=20, the "Auto Save" checkbox at
        // x=143 with its label at x=173, and the document title centred on the
        // full window width (Figma's title box is x=815..1104, midpoint 959.5
        // against a canvas centre of 960). Nothing else lives in this row.
        //
        // These are absolute Figma positions rather than a left-to-right flow:
        // a flow accumulates each element's padding and drifts right, which is
        // exactly what pushed the ribbon clusters out of place before.
        toggleAutosave_.setBounds (UiConfig::Scale::px (UiConfig::Layout::headerAutoSaveX),
                                   headerBtnTop,
                                   UiConfig::Scale::px (UiConfig::Layout::headerAutoSaveW),
                                   Brand::UI::headerIconH);

        // Figma has no version chip in the title row. Hidden, not removed --
        // versionLabel_ still carries the build string for the Help/About box.
        versionLabel_.setVisible (false);
        versionLabel_.setBounds (0, 0, 0, 0);

        const int autosaveRight = toggleAutosave_.getRight() + UiConfig::Scale::px (12);
        const int regionR = W - rightPad;

        // Figma renders the document title in the Regular weight, not SemiBold.
        juce::Font titleFont = Brand::tech (Brand::UI::scaledFont (Brand::Type::appTitle));
        float fontH = titleFont.getHeight();

        auto glyphW = [] (const juce::Font& font, const juce::String& text) -> int
        {
            return juce::roundToInt (font.getStringWidthFloat (text) + 8.0f);
        };

        const juce::String titleText = titleLabel_.getText();
        int tw = glyphW (titleFont, titleText);

        // Shrink only if the title would collide with AutoSave or the right pad.
        const int regionW = juce::jmax (1, regionR - autosaveRight);
        while (fontH > UiConfig::Laf::titleShrinkMin && tw > regionW)
        {
            fontH -= 0.5f;
            titleFont = titleFont.withHeight (fontH);
            tw = glyphW (titleFont, titleText);
        }
        titleLabel_.setFont (titleFont);
        tw = juce::jmin (tw, regionW);

        // Centred on the window, nudged right only if AutoSave is in the way.
        int titleX = juce::roundToInt (0.5f * (float) (W - tw));
        titleX = juce::jlimit (autosaveRight, juce::jmax (autosaveRight, regionR - tw), titleX);

        const int titleTop = UiConfig::Scale::px (6);
        const int labelH   = juce::jmax (16, titleH - UiConfig::Scale::px (8));
        titleLabel_.setBounds (titleX, titleTop, tw, labelH);
    }

    // Param stats live in Help; keep bar out of the layout.
    juce::ignoreUnused (paramH);
    paramBar_.setBounds (0, 0, 0, 0);

    // Figma redesign: the tool ribbon (plotHeader_) spans the FULL body width —
    // above both the sidebar and the canvas, like one continuous two-row header —
    // instead of sitting only above the canvas. Sidebar + canvas share a lower
    // top, below that full-width strip.
    const int bodyTop        = titleH;            // ribbon sits flush under the title row
    const int toolbarStripH  = plotHdrH;
    const int bodyTop2       = bodyTop + toolbarStripH;   // flush, no gap (Figma y=132)
    const int bottomTop  = H - bottomH;
    const int bodyH      = juce::jmax (80, bottomTop - bodyTop2);

    // Sidebar runs the full height from the header down to the window bottom
    // (Figma y=132..1080) — the bottom strip only spans the canvas column.
    const int sidebarBottom = H;
    const int sidebarH = juce::jmax (80, sidebarBottom - bodyTop2);
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
                                     bodyTop2 + burgerPad,
                                     burger, burger);
    }
    else
    {
        controlViewport_.setVisible (true);
        controlViewport_.setScrollBarThickness (UiConfig::Scale::px (8));
        // Content starts at the very top of the panel so section 1 sits level
        // with the hamburger, which overlays the top-right corner — the old
        // reserved strip just left a band of empty grey above "1. FREQUENCY".
        const int panelH = juce::jmax (40, sidebarH);
        controlViewport_.setBounds (sideX, bodyTop2, sideW, panelH);
        const int innerW = sideW - controlViewport_.getScrollBarThickness();
        controlPanel_.setSize (innerW, panelH);
        if (controlPanel_.getContentHeight() > panelH)
            controlPanel_.setSize (innerW, controlPanel_.getContentHeight());

        btnSidebarToggle_.setBounds (sideX + sideW - burger - burgerPad,
                                     bodyTop2 + burgerPad,
                                     burger, burger);
    }
    btnSidebarToggle_.toFront (false);

    // Tool ribbon — Figma draws it full-bleed: edge to edge, no side margin
    // and no rounded card, closed by a hairline along its bottom edge (drawn
    // in PlotHeaderBar::paint). Info/Settings/Help are this ribbon's own
    // "Help" cluster (see PlotHeaderBar::setHelpIcons / resized()).
    plotHeader_.setBounds (0, titleH, W, toolbarStripH);

    patternComp_.setBounds (plotX, bodyTop2, centreW, bodyH);

    // Red "SPL Heatmap | ..." caption: canvas top-left, as in the Figma mock
    // (it used to sit inside the ribbon and push every cluster to the right).
    {
        // Figma: caption at x=357, y=147 — 17px in from the canvas's left edge
        // and 15px below its top.
        auto& caption = plotHeader_.getTitleLabel();
        const int capPadX = UiConfig::Scale::px (13);
        const int capPadY = UiConfig::Scale::px (7);
        const int capH    = UiConfig::Scale::px (16);
        // Font lives here now that the caption is a MainComponent child, so it
        // keeps rescaling with the window. Semibold keeps the brand red legible
        // over the dark heatmap (Figma's mock canvas is empty/light).
        caption.setFont (Brand::techSemi (Brand::UI::scaledFont (Brand::Type::panelTitle)));
        // Hard against the canvas's left edge. It used to be inset to the
        // field's own left edge, which mattered while the view letterboxed a
        // square world and left pale margins; now the solved region fills the
        // canvas, that inset only pushed the caption toward the middle.
        caption.setBounds (plotX + capPadX, bodyTop2 + capPadY,
                           juce::jmax (0, centreW - capPadX * 2), capH);
        caption.toFront (false);
        // PlotHeaderBar::lookAndFeelChanged() re-stamps the label red, so put
        // the contrast-aware colour back on every layout pass.
        refreshCaptionColour();
    }

    // Bottom strip (Figma y=979..1080, x=340..1920): "Last run" / "Elapsed"
    // stacked at the left, SAVE IMAGE (PNG) + EXPORT SPL (CSV) at the right.
    // View Mode and Terminal have no slot in the mock — hidden, code intact.
    exportHeader_.setVisible (false);
    exportHeader_.setBounds (0, 0, 0, 0);

    for (juce::Component* c : { (juce::Component*) &viewHeader_, (juce::Component*) &btnViewSPL_,
                                 (juce::Component*) &btnViewDirectivity_, (juce::Component*) &btnViewMeasured_ })
    {
        c->setVisible (false);
        c->setBounds (0, 0, 0, 0);
    }

    for (juce::Component* c : { (juce::Component*) &terminalHeader_, (juce::Component*) &btnTerminalDock_,
                                 (juce::Component*) &commandTerminal_ })
    {
        c->setVisible (false);
        c->setBounds (0, 0, 0, 0);
    }

    {
        namespace L = UiConfig::Layout;
        const int btnW    = UiConfig::Scale::px (L::bottomButtonWidth);
        const int btnH    = UiConfig::Scale::px (L::bottomButtonHeight);
        const int btnGap  = UiConfig::Scale::px (L::bottomButtonGap);
        const int rightPd = UiConfig::Scale::px (L::bottomButtonRightPad);
        // Centre the row's contents in the strip rather than hanging them from
        // its top edge: both the run-info block and these buttons sat high,
        // leaving all the slack underneath.
        const int btnTop  = bottomTop + (bottomH - btnH) / 2;

        int erx = W - rightPd - btnW;
        btnExportCSV_.setBounds (erx, btnTop, btnW, btnH);
        erx -= btnGap + btnW;
        btnExportPNG_.setBounds (erx, btnTop, btnW, btnH);

        const int infoX = plotX + UiConfig::Scale::px (L::bottomRunInfoLeft);
        const int infoH = UiConfig::Scale::px (L::bottomRunInfoLineGap) * 2;
        const int infoY = bottomTop + (bottomH - infoH) / 2;
        statusStrip_.setBounds (infoX, infoY, juce::jmax (0, erx - infoX), infoH);
    }
    layoutPrefsPanel();
    layoutInfoPanel();
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
    reportStatus ("Computing...", false);
    startThread();
}

void MainComponent::run()
{
    const double t0 = juce::Time::getMillisecondCounterHiRes();
    SimParams p = controlPanel_.getParams();
    p.viewMode  = currentView_;
    {
        juce::ScopedLock sl (measLock_);
        p.directivity       = directivityQ21STables_;
        p.directivityBEM2inch = directivityBEM2inchTables_;
        p.bemFields         = bemFieldTablesQ21S_;
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
    // Not a blanket "Ready": every edit triggers a recompute, so hard-coding
    // it here overwrote the "Unsaved changes" that markProjectDirty had just
    // set, milliseconds earlier. The pill then always read "Ready" no matter
    // how much unsaved work was pending. Report the actual save state.
    updateSaveIndicator();
    // Figma's bottom strip spells these "Last run : 11 JUL 2026 14:52:31" and
    // "Elapsed : 1.5s" — spaced colon, uppercase month, no space before "s".
    statusStrip_.setLastRun ("Last run : "
        + juce::Time::getCurrentTime().formatted ("%d %b %Y %H:%M:%S").toUpperCase());
    statusStrip_.setElapsed ("Elapsed : " + juce::String (lastElapsedSec_, 1) + "s");
}

void MainComponent::syncRenderer()
{
    patternComp_.setSpeakers (controlPanel_.getSpeakers(),
                              controlPanel_.getSelectedIndex());
}

void MainComponent::updatePlotChrome()
{
    const auto& p = lastParams_;
    const char* vmName = (currentView_ == ViewMode::SPL)         ? "SPL Gradient Plot"
                       : (currentView_ == ViewMode::Directivity) ? "Directivity"
                                                               : "Measured Polar";

    juce::String title = vmName;
    title += "  |  " + juce::String (lastResult_.activeSpeakers) + " devices";
    title += "  |  " + juce::String ((int) p.frequency) + " Hz";
    if (currentView_ == ViewMode::MeasuredPolar)
    {
        title += "  |  Measured @ " + Units::metres ((double) measDistanceM_, 1);
    }
    else if (lastResult_.usedMeasuredDirectivity)
    {
        const float simDist = MeasurementData::farFieldDirectivityDistance (measured_, measDistanceM_);
        title += "  |  Measured @ " + Units::metres ((double) simDist, 1);
    }
    plotHeader_.setTitle (title);
    refreshCaptionColour();
}

void MainComponent::refreshCaptionColour()
{
    // With no devices the canvas is the pale empty grid and the brand red
    // reads well on it. As soon as a speaker is placed the field renders
    // underneath -- near-black at the dB floor -- and red on that is as good
    // as invisible, so the caption switches to white.
    const bool overField = lastResult_.activeSpeakers > 0;
    plotHeader_.getTitleLabel().setColour (juce::Label::textColourId,
                                           overField ? Brand::white()
                                                     : Brand::plotTitle());
}

void MainComponent::applyPlotTool (RadiationPatternComponent::Tool tool,
                                   bool openColourPicker,
                                   bool focusPlot)
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
    if (focusPlot)
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

void MainComponent::refreshTitleLabel()
{
    const auto name = project_.displayName();
    const juce::String text = name.isNotEmpty()
                                ? "Atomik Simulation Engine - " + name
                                : juce::String ("Atomik Simulation Engine");
    if (titleLabel_.getText() != text)
    {
        titleLabel_.setText (text, juce::dontSendNotification);
        resized();   // the title is centre-anchored, so its width drives its x
    }
}

void MainComponent::updateSettingsBar()
{
    refreshTitleLabel();
    const auto& p = lastParams_;
    juce::StringArray chips;

    if (currentView_ == ViewMode::MeasuredPolar)
    {
        const int hz = (int) (controlPanel_.getParams().frequency + 0.5);
        chips.add ("f = " + juce::String (hz) + " Hz");
        chips.add (MeasurementData::sourceName (measSource_));
        chips.add (Units::metres ((double) measDistanceM_, 1));
        chips.add ("View: Measured Polar");
    }
    else
    {
        const char* vm = (currentView_ == ViewMode::SPL) ? "SPL Gradient Plot" : "Directivity";
        chips.add ("f = " + juce::String ((int) p.frequency) + " Hz");
        chips.add ("lambda = " + juce::String (Units::metresToDisplay (lastResult_.lambda), 2)
                     + " " + Units::lengthUnit());
        chips.add ("Units = " + juce::String (lastResult_.activeSpeakers)
                     + " / " + juce::String ((int) p.speakers.size()));
        chips.add ("View: " + juce::String (vm));
        chips.add ("Grid: " + juce::String (p.resolution) + " x " + juce::String (p.resolution));
        if (lastResult_.usedMeasuredDirectivity)
        {
            const float simDist = MeasurementData::farFieldDirectivityDistance (measured_, measDistanceM_);
            juce::String m = "Measured @ " + Units::metres ((double) simDist, 1);
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

    // Help / Info badges: bake disk/mark colours into SVG so the idle fill always shows
    // (replaceColour on SVG #fff can leave a white disk = invisible on light header).
    auto makeBadge = [&] (const juce::String& markPath, juce::Colour disk, juce::Colour mark)
        -> std::unique_ptr<juce::Drawable>
    {
        const auto svg = juce::String()
            + R"SVG(<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24"><circle cx="12" cy="12" r="11" fill="#)SVG"
            + hexRgb (disk)
            + R"SVG("/><path fill="#)SVG"
            + hexRgb (mark)
            + R"SVG(" d=")SVG"
            + markPath
            + R"SVG("/></svg>)SVG";
        if (auto xml = juce::parseXML (svg))
            return juce::Drawable::createFromSVG (*xml);
        return {};
    };

    const bool dark = AppSettings::get().isDark();
    const auto ink    = dark ? Brand::white() : Brand::text();
    const auto inkHi  = Brand::accent();
    // Always-visible badge disk: charcoal on light header, white on dark.
    const auto badgeDisk = dark ? Brand::white() : juce::Colour (0xff333131);
    const auto badgeMark = dark ? Brand::charcoal() : Brand::white();
    static const juce::String kHelpMarkPath =
        "M10.2 8.6c.35-1.15 1.3-1.9 2.7-1.9 1.55 0 2.65.95 2.65 2.35 0 .95-.45 1.55-1.35 2.15-.85.55-1.15.95-1.15 1.7v.45h-1.55v-.55c0-1.15.4-1.7 1.3-2.3.7-.45 1-0.85 1-1.4 0-.7-.55-1.15-1.35-1.15-.8 0-1.35.45-1.55 1.2l-1.7-.4zm1.95 7.55c.65 0 1.15-.5 1.15-1.15s-.5-1.15-1.15-1.15-1.15.5-1.15 1.15.5 1.15 1.15 1.15z";
    static const juce::String kInfoMarkPath =
        "M11.1 10.2h1.8v7.1h-1.8zm0-3.9h1.8V8h-1.8z";

    auto style = [&] (juce::DrawableButton& b, const char* svg, bool twoTone)
    {
        b.setEdgeIndent (Brand::UI::headerIconIndent);
        const auto c0 = colourise (svg, ink,   twoTone ? Brand::white() : ink);
        const auto c1 = colourise (svg, inkHi, twoTone ? Brand::white() : inkHi);
        b.setImages (c0.get(), c1.get(), c1.get());
        b.setColour (juce::DrawableButton::backgroundColourId,   juce::Colours::transparentBlack);
        b.setColour (juce::DrawableButton::backgroundOnColourId, Brand::btnIn().withAlpha (0.35f));
    };

    auto styleBadge = [&] (juce::DrawableButton& b, const juce::String& markPath)
    {
        b.setEdgeIndent (Brand::UI::headerIconIndent);
        const auto c0 = makeBadge (markPath, badgeDisk, badgeMark);
        const auto c1 = makeBadge (markPath, Brand::accent(), Brand::white());
        b.setImages (c0.get(), c1.get(), c1.get());
        b.setColour (juce::DrawableButton::backgroundColourId,   juce::Colours::transparentBlack);
        b.setColour (juce::DrawableButton::backgroundOnColourId, Brand::btnIn().withAlpha (0.35f));
    };

    // Help cluster: use the Figma-exported glyphs when they're on disk, so the
    // gear / info / "?" match the mock exactly; fall back to the drawn badges.
    auto styleFromFile = [] (juce::DrawableButton& b, const juce::String& iconName) -> bool
    {
        // Baked-in bytes first, so these survive a bare-EXE download.
        const auto mb = Brand::assetBytes ("ToolIcons/" + iconName + ".png");
        if (mb.getSize() == 0) return false;
        const auto img = juce::ImageFileFormat::loadFrom (mb.getData(), mb.getSize());
        if (! img.isValid()) return false;
        juce::DrawableImage d;
        d.setImage (img);
        b.setImages (&d);
        b.setEdgeIndent (Brand::UI::headerIconIndent);
        b.setColour (juce::DrawableButton::backgroundColourId,   juce::Colours::transparentBlack);
        b.setColour (juce::DrawableButton::backgroundOnColourId, Brand::btnIn().withAlpha (0.35f));
        return true;
    };

    if (! styleFromFile (btnInfo_, "Info"))      styleBadge (btnInfo_, kInfoMarkPath);
    if (! styleFromFile (btnHelp_, "Help"))      styleBadge (btnHelp_, kHelpMarkPath);
    if (! styleFromFile (btnPrefsIcon_, "Settings")) style (btnPrefsIcon_, HeaderIcons::kGear, false);
    style (btnMore_,      HeaderIcons::kMenu, false);

    // The gear / info / "?" live in the ribbon's Help cluster, so the ribbon --
    // not this header styling -- owns their sizing. Without this they shrink
    // every time the units or theme change.
    plotHeader_.refreshHelpIconMetrics();
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
        base.directivity       = directivityQ21STables_;
        base.directivityBEM2inch = directivityBEM2inchTables_;
        base.bemFields         = bemFieldTablesQ21S_;
    }

    // The sweep walks the currently-browsed model's own catalogue only; that
    // model's speakers must be probed at each swept Hz, so its own resolved
    // frequency field is updated alongside p.frequency — the OTHER model's
    // field (and therefore its speakers' rendered pattern) stays untouched.
    const int browsedModel = controlPanel_.getBrowsedModel();

    std::vector<std::vector<float>> curves ((size_t) mics.size());
    for (size_t mi = 0; mi < mics.size(); ++mi)
    {
        curves[mi].resize ((size_t) kNumSupportedFrequencies, 0.0f);
        for (int hi = 0; hi < kNumSupportedFrequencies; ++hi)
        {
            SimParams p = base;
            p.frequency = kSupportedFrequencies[hi];
            if (browsedModel == MeasurementData::BEM2in) p.frequencyBEM2inch = p.frequency;
            else                                         p.frequencyQ21S   = p.frequency;
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
    m.addItem (4, "Save Project\tCtrl+S", true, false);
    m.addItem (5, "Save Project As...", true, false);
    m.addSeparator();
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
            else if (r == 4) saveProject();
            else if (r == 5) saveProjectAs();
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
        reportStatus ("Could not open: " + f.getFileName(), false);
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
    projectDirty_ = false;
    lastAutosaveMs_ = 0;
    refreshFrequencyResponse();
    resized();

    AppSettings::get().addRecentProject (project_.file);
    updateSettingsBar();
    updatePlotChrome();
    scheduleRecompute();
    reportStatus ("Opened: " + project_.displayName(), true);

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
    if (! hasResult_) { reportStatus ("Nothing to export.", false); return; }

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
                reportStatus ("Saved: " + f.getFileName(), true);
            }
        });
}

void MainComponent::exportPdfReport()
{
    if (! hasResult_) { reportStatus ("Run a simulation before exporting a report.", false); return; }

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

            reportStatus ("Generating PDF report...", false);
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
    { juce::ScopedLock sl (measLock_);
      base.directivity = directivityQ21STables_; base.directivityBEM2inch = directivityBEM2inchTables_;
      base.bemFields = bemFieldTablesQ21S_; }
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
        reportStatus ("Report exported: " + f.getFileName(), true);
    }
    else
        reportStatus ("Could not write PDF report.", false);
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
                    reportStatus ("Could not read DXF: " + f.getFileName(), false);
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
                    reportStatus ("Could not read image: " + f.getFileName(), false);
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
            reportStatus ("Layout imported: " + f.getFileName(), true);
        });
}

void MainComponent::removeLayout()
{
    layout_.clear();
    controlPanel_.refreshLayoutControls();
    patternComp_.repaint();
    reportStatus ("Layout removed.", true);
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
    // freq is drawn from measured_.freqs, i.e. measSource_'s own catalogue —
    // update only that model's resolved frequency so the other model's
    // speakers keep rendering at their own unrelated frequency.
    if (measSource_ == MeasurementData::BEM2in) p.frequencyBEM2inch = freq;
    else                                        p.frequencyQ21S   = freq;
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
    if (! hasResult_) { reportStatus ("Nothing to export.", false); return; }

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
            int nQ21S = 0, nBEM2inch = 0;
            for (const auto& s : pr.speakers)
                if (s.enabled) { ++nDev; if (s.model == 2) ++nBEM2inch; else ++nQ21S; }

            const bool absOk = r.hasAbsoluteSpl
                && r.splAbsDB.size() == (size_t) W * (size_t) H;
            const bool relOk = r.splRelDB.size() == (size_t) W * (size_t) H;

            auto line = [&] (const juce::String& s)
            {
                fos.writeText (s + "\n", false, false, nullptr);
            };

            juce::String product;
            if (nQ21S > 0)   product << "Q21S(" << nQ21S << ")";
            if (nBEM2inch > 0) product << (product.isEmpty() ? "" : "+") << "BEM2inch(" << nBEM2inch << ")";
            if (product.isEmpty()) product = "Q21S";

            line ("# Atomik Simulation Engine v1.4.0.5");
            line ("# Product," + product);
            line ("# www.atomikaudio.com");
            line ("# Generated," + now.formatted ("%d %b %Y") + "," + now.formatted ("%H:%M:%S"));
            line ("# Frequency_Hz," + juce::String (r.frequency, 1));
            line ("# Units," + juce::String (nDev));
            line ("# World_m," + juce::String (r.worldW, 3) + " x " + juce::String (r.worldH, 3));
            // The solved region moves with the view, so record where it starts.
            line ("# Origin_m," + juce::String (r.worldX0, 3) + " , " + juce::String (r.worldY0, 3));
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
                // Offset by the solved region's origin: the region is no longer
                // anchored at (0, 0), so exported coordinates have to say where
                // the samples actually are.
                const double y = r.worldY0 + row * dy;
                for (int col = 0; col < W; ++col)
                {
                    const size_t i = (size_t) row * (size_t) W + (size_t) col;
                    const float rel = relOk ? r.splRelDB[i]
                                            : (absOk ? (r.splAbsDB[i] - (float) r.peakAbsDb)
                                                     : r.splDB[i]);
                    const float spl = absOk ? r.splAbsDB[i] : rel;
                    mos << juce::String (r.worldX0 + col * dx, 4) << ","
                        << juce::String (y, 4) << ","
                        << juce::String (rel, 2) << ","
                        << juce::String (spl, 2) << "\n";
                }
            }
            fos.write (mos.getData(), mos.getDataSize());
            reportStatus ("Saved: " + f.getFileName(), true);
        });
}

// ---------------------------------------------------------------------------
// Measured polar data: load + live auto-refresh
// ---------------------------------------------------------------------------
MeasuredSet MainComponent::referenceSetFor (int source) const
{
    if (source == MeasurementData::BEM2in)      return measuredBEM2inch_;
    if (source == MeasurementData::OpenField) return measuredQ21S_;
    // Legacy Room — not part of the always-loaded pair (no Speaker::model
    // ever selects it); load on demand only if explicitly chosen.
    return MeasurementData::loadMeasurements (MeasurementData::folderForSource (source), source);
}

// Q21S and BEM2inch are always both (re)loaded together — a scene can mix
// units of either, so neither model's data may depend on which one the
// "Measurement set" reference view (section 4) currently shows.
void MainComponent::reloadAllMeasurements()
{
    MeasuredSet mQ21S = MeasurementData::loadMeasurements (
        MeasurementData::folderForSource (MeasurementData::OpenField), MeasurementData::OpenField);
    MeasuredSet mBEM2inch = MeasurementData::loadMeasurements (
        MeasurementData::folderForSource (MeasurementData::BEM2in), MeasurementData::BEM2in);

    // Heatmap / array sim: far-field BEM arc (not 0.5 m near-field lobes),
    // computed independently per model.
    const float distQ21S   = MeasurementData::farFieldDirectivityDistance (mQ21S,   1.0f);
    const float distBEM2inch = MeasurementData::farFieldDirectivityDistance (mBEM2inch, 1.0f);

    auto dirQ21S   = MeasurementData::buildDirectivityTables (mQ21S,   distQ21S);
    auto dirBEM2inch = MeasurementData::buildDirectivityTables (mBEM2inch, distBEM2inch);
    auto bemQ21S   = MeasurementData::loadBemFieldTables (mQ21S);
    auto bemBEM2inch = MeasurementData::loadBemFieldTables (mBEM2inch);

    {
        juce::ScopedLock sl (measLock_);
        measuredQ21S_            = std::move (mQ21S);
        measuredBEM2inch_          = std::move (mBEM2inch);
        directivityQ21STables_   = std::move (dirQ21S);
        directivityBEM2inchTables_ = std::move (dirBEM2inch);
        bemFieldTablesQ21S_      = std::move (bemQ21S);
        bemFieldTablesBEM2inch_    = std::move (bemBEM2inch);

        measured_ = referenceSetFor (measSource_);
    }

    measDir_       = MeasurementData::folderForSource (measSource_);
    measSignature_ = measurementsSignature();

    const auto dists = MeasurementData::availableDistances (measured_);
    controlPanel_.setAvailableDistances (dists, 1.0f);
    measDistanceM_ = controlPanel_.getMeasurementDistance();

    patternComp_.setMeasuredDistance (measDistanceM_);
    patternComp_.setMeasuredData (measured_);
    patternComp_.setMeasuredFrequency ((int) (controlPanel_.getParams().frequency + 0.5));
    updateSettingsBar();
}

void MainComponent::setMeasurementDistance (float distanceM)
{
    if (std::abs (measDistanceM_ - distanceM) < 1.0e-4f) return;
    measDistanceM_ = distanceM;
    patternComp_.setMeasuredDistance (measDistanceM_);
    patternComp_.setMeasuredData (measured_);

    reportStatus ("Polar distance: " + Units::metres ((double) measDistanceM_, 1), true);
    updateSettingsBar();

    // UI distance drives Measured Polar only. SPL prediction keeps the far-field
    // (≈2 m) pattern — no need to rebuild engine tables or recompute.
    if (currentView_ == ViewMode::MeasuredPolar)
        patternComp_.repaint();
}

// Section 4's "Measurement set" now only selects which device the Measured
// Polar reference view shows. It no longer gates the engine — every placed
// speaker already simulates with its own model's directivity (both are
// always loaded), so switching this never needs a heatmap recompute.
void MainComponent::setMeasurementSource (int src)
{
    src = juce::jlimit (0, 2, src);
    if (src == measSource_) return;

    measSource_ = src;
    AppSettings::get().setMeasurementSource (src);

    {
        juce::ScopedLock sl (measLock_);
        measured_ = referenceSetFor (src);
    }
    measDir_ = MeasurementData::folderForSource (src);

    const auto dists = MeasurementData::availableDistances (measured_);
    controlPanel_.setAvailableDistances (dists, 1.0f);
    measDistanceM_ = controlPanel_.getMeasurementDistance();

    patternComp_.setMeasuredDistance (measDistanceM_);
    patternComp_.setMeasuredData (measured_);

    reportStatus (juce::String ("Measurement set: ")
                        + MeasurementData::sourceName (src), true);
    updateSettingsBar();

    if (currentView_ == ViewMode::MeasuredPolar)
        patternComp_.repaint();
}

juce::int64 MainComponent::measurementsSignature() const
{
    // Combined fingerprint across BOTH always-loaded devices (plus legacy
    // Room only if that happens to be the reference selection), so a live
    // edit to either CSV pack is caught regardless of what's on screen.
    juce::int64 sig = 0;
    const juce::File pack = MeasurementData::packDataFolder();
    const float dists[] = { 0.5f, 1.0f, 2.0f };

    auto addModel = [&] (int source)
    {
        const juce::File xlsx = MeasurementData::xlsxFolderForSource (source);
        const juce::String setName = MeasurementData::packSetName (source);
        std::vector<int> freqList;
        if (source == MeasurementData::Gylt)
            freqList = { 30, 80, 200, 500 };
        else
        {
            const double* freqs; int n;
            frequencyCatalogue (source, freqs, n);
            for (int i = 0; i < n; ++i)
                freqList.push_back ((int) std::lround (freqs[i]));
        }
        for (int hz : freqList)
            for (float d : dists)
            {
                const juce::File csv = pack.getChildFile (
                    MeasurementData::csvFileName (setName, hz, d));
                if (csv.existsAsFile())
                    sig += csv.getLastModificationTime().toMilliseconds() + csv.getSize();

                const juce::String distTag = (std::abs (d - 0.5f) < 1.0e-3f) ? "0.5"
                                          : (std::abs (d - 2.0f) < 1.0e-3f) ? "2" : "1";
                const juce::File xf = MeasurementData::fileFor (xlsx, hz, distTag);
                if (xf.existsAsFile())
                    sig += xf.getLastModificationTime().toMilliseconds() + xf.getSize();
            }
    };

    addModel (MeasurementData::OpenField);
    addModel (MeasurementData::BEM2in);
    if (measSource_ == MeasurementData::Gylt) addModel (MeasurementData::Gylt);
    return sig;
}

void MainComponent::pollMeasurements()
{
    const juce::int64 sig = measurementsSignature();
    if (sig == measSignature_) return;   // unchanged

    reloadAllMeasurements();
    reportStatus ("Measurements refreshed: "
                        + juce::Time::getCurrentTime().formatted ("%H:%M:%S"), true);
    if (currentView_ == ViewMode::MeasuredPolar)
        patternComp_.repaint();
    else
        scheduleRecompute();   // field views: re-run so heat map reflects new directivity
}

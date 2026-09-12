#pragma once
#include <JuceHeader.h>
#include "AcousticEngine.h"
#include "ControlPanel.h"
#include "RadiationPatternComponent.h"
#include "InfoPanel.h"
#include "MeasurementData.h"
#include "BrandTheme.h"
#include "AppSettings.h"
#include "ProjectData.h"
#include "LayoutLayer.h"
#include "UiChrome.h"
#include "MicReceiver.h"
#include "FrequencyResponseComponent.h"
#include "MicRefLockDialog.h"
#include "CommandTerminal.h"

// ---------------------------------------------------------------------------
// MainComponent — owns all sub-panels, runs AcousticEngine::compute on a
// background thread, and keeps the scene/renderer/info panels in sync.
// Edits are debounced via a Timer so dragging sliders or speakers stays
// responsive; switching view mode only re-colours the cached result.
// ---------------------------------------------------------------------------
class MainComponent : public juce::Component,
                      private juce::Thread,
                      private juce::Timer,
                      private juce::ChangeListener,
                      private juce::KeyListener
{
public:
    explicit MainComponent (ProjectData project = {});
    ~MainComponent() override;

    void paint              (juce::Graphics&) override;
    void paintOverChildren  (juce::Graphics&) override;
    void resized () override;
    void lookAndFeelChanged() override;                       // re-apply theme colours
    void parentHierarchyChanged() override;
    bool keyPressed (const juce::KeyPress&) override;                 // Component
    bool keyPressed (const juce::KeyPress&, juce::Component*) override; // KeyListener

private:
    void run() override;            // juce::Thread
    void timerCallback() override;  // juce::Timer (debounce)
    void changeListenerCallback (juce::ChangeBroadcaster*) override;  // AppSettings

    void openPreferences();
    void saveProject();
    void saveProjectAs();
    void markProjectDirty();
    void updateSaveIndicator();
    bool writeProjectToFile (const juce::File& f, bool quiet);
    void autosaveIfNeeded();
    juce::File autosaveFileForProject() const;
    ProjectData currentProject() const;   // metadata + live scene
    void exportPdfReport();
    void buildAndWriteReport (const juce::File& f);
    juce::Image renderHeatmapImage (double freq, const SimParams& base, double& coverageOut,
                                    double* peakAbsOut = nullptr, bool* hasAbsOut = nullptr);

    void scheduleRecompute();
    void runSimulation();
    void applyResult (const SimResult& r);
    void updateSettingsBar();
    // Header row 1's document title: "Atomik Simulation Engine - <project>",
    // mirroring the OS window title (Figma row 1 shows the same composite).
    void refreshTitleLabel();
    // The canvas caption's ink: brand red on the bare grid, white once a field
    // is rendered under it. Same contrast problem the axis numbers had.
    void refreshCaptionColour();
    void updatePlotChrome();
    void setViewMode (ViewMode mode);
    void highlightViewBtn (ViewMode mode);
    void syncRenderer();
    void showDrawColourPicker();
    void applyPlotTool (RadiationPatternComponent::Tool tool,
                        bool openColourPicker = false,
                        bool focusPlot = true);
    void preferTerminalFocus();
    void cancelCurrentCommand (bool focusPlot = true);
    // Mirrors to both statusStrip_ (bottom, full detail) and headerStatus_
    // (compact "Ready" pill, header row 1) so callers only need one call.
    void reportStatus (const juce::String& state, bool ready = true);

    struct EditSnapshot
    {
        ProjectData scene;
        std::vector<RadiationPatternComponent::Annotation> drawings;
        std::vector<MicReceiver> mics;
    };
    void willEdit();
    void commitEdit();
    void undoEdit();
    void redoEdit();
    bool handleEditShortcut (const juce::KeyPress&);
    void showKeyboardShortcuts();
    using TerminalResult = CommandTerminal::CommandResult;
    TerminalResult handleTerminalCommand (const juce::String& verb, const juce::String& args);
    TerminalResult handleTerminalSessionLine (const juce::String& line);
    TerminalResult armDrawCommand (const juce::String& verb);
    TerminalResult beginMoveCommand();
    TerminalResult beginZoomCommand (const juce::String& args);
    static bool parseAnnotPoint (const juce::String& text, juce::Point<float>& out);

    enum class TerminalSessionKind { None, Draw, Dist, MoveBase, MoveSecond, Zoom };
    TerminalSessionKind terminalSessionKind_ = TerminalSessionKind::None;
    juce::Point<float>  terminalMoveBase_ { 0, 0 };
    EditSnapshot takeEditSnapshot() const;
    void applyEditSnapshot (const EditSnapshot&);
    static juce::juce_wchar shortcutLetter (const juce::KeyPress&);

    void exportPNG();
    void exportCSV();

    void loadMeasurements();
    juce::int64 measurementsSignature() const;   // mtime/size fingerprint
    void pollMeasurements();

    // Workspace / layout import (Phase 5 & 6)
    void applyGridPref();
    void importLayout();
    void removeLayout();
    void applyLayoutSettings();

    void toggleSidebar();
    void syncSidebarToggleChrome();
    int  effectiveSidebarWidth() const;

    ControlPanel              controlPanel_;
    juce::Viewport            controlViewport_;
    juce::DrawableButton      btnSidebarToggle_ { "sidebarToggle", juce::DrawableButton::ImageFitted };
    PlotHeaderBar             plotHeader_;
    RadiationPatternComponent patternComp_;
    std::unique_ptr<MicFrequencyResponseWindow> frWindow_;
    int                       frRefMic_ = 0;

    void showMicPlaceOnRingDialog();
    void refreshFrequencyResponse();
    void ensureFrequencyResponseWindow();
    void showFrequencyResponseWindow();

    LayoutLayer layout_;

    std::unique_ptr<juce::Drawable> logo_;

    juce::Label titleLabel_, versionLabel_;
    ParamBar    paramBar_;
    StatusStrip statusStrip_;
    StatusStrip headerStatus_;   // compact "Ready" pill, top-right of header row 1 (Figma redesign)
    juce::TooltipWindow tooltipWindow_ { this, 450 };
    juce::StringArray statChips_;   // live simulation stats, shown in the Help (?) popup

    juce::Label exportHeader_, viewHeader_, terminalHeader_;
    juce::TextButton btnTerminalDock_ { "Undock" };
    void undockTerminal();
    void dockTerminal();
    void syncTerminalDockChrome();
    bool isTerminalDocked() const noexcept { return terminalFloat_ == nullptr; }
    std::unique_ptr<TerminalFloatWindow> terminalFloat_;

    juce::TextButton     btnStats_ { "Statistics" };
    juce::TextButton     btnProject_ { "Project" };
    juce::ToggleButton   toggleAutosave_ { "AutoSave" };
    juce::DrawableButton btnInfo_  { "info", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnHelp_  { "help", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnPrefsIcon_ { "prefs", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnMore_  { "more", juce::DrawableButton::ImageFitted };
    void showOverflowMenu();
    void showStatsPopup();
    void showProjectMenu();
    void launchNewProjectInstance();
    void openProjectInNewWindow();
    void openProjectInCurrentWindow();
    void loadProjectFile (const juce::File& f);
    static bool launchAppInstance (const juce::String& args = {});
    void refreshHeaderIcons();
    std::unique_ptr<class PreferencesComponent> prefsPanel_;
    // Help cluster's info / "?" popups. A panel rather than an AlertWindow:
    // the stock message box forces an icon disc and an OK button.
    std::unique_ptr<class InfoDialogComponent>  infoPanel_;
    void layoutPrefsPanel();
    void showInfoPanel (const juce::String& heading,
                        const juce::String& subtitle,
                        const juce::String& body);
    void layoutInfoPanel();

    ProjectData project_;

    juce::TextButton btnExportPNG_, btnExportCSV_;
    juce::TextButton btnViewSPL_, btnViewDirectivity_, btnViewMeasured_;
    CommandTerminal  commandTerminal_;
    void updateViewButtonHighlights();

    SimResult lastResult_;
    SimParams lastParams_;
    bool      hasResult_ = false;
    ViewMode  currentView_ = ViewMode::SPL;
    double    lastElapsedSec_ = 0.0;

    juce::CriticalSection resultLock_;
    std::unique_ptr<juce::FileChooser> fileChooser_;

    // Measured polar data + live auto-refresh -------------------------------
    MeasuredSet  measured_;
    juce::File   measDir_;
    juce::int64  measSignature_ = 0;
    int          measSource_ = 1;    // 0 = Q21S (Open Field), 1 = Room, 2 = 15W750
    float        measDistanceM_ = 0.5f;
    void         setMeasurementSource (int src, bool recompute = true);
    void         setMeasurementDistance (float distanceM);
    void         rebuildDirectivityTables();

    // Per-frequency directivity tables derived from measured_, fed to the engine.
    std::vector<DirectivityPattern> directivityTables_;
    std::vector<BemFieldPattern>    bemFieldTables_;
    juce::CriticalSection           measLock_;

    // Lightweight second timer for polling the measurement files (the class's
    // own juce::Timer base is reserved for the edit-debounce one-shot).
    struct LambdaTimer : public juce::Timer
    {
        std::function<void()> fn;
        void timerCallback() override { if (fn) fn(); }
    };
    LambdaTimer measPoll_;
    // Autosave cadence. The tick is how often a dirty project is checked; the
    // gap is the floor between two writes so a continuous drag cannot write on
    // every tick. Keep the gap <= the tick, or the two fight each other and
    // saves are delayed by their difference.
    static constexpr int         kAutosaveTickMs   = 2000;
    static constexpr juce::int64 kAutosaveMinGapMs = 2000;

    LambdaTimer autoSaveTimer_;
    bool        projectDirty_ = false;
    juce::int64 lastAutosaveMs_ = 0;

    static juce::Colour kBg() { return Brand::base(); }

    std::vector<EditSnapshot> undoStack_, redoStack_;
    EditSnapshot              editBaseline_;
    bool                      restoringEdit_ = false;
    juce::Component*          keyHost_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

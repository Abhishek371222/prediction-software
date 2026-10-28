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
#include "TopMenuBar.h"
#include "ViewHeaderBar.h"
#include "SourcePanel.h"
#include "VenuePanel.h"
#include "MicReceiver.h"
#include "FrequencyResponseComponent.h"
#include "MicListenPanel.h"
#include "MicRefLockDialog.h"
#include "CommandTerminal.h"
#include "DistanceSplPlot.h"

// ---------------------------------------------------------------------------
// MainComponent - owns all sub-panels, runs AcousticEngine::compute on a
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
    // Header row 1's document title: "Atomik Integral - <project>",
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
    void copyPlotToClipboard();
    /** Push the plot's mic selection into the Selected panel, so its X / Y
        fields follow whichever mic is chosen. */
    void syncMicSelectionToPanel();
    void exportCSV();

    juce::int64 measurementsSignature() const;   // mtime/size fingerprint, both models combined
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
    // The Figma has ONE strip at the top, not a title row and a ribbon
    // under it. The ribbon is kept in the build and hidden; its actions
    // live in this bar's five menus.
    TopMenuBar     topMenu_;
    // The strip under the menu: the side panel's own header on the left,
    // the canvas's on the right, split on the sidebar's edge.
    ViewHeaderBar  viewBar_;
    // The reference tool stacks a plan over an elevation. The elevation is
    // the same renderer in ElevationSide mode, fed the same solve - a
    // display only, so none of its editing is switched on.
    PaneHeaderBar  elevHeader_ { "Elevation view" };
    PaneHeaderBar  splHeader_  { "Distance Vs. dbSPL" };
    RadiationPatternComponent elevationComp_;
    DistanceSplPlot           splPlot_;
    void syncElevation();
    /** The elevation and the dB pane take the plan's horizontal mapping and
        the dB pane re-reads its line from the last solve. */
    void syncStackedPanes();
    // The Figma's side panel. The old ControlPanel stays in the build,
    // hidden, until its controls have been moved across.
    SourcePanel    sourcePanel_;
    void showTopMenu (int which, juce::Component* under);
    void refreshSourcePanel();
    void showAddShapeMenu (juce::Component* under);
    void addQuadrangularAtOrigin();
    // The source kinds the reference tool's "+" offers. Each lands on
    // the origin; they differ in how many cabinets they bring and how
    // those cabinets are arranged.
    enum class AddSource { Array, PointSource, SubArray };
    void addSourceAtOrigin (AddSource kind, int count);
    void setSourceCabinetCount (int sourceIndex, int count);
    void promptAddMultipleSources();
    // The listening planes, in the order they appear in the Sources
    // list. The index is into THAT list, not into the annotation
    // vector, which also holds free drawings - planeAnnotIndex maps
    // one to the other so nothing else has to know the difference.
    juce::StringArray planeNames() const;
    int  planeAnnotIndex (int planeIndex) const;
    void deletePlane (int planeIndex);
    int  selectedPlane_ = -1;

    // --- the Venue tab ---------------------------------------------------
    // A tab, not a mode of the plan: it draws the room rather than the rig,
    // and brings its own left panel with it.
    bool                      venueMode_ = false;
    std::vector<VenueElement> venue_;
    int                       venueSel_ = -1;
    VenueView                 venueView_;
    VenuePanel                venuePanel_;
    juce::Viewport            venueScroll_;
    void setVenueMode (bool on);
    void refreshVenuePanel();
    void addVenueElement (VenueElement::Type type);
    void setVenueElement (int index, const VenueElement& e);
    void syncVenueToPlanes();
    /** The audience the solver predicts onto: every Venue listening element
        at its real floor and ear height, then the closed listening planes
        drawn on the plan. Rebuilt before each solve; read under measLock_. */
    void rebuildSurfaces();
    std::vector<ListeningSurface> surfaces_;
    std::vector<ListeningSurface> handSurfaces_;   // the plan's share of the last build
    /** The non-audience Venue elements, under the Sources plan. */
    void syncVenueShapes();
    /** The Sources rig, drawn on the Venue panes. */
    void syncVenueRig();
    void deleteVenueElement (int index);
    void venueAction (int index, int action);
    void setVenueFurnitureHidden (bool hidden);
    // Measured off ArrayCalc V12: a new array is created with its frame
    // at 8.00 m. The cabinet pitch and sub spacing are ours - the
    // reference tool reads them from its own cabinet database, which we
    // do not carry, so they are stated here rather than hidden in the
    // arithmetic.
    static constexpr float kArrayFrameHeightM = 8.0f;
    static constexpr float kCabinetPitchM     = 0.35f;
    static constexpr float kSubSpacingM       = 1.2f;
    static constexpr int   kAddArrayCabinets  = 4;
    static constexpr int   kAddSubCabinets    = 4;
    // The menus drive the ribbon's own callbacks, so the app behaves
    // exactly as before. The ribbon's buttons held the on/off state;
    // with it hidden, these carry it so the ticks can be right.
    bool menuRays_ = true, menuInterdist_ = true, menuSplValues_ = false;
    int  menuMapping_ = 0;
    RadiationPatternComponent patternComp_;
    std::unique_ptr<MicFrequencyResponseWindow> frWindow_;
    std::unique_ptr<MicListenWindow>            listenWindow_;
    // WHICH mic that window is listening at, held by id rather than by index:
    // deleting an earlier mic shuffles every index after it, and the window
    // would have carried on listening at whatever slid into the slot.
    int                                         listenMicId_ = -1;
    bool buildMicCurve (int micIndex, std::vector<MicListener::Point>& curve,
                        float& refDb);
    void refreshMicListener();
    void  syncRangeChips();
    void  promptForRange();
    static float parseLength (const juce::String& raw);
    /** Several distances typed in one box, each ready for parseLength. */
    static juce::StringArray splitLengthList (const juce::String& raw);
    void closeMicListener();
    int  micIndexForId (int micId) const;
    void showMicListener (int micIndex);
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
    // Projection switcher over the plot: the plan is the prediction, the two
    // elevations are scale drawings of the rig.
    juce::TextButton btnProjPlan_, btnProjFront_, btnProjSide_;
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
    // Q21S and BEM2inch are two fully separate devices: both are ALWAYS loaded
    // (measuredQ21S_/measuredBEM2inch_ + their own directivity tables below) so
    // a scene can mix units of either model, each simulated with its own
    // data. measured_/measSource_/measDir_ track only which one is shown by
    // the "Measurement set" Measured Polar reference view - they no longer
    // gate what the engine uses (see AcousticEngine::Speaker::model).
    MeasuredSet  measured_;
    MeasuredSet  measuredQ21S_, measuredBEM2inch_, measuredQ212P_;
    juce::File   measDir_;
    juce::int64  measSignature_ = 0;
    int          measSource_ = 1;    // reference-view selector: 0 = Q21S, 1 = Room, 2 = BEM2inch
    float        measDistanceM_ = 0.5f;
    void         setMeasurementSource (int src);
    void         setMeasurementDistance (float distanceM);
    void         syncTextControls();   // ribbon Text cluster <- plot state
    void         syncPlaneControls();  // ribbon Plane cluster <- plot state
    void         reloadAllMeasurements();      // (re)loads both models + their directivity tables
    MeasuredSet  referenceSetFor (int source) const;

    // Per-frequency directivity tables, one array per device, always current -
    // fed into SimParams together every recompute (see run()) so each speaker
    // can pick its own model's table; never merged into one shared table.
    std::vector<DirectivityPattern> directivityQ21STables_, directivityBEM2inchTables_;
    std::vector<DirectivityPattern> directivityQ212PTables_;
    std::vector<BemFieldPattern>    bemFieldTablesQ21S_, bemFieldTablesBEM2inch_;
    std::vector<BemFieldPattern>    bemFieldTablesQ212P_;
    juce::CriticalSection           measLock_;

    // Lightweight second timer for polling the measurement files (the class's
    // own juce::Timer base is reserved for the edit-debounce one-shot).
    struct LambdaTimer : public juce::Timer
    {
        std::function<void()> fn;
        void timerCallback() override { if (fn) fn(); }
    };
    LambdaTimer measPoll_;
    // Coalesces a mic drag into one re-prediction for the Listen window.
    LambdaTimer listenFollow_;
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

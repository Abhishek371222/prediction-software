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

// ---------------------------------------------------------------------------
// MainComponent — owns all sub-panels, runs AcousticEngine::compute on a
// background thread, and keeps the scene/renderer/info panels in sync.
// Edits are debounced via a Timer so dragging sliders or speakers stays
// responsive; switching view mode only re-colours the cached result.
// ---------------------------------------------------------------------------
class MainComponent : public juce::Component,
                      private juce::Thread,
                      private juce::Timer,
                      private juce::ChangeListener
{
public:
    explicit MainComponent (ProjectData project = {});
    ~MainComponent() override;

    void paint              (juce::Graphics&) override;
    void paintOverChildren  (juce::Graphics&) override;
    void resized () override;
    void lookAndFeelChanged() override;                       // re-apply theme colours

private:
    void run() override;            // juce::Thread
    void timerCallback() override;  // juce::Timer (debounce)
    void changeListenerCallback (juce::ChangeBroadcaster*) override;  // AppSettings

    void openPreferences();
    void saveProject();
    void saveProjectAs();
    ProjectData currentProject() const;   // metadata + live scene
    void exportPdfReport();
    void buildAndWriteReport (const juce::File& f);
    juce::Image renderHeatmapImage (double freq, const SimParams& base, double& coverageOut);

    void scheduleRecompute();
    void runSimulation();
    void applyResult (const SimResult& r);
    void updateSettingsBar();
    void updatePlotChrome();
    void setViewMode (ViewMode mode);
    void highlightViewBtn (ViewMode mode);
    void syncRenderer();

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

    LayoutLayer layout_;

    std::unique_ptr<juce::Drawable> logo_;

    juce::Label titleLabel_, versionLabel_;
    ParamBar    paramBar_;
    StatusStrip statusStrip_;
    juce::StringArray statChips_;   // live simulation stats, shown in the Help (?) popup

    juce::Label exportHeader_, viewHeader_;

    juce::TextButton     btnStats_ { "Stats" };
    juce::DrawableButton btnHelp_  { "help", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnPrefsIcon_ { "prefs", juce::DrawableButton::ImageFitted };
    juce::DrawableButton btnMore_  { "more", juce::DrawableButton::ImageFitted };
    void showOverflowMenu();
    void showStatsPopup();
    void refreshHeaderIcons();
    std::unique_ptr<class PreferencesComponent> prefsPanel_;
    void layoutPrefsPanel();

    ProjectData project_;

    juce::TextButton btnExportPNG_, btnExportCSV_;
    juce::TextButton btnViewSPL_, btnViewDirectivity_, btnViewMeasured_;
    juce::TextButton btnViewPhase_, btnViewArrival_, btnViewSTI_;
    juce::Label      comingSoonOverlay_;
    enum class PlaceholderView { None, Phase, ArrivalTime, STI };
    PlaceholderView  placeholderView_ = PlaceholderView::None;
    void showComingSoon (const juce::String& label, PlaceholderView slot);
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
    int          measSource_ = 1;    // 0 = Open Field, 1 = Room
    float        measDistanceM_ = 0.5f;
    void         setMeasurementSource (int src);
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

    static juce::Colour kBg() { return Brand::base(); }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

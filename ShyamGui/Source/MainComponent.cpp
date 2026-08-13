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

    titleLabel_.setText ("Atomik Acoustic Simulation Engine",
                         juce::dontSendNotification);
    titleLabel_.setMinimumHorizontalScale (0.7f);
    titleLabel_.setFont (Brand::techSemi (Brand::Type::appTitle));
    titleLabel_.setColour (juce::Label::textColourId, Brand::text());
    titleLabel_.setJustificationType (juce::Justification::centredRight);
    addAndMakeVisible (titleLabel_);

    versionLabel_.setText ("beta_v1.3.0", juce::dontSendNotification);
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
    configHdr (exportHeader_, "SAVE / EXPORT");
    configHdr (viewHeader_,   "VIEW MODE");

    styleActionBtn (btnExportPNG_, "SAVE IMAGE (PNG)", Brand::exportPill());
    styleActionBtn (btnExportCSV_, "EXPORT SPL (CSV)", Brand::exportPill());
    addAndMakeVisible (btnExportPNG_);
    addAndMakeVisible (btnExportCSV_);
    btnExportPNG_.onClick = [this] { exportPNG(); };
    btnExportCSV_.onClick = [this] { exportCSV(); };

    styleActionBtn (btnViewSPL_,         "SPL HEAT MAP", Brand::idleViewPill());
    styleActionBtn (btnViewDirectivity_, "DIRECTIVITY", Brand::idleViewPill());
    styleActionBtn (btnViewMeasured_,    "MEASURED POLAR", Brand::idleViewPill());
    styleActionBtn (btnViewPhase_,        "PHASE", Brand::idleViewPill());
    styleActionBtn (btnViewArrival_,      "ARRIVAL TIME", Brand::idleViewPill());
    styleActionBtn (btnViewSTI_,          "STI MAP", Brand::idleViewPill());
    for (auto* b : { &btnViewSPL_, &btnViewDirectivity_, &btnViewMeasured_,
                     &btnViewPhase_, &btnViewArrival_, &btnViewSTI_ })
        addAndMakeVisible (*b);
    btnViewSPL_.onClick         = [this] { setViewMode (ViewMode::SPL); };
    btnViewDirectivity_.onClick = [this] { setViewMode (ViewMode::Directivity); };
    btnViewMeasured_.onClick    = [this] { setViewMode (ViewMode::MeasuredPolar); };
    btnViewPhase_.onClick       = [this] { showComingSoon ("Phase view", PlaceholderView::Phase); };
    btnViewArrival_.onClick     = [this] { showComingSoon ("Arrival time map", PlaceholderView::ArrivalTime); };
    btnViewSTI_.onClick         = [this] { showComingSoon ("STI map", PlaceholderView::STI); };

    comingSoonOverlay_.setJustificationType (juce::Justification::centred);
    comingSoonOverlay_.setFont (Brand::tech (Brand::Type::sectionHdr + 6.0f, true));
    comingSoonOverlay_.setColour (juce::Label::textColourId, Brand::ash());
    comingSoonOverlay_.setColour (juce::Label::backgroundColourId, Brand::panelDark());
    comingSoonOverlay_.setInterceptsMouseClicks (false, false);
    addChildComponent (comingSoonOverlay_);

    btnStats_.setComponentID ("headerStats");
    btnStats_.setButtonText ("Stats");
    btnStats_.setTooltip ("Scene Summary & Selected Speaker");
    btnStats_.setColour (juce::TextButton::buttonColourId,   Brand::statsBtn());
    btnStats_.setColour (juce::TextButton::textColourOffId,  Brand::statsText());
    btnStats_.onClick = [this] { showStatsPopup(); };
    addAndMakeVisible (btnStats_);

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
    plotHeader_.btnSelect_.onClick   = [this] { patternComp_.refreshView(); };
    plotHeader_.btnPan_.onClick      = [this] { patternComp_.refreshView(); };

    addAndMakeVisible (statusStrip_);
    statusStrip_.setStatus ("Ready", true);

    // Wire panels -----------------------------------------------------------
    controlPanel_.onChanged     = [this]
    {
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
            scheduleRecompute();
        }
    };
    controlPanel_.onRunClicked  = [this] { runSimulation(); };
    controlPanel_.onSectionsChanged = [this] { resized(); };   // re-fit sidebar viewport
    controlPanel_.onSelectionChanged = [this] (int idx)
    {
        patternComp_.setSpeakers (controlPanel_.getSpeakers(), idx);
        patternComp_.repaint();
    };

    patternComp_.onSpeakerSelected = [this] (int idx)
    {
        controlPanel_.selectSpeaker (idx);
    };
    patternComp_.onSpeakerMoved = [this] (int idx, float x, float y)
    {
        controlPanel_.setSpeakerPosition (idx, x, y);
        scheduleRecompute();
    };
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

    // Load the project's scene + show its identity.
    if (! project_.speakers.empty())
        controlPanel_.applyProject (project_);

    AppSettings::get().addChangeListener (this);
    applyGridPref();

    setSize (1340, 820);   // after all child components exist (setSize calls resized)

    runSimulation();   // first compute already has the measured directivity tables
}

MainComponent::~MainComponent()
{
    AppSettings::get().removeChangeListener (this);
    measPoll_.stopTimer();
    stopTimer();
    stopThread (3000);
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

    btnStats_.setColour (juce::TextButton::buttonColourId,   Brand::statsBtn());
    btnStats_.setColour (juce::TextButton::textColourOffId,  Brand::statsText());

    auto restyle = [] (juce::TextButton& b)
    {
        const bool isExport = b.getButtonText().containsIgnoreCase ("SAVE")
                           || b.getButtonText().containsIgnoreCase ("EXPORT");
        styleActionBtn (b, b.getButtonText(),
                        isExport ? Brand::exportPill() : Brand::idleViewPill(), false);
    };
    for (auto* b : { &btnExportPNG_, &btnExportCSV_,
                     &btnViewSPL_, &btnViewDirectivity_, &btnViewMeasured_,
                     &btnViewPhase_, &btnViewArrival_, &btnViewSTI_ })
        restyle (*b);
    updateViewButtonHighlights();
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

    // Atomik wordmark, top-left of the header band.
    if (logo_ != nullptr)
    {
        juce::Rectangle<float> logoBox (16.0f, 8.0f, 180.0f, 44.0f);
        logo_->drawWithin (g, logoBox,
                           juce::RectanglePlacement::xLeft
                         | juce::RectanglePlacement::yMid
                         | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
    }
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
    comingSoonOverlay_.setFont (Brand::techSemi (Brand::UI::scaledFont (Brand::Type::sectionHdr + 6.0f)));

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

    const int statsW = UiConfig::Scale::px (78);
    btnStats_.setBounds      (rx - statsW, headerBtnTop, statsW, Brand::UI::headerIconH); rx -= statsW;

    {
        const int logoRight = UiConfig::Scale::px (200);
        const int regionR   = rx - UiConfig::Scale::px (12);
        const int regionW   = juce::jmax (80, regionR - logoRight);
        juce::Font f = Brand::techSemi (Brand::UI::scaledFont (Brand::Type::appTitle));
        float fontH  = Brand::UI::scaledFont (Brand::Type::appTitle);
        while (fontH > UiConfig::Laf::titleShrinkMin
               && f.withHeight (fontH).getStringWidthFloat (titleLabel_.getText()) > (float) regionW - 40.0f)
            fontH -= 0.5f;
        f = f.withHeight (fontH);
        titleLabel_.setFont (f);
        versionLabel_.setFont (Brand::techSemi (juce::jmax (UiConfig::Laf::versionMin,
                                                       fontH * UiConfig::Laf::versionFromTitle)));

        const float tw = f.getStringWidthFloat (titleLabel_.getText());
        const float vw = versionLabel_.getFont().getStringWidthFloat (versionLabel_.getText());
        const float pairW = tw + UiConfig::Scale::px (8) + vw;
        float pairX = W * 0.5f - pairW * 0.5f;
        pairX = juce::jlimit ((float) logoRight, (float) regionR - pairW, pairX);

        const int titleTop = UiConfig::Scale::px (6);
        titleLabel_.setBounds ((int) pairX, titleTop, (int) tw + 2, titleH - UiConfig::Scale::px (8));
        versionLabel_.setBounds ((int) (pairX + tw + UiConfig::Scale::px (8)), titleTop,
                                 (int) vw + 4, titleH - UiConfig::Scale::px (8));
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

    // Heatmap window — same bodyTop as sidebar.
    plotHeader_.setBounds (plotX, bodyTop, centreW, plotHdrH);
    patternComp_.setBounds (plotX, bodyTop + plotHdrH, centreW, juce::jmax (40, bodyH - plotHdrH));
    comingSoonOverlay_.setBounds (patternComp_.getBounds());

    // Bottom panel — v1.1: narrow Export column + two equal View Mode columns.
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

    // Two equal View Mode columns; header spans both (v1.1).
    const int remainW  = centreW - exportW - sectionGap;
    const int viewColW = (remainW - viewColGap) / 2;
    const int viewSpan = remainW;

    viewHeader_.setBounds (bx, bottomTop, viewSpan, secHdrH);
    int vy = contentTop;
    btnViewSPL_.setBounds         (bx, vy, viewColW, btnH); vy += btnH + btnGap;
    btnViewDirectivity_.setBounds (bx, vy, viewColW, btnH); vy += btnH + btnGap;
    btnViewMeasured_.setBounds    (bx, vy, viewColW, btnH);
    bx += viewColW + viewColGap;

    vy = contentTop;
    btnViewPhase_.setBounds    (bx, vy, viewColW, btnH); vy += btnH + btnGap;
    btnViewArrival_.setBounds  (bx, vy, viewColW, btnH); vy += btnH + btnGap;
    btnViewSTI_.setBounds      (bx, vy, viewColW, btnH);

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
        chips.add ("Grid: " + juce::String (p.resolution) + " ^2");
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
    placeholderView_ = PlaceholderView::None;
    comingSoonOverlay_.setVisible (false);
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

    const bool place = (placeholderView_ != PlaceholderView::None);
    styleView (btnViewSPL_,         ! place && currentView_ == ViewMode::SPL);
    styleView (btnViewDirectivity_, ! place && currentView_ == ViewMode::Directivity);
    styleView (btnViewMeasured_,    ! place && currentView_ == ViewMode::MeasuredPolar);
    styleView (btnViewPhase_,       place && placeholderView_ == PlaceholderView::Phase);
    styleView (btnViewArrival_,     place && placeholderView_ == PlaceholderView::ArrivalTime);
    styleView (btnViewSTI_,         place && placeholderView_ == PlaceholderView::STI);
}

void MainComponent::showComingSoon (const juce::String& label, PlaceholderView slot)
{
    placeholderView_ = slot;
    comingSoonOverlay_.setText (label + "\nComing soon", juce::dontSendNotification);
    comingSoonOverlay_.setVisible (true);
    comingSoonOverlay_.toFront (false);
    updateViewButtonHighlights();
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
        e.image = renderHeatmapImage ((double) hz, base, e.coveragePct);
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

juce::Image MainComponent::renderHeatmapImage (double freq, const SimParams& base, double& coverageOut)
{
    SimParams p = base;
    p.frequency = freq;
    p.viewMode  = ViewMode::SPL;

    SimResult r = AcousticEngine::compute (p);
    coverageOut = AcousticAnalysis::coverageWithin (r, 6.0);

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

            line ("# Atomik Acoustic Simulation Engine v1.3.0");
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

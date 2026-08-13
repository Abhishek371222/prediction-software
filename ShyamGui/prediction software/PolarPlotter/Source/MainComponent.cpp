#include "MainComponent.h"

MainComponent::MainComponent()
{
    setSize (1180, 760);
    setOpaque (true);

    titleLabel.setText ("ATOMIK POLAR - Phase 1", juce::dontSendNotification);
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff7dd3fc));
    titleLabel.setFont (juce::FontOptions (18.0f).withStyle ("Bold"));
    addAndMakeVisible (titleLabel);

    auto styleLabel = [] (juce::Label& l, const juce::String& text)
    {
        l.setText (text, juce::dontSendNotification);
        l.setColour (juce::Label::textColourId, juce::Colour (0xff94a3b8));
        l.setFont (juce::FontOptions (13.0f));
    };

    styleLabel (setLabel, "Measurement set");
    styleLabel (distLabel, "Distance (m)");
    styleLabel (normLabel, "Normalize");
    styleLabel (freqLabel, "Frequencies (overlay like CLIO)");

    addAndMakeVisible (setLabel);
    addAndMakeVisible (distLabel);
    addAndMakeVisible (normLabel);
    addAndMakeVisible (freqLabel);

    setBox.addListener (this);
    distBox.addListener (this);
    normBox.addListener (this);
    addAndMakeVisible (setBox);
    addAndMakeVisible (distBox);
    addAndMakeVisible (normBox);

    normBox.addItem ("On-axis (0 deg) - CLIO", 1);
    normBox.addItem ("Peak", 2);
    normBox.addItem ("Absolute SPL", 3);
    normBox.setSelectedId (1, juce::dontSendNotification);

    selectAllBtn.addListener (this);
    selectNoneBtn.addListener (this);
    addAndMakeVisible (selectAllBtn);
    addAndMakeVisible (selectNoneBtn);

    statusLabel.setColour (juce::Label::textColourId, juce::Colour (0xffcbd5e1));
    statusLabel.setFont (juce::FontOptions (12.0f));
    addAndMakeVisible (statusLabel);

    helpLabel.setColour (juce::Label::textColourId, juce::Colour (0xff64748b));
    helpLabel.setFont (juce::FontOptions (12.0f));
    helpLabel.setText (
        "ShyamGuild is the reference set (real Excel curves). "
        "All folders are loaded: ShyamGuild, Factory, 3inch, XN18.",
        juce::dontSendNotification);
    helpLabel.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (helpLabel);

    addAndMakeVisible (polarPlot);

    const auto dataFolder = findDataFolder();
    if (library.loadFromFolder (dataFolder))
    {
        statusLabel.setText (library.getStatus(), juce::dontSendNotification);
        rebuildSetList();
    }
    else
    {
        statusLabel.setText (library.getStatus(), juce::dontSendNotification);
        polarPlot.setSubtitle ("No data loaded");
    }
}

MainComponent::~MainComponent()
{
    setBox.removeListener (this);
    distBox.removeListener (this);
    normBox.removeListener (this);
    selectAllBtn.removeListener (this);
    selectNoneBtn.removeListener (this);
}

juce::File MainComponent::findDataFolder() const
{
    // Next to the executable (POST_BUILD copies Data there)
    auto exeDir = juce::File::getSpecialLocation (juce::File::currentExecutableFile).getParentDirectory();
    auto candidate = exeDir.getChildFile ("Data");
    if (candidate.isDirectory())
        return candidate;

    // Dev / source tree fallback
    auto walk = juce::File::getCurrentWorkingDirectory();
    for (int i = 0; i < 6; ++i)
    {
        candidate = walk.getChildFile ("Data");
        if (candidate.isDirectory())
            return candidate;
        candidate = walk.getChildFile ("PolarPlotter").getChildFile ("Data");
        if (candidate.isDirectory())
            return candidate;
        walk = walk.getParentDirectory();
    }

    return exeDir.getChildFile ("Data");
}

void MainComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0b1220));

    auto panel = getLocalBounds().removeFromLeft (300).reduced (10);
    g.setColour (juce::Colour (0xff111827));
    g.fillRoundedRectangle (panel.toFloat(), 10.0f);
    g.setColour (juce::Colour (0xff1e293b));
    g.drawRoundedRectangle (panel.toFloat(), 10.0f, 1.0f);
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    auto left = area.removeFromLeft (300).reduced (18, 16);
    auto right = area.reduced (8, 10);

    titleLabel.setBounds (left.removeFromTop (28));
    left.removeFromTop (12);

    setLabel.setBounds (left.removeFromTop (18));
    setBox.setBounds (left.removeFromTop (28));
    left.removeFromTop (10);

    distLabel.setBounds (left.removeFromTop (18));
    distBox.setBounds (left.removeFromTop (28));
    left.removeFromTop (10);

    normLabel.setBounds (left.removeFromTop (18));
    normBox.setBounds (left.removeFromTop (28));
    left.removeFromTop (12);

    freqLabel.setBounds (left.removeFromTop (18));
    auto btnRow = left.removeFromTop (28);
    selectAllBtn.setBounds (btnRow.removeFromLeft (70));
    btnRow.removeFromLeft (8);
    selectNoneBtn.setBounds (btnRow.removeFromLeft (70));
    left.removeFromTop (8);

    const int toggleH = 26;
    for (auto* t : freqToggles)
    {
        if (left.getHeight() < toggleH + 60)
            break;
        t->setBounds (left.removeFromTop (toggleH));
        left.removeFromTop (2);
    }

    helpLabel.setBounds (left.removeFromBottom (48));
    statusLabel.setBounds (left.removeFromBottom (40));

    polarPlot.setBounds (right);
}

void MainComponent::rebuildSetList()
{
    setBox.clear (juce::dontSendNotification);
    auto names = library.getSetNames();
    for (int i = 0; i < names.size(); ++i)
    {
        juce::String label = names[i];
        if (names[i] == "ShyamGuild")
            label = "ShyamGuild (reference)";
        else if (names[i] == "XN18")
            label = "XN18 / Gylt";
        setBox.addItem (label, i + 1);
    }

    // Prefer ShyamGuild — real data used for matching curves
    int prefer = names.indexOf ("ShyamGuild");
    if (prefer < 0)
        prefer = 0;
    if (names.size() > 0)
        setBox.setSelectedItemIndex (prefer, juce::sendNotificationSync);
}

static juce::String activeSetName (const juce::ComboBox& box)
{
    auto t = box.getText();
    if (t.startsWith ("ShyamGuild"))
        return "ShyamGuild";
    if (t.startsWith ("XN18"))
        return "XN18";
    return t;
}

void MainComponent::rebuildDistanceList()
{
    distBox.clear (juce::dontSendNotification);
    const auto setName = activeSetName (setBox);
    auto dists = library.getDistances (setName);

    for (int i = 0; i < (int) dists.size(); ++i)
    {
        juce::String label = juce::String (dists[(size_t) i], 1) + " m";
        distBox.addItem (label, i + 1);
    }

    // Prefer 0.5 m for ShyamGuild (CLIO reference), else 1 m
    int prefer = 0;
    for (int i = 0; i < (int) dists.size(); ++i)
    {
        if (std::abs (dists[(size_t) i] - 0.5f) < 1.0e-4f)
        {
            prefer = i;
            break;
        }
        if (std::abs (dists[(size_t) i] - 1.0f) < 1.0e-4f)
            prefer = i;
    }

    if (! dists.empty())
        distBox.setSelectedItemIndex (prefer, juce::sendNotificationSync);
}

void MainComponent::rebuildFrequencyToggles()
{
    for (auto* t : freqToggles)
        removeChildComponent (t);
    freqToggles.clear();
    currentFreqs.clear();

    const auto setName = activeSetName (setBox);
    const float dist = distBox.getText().upToFirstOccurrenceOf (" ", false, false).getFloatValue();
    currentFreqs = library.getFrequencies (setName, dist);

    for (int freq : currentFreqs)
    {
        auto* toggle = new juce::ToggleButton (juce::String (freq) + " Hz");
        toggle->setToggleState (true, juce::dontSendNotification);
        toggle->setColour (juce::ToggleButton::textColourId, colourForFrequency (freq));
        toggle->setColour (juce::ToggleButton::tickColourId, colourForFrequency (freq));
        toggle->onClick = [this] { refreshPlot(); };
        addAndMakeVisible (toggle);
        freqToggles.add (toggle);
    }

    resized();
    refreshPlot();
}

juce::Colour MainComponent::colourForFrequency (int freqHz) const
{
    // CLIO-like palette for common LF set; distinct hues for HF
    switch (freqHz)
    {
        case 30:   return juce::Colours::red;
        case 60:   return juce::Colour (0xfff97316);
        case 80:   return juce::Colour (0xff9ca3af);
        case 100:  return juce::Colour (0xffa78bfa);
        case 150:  return juce::Colour (0xff22d3ee);
        case 200:  return juce::Colours::limegreen;
        case 300:  return juce::Colour (0xff38bdf8);
        case 500:  return juce::Colour (0xffd4c200);
        case 1000: return juce::Colour (0xfff472b6);
        case 2000: return juce::Colour (0xfffb7185);
        case 4000: return juce::Colour (0xffc084fc);
        case 8000: return juce::Colour (0xff2dd4bf);
        case 16000: return juce::Colour (0xffe879f9);
        default:   return juce::Colours::white;
    }
}

void MainComponent::refreshPlot()
{
    const auto setName = activeSetName (setBox);
    const float dist = distBox.getText().upToFirstOccurrenceOf (" ", false, false).getFloatValue();
    const int normMode = normBox.getSelectedId();

    std::vector<PlotSeries> series;
    juce::StringArray activeSources;
    int pointCount = 0;

    for (int i = 0; i < freqToggles.size(); ++i)
    {
        if (! freqToggles[i]->getToggleState())
            continue;

        const int freq = currentFreqs[(size_t) i];
        const auto* sweep = library.find (setName, freq, dist);
        if (sweep == nullptr)
            continue;

        PlotSeries ps;
        ps.label = juce::String (freq) + "Hz";
        ps.colour = colourForFrequency (freq);

        std::vector<float> rel;
        if (normMode == 1)
            rel = sweep->relativeToOnAxis();
        else if (normMode == 2)
            rel = sweep->relativeToPeak();
        else
        {
            // Absolute: shift so peak sits near 0 for display in same ring scale
            const float peak = sweep->peakSpl();
            for (const auto& p : sweep->points)
                rel.push_back (p.splDb - peak);
        }

        // Exact measured points only — same curve as the Excel/CSV readings
        for (size_t k = 0; k < sweep->points.size(); ++k)
        {
            PolarPoint pt;
            pt.degree = sweep->points[k].degree;
            pt.splDb = rel[k];
            ps.points.push_back (pt);
        }

        pointCount = juce::jmax (pointCount, (int) sweep->points.size());
        if (sweep->sourceXlsx.isNotEmpty())
            activeSources.addIfNotAlreadyThere (sweep->sourceXlsx);
        else
            activeSources.addIfNotAlreadyThere (sweep->fileName);

        series.push_back (std::move (ps));
    }

    polarPlot.setSeries (std::move (series));
    polarPlot.setTitle ("2D Directivity Analysis");
    polarPlot.setSubtitle (setName + "  -  Horizontal @ " + juce::String (dist, 1) + " m"
                           + "  |  " + juce::String (pointCount) + " pts");

    juce::String status = library.getStatus();
    if (activeSources.size() > 0)
        status = setName + " @ " + juce::String (dist, 1) + " m | real data: "
                 + activeSources.joinIntoString (", ");
    statusLabel.setText (status, juce::dontSendNotification);

    polarPlot.setDbRange (-30.0f, 6.0f);
}

void MainComponent::comboBoxChanged (juce::ComboBox* box)
{
    if (box == &setBox)
        rebuildDistanceList();
    else if (box == &distBox)
        rebuildFrequencyToggles();
    else if (box == &normBox)
        refreshPlot();
}

void MainComponent::buttonClicked (juce::Button* button)
{
    const bool all = (button == &selectAllBtn);
    for (auto* t : freqToggles)
        t->setToggleState (all, juce::dontSendNotification);
    refreshPlot();
}

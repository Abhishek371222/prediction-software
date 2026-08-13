#include "InfoPanel.h"
#include "BrandTheme.h"
#include "AppSettings.h"

// Row indices
enum { kSpeed = 0, kWorld, kFreq, kLambda, kWaveNo, kActive, kCoverage, kPeak,
       kSelected, kPos, kGain, kDelay, kPolarity, kOrient, kNumRows };

// ---------------------------------------------------------------------------
InfoPanel::InfoPanel()
{
    auto configHdr = [&] (juce::Label& l, const juce::String& t)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (Brand::tech (Brand::Type::sectionHdr, true));
        l.setColour (juce::Label::textColourId, Brand::heading());
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    };

    configHdr (sceneHeader_, "Scene Summary");
    addRow ("Speed of sound");
    addRow ("World");
    addRow ("Frequency");
    addRow ("Wavelength");
    addRow ("Wave number k");
    addRow ("Active Q21S");
    addRow ("Coverage (6 dB)");
    addRow ("Peak SPL");

    configHdr (speakerHeader_, "Selected Speaker");
    addRow ("ID");
    addRow ("Position");
    addRow ("Gain");
    addRow ("Delay");
    addRow ("Polarity");
    addRow ("Orientation");

    setRowVal (kSpeed, "343 m/s");
    setRowVal (kWorld, "30 x 30 m");
    applyColours();
}

void InfoPanel::addRow (const juce::String& key)
{
    auto* r = rows_.add (new Row());
    r->key.setText (key, juce::dontSendNotification);
    r->key.setFont (Brand::tech (UiConfig::FontSize::infoKey));
    r->key.setJustificationType (juce::Justification::centredLeft);

    r->val.setFont (Brand::mono (Brand::Type::input, true));
    r->val.setJustificationType (juce::Justification::centredRight);

    addAndMakeVisible (r->key);
    addAndMakeVisible (r->val);
}

void InfoPanel::setRowVal (int i, const juce::String& v)
{
    if (i >= 0 && i < rows_.size()) rows_[i]->val.setText (v, juce::dontSendNotification);
}

// ---------------------------------------------------------------------------
void InfoPanel::updateInfo (const SimResult& r, const SimParams& p, int selectedIndex)
{
    const juce::String u = Units::lengthUnit();
    const double speed = Units::metresToDisplay (343.0);
    setRowVal (kSpeed,  juce::String (speed, 0) + " " + u + "/s");
    setRowVal (kWorld,  juce::String (Units::metresToDisplay (p.worldW), 0) + " x "
                        + juce::String (Units::metresToDisplay (p.worldH), 0) + " " + u);
    setRowVal (kFreq,   juce::String ((int) r.frequency) + " Hz");
    setRowVal (kLambda, juce::String (Units::metresToDisplay (r.lambda), 3) + " " + u);
    setRowVal (kWaveNo, juce::String (r.k, 3) + " rad/m");
    setRowVal (kActive, juce::String (r.activeSpeakers) + " / " + juce::String ((int) p.speakers.size()));

    // Coverage within 6 dB of peak + peak relative SPL (always 0 dB by definition).
    if (! r.splDB.empty())
    {
        int within = 0;
        for (float v : r.splDB)
            if (v >= -6.0f) ++within;
        const double pct = 100.0 * (double) within / (double) r.splDB.size();
        setRowVal (kCoverage, juce::String (pct, 1) + " %");
        setRowVal (kPeak, "0 dB (rel.)");
    }
    else
    {
        setRowVal (kCoverage, "-");
        setRowVal (kPeak, "-");
    }

    if (selectedIndex >= 0 && selectedIndex < (int) p.speakers.size())
    {
        const auto& s = p.speakers[(size_t) selectedIndex];
        setRowVal (kSelected, "Q21S-" + juce::String (selectedIndex + 1)
                              + (s.enabled ? "" : " (off)"));
        setRowVal (kPos,      "(" + juce::String (Units::metresToDisplay (s.x), 1) + ", "
                              + juce::String (Units::metresToDisplay (s.y), 1) + ") " + u);
        setRowVal (kGain,     juce::String (s.gainDB, 0) + " dB");
        setRowVal (kDelay,    juce::String (s.delayMs, 1) + " ms");
        setRowVal (kPolarity, s.polarityInverted ? "Reverse" : "Normal");
        setRowVal (kOrient,   s.reverseOrientation ? "Reverse (-x)" : "Forward (+x)");
    }
    else
    {
        setRowVal (kSelected, "-");
        setRowVal (kPos, "-"); setRowVal (kGain, "-"); setRowVal (kDelay, "-");
        setRowVal (kPolarity, "-"); setRowVal (kOrient, "-");
    }
    repaint();
}

// ---------------------------------------------------------------------------
void InfoPanel::paint (juce::Graphics& g)
{
    g.fillAll (Brand::panel());

    const int pad = Brand::UI::panelPad;
    const int W = getWidth() - 2 * pad;
    auto drawCard = [&] (int y0, int y1)
    {
        if (y1 <= y0) return;
        auto r = juce::Rectangle<float> ((float) pad - 4.0f, (float) y0 - 2.0f,
                                         (float) W + 8.0f, (float) (y1 - y0 + 4));
        g.setColour (Brand::card());
        g.fillRoundedRectangle (r, Brand::UI::cardRadius);
        g.setColour (Brand::border().withAlpha (0.35f));
        g.drawRoundedRectangle (r, Brand::UI::cardRadius, 1.0f);
    };

    drawCard (pad - 2, sceneCardEnd_);
    drawCard (speakerCardY_, speakerCardEnd_);
}

void InfoPanel::applyColours()
{
    // Cards use Brand::card() (= light btnIn in dark theme). Ink must stay
    // dark on those surfaces or Scene Summary / Selected Speaker go invisible.
    const bool lightCards = Brand::card().getPerceivedBrightness() > 0.55f;
    const auto ink   = lightCards ? Brand::charcoal() : Brand::text();
    const auto muted = lightCards ? Brand::charcoal().withAlpha (0.62f) : Brand::ash();
    const auto hdr   = lightCards ? Brand::charcoal() : Brand::heading();

    sceneHeader_.setColour   (juce::Label::textColourId, hdr);
    speakerHeader_.setColour (juce::Label::textColourId, hdr);
    for (auto* r : rows_)
    {
        r->key.setColour (juce::Label::textColourId, muted);
        r->val.setColour (juce::Label::textColourId, ink);
    }
}

void InfoPanel::lookAndFeelChanged()
{
    applyColours();
    repaint();
}

// ---------------------------------------------------------------------------
void InfoPanel::resized()
{
    const int pad  = Brand::UI::panelPad;
    const int W    = getWidth() - 2 * pad;
    const int rowH = 24;
    const int keyW = (int) (W * 0.52f);
    int y = pad;

    auto placeRows = [&] (int from, int to)
    {
        for (int i = from; i < to && i < rows_.size(); ++i)
        {
            rows_[i]->key.setBounds (pad + 8, y, keyW - 8, rowH);
            rows_[i]->val.setBounds (pad + keyW,  y, W - keyW,     rowH);
            y += rowH + 3;
        }
    };

    sceneHeader_.setBounds (pad + 8, y, W - 16, 22); y += 26;
    const int sceneStart = y;
    placeRows (kSpeed, kSelected);
    sceneCardEnd_ = y + 4;
    y += Brand::UI::sectionGap;

    speakerCardY_ = y;
    speakerHeader_.setBounds (pad + 8, y, W - 16, 22); y += 26;
    placeRows (kSelected, kNumRows);
    speakerCardEnd_ = y + 4;
    contentHeight_ = speakerCardEnd_ + pad;
}

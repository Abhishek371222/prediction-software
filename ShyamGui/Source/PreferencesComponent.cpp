#include "PreferencesComponent.h"

PreferencesComponent::PreferencesComponent()
{
    setSize (UiConfig::Layout::prefsPanelWidth, UiConfig::Layout::prefsPanelHeight);

    auto configHdr = [&] (juce::Label& l, const juce::String& t, float basePx, bool hdr)
    {
        l.setText (t, juce::dontSendNotification);
        l.setFont (Brand::tech (Brand::UI::scaledFont (basePx), hdr));
        l.setColour (juce::Label::textColourId, hdr ? Brand::heading() : Brand::ash());
        l.setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (l);
    };

    configHdr (title_,         "PREFERENCES", Brand::Type::prefsTitle,      true);
    configHdr (appearanceHdr_, "APPEARANCE",  Brand::Type::prefsSectionHdr, true);
    configHdr (unitsHdr_,      "UNITS",       Brand::Type::prefsSectionHdr, true);
    configHdr (themeLabel_,    "Theme",       Brand::Type::prefsLabel,      false);
    configHdr (unitsLabel_,    "Unit system", Brand::Type::prefsLabel,      false);

    unitsNote_.setText ("SI: m / mm / kg / " + Units::degree() + "C        "
                        "Imperial: ft / in / lbs / " + Units::degree() + "F",
                        juce::dontSendNotification);
    unitsNote_.setFont (Brand::mono (Brand::UI::scaledFont (Brand::Type::prefsNote)));
    unitsNote_.setColour (juce::Label::textColourId, Brand::ash());
    unitsNote_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (unitsNote_);

    styleSeg (themeDarkBtn_,  "DARK");
    styleSeg (themeLightBtn_, "LIGHT");
    styleSeg (unitsSiBtn_,    "SI");
    styleSeg (unitsImpBtn_,   "IMPERIAL");
    styleSeg (closeBtn_,      "CLOSE");

    themeDarkBtn_.onClick  = [this] { AppSettings::get().setTheme (ThemeMode::Dark);  refreshSelection(); };
    themeLightBtn_.onClick = [this] { AppSettings::get().setTheme (ThemeMode::Light); refreshSelection(); };
    unitsSiBtn_.onClick    = [this] { AppSettings::get().setUnits (UnitSystem::SI);       refreshSelection(); };
    unitsImpBtn_.onClick   = [this] { AppSettings::get().setUnits (UnitSystem::Imperial); refreshSelection(); };
    closeBtn_.onClick      = [this] { if (onClose) onClose(); };

    refreshSelection();
}

void PreferencesComponent::styleSeg (juce::TextButton& b, const juce::String& txt)
{
    b.setButtonText (txt);
    b.setComponentID ("prefsBtn");
    b.setClickingTogglesState (false);
    b.setColour (juce::TextButton::buttonColourId,  Brand::btnIn());
    b.setColour (juce::TextButton::textColourOffId, Brand::onBtnIn());
    b.setColour (juce::TextButton::textColourOnId,  Brand::white());
    addAndMakeVisible (b);
}

void PreferencesComponent::applyScaledChrome()
{
    title_.setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::prefsTitle), true));
    appearanceHdr_.setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::prefsSectionHdr), true));
    unitsHdr_.setFont (appearanceHdr_.getFont());
    themeLabel_.setFont (Brand::tech (Brand::UI::scaledFont (Brand::Type::prefsLabel)));
    unitsLabel_.setFont (themeLabel_.getFont());
    unitsNote_.setFont (Brand::mono (Brand::UI::scaledFont (Brand::Type::prefsNote)));
}

void PreferencesComponent::refreshSelection()
{
    const bool dark     = AppSettings::get().isDark();
    const bool imperial = AppSettings::get().isImperial();

    themeDarkBtn_.setColour  (juce::TextButton::buttonColourId, dark      ? Brand::accent() : Brand::btnIn());
    themeLightBtn_.setColour (juce::TextButton::buttonColourId, ! dark    ? Brand::accent() : Brand::btnIn());
    unitsSiBtn_.setColour    (juce::TextButton::buttonColourId, ! imperial ? Brand::accent() : Brand::btnIn());
    unitsImpBtn_.setColour   (juce::TextButton::buttonColourId, imperial  ? Brand::accent() : Brand::btnIn());

    themeDarkBtn_.setColour  (juce::TextButton::textColourOffId, dark      ? Brand::white() : Brand::onBtnIn());
    themeLightBtn_.setColour (juce::TextButton::textColourOffId, ! dark    ? Brand::white() : Brand::onBtnIn());
    unitsSiBtn_.setColour    (juce::TextButton::textColourOffId, ! imperial ? Brand::white() : Brand::onBtnIn());
    unitsImpBtn_.setColour   (juce::TextButton::textColourOffId, imperial  ? Brand::white() : Brand::onBtnIn());

    repaint();
}

void PreferencesComponent::applyColours()
{
    title_.setColour         (juce::Label::textColourId, Brand::heading());
    appearanceHdr_.setColour (juce::Label::textColourId, Brand::heading());
    unitsHdr_.setColour      (juce::Label::textColourId, Brand::heading());
    themeLabel_.setColour    (juce::Label::textColourId, Brand::ash());
    unitsLabel_.setColour    (juce::Label::textColourId, Brand::ash());
    unitsNote_.setColour     (juce::Label::textColourId, Brand::ash());
    closeBtn_.setColour      (juce::TextButton::textColourOffId, Brand::onBtnIn());
    refreshSelection();
}

void PreferencesComponent::lookAndFeelChanged()
{
    applyScaledChrome();
    applyColours();
    repaint();
}

void PreferencesComponent::paint (juce::Graphics& g)
{
    g.fillAll (Brand::panel());
    g.setColour (Brand::border());
    g.drawRect (getLocalBounds(), 1);

    const float pad = (float) UiConfig::Scale::px (UiConfig::Layout::prefsPadding);
    const float dividerY = (float) UiConfig::Scale::px (56);
    g.setColour (Brand::border().withAlpha (0.45f));
    g.drawLine (pad, dividerY, (float) getWidth() - pad, dividerY, 1.0f);
}

void PreferencesComponent::resized()
{
    applyScaledChrome();

    const int pad   = UiConfig::Scale::px (UiConfig::Layout::prefsPadding);
    const int W     = getWidth() - 2 * pad;
    const int titleH = UiConfig::Scale::px (UiConfig::Layout::prefsTitleRowH);
    const int secHdrH = UiConfig::Scale::px (UiConfig::Layout::prefsSectionHeaderH);
    const int rowH  = UiConfig::Scale::px (UiConfig::Layout::prefsRowH);
    const int labelW = UiConfig::Scale::px (UiConfig::Layout::prefsLabelColW);
    const int segW  = UiConfig::Scale::px (UiConfig::Layout::prefsSegBtnW);
    const int noteH = UiConfig::Scale::px (UiConfig::Layout::prefsNoteRowH);
    const int closeW = UiConfig::Scale::px (UiConfig::Layout::prefsCloseBtnW);
    const int closeH = UiConfig::Scale::px (UiConfig::Layout::prefsCloseBtnH);
    const int labelGap = UiConfig::Scale::px (10);
    const int segGap = UiConfig::Scale::px (10);

    int y = UiConfig::Scale::px (18);

    title_.setBounds (pad, y, W, titleH);
    y = UiConfig::Scale::px (74);

    appearanceHdr_.setBounds (pad, y, W, secHdrH);
    y += UiConfig::Scale::px (28);
    themeLabel_.setBounds (pad, y, labelW, rowH);
    themeDarkBtn_.setBounds  (pad + labelW + labelGap, y, segW, rowH);
    themeLightBtn_.setBounds (pad + labelW + labelGap + segW + segGap, y, segW, rowH);
    y += UiConfig::Scale::px (54);

    unitsHdr_.setBounds (pad, y, W, secHdrH);
    y += UiConfig::Scale::px (28);
    unitsLabel_.setBounds (pad, y, labelW, rowH);
    unitsSiBtn_.setBounds  (pad + labelW + labelGap, y, segW, rowH);
    unitsImpBtn_.setBounds (pad + labelW + labelGap + segW + segGap, y, segW, rowH);
    y += UiConfig::Scale::px (42);
    unitsNote_.setBounds (pad, y, W, noteH);
    y += UiConfig::Scale::px (42);

    closeBtn_.setBounds (getWidth() - pad - closeW, getHeight() - UiConfig::Scale::px (46),
                         closeW, closeH);
}

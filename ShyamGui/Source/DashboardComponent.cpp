#include "DashboardComponent.h"

namespace
{
    // Card design footprint (logical px, matching the Figma frame). Everything
    // inside is positioned against these and scaled together.
    constexpr int   kCardW    = 540;
    constexpr int   kCardH    = 344;
    constexpr int   kMargin   = 40;     // page gutter around the card
    constexpr float kMinScale = 0.85f;
    constexpr float kMaxScale = 1.70f;
    // Text gets its own floor: the card may shrink on a small window, but the
    // labels must stay readable rather than scaling all the way down with it.
    constexpr float kMinTextScale = 0.95f;

    juce::Colour cardFill()  { return juce::Colour (0xfff6f6f6); }
    juce::Colour cardEdge()  { return juce::Colour (0xffcfcfcf); }
    juce::Colour fieldFill() { return juce::Colour (0xffe8e8e8); }

    void stylePrimary (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId,   Brand::accent());
        b.setColour (juce::TextButton::buttonOnColourId, Brand::accent());
        b.setColour (juce::TextButton::textColourOffId,  Brand::white());
        b.setColour (juce::TextButton::textColourOnId,   Brand::white());
    }

    void styleSecondary (juce::TextButton& b)
    {
        b.setColour (juce::TextButton::buttonColourId,   juce::Colours::white);
        b.setColour (juce::TextButton::buttonOnColourId, juce::Colours::white);
        b.setColour (juce::TextButton::textColourOffId,  Brand::text());
        b.setColour (juce::TextButton::textColourOnId,   Brand::text());
    }

    void styleEditor (juce::TextEditor& e)
    {
        // Figma draws these as flat grey wells with no outline.
        e.setColour (juce::TextEditor::backgroundColourId, fieldFill());
        e.setColour (juce::TextEditor::textColourId,       Brand::text());
        e.setColour (juce::TextEditor::outlineColourId,    juce::Colours::transparentBlack);
        e.setColour (juce::TextEditor::focusedOutlineColourId, Brand::accent());
        e.setColour (juce::TextEditor::highlightColourId,  Brand::accent().withAlpha (0.3f));
        e.setColour (juce::CaretComponent::caretColourId,  Brand::accent());
    }
}

// ---------------------------------------------------------------------------
DashboardComponent::DashboardComponent()
{
    setSize (760, 600);
    logo_ = Brand::createLogo (Brand::text());

    title_.setText ("ACOUSTIC SIMULATION ENGINE", juce::dontSendNotification);
    title_.setColour (juce::Label::textColourId, Brand::text());
    title_.setJustificationType (juce::Justification::centred);
    title_.setMinimumHorizontalScale (1.0f);
    title_.setBorderSize ({});
    addAndMakeVisible (title_);

    footer_.setText ("Atomik - Simulation Engine - v1.4.0.1", juce::dontSendNotification);
    footer_.setColour (juce::Label::textColourId, Brand::ash().withAlpha (0.85f));
    footer_.setJustificationType (juce::Justification::centred);
    footer_.setMinimumHorizontalScale (1.0f);
    footer_.setBorderSize ({});
    addAndMakeVisible (footer_);

    newBtn_.setButtonText ("NEW PROJECT");
    openBtn_.setButtonText ("OPEN EXISTING PROJECT");
    newBtn_.setComponentID ("dashAction");
    openBtn_.setComponentID ("dashAction");
    stylePrimary (newBtn_);
    styleSecondary (openBtn_);
    addAndMakeVisible (newBtn_);
    addAndMakeVisible (openBtn_);
    newBtn_.onClick  = [this] { showNewForm(); };
    openBtn_.onClick = [this] { openExisting(); };

    recentBox_.setComponentID ("ctrlCombo");
    recentBox_.setTextWhenNothingSelected ("Recent Projects");
    recentBox_.setTextWhenNoChoicesAvailable ("Recent Projects");
    recentBox_.setColour (juce::ComboBox::backgroundColourId, juce::Colours::white);
    recentBox_.setColour (juce::ComboBox::textColourId,       Brand::text());
    recentBox_.onChange = [this]
    {
        const int i = recentBox_.getSelectedId() - 1;
        if (juce::isPositiveAndBelow (i, recentFiles_.size()))
        {
            const auto f = recentFiles_[i];
            recentBox_.setSelectedId (0, juce::dontSendNotification);
            openProjectFile (f);
        }
    };
    addAndMakeVisible (recentBox_);

    // New-project form. Fields are added in Figma reading order (left column,
    // then right, row by row) so the 2-column layout below just alternates.
    addField ("ownerName",    "Owner Name*");
    addField ("engineerName", "Engineer Name*");
    addField ("projectName",  "Project Name*");
    addField ("city",         "City*");
    addField ("email",        "Email*");
    addField ("address",      "Address*");
    addField ("mobile",       "Contact Number*");
    if (auto* d = addField ("date", "Date*"))
        d->editor.setText (ProjectMeta::today(), false);

    createBtn_.setButtonText ("CREATE PROJECT");
    cancelBtn_.setButtonText ("CANCEL");
    createBtn_.setComponentID ("dashAction");
    cancelBtn_.setComponentID ("dashAction");
    stylePrimary (createBtn_);
    styleSecondary (cancelBtn_);
    addChildComponent (createBtn_);
    addChildComponent (cancelBtn_);
    createBtn_.onClick = [this] { createFromForm(); };
    cancelBtn_.onClick = [this] { showMenu(); };

    rebuildRecent();
    showMenu();
}

DashboardComponent::Field* DashboardComponent::addField (const juce::String& key,
                                                         const juce::String& label,
                                                         const juce::String& placeholder)
{
    auto* f = fields_.add (new Field());
    f->key = key;
    f->label.setText (label, juce::dontSendNotification);
    f->label.setColour (juce::Label::textColourId, Brand::text());
    f->label.setJustificationType (juce::Justification::centredLeft);
    f->label.setMinimumHorizontalScale (1.0f);
    f->label.setBorderSize ({});
    addChildComponent (f->label);

    f->editor.setTextToShowWhenEmpty (placeholder, Brand::ash().withAlpha (0.6f));
    f->editor.setIndents (8, 4);
    styleEditor (f->editor);
    addChildComponent (f->editor);
    return f;
}

juce::String DashboardComponent::fieldText (const juce::String& key) const
{
    for (auto* f : fields_) if (f->key == key) return f->editor.getText().trim();
    return {};
}

// ---------------------------------------------------------------------------
float DashboardComponent::cardScale() const
{
    const float sx = (float) getWidth()  / (float) (kCardW + 2 * kMargin);
    const float sy = (float) getHeight() / (float) (kCardH + 2 * kMargin);
    return juce::jlimit (kMinScale, kMaxScale, juce::jmin (sx, sy));
}

juce::Rectangle<int> DashboardComponent::cardBounds() const
{
    const float s = cardScale();
    const int w = juce::roundToInt (kCardW * s);
    const int h = juce::roundToInt (kCardH * s);
    return juce::Rectangle<int> ((getWidth() - w) / 2, (getHeight() - h) / 2, w, h);
}

// ---------------------------------------------------------------------------
void DashboardComponent::showMenu()
{
    view_ = View::Menu;
    newBtn_.setVisible (true);
    openBtn_.setVisible (true);
    recentBox_.setVisible (true);
    createBtn_.setVisible (false);
    cancelBtn_.setVisible (false);
    for (auto* f : fields_) { f->label.setVisible (false); f->editor.setVisible (false); }
    rebuildRecent();
    resized();
    repaint();
}

void DashboardComponent::showNewForm()
{
    view_ = View::NewForm;
    newBtn_.setVisible (false);
    openBtn_.setVisible (false);
    recentBox_.setVisible (false);
    createBtn_.setVisible (true);
    cancelBtn_.setVisible (true);
    for (auto* f : fields_) { f->label.setVisible (true); f->editor.setVisible (true); }
    resized();
    repaint();
    if (! fields_.isEmpty()) fields_.getFirst()->editor.grabKeyboardFocus();
}

void DashboardComponent::rebuildRecent()
{
    recentBox_.clear (juce::dontSendNotification);
    recentFiles_.clearQuick();

    int id = 1;
    for (const auto& path : AppSettings::get().recentProjects())
    {
        juce::File f (path);
        recentFiles_.add (f);
        recentBox_.addItem (f.getFileNameWithoutExtension(), id);
        recentBox_.setItemEnabled (id, f.existsAsFile());
        ++id;
    }
    recentBox_.setSelectedId (0, juce::dontSendNotification);
}

// ---------------------------------------------------------------------------
void DashboardComponent::createFromForm()
{
    ProjectMeta meta;
    meta.projectName  = fieldText ("projectName");
    meta.engineerName = fieldText ("engineerName");
    meta.ownerName    = fieldText ("ownerName");
    meta.address      = fieldText ("address");
    meta.city         = fieldText ("city");
    meta.email        = fieldText ("email");
    meta.mobile       = fieldText ("mobile");
    meta.date         = fieldText ("date");
    if (meta.date.isEmpty()) meta.date = ProjectMeta::today();

    if (meta.projectName.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
            "Project Name required", "Please enter a project name before creating the project.");
        for (auto* f : fields_)
            if (f->key == "projectName") { f->editor.grabKeyboardFocus(); break; }
        return;
    }

    auto project = ProjectData::makeDefault (meta);

    const auto suggested = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                               .getChildFile (juce::File::createLegalFileName (meta.projectName) + ".atmk");

    chooser_ = std::make_unique<juce::FileChooser> ("Save new project", suggested, "*.atmk");
    chooser_->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this, project] (const juce::FileChooser& fc) mutable
        {
            auto f = fc.getResult();
            if (f == juce::File()) return;   // cancelled -> stay on form
            f = f.withFileExtension ("atmk");
            if (project.saveToFile (f))
                AppSettings::get().addRecentProject (f);
            if (onProjectReady) onProjectReady (project);
        });
}

void DashboardComponent::openExisting()
{
    chooser_ = std::make_unique<juce::FileChooser> (
        "Open project", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory), "*.atmk");
    chooser_->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this] (const juce::FileChooser& fc)
        {
            auto f = fc.getResult();
            if (f != juce::File()) openProjectFile (f);
        });
}

void DashboardComponent::openProjectFile (const juce::File& f)
{
    ProjectData p;
    if (! ProjectData::loadFromFile (f, p))
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
            "Could not open project",
            "The file could not be read:\n" + f.getFullPathName());
        return;
    }
    AppSettings::get().addRecentProject (f);
    if (onProjectReady) onProjectReady (p);
}

// ---------------------------------------------------------------------------
void DashboardComponent::applyColours()
{
    logo_ = Brand::createLogo (Brand::text());
    title_.setColour  (juce::Label::textColourId, Brand::text());
    footer_.setColour (juce::Label::textColourId, Brand::ash().withAlpha (0.85f));
    stylePrimary (newBtn_);    styleSecondary (openBtn_);
    stylePrimary (createBtn_); styleSecondary (cancelBtn_);
    recentBox_.setColour (juce::ComboBox::backgroundColourId, juce::Colours::white);
    recentBox_.setColour (juce::ComboBox::textColourId,       Brand::text());
    for (auto* f : fields_)
    {
        f->label.setColour (juce::Label::textColourId, Brand::text());
        styleEditor (f->editor);
    }
}

void DashboardComponent::lookAndFeelChanged()
{
    applyColours();
    repaint();
}

// ---------------------------------------------------------------------------
void DashboardComponent::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::white);

    const auto card = cardBounds().toFloat();
    const float s = cardScale();
    const float radius = 6.0f * s;

    g.setColour (cardFill());
    g.fillRoundedRectangle (card, radius);
    g.setColour (cardEdge());
    g.drawRoundedRectangle (card.reduced (0.5f), radius, 1.0f);

    // Wordmark, centred near the top of the card.
    const float logoH = 20.0f * s;
    const float logoW = logoH * Brand::logoAspect;
    const float logoY = card.getY() + (view_ == View::Menu ? 46.0f : 30.0f) * s;
    Brand::drawLogo (g, logo_.get(),
                     { card.getCentreX() - logoW * 0.5f, logoY, logoW, logoH });
}

void DashboardComponent::resized()
{
    // Keep the app-wide scale coherent for this window too — the LookAndFeel
    // sizes button text from it.
    Brand::UI::applyWindowScale (getWidth(), getHeight());

    const auto card = cardBounds();
    const float s = cardScale();
    const float ts = juce::jmax (kMinTextScale, s);   // text scale
    auto px = [s] (float v) { return juce::roundToInt (v * s); };

    // Sizes calibrated against the Figma render's ink-width-to-card-width
    // ratios; JUCE's Font height is the whole line box, so these run larger
    // than the design's nominal point sizes.
    title_.setFont (Brand::techSemi (15.0f * ts));
    footer_.setFont (Brand::tech (11.0f * ts));

    if (view_ == View::Menu)
    {
        title_.setBounds (card.getX(), card.getY() + px (100), card.getWidth(), px (20));

        const int btnW = px (215), btnH = px (33), gap = px (11);
        const int rowX = card.getCentreX() - (btnW * 2 + gap) / 2;
        const int rowY = card.getY() + px (150);
        newBtn_.setBounds  (rowX, rowY, btnW, btnH);
        openBtn_.setBounds (rowX + btnW + gap, rowY, btnW, btnH);

        recentBox_.setBounds (rowX, rowY + btnH + px (17), btnW, px (32));

        footer_.setBounds (card.getX(), card.getBottom() - px (34), card.getWidth(), px (16));
    }
    else // NewForm
    {
        title_.setBounds (card.getX(), card.getY() + px (66), card.getWidth(), px (20));
        footer_.setBounds (0, 0, 0, 0);

        const int colW = px (212), colGap = px (16);
        const int leftX = card.getCentreX() - (colW * 2 + colGap) / 2;
        const int labelH = px (16), editH = px (22), rowPitch = px (41);
        const int top = card.getY() + px (106);

        for (int i = 0; i < fields_.size(); ++i)
        {
            auto* f = fields_[i];
            const int col = i % 2, row = i / 2;
            const int x = leftX + col * (colW + colGap);
            const int y = top + row * rowPitch;
            f->label.setFont (Brand::tech (11.5f * ts));
            f->label.setBounds  (x, y, colW, labelH);
            f->editor.setFont (Brand::tech (12.5f * ts));
            f->editor.setBounds (x, y + labelH, colW, editH);
        }

        const int rows = (fields_.size() + 1) / 2;
        const int btnY = top + rows * rowPitch + px (14);
        createBtn_.setBounds (leftX, btnY, colW, px (27));
        cancelBtn_.setBounds (leftX + colW + colGap, btnY, colW, px (27));
    }
}

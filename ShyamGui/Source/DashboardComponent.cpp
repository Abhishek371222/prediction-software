#include "DashboardComponent.h"

namespace
{
    void styleButton (juce::TextButton& b, bool primary)
    {
        b.setColour (juce::TextButton::buttonColourId,   primary ? Brand::accent() : Brand::btnIn());
        b.setColour (juce::TextButton::buttonOnColourId, Brand::accent());
        b.setColour (juce::TextButton::textColourOffId,  primary ? Brand::white() : Brand::onBtnIn());
        b.setColour (juce::TextButton::textColourOnId,   Brand::white());
    }

    void styleEditor (juce::TextEditor& e)
    {
        e.setColour (juce::TextEditor::backgroundColourId, Brand::btnIn());
        e.setColour (juce::TextEditor::textColourId,       Brand::onBtnIn());
        e.setColour (juce::TextEditor::outlineColourId,    Brand::border());
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
    title_.setFont (Brand::tech (26.0f, true));
    title_.setColour (juce::Label::textColourId, Brand::text());
    title_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (title_);

    subtitle_.setText ("Project Dashboard", juce::dontSendNotification);
    subtitle_.setFont (Brand::tech (Brand::Type::dashSubtitle));
    subtitle_.setColour (juce::Label::textColourId, Brand::ash());
    subtitle_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (subtitle_);

    newBtn_.setButtonText ("NEW PROJECT");
    openBtn_.setButtonText ("OPEN EXISTING PROJECT");
    newBtn_.setComponentID ("dashAction");
    openBtn_.setComponentID ("dashAction");
    styleButton (newBtn_,  true);
    styleButton (openBtn_, false);
    addAndMakeVisible (newBtn_);
    addAndMakeVisible (openBtn_);
    newBtn_.onClick  = [this] { showNewForm(); };
    openBtn_.onClick = [this] { openExisting(); };

    recentHdr_.setText ("RECENT PROJECTS", juce::dontSendNotification);
    recentHdr_.setFont (Brand::tech (Brand::Type::dashRecentHeader, true));
    recentHdr_.setColour (juce::Label::textColourId, Brand::heading());
    recentHdr_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (recentHdr_);

    noRecent_.setText ("No recent projects yet.", juce::dontSendNotification);
    noRecent_.setFont (Brand::mono (Brand::Type::dashRecentItem));
    noRecent_.setColour (juce::Label::textColourId, Brand::ash());
    noRecent_.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (noRecent_);

    // --- New-project form --------------------------------------------------
    formTitle_.setText ("NEW PROJECT DETAILS", juce::dontSendNotification);
    formTitle_.setFont (Brand::tech (20.0f, true));
    formTitle_.setColour (juce::Label::textColourId, Brand::heading());
    formTitle_.setJustificationType (juce::Justification::centredLeft);
    addChildComponent (formTitle_);

    addField ("projectName",  "Project Name");
    addField ("engineerName", "Engineer Name");
    addField ("ownerName",    "Owner Name");
    addField ("address",      "Address");
    addField ("city",         "City");
    addField ("country",      "Country");
    addField ("email",        "Email");
    addField ("mobile",       "Mobile Number");
    if (auto* d = addField ("date", "Date"))
        d->editor.setText (ProjectMeta::today(), false);

    createBtn_.setButtonText ("CREATE PROJECT");
    cancelBtn_.setButtonText ("CANCEL");
    styleButton (createBtn_, true);
    styleButton (cancelBtn_, false);
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
    f->label.setFont (Brand::tech (14.0f));
    f->label.setColour (juce::Label::textColourId, Brand::ash());
    f->label.setJustificationType (juce::Justification::centredLeft);
    addChildComponent (f->label);

    f->editor.setFont (Brand::tech (15.0f));
    f->editor.setTextToShowWhenEmpty (placeholder, Brand::ash().withAlpha (0.6f));
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
void DashboardComponent::showMenu()
{
    view_ = View::Menu;
    const bool m = true;
    title_.setVisible (m); subtitle_.setVisible (m);
    newBtn_.setVisible (m); openBtn_.setVisible (m);
    recentHdr_.setVisible (m);
    rebuildRecent();

    formTitle_.setVisible (false);
    createBtn_.setVisible (false); cancelBtn_.setVisible (false);
    for (auto* f : fields_) { f->label.setVisible (false); f->editor.setVisible (false); }
    resized();
    repaint();
}

void DashboardComponent::showNewForm()
{
    view_ = View::NewForm;
    title_.setVisible (false); subtitle_.setVisible (false);
    newBtn_.setVisible (false); openBtn_.setVisible (false);
    recentHdr_.setVisible (false); noRecent_.setVisible (false);
    for (auto* b : recentBtns_) b->setVisible (false);

    formTitle_.setVisible (true);
    createBtn_.setVisible (true); cancelBtn_.setVisible (true);
    for (auto* f : fields_) { f->label.setVisible (true); f->editor.setVisible (true); }
    resized();
    repaint();
    if (! fields_.isEmpty()) fields_.getFirst()->editor.grabKeyboardFocus();
}

void DashboardComponent::rebuildRecent()
{
    recentBtns_.clear();
    auto recents = AppSettings::get().recentProjects();
    const bool any = ! recents.isEmpty() && view_ == View::Menu;
    noRecent_.setVisible (view_ == View::Menu && recents.isEmpty());

    for (const auto& path : recents)
    {
        juce::File f (path);
        auto* b = recentBtns_.add (new juce::TextButton());
        b->setComponentID ("dashRecent");
        b->setButtonText (f.getFileNameWithoutExtension() + "   -   " + f.getFullPathName());
        b->setColour (juce::TextButton::buttonColourId,   Brand::panel());
        b->setColour (juce::TextButton::textColourOffId,  f.existsAsFile() ? Brand::text() : Brand::ash());
        b->onClick = [this, f] { openProjectFile (f); };
        addAndMakeVisible (*b);
        b->setVisible (view_ == View::Menu);
    }
    juce::ignoreUnused (any);
    resized();
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
    meta.country      = fieldText ("country");
    meta.email        = fieldText ("email");
    meta.mobile       = fieldText ("mobile");
    meta.date         = fieldText ("date");
    if (meta.date.isEmpty()) meta.date = ProjectMeta::today();

    if (meta.projectName.isEmpty())
    {
        juce::AlertWindow::showMessageBoxAsync (juce::MessageBoxIconType::WarningIcon,
            "Project Name required", "Please enter a project name before creating the project.");
        if (! fields_.isEmpty()) fields_.getFirst()->editor.grabKeyboardFocus();
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
    title_.setColour    (juce::Label::textColourId, Brand::text());
    subtitle_.setColour (juce::Label::textColourId, Brand::ash());
    recentHdr_.setColour(juce::Label::textColourId, Brand::heading());
    noRecent_.setColour (juce::Label::textColourId, Brand::ash());
    formTitle_.setColour(juce::Label::textColourId, Brand::heading());
    styleButton (newBtn_, true);  styleButton (openBtn_, false);
    styleButton (createBtn_, true); styleButton (cancelBtn_, false);
    for (auto* f : fields_) { f->label.setColour (juce::Label::textColourId, Brand::ash()); styleEditor (f->editor); }
    rebuildRecent();
}

void DashboardComponent::lookAndFeelChanged()
{
    applyColours();
    repaint();
}

// ---------------------------------------------------------------------------
void DashboardComponent::paint (juce::Graphics& g)
{
    g.fillAll (Brand::base());

    Brand::UI::applyWindowScale (getWidth(), getHeight());
    const float pad = (float) UiConfig::Scale::px (40);
    const float logoH = (float) UiConfig::Scale::px (19);  // −20% vs prior 24 px
    const float logoW = logoH * Brand::logoAspect;
    Brand::drawLogo (g, logo_.get(), { pad, (float) UiConfig::Scale::px (28), logoW, logoH });

    const float divY = (float) UiConfig::Scale::px (104);
    g.setColour (Brand::border());
    g.drawLine (pad, divY, (float) getWidth() - pad, divY, 1.0f);
}

void DashboardComponent::resized()
{
    Brand::UI::applyWindowScale (getWidth(), getHeight());
    const int pad = UiConfig::Scale::px (40);
    const int W = getWidth() - 2 * pad;

    // Divider line sits under the logo (see paint); keep title + content below it.
    const int divY = UiConfig::Scale::px (104);
    title_.setBounds    (pad, divY + UiConfig::Scale::px (12), W, UiConfig::Scale::px (34));
    subtitle_.setBounds (pad, title_.getBottom() + UiConfig::Scale::px (2), W, UiConfig::Scale::px (28));

    if (view_ == View::Menu)
    {
        int y = subtitle_.getBottom() + UiConfig::Scale::px (18);
        const int bw = (W - UiConfig::Scale::px (16)) / 2;
        const int btnH = UiConfig::Scale::px (56);
        const int gap = UiConfig::Scale::px (16);
        newBtn_.setBounds  (pad, y, bw, btnH);
        openBtn_.setBounds (pad + bw + gap, y, bw, btnH);
        y += btnH + UiConfig::Scale::px (30);

        recentHdr_.setBounds (pad, y, W, UiConfig::Scale::px (26)); y += UiConfig::Scale::px (34);
        noRecent_.setBounds (pad, y, W, UiConfig::Scale::px (26));
        for (auto* b : recentBtns_)
        {
            b->setBounds (pad, y, W, UiConfig::Scale::px (40));
            y += UiConfig::Scale::px (44);
        }
    }
    else // NewForm
    {
        int y = subtitle_.getBottom() + UiConfig::Scale::px (8);
        formTitle_.setBounds (pad, y, W, UiConfig::Scale::px (28)); y += UiConfig::Scale::px (44);

        const int colGap = UiConfig::Scale::px (24);
        const int colW = (W - colGap) / 2;
        const int rowH = UiConfig::Scale::px (56);
        int idx = 0;
        for (auto* f : fields_)
        {
            const int col = idx % 2;
            const int rx = pad + col * (colW + colGap);
            if (col == 0 && idx > 0) y += rowH;
            f->label.setBounds  (rx, y, colW, UiConfig::Scale::px (18));
            f->editor.setBounds (rx, y + UiConfig::Scale::px (20), colW, UiConfig::Scale::px (30));
            ++idx;
        }
        y += rowH + UiConfig::Scale::px (16);

        createBtn_.setBounds (getWidth() - pad - UiConfig::Scale::px (180),
                              getHeight() - UiConfig::Scale::px (60),
                              UiConfig::Scale::px (180), UiConfig::Scale::px (38));
        cancelBtn_.setBounds (getWidth() - pad - UiConfig::Scale::px (180) - UiConfig::Scale::px (140),
                              getHeight() - UiConfig::Scale::px (60),
                              UiConfig::Scale::px (130), UiConfig::Scale::px (38));
    }
}

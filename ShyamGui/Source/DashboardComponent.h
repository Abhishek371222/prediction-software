#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "AppSettings.h"
#include "ProjectData.h"

// ---------------------------------------------------------------------------
// DashboardComponent - the project launcher shown at startup. Lets the user
// create a new project (collecting full metadata), open an existing .atmk
// project, or pick from recent projects. When a project is ready it fires
// onProjectReady, and the app swaps in the main simulation window.
// ---------------------------------------------------------------------------
class DashboardComponent : public juce::Component
{
public:
    DashboardComponent();

    std::function<void(ProjectData)> onProjectReady;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void lookAndFeelChanged() override;

private:
    enum class View { Menu, NewForm };

    void showMenu();
    void showNewForm();
    void rebuildRecent();
    void createFromForm();
    void openExisting();
    void openProjectFile (const juce::File&);
    void applyColours();

    struct Field
    {
        juce::Label      label;
        juce::TextEditor editor;
        juce::String     key;
    };

    Field* addField (const juce::String& key, const juce::String& label,
                     const juce::String& placeholder = {});
    juce::String fieldText (const juce::String& key) const;

    View view_ = View::Menu;

    std::unique_ptr<juce::Drawable> logo_;

    // Menu view
    juce::Label      title_, subtitle_, recentHdr_, noRecent_;
    juce::TextButton newBtn_, openBtn_;
    juce::OwnedArray<juce::TextButton> recentBtns_;

    // New-project form view
    juce::Label      formTitle_;
    juce::OwnedArray<Field> fields_;
    juce::TextButton createBtn_, cancelBtn_;

    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DashboardComponent)
};

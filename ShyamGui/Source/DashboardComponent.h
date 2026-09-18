#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "AppSettings.h"
#include "ProjectData.h"

// ---------------------------------------------------------------------------
// DashboardComponent — the launch screen ("Home Screen" in the Figma file).
// Two views inside one centred card: a menu (New / Open / Recent) and the
// new-project details form.
// ---------------------------------------------------------------------------
class DashboardComponent : public juce::Component
{
public:
    DashboardComponent();

    std::function<void (ProjectData)> onProjectReady;

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

    /** The card is a fixed design footprint, centred in whatever window size
        we're given; everything inside is laid out against it. */
    juce::Rectangle<int> cardBounds() const;
    float cardScale() const;

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

    juce::Label      title_, footer_;
    juce::TextButton newBtn_, openBtn_;
    juce::ComboBox   recentBox_;
    juce::Array<juce::File> recentFiles_;

    juce::OwnedArray<Field> fields_;
    juce::TextButton createBtn_, cancelBtn_;

    std::unique_ptr<juce::FileChooser> chooser_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DashboardComponent)
};

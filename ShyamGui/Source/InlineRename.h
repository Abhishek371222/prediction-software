#pragma once
#include <JuceHeader.h>

// =============================================================================
// Ends an in-place name edit when the user clicks ANYWHERE outside the editor.
//
// Focus-lost alone is not enough: most of the app's surfaces (the plans, the
// panel backgrounds, the rows) never take keyboard focus, so clicking them
// left the editor open and the old name still showing until Enter was
// pressed. A global mouse listener sees every click in the app instead.
// =============================================================================
class ClickAwayWatcher : private juce::MouseListener
{
public:
    std::function<void()> onClickAway;

    ~ClickAwayWatcher() override { stop(); }

    void start (juce::Component& editor)
    {
        stop();
        editor_ = &editor;
        juce::Desktop::getInstance().addGlobalMouseListener (this);
        active_ = true;
    }

    void stop()
    {
        if (active_)
            juce::Desktop::getInstance().removeGlobalMouseListener (this);
        active_ = false;
        editor_ = nullptr;
    }

private:
    void mouseDown (const juce::MouseEvent& e) override
    {
        auto* ed = editor_.getComponent();
        if (ed == nullptr) return;
        auto* c = e.originalComponent;
        if (c == ed || ed->isParentOf (c)) return;   // a click inside the editor
        if (onClickAway) onClickAway();
    }

    juce::Component::SafePointer<juce::Component> editor_;
    bool active_ = false;
};

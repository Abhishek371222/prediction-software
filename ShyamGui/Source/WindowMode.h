#pragma once
#include <JuceHeader.h>
#include "AppSettings.h"
#include "BrandTheme.h"

// =============================================================================
// Full-screen workspace.
//
// The app opens MAXIMISED by default: the whole work area, with the normal
// title bar so minimise / maximise / close stay in reach. F11, or Settings >
// Full Screen, switches to borderless full screen (JUCE kiosk mode: the whole
// monitor, over the taskbar) and back. The choice is remembered, so the next
// launch opens the same way.
//
// Windows takes kiosk mode away whenever the app loses focus (Alt+Tab, a
// click on another app). Each window therefore puts it back when it is
// activated again, as long as full screen is still the user's choice - so it
// survives switching apps, dialogs and file choosers.
// =============================================================================
namespace WindowMode
{
    inline bool isFullScreen (const juce::Component* window)
    {
        return window != nullptr && juce::Desktop::getInstance().getKioskModeComponent() == window;
    }

    /** Switching kiosk mode on Windows re-creates the native window, which
        drops its icon - the title bar and taskbar showed a black square
        instead of the Atomik mark. Put it back after every switch. */
    inline void restoreIcon (juce::ResizableWindow& w)
    {
        const auto icon = Brand::createIcon (256);
        if (auto* dw = dynamic_cast<juce::DocumentWindow*> (&w)) dw->setIcon (icon);
        if (auto* peer = w.getPeer()) peer->setIcon (icon);
    }

    /** Full screen (kiosk) or maximised, for this window. */
    inline void apply (juce::ResizableWindow& w, bool fullScreen)
    {
        auto& desk = juce::Desktop::getInstance();
        if (fullScreen)
        {
            if (w.getPeer() == nullptr || ! w.isShowing()) return;
            if (desk.getKioskModeComponent() != &w)
                desk.setKioskModeComponent (&w, false);
            // The monitor it is on, all of it - including after a resolution
            // or monitor change.
            if (auto* d = desk.getDisplays().getDisplayForRect (w.getScreenBounds()))
                if (w.getBounds() != d->totalArea)
                    w.setBounds (d->totalArea);
        }
        else
        {
            if (desk.getKioskModeComponent() == &w)
                desk.setKioskModeComponent (nullptr, false);
            w.setFullScreen (true);   // maximised: the work area, title bar back
        }
        restoreIcon (w);
    }

    /** F11 / the menu item: flip it for the window @p anyChild sits in, and
        remember the choice for the next launch. */
    inline void toggle (juce::Component* anyChild)
    {
        if (anyChild == nullptr) return;
        auto* w = dynamic_cast<juce::ResizableWindow*> (anyChild->getTopLevelComponent());
        if (w == nullptr) return;
        const bool want = ! isFullScreen (w);
        AppSettings::get().setStartFullScreen (want);
        apply (*w, want);
    }

    inline bool isToggleKey (const juce::KeyPress& k)
    {
        return k == juce::KeyPress (juce::KeyPress::F11Key);
    }

    /** Mixed into the app's top-level windows: opens them in the remembered
        mode, keeps full screen through focus changes and display changes,
        and answers F11. */
    class Keeper : private juce::Timer
    {
    public:
        explicit Keeper (juce::ResizableWindow& w) : window_ (w) {}

        /** Call once the window is visible. */
        void start()
        {
            window_.setFullScreen (true);   // maximised underneath, so leaving full screen lands here
            apply (window_, AppSettings::get().startFullScreen());
            startTimer (1000);
        }

        /** From the window's activeWindowStatusChanged(). */
        void activeChanged()
        {
            if (window_.isActiveWindow() && AppSettings::get().startFullScreen()
                && ! isFullScreen (&window_))
            {
                juce::Component::SafePointer<juce::ResizableWindow> safe (&window_);
                juce::MessageManager::callAsync ([safe]
                {
                    if (safe != nullptr && safe->isActiveWindow()
                        && AppSettings::get().startFullScreen())
                        apply (*safe, true);
                });
            }
        }

    private:
        // A monitor's resolution can change under a full-screen window (or it
        // can be dragged to another screen while windowed); keep it covering
        // the display it is on.
        void timerCallback() override
        {
            if (isFullScreen (&window_))
                if (auto* d = juce::Desktop::getInstance().getDisplays().getDisplayForRect (window_.getScreenBounds()))
                    if (window_.getBounds() != d->totalArea)
                        window_.setBounds (d->totalArea);
        }

        juce::ResizableWindow& window_;
    };
}

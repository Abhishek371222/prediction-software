#include <JuceHeader.h>
#include "MainComponent.h"
#include "DashboardComponent.h"
#include "BrandTheme.h"

class TwoSpeakerApp : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName()    override { return "Atomik Acoustic Simulation Engine"; }
    const juce::String getApplicationVersion() override { return "1.3.0"; }
    bool moreThanOneInstanceAllowed()          override { return false; }

    void initialise (const juce::String&) override
    {
        juce::LookAndFeel::setDefaultLookAndFeel (&lookAndFeel_);
        showDashboard();
    }

    void shutdown() override
    {
        mainWindow_.reset();
        dashboardWindow_.reset();
        juce::LookAndFeel::setDefaultLookAndFeel (nullptr);
    }

    void systemRequestedQuit() override { quit(); }

    // Second launch (build/open while already running) must raise the UI —
    // otherwise `open` looks like a no-op because only one instance is allowed.
    void anotherInstanceStarted (const juce::String&) override { raiseUi(); }

    void raiseUi()
    {
        auto raise = [] (juce::DocumentWindow* w)
        {
            if (w == nullptr) return;
            w->setVisible (true);
            w->toFront (true);
        };
        raise (mainWindow_.get());
        raise (dashboardWindow_.get());
        juce::Process::makeForegroundProcess();
    }

    // -----------------------------------------------------------------------
    void showDashboard()
    {
        mainWindow_.reset();
        dashboardWindow_.reset (new DashboardWindow (
            [this] (ProjectData p) { openProject (std::move (p)); }));
        raiseUi();
    }

    void openProject (ProjectData project)
    {
        // Defer: the call originates from inside the dashboard's own component.
        juce::MessageManager::callAsync ([this, project]
        {
            mainWindow_.reset (new MainWindow (getApplicationName(), project));
            dashboardWindow_.reset();
            raiseUi();
        });
    }

    // ======================================================================
    class DashboardWindow : public juce::DocumentWindow
    {
    public:
        explicit DashboardWindow (std::function<void(ProjectData)> onReady)
            : DocumentWindow ("Atomik - Project Dashboard", Brand::base(),
                              DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            auto* dash = new DashboardComponent();
            dash->onProjectReady = std::move (onReady);
            setContentOwned (dash, true);
            setResizable (true, true);

            // Keep the dashboard proportional with a sensible minimum size.
            const double aspect = 760.0 / 600.0;
            const int    minW   = 680;
            const int    minH   = juce::roundToInt (minW / aspect);
            setResizeLimits (minW, minH, 4000, juce::roundToInt (4000.0 / aspect));
            if (auto* c = getConstrainer())
                c->setFixedAspectRatio (aspect);

            centreWithSize (getWidth(), getHeight());
            setVisible (true);
            setIcon (Brand::createIcon (256));
            if (auto* peer = getPeer()) peer->setIcon (Brand::createIcon (256));
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DashboardWindow)
    };

    // ======================================================================
    class MainWindow : public juce::DocumentWindow
    {
    public:
        MainWindow (const juce::String& name, const ProjectData& project)
            : DocumentWindow (name, Brand::base(), DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar (true);
            setContentOwned (new MainComponent (project), true);
            setResizable (true, true);

            // Lock the widescreen proportions and enforce a usable minimum so
            // the layout can never collapse or distort while resizing.
            const double aspect = 1340.0 / 820.0;             // design ratio
            const int    minW   = 1120;
            const int    minH   = juce::roundToInt (minW / aspect);
            setResizeLimits (minW, minH, 10000, juce::roundToInt (10000.0 / aspect));
            if (auto* c = getConstrainer())
                c->setFixedAspectRatio (aspect);

            centreWithSize (getWidth(), getHeight());
            setVisible (true);
            setIcon (Brand::createIcon (256));
            if (auto* peer = getPeer()) peer->setIcon (Brand::createIcon (256));
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }

    private:
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainWindow)
    };

private:
    Brand::AtomikLookAndFeel         lookAndFeel_;
    std::unique_ptr<DashboardWindow> dashboardWindow_;
    std::unique_ptr<MainWindow>      mainWindow_;
};

START_JUCE_APPLICATION (TwoSpeakerApp)

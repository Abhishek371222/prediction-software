#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include "MicListener.h"

// ---------------------------------------------------------------------------
// The window behind "Listen here". Small on purpose: a transport, an A/B, and
// the three caveats that stop this being read as an auralisation of a room.
// ---------------------------------------------------------------------------
/** The response you are listening through, drawn. Log frequency across,
    dB up: the same numbers the filters are built from, so the picture cannot
    disagree with the sound. */
class ResponsePlot final : public juce::Component
{
public:
    void setCurve (std::vector<MicListener::Point> c, bool absolute)
    {
        curve_ = std::move (c);
        absolute_ = absolute;
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (Brand::plotBg());
        g.fillRect (r);
        g.setColour (Brand::border().withAlpha (0.5f));
        g.drawRect (r, 1.0f);

        if (curve_.size() < 2)
        {
            g.setColour (Brand::muted());
            g.setFont (Brand::tech (Brand::UI::scaledFont (12.0f)));
            g.drawText ("No response at this point", getLocalBounds(),
                        juce::Justification::centred);
            return;
        }

        const double f0 = curve_.front().hz, f1 = curve_.back().hz;
        float lo = curve_.front().db, hi = lo;
        for (const auto& p : curve_) { lo = juce::jmin (lo, p.db); hi = juce::jmax (hi, p.db); }
        // Always show at least 24 dB, or a flat response looks like noise.
        const float mid = 0.5f * (lo + hi);
        const float half = juce::jmax (12.0f, 0.6f * (hi - lo));
        lo = mid - half; hi = mid + half;

        auto xOf = [&] (double hz)
        {
            const double t = (std::log10 (hz) - std::log10 (f0))
                           / juce::jmax (1.0e-6, std::log10 (f1) - std::log10 (f0));
            return r.getX() + (float) t * r.getWidth();
        };
        auto yOf = [&] (float db)
        {
            return r.getBottom() - (db - lo) / (hi - lo) * r.getHeight();
        };

        // decade gridlines, labelled - an unlabelled curve is a decoration
        g.setFont (Brand::tech (juce::jmax (9.0f, Brand::UI::scaledFont (10.0f))));
        for (double hz : { 20.0, 50.0, 100.0, 200.0, 500.0, 1000.0, 2000.0,
                           5000.0, 10000.0 })
        {
            if (hz < f0 || hz > f1) continue;
            const float x = xOf (hz);
            g.setColour (Brand::plotGrid().withAlpha (0.5f));
            g.drawVerticalLine ((int) x, r.getY(), r.getBottom() - 12.0f);
            g.setColour (Brand::muted());
            const int lw = 36;
            const int lx = juce::jlimit ((int) r.getX(), (int) r.getRight() - lw,
                                         (int) x - lw / 2);
            g.drawText (hz >= 1000.0 ? juce::String ((int) (hz / 1000.0)) + "k"
                                     : juce::String ((int) hz),
                        lx, (int) r.getBottom() - 12, lw, 12,
                        juce::Justification::centred);
        }

        juce::Path path;
        for (size_t i = 0; i < curve_.size(); ++i)
        {
            const float x = xOf (curve_[i].hz), y = yOf (curve_[i].db);
            if (i == 0) path.startNewSubPath (x, y); else path.lineTo (x, y);
        }
        g.setColour (Brand::accent());
        g.strokePath (path, juce::PathStrokeType (2.0f));
        for (const auto& pt : curve_)
            g.fillEllipse (xOf (pt.hz) - 2.0f, yOf (pt.db) - 2.0f, 4.0f, 4.0f);

        g.setColour (Brand::muted());
        g.drawText (juce::String ((int) std::round (hi)) + (absolute_ ? " dB" : " dB rel"),
                    (int) r.getX() + 4, (int) r.getY() + 2, 90, 12,
                    juce::Justification::centredLeft);
        g.drawText (juce::String ((int) std::round (lo)), (int) r.getX() + 4,
                    (int) r.getBottom() - 26, 90, 12, juce::Justification::centredLeft);
    }

private:
    std::vector<MicListener::Point> curve_;
    bool absolute_ = false;
};

class MicListenPanel final : public juce::Component
{
public:
    MicListenPanel()
    {
        auto styleBtn = [this] (juce::TextButton& b, const juce::String& t)
        {
            b.setButtonText (t);
            b.setColour (juce::TextButton::buttonColourId, Brand::panelDark());
            b.setColour (juce::TextButton::buttonOnColourId, Brand::accent().withAlpha (0.35f));
            b.setColour (juce::TextButton::textColourOffId, Brand::text());
            b.setColour (juce::TextButton::textColourOnId,  Brand::text());
            addAndMakeVisible (b);
        };
        styleBtn (playBtn_, "Play");
        playBtn_.setClickingTogglesState (true);
        playBtn_.onClick = [this]
        {
            if (playBtn_.getToggleState()) listener_.start();
            else                           listener_.stop();
            playBtn_.setButtonText (playBtn_.getToggleState() ? "Stop" : "Play");
            refresh();
        };

        styleBtn (srcBtn_, "Source: noise");
        srcBtn_.setClickingTogglesState (true);
        srcBtn_.setTooltip ("Noise shows you the response; music tells you whether "
                            "you would want to sit here.");
        srcBtn_.onClick = [this]
        {
            listener_.setUseTrack (srcBtn_.getToggleState());
            refreshSource();
        };

        styleBtn (loadBtn_, "Load track...");
        loadBtn_.onClick = [this] { chooseTrack(); };

        styleBtn (followBtn_, "Follow mic");
        followBtn_.setClickingTogglesState (true);
        followBtn_.setToggleState (true, juce::dontSendNotification);
        followBtn_.setTooltip ("Keep what you hear in step with the prediction - "
                               "as you drag the mic, and as you change the rig "
                               "(filters, gain, delay, positions). Switch off to "
                               "hold what you have and re-predict only when you ask.");
        followBtn_.onClick = [this] { refreshFollow(); };

        styleBtn (calcBtn_, "Recalculate");
        calcBtn_.setTooltip ("Predict again at the mic's current position.");
        calcBtn_.onClick = [this] { if (onRecalculate) onRecalculate(); };

        styleBtn (abBtn_, "Compare: flat");
        abBtn_.setClickingTogglesState (true);
        abBtn_.setTooltip ("Hold the comparison on to hear the same noise without "
                           "the prediction applied.");
        abBtn_.onClick = [this] { listener_.setBypass (abBtn_.getToggleState()); };

        auto styleLbl = [this] (juce::Label& l, float sz, juce::Colour c)
        {
            l.setFont (Brand::tech (Brand::UI::scaledFont (sz)));
            l.setColour (juce::Label::textColourId, c);
            l.setJustificationType (juce::Justification::topLeft);
            l.setBorderSize ({});
            addAndMakeVisible (l);
        };
        addAndMakeVisible (plot_);
        styleLbl (titleLbl_,  15.0f, Brand::text());
        styleLbl (levelLbl_,  14.0f, Brand::accent());
        styleLbl (trackLbl_,  12.0f, Brand::accent());
        styleLbl (caveatLbl_, 12.0f, Brand::muted());
        caveatLbl_.setJustificationType (juce::Justification::topLeft);
    }

    ~MicListenPanel() override { listener_.stop(); }

    /** Asked for by the panel's own Recalculate button, and by Follow mic. */
    std::function<void()> onRecalculate;

    /** Silence it and put the transport back to where it stops. Used when the
        window is closed and when the mic being listened to is deleted - in
        both cases there is no longer anything on screen that says what you are
        hearing, or any way to stop it. */
    void stopPlayback()
    {
        listener_.stop();
        playBtn_.setToggleState (false, juce::dontSendNotification);
        playBtn_.setButtonText ("Play");
    }

    bool isFollowing() const noexcept { return followBtn_.getToggleState(); }

    /** The mic moved: same mic, new numbers. Everything about the window that
        is not the curve - the track, the transport, the title - is left alone,
        so the sound shifts without the playback being interrupted. */
    void updateCurve (std::vector<MicListener::Point> curve, float refDb)
    {
        if (curve.empty()) return;
        plot_.setCurve (curve, absolute_);
        // Keeps the yardstick set when this window opened, so the move is
        // heard as a move.
        listener_.updateResponse (std::move (curve));
        juce::ignoreUnused (refDb);
        refresh();
    }

    /** @param curve   predicted level per catalogue frequency at this mic
        @param refDb   level that plays at full scale (the map's peak)
        @param absolute true when the figures are real dB SPL
        @param micName what to call this mic in the title */
    void setMic (const juce::String& micName,
                 std::vector<MicListener::Point> curve,
                 float refDb, bool absolute)
    {
        absolute_ = absolute;
        double lo = 0.0, hi = 0.0;
        if (! curve.empty())
        {
            lo = curve.front().hz;
            hi = curve.back().hz;
        }
        plot_.setCurve (curve, absolute);
        listener_.setResponse (curve, refDb);
        refreshFollow();

        // Load a track the first time the window is used, so "Listen" plays
        // music straight away rather than making you go and find a file.
        if (! listener_.hasTrack())
            tryDefaultTrack();
        refreshSource();

        titleLbl_.setText ("Listening at " + micName, juce::dontSendNotification);
        caveatLbl_.setText (
            juce::String ("Rendered band ")
                + juce::String ((int) (lo + 0.5)) + " - " + juce::String ((int) (hi + 0.5))
                + " Hz, the measured catalogue for this device.\n"
                  "Direct sound only: no room, no reflections, no air absorption.\n"
                + (absolute_ ? "Levels are calibrated dB SPL. Loudness is judged "
                               "against the spot you opened this window at."
                             : "Levels are RELATIVE - this device has no sensitivity "
                               "figure, so absolute loudness is unknown."),
            juce::dontSendNotification);
        refresh();
    }

    void paint (juce::Graphics& g) override
    {
        g.fillAll (Brand::panel());
        g.setColour (Brand::border().withAlpha (0.4f));
        g.drawHorizontalLine (titleLbl_.getBottom() + 6, 0.0f, (float) getWidth());
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (UiConfig::Scale::px (14));
        const int rowH = UiConfig::Scale::px (26);
        titleLbl_.setBounds (r.removeFromTop (UiConfig::Scale::px (20)));
        r.removeFromTop (UiConfig::Scale::px (12));
        levelLbl_.setBounds (r.removeFromTop (UiConfig::Scale::px (20)));
        r.removeFromTop (UiConfig::Scale::px (8));
        auto row = r.removeFromTop (rowH);
        const int halfW = (row.getWidth() - UiConfig::Scale::px (8)) / 2;
        playBtn_.setBounds (row.removeFromLeft (halfW));
        row.removeFromLeft (UiConfig::Scale::px (8));
        abBtn_.setBounds (row);

        r.removeFromTop (UiConfig::Scale::px (8));
        auto row2 = r.removeFromTop (rowH);
        const int halfW2 = (row2.getWidth() - UiConfig::Scale::px (8)) / 2;
        srcBtn_.setBounds (row2.removeFromLeft (halfW2));
        row2.removeFromLeft (UiConfig::Scale::px (8));
        loadBtn_.setBounds (row2);

        r.removeFromTop (UiConfig::Scale::px (8));
        auto row3 = r.removeFromTop (rowH);
        const int halfW3 = (row3.getWidth() - UiConfig::Scale::px (8)) / 2;
        followBtn_.setBounds (row3.removeFromLeft (halfW3));
        row3.removeFromLeft (UiConfig::Scale::px (8));
        calcBtn_.setBounds (row3);

        r.removeFromTop (UiConfig::Scale::px (6));
        trackLbl_.setBounds (r.removeFromTop (UiConfig::Scale::px (14)));
        r.removeFromTop (UiConfig::Scale::px (6));
        plot_.setBounds (r.removeFromTop (juce::jmax (UiConfig::Scale::px (90),
                                                      r.getHeight() - UiConfig::Scale::px (62))));
        r.removeFromTop (UiConfig::Scale::px (8));
        caveatLbl_.setBounds (r);
    }

private:
    void tryDefaultTrack()
    {
        // A convenience, not a dependency: if this folder happens to hold
        // audio, take the first file so the tool is useful on first open.
        // Anything else comes from "Load track...".
        const juce::File dir (juce::File::getSpecialLocation (juce::File::userHomeDirectory)
                                  .getChildFile ("Downloads").getChildFile ("Songs"));
        if (! dir.isDirectory())
        {
            trackStatus_ = "No ~/Downloads/Songs folder - use Load track...";
            return;
        }

        // Everything in the folder, filtered by extension here rather than by
        // a multi-pattern wildcard: one less thing between the file and the
        // decoder when it does not load.
        int seen = 0;
        for (const auto& f : dir.findChildFiles (juce::File::findFiles, false))
        {
            const auto ext = f.getFileExtension().toLowerCase();
            if (ext != ".mp3" && ext != ".wav" && ext != ".aiff" && ext != ".aif"
                && ext != ".ogg" && ext != ".flac")
                continue;
            ++seen;
            if (listener_.loadTrack (f))
            {
                srcBtn_.setToggleState (true, juce::dontSendNotification);
                listener_.setUseTrack (true);
                trackStatus_.clear();
                return;
            }
            trackStatus_ = "Could not decode " + f.getFileName();
        }
        if (seen == 0) trackStatus_ = "No audio files in ~/Downloads/Songs";
    }

    void chooseTrack()
    {
        chooser_ = std::make_unique<juce::FileChooser> (
            "Choose a track to listen with",
            juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Downloads"),
            "*.mp3;*.wav;*.aiff;*.aif;*.ogg;*.flac");
        chooser_->launchAsync (juce::FileBrowserComponent::openMode
                                 | juce::FileBrowserComponent::canSelectFiles,
            [this] (const juce::FileChooser& fc)
            {
                const auto f = fc.getResult();
                if (f == juce::File()) return;
                if (listener_.loadTrack (f))
                {
                    srcBtn_.setToggleState (true, juce::dontSendNotification);
                    listener_.setUseTrack (true);
                    trackStatus_.clear();
                }
                else
                {
                    trackStatus_ = "Could not decode " + f.getFileName();
                }
                refreshSource();
            });
    }

    void refreshSource()
    {
        srcBtn_.setButtonText (listener_.usingTrack()
                                 ? "Source: " + listener_.trackName()
                                 : "Source: pink noise");
        srcBtn_.setEnabled (listener_.hasTrack());
        // Say WHY there is no track, rather than just greying the button and
        // leaving you to guess.
        trackLbl_.setText (trackStatus_, juce::dontSendNotification);
        trackLbl_.setVisible (trackStatus_.isNotEmpty());
    }

    void refreshFollow()
    {
        // Following makes the button redundant, so it greys rather than
        // disappearing - you can see what it would do before you need it.
        calcBtn_.setEnabled (! followBtn_.getToggleState());
        followBtn_.setButtonText (followBtn_.getToggleState() ? "Following mic"
                                                              : "Follow mic");
    }

    void refresh()
    {
        const auto err = listener_.getLastError();
        if (err.isNotEmpty())
        {
            levelLbl_.setText ("No audio device: " + err, juce::dontSendNotification);
            return;
        }
        const float off = listener_.levelOffsetDb();
        levelLbl_.setText (juce::String (off, 1)
                               + " dB relative to where you started listening",
                           juce::dontSendNotification);
    }

    MicListener      listener_;
    ResponsePlot     plot_;
    juce::TextButton playBtn_, abBtn_, srcBtn_, loadBtn_, followBtn_, calcBtn_;
    std::unique_ptr<juce::FileChooser> chooser_;
    juce::Label      trackLbl_;
    juce::String     trackStatus_;
    juce::Label      titleLbl_, levelLbl_, caveatLbl_;
    bool             absolute_ = false;
};

class MicListenWindow : public juce::DocumentWindow
{
public:
    MicListenWindow()
        : DocumentWindow ("Listen", Brand::panelDark(), DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setResizable (true, false);
        setContentNonOwned (&content, true);
        setResizeLimits (430, 380, 980, 760);
        centreWithSize (540, 430);
    }

    // Closing must silence it - a hidden window still holding the audio device
    // would keep playing with nothing on screen to stop it.
    void closeButtonPressed() override
    {
        content.stopPlayback();
        setVisible (false);
    }

    MicListenPanel content;
};

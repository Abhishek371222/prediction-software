#pragma once
#include <JuceHeader.h>
#include "BrandTheme.h"
#include <vector>
#include <cmath>

// ---------------------------------------------------------------------------
// MicListener - plays what the MODEL predicts at a mic position.
//
// How it works, and what that does and does not mean:
//
//   Pink noise is passed through one peaking filter per measured frequency,
//   each set to the level the engine predicts at that mic. The predicted level
//   already contains the summation of every speaker - so comb filtering,
//   nulls and power alleys are in the curve, and therefore in what you hear.
//
//   It is NOT an auralisation of a venue. There is no room, no reflections and
//   no air absorption: this is direct sound only. The rendered band is limited
//   to the device's measured catalogue, and the level is relative unless the
//   device is calibrated. Those three facts are shown in the window, because a
//   listening tool is believed further than a number is.
// ---------------------------------------------------------------------------
class MicListener final : public juce::AudioIODeviceCallback
{
public:
    /** One measured point: frequency and the predicted level there. */
    struct Point { double hz = 0.0; float db = 0.0f; };

    MicListener()
    {
        formats_.registerBasicFormats();          // WAV / AIFF / MP3 / Ogg
        readAhead_.startThread (juce::Thread::Priority::normal);
    }

    ~MicListener() override
    {
        stop();
        transport_.setSource (nullptr);
        readAhead_.stopThread (2000);
    }

    /** Stream a track instead of noise. Music is the honest check - noise
        shows you the response, music tells you whether you would sit there. */
    bool loadTrack (const juce::File& f)
    {
        std::unique_ptr<juce::AudioFormatReader> r (formats_.createReaderFor (f));
        if (r == nullptr) return false;

        auto src = std::make_unique<juce::AudioFormatReaderSource> (r.release(), true);
        src->setLooping (true);
        {
            const juce::SpinLock::ScopedLockType lock (lock_);
            transport_.setSource (src.get(), 32768, &readAhead_,
                                  0.0, 2);
            reader_ = std::move (src);
            trackName_ = f.getFileNameWithoutExtension();
        }
        transport_.setPosition (0.0);
        return true;
    }

    bool hasTrack() const noexcept { return reader_ != nullptr; }
    juce::String trackName() const { return trackName_; }
    void setUseTrack (bool on) noexcept { useTrack_ = on && reader_ != nullptr; }
    bool usingTrack() const noexcept { return useTrack_; }

    /** Start listening somewhere: this position becomes the level everything
        afterwards is judged against.
        @param curve      predicted level per catalogue frequency at this mic
        @param referenceDb the level that plays at full scale */
    void setResponse (std::vector<Point> curve, float referenceDb)
    {
        curve_ = std::move (curve);
        reference_ = referenceDb;
        rebuild();
    }

    /** The mic MOVED: new numbers, same yardstick. Keeping the reference is
        the whole point - renormalising to each new position would make a null
        sound exactly as loud as a hot spot, which is the opposite of what a
        listening tool is for. */
    void updateResponse (std::vector<Point> curve)
    {
        curve_ = std::move (curve);
        rebuild();
    }

    void start()
    {
        if (running_) return;
        // 2 channels out, none in: we are a source, and asking for input would
        // make Windows prompt for microphone permission for no reason.
        const auto err = devices_.initialiseWithDefaultDevices (0, 2);
        if (err.isNotEmpty()) { lastError_ = err; return; }
        // An empty error is not proof of a device: with no backends compiled
        // in, initialise succeeds and leaves you with nothing playing. Ask for
        // the device itself.
        if (devices_.getCurrentAudioDevice() == nullptr)
        {
            lastError_ = "no output device available";
            return;
        }
        lastError_.clear();
        devices_.addAudioCallback (this);
        running_ = true;
        if (useTrack_ && reader_ != nullptr) transport_.start();
    }

    void stop()
    {
        if (! running_) return;
        transport_.stop();
        devices_.removeAudioCallback (this);
        devices_.closeAudioDevice();
        running_ = false;
    }

    bool isRunning()  const noexcept { return running_; }
    void setBypass (bool b) noexcept { bypass_ = b; }   // A/B against flat noise
    juce::String getLastError() const { return lastError_; }

    /** Overall offset applied, in dB - what the window reports to the user. */
    float levelOffsetDb() const noexcept { return offsetDb_; }

    /** The response being applied, for the plot: you should be able to SEE
        what you are hearing. */
    const std::vector<Point>& response() const noexcept { return curve_; }
    float referenceDb() const noexcept { return reference_; }

    // --- AudioIODeviceCallback --------------------------------------------
    void audioDeviceAboutToStart (juce::AudioIODevice* d) override
    {
        sampleRate_ = d != nullptr ? d->getCurrentSampleRate() : 48000.0;
        const int block = d != nullptr ? d->getCurrentBufferSizeSamples() : 512;
        transport_.prepareToPlay (block, sampleRate_);
        scratch_.setSize (2, juce::jmax (block, 1024));
        rebuild();
    }

    void audioDeviceStopped() override {}

    void audioDeviceIOCallbackWithContext (const float* const*, int,
                                           float* const* out, int numOut,
                                           int numSamples,
                                           const juce::AudioIODeviceCallbackContext&) override
    {
        const juce::SpinLock::ScopedTryLockType lock (lock_);
        if (! lock.isLocked())
        {
            for (int ch = 0; ch < numOut; ++ch)
                juce::FloatVectorOperations::clear (out[ch], numSamples);
            return;
        }

        // Pull the track once per block; its own sample rate is resampled by
        // the transport, so a 44.1k file on a 48k device still plays in tune.
        if (useTrack_ && reader_ != nullptr)
        {
            if (scratch_.getNumSamples() < numSamples) scratch_.setSize (2, numSamples, false, false, true);
            juce::AudioBuffer<float> view (scratch_.getArrayOfWritePointers(), 2, numSamples);
            juce::AudioSourceChannelInfo info (&view, 0, numSamples);
            transport_.getNextAudioBlock (info);
        }

        for (int i = 0; i < numSamples; ++i)
        {
            // Mono sum: the prediction is one point in space, so there is only
            // one thing to hear at it.
            float s = (useTrack_ && reader_ != nullptr)
                        ? 0.5f * (scratch_.getSample (0, i) + scratch_.getSample (1, i))
                        : pink();
            if (! bypass_)
            {
                for (auto& f : filters_) s = f.process (s);
                // Glide to the new level rather than stepping to it: moving a
                // mic from a hot spot into a null is a 20 dB change, and a
                // step that size is a bang, not a quieter signal.
                gainNow_ += (gain_ - gainNow_) * kGainGlide;
                s *= gainNow_;
            }
            else
            {
                s *= 0.25f;   // same ballpark as the shaped path, for a fair A/B
            }
            if (useTrack_) s *= 2.0f;   // music is quieter than full-scale noise
            s = juce::jlimit (-1.0f, 1.0f, s);
            for (int ch = 0; ch < numOut; ++ch)
                if (out[ch] != nullptr) out[ch][i] = s;
        }
    }

private:
    // --- a peaking (bell) filter, one per measured frequency ---------------
    struct Peak
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float x1 = 0, x2 = 0, y1 = 0, y2 = 0;

        /** @param keepState true while the mic is being dragged: the filter
                   keeps the samples already inside it, so new coefficients
                   slide in instead of the bank restarting from silence with
                   an audible click on every mouse move. */
        void set (double fs, double f0, double gainDb, double q,
                  bool keepState = false)
        {
            const double A = std::pow (10.0, gainDb / 40.0);
            const double w = 2.0 * juce::MathConstants<double>::pi * f0 / fs;
            const double alpha = std::sin (w) / (2.0 * q);
            const double cosw = std::cos (w);
            const double a0 =  1 + alpha / A;
            b0 = (float) ((1 + alpha * A) / a0);
            b1 = (float) ((-2 * cosw)     / a0);
            b2 = (float) ((1 - alpha * A) / a0);
            a1 = (float) ((-2 * cosw)     / a0);
            a2 = (float) ((1 - alpha / A) / a0);
            if (! keepState) x1 = x2 = y1 = y2 = 0;
        }

        float process (float x) noexcept
        {
            const float y = b0 * x + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
            x2 = x1; x1 = x; y2 = y1; y1 = y;
            return y;
        }
    };

    void rebuild()
    {
        const juce::SpinLock::ScopedLockType lock (lock_);
        gain_ = 0.0f;
        offsetDb_ = 0.0f;
        if (curve_.empty() || sampleRate_ < 8000.0) { filters_.clear(); return; }

        // The shape goes in the filters and the overall level goes in one gain.
        //
        // The shape is referred to the curve's PEAK, not its mean, so every
        // filter is a cut and the bank can only ever take away. Referred to
        // the mean, every band above it was a boost; a dozen boosting bells in
        // series multiplied the signal into the limiter and pinned it there,
        // and once you are clipping, dropping the level 7 dB sounds like
        // nothing at all. That is why moving the mic changed the graph but not
        // the sound. All the loudness now lives in the one gain below, which
        // is the thing that actually tracks where the mic is.
        double peak = -1.0e9;
        for (const auto& p : curve_) peak = juce::jmax (peak, (double) p.db);

        // Which bands this curve needs. Dragging a mic around changes the
        // LEVELS but never the catalogue, so the bank is almost always the
        // one already running - and re-tuning it in place is what makes a
        // move audible as a move rather than as a series of clicks.
        std::vector<double> wanted;
        wanted.reserve (curve_.size());
        for (const auto& p : curve_)
            if (p.hz >= 20.0 && p.hz <= sampleRate_ * 0.45)
                wanted.push_back (p.hz);

        const bool sameBank = (wanted == filterHz_ && filters_.size() == wanted.size());
        if (! sameBank)
        {
            filters_.assign (wanted.size(), Peak());
            filterHz_ = wanted;
        }

        size_t fi = 0;
        for (const auto& p : curve_)
        {
            if (p.hz < 20.0 || p.hz > sampleRate_ * 0.45) continue;
            if (fi >= filters_.size()) break;
            // Wide-ish bells so neighbouring bands join up instead of ringing.
            filters_[fi++].set (sampleRate_, p.hz,
                                juce::jlimit (-24.0, 0.0, (double) p.db - peak),
                                1.2, sameBank);
        }

        // Measured against where listening STARTED, so moving the mic
        // somewhere quieter actually sounds quieter - and somewhere louder,
        // louder. The headroom above the anchor is limited to +6 dB, which
        // with the 0.25 base keeps the peak at half scale and makes clipping
        // impossible however hot a spot you drag into.
        offsetDb_ = juce::jlimit (-60.0f, 6.0f, (float) peak - reference_);
        gain_ = 0.25f * std::pow (10.0f, offsetDb_ / 20.0f);
    }

    float pink() noexcept
    {
        // Voss-McCartney-ish: cheap, and pink is the right test signal because
        // it puts equal energy in every octave.
        const float w = rng_.nextFloat() * 2.0f - 1.0f;
        b0_ = 0.99886f * b0_ + w * 0.0555179f;
        b1_ = 0.99332f * b1_ + w * 0.0750759f;
        b2_ = 0.96900f * b2_ + w * 0.1538520f;
        b3_ = 0.86650f * b3_ + w * 0.3104856f;
        b4_ = 0.55000f * b4_ + w * 0.5329522f;
        b5_ = -0.7616f * b5_ - w * 0.0168980f;
        const float out = b0_ + b1_ + b2_ + b3_ + b4_ + b5_ + b6_ + w * 0.5362f;
        b6_ = w * 0.115926f;
        return out * 0.11f;
    }

    juce::AudioDeviceManager devices_;
    juce::AudioFormatManager  formats_;
    juce::TimeSliceThread     readAhead_ { "listen-readahead" };
    juce::AudioTransportSource transport_;
    std::unique_ptr<juce::AudioFormatReaderSource> reader_;
    juce::AudioBuffer<float>  scratch_;
    juce::String              trackName_;
    bool   useTrack_   = false;
    std::vector<Point>  curve_;
    std::vector<Peak>   filters_;
    std::vector<double> filterHz_;      // what the bank is tuned to right now
    juce::SpinLock      lock_;
    juce::Random        rng_;
    double sampleRate_ = 48000.0;
    float  reference_  = 0.0f;
    float  gain_       = 0.0f;   // where the level is heading
    float  gainNow_    = 0.0f;   // where it has got to
    // ~20 ms at 48k: fast enough to feel like the move, slow enough not to click.
    static constexpr float kGainGlide = 0.001f;
    float  offsetDb_   = 0.0f;
    bool   running_    = false;
    bool   bypass_     = false;
    juce::String lastError_;
    float b0_ = 0, b1_ = 0, b2_ = 0, b3_ = 0, b4_ = 0, b5_ = 0, b6_ = 0;
};

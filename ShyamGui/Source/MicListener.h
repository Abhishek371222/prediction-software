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

    MicListener() = default;
    ~MicListener() override { stop(); }

    /** @param curve      predicted level per catalogue frequency at this mic
        @param referenceDb level that should play at full scale (the map's peak) */
    void setResponse (std::vector<Point> curve, float referenceDb)
    {
        curve_ = std::move (curve);
        reference_ = referenceDb;
        rebuild();
    }

    void start()
    {
        if (running_) return;
        // 2 channels out, none in: we are a source, and asking for input would
        // make Windows prompt for microphone permission for no reason.
        const auto err = devices_.initialiseWithDefaultDevices (0, 2);
        if (err.isNotEmpty()) { lastError_ = err; return; }
        devices_.addAudioCallback (this);
        running_ = true;
    }

    void stop()
    {
        if (! running_) return;
        devices_.removeAudioCallback (this);
        devices_.closeAudioDevice();
        running_ = false;
    }

    bool isRunning()  const noexcept { return running_; }
    void setBypass (bool b) noexcept { bypass_ = b; }   // A/B against flat noise
    juce::String getLastError() const { return lastError_; }

    /** Overall offset applied, in dB - what the window reports to the user. */
    float levelOffsetDb() const noexcept { return offsetDb_; }

    // --- AudioIODeviceCallback --------------------------------------------
    void audioDeviceAboutToStart (juce::AudioIODevice* d) override
    {
        sampleRate_ = d != nullptr ? d->getCurrentSampleRate() : 48000.0;
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

        for (int i = 0; i < numSamples; ++i)
        {
            float s = pink();
            if (! bypass_)
            {
                for (auto& f : filters_) s = f.process (s);
                s *= gain_;
            }
            else
            {
                s *= 0.25f;   // same ballpark as the shaped path, for a fair A/B
            }
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

        void set (double fs, double f0, double gainDb, double q)
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
            x1 = x2 = y1 = y2 = 0;
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
        filters_.clear();
        gain_ = 0.0f;
        offsetDb_ = 0.0f;
        if (curve_.empty() || sampleRate_ < 8000.0) return;

        // The shape goes in the filters and the overall level goes in one gain.
        // Folding a large common offset into every bell would stack it.
        double mean = 0.0;
        for (const auto& p : curve_) mean += p.db;
        mean /= (double) curve_.size();

        for (const auto& p : curve_)
        {
            if (p.hz < 20.0 || p.hz > sampleRate_ * 0.45) continue;
            Peak f;
            // Wide-ish bells so neighbouring bands join up instead of ringing.
            f.set (sampleRate_, p.hz, juce::jlimit (-24.0, 24.0, (double) p.db - mean), 1.2);
            filters_.push_back (f);
        }

        // Never louder than the loudest point on the map, so moving the mic
        // somewhere quieter actually sounds quieter.
        offsetDb_ = juce::jlimit (-60.0f, 0.0f, (float) mean - reference_);
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
    std::vector<Point>  curve_;
    std::vector<Peak>   filters_;
    juce::SpinLock      lock_;
    juce::Random        rng_;
    double sampleRate_ = 48000.0;
    float  reference_  = 0.0f;
    float  gain_       = 0.0f;
    float  offsetDb_   = 0.0f;
    bool   running_    = false;
    bool   bypass_     = false;
    juce::String lastError_;
    float b0_ = 0, b1_ = 0, b2_ = 0, b3_ = 0, b4_ = 0, b5_ = 0, b6_ = 0;
};

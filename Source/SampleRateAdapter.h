#pragma once

#include <juce_dsp/juce_dsp.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include "CloudsEngine.h"

class SampleRateAdapter
{
public:
    SampleRateAdapter() = default;
    ~SampleRateAdapter() = default;

    /// Call from prepareToPlay
    void prepare(double hostSampleRate, int maxBlockSize);

    /// Resample host audio → 32kHz, process through engine, resample back.
    /// inL/inR and outL/outR are host-rate buffers of numSamples length.
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 int numSamples,
                 CloudsEngine& engine);

    static constexpr double kInternalSampleRate = 32000.0;

private:
    double hostSampleRate_ = 44100.0;
    double ratio_ = 1.0;  // hostSR / internalSR

    // Resamplers: host → internal (down) and internal → host (up)
    juce::LagrangeInterpolator interpDownL_, interpDownR_;
    juce::LagrangeInterpolator interpUpL_, interpUpR_;

    // Internal-rate buffers
    juce::AudioBuffer<float> internalInput_;
    juce::AudioBuffer<float> internalOutput_;

    // Maximum sizes
    int maxInternalSamples_ = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleRateAdapter)
};

#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include "CloudsEngine.h"
#include <cmath>
#include <cstring>

class SampleRateAdapter
{
public:
    SampleRateAdapter() = default;
    ~SampleRateAdapter() = default;

    void prepare(double hostSampleRate, int maxBlockSize);

    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 int numSamples,
                 CloudsEngine& engine);

    static constexpr double kInternalSampleRate = 32000.0;
    static constexpr int kBlockSize = 32;

private:
    double hostSampleRate_ = 44100.0;
    double ratio_ = 1.0;

    double inputPhase_ = 0.0;

    static constexpr int kInputRingSize = 16384;
    float inputRingL_[kInputRingSize] = {};
    float inputRingR_[kInputRingSize] = {};
    int inputWritePos_ = 0;
    int inputSamplesAvailable_ = 0;

    double outputPhase_ = 0.0;

    static constexpr int kOutputRingSize = 16384;
    float outputRingL_[kOutputRingSize] = {};
    float outputRingR_[kOutputRingSize] = {};
    int outputWritePos_ = 0;
    int outputReadPos_ = 0;
    int outputSamplesAvailable_ = 0;

    float engineInL_[kBlockSize] = {};
    float engineInR_[kBlockSize] = {};
    float engineOutL_[kBlockSize] = {};
    float engineOutR_[kBlockSize] = {};

    float lastOutputL_ = 0.0f;
    float lastOutputR_ = 0.0f;

    static inline float hermite(float xm1, float x0, float x1, float x2, float t)
    {
        float c = (x1 - xm1) * 0.5f;
        float v = x0 - x1;
        float w = c + v;
        float a = w + v + (x2 - x0) * 0.5f;
        float b_neg = w + a;
        return ((a * t - b_neg) * t + c) * t + x0;
    }

    inline float readInputRing(const float* ring, int basePos, float frac) const
    {
        int im1 = (basePos - 1 + kInputRingSize) % kInputRingSize;
        int i0  = basePos % kInputRingSize;
        int i1  = (basePos + 1) % kInputRingSize;
        int i2  = (basePos + 2) % kInputRingSize;
        return hermite(ring[im1], ring[i0], ring[i1], ring[i2], frac);
    }

    inline float readOutputRing(const float* ring, int basePos, float frac) const
    {
        int im1 = (basePos - 1 + kOutputRingSize) % kOutputRingSize;
        int i0  = basePos % kOutputRingSize;
        int i1  = (basePos + 1) % kOutputRingSize;
        int i2  = (basePos + 2) % kOutputRingSize;
        return hermite(ring[im1], ring[i0], ring[i1], ring[i2], frac);
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SampleRateAdapter)
};

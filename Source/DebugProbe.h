#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

// DebugProbe is only active in debug builds to avoid CPU overhead in release
class DebugProbe
{
public:
    DebugProbe(const char* name) : name_(name) {}

    void measureStereo(const float* L, const float* R, int numSamples,
                       int printInterval = 200)
    {
#ifndef JUCE_DEBUG
        // Release builds: completely disable to avoid CPU overhead
        juce::ignoreUnused(L);
        juce::ignoreUnused(R);
        juce::ignoreUnused(numSamples);
        juce::ignoreUnused(printInterval);
        return;
#else
        if (numSamples <= 0) return;

        float sum = 0.0f;
        float peak = 0.0f;
        for (int i = 0; i < numSamples; ++i)
        {
            float v = (std::abs(L[i]) + std::abs(R[i])) * 0.5f;
            sum += v * v;
            if (v > peak) peak = v;
        }
        lastRms_ = std::sqrt(sum / static_cast<float>(numSamples));
        lastPeak_ = peak;

        if (++blockCount_ >= printInterval)
        {
            blockCount_ = 0;
            juce::String msg = juce::String("[PROBE ") + name_ + "] "
                + "RMS=" + juce::String(lastRms_, 6)
                + "  Peak=" + juce::String(lastPeak_, 6)
                + "  (" + (lastPeak_ > 0.0001f ? "SIGNAL" : "SILENT") + ")";

            DBG(msg);
        }
#endif
    }

    float lastRms() const { return lastRms_; }
    float lastPeak() const { return lastPeak_; }

private:
    const char* name_;
    float lastRms_ = 0.0f;
    float lastPeak_ = 0.0f;
    int blockCount_ = 0;
};

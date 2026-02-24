#pragma once

#include <memory>
#include <cstring>
#include <juce_core/juce_core.h>

// Forward declaration — avoid pulling in full clouds headers here
namespace clouds {
    class GranularProcessor;
}

class CloudsEngine
{
public:
    CloudsEngine();
    ~CloudsEngine();

    /// Allocate buffers and initialise the GranularProcessor.
    void init();

    /// Process exactly kBlockSize (32) stereo frames.
    /// Input/output are interleaved L/R float pairs,
    /// but this method accepts split channels for convenience.
    void process(const float* inputL, const float* inputR,
                 float* outputL, float* outputR,
                 int numSamples);

    // --- Parameter setters (call before process) ---
    void setPosition(float v);
    void setSize(float v);
    void setPitch(float v);
    void setDensity(float v);
    void setTexture(float v);
    void setDryWet(float v);
    void setStereoSpread(float v);
    void setFeedback(float v);
    void setReverb(float v);
    void setFreeze(bool v);
    void setTrigger(bool v);
    void setPlaybackMode(int mode);   // 0-3
    void setQuality(int quality);     // 0-3

    static constexpr int kBlockSize = 32;

private:
    static constexpr size_t kLargeBufferSize = 118784;
    static constexpr size_t kSmallBufferSize = 65535;

    std::unique_ptr<uint8_t[]> largeBuffer_;
    std::unique_ptr<uint8_t[]> smallBuffer_;
    std::unique_ptr<clouds::GranularProcessor> processor_;

    bool initialised_ = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsEngine)
};

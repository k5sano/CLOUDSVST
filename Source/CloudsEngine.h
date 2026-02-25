#pragma once

#include <memory>
#include <cstring>
#include <atomic>
#include <juce_core/juce_core.h>

namespace clouds {
    class GranularProcessor;
}

class CloudsEngine
{
public:
    CloudsEngine();
    ~CloudsEngine();

    void init();

    void process(const float* inputL, const float* inputR,
                 float* outputL, float* outputR,
                 int numSamples);

    // --- Parameter setters ---
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
    void setQuality(int quality);
    void setPlaybackMode(int mode);

    void setInputTrim(float v)  { inputTrim_ = v; }
    void setOutputGain(float v) { outputGain_ = v; }

    // Set pointers to atomic meters owned by the Processor
    void setMeterPointers(std::atomic<float>* meterD, std::atomic<float>* meterE)
    {
        meterD_ = meterD;
        meterE_ = meterE;
    }

    static constexpr int kBlockSize = 32;

private:
    static constexpr size_t kLargeBufferSize = 118784;
    static constexpr size_t kSmallBufferSize = 65536 - 128;  // VCV Rack と同じ 65408

    std::unique_ptr<uint8_t[]> largeBuffer_;
    std::unique_ptr<uint8_t[]> smallBuffer_;
    std::unique_ptr<clouds::GranularProcessor> processor_;

    bool initialised_ = false;
    float inputTrim_ = 0.5f;
    float outputGain_ = 1.6f;

    // Non-owning pointers to processor's atomic meters (null = no metering)
    std::atomic<float>* meterD_ = nullptr;
    std::atomic<float>* meterE_ = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsEngine)
};

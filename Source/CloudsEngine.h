#pragma once

#include <memory>
#include <cstring>
#include <atomic>
#include <juce_core/juce_core.h>
#include "DebugProbe.h"

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

    // Set atomic meter pointers for real-time GUI updates
    void setMeterPointers(std::atomic<float>* d, std::atomic<float>* e);

    static constexpr int kBlockSize = 32;

private:
    static constexpr size_t kLargeBufferSize = 118784;
    static constexpr size_t kSmallBufferSize = 65535;

    std::unique_ptr<uint8_t[]> largeBuffer_;
    std::unique_ptr<uint8_t[]> smallBuffer_;
    std::unique_ptr<clouds::GranularProcessor> processor_;

    bool initialised_ = false;

    // Debug probes
    DebugProbe probeD_{"D:Engine_In"};
    DebugProbe probeE_{"E:Engine_Out"};

    // Atomic meter pointers for GUI
    std::atomic<float>* meterD_ = nullptr;
    std::atomic<float>* meterE_ = nullptr;

    // Stretch mode needs many Prepare() calls for correlator to converge
    int prepareCallsPerBlock_ = 1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsEngine)
};

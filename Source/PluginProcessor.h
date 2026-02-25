#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "CloudsEngine.h"
#include "SampleRateAdapter.h"
#include "DebugProbe.h"

class CloudsVSTProcessor : public juce::AudioProcessor
{
public:
    CloudsVSTProcessor();
    ~CloudsVSTProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "CloudsVST"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }

    // Meter values for GUI (atomic for lock-free read)
    std::atomic<float>& getMeterA() { return meterA_; }
    std::atomic<float>& getMeterB() { return meterB_; }
    std::atomic<float>& getMeterD() { return meterD_; }
    std::atomic<float>& getMeterE() { return meterE_; }
    std::atomic<float>& getMeterF() { return meterF_; }

private:
    juce::AudioProcessorValueTreeState apvts_;
    CloudsEngine engine_;
    SampleRateAdapter srcAdapter_;

    // Cached parameter pointers for real-time access
    std::atomic<float>* positionParam_     = nullptr;
    std::atomic<float>* sizeParam_         = nullptr;
    std::atomic<float>* pitchParam_        = nullptr;
    std::atomic<float>* densityParam_      = nullptr;
    std::atomic<float>* textureParam_      = nullptr;
    std::atomic<float>* dryWetParam_       = nullptr;
    std::atomic<float>* spreadParam_       = nullptr;
    std::atomic<float>* feedbackParam_     = nullptr;
    std::atomic<float>* reverbParam_       = nullptr;
    std::atomic<float>* freezeParam_       = nullptr;
    std::atomic<float>* triggerParam_      = nullptr;
    std::atomic<float>* modeParam_         = nullptr;
    std::atomic<float>* qualityParam_      = nullptr;
    std::atomic<float>* inputGainParam_    = nullptr;
    std::atomic<float>* inputTrimParam_    = nullptr;
    std::atomic<float>* outputGainParam_   = nullptr;

    // Debug probes
    DebugProbe probeA_{"A:Input"};
    DebugProbe probeB_{"B:PostGain"};
    DebugProbe probeF_{"F:Output"};

    // Buffer for avoiding aliasing
    juce::AudioBuffer<float> inputCopyBuffer_;

    // Real-time meter values (Peak level, 0.0 to 1.0)
    std::atomic<float> meterA_{0.f}, meterB_{0.f},
                       meterD_{0.f}, meterE_{0.f}, meterF_{0.f};

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsVSTProcessor)
};

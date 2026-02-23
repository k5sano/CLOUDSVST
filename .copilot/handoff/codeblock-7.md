Copy
#pragma once

#include <JuceHeader.h>
#include "clouds/dsp/granular_processor.h"

//==============================================================================
class CloudsAudioProcessor : public juce::AudioProcessor
{
public:
    CloudsAudioProcessor();
    ~CloudsAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
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

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

private:
    // ---- Clouds DSP ----
    clouds::GranularProcessor processor_;

    // Clouds は内部 32kHz 動作。large=118784, small=65536 が元ハード相当
    static constexpr size_t kLargeBufferSize = 118784;
    static constexpr size_t kSmallBufferSize = 65536;
    uint8_t largeBuffer_[kLargeBufferSize];
    uint8_t smallBuffer_[kSmallBufferSize];

    // DAW サンプルレート → 32kHz 変換用
    double hostSampleRate_ = 44100.0;
    static constexpr float kCloudsSampleRate = 32000.0f;

    // リサンプリング用リングバッファ
    std::vector<float> inputResampleBufL_;
    std::vector<float> inputResampleBufR_;
    std::vector<float> outputResampleBufL_;
    std::vector<float> outputResampleBufR_;
    double resamplePhase_ = 0.0;

    // JUCE パラメータ
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsAudioProcessor)
};

Copy
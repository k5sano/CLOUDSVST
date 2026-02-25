```cpp
Copy#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "DSP/MT2GainStage.h"
#include "DSP/MT2ToneStack.h"
#include "DSP/DiodeMorpher.h"

class MT2Plugin : public juce::AudioProcessor {
public:
    MT2Plugin();
    ~MT2Plugin() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "MetalCosmos"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    // Parameter pointers (atomic)
    std::atomic<float>* distParam      = nullptr;
    std::atomic<float>* levelParam     = nullptr;
    std::atomic<float>* diodeMorphParam  = nullptr;
    std::atomic<float>* diodeLinkParam   = nullptr;
    std::atomic<float>* diodeMorph2Param = nullptr;
    std::atomic<float>* eqLowParam     = nullptr;
    std::atomic<float>* eqMidParam     = nullptr;
    std::atomic<float>* eqMidFreqParam = nullptr;
    std::atomic<float>* eqMidQParam    = nullptr;
    std::atomic<float>* eqHighParam    = nullptr;

    // DSP
    MT2GainStage mGainStage;
    MT2ToneStack mToneStack;
    DiodeMorpher mDiodeMorpher;
    juce::dsp::Oversampling<double> mOversampling{2, 2,
        juce::dsp::Oversampling<double>::filterHalfBandPolyphaseIIR, true};

    // Internal double buffer for oversampled processing
    juce::AudioBuffer<double> mDoubleBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MT2Plugin)
};
Copy
```
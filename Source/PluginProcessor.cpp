#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Parameters.h"

namespace {
    using Layout = juce::AudioProcessorValueTreeState::ParameterLayout;
}

//==============================================================================
CloudsVSTProcessor::CloudsVSTProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_(*this, nullptr, "CloudsVSTState", CloudsVST::createParameterLayout())
{
    // Cache raw parameter pointers for real-time access (no locks)
    positionParam_  = apvts_.getRawParameterValue("position");
    sizeParam_      = apvts_.getRawParameterValue("size");
    pitchParam_     = apvts_.getRawParameterValue("pitch");
    densityParam_   = apvts_.getRawParameterValue("density");
    textureParam_   = apvts_.getRawParameterValue("texture");
    dryWetParam_    = apvts_.getRawParameterValue("dry_wet");
    spreadParam_    = apvts_.getRawParameterValue("stereo_spread");
    feedbackParam_  = apvts_.getRawParameterValue("feedback");
    reverbParam_    = apvts_.getRawParameterValue("reverb");
    freezeParam_    = apvts_.getRawParameterValue("freeze");
    triggerParam_   = apvts_.getRawParameterValue("trigger");
    modeParam_      = apvts_.getRawParameterValue("playback_mode");
    qualityParam_   = apvts_.getRawParameterValue("quality");
    inputGainParam_ = apvts_.getRawParameterValue("input_gain");
    inputTrimParam_  = apvts_.getRawParameterValue("engine_input_trim");
    outputGainParam_ = apvts_.getRawParameterValue("engine_output_gain");
}

CloudsVSTProcessor::~CloudsVSTProcessor()
{
}

//==============================================================================
void CloudsVSTProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine_.init();
    srcAdapter_.prepare(sampleRate, samplesPerBlock);
    inputCopyBuffer_.setSize(2, samplesPerBlock);
}

void CloudsVSTProcessor::releaseResources()
{
}

bool CloudsVSTProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Only support stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

void CloudsVSTProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                      juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Ensure at least stereo
    if (numChannels < 2)
        return;

    // [A] Input probe & meter
    float peakA = 0.0f;
    {
        const float* inL = buffer.getReadPointer(0);
        const float* inR = buffer.getReadPointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            float absL = std::abs(inL[i]);
            float absR = std::abs(inR[i]);
            float v = (absL + absR) * 0.5f;
            if (v > peakA) peakA = v;
        }
    }
    meterA_.store(peakA, std::memory_order_relaxed);
    probeA_.measureStereo(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);

    // --- Read parameters and push to engine ---
    engine_.setPosition(positionParam_->load());
    engine_.setSize(sizeParam_->load());
    engine_.setPitch(pitchParam_->load());
    engine_.setDensity(densityParam_->load());
    engine_.setTexture(textureParam_->load());
    engine_.setDryWet(dryWetParam_->load());
    engine_.setStereoSpread(spreadParam_->load());
    engine_.setFeedback(feedbackParam_->load());
    engine_.setReverb(reverbParam_->load());
    engine_.setFreeze(freezeParam_->load() >= 0.5f);
    engine_.setPlaybackMode(static_cast<int>(modeParam_->load()));
    engine_.setQuality(static_cast<int>(qualityParam_->load()));
    engine_.setInputTrim(inputTrimParam_->load());
    engine_.setOutputGain(outputGainParam_->load());

    // Handle trigger (momentary: set once, CloudsEngine resets after process)
    if (triggerParam_->load() >= 0.5f)
        engine_.setTrigger(true);

    // --- Apply input gain ---
    const float gainDb = inputGainParam_->load();
    const float gainLinear = std::pow(10.0f, gainDb / 20.0f);

    if (std::abs(gainLinear - 1.0f) > 0.0001f)
    {
        buffer.applyGain(0, numSamples, gainLinear);
    }

    // [B] Post-gain probe & meter
    float peakB = 0.0f;
    {
        const float* inL = buffer.getReadPointer(0);
        const float* inR = buffer.getReadPointer(1);
        for (int i = 0; i < numSamples; ++i)
        {
            float absL = std::abs(inL[i]);
            float absR = std::abs(inR[i]);
            float v = (absL + absR) * 0.5f;
            if (v > peakB) peakB = v;
        }
    }
    meterB_.store(peakB, std::memory_order_relaxed);
    probeB_.measureStereo(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);

    // --- Avoid aliasing: copy input to separate buffer ---
    if (inputCopyBuffer_.getNumSamples() < numSamples)
        inputCopyBuffer_.setSize(2, numSamples, false, false, true);
    inputCopyBuffer_.copyFrom(0, 0, buffer, 0, 0, numSamples);
    inputCopyBuffer_.copyFrom(1, 0, buffer, 1, 0, numSamples);

    auto* inL  = inputCopyBuffer_.getReadPointer(0);
    auto* inR  = inputCopyBuffer_.getReadPointer(1);
    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);

    srcAdapter_.process(inL, inR, outL, outR, numSamples, engine_);

    // [F] Output probe & meter
    float peakF = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        float absL = std::abs(outL[i]);
        float absR = std::abs(outR[i]);
        float v = (absL + absR) * 0.5f;
        if (v > peakF) peakF = v;
    }
    meterF_.store(peakF, std::memory_order_relaxed);
    probeF_.measureStereo(outL, outR, numSamples);
}

//==============================================================================
juce::AudioProcessorEditor* CloudsVSTProcessor::createEditor()
{
    return new CloudsVSTEditor(*this);
}

void CloudsVSTProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CloudsVSTProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts_.state.getType()))
            apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

//==============================================================================
// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CloudsVSTProcessor();
}

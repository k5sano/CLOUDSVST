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
    limiterParam_    = apvts_.getRawParameterValue("output_limiter");
}

CloudsVSTProcessor::~CloudsVSTProcessor()
{
}

//==============================================================================
void CloudsVSTProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    engine_.init();
    engine_.setMeterPointers(&meterD_, &meterE_);
    srcAdapter_.prepare(sampleRate, samplesPerBlock);

    // Allocate buffer with extra headroom to prevent reallocation in processBlock
    // Most DAWs use blocks up to 2048 samples, so use 4096 as safety margin
    const int maxBlockSize = samplesPerBlock * 2;
    inputCopyBuffer_.setSize(2, maxBlockSize, false, true, false);
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

    // --- Apply input gain and measure input (combined single pass) ---
    const float gainDb = inputGainParam_->load();
    const float gainLinear = std::pow(10.0f, gainDb / 20.0f);

    float* inL = buffer.getWritePointer(0);
    float* inR = buffer.getWritePointer(1);

    float peakB = 0.0f;
    if (std::abs(gainLinear - 1.0f) > 0.0001f)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            inL[i] *= gainLinear;
            inR[i] *= gainLinear;
            float v = (std::abs(inL[i]) + std::abs(inR[i])) * 0.5f;
            if (v > peakB) peakB = v;
        }
    }
    else
    {
        // No gain needed, just measure
        for (int i = 0; i < numSamples; ++i)
        {
            float v = (std::abs(inL[i]) + std::abs(inR[i])) * 0.5f;
            if (v > peakB) peakB = v;
        }
    }
    meterB_.store(peakB, std::memory_order_relaxed);

#if JUCE_DEBUG
    // Debug probes only in debug builds
    probeA_.measureStereo(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);
    probeB_.measureStereo(buffer.getReadPointer(0), buffer.getReadPointer(1), numSamples);
#else
    juce::ignoreUnused(probeA_);
    juce::ignoreUnused(probeB_);
#endif

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

    // --- Avoid aliasing: copy input to separate buffer (no reallocation!) ---
    // Note: Buffer size is fixed in prepareToPlay to prevent heap allocation
    jassert(inputCopyBuffer_.getNumSamples() >= numSamples);
    inputCopyBuffer_.copyFrom(0, 0, buffer, 0, 0, numSamples);
    inputCopyBuffer_.copyFrom(1, 0, buffer, 1, 0, numSamples);

    auto* procInL  = inputCopyBuffer_.getReadPointer(0);
    auto* procInR  = inputCopyBuffer_.getReadPointer(1);
    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);

    srcAdapter_.process(procInL, procInR, outL, outR, numSamples, engine_);

    // --- Output safety clamp, metering, and Lissajous (combined single pass) ---
    const float limit = limiterParam_->load();
    float peakF = 0.0f;
    int writePos = lissajousWritePos_.load(std::memory_order_relaxed);

    for (int i = 0; i < numSamples; ++i)
    {
        // Clamp
        outL[i] = juce::jlimit(-limit, limit, outL[i]);
        outR[i] = juce::jlimit(-limit, limit, outR[i]);

        // Meter
        float v = (std::abs(outL[i]) + std::abs(outR[i])) * 0.5f;
        if (v > peakF) peakF = v;

        // Lissajous (every 8th sample)
        if ((i & 7) == 0)
        {
            int idx = writePos % kLissajousSize;
            lissajousL_[idx].store(outL[i], std::memory_order_relaxed);
            lissajousR_[idx].store(outR[i], std::memory_order_relaxed);
            ++writePos;
        }
    }
    meterF_.store(peakF, std::memory_order_relaxed);
    lissajousWritePos_.store(writePos, std::memory_order_relaxed);

#if JUCE_DEBUG
    probeF_.measureStereo(outL, outR, numSamples);
#else
    juce::ignoreUnused(probeF_);
#endif
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

    // Save background image path
    if (backgroundImagePath_.exists())
        xml->setAttribute("backgroundImagePath", backgroundImagePath_.getFullPathName());

    copyXmlToBinary(*xml, destData);
}

void CloudsVSTProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr)
    {
        if (xmlState->hasTagName(apvts_.state.getType()))
            apvts_.replaceState(juce::ValueTree::fromXml(*xmlState));

        // Restore background image path
        if (xmlState->hasAttribute("backgroundImagePath"))
        {
            juce::String path = xmlState->getStringAttribute("backgroundImagePath");
            backgroundImagePath_ = juce::File(path);
        }
    }
}

//==============================================================================
// This creates the plugin instance
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CloudsVSTProcessor();
}

#include "PluginProcessor.h" #include "PluginEditor.h" #include "Parameters.h" #include

CloudsVSTProcessor::CloudsVSTProcessor()

AudioProcessor(BusesProperties() .withInput("Input", juce::AudioChannelSet::stereo(), true) .withOutput("Output", juce::AudioChannelSet::stereo(), true)), apvts\_(\*this, nullptr, "CloudsVSTState", CloudsVST::createParameterLayout()) { // Cache raw parameter pointers for real-time access (no locks) positionParam\_ = apvts\_.getRawParameterValue("position"); sizeParam\_ = apvts\_.getRawParameterValue("size"); pitchParam\_ = apvts\_.getRawParameterValue("pitch"); densityParam\_ = apvts\_.getRawParameterValue("density"); textureParam\_ = apvts\_.getRawParameterValue("texture"); dryWetParam\_ = apvts\_.getRawParameterValue("dry\_wet"); spreadParam\_ = apvts\_.getRawParameterValue("stereo\_spread"); feedbackParam\_ = apvts\_.getRawParameterValue("feedback"); reverbParam\_ = apvts\_.getRawParameterValue("reverb"); freezeParam\_ = apvts\_.getRawParameterValue("freeze"); triggerParam\_ = apvts\_.getRawParameterValue("trigger"); modeParam\_ = apvts\_.getRawParameterValue("playback\_mode"); qualityParam\_ = apvts\_.getRawParameterValue("quality"); inputGainParam\_ = apvts\_.getRawParameterValue("input\_gain"); }

CloudsVSTProcessor::~CloudsVSTProcessor() { }

void CloudsVSTProcessor::prepareToPlay(double sampleRate, int samplesPerBlock) { engine\_.init(); srcAdapter\_.prepare(sampleRate, samplesPerBlock); }

void CloudsVSTProcessor::releaseResources() { }

bool CloudsVSTProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const { // Only support stereo if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false; if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo()) return false; return true; }

void CloudsVSTProcessor::processBlock(juce::AudioBuffer& buffer, juce::MidiBuffer& /_midiMessages_/) { juce::ScopedNoDenormals noDenormals;

```
const int numSamples = buffer.getNumSamples();
const int numChannels = buffer.getNumChannels();

// Ensure at least stereo
if (numChannels < 2)
    return;

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

// --- Process through sample rate adapter + engine ---
auto* inL  = buffer.getReadPointer(0);
auto* inR  = buffer.getReadPointer(1);
auto* outL = buffer.getWritePointer(0);
auto* outR = buffer.getWritePointer(1);

srcAdapter_.process(inL, inR, outL, outR, numSamples, engine_);
```

}

juce::AudioProcessorEditor\* CloudsVSTProcessor::createEditor() { return new CloudsVSTEditor(\*this); }

void CloudsVSTProcessor::getStateInformation(juce::MemoryBlock& destData) { auto state = apvts\_.copyState(); std::unique\_ptrjuce::XmlElement xml(state.createXml()); copyXmlToBinary(\*xml, destData); }

void CloudsVSTProcessor::setStateInformation(const void\* data, int sizeInBytes) { std::unique\_ptrjuce::XmlElement xmlState(getXmlFromBinary(data, sizeInBytes)); if (xmlState != nullptr) { if (xmlState->hasTagName(apvts\_.state.getType())) apvts\_.replaceState(juce::ValueTree::fromXml(\*xmlState)); } }

// This creates the plugin instance juce::AudioProcessor\* JUCE\_CALLTYPE createPluginFilter() { return new CloudsVSTProcessor(); }
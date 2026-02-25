```cpp
Copy#include "PluginProcessor.h"
#include "PluginEditor.h"

MT2Plugin::MT2Plugin()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", MT2Params::createLayout())
{
    distParam        = apvts.getRawParameterValue("dist");
    levelParam       = apvts.getRawParameterValue("level");
    diodeMorphParam  = apvts.getRawParameterValue("diode_morph");
    diodeLinkParam   = apvts.getRawParameterValue("diode_link");
    diodeMorph2Param = apvts.getRawParameterValue("diode_morph_2");
    eqLowParam       = apvts.getRawParameterValue("eq_low");
    eqMidParam       = apvts.getRawParameterValue("eq_mid");
    eqMidFreqParam   = apvts.getRawParameterValue("eq_mid_freq");
    eqMidQParam      = apvts.getRawParameterValue("eq_mid_q");
    eqHighParam      = apvts.getRawParameterValue("eq_high");
}

void MT2Plugin::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // 4x oversampling (order=2 → 2^2=4x)
    mOversampling.initProcessing(static_cast<size_t>(samplesPerBlock));
    setLatencySamples(static_cast<int>(mOversampling.getLatencyInSamples()));

    double oversampledRate = sampleRate * 4.0;
    mGainStage.prepare(oversampledRate);
    mToneStack.prepare(sampleRate); // EQ runs at base rate

    mDoubleBuffer.setSize(2, samplesPerBlock);
}

void MT2Plugin::releaseResources()
{
    mOversampling.reset();
    mGainStage.reset();
    mToneStack.reset();
}

void MT2Plugin::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numChannels = buffer.getNumChannels();
    const int numSamples  = buffer.getNumSamples();

    // --- Read parameters ---
    const float dist       = distParam->load();
    const float level      = levelParam->load();
    const float morphVal   = diodeMorphParam->load();
    const bool  linked     = diodeLinkParam->load() >= 0.5f;
    const float morphVal2  = diodeMorph2Param->load();
    const float eqLow      = eqLowParam->load();
    const float eqMid      = eqMidParam->load();
    const float eqMidFreq  = eqMidFreqParam->load();
    const float eqMidQ     = eqMidQParam->load();
    const float eqHigh     = eqHighParam->load();

    // --- Map dist to gain: logarithmic 5.6 .. 200 ---
    const double gain = 5.6 * std::pow(200.0 / 5.6, static_cast<double>(dist));

    // --- Diode morph ---
    auto diode1 = mDiodeMorpher.getMorphedParams(morphVal);
    auto diode2 = linked ? diode1 : mDiodeMorpher.getMorphedParams(morphVal2);

    mGainStage.setGain(gain);
    mGainStage.setStage1Diode(diode1.is, diode1.n, diode1.noClip);
    mGainStage.setStage2Diode(diode2.is, diode2.n, diode2.noClip);

    // --- EQ coefficients update (once per block) ---
    mToneStack.updateCoefficients(eqLow, eqMid, eqMidFreq, eqMidQ, eqHigh);

    // --- Output level ---
    const double outputLevel = static_cast<double>(level);

    // --- Convert float → double ---
    mDoubleBuffer.setSize(numChannels, numSamples, false, false, true);
    for (int ch = 0; ch < numChannels; ++ch)
        for (int i = 0; i < numSamples; ++i)
            mDoubleBuffer.setSample(ch, i,
                static_cast<double>(buffer.getSample(ch, i)));

    // --- Oversampled processing ---
    juce::dsp::AudioBlock<double> block(mDoubleBuffer);
    auto oversampledBlock = mOversampling.processSamplesUp(block);

    const int osNumSamples = static_cast<int>(oversampledBlock.getNumSamples());
    for (int ch = 0; ch < static_cast<int>(oversampledBlock.getNumChannels()); ++ch) {
        auto* data = oversampledBlock.getChannelPointer(static_cast<size_t>(ch));
        for (int i = 0; i < osNumSamples; ++i) {
            data[i] = mGainStage.processSample(data[i]);
        }
    }

    mOversampling.processSamplesDown(block);

    // --- EQ + output level (at base sample rate) ---
    for (int ch = 0; ch < numChannels; ++ch) {
        for (int i = 0; i < numSamples; ++i) {
            double sample = mDoubleBuffer.getSample(ch, i);
            sample = mToneStack.processSample(sample);
            sample *= outputLevel;
            buffer.setSample(ch, i, static_cast<float>(sample));
        }
    }
}

juce::AudioProcessorEditor* MT2Plugin::createEditor()
{
    return new MT2PluginEditor(*this);
}

void MT2Plugin::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MT2Plugin::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MT2Plugin();
}
Copy
```
```cpp
Copy#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace MT2Params {

    inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout() {
        std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

        // --- Core ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"dist", 1}, "Dist",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"level", 1}, "Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        // --- Diode Morphing (Feature A) ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"diode_morph", 1}, "Diode Morph",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        params.push_back(std::make_unique<juce::AudioParameterBool>(
            juce::ParameterID{"diode_link", 1}, "Diode Link", true));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"diode_morph_2", 1}, "Diode Morph 2",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));

        // --- EQ (Feature B) ---
        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"eq_low", 1}, "Low",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"eq_mid", 1}, "Mid Level",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"eq_mid_freq", 1}, "Mid Freq",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"eq_mid_q", 1}, "Mid Q",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.3f));

        params.push_back(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID{"eq_high", 1}, "High",
            juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.5f));

        return { params.begin(), params.end() };
    }

} // namespace MT2Params
Copy
```
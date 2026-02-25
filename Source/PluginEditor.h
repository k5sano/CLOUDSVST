```cpp
Copy#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class MT2PluginEditor : public juce::AudioProcessorEditor {
public:
    explicit MT2PluginEditor(MT2Plugin&);
    ~MT2PluginEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MT2Plugin& processor;
    juce::GenericAudioProcessorEditor genericEditor;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MT2PluginEditor)
};
```
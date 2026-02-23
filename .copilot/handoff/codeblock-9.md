Copy
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class CloudsAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    CloudsAudioProcessorEditor(CloudsAudioProcessor&);
    ~CloudsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CloudsAudioProcessor& processorRef;

    // ノブ群
    juce::Slider positionSlider, sizeSlider, pitchSlider,
                 densitySlider, textureSlider,
                 dryWetSlider, stereoSlider,
                 feedbackSlider, reverbSlider;

    juce::ToggleButton freezeButton;
    juce::ComboBox modeBox, qualityBox;

    // ラベル
    juce::Label positionLabel, sizeLabel, pitchLabel,
                densityLabel, textureLabel,
                dryWetLabel, stereoLabel,
                feedbackLabel, reverbLabel,
                modeLabel, qualityLabel;

    // APVTS アタッチメント
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> positionAtt, sizeAtt, pitchAtt,
                                       densityAtt, textureAtt,
                                       dryWetAtt, stereoAtt,
                                       feedbackAtt, reverbAtt;
    std::unique_ptr<ButtonAttachment> freezeAtt;
    std::unique_ptr<ComboBoxAttachment> modeAtt, qualityAtt;

    void setupSlider(juce::Slider& slider, juce::Label& label,
                     const juce::String& name,
                     std::unique_ptr<SliderAttachment>& att,
                     const juce::String& paramId);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsAudioProcessorEditor)
};

Copy
#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

class CloudsVSTEditor : public juce::AudioProcessorEditor
{
public:
    explicit CloudsVSTEditor(CloudsVSTProcessor&);
    ~CloudsVSTEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    CloudsVSTProcessor& processorRef_;

    // --- Main knobs ---
    juce::Slider positionKnob_, sizeKnob_, pitchKnob_, densityKnob_, textureKnob_;

    // --- Blend knobs ---
    juce::Slider dryWetKnob_, spreadKnob_, feedbackKnob_, reverbKnob_;

    // --- Input gain ---
    juce::Slider inputGainSlider_;

    // --- Buttons ---
    juce::TextButton freezeButton_{"Freeze"};
    juce::TextButton triggerButton_{"Trigger"};

    // --- Mode / Quality selectors ---
    juce::ComboBox modeSelector_;
    juce::ComboBox qualitySelector_;

    // --- Labels ---
    juce::Label positionLabel_, sizeLabel_, pitchLabel_, densityLabel_, textureLabel_;
    juce::Label dryWetLabel_, spreadLabel_, feedbackLabel_, reverbLabel_;
    juce::Label inputGainLabel_, modeLabel_, qualityLabel_;

    // --- APVTS Attachments ---
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> positionAtt_, sizeAtt_, pitchAtt_,
        densityAtt_, textureAtt_;
    std::unique_ptr<SliderAttachment> dryWetAtt_, spreadAtt_, feedbackAtt_,
        reverbAtt_;
    std::unique_ptr<SliderAttachment> inputGainAtt_;
    std::unique_ptr<ButtonAttachment> freezeAtt_, triggerAtt_;
    std::unique_ptr<ComboBoxAttachment> modeAtt_, qualityAtt_;

    void setupKnob(juce::Slider& slider, juce::Label& label,
                   const juce::String& text);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsVSTEditor)
};

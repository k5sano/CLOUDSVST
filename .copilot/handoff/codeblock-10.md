Copy
#include "PluginEditor.h"

//==============================================================================
CloudsAudioProcessorEditor::CloudsAudioProcessorEditor(CloudsAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setSize(720, 480);

    auto& apvts = processorRef.getAPVTS();

    // ---- スライダー (ロータリーノブ) セットアップ ----
    setupSlider(positionSlider, positionLabel, "Position", positionAtt, "position");
    setupSlider(sizeSlider, sizeLabel, "Size", sizeAtt, "size");
    setupSlider(pitchSlider, pitchLabel, "Pitch", pitchAtt, "pitch");
    setupSlider(densitySlider, densityLabel, "Density", densityAtt, "density");
    setupSlider(textureSlider, textureLabel, "Texture", textureAtt, "texture");
    setupSlider(dryWetSlider, dryWetLabel, "Dry/Wet", dryWetAtt, "drywet");
    setupSlider(stereoSlider, stereoLabel, "Stereo", stereoAtt, "stereo");
    setupSlider(feedbackSlider, feedbackLabel, "Feedback", feedbackAtt, "feedback");
    setupSlider(reverbSlider, reverbLabel, "Reverb", reverbAtt, "reverb");

    // ---- Freeze ボタン ----
    freezeButton.setButtonText("Freeze");
    addAndMakeVisible(freezeButton);
    freezeAtt = std::make_unique<ButtonAttachment>(apvts, "freeze", freezeButton);

    // ---- Mode コンボボックス ----
    modeLabel.setText("Mode", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modeLabel);
    modeBox.addItemList({"Granular", "Stretch", "Looping Delay", "Spectral"}, 1);
    addAndMakeVisible(modeBox);
    modeAtt = std::make_unique<ComboBoxAttachment>(apvts, "mode", modeBox);

    // ---- Quality コンボボックス ----
    qualityLabel.setText("Quality", juce::dontSendNotification);
    qualityLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(qualityLabel);
    qualityBox.addItemList({"16b Stereo", "16b Mono", "8b Stereo", "8b Mono"}, 1);
    addAndMakeVisible(qualityBox);
    qualityAtt = std::make_unique<ComboBoxAttachment>(apvts, "quality", qualityBox);
}

CloudsAudioProcessorEditor::~CloudsAudioProcessorEditor()
{
}

//==============================================================================
void CloudsAudioProcessorEditor::setupSlider(
    juce::Slider& slider, juce::Label& label,
    const juce::String& name,
    std::unique_ptr<SliderAttachment>& att,
    const juce::String& paramId)
{
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    addAndMakeVisible(slider);

    label.setText(name, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);

    att = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), paramId, slider);
}

//==============================================================================
void CloudsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    g.setColour(juce::Colour(0xffe0e0e0));
    g.setFont(22.0f);
    g.drawFittedText("Clouds", getLocalBounds().removeFromTop(40),
                     juce::Justification::centred, 1);
}

void CloudsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(40); // タイトル領域

    // 上段: メインノブ 5つ（Position, Size, Pitch, Density, Texture）
    const int knobW = 120;
    const int knobH = 130;
    const int labelH = 20;

    auto topRow = area.removeFromTop(knobH + labelH);
    auto knobs1 = topRow.withSizeKeepingCentre(knobW * 5, knobH + labelH);

    struct KnobSet { juce::Slider* s; juce::Label* l; };
    KnobSet topKnobs[] = {
        {&positionSlider, &positionLabel},
        {&sizeSlider, &sizeLabel},
        {&pitchSlider, &pitchLabel},
        {&densitySlider, &densityLabel},
        {&textureSlider, &textureLabel}
    };
    for (auto& k : topKnobs)
    {
        auto col = knobs1.removeFromLeft(knobW);
        k.l->setBounds(col.removeFromTop(labelH));
        k.s->setBounds(col);
    }

    area.removeFromTop(10);

    // 中段: サブノブ 4つ（Dry/Wet, Stereo, Feedback, Reverb）
    auto midRow = area.removeFromTop(knobH + labelH);
    auto knobs2 = midRow.withSizeKeepingCentre(knobW * 4, knobH + labelH);

    KnobSet midKnobs[] = {
        {&dryWetSlider, &dryWetLabel},
        {&stereoSlider, &stereoLabel},
        {&feedbackSlider, &feedbackLabel},
        {&reverbSlider, &reverbLabel}
    };
    for (auto& k : midKnobs)
    {
        auto col = knobs2.removeFromLeft(knobW);
        k.l->setBounds(col.removeFromTop(labelH));
        k.s->setBounds(col);
    }

    area.removeFromTop(10);

    // 下段: Mode, Quality, Freeze
    auto bottomRow = area.removeFromTop(60);
    auto bottomArea = bottomRow.withSizeKeepingCentre(500, 55);

    auto modeArea = bottomArea.removeFromLeft(160);
    modeLabel.setBounds(modeArea.removeFromTop(20));
    modeBox.setBounds(modeArea);

    bottomArea.removeFromLeft(20);

    auto qualArea = bottomArea.removeFromLeft(160);
    qualityLabel.setBounds(qualArea.removeFromTop(20));
    qualityBox.setBounds(qualArea);

    bottomArea.removeFromLeft(20);
    freezeButton.setBounds(bottomArea.removeFromLeft(120));
}

Copy
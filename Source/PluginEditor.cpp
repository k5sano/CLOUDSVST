#include "PluginEditor.h"

CloudsVSTEditor::CloudsVSTEditor(CloudsVSTProcessor& p)
    : AudioProcessorEditor(&p), processorRef_(p)
{
    setSize(620, 420);

    auto& apvts = processorRef_.getAPVTS();

    // --- Setup main knobs ---
    setupKnob(positionKnob_, positionLabel_, "Position");
    setupKnob(sizeKnob_,     sizeLabel_,     "Size");
    setupKnob(pitchKnob_,    pitchLabel_,    "Pitch");
    setupKnob(densityKnob_,  densityLabel_,  "Density");
    setupKnob(textureKnob_,  textureLabel_,  "Texture");

    // --- Setup blend knobs ---
    setupKnob(dryWetKnob_,    dryWetLabel_,    "Dry/Wet");
    setupKnob(spreadKnob_,    spreadLabel_,    "Spread");
    setupKnob(feedbackKnob_,  feedbackLabel_,  "Feedback");
    setupKnob(reverbKnob_,    reverbLabel_,    "Reverb");

    // --- Input gain slider ---
    inputGainSlider_.setSliderStyle(juce::Slider::LinearVertical);
    inputGainSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    addAndMakeVisible(inputGainSlider_);
    inputGainLabel_.setText("Gain", juce::dontSendNotification);
    inputGainLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputGainLabel_);

    // --- Freeze button ---
    freezeButton_.setClickingTogglesState(true);
    freezeButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colours::cyan);
    addAndMakeVisible(freezeButton_);

    // --- Trigger button ---
    triggerButton_.setClickingTogglesState(false);
    addAndMakeVisible(triggerButton_);

    // --- Mode selector ---
    modeSelector_.addItemList({"Granular", "Stretch", "Looping Delay", "Spectral"}, 1);
    addAndMakeVisible(modeSelector_);
    modeLabel_.setText("Mode", juce::dontSendNotification);
    modeLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(modeLabel_);

    // --- Quality selector ---
    qualitySelector_.addItemList({"16b Stereo", "16b Mono", "8b Stereo", "8b Mono"}, 1);
    addAndMakeVisible(qualitySelector_);
    qualityLabel_.setText("Quality", juce::dontSendNotification);
    qualityLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(qualityLabel_);

    // --- Attach to APVTS ---
    positionAtt_  = std::make_unique<SliderAttachment>(apvts, "position",       positionKnob_);
    sizeAtt_      = std::make_unique<SliderAttachment>(apvts, "size",           sizeKnob_);
    pitchAtt_     = std::make_unique<SliderAttachment>(apvts, "pitch",          pitchKnob_);
    densityAtt_   = std::make_unique<SliderAttachment>(apvts, "density",        densityKnob_);
    textureAtt_   = std::make_unique<SliderAttachment>(apvts, "texture",        textureKnob_);
    dryWetAtt_    = std::make_unique<SliderAttachment>(apvts, "dry_wet",        dryWetKnob_);
    spreadAtt_    = std::make_unique<SliderAttachment>(apvts, "stereo_spread",  spreadKnob_);
    feedbackAtt_  = std::make_unique<SliderAttachment>(apvts, "feedback",       feedbackKnob_);
    reverbAtt_    = std::make_unique<SliderAttachment>(apvts, "reverb",         reverbKnob_);
    inputGainAtt_ = std::make_unique<SliderAttachment>(apvts, "input_gain",     inputGainSlider_);
    freezeAtt_    = std::make_unique<ButtonAttachment>(apvts, "freeze",         freezeButton_);
    triggerAtt_   = std::make_unique<ButtonAttachment>(apvts, "trigger",        triggerButton_);
    modeAtt_      = std::make_unique<ComboBoxAttachment>(apvts, "playback_mode", modeSelector_);
    qualityAtt_   = std::make_unique<ComboBoxAttachment>(apvts, "quality",       qualitySelector_);
}

CloudsVSTEditor::~CloudsVSTEditor()
{
}

void CloudsVSTEditor::setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 16);
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.attachToComponent(&slider, false);
    addAndMakeVisible(label);
}

void CloudsVSTEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("CloudsVST", getLocalBounds().removeFromTop(30),
               juce::Justification::centred);

    // Credit
    g.setFont(10.0f);
    g.setColour(juce::Colours::grey);
    g.drawText("Based on Mutable Instruments Clouds by Emilie Gillet (MIT License)",
               getLocalBounds().removeFromBottom(18),
               juce::Justification::centred);
}

void CloudsVSTEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(30);  // Title
    area.removeFromBottom(20);  // Credit

    const int knobW = 80;
    const int knobH = 90;
    const int labelH = 18;

    // --- Row 1: Main knobs (Position, Size, Pitch, Density, Texture) ---
    auto row1 = area.removeFromTop(knobH + labelH);
    // Leave space for Input Gain slider on the right
    auto gainArea = row1.removeFromRight(50);

    int mainKnobSpacing = row1.getWidth() / 5;
    positionKnob_.setBounds(row1.removeFromLeft(mainKnobSpacing).reduced(8, labelH));
    sizeKnob_.setBounds(row1.removeFromLeft(mainKnobSpacing).reduced(8, labelH));
    pitchKnob_.setBounds(row1.removeFromLeft(mainKnobSpacing).reduced(8, labelH));
    densityKnob_.setBounds(row1.removeFromLeft(mainKnobSpacing).reduced(8, labelH));
    textureKnob_.setBounds(row1.reduced(8, labelH));

    area.removeFromTop(8);

    // --- Row 2: Blend knobs (DryWet, Spread, Feedback, Reverb) ---
    auto row2 = area.removeFromTop(knobH + labelH);
    row2.removeFromRight(50);  // Align with row 1

    int blendKnobSpacing = row2.getWidth() / 4;
    dryWetKnob_.setBounds(row2.removeFromLeft(blendKnobSpacing).reduced(8, labelH));
    spreadKnob_.setBounds(row2.removeFromLeft(blendKnobSpacing).reduced(8, labelH));
    feedbackKnob_.setBounds(row2.removeFromLeft(blendKnobSpacing).reduced(8, labelH));
    reverbKnob_.setBounds(row2.reduced(8, labelH));

    area.removeFromTop(8);

    // --- Row 3: Mode, Quality, Freeze, Trigger ---
    auto row3 = area.removeFromTop(40);
    row3.removeFromRight(50);

    modeLabel_.setBounds(row3.removeFromLeft(40));
    modeSelector_.setBounds(row3.removeFromLeft(130).reduced(2));
    row3.removeFromLeft(10);
    qualityLabel_.setBounds(row3.removeFromLeft(48));
    qualitySelector_.setBounds(row3.removeFromLeft(110).reduced(2));
    row3.removeFromLeft(10);
    freezeButton_.setBounds(row3.removeFromLeft(70).reduced(2));
    row3.removeFromLeft(6);
    triggerButton_.setBounds(row3.removeFromLeft(70).reduced(2));

    // --- Input Gain slider (right side, spanning rows 1-2) ---
    auto totalGainArea = getLocalBounds().reduced(10);
    totalGainArea.removeFromTop(30);
    totalGainArea.removeFromBottom(20);
    auto gainSliderArea = totalGainArea.removeFromRight(50);
    inputGainLabel_.setBounds(gainSliderArea.removeFromTop(labelH));
    inputGainSlider_.setBounds(gainSliderArea.removeFromTop(knobH * 2 + labelH));
}

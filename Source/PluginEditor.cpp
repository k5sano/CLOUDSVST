#include "PluginEditor.h"

CloudsVSTEditor::CloudsVSTEditor(CloudsVSTProcessor& p)
    : AudioProcessorEditor(&p), processorRef_(p)
{
    setSize(800, 520);  // Expanded width for meters, height for Row 4

    // Start timer for GUI updates (30fps)
    startTimerHz(30);

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

    // --- Engine gain staging knobs ---
    setupKnob(inputTrimKnob_,  inputTrimLabel_,  "Eng.Trim");
    setupKnob(outputGainKnob_, outputGainLabel_, "Eng.Gain");
    inputTrimAtt_  = std::make_unique<SliderAttachment>(apvts, "engine_input_trim",  inputTrimKnob_);
    outputGainAtt_ = std::make_unique<SliderAttachment>(apvts, "engine_output_gain", outputGainKnob_);
}

CloudsVSTEditor::~CloudsVSTEditor()
{
    stopTimer();
}

void CloudsVSTEditor::timerCallback()
{
    // Read meter values from processor (atomic, lock-free)
    meterValues_[0] = processorRef_.getMeterA().load(std::memory_order_relaxed);
    meterValues_[1] = processorRef_.getMeterB().load(std::memory_order_relaxed);
    meterValues_[2] = 0.0f;  // Meter C (SRC Down) - not implemented in simplified version
    meterValues_[3] = processorRef_.getMeterD().load(std::memory_order_relaxed);
    meterValues_[4] = processorRef_.getMeterE().load(std::memory_order_relaxed);
    meterValues_[5] = processorRef_.getMeterF().load(std::memory_order_relaxed);

    // Repaint to show updated meters
    repaint();
}

void CloudsVSTEditor::drawMeter(juce::Graphics& g, const juce::String& label,
                                 float value, int y, int width, int height)
{
    const int x = getLocalBounds().getWidth() - width - 10;

    // Draw background
    g.setColour(juce::Colour(30, 30, 40));
    g.fillRect(x, y, width, height);

    // Draw label
    g.setColour(juce::Colours::lightgrey);
    g.setFont(11.0f);
    g.drawText(label, x + 5, y + 2, 60, 14, juce::Justification::left);

    // Draw bar
    const float barWidth = std::min(value * 140.0f, 140.0f);  // Scale to 140px max
    const int barX = x + 65;
    const int barY = y + 3;
    const int barHeight = height - 6;

    if (value > 0.001f)
        g.setColour(juce::Colours::green);
    else
        g.setColour(juce::Colour(60, 60, 70));

    g.fillRect(barX, barY, static_cast<int>(barWidth), barHeight);

    // Draw border
    g.setColour(juce::Colour(80, 80, 90));
    g.drawRect(x, y, width, height, 1);
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

    // Draw signal meters on the right side
    const int meterWidth = 220;
    const int meterHeight = 20;
    const int meterYStart = 50;
    const int meterSpacing = 25;

    drawMeter(g, "A: Input",      meterValues_[0], meterYStart,               meterWidth, meterHeight);
    drawMeter(g, "B: PostGain",   meterValues_[1], meterYStart + meterSpacing, meterWidth, meterHeight);
    drawMeter(g, "C: SRC Down",   meterValues_[2], meterYStart + meterSpacing * 2, meterWidth, meterHeight);
    drawMeter(g, "D: Engine In",  meterValues_[3], meterYStart + meterSpacing * 3, meterWidth, meterHeight);
    drawMeter(g, "E: Engine Out", meterValues_[4], meterYStart + meterSpacing * 4, meterWidth, meterHeight);
    drawMeter(g, "F: Output",     meterValues_[5], meterYStart + meterSpacing * 5, meterWidth, meterHeight);
}

void CloudsVSTEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    area.removeFromTop(30);  // Title
    area.removeFromBottom(20);  // Credit

    // Reserve right side for meters
    area.removeFromRight(230);

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

    area.removeFromTop(8);

    // --- Row 4: Engine gain staging ---
    auto row4 = area.removeFromTop(knobH + labelH);
    row4.removeFromRight(50);
    int trimSpacing = row4.getWidth() / 4;
    inputTrimKnob_.setBounds(row4.removeFromLeft(trimSpacing).reduced(8, labelH));
    outputGainKnob_.setBounds(row4.removeFromLeft(trimSpacing).reduced(8, labelH));

    // --- Input Gain slider (right side of controls, spanning rows 1-2) ---
    auto totalGainArea = getLocalBounds().reduced(10);
    totalGainArea.removeFromTop(30);
    totalGainArea.removeFromBottom(20);
    totalGainArea.removeFromRight(230);  // Meters area
    auto gainSliderArea = totalGainArea.removeFromRight(50);
    inputGainLabel_.setBounds(gainSliderArea.removeFromTop(labelH));
    inputGainSlider_.setBounds(gainSliderArea.removeFromTop(knobH * 2 + labelH));
}

#include "PluginEditor.h"

CloudsVSTEditor::CloudsVSTEditor(CloudsVSTProcessor& p)
    : AudioProcessorEditor(&p), processorRef_(p)
{
    // Load background image from processor state
    juce::File bgPath = processorRef_.getBackgroundImagePath();
    if (bgPath.exists())
    {
        backgroundImage_ = juce::ImageFileFormat::loadFrom(bgPath);
    }

    setSize(950, 520);  // Expanded width for meters and new buttons

    // Start timer for GUI updates (30fps)
    startTimerHz(30);

    auto& apvts = processorRef_.getAPVTS();

    // --- Setup main knobs (Blue) ---
    setupKnob(positionKnob_, positionLabel_, "Position", juce::Colours::cyan);
    setupKnob(sizeKnob_,     sizeLabel_,     "Size", juce::Colours::cyan);
    setupKnob(pitchKnob_,    pitchLabel_,    "Pitch", juce::Colours::cyan);
    setupKnob(densityKnob_,  densityLabel_,  "Density", juce::Colours::cyan);
    setupKnob(textureKnob_,  textureLabel_,  "Texture", juce::Colours::cyan);

    // --- Setup blend knobs (Green) ---
    setupKnob(dryWetKnob_,    dryWetLabel_,    "Dry/Wet", juce::Colours::limegreen);
    setupKnob(spreadKnob_,    spreadLabel_,    "Spread", juce::Colours::limegreen);
    setupKnob(feedbackKnob_,  feedbackLabel_,  "Feedback", juce::Colours::limegreen);
    setupKnob(reverbKnob_,    reverbLabel_,    "Reverb", juce::Colours::limegreen);

    // --- Input gain slider ---
    inputGainSlider_.setSliderStyle(juce::Slider::LinearVertical);
    inputGainSlider_.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 50, 18);
    addAndMakeVisible(inputGainSlider_);
    inputGainLabel_.setText("Gain", juce::dontSendNotification);
    inputGainLabel_.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inputGainLabel_);

    // --- Freeze button (Orange, will be repositioned in resized()) ---
    freezeButton_.setClickingTogglesState(true);
    freezeButton_.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    freezeButton_.setColour(juce::TextButton::buttonColourId, juce::Colour(60, 40, 30));
    addAndMakeVisible(freezeButton_);

    // --- Trigger button ---
    triggerButton_.setClickingTogglesState(false);
    addAndMakeVisible(triggerButton_);

    // --- BG Image button ---
    loadImageButton_.onClick = [this] { loadBackgroundImage(); };
    addAndMakeVisible(loadImageButton_);

    // --- Preset buttons ---
    savePresetButton_.onClick = [this] { savePreset(); };
    loadPresetButton_.onClick = [this] { loadPreset(); };
    addAndMakeVisible(savePresetButton_);
    addAndMakeVisible(loadPresetButton_);

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

    // --- Engine gain staging knobs (Red/Pink) ---
    setupKnob(inputTrimKnob_,  inputTrimLabel_,  "Eng.Trim", juce::Colours::pink);
    setupKnob(outputGainKnob_, outputGainLabel_, "Eng.Gain", juce::Colours::pink);
    setupKnob(limiterKnob_,    limiterLabel_,    "Limiter", juce::Colours::pink);
    inputTrimAtt_  = std::make_unique<SliderAttachment>(apvts, "engine_input_trim",  inputTrimKnob_);
    outputGainAtt_ = std::make_unique<SliderAttachment>(apvts, "engine_output_gain", outputGainKnob_);
    limiterAtt_    = std::make_unique<SliderAttachment>(apvts, "output_limiter",    limiterKnob_);
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

void CloudsVSTEditor::setupKnob(juce::Slider& slider, juce::Label& label, const juce::String& text, const juce::Colour& colour)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 56, 18);
    slider.setColour(juce::Slider::rotarySliderFillColourId, colour);
    slider.setColour(juce::Slider::thumbColourId, colour.brighter(0.3f));
    addAndMakeVisible(slider);

    label.setText(text, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setFont(13.0f);  // Larger font for labels
    label.setColour(juce::Label::textColourId, colour);
    label.attachToComponent(&slider, false);
    addAndMakeVisible(label);
}

void CloudsVSTEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    // Draw background image with 40% opacity
    if (backgroundImage_.isValid())
    {
        g.setOpacity(0.4f);
        g.drawImageWithin(backgroundImage_, 0, 0, getWidth(), getHeight(),
                         juce::RectanglePlacement::stretchToFit);
        g.setOpacity(1.0f);
    }

    g.setColour(juce::Colours::white);
    g.setFont(18.0f);
    g.drawText("CloudsCOSMOS b012", getLocalBounds().removeFromTop(30),
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

    // --- Row 3: Mode, Quality, Trigger ------------------------------------------------
    auto row3 = area.removeFromTop(40);
    row3.removeFromRight(50);

    modeLabel_.setBounds(row3.removeFromLeft(40));
    modeSelector_.setBounds(row3.removeFromLeft(130).reduced(2));
    row3.removeFromLeft(10);
    qualityLabel_.setBounds(row3.removeFromLeft(48));
    qualitySelector_.setBounds(row3.removeFromLeft(110).reduced(2));
    row3.removeFromLeft(10);
    triggerButton_.setBounds(row3.removeFromLeft(60).reduced(2));

    // Right side buttons (BG Image, Save, Load) - take from remaining space
    int buttonWidth = 70;
    int buttonSpacing = 4;
    auto buttonsArea = row3.removeFromRight(buttonWidth * 3 + buttonSpacing * 2 + 10);
    loadImageButton_.setBounds(buttonsArea.removeFromLeft(buttonWidth).reduced(2));
    buttonsArea.removeFromLeft(buttonSpacing);
    savePresetButton_.setBounds(buttonsArea.removeFromLeft(buttonWidth).reduced(2));
    buttonsArea.removeFromLeft(buttonSpacing);
    loadPresetButton_.setBounds(buttonsArea.removeFromLeft(buttonWidth).reduced(2));

    area.removeFromTop(8);

    // --- Row 4: Engine gain staging + Freeze button ------------------------------------
    auto row4 = area.removeFromTop(knobH + labelH);
    row4.removeFromRight(50);
    int trimSpacing = row4.getWidth() / 5;  // 5 sections now
    inputTrimKnob_.setBounds(row4.removeFromLeft(trimSpacing).reduced(8, labelH));
    outputGainKnob_.setBounds(row4.removeFromLeft(trimSpacing).reduced(8, labelH));

    // Limiter knob + Freeze button combo
    auto limiterFreezeArea = row4.removeFromLeft(trimSpacing * 2);
    limiterKnob_.setBounds(limiterFreezeArea.removeFromLeft(trimSpacing).reduced(8, labelH));

    // Freeze button: square, orange, next to limiter, aligned with value box
    int freezeSize = 50;
    int freezeY = limiterKnob_.getBounds().getBottom() - 10;
    int freezeX = limiterFreezeArea.getX() + 20;
    freezeButton_.setBounds(freezeX, freezeY, freezeSize, freezeSize);

    // --- Input Gain slider (right side of controls, spanning rows 1-2) ---
    auto totalGainArea = getLocalBounds().reduced(10);
    totalGainArea.removeFromTop(30);
    totalGainArea.removeFromBottom(20);
    totalGainArea.removeFromRight(230);  // Meters area
    auto gainSliderArea = totalGainArea.removeFromRight(50);
    inputGainLabel_.setBounds(gainSliderArea.removeFromTop(labelH));
    inputGainSlider_.setBounds(gainSliderArea.removeFromTop(knobH * 2 + labelH));
}

//==============================================================================
void CloudsVSTEditor::loadBackgroundImage()
{
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Select Background Image",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.jpg;*.jpeg;*.png;*.gif;*.bmp");

    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                backgroundImage_ = juce::ImageFileFormat::loadFrom(file);
                processorRef_.setBackgroundImagePath(file);
                repaint();
            }
        });
}

void CloudsVSTEditor::savePreset()
{
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Save Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("CloudsCOSMOS.preset"),
        "*.preset");

    fileChooser_->launchAsync(juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (!file.getFullPathName().isEmpty())
            {
                // Ensure .preset extension
                juce::File targetFile = file.hasFileExtension("preset") ? file : file.withFileExtension("preset");

                // Get current state
                juce::MemoryBlock data;
                processorRef_.getStateInformation(data);

                // Write to file
                targetFile.replaceWithData(data.getData(), data.getSize());
            }
        });
}

void CloudsVSTEditor::loadPreset()
{
    fileChooser_ = std::make_unique<juce::FileChooser>(
        "Load Preset",
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("CloudsCOSMOS.preset"),
        "*.preset");

    fileChooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this](const juce::FileChooser& chooser)
        {
            auto file = chooser.getResult();
            if (file.existsAsFile())
            {
                juce::MemoryBlock data;
                if (file.loadFileAsData(data))
                {
                    processorRef_.setStateInformation(data.getData(), static_cast<int>(data.getSize()));

                    // Reload background image if saved in state
                    juce::File bgPath = processorRef_.getBackgroundImagePath();
                    if (bgPath.exists())
                    {
                        backgroundImage_ = juce::ImageFileFormat::loadFrom(bgPath);
                        repaint();
                    }
                }
            }
        });
}

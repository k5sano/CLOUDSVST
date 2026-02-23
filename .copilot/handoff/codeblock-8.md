Copy
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstring>

//==============================================================================
CloudsAudioProcessor::CloudsAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

CloudsAudioProcessor::~CloudsAudioProcessor()
{
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
CloudsAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Clouds の主要パラメータをマッピング
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"position", 1}, "Position", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"size", 1}, "Size", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"pitch", 1}, "Pitch",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"density", 1}, "Density", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"texture", 1}, "Texture", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drywet", 1}, "Dry/Wet", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"stereo", 1}, "Stereo Spread", 0.0f, 1.0f, 0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"feedback", 1}, "Feedback", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb", 1}, "Reverb", 0.0f, 1.0f, 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"freeze", 1}, "Freeze", false));

    // Playback Mode: 0=Granular, 1=Stretch, 2=Looping Delay, 3=Spectral
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"mode", 1}, "Mode",
        juce::StringArray{"Granular", "Stretch", "Looping Delay", "Spectral"}, 0));

    // Quality: 0=16bit Stereo, 1=16bit Mono, 2=8bit Stereo(μ-law), 3=8bit Mono
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"quality", 1}, "Quality",
        juce::StringArray{"16bit Stereo", "16bit Mono", "8bit Stereo", "8bit Mono"}, 0));

    return { params.begin(), params.end() };
}

//==============================================================================
void CloudsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    hostSampleRate_ = sampleRate;
    resamplePhase_ = 0.0;

    // Clouds DSP の初期化
    std::memset(largeBuffer_, 0, kLargeBufferSize);
    std::memset(smallBuffer_, 0, kSmallBufferSize);
    processor_.Init(largeBuffer_, kLargeBufferSize,
                    smallBuffer_, kSmallBufferSize);
    processor_.set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
    processor_.set_quality(0);
    processor_.set_bypass(false);
    processor_.set_silence(false);

    // リサンプリングバッファを余裕をもって確保
    size_t maxCloudsFrames = static_cast<size_t>(
        std::ceil(static_cast<double>(samplesPerBlock) *
                  kCloudsSampleRate / sampleRate)) + 64;
    inputResampleBufL_.resize(maxCloudsFrames, 0.0f);
    inputResampleBufR_.resize(maxCloudsFrames, 0.0f);
    outputResampleBufL_.resize(maxCloudsFrames, 0.0f);
    outputResampleBufR_.resize(maxCloudsFrames, 0.0f);
}

void CloudsAudioProcessor::releaseResources()
{
}

bool CloudsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

//==============================================================================
void CloudsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                         juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const float* inL = buffer.getReadPointer(0);
    const float* inR = buffer.getNumChannels() > 1 ? buffer.getReadPointer(1) : inL;
    float* outL = buffer.getWritePointer(0);
    float* outR = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : outL;

    // ---- パラメータ読み取り ----
    clouds::Parameters* p = processor_.mutable_parameters();

    p->position      = *apvts.getRawParameterValue("position");
    p->size           = *apvts.getRawParameterValue("size");
    p->pitch          = *apvts.getRawParameterValue("pitch");
    p->density        = *apvts.getRawParameterValue("density");
    p->texture        = *apvts.getRawParameterValue("texture");
    p->dry_wet        = *apvts.getRawParameterValue("drywet");
    p->stereo_spread  = *apvts.getRawParameterValue("stereo");
    p->feedback       = *apvts.getRawParameterValue("feedback");
    p->reverb         = *apvts.getRawParameterValue("reverb");
    p->freeze         = apvts.getRawParameterValue("freeze")->load() > 0.5f;
    p->trigger        = false;
    p->gate           = false;

    int modeIndex = static_cast<int>(apvts.getRawParameterValue("mode")->load());
    processor_.set_playback_mode(
        static_cast<clouds::PlaybackMode>(
            juce::jlimit(0, static_cast<int>(clouds::PLAYBACK_MODE_LAST) - 1, modeIndex)));

    int qualityIndex = static_cast<int>(apvts.getRawParameterValue("quality")->load());
    processor_.set_quality(juce::jlimit(0, 3, qualityIndex));

    // ---- リサンプリング: DAW rate → 32kHz ----
    // 線形補間による簡易リサンプリング
    const double ratio = kCloudsSampleRate / hostSampleRate_;
    int cloudsFrames = 0;

    // 入力をダウンサンプリング（もしくはアップサンプリング）
    resamplePhase_ = 0.0; // ブロック先頭でリセット（簡易実装）
    for (int i = 0; ; ++i)
    {
        double srcPos = static_cast<double>(i) / ratio;
        int srcIndex = static_cast<int>(srcPos);
        if (srcIndex >= numSamples - 1)
            break;
        float frac = static_cast<float>(srcPos - srcIndex);

        inputResampleBufL_[i] = inL[srcIndex] + frac * (inL[srcIndex + 1] - inL[srcIndex]);
        inputResampleBufR_[i] = inR[srcIndex] + frac * (inR[srcIndex + 1] - inR[srcIndex]);
        cloudsFrames = i + 1;
    }

    // ---- Clouds 処理（32サンプルブロック単位） ----
    // Clouds の Process() は ShortFrame (int16) を受け取る
    constexpr int kBlockSize = 32; // clouds::kMaxBlockSize

    int processed = 0;
    while (processed < cloudsFrames)
    {
        int blockSize = std::min(kBlockSize, cloudsFrames - processed);

        clouds::ShortFrame inputFrames[kBlockSize];
        clouds::ShortFrame outputFrames[kBlockSize];

        // float → int16 変換
        for (int j = 0; j < blockSize; ++j)
        {
            inputFrames[j].l = static_cast<int16_t>(
                juce::jlimit(-32768.0f, 32767.0f,
                             inputResampleBufL_[processed + j] * 32768.0f));
            inputFrames[j].r = static_cast<int16_t>(
                juce::jlimit(-32768.0f, 32767.0f,
                             inputResampleBufR_[processed + j] * 32768.0f));
        }

        // Prepare() は Process() の前に毎ブロック呼ぶ
        processor_.Prepare();
        processor_.Process(inputFrames, outputFrames, blockSize);

        // int16 → float 変換
        for (int j = 0; j < blockSize; ++j)
        {
            outputResampleBufL_[processed + j] =
                static_cast<float>(outputFrames[j].l) / 32768.0f;
            outputResampleBufR_[processed + j] =
                static_cast<float>(outputFrames[j].r) / 32768.0f;
        }

        processed += blockSize;
    }

    // ---- リサンプリング: 32kHz → DAW rate ----
    const double invRatio = hostSampleRate_ / kCloudsSampleRate;
    for (int i = 0; i < numSamples; ++i)
    {
        double srcPos = static_cast<double>(i) / invRatio;
        int srcIndex = static_cast<int>(srcPos);
        if (srcIndex >= cloudsFrames - 1)
            srcIndex = cloudsFrames - 2;
        if (srcIndex < 0)
            srcIndex = 0;
        float frac = static_cast<float>(srcPos - srcIndex);

        outL[i] = outputResampleBufL_[srcIndex]
                + frac * (outputResampleBufL_[srcIndex + 1]
                        - outputResampleBufL_[srcIndex]);
        outR[i] = outputResampleBufR_[srcIndex]
                + frac * (outputResampleBufR_[srcIndex + 1]
                        - outputResampleBufR_[srcIndex]);
    }
}

//==============================================================================
void CloudsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void CloudsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

//==============================================================================
juce::AudioProcessorEditor* CloudsAudioProcessor::createEditor()
{
    return new CloudsAudioProcessorEditor(*this);
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new CloudsAudioProcessor();
}

Copy
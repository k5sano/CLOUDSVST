#include "CloudsEngine.h"

// Include the actual Clouds DSP code
#include "clouds/dsp/granular_processor.h"
#include "clouds/dsp/frame.h"

#include <algorithm>
#include <cmath>

CloudsEngine::CloudsEngine()
{
}

CloudsEngine::~CloudsEngine()
{
}

void CloudsEngine::init()
{
    largeBuffer_ = std::make_unique<uint8_t[]>(kLargeBufferSize);
    smallBuffer_ = std::make_unique<uint8_t[]>(kSmallBufferSize);

    std::memset(largeBuffer_.get(), 0, kLargeBufferSize);
    std::memset(smallBuffer_.get(), 0, kSmallBufferSize);

    processor_ = std::make_unique<clouds::GranularProcessor>();
    processor_->Init(
        largeBuffer_.get(), kLargeBufferSize,
        smallBuffer_.get(), kSmallBufferSize);

    // Sensible defaults
    processor_->set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
    processor_->set_quality(0);  // 16-bit stereo
    processor_->set_bypass(false);
    processor_->set_silence(false);

    auto* p = processor_->mutable_parameters();
    p->position = 0.5f;
    p->size = 0.5f;
    p->pitch = 0.0f;
    p->density = 0.7f;
    p->texture = 0.5f;
    p->dry_wet = 0.5f;
    p->stereo_spread = 0.0f;
    p->feedback = 0.0f;
    p->reverb = 0.0f;
    p->freeze = false;
    p->trigger = false;
    p->gate = false;

    initialised_ = true;
}

void CloudsEngine::setMeterPointers(std::atomic<float>* d, std::atomic<float>* e)
{
    meterD_ = d;
    meterE_ = e;
}

void CloudsEngine::process(const float* inputL, const float* inputR,
                           float* outputL, float* outputR,
                           int numSamples)
{
    if (!initialised_ || processor_ == nullptr)
    {
        std::fill(outputL, outputL + numSamples, 0.0f);
        std::fill(outputR, outputR + numSamples, 0.0f);
        return;
    }

    // Process in kBlockSize chunks
    int remaining = numSamples;
    int offset = 0;
    float maxPeakD = 0.0f;
    float maxPeakE = 0.0f;

    while (remaining > 0)
    {
        const int blockSize = std::min(remaining, kBlockSize);

        // [D] Engine input probe - measure peak for meter
        float peakD = 0.0f;
        for (int i = 0; i < blockSize; ++i)
        {
            float absL = std::abs(inputL[offset + i]);
            float absR = std::abs(inputR[offset + i]);
            float v = (absL + absR) * 0.5f;
            if (v > peakD) peakD = v;
        }
        if (peakD > maxPeakD) maxPeakD = peakD;

        probeD_.measureStereo(inputL + offset, inputR + offset, blockSize);

        // Convert float to ShortFrame (int16 stereo)
        clouds::ShortFrame inputFrames[kBlockSize];
        clouds::ShortFrame outputFrames[kBlockSize];

        for (int i = 0; i < blockSize; ++i)
        {
            float l = inputL[offset + i];
            float r = inputR[offset + i];

            // Clip to [-1, 1] and convert to int16
            l = std::max(-1.0f, std::min(1.0f, l));
            r = std::max(-1.0f, std::min(1.0f, r));

            inputFrames[i].l = static_cast<int16_t>(l * 32767.0f);
            inputFrames[i].r = static_cast<int16_t>(r * 32767.0f);
        }

        // Hold last valid sample instead of zero-filling (reduces clicks)
        if (blockSize < kBlockSize)
        {
            const auto& lastSample = inputFrames[blockSize - 1];
            for (int i = blockSize; i < kBlockSize; ++i)
            {
                inputFrames[i] = lastSample;
            }
        }

        std::memset(outputFrames, 0, sizeof(outputFrames));

        // Run Prepare() multiple times to allow correlator to converge (especially for Stretch mode)
        // On hardware, Prepare() runs thousands of times between Process() calls
        for (int p = 0; p < prepareCallsPerBlock_; ++p)
            processor_->Prepare();

        // Run the Clouds DSP
        processor_->Process(inputFrames, outputFrames, kBlockSize);

        // Convert back to float
        for (int i = 0; i < blockSize; ++i)
        {
            outputL[offset + i] = static_cast<float>(outputFrames[i].l) / 32768.0f;
            outputR[offset + i] = static_cast<float>(outputFrames[i].r) / 32768.0f;
        }

        // [E] Engine output probe - measure peak for meter
        float peakE = 0.0f;
        for (int i = 0; i < blockSize; ++i)
        {
            float absL = std::abs(outputL[offset + i]);
            float absR = std::abs(outputR[offset + i]);
            float v = (absL + absR) * 0.5f;
            if (v > peakE) peakE = v;
        }
        if (peakE > maxPeakE) maxPeakE = peakE;

        probeE_.measureStereo(outputL + offset, outputR + offset, blockSize);

        offset += blockSize;
        remaining -= blockSize;
    }

    // Update atomic meters for GUI
    if (meterD_) meterD_->store(maxPeakD, std::memory_order_relaxed);
    if (meterE_) meterE_->store(maxPeakE, std::memory_order_relaxed);

    // Reset trigger after processing all chunks (momentary behaviour)
    processor_->mutable_parameters()->trigger = false;
}

// --- Parameter setters ---

void CloudsEngine::setPosition(float v)
{
    if (processor_)
        processor_->mutable_parameters()->position = v;
}

void CloudsEngine::setSize(float v)
{
    if (processor_)
        processor_->mutable_parameters()->size = v;
}

void CloudsEngine::setPitch(float v)
{
    if (processor_)
        processor_->mutable_parameters()->pitch = v;
}

void CloudsEngine::setDensity(float v)
{
    if (processor_)
        processor_->mutable_parameters()->density = v;
}

void CloudsEngine::setTexture(float v)
{
    if (processor_)
        processor_->mutable_parameters()->texture = v;
}

void CloudsEngine::setDryWet(float v)
{
    if (processor_)
        processor_->mutable_parameters()->dry_wet = v;
}

void CloudsEngine::setStereoSpread(float v)
{
    if (processor_)
        processor_->mutable_parameters()->stereo_spread = v;
}

void CloudsEngine::setFeedback(float v)
{
    if (processor_)
        processor_->mutable_parameters()->feedback = v;
}

void CloudsEngine::setReverb(float v)
{
    if (processor_)
        processor_->mutable_parameters()->reverb = v;
}

void CloudsEngine::setFreeze(bool v)
{
    if (processor_)
        processor_->mutable_parameters()->freeze = v;
}

void CloudsEngine::setTrigger(bool v)
{
    if (processor_)
        processor_->mutable_parameters()->trigger = v;
}

void CloudsEngine::setPlaybackMode(int mode)
{
    if (processor_ && mode >= 0 && mode < clouds::PLAYBACK_MODE_LAST)
    {
        processor_->set_playback_mode(static_cast<clouds::PlaybackMode>(mode));
        // Stretch mode needs many Prepare() calls for correlator
        prepareCallsPerBlock_ = (mode == clouds::PLAYBACK_MODE_STRETCH) ? 32 : 1;
    }
}

void CloudsEngine::setQuality(int quality)
{
    if (processor_ && quality >= 0 && quality <= 3)
        processor_->set_quality(quality);
}

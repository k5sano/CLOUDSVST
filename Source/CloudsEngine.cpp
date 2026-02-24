#include "CloudsEngine.h"

// Include the actual Clouds DSP code #include "clouds/dsp/granular\_processor.h"

#include #include

CloudsEngine::CloudsEngine() { }

CloudsEngine::~CloudsEngine() { }

void CloudsEngine::init() { largeBuffer\_ = std::make\_unique<uint8\_t\[\]>(kLargeBufferSize); smallBuffer\_ = std::make\_unique<uint8\_t\[\]>(kSmallBufferSize);

```
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
p->density = 0.5f;
p->texture = 0.5f;
p->dry_wet = 0.5f;
p->stereo_spread = 0.0f;
p->feedback = 0.0f;
p->reverb = 0.0f;
p->freeze = false;
p->trigger = false;
p->gate = false;

initialised_ = true;
```

}

void CloudsEngine::process(const float\* inputL, const float\* inputR, float\* outputL, float\* outputR, int numSamples) { if (!initialised\_ || processor\_ == nullptr) { std::fill(outputL, outputL + numSamples, 0.0f); std::fill(outputR, outputR + numSamples, 0.0f); return; }

```
// Process in kBlockSize chunks
int remaining = numSamples;
int offset = 0;

while (remaining > 0)
{
    const int blockSize = std::min(remaining, kBlockSize);

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

    // Zero-fill any remainder if blockSize < kBlockSize
    for (int i = blockSize; i < kBlockSize; ++i)
    {
        inputFrames[i].l = 0;
        inputFrames[i].r = 0;
    }

    std::memset(outputFrames, 0, sizeof(outputFrames));

    // Run the Clouds DSP
    processor_->Prepare();
    processor_->Process(inputFrames, outputFrames, kBlockSize);

    // Reset trigger after processing (momentary behaviour)
    processor_->mutable_parameters()->trigger = false;

    // Convert back to float
    for (int i = 0; i < blockSize; ++i)
    {
        outputL[offset + i] = static_cast<float>(outputFrames[i].l) / 32768.0f;
        outputR[offset + i] = static_cast<float>(outputFrames[i].r) / 32768.0f;
    }

    offset += blockSize;
    remaining -= blockSize;
}
```

}

// --- Parameter setters ---

void CloudsEngine::setPosition(float v) { if (processor\_) processor\_->mutable\_parameters()->position = v; }

void CloudsEngine::setSize(float v) { if (processor\_) processor\_->mutable\_parameters()->size = v; }

void CloudsEngine::setPitch(float v) { if (processor\_) processor\_->mutable\_parameters()->pitch = v; }

void CloudsEngine::setDensity(float v) { if (processor\_) processor\_->mutable\_parameters()->density = v; }

void CloudsEngine::setTexture(float v) { if (processor\_) processor\_->mutable\_parameters()->texture = v; }

void CloudsEngine::setDryWet(float v) { if (processor\_) processor\_->mutable\_parameters()->dry\_wet = v; }

void CloudsEngine::setStereoSpread(float v) { if (processor\_) processor\_->mutable\_parameters()->stereo\_spread = v; }

void CloudsEngine::setFeedback(float v) { if (processor\_) processor\_->mutable\_parameters()->feedback = v; }

void CloudsEngine::setReverb(float v) { if (processor\_) processor\_->mutable\_parameters()->reverb = v; }

void CloudsEngine::setFreeze(bool v) { if (processor\_) processor\_->mutable\_parameters()->freeze = v; }

void CloudsEngine::setTrigger(bool v) { if (processor\_) processor\_->mutable\_parameters()->trigger = v; }

void CloudsEngine::setPlaybackMode(int mode) { if (processor\_ && mode >= 0 && mode < clouds::PLAYBACK\_MODE\_LAST) processor\_->set\_playback\_mode(static\_castclouds::PlaybackMode(mode)); }

void CloudsEngine::setQuality(int quality) { if (processor\_ && quality >= 0 && quality <= 3) processor\_->set\_quality(quality); }
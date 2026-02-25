#include "CloudsEngine.h"
#include "clouds/dsp/granular_processor.h"
#include "clouds/dsp/frame.h"

#include <algorithm>
#include <cmath>

CloudsEngine::CloudsEngine() {}
CloudsEngine::~CloudsEngine() {}

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

    processor_->set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
    processor_->set_quality(0);
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

    // Init() 後 previous_playback_mode_ = PLAYBACK_MODE_LAST のため
    // 最初の数ブロックは Process() がゼロ出力する。
    // VCV Rack と同様に、Granular モードで空回しして状態を安定させる。
    {
        clouds::ShortFrame dummyIn[kBlockSize] = {};
        clouds::ShortFrame dummyOut[kBlockSize] = {};
        for (int i = 0; i < 10; ++i) {
            processor_->Prepare();
            processor_->Process(dummyIn, dummyOut, kBlockSize);
        }
    }

    initialised_ = true;
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

    float peakD = 0.0f;
    float peakE = 0.0f;

    int remaining = numSamples;
    int offset = 0;

    while (remaining > 0)
    {
        const int blockSize = std::min(remaining, kBlockSize);

        clouds::ShortFrame inputFrames[kBlockSize];
        clouds::ShortFrame outputFrames[kBlockSize];

        for (int i = 0; i < blockSize; ++i)
        {
            float l = inputL[offset + i] * inputTrim_;
            float r = inputR[offset + i] * inputTrim_;

            l = std::max(-1.0f, std::min(1.0f, l));
            r = std::max(-1.0f, std::min(1.0f, r));

            inputFrames[i].l = static_cast<int16_t>(l * 32767.0f);
            inputFrames[i].r = static_cast<int16_t>(r * 32767.0f);

            // Meter D: engine input level (after trim, before int16 conversion is lossy, measure float)
            float absV = (std::abs(l) + std::abs(r)) * 0.5f;
            if (absV > peakD) peakD = absV;
        }

        // Parameter smoothing (one-pole filter per block)
        auto smooth = [](float& current, float target, float coeff) {
            current += coeff * (target - current);
        };
        smooth(smoothedPosition_, targetPosition_, kSmoothingCoeff);
        smooth(smoothedSize_, targetSize_, kSmoothingCoeff);
        smooth(smoothedPitch_, targetPitch_, kSmoothingCoeff);
        smooth(smoothedDensity_, targetDensity_, kSmoothingCoeff);
        smooth(smoothedTexture_, targetTexture_, kSmoothingCoeff);
        smooth(smoothedDryWet_, targetDryWet_, kSmoothingCoeff);
        smooth(smoothedSpread_, targetSpread_, kSmoothingCoeff);
        smooth(smoothedFeedback_, targetFeedback_, kSmoothingCoeff);
        smooth(smoothedReverb_, targetReverb_, kSmoothingCoeff);

        // Apply smoothed parameters to processor
        auto* p = processor_->mutable_parameters();
        p->position = smoothedPosition_;
        p->size = smoothedSize_;
        p->pitch = smoothedPitch_;
        p->density = smoothedDensity_;
        p->texture = smoothedTexture_;
        p->dry_wet = smoothedDryWet_;
        p->stereo_spread = smoothedSpread_;
        p->feedback = smoothedFeedback_;
        p->reverb = smoothedReverb_;

        for (int i = blockSize; i < kBlockSize; ++i)
        {
            inputFrames[i].l = 0;
            inputFrames[i].r = 0;
        }

        std::memset(outputFrames, 0, sizeof(outputFrames));

        // VCV Rack / ctag-tbd approach: Prepare 1回 → Process 1回
        processor_->Prepare();
        processor_->Process(inputFrames, outputFrames, kBlockSize);
        processor_->mutable_parameters()->trigger = false;

        for (int i = 0; i < blockSize; ++i)
        {
            float outLf = static_cast<float>(outputFrames[i].l) / 32768.0f * outputGain_;
            float outRf = static_cast<float>(outputFrames[i].r) / 32768.0f * outputGain_;

            outputL[offset + i] = outLf;
            outputR[offset + i] = outRf;

            // Meter E: engine output level
            float absV = (std::abs(outLf) + std::abs(outRf)) * 0.5f;
            if (absV > peakE) peakE = absV;
        }

        offset += blockSize;
        remaining -= blockSize;
    }

    // Store meter values (lock-free)
    if (meterD_) meterD_->store(peakD, std::memory_order_relaxed);
    if (meterE_) meterE_->store(peakE, std::memory_order_relaxed);
}

void CloudsEngine::setPlaybackMode(int mode)
{
    if (processor_ && mode >= 0 && mode < 4)
        processor_->set_playback_mode(static_cast<clouds::PlaybackMode>(mode));
}

void CloudsEngine::setQuality(int quality)
{
    if (processor_ && quality >= 0 && quality <= 3)
        processor_->set_quality(quality);
}

void CloudsEngine::setFreeze(bool v)
{
    if (processor_) processor_->mutable_parameters()->freeze = v;
}

void CloudsEngine::setTrigger(bool v)
{
    if (processor_) processor_->mutable_parameters()->trigger = v;
}

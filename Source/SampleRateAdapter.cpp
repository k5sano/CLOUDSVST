#include "SampleRateAdapter.h"
#include <algorithm>

void SampleRateAdapter::prepare(double hostSampleRate, int /*maxBlockSize*/)
{
    hostSampleRate_ = hostSampleRate;
    ratio_ = hostSampleRate / kInternalSampleRate;

    inputPhase_ = 0.0;
    inputWritePos_ = 4;
    inputSamplesAvailable_ = 0;
    std::memset(inputRingL_, 0, sizeof(inputRingL_));
    std::memset(inputRingR_, 0, sizeof(inputRingR_));

    outputPhase_ = 0.0;
    outputWritePos_ = 0;
    outputReadPos_ = 0;
    outputSamplesAvailable_ = 0;
    std::memset(outputRingL_, 0, sizeof(outputRingL_));
    std::memset(outputRingR_, 0, sizeof(outputRingR_));

    std::memset(engineInL_, 0, sizeof(engineInL_));
    std::memset(engineInR_, 0, sizeof(engineInR_));
    std::memset(engineOutL_, 0, sizeof(engineOutL_));
    std::memset(engineOutR_, 0, sizeof(engineOutR_));
}

void SampleRateAdapter::process(const float* inL, const float* inR,
                                 float* outL, float* outR,
                                 int numSamples,
                                 CloudsEngine& engine)
{
    if (numSamples <= 0)
        return;

    if (std::abs(hostSampleRate_ - kInternalSampleRate) < 1.0)
    {
        engine.process(inL, inR, outL, outR, numSamples);
        return;
    }

    // Step 1: Write host input into input ring buffer
    for (int i = 0; i < numSamples; ++i)
    {
        inputRingL_[inputWritePos_] = inL[i];
        inputRingR_[inputWritePos_] = inR[i];
        inputWritePos_ = (inputWritePos_ + 1) % kInputRingSize;
    }
    inputSamplesAvailable_ += numSamples;

    // Step 2: Downsample → engine → output ring
    while (inputSamplesAvailable_ >= static_cast<int>(ratio_ * kBlockSize) + 4)
    {
        int readBase = (inputWritePos_ - inputSamplesAvailable_ + kInputRingSize) % kInputRingSize;

        for (int i = 0; i < kBlockSize; ++i)
        {
            double hostPos = inputPhase_ + i * ratio_;
            int intPos = static_cast<int>(hostPos);
            float frac = static_cast<float>(hostPos - intPos);
            int ringPos = (readBase + intPos) % kInputRingSize;

            engineInL_[i] = readInputRing(inputRingL_, ringPos, frac);
            engineInR_[i] = readInputRing(inputRingR_, ringPos, frac);
        }

        double consumed = kBlockSize * ratio_;
        int consumedInt = static_cast<int>(consumed);
        inputPhase_ = (inputPhase_ + consumed) - consumedInt;
        inputPhase_ -= static_cast<int>(inputPhase_);
        inputSamplesAvailable_ -= consumedInt;

        engine.process(engineInL_, engineInR_, engineOutL_, engineOutR_, kBlockSize);

        for (int i = 0; i < kBlockSize; ++i)
        {
            outputRingL_[outputWritePos_] = engineOutL_[i];
            outputRingR_[outputWritePos_] = engineOutR_[i];
            outputWritePos_ = (outputWritePos_ + 1) % kOutputRingSize;
        }
        outputSamplesAvailable_ += kBlockSize;
    }

    // Step 3: Upsample from output ring → host output
    double invRatio = 1.0 / ratio_;

    for (int i = 0; i < numSamples; ++i)
    {
        if (outputSamplesAvailable_ > 4)
        {
            int intPos = static_cast<int>(outputPhase_);
            float frac = static_cast<float>(outputPhase_ - intPos);
            int ringPos = (outputReadPos_ + intPos) % kOutputRingSize;

            outL[i] = readOutputRing(outputRingL_, ringPos, frac);
            outR[i] = readOutputRing(outputRingR_, ringPos, frac);

            outputPhase_ += invRatio;
            int advance = static_cast<int>(outputPhase_);
            outputPhase_ -= advance;
            outputReadPos_ = (outputReadPos_ + advance) % kOutputRingSize;
            outputSamplesAvailable_ -= advance;
        }
        else
        {
            outL[i] = lastOutputL_;
            outR[i] = lastOutputR_;
        }

        // Update held values
        if (outputSamplesAvailable_ > 4)
        {
            lastOutputL_ = outL[i];
            lastOutputR_ = outR[i];
        }
    }
}

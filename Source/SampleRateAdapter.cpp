#include "SampleRateAdapter.h"
#include <algorithm>
#include <cstring>

void SampleRateAdapter::prepare(double hostSampleRate, int /*maxBlockSize*/)
{
    hostSampleRate_ = hostSampleRate;
    ratio_ = hostSampleRate / kInternalSampleRate;

    inputPhase_ = 0.0;
    inputWritePos_ = 4;  // Leave room for interpolation lookback
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

    // --- Bypass if host rate matches internal rate ---
    if (std::abs(hostSampleRate_ - kInternalSampleRate) < 1.0)
    {
        engine.process(inL, inR, outL, outR, numSamples);
        return;
    }

    // === Step 1: Write host input into input ring buffer ===
    for (int i = 0; i < numSamples; ++i)
    {
        inputRingL_[inputWritePos_] = inL[i];
        inputRingR_[inputWritePos_] = inR[i];
        inputWritePos_ = (inputWritePos_ + 1) % kInputRingSize;
    }
    inputSamplesAvailable_ += numSamples;

    // === Step 2: Downsample from input ring → engine, process, write to output ring ===
    // For each internal sample, we consume ratio_ host samples
    // We can produce internal samples as long as we have enough host input
    while (inputSamplesAvailable_ >= (int)(ratio_ * kBlockSize) + 4)
    {
        // Read kBlockSize samples at internal rate from host input ring
        int readBase = (inputWritePos_ - inputSamplesAvailable_ + kInputRingSize) % kInputRingSize;

        for (int i = 0; i < kBlockSize; ++i)
        {
            double hostPos = inputPhase_ + i * ratio_;
            int intPos = (int)hostPos;
            float frac = (float)(hostPos - intPos);
            int ringPos = (readBase + intPos) % kInputRingSize;

            engineInL_[i] = readInputRing(inputRingL_, ringPos, frac);
            engineInR_[i] = readInputRing(inputRingR_, ringPos, frac);
        }

        // Advance input consumption
        double consumed = kBlockSize * ratio_;
        int consumedInt = (int)consumed;
        inputPhase_ = (inputPhase_ + consumed) - consumedInt;
        // Keep only fractional part relative to next block
        inputPhase_ -= (int)inputPhase_;
        inputSamplesAvailable_ -= consumedInt;

        // Process through Clouds engine
        engine.process(engineInL_, engineInR_, engineOutL_, engineOutR_, kBlockSize);

        // Write engine output to output ring
        for (int i = 0; i < kBlockSize; ++i)
        {
            outputRingL_[outputWritePos_] = engineOutL_[i];
            outputRingR_[outputWritePos_] = engineOutR_[i];
            outputWritePos_ = (outputWritePos_ + 1) % kOutputRingSize;
        }
        outputSamplesAvailable_ += kBlockSize;
    }

    // === Step 3: Upsample from output ring → host output ===
    // For each host output sample, we consume 1/ratio_ internal samples
    double invRatio = 1.0 / ratio_;

    for (int i = 0; i < numSamples; ++i)
    {
        if (outputSamplesAvailable_ > 4)
        {
            int intPos = (int)outputPhase_;
            float frac = (float)(outputPhase_ - intPos);
            int ringPos = (outputReadPos_ + intPos) % kOutputRingSize;

            outL[i] = readOutputRing(outputRingL_, ringPos, frac);
            outR[i] = readOutputRing(outputRingR_, ringPos, frac);

            outputPhase_ += invRatio;
            int advance = (int)outputPhase_;
            outputPhase_ -= advance;
            outputReadPos_ = (outputReadPos_ + advance) % kOutputRingSize;
            outputSamplesAvailable_ -= advance;
        }
        else
        {
            // Underrun: output silence
            outL[i] = 0.0f;
            outR[i] = 0.0f;
        }
    }
}

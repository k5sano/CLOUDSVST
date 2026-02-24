#include "SampleRateAdapter.h" #include #include

void SampleRateAdapter::prepare(double hostSampleRate, int maxBlockSize) { hostSampleRate\_ = hostSampleRate; ratio\_ = hostSampleRate / kInternalSampleRate;

```
// Calculate the maximum number of internal samples we might need
// Add generous padding for resampler latency
maxInternalSamples_ = static_cast<int>(
    std::ceil(static_cast<double>(maxBlockSize) / ratio_)) + 64;

internalInput_.setSize(2, maxInternalSamples_);
internalOutput_.setSize(2, maxInternalSamples_);

interpDownL_.reset();
interpDownR_.reset();
interpUpL_.reset();
interpUpR_.reset();
```

}

void SampleRateAdapter::process(const float\* inL, const float\* inR, float\* outL, float\* outR, int numSamples, CloudsEngine& engine) { if (numSamples <= 0) return;

```
// --- Special case: host SR == internal SR ---
if (std::abs(hostSampleRate_ - kInternalSampleRate) < 1.0)
{
    engine.process(inL, inR, outL, outR, numSamples);
    return;
}

// --- Downsample host → internal rate ---
const int internalSamples = static_cast<int>(
    std::ceil(static_cast<double>(numSamples) / ratio_));

jassert(internalSamples <= maxInternalSamples_);

auto* intInL = internalInput_.getWritePointer(0);
auto* intInR = internalInput_.getWritePointer(1);

// LagrangeInterpolator::process(speedRatio, inputData, outputData, numOutputSamples)
// speedRatio = input_rate / output_rate = hostSR / internalSR = ratio_
interpDownL_.process(ratio_, inL, intInL, internalSamples);
interpDownR_.process(ratio_, inR, intInR, internalSamples);

// --- Process at internal rate ---
auto* intOutL = internalOutput_.getWritePointer(0);
auto* intOutR = internalOutput_.getWritePointer(1);

engine.process(intInL, intInR, intOutL, intOutR, internalSamples);

// --- Upsample internal → host rate ---
// speedRatio = internal_rate / host_rate = 1.0 / ratio_
interpUpL_.process(1.0 / ratio_, intOutL, outL, numSamples);
interpUpR_.process(1.0 / ratio_, intOutR, outR, numSamples);
```

}
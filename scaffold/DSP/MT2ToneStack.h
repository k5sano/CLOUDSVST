```cpp
Copy#pragma once
#include "BiquadFilter.h"

class MT2ToneStack {
public:
    MT2ToneStack() = default;

    void prepare(double sampleRate);
    void reset();

    /** Update all EQ coefficients. Call once per block. */
    void updateCoefficients(float eqLow, float eqMid, float eqMidFreq,
                            float eqMidQ, float eqHigh);

    double processSample(double input);

private:
    BiquadFilter mLowShelf;
    BiquadFilter mMidPeak;
    BiquadFilter mHighShelf;
    double mSampleRate = 44100.0;
};
```
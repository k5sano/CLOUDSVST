```cpp
Copy#include "BiquadFilter.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

void BiquadFilter::setLowShelf(double freqHz, double gainDb, double q, double sampleRate)
{
    const double A  = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * M_PI * freqHz / sampleRate;
    const double cosw0 = std::cos(w0);
    const double sinw0 = std::sin(w0);
    const double alpha = sinw0 / (2.0 * q);
    const double sqrtA = std::sqrt(A);

    b0 =     A * ((A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha);
    b1 = 2.0*A * ((A - 1.0) - (A + 1.0) * cosw0);
    b2 =     A * ((A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha);
    a0 =          (A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha;
    a1 =   -2.0*((A - 1.0) + (A + 1.0) * cosw0);
    a2 =          (A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha;

    // Normalize
    b0 /= a0; b1 /= a0; b2 /= a0;
    a1 /= a0; a2 /= a0; a0 = 1.0;
}

void BiquadFilter::setPeak(double freqHz, double gainDb, double q, double sampleRate)
{
    const double A  = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * M_PI * freqHz / sampleRate;
    const double cosw0 = std::cos(w0);
    const double sinw0 = std::sin(w0);
    const double alpha = sinw0 / (2.0 * q);

    b0 =  1.0 + alpha * A;
    b1 = -2.0 * cosw0;
    b2 =  1.0 - alpha * A;
    a0 =  1.0 + alpha / A;
    a1 = -2.0 * cosw0;
    a2 =  1.0 - alpha / A;

    b0 /= a0; b1 /= a0; b2 /= a0;
    a1 /= a0; a2 /= a0; a0 = 1.0;
}

void BiquadFilter::setHighShelf(double freqHz, double gainDb, double q, double sampleRate)
{
    const double A  = std::pow(10.0, gainDb / 40.0);
    const double w0 = 2.0 * M_PI * freqHz / sampleRate;
    const double cosw0 = std::cos(w0);
    const double sinw0 = std::sin(w0);
    const double alpha = sinw0 / (2.0 * q);
    const double sqrtA = std::sqrt(A);

    b0 =     A * ((A + 1.0) + (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha);
    b1 =-2.0*A * ((A - 1.0) + (A + 1.0) * cosw0);
    b2 =     A * ((A + 1.0) + (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha);
    a0 =          (A + 1.0) - (A - 1.0) * cosw0 + 2.0 * sqrtA * alpha;
    a1 =    2.0*((A - 1.0) - (A + 1.0) * cosw0);
    a2 =          (A + 1.0) - (A - 1.0) * cosw0 - 2.0 * sqrtA * alpha;

    b0 /= a0; b1 /= a0; b2 /= a0;
    a1 /= a0; a2 /= a0; a0 = 1.0;
}

void BiquadFilter::reset()
{
    z1 = 0.0;
    z2 = 0.0;
}

double BiquadFilter::processSample(double input)
{
    // Direct Form II Transposed
    const double output = b0 * input + z1;
    z1 = b1 * input - a1 * output + z2;
    z2 = b2 * input - a2 * output;
    return output;
}
Copy
```
```cpp
Copy#include "OnePoleFilter.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

OnePoleFilter::OnePoleFilter(Type type) : mType(type) {}

void OnePoleFilter::setType(Type type)
{
    mType = type;
}

void OnePoleFilter::setCutoffFrequency(double freqHz, double sampleRate)
{
    const double w = 2.0 * M_PI * freqHz / sampleRate;
    const double g = std::tan(w * 0.5);

    if (mType == Type::LPF) {
        mA0 = g / (1.0 + g);
        mB1 = (1.0 - g) / (1.0 + g); // Not used directly in TPT form
    } else {
        mA0 = 1.0 / (1.0 + g);
        mB1 = g; // Not used directly in TPT form
    }

    // We use the Topology Preserving Transform (TPT) form:
    // For LPF: y = g/(1+g) * (x - z1) + z1;  z1 = 2*y - z1
    // For HPF: hp = x - lp
    // Store 'g' for the TPT form
    mA0 = g;
}

void OnePoleFilter::reset()
{
    mZ1 = 0.0;
}

double OnePoleFilter::processSample(double input)
{
    // TPT one-pole
    const double g = mA0;
    const double v = (input - mZ1) * g / (1.0 + g);
    const double lp = v + mZ1;
    mZ1 = lp + v;

    if (mType == Type::LPF)
        return lp;
    else
        return input - lp; // HPF
}
Copy
```
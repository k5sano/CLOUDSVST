```cpp
Copy#include "DiodeFeedbackClipper.h"

void DiodeFeedbackClipper::setDiodeParams(double is, double n)
{
    mIs = is;
    mN  = n;
}

void DiodeFeedbackClipper::setGain(double gain)
{
    mGain = gain;
}

void DiodeFeedbackClipper::setSampleRate(double /*sampleRate*/)
{
    // Reserved for future oversampling-aware processing
}

void DiodeFeedbackClipper::reset()
{
    mPrevOutput = 0.0;
}

void DiodeFeedbackClipper::setBypass(bool shouldBypass)
{
    mBypassed = shouldBypass;
}

double DiodeFeedbackClipper::processSample(double input)
{
    const double target = input * mGain;

    if (mBypassed)
        return target;

    // Newton-Raphson to solve: f(Vout) = Vout + Rf * 2 * Is * sinh(Vout / (n*VT)) - target = 0
    const double nVT = mN * VT;
    double vOut = mPrevOutput; // initial guess

    for (int iter = 0; iter < MAX_ITER; ++iter) {
        const double sinhVal = std::sinh(vOut / nVT);
        const double coshVal = std::cosh(vOut / nVT);

        const double f  = vOut + mRf * 2.0 * mIs * sinhVal - target;
        const double fp = 1.0 + mRf * 2.0 * mIs * coshVal / nVT;

        if (std::abs(fp) < 1e-15)
            break;

        const double delta = f / fp;
        vOut -= delta;

        if (std::abs(delta) < TOLERANCE)
            break;
    }

    mPrevOutput = vOut;
    return vOut;
}
Copy
```
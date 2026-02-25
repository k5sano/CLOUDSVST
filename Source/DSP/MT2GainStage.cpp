```cpp
Copy#include "MT2GainStage.h"

MT2GainStage::MT2GainStage()
    : mInterstageHPF(OnePoleFilter::Type::HPF),
      mInterStageLPF(OnePoleFilter::Type::LPF)
{
}

void MT2GainStage::prepare(double sampleRate)
{
    mStage1.setSampleRate(sampleRate);
    mStage2.setSampleRate(sampleRate);
    mInterstageHPF.setCutoffFrequency(200.0, sampleRate);
    mInterStageLPF.setCutoffFrequency(5500.0, sampleRate);
    reset();
}

void MT2GainStage::reset()
{
    mStage1.reset();
    mStage2.reset();
    mInterstageHPF.reset();
    mInterStageLPF.reset();
}

void MT2GainStage::setGain(double gain)
{
    mStage1.setGain(gain);
    mStage2.setGain(gain);
}

void MT2GainStage::setStage1Diode(double is, double n, bool noClip)
{
    mStage1.setDiodeParams(is, n);
    mStage1.setBypass(noClip);
}

void MT2GainStage::setStage2Diode(double is, double n, bool noClip)
{
    mStage2.setDiodeParams(is, n);
    mStage2.setBypass(noClip);
}

double MT2GainStage::processSample(double input)
{
    double x = mStage1.processSample(input);
    x = mInterstageHPF.processSample(x);
    x = mInterStageLPF.processSample(x);
    x = mStage2.processSample(x);
    return x;
}
Copy
```
```cpp
Copy#pragma once
#include "DiodeFeedbackClipper.h"
#include "OnePoleFilter.h"

class MT2GainStage {
public:
    MT2GainStage();

    void prepare(double sampleRate);
    void reset();

    void setGain(double gain);
    void setStage1Diode(double is, double n, bool noClip);
    void setStage2Diode(double is, double n, bool noClip);

    double processSample(double input);

private:
    DiodeFeedbackClipper mStage1;
    DiodeFeedbackClipper mStage2;
    OnePoleFilter mInterstageHPF;
    OnePoleFilter mInterStageLPF;
};
```
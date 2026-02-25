```cpp
Copy#pragma once

class OnePoleFilter {
public:
    enum class Type { HPF, LPF };

    OnePoleFilter() = default;
    explicit OnePoleFilter(Type type);

    void setType(Type type);
    void setCutoffFrequency(double freqHz, double sampleRate);
    void reset();

    double processSample(double input);

private:
    Type   mType = Type::LPF;
    double mA0 = 1.0;
    double mB1 = 0.0;
    double mZ1 = 0.0;
};
```
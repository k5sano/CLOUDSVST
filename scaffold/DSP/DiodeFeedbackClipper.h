```cpp
Copy#pragma once
#include <cmath>

class DiodeFeedbackClipper {
public:
    DiodeFeedbackClipper() = default;

    void setDiodeParams(double is, double n);
    void setGain(double gain);
    void setSampleRate(double sampleRate);
    void reset();

    /** Process a single sample through the diode feedback clipper.
        Uses Newton-Raphson iteration to solve the implicit nonlinear equation:
        Vout + Rf * 2 * Is * sinh(Vout / (n * VT)) = Vin * Gain
    */
    double processSample(double input);

    /** When true, bypass diode clipping: Vout = Vin * Gain */
    void setBypass(bool shouldBypass);

private:
    double mIs = 2.52e-9;    // Saturation current
    double mN  = 1.7;         // Ideality factor
    double mGain = 100.0;
    double mRf = 1.0;
    double mPrevOutput = 0.0; // Initial guess for Newton-Raphson
    bool   mBypassed = false;

    static constexpr double VT = 0.02585; // Thermal voltage at ~25°C
    static constexpr int    MAX_ITER = 8;
    static constexpr double TOLERANCE = 1e-7;
};
Copy
```
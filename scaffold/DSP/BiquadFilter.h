```cpp
Copy#pragma once
#include <cmath>

class BiquadFilter {
public:
    enum class Type { LowShelf, Peak, HighShelf };

    BiquadFilter() = default;

    /** Compute coefficients for Low Shelf filter */
    void setLowShelf(double freqHz, double gainDb, double q, double sampleRate);

    /** Compute coefficients for Peaking EQ filter */
    void setPeak(double freqHz, double gainDb, double q, double sampleRate);

    /** Compute coefficients for High Shelf filter */
    void setHighShelf(double freqHz, double gainDb, double q, double sampleRate);

    void reset();

    double processSample(double input);

private:
    // Direct Form II Transposed coefficients
    double b0 = 1.0, b1 = 0.0, b2 = 0.0;
    double a0 = 1.0, a1 = 0.0, a2 = 0.0;

    // State
    double z1 = 0.0, z2 = 0.0;
};
Copy
```
```cpp
Copy#pragma once
#include <array>

struct DiodeParams {
    double is;
    double n;
    bool   noClip;
};

class DiodeMorpher {
public:
    DiodeMorpher() = default;

    /** Given a morph value 0.0..1.0, return interpolated diode parameters.
        Morph regions: Si(0.0) → Ge(0.25) → LED(0.5) → Schottky(0.75) → NoClip(1.0)
    */
    DiodeParams getMorphedParams(float morphValue) const;

private:
    struct DiodeModel {
        double is;
        double n;
    };

    static constexpr std::array<DiodeModel, 4> models {{
        { 2.52e-9,  1.7  },  // Silicon
        { 2.2e-8,   1.05 },  // Germanium
        { 4.35e-10, 1.9  },  // LED
        { 7.4e-9,   1.9  },  // Schottky
    }};
};
Copy
```
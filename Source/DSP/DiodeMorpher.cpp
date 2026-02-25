```cpp
Copy#include "DiodeMorpher.h"
#include <algorithm>
#include <cmath>

DiodeParams DiodeMorpher::getMorphedParams(float morphValue) const
{
    morphValue = std::clamp(morphValue, 0.0f, 1.0f);

    // 4 regions: Si→Ge→LED→Schottky→NoClip
    // Region boundaries: 0.0, 0.25, 0.5, 0.75, 1.0
    constexpr float regionSize = 0.25f;
    constexpr int numModels = 4;

    // NoClip threshold
    constexpr float noClipStart = 0.75f;

    if (morphValue >= 1.0f) {
        return { 0.0, 0.0, true };
    }

    const int regionIndex = std::min(static_cast<int>(morphValue / regionSize),
                                     numModels - 2);
    const float regionFrac = (morphValue - regionIndex * regionSize) / regionSize;

    if (regionIndex < numModels - 1) {
        const auto& m0 = models[static_cast<size_t>(regionIndex)];
        const auto& m1 = models[static_cast<size_t>(regionIndex + 1)];

        double is = m0.is + (m1.is - m0.is) * static_cast<double>(regionFrac);
        double n  = m0.n  + (m1.n  - m0.n)  * static_cast<double>(regionFrac);

        // Crossfade into NoClip in the last region
        if (morphValue > noClipStart) {
            float noClipFrac = (morphValue - noClipStart) / regionSize;
            noClipFrac = std::clamp(noClipFrac, 0.0f, 1.0f);
            if (noClipFrac >= 1.0f)
                return { 0.0, 0.0, true };
            // We still return the interpolated params; the caller uses noClip flag
            return { is, n, false };
        }

        return { is, n, false };
    }

    // Fallback: last model
    return { models.back().is, models.back().n, false };
}
Copy
```
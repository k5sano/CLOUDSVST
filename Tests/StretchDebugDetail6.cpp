#include <iostream>
#include <cmath>
#include <cstring>
#include <memory>
#include "clouds/dsp/granular_processor.h"
#include "clouds/dsp/frame.h"

int main()
{
    const size_t kLarge = 118784;
    const size_t kSmall = 65536 - 128;
    auto lb = std::make_unique<uint8_t[]>(kLarge);
    auto sb = std::make_unique<uint8_t[]>(kSmall);
    std::memset(lb.get(), 0, kLarge);
    std::memset(sb.get(), 0, kSmall);

    clouds::GranularProcessor proc;
    proc.Init(lb.get(), kLarge, sb.get(), kSmall);
    
    proc.set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
    proc.set_quality(0);
    proc.set_bypass(false);
    proc.set_silence(false);
    
    auto* p = proc.mutable_parameters();
    p->position = 0.5f; p->size = 0.5f; p->pitch = 0.0f;
    p->density = 0.7f; p->texture = 0.5f;
    p->dry_wet = 1.0f; p->stereo_spread = 0.0f;
    p->feedback = 0.0f; p->reverb = 0.0f;
    p->freeze = false; p->trigger = false; p->gate = false;

    const int BS = 32;
    float phase = 0.0f;

    // 重要: Process の前に Prepare を呼ぶ（VCV Rack順序）
    std::cout << "Filling buffer with Prepare -> Process order..." << std::endl;
    for (int b = 0; b < 500; ++b) {
        proc.Prepare();  // Prepare first!
        
        clouds::ShortFrame in[BS], out[BS];
        for (int i = 0; i < BS; ++i) {
            float v = std::sin(2.f*3.14159265f*phase)*0.5f;
            in[i].l = in[i].r = static_cast<int16_t>(v*32767.f);
            phase += 440.f/32000.f;
        }
        std::memset(out, 0, sizeof(out));
        proc.Process(in, out, BS);  // Then Process
        
        if (b < 5 || b % 100 == 0) {
            float maxO = 0;
            for (int i = 0; i < BS; ++i) {
                float a = std::abs(static_cast<float>(out[i].l));
                if (a > maxO) maxO = a;
            }
            std::cout << "  Block " << b << " max=" << maxO << std::endl;
        }
    }

    // Freeze on
    std::cout << "\nFreeze ON..." << std::endl;
    p->freeze = true;
    for (int b = 0; b < 10; ++b) {
        proc.Prepare();
        clouds::ShortFrame in[BS], out[BS];
        std::memset(in, 0, sizeof(in));
        std::memset(out, 0, sizeof(out));
        proc.Process(in, out, BS);
        
        float maxO = 0;
        for (int i = 0; i < BS; ++i) {
            float a = std::abs(static_cast<float>(out[i].l));
            if (a > maxO) maxO = a;
        }
        std::cout << "  Freeze block " << b << " max=" << maxO << std::endl;
    }

    // Switch to Stretch
    std::cout << "\nSwitch to Stretch..." << std::endl;
    proc.set_playback_mode(clouds::PLAYBACK_MODE_STRETCH);
    
    for (int b = 0; b < 10; ++b) {
        proc.Prepare();
        clouds::ShortFrame in[BS], out[BS];
        std::memset(in, 0, sizeof(in));
        std::memset(out, 0, sizeof(out));
        proc.Process(in, out, BS);
        
        float maxO = 0;
        for (int i = 0; i < BS; ++i) {
            float a = std::abs(static_cast<float>(out[i].l));
            if (a > maxO) maxO = a;
        }
        std::cout << "  Stretch block " << b << " max=" << maxO << std::endl;
    }

    std::cout << "Done." << std::endl;
    return 0;
}

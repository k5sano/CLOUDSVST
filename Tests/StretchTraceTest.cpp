#include <iostream>
#include <cmath>
#include <cstring>
#include <memory>
#include "clouds/dsp/granular_processor.h"
#include "clouds/dsp/frame.h"

// Process() のガード条件を外部から推測するためのテスト
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

    // ===== テスト 1: Granular で空回し (PLAYBACK_MODE_LAST → GRANULAR 遷移) =====
    std::cout << "=== Test 1: Granular warmup ===" << std::endl;
    for (int b = 0; b < 20; ++b) {
        clouds::ShortFrame in[BS], out[BS];
        for (int i = 0; i < BS; ++i) {
            float v = std::sin(2.f*3.14159265f*phase)*0.5f;
            in[i].l = in[i].r = static_cast<int16_t>(v*32767.f);
            phase += 440.f/32000.f;
        }
        std::memset(out, 0, sizeof(out));
        proc.Prepare();
        proc.Process(in, out, BS);
        float mx = 0;
        for (int i = 0; i < BS; ++i) {
            float a = std::abs((float)out[i].l);
            if (a > mx) mx = a;
        }
        std::cout << "  Granular block " << b << " max=" << mx << std::endl;
    }

    // ===== テスト 2: Granular で500ブロック (バッファ充填) =====
    std::cout << "\n=== Test 2: Fill buffer (500 blocks) ===" << std::endl;
    for (int b = 0; b < 500; ++b) {
        clouds::ShortFrame in[BS], out[BS];
        for (int i = 0; i < BS; ++i) {
            float v = std::sin(2.f*3.14159265f*phase)*0.5f;
            in[i].l = in[i].r = static_cast<int16_t>(v*32767.f);
            phase += 440.f/32000.f;
        }
        std::memset(out, 0, sizeof(out));
        proc.Prepare();
        proc.Process(in, out, BS);
    }
    std::cout << "  Done filling." << std::endl;

    // ===== テスト 3: Stretch に切替、1ブロックずつ詳細観察 =====
    std::cout << "\n=== Test 3: Switch to Stretch ===" << std::endl;
    proc.set_playback_mode(clouds::PLAYBACK_MODE_STRETCH);

    for (int b = 0; b < 50; ++b) {
        clouds::ShortFrame in[BS], out[BS];
        for (int i = 0; i < BS; ++i) {
            float v = std::sin(2.f*3.14159265f*phase)*0.5f;
            in[i].l = in[i].r = static_cast<int16_t>(v*32767.f);
            phase += 440.f/32000.f;
        }
        std::memset(out, 0, sizeof(out));

        proc.Prepare();
        proc.Process(in, out, BS);

        float mx = 0;
        int nonzero = 0;
        for (int i = 0; i < BS; ++i) {
            if (out[i].l != 0 || out[i].r != 0) nonzero++;
            float a = std::abs((float)out[i].l);
            if (a > mx) mx = a;
        }
        std::cout << "  Stretch block " << b 
                  << " max=" << mx << " nonzero=" << nonzero << std::endl;
    }

    // ===== テスト 4: dry_wet を変えてみる =====
    std::cout << "\n=== Test 4: dry_wet=0.5 ===" << std::endl;
    p->dry_wet = 0.5f;
    for (int b = 0; b < 20; ++b) {
        clouds::ShortFrame in[BS], out[BS];
        for (int i = 0; i < BS; ++i) {
            float v = std::sin(2.f*3.14159265f*phase)*0.5f;
            in[i].l = in[i].r = static_cast<int16_t>(v*32767.f);
            phase += 440.f/32000.f;
        }
        std::memset(out, 0, sizeof(out));
        proc.Prepare();
        proc.Process(in, out, BS);
        float mx = 0;
        for (int i = 0; i < BS; ++i) {
            float a = std::abs((float)out[i].l);
            if (a > mx) mx = a;
        }
        if (b < 5 || mx > 0)
            std::cout << "  dry_wet=0.5 block " << b << " max=" << mx << std::endl;
    }

    // ===== テスト 5: Granular に戻して音が出るか確認 =====
    std::cout << "\n=== Test 5: Back to Granular ===" << std::endl;
    proc.set_playback_mode(clouds::PLAYBACK_MODE_GRANULAR);
    p->dry_wet = 1.0f;
    for (int b = 0; b < 20; ++b) {
        clouds::ShortFrame in[BS], out[BS];
        for (int i = 0; i < BS; ++i) {
            float v = std::sin(2.f*3.14159265f*phase)*0.5f;
            in[i].l = in[i].r = static_cast<int16_t>(v*32767.f);
            phase += 440.f/32000.f;
        }
        std::memset(out, 0, sizeof(out));
        proc.Prepare();
        proc.Process(in, out, BS);
        float mx = 0;
        for (int i = 0; i < BS; ++i) {
            float a = std::abs((float)out[i].l);
            if (a > mx) mx = a;
        }
        std::cout << "  Granular block " << b << " max=" << mx << std::endl;
    }

    std::cout << "\nAll tests done." << std::endl;
    return 0;
}

# CloudsVST — アーキテクチャ設計書

## 1\. 全体構成

```
CloudsVST/
├── CMakeLists.txt                    # トップレベル CMake
├── libs/
│   ├── eurorack/                     # git submodule: pichenettes/eurorack
│   └── stmlib/                       # git submodule: pichenettes/stmlib
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   ├── PluginEditor.cpp
│   ├── Parameters.h                  # APVTS パラメータ定義
│   ├── CloudsEngine.h               # DSP ラッパー（核心部）
│   ├── CloudsEngine.cpp
│   ├── SampleRateAdapter.h          # ホスト SR ↔ 内部 SR 変換
│   ├── SampleRateAdapter.cpp
│   └── stmlib_compat.h              # stmlib ARM 依存コードの PC 互換シム
├── Tests/
│   ├── PluginTests.cpp
│   └── AudioProcessingTests.cpp
└── Resources/
    └── (将来: GUI 画像等)
```

## 2\. クラス設計

### 2.1 PluginProcessor (juce::AudioProcessor)

責務: JUCE ホスト連携、パラメータ管理、オーディオコールバック

```cpp
Copyclass CloudsVSTProcessor : public juce::AudioProcessor
{
public:
    CloudsVSTProcessor();
    ~CloudsVSTProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // 状態保存/復元
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts_; }

private:
    juce::AudioProcessorValueTreeState apvts_;
    CloudsEngine engine_;
    SampleRateAdapter srcAdapter_;

    // パラメータポインタ（processBlock 内で atomic load）
    std::atomic<float>* positionParam_  = nullptr;
    std::atomic<float>* sizeParam_      = nullptr;
    std::atomic<float>* pitchParam_     = nullptr;
    std::atomic<float>* densityParam_   = nullptr;
    std::atomic<float>* textureParam_   = nullptr;
    std::atomic<float>* dryWetParam_    = nullptr;
    std::atomic<float>* spreadParam_    = nullptr;
    std::atomic<float>* feedbackParam_  = nullptr;
    std::atomic<float>* reverbParam_    = nullptr;
    std::atomic<float>* freezeParam_    = nullptr;
    std::atomic<float>* triggerParam_   = nullptr;
    std::atomic<float>* modeParam_      = nullptr;
    std::atomic<float>* qualityParam_   = nullptr;
    std::atomic<float>* inputGainParam_ = nullptr;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsVSTProcessor)
};
Copy
```

### 2.2 CloudsEngine（DSP ラッパー）

責務: `clouds::GranularProcessor` の生成・設定・呼出しをカプセル化

```cpp
Copyclass CloudsEngine
{
public:
    CloudsEngine();
    ~CloudsEngine();

    void init();  // バッファ確保、GranularProcessor::Init 呼出し
    void process(const float* inputL, const float* inputR,
                 float* outputL, float* outputR,
                 int numSamples);

    // パラメータ設定（processBlock の冒頭で呼ぶ）
    void setPosition(float v);
    void setSize(float v);
    void setPitch(float v);
    void setDensity(float v);
    void setTexture(float v);
    void setDryWet(float v);
    void setStereoSpread(float v);
    void setFeedback(float v);
    void setReverb(float v);
    void setFreeze(bool v);
    void setTrigger(bool v);
    void setPlaybackMode(int mode);  // 0-3
    void setQuality(int quality);    // 0-3

private:
    static constexpr size_t kLargeBufferSize = 118784;
    static constexpr size_t kSmallBufferSize = 65535;
    static constexpr size_t kBlockSize = 32;  // clouds::kMaxBlockSize

    std::unique_ptr<uint8_t[]> largeBuffer_;
    std::unique_ptr<uint8_t[]> smallBuffer_;
    clouds::GranularProcessor processor_;

    // 内部ワークバッファ（kBlockSize 単位処理用）
    clouds::ShortFrame inputFrames_[kBlockSize];
    clouds::ShortFrame outputFrames_[kBlockSize];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CloudsEngine)
};
Copy
```

設計ポイント:

-   Clouds 内部は 32kHz/32サンプル単位で処理する。ホスト側のバッファサイズに関わらず、 SampleRateAdapter が 32 サンプルブロック単位に分割して CloudsEngine::process を呼ぶ。
-   `GranularProcessor::Process()` は ShortFrame (int16) で I/O する。 float→int16 変換は CloudsEngine 内部で行う。

### 2.3 SampleRateAdapter

責務: ホストサンプルレート ↔ Clouds 内部 32kHz の変換

```cpp
Copyclass SampleRateAdapter
{
public:
    void prepare(double hostSampleRate, int maxBlockSize);
    void process(const float* inL, const float* inR,
                 float* outL, float* outR,
                 int numSamples,
                 CloudsEngine& engine);

private:
    double hostSampleRate_ = 44100.0;
    double ratio_ = 1.0;  // hostSR / 32000.0

    // リサンプリング用 FIFO
    juce::AudioBuffer<float> downsampledInput_;
    juce::AudioBuffer<float> upsampledOutput_;

    // juce::LagrangeInterpolator もしくは線形補間
    juce::LagrangeInterpolator interpDownL_, interpDownR_;
    juce::LagrangeInterpolator interpUpL_, interpUpR_;
};
```

### 2.4 stmlib\_compat.h

責務: ARM 固有コード（`__asm ssat`, `__asm vsqrt`）を標準 C++ に置換

```cpp
Copy// stmlib の dsp.h における ARM アセンブリ命令を PC 向けに置換
// オリジナルの stmlib/dsp/dsp.h は #ifdef TEST 分岐で対応済みだが、
// ビルド時に TEST マクロを定義して ARM コードを回避する。
//
// 方針: CMakeLists.txt にて target_compile_definitions に TEST を追加
// これにより stmlib 内の Clip16, ClipU16, Sqrt が標準 C++ 版になる。
```

実際の対処: CMake で `-DTEST` を定義するだけでよい。stmlib は既に `#ifdef TEST` ガードで ARM intrinsics を回避するコードパスを持っている。

### 2.5 clouds/drivers/debug\_pin.h の除去

`granular_processor.cc` が `#include "clouds/drivers/debug_pin.h"` しているが、 これは STM32 の GPIO デバッグピン操作。空の TIC/TOC マクロを定義したスタブヘッダで置換:

```cpp
Copy// Source/stubs/clouds/drivers/debug_pin.h
#ifndef CLOUDS_DRIVERS_DEBUG_PIN_H_
#define CLOUDS_DRIVERS_DEBUG_PIN_H_
#define TIC
#define TOC
#endif
```

## 3\. ビルドシステム (CMake)

```cmake
Copycmake_minimum_required(VERSION 3.22)
project(CloudsVST VERSION 1.0.0)

# JUCE
add_subdirectory(JUCE)  # FetchContent or submodule

# stmlib / eurorack ソースを直接コンパイル
# TEST マクロで ARM intrinsics を回避
set(CLOUDS_DSP_SOURCES
    libs/eurorack/clouds/dsp/granular_processor.cc
    libs/eurorack/clouds/dsp/correlator.cc
    libs/eurorack/clouds/dsp/mu_law.cc
    libs/eurorack/clouds/dsp/pvoc/frame_transformation.cc
    libs/eurorack/clouds/dsp/pvoc/phase_vocoder.cc
    libs/eurorack/clouds/dsp/pvoc/stacking_order.cc
    libs/eurorack/clouds/resources.cc
)

juce_add_plugin(CloudsVST
    COMPANY_NAME "CloudsVST"
    PLUGIN_MANUFACTURER_CODE Clds
    PLUGIN_CODE Cld1
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "CloudsVST"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
)

target_sources(CloudsVST PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/CloudsEngine.cpp
    Source/SampleRateAdapter.cpp
    ${CLOUDS_DSP_SOURCES}
)

target_include_directories(CloudsVST PRIVATE
    Source/
    Source/stubs/              # debug_pin.h スタブ
    libs/eurorack/
    libs/stmlib/              # stmlib ヘッダのルート（stmlib/stmlib.h 等）
    libs/                     # "stmlib/xxx.h" 形式のインクルードパス解決
)

target_compile_definitions(CloudsVST PRIVATE
    TEST=1                    # stmlib ARM intrinsics 回避
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
)

target_compile_features(CloudsVST PRIVATE cxx_std_17)

target_link_libraries(CloudsVST
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
Copy
```

## 4\. processBlock フロー詳細

```
processBlock(buffer, midi) {
    1. APVTS からパラメータ値を読み取り、engine_.setXxx() で設定
    2. 入力に inputGain を適用
    3. srcAdapter_.process(inL, inR, outL, outR, numSamples, engine_) を呼ぶ
       └── 内部:
           a. ホスト SR から 32kHz にダウンサンプリング
           b. 32 サンプルブロック単位で engine_.process() を繰り返し呼ぶ
              └── CloudsEngine::process() 内部:
                  i.   float→int16 変換 → ShortFrame[]
                  ii.  processor_.Prepare()
                  iii. processor_.Process(input, output, kBlockSize)
                  iv.  ShortFrame[] → float 変換
           c. 32kHz からホスト SR にアップサンプリング
    4. 出力を buffer に書き戻す
}
```

## 5\. パラメータとオリジナル Parameters 構造体の対応

| APVTS パラメータ | → clouds::Parameters フィールド |
| --- | --- |
| `position` | `parameters.position` |
| `size` | `parameters.size` |
| `pitch` | `parameters.pitch` (半音→そのまま渡す。内部で SemitonesToRatio) |
| `density` | `parameters.density` |
| `texture` | `parameters.texture` |
| `dry_wet` | `parameters.dry_wet` |
| `stereo_spread` | `parameters.stereo_spread` |
| `feedback` | `parameters.feedback` |
| `reverb` | `parameters.reverb` |
| `freeze` | `parameters.freeze` |
| `trigger` | `parameters.trigger` |
| `playback_mode` | `processor_.set_playback_mode()` |
| `quality` | `processor_.set_quality()` |
| `input_gain` | PluginProcessor 側で入力バッファに適用 |

## 6\. 状態保存/復元 (getStateInformation / setStateInformation)

-   `juce::AudioProcessorValueTreeState::state` を XML シリアライズ
-   Freeze 中のバッファ内容は保存しない（Phase 1）
-   Phase 2 でバッファ保存/復元に対応予定

## 7\. スレッドセーフティ

-   パラメータは APVTS の std::atomic 経由で読み取り → ロック不要
-   `CloudsEngine` はオーディオスレッドからのみアクセス
-   `setPlaybackMode()` / `setQuality()` はバッファリセットを伴うため、 次の `Prepare()` 呼出し時に反映。切り替え中の無音は `GranularProcessor` が内部処理

## 8\. 移植時の既知の課題と対策

### 8.1 ARM 固有コード

-   `ssat`, `usat` ASM: `-DTEST` で C++ フォールバック使用
-   `vsqrt.f32` ASM: `-DTEST` で `sqrtf()` フォールバック使用

### 8.2 debug\_pin.h

-   STM32 GPIO デバッグ。スタブヘッダで TIC/TOC を空マクロ化

### 8.3 resources.cc

-   ルックアップテーブル（lut\_sin, lut\_window, lut\_xfade\_in/out, lut\_grain\_size 等）
-   `resources.cc` をそのままコンパイル。Python 再生成は不要

### 8.4 サンプルレート差

-   オリジナル: 32kHz 固定（低品質時 16kHz）
-   VST: ホスト SR は可変。`SampleRateAdapter` で 32kHz に変換してから処理
-   `juce::LagrangeInterpolator`（5次）を使用し高品質なリサンプリングを実現

### 8.5 メモリアロケーション

-   Clouds のバッファは `Init()` 時に一度だけ確保
-   `processBlock` 内では一切のヒープアロケーションなし（オリジナル準拠）
-   `prepareToPlay()` で `CloudsEngine::init()` を呼び、バッファを確保/リセット
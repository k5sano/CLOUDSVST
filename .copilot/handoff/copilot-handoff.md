# Claude Code への実装指示書

## プロジェクト概要

Mutable Instruments Clouds を JUCE VST3 プラグインに移植する。 詳細は `spec.md`（仕様）と `design.md`（設計）を参照。

## 実装フェーズ

### Phase 1: プロジェクト骨格 + DSP 統合（今回のスコープ）

以下の順序で実装せよ:

#### Step 1: プロジェクト構造の作成

1.  `CMakeLists.txt` を作成。JUCE は `FetchContent` で取得:
    
    ```cmake
    Copyinclude(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
    )
    FetchContent_MakeAvailable(JUCE)
    ```
    
2.  `libs/` ディレクトリに eurorack と stmlib を git submodule 追加:
    
    ```bash
    Copygit submodule add https://github.com/pichenettes/eurorack.git libs/eurorack
    git submodule add https://github.com/pichenettes/stmlib.git libs/stmlib
    ```
    
3.  `Source/stubs/clouds/drivers/debug_pin.h` スタブファイルを作成
4.  全ソースディレクトリを作成

#### Step 2: stmlib\_compat — ARM コードの PC 互換化

1.  CMake に `target_compile_definitions(CloudsVST PRIVATE TEST=1)` を追加
2.  これだけで stmlib の ARM asm が回避される。追加コード不要
3.  ビルドして stmlib + clouds DSP ソースがコンパイルできることを確認

#### Step 3: CloudsEngine の実装

1.  `design.md` の CloudsEngine クラス設計に従って実装
2.  `init()` で large\_buffer(118784B) と small\_buffer(65535B) をヒープ確保
3.  `process()` で:
    -   float→int16 変換（`* 32768.0f` → clip）
    -   `processor_.Prepare()` 呼出し
    -   `processor_.Process(inputFrames_, outputFrames_, kBlockSize)` 呼出し
    -   int16→float 変換（`/ 32768.0f`）
4.  各 setXxx メソッドで `processor_.mutable_parameters()->xxx = v` を設定

#### Step 4: SampleRateAdapter の実装

1.  `juce::LagrangeInterpolator` を使用（L/R × Up/Down = 4インスタンス）
2.  `prepare()`: `ratio_ = hostSampleRate / 32000.0` 計算、内部バッファ確保
3.  `process()`:
    -   入力をダウンサンプリング（hostSR → 32kHz）
    -   32 サンプルずつ `engine.process()` を呼ぶループ
    -   出力をアップサンプリング（32kHz → hostSR）

#### Step 5: PluginProcessor の実装

1.  `Parameters.h` で APVTS パラメータレイアウトを定義（spec.md 準拠）
2.  `prepareToPlay()` で `engine_.init()` と `srcAdapter_.prepare()` を呼ぶ
3.  `processBlock()` で:
    -   パラメータ読み取り → `engine_.setXxx()` 呼出し
    -   入力ゲイン適用
    -   `srcAdapter_.process()` 呼出し
4.  `getStateInformation()` / `setStateInformation()` を APVTS ベースで実装

#### Step 6: PluginEditor の実装（最小構成）

1.  600×400 px のウィンドウ
2.  以下の GUI コンポーネントを配置:
    -   Position, Size, Pitch, Density, Texture: ロータリーノブ（上段）
    -   Dry/Wet, Spread, Feedback, Reverb: ロータリーノブ（中段）
    -   Mode: 4ボタンセレクタ
    -   Quality: 4ボタンセレクタ
    -   Freeze: トグルボタン
    -   Trigger: モメンタリボタン
    -   Input Gain: 縦スライダー
3.  全て `juce::AudioProcessorValueTreeState::SliderAttachment` / `ButtonAttachment` で接続

#### Step 7: テスト

1.  ビルドテスト: `cmake --build build/` が通ること
2.  ユニットテスト: パラメータのレンジとデフォルト値を検証
3.  オーディオテスト:
    -   無音入力+DryWet=0 → 無音出力
    -   無音入力+DryWet=1+Freeze=OFF → ほぼ無音出力
    -   各サンプルレート(44100, 48000, 96000)でクラッシュしないこと
    -   各バッファサイズ(64, 128, 256, 512, 1024)でクラッシュしないこと

## 重要な制約

-   **Clouds DSP コードは極力変更しない**。変更が必要な場合は最小限に留め、 変更理由をコミットメッセージに明記すること
-   **processBlock 内でメモリアロケーション禁止**
-   `-DTEST` で ARM 固有コードは回避できる。stmlib を書き換えない
-   resources.cc はオリジナルをそのまま使う。Python 再生成しない
-   `clouds/drivers/` 以下のハードウェアドライバは一切使用しない。 スタブヘッダで置換する

## コミット規約

-   `feat: 機能追加の説明`
-   `fix: バグ修正の説明`
-   `refactor: リファクタリングの説明`
-   `chore: ビルド設定等の変更`
-   1 Step = 1 コミット を目安に

## ビルド手順

```bash
Copy# 初回
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build/

# テスト
ctest --test-dir build/

# Release ビルド
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release/
```
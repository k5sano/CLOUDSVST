```markdown
Copy# MetalCosmos 設計書

## クラス構成

```

MT2Plugin : juce::AudioProcessor ├── MT2GainStage │ ├── DiodeFeedbackClipper (×2: stage1, stage2) │ ├── OnePoleHPF (段間) │ └── OnePoleLPF (段間) ├── DiodeMorpher (Is/n の補間ロジック) ├── MT2ToneStack │ ├── BiquadFilter (Low Shelf) │ ├── BiquadFilter (Mid Peak, Q可変) │ └── BiquadFilter (High Shelf) └── juce::dsp::Oversampling

```

## オーディオ処理フロー
1. APVTS から全パラメータを atomic 読み出し
2. 入力バッファを 4x オーバーサンプリング
3. DiodeMorpher で diode_morph (/ diode_morph_2) から Is, n を算出
4. Gain Stage 1: DiodeFeedbackClipper (Newton-Raphson) で非線形処理
5. 段間フィルタ: OnePoleHPF (fc≈200Hz) + OnePoleLPF (fc≈5500Hz)
6. Gain Stage 2: DiodeFeedbackClipper (Newton-Raphson) で非線形処理
7. ダウンサンプリング (Oversampling が自動処理)
8. MT2ToneStack: 3バンド Biquad EQ (係数はブロック先頭で更新)
9. 出力レベル適用

## ファイル構成
```

Source/ PluginProcessor.h PluginProcessor.cpp PluginEditor.h PluginEditor.cpp Parameters.h DSP/ DiodeFeedbackClipper.h DiodeFeedbackClipper.cpp DiodeMorpher.h DiodeMorpher.cpp MT2GainStage.h MT2GainStage.cpp OnePoleFilter.h OnePoleFilter.cpp BiquadFilter.h BiquadFilter.cpp MT2ToneStack.h MT2ToneStack.cpp CMakeLists.txt tests/ TestMain.cpp DiodeClipperTest.cpp DiodeMorpherTest.cpp ToneStackTest.cpp ParameterTest.cpp

```

## モジュール責務

### DiodeFeedbackClipper
ダイオード方程式 `Id(V) = 2·Is·sinh(V/(n·VT))` を Newton-Raphson 法でサンプルごとに解く。前サンプルの出力を初期推定値に使い 3〜4 回の反復で収束させる。

### DiodeMorpher
ノブ値 0.0〜1.0 → 4区間に分割 → 隣接ダイオードモデルの Is, n を線形補間。
No Clip 区間への遷移はクロスフェード。

### MT2GainStage
DiodeFeedbackClipper ×2 + 段間フィルタ (OnePoleHPF + OnePoleLPF) を管理。
DiodeMorpher から受け取った Is, n を各 Clipper に渡す。

### OnePoleFilter
1次 IIR フィルタ。HPF / LPF をコンストラクタまたはモードで切り替え。

### BiquadFilter
RBJ Audio EQ Cookbook 準拠の Biquad。Direct Form II Transposed 実装。
Low Shelf / Mid Peak / High Shelf の係数計算メソッドを持つ。

### MT2ToneStack
3 つの BiquadFilter を管理。ブロック先頭で係数を更新し、サンプルごとにフィルタ処理。

### MT2Plugin (PluginProcessor)
APVTS による全パラメータ管理、Oversampling、全 DSP モジュールのオーケストレーション。
```
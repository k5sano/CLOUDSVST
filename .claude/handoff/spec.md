```markdown
Copy# MetalCosmos VST3 プラグイン — 要件仕様書

## 1. 製品概要
BOSS MT-2 Metal Zone の回路を DSP モデリングした VST3 ディストーションプラグイン。

## 2. シグナルチェーン
```

Input │ ├─ \[入力バッファ\] ├─ \[Gain Stage 1\] ── ダイオード・フィードバック・クリッピング (ダイオード種別可変) ├─ \[段間フィルタ\] ── HPF ~200Hz / LPF ~5.5kHz ├─ \[Gain Stage 2\] ── ダイオード・フィードバック・クリッピング (ダイオード種別可変) ├─ \[Parametric EQ\] ── 3バンド (Low Shelf / Mid Peak / High Shelf) + Qコントロール ├─ \[出力レベル\] └─ Output

```

## 3. パラメータ一覧 (全10パラメータ)

| パラメータ名 | ID | 範囲 | デフォルト | 説明 |
|---|---|---|---|---|
| Dist | `dist` | 0.0〜1.0 | 0.5 | ゲイン量。対数マッピングで実効ゲイン 5.6〜200 |
| Level | `level` | 0.0〜1.0 | 0.5 | 出力レベル |
| Diode Morph | `diode_morph` | 0.0〜1.0 | 0.0 | Si→Ge→LED→Schottky→NoClip を連続スイープ |
| Diode Link | `diode_link` | bool | true | true: 両ステージ同じダイオード / false: Stage2は別ノブ |
| Diode Morph 2 | `diode_morph_2` | 0.0〜1.0 | 0.0 | Link=false時のみ有効。Stage2用 |
| Low | `eq_low` | 0.0〜1.0 | 0.5 | Low Shelf ±15dB |
| Mid Level | `eq_mid` | 0.0〜1.0 | 0.5 | Mid Peak ±20dB |
| Mid Freq | `eq_mid_freq` | 0.0〜1.0 | 0.5 | 200Hz〜5kHz 対数スイープ |
| Mid Q | `eq_mid_q` | 0.0〜1.0 | 0.3 | Q値 0.3〜10.0 対数マッピング |
| High | `eq_high` | 0.0〜1.0 | 0.5 | High Shelf ±15dB |

## 4. ダイオードモデル定義

| モデル名 | Is | n | 音の傾向 |
|---|---|---|---|
| Silicon (MT-2 stock) | 2.52e-9 | 1.7 | 標準、ジリジリ |
| Germanium | 2.2e-8 | 1.05 | 柔らかい、ヴィンテージファズ寄り |
| LED | 4.35e-10 | 1.9 | 硬い、ヘッドルーム大、クランチ |
| Schottky | 7.4e-9 | 1.9 | 攻撃的、エッジが立つ |
| No Clip (bypass) | - | - | ダイオード無効 = クリーンブースト |

## 5. Feature A: ダイオード・モーフィング
ノブ値 0.0〜1.0 を4区間に分割し、隣接するモデルの Is と n を線形補間。
No Clip モードではダイオード電流計算をバイパスし `Vout = Vin * Gain`。
NoClip への遷移はクロスフェードで処理。
Stage 1 / Stage 2 はリンク or 独立制御。

## 6. Feature B: パラメトリック EQ Q コントロール
Mid バンドの Q 値をユーザーが操作可能。
マッピング: `Q = 0.3 * (10.0 / 0.3) ^ param`
- param=0.0 → Q=0.3 (ブロード)
- param=0.3 → Q≈1.5 (MT-2ストック相当)
- param=1.0 → Q=10.0 (鋭いレゾナンス)

EQフィルタは Biquad (RBJ Audio EQ Cookbook 準拠)。
Mid PeakバンドのみQ可変。Low Shelf / High Shelf のQは固定 (0.707)。
係数更新はブロック先頭で1回、ブロック内はダイレクトフォームII転置で処理。

## 7. DSP 要件

| 項目 | 仕様 |
|---|---|
| ゲインステージ | オペアンプのフィードバックループ内にアンチパラレル・ダイオード対 (ソフトクリッピング) |
| ダイオード方程式 | Id(V) = 2·Is·sinh(V / (n·VT)) |
| 回路方程式 | Vout + Rf·2·Is·sinh(Vout/(n·VT)) = Vin·Gain (Newton-Raphson法で解く) |
| 段間フィルタ | 1次HPF (fc≈200Hz) + 1次LPF (fc≈5500Hz) |
| オーバーサンプリング | 4x必須 (`juce::dsp::Oversampling`) |
| サンプルレート | 44.1kHz〜192kHz 対応 |
| レイテンシ | `setLatencySamples()` でオーバーサンプリング由来のレイテンシを報告 |

## 8. 技術要件

| 項目 | 仕様 |
|---|---|
| フレームワーク | JUCE 7 or 8 |
| プラグイン形式 | VST3 (AUも可能なら対応) |
| 処理精度 | 内部 double (64bit float) |
| パラメータ管理 | `AudioProcessorValueTreeState` |
| スレッドセーフ | `getRawParameterValue()` のアトミック読み出し。EQ係数更新はオーディオスレッドで |
| GUI | 最初はジェネリックエディタ。後からカスタムUI化 |

## 9. 受け入れ基準
- AC-01: `cmake --build build/` がエラーなしで成功する
- AC-02: `ctest --test-dir build/` が全 PASS する
- AC-03: VST3 プラグインバイナリが生成される
- AC-04: dist パラメータ 0.0 → 1.0 でゲインが 5.6 → 200 の範囲に対応
- AC-05: diode_morph=0.0 で Silicon パラメータ (Is=2.52e-9, n=1.7) が使われる
- AC-06: diode_morph=1.0 で No Clip モード (Vout=Vin*Gain) が動作する
- AC-07: eq_mid_q=0.3 で Q≈1.5 (MT-2ストック相当) になる
- AC-08: 4x オーバーサンプリングが有効で、setLatencySamples が正しく報告される
- AC-09: processBlock 内で動的メモリ確保がない
- AC-10: 全パラメータが APVTS で管理され、ID が仕様通りである

## 10. 将来拡張 (今回未実装、設計時に考慮)
- Power Sag (電池消耗シミュレーション)
- Chaos Feedback (自己共振フィードバック)
- Stereo Doubler (デチューン・ダブラー)
- Gate Sequencer (リズミック・ゲート)
```
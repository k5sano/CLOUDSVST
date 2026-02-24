# CloudsVST — Mutable Instruments Clouds VST3 移植 仕様書

## 1\. 概要

Mutable Instruments Clouds（グラニュラーオーディオプロセッサ）のオープンソース DSP コアを JUCE フレームワークで VST3/AU/Standalone プラグインとして移植する。

-   原作: Émilie Gillet (pichenettes)
-   ソースコード: [https://github.com/pichenettes/eurorack/tree/master/clouds](https://github.com/pichenettes/eurorack/tree/master/clouds)
-   ライセンス: MIT
-   依存ライブラリ: stmlib（同リポジトリ、MIT ライセンス）

## 2\. ターゲット

-   フォーマット: VST3, AU (macOS), Standalone
-   OS: macOS (ARM64/x86\_64), Windows (x64), Linux (x64)
-   DAW: Ableton Live, Logic Pro, Reaper, Cubase, FL Studio 等
-   サンプルレート: 44100, 48000, 96000, 192000 Hz
-   バッファサイズ: 64〜2048 サンプル
-   内部処理サンプルレート: 32000 Hz（オリジナル準拠）。ホストSR との変換は内部 SRC で行う
-   チャンネル: ステレオ入力 → ステレオ出力

## 3\. 処理モード（4種）

| モード ID | 名前 | 説明 |
| --- | --- | --- |
| 0 | GRANULAR | グラニュラープロセッサ（デフォルト）。最大40-60グレインの重畳 |
| 1 | STRETCH | ピッチシフター/タイムストレッチャー。WSOLA ベース |
| 2 | LOOPING\_DELAY | ルーピングディレイ。ピッチシフターポスト処理付き |
| 3 | SPECTRAL | スペクトラルプロセッサ。Phase Vocoder ベースの FFT 処理 |

## 4\. パラメータ一覧

### 4.1 メインパラメータ

| パラメータ ID | 表示名 | 型 | 範囲 | デフォルト | 説明 |
| --- | --- | --- | --- | --- | --- |
| `position` | Position | float | 0.0–1.0 | 0.5 | バッファ内再生位置 |
| `size` | Size | float | 0.0–1.0 | 0.5 | グレインサイズ / ウィンドウサイズ |
| `pitch` | Pitch | float | \-24.0–+24.0 | 0.0 | トランスポーズ（半音単位） |
| `density` | Density | float | 0.0–1.0 | 0.5 | グレイン密度。0.5=無音、右=ランダム、左=定速 |
| `texture` | Texture | float | 0.0–1.0 | 0.5 | エンベロープ形状。0.75以上でディフューザー起動 |

### 4.2 ブレンドパラメータ

| パラメータ ID | 表示名 | 型 | 範囲 | デフォルト | 説明 |
| --- | --- | --- | --- | --- | --- |
| `dry_wet` | Dry/Wet | float | 0.0–1.0 | 0.5 | ドライ/ウェット比 |
| `stereo_spread` | Stereo Spread | float | 0.0–1.0 | 0.0 | ランダムパンニング量 |
| `feedback` | Feedback | float | 0.0–1.0 | 0.0 | フィードバック量 |
| `reverb` | Reverb | float | 0.0–1.0 | 0.0 | リバーブ量 |

### 4.3 コントロールパラメータ

| パラメータ ID | 表示名 | 型 | 範囲 | デフォルト | 説明 |
| --- | --- | --- | --- | --- | --- |
| `freeze` | Freeze | bool | OFF/ON | OFF | バッファ録音停止 |
| `trigger` | Trigger | bool | OFF/ON | OFF | 単一グレイン発生（モメンタリ） |
| `playback_mode` | Mode | choice | 0–3 | 0 | 処理モード選択 |
| `quality` | Quality | choice | 0–3 | 0 | 音質設定（下記参照） |
| `input_gain` | Input Gain | float | \-18.0–+6.0 dB | 0.0 | 入力ゲイン |

### 4.4 音質設定

| quality 値 | 解像度 | チャンネル | バッファ長（概算） |
| --- | --- | --- | --- |
| 0 | 16bit | Stereo | ~1秒 |
| 1 | 16bit | Mono | ~2秒 |
| 2 | 8bit µ-law | Stereo | ~2秒 |
| 3 | 8bit µ-law | Mono | ~4秒 |

## 5\. 内部バッファ

-   large\_buffer: 118784 bytes（オリジナル STM32F4 の CCM RAM 相当）
-   small\_buffer: 65536-1 bytes（オリジナル SRAM 相当）
-   VST 版ではヒープから確保。サイズは拡大可能（オプション: ×2, ×4, ×8）

## 6\. シグナルフロー

```
ステレオ入力
    ↓
Input Gain（-18dB 〜 +6dB）
    ↓
ホスト SR → 内部 SR (32kHz) ダウンサンプリング（SRC）
    ↓
フィードバック混合（HP フィルタ付き、フリーズ時は減衰）
    ↓
[低品質時: 32kHz → 16kHz ダウンサンプリング]
    ↓
モード別処理（GRANULAR / STRETCH / LOOPING_DELAY / SPECTRAL）
    ↓
[低品質時: 16kHz → 32kHz アップサンプリング]
    ↓
ポスト処理（Diffuser, Pitch Shifter, LP/HP フィルタ）
    ↓
フィードバックタップ
    ↓
リバーブ
    ↓
Dry/Wet ミックス（クロスフェードカーブ使用）
    ↓
内部 SR (32kHz) → ホスト SR アップサンプリング（SRC）
    ↓
ステレオ出力
```

## 7\. GUI 要件（Phase 1: 最小構成）

-   全パラメータのノブ/スライダー
-   モード選択ボタン（4モード LED 風表示）
-   Freeze ボタン（トグル、点灯表示）
-   Trigger ボタン（モメンタリ）
-   Quality 選択
-   入力/出力レベルメーター
-   サイズ: 600×400 px 推奨

## 8\. GUI 要件（Phase 2: 将来拡張）

-   オリジナルパネル風デザイン
-   バッファ波形表示
-   グレイン可視化
-   プリセットマネージャ

## 9\. ライセンスと帰属表示

-   オリジナルコード: MIT ライセンス（Émilie Gillet）
-   プラグイン内の About / クレジット画面に以下を表示:
    -   "Based on Mutable Instruments Clouds by Émilie Gillet"
    -   "Original source code licensed under MIT License"
    -   GitHub リポジトリへのリンク
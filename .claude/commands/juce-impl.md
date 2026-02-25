```markdown
Copy# /juce-impl — JUCE 実装ルール

Source/ 配下のファイルを編集するときに参照する JUCE コーディングルール。

## 基本規約
- JUCE 8 のコーディングガイドラインに従う
- ヘッダファイル (.h) と実装ファイル (.cpp) を分離
- JUCE module スタイルのインクルードを使う: `#include <juce_audio_processors/juce_audio_processors.h>`
- pragma once をヘッダに使用

## スレッドセーフ
- processBlock は常にオーディオスレッドで呼ばれる前提
- prepareToPlay, releaseResources はメッセージスレッド
- パラメータ読み取りは std::atomic または AudioProcessorValueTreeState 経由
- GUI→DSP の通信は lock-free で行う

## パラメータ
- AudioProcessorValueTreeState で管理
- パラメータID は小文字スネークケース: `"filter_cutoff"`
- NormalisableRange で範囲を定義
- skew factor はUIの操作感に応じて設定

## GUI
- Component 派生クラスで構築
- LookAndFeel は分離して差し替え可能に
- resized() でレイアウト計算
- Timer や AsyncUpdater でDSPパラメータをGUIに反映

## scaffold/ との関係
- scaffold/ にヘッダがあればインターフェースをそのまま実装
- クラス名、関数名、引数を変更しない
- design.md のモジュール責務に従う

## サイズ目安
- 1関数 50行以内
- 1ファイル 300行以内
- 超えたら分割を検討するが、勝手にやらず報告
Copy
```
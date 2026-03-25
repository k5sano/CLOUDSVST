# CloudsVST — Mutable Instruments Clouds JUCE Plugin

## プロジェクト概要

Mutable Instruments Clouds のグラニュラープロセッサを JUCE VST3/AU/Standalone プラグインとして移植したもの。
eurorack リポジトリの `clouds::GranularProcessor` をそのまま利用し、SampleRateAdapter でホストのサンプルレートと内部 32kHz を変換する。

## アーキテクチャ

- **PluginProcessor** — APVTS パラメータ管理、CloudsEngine 呼び出し、メータリング、リミッター
- **PluginEditor** — GUI（ノブ、メーター、Lissajous表示、プリセット管理）
- **CloudsEngine** — `clouds::GranularProcessor` のラッパー。32サンプルブロック処理、パラメータスムージング
- **SampleRateAdapter** — Hermite補間によるリサンプリング（ホスト ↔ 32kHz）
- **Parameters.h** — 全パラメータ定義（18個）

## ビルド

```bash
cmake -B build -G Ninja
cmake --build build --config Release
```

## 注意事項

- eurorack のコードは ARM 向け。x86/x64 ビルドには `Source/stubs/` のスタブと `TEST=1` マクロが必要
- `COPY_PLUGIN_AFTER_BUILD TRUE` によりビルド後自動でDAWプラグインディレクトリにコピーされる
- オーディオスレッド内でのメモリアロケーション禁止

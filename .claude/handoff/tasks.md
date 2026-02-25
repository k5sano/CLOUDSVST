```markdown
Copy# Claude Code 実装タスク

## 優先順位

1. CMakeLists.txt を作成（juce_add_plugin 使用）
2. Parameters.h にパラメータ定義（全10パラメータ）
3. DSP クラス群の実装
   a. OnePoleFilter (HPF/LPF)
   b. BiquadFilter (RBJ Cookbook)
   c. DiodeFeedbackClipper (Newton-Raphson)
   d. DiodeMorpher (Is/n 補間)
   e. MT2GainStage (Clipper×2 + 段間フィルタ)
   f. MT2ToneStack (3バンド EQ)
4. PluginProcessor の実装
5. PluginEditor の実装（最初はジェネリックエディタ、400×300）
6. ビルド確認
7. テストコード作成 (tests/ 配下)

## 注意事項
- processBlock 内で new/delete 禁止
- APVTS は PluginProcessor のコンストラクタで初期化
- エディタサイズは 400×300 で固定（ジェネリックエディタ初期段階）
- 内部処理はすべて double (64bit float)
- オーバーサンプリング 4x は `juce::dsp::Oversampling<double>` を使用

## 完了条件
- `cmake --build build/` が成功する
- `ctest --test-dir build/` が全 PASS する
- VST3 プラグインが生成される
- 全10パラメータが APVTS に登録され仕様通りに動作する
Copy
```
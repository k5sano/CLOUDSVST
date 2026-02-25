```markdown
Copy# /build-check — ビルド＆テスト実行

ビルドとテストを実行し結果をファイルに書き出す。
コードは一切変更しない。

## 実行手順

1. `.claude/fix-counter.json` が残っていたら中身を表示して現状把握
2. ビルド実行:
   ```bash
   cmake -B build 2>&1 | tee .claude/cmake-config.log
   cmake --build build/ 2>&1 | tee .claude/build-output.log
```

1.  ビルド結果の判定:
    -   成功: 「✅ ビルド成功」と報告
    -   失敗: エラー部分を抽出して `.claude/build-errors.txt` に保存
2.  テスト実行（build/ ディレクトリに CTest が設定されている場合）:
    
    ```bash
    Copyctest --test-dir build/ --output-on-failure 2>&1 | tee .claude/test-output.log
    ```
    
3.  結果サマリーを表示:
    
    ```
    Build Check Result
    Config: ✅/❌
    Build:  ✅/❌ (warnings: X)
    Tests:  ✅/❌ (passed: X, failed: Y) / ⏭ not configured
    Errors: (エラー数。0なら「なし」)
    ```
    
4.  エラーがある場合、各エラーについて:
    -   ファイルと行番号
    -   エラーメッセージ
    -   build-fix の🟡ルールに該当しそうか（判定だけ、修正はしない）

## 重要

このコマンドはソースコードを変更しない。 修正の判断と実行は `/build-fix` コマンドに委ねる。

````
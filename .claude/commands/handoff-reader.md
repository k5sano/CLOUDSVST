```markdown
Copy# /handoff-reader — ハンドオフ仕様読み込み

仕様書・設計書・タスクリストを読み込んで構造化サマリーを出力する。

## 読み込み手順
以下のファイルをこの順番で読む:

1. `.claude/handoff/spec.md` — 要件と受け入れ基準
2. `.claude/handoff/design.md` — アーキテクチャとモジュール設計
3. `.claude/handoff/tasks.md` — 実装タスクリスト
4. `scaffold/` 配下の全ファイル — ヘッダ定義、テストスタブ

## 出力フォーマット
読み終わったら以下のサマリーを出力:

```

Handoff Summary Feature: (tasks.md のタイトルから) Tasks: (タスク数) Key Components: (JUCEコンポーネント名: Processor, Editor, DSP等) Acceptance Criteria: (spec.md の基準数) New Dependencies: (あれば。なければ「なし」) Ready to implement: yes / no

```

## エラー処理
ハンドオフファイルが1つでも見つからなければ:
- どのファイルが見つからないか明示
- 「ハンドオフファイルを確認してください」と表示
- 処理を停止
```
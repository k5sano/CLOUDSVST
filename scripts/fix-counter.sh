```bash
Copy#!/bin/bash
# fix-counter.sh
# Claude Code の hook から呼ばれる。
# Source/ 配下のファイル編集回数を記録し、3回到達で自動ロールバック。

set -eo pipefail

INPUT=$(cat)
TOOL_NAME=$(echo "$INPUT" | jq -r '.toolName // empty')
RESULT_TYPE=$(echo "$INPUT" | jq -r '.toolResult.resultType // empty')

# edit または create が成功した場合のみカウント
if [[ "$TOOL_NAME" != "edit" && "$TOOL_NAME" != "create" ]]; then
  exit 0
fi
if [[ "$RESULT_TYPE" != "success" ]]; then
  exit 0
fi

# 対象ファイルのパスを取得
FILE_PATH=$(echo "$INPUT" | jq -r '.toolArgs' | jq -r '.path // empty' 2>/dev/null)
if [[ -z "$FILE_PATH" ]]; then
  exit 0
fi

# Source/ 配下のファイルのみ追跡
if [[ "$FILE_PATH" != Source/* && "$FILE_PATH" != Source/DSP/* ]]; then
  exit 0
fi

COUNTER_FILE=".claude/fix-counter.json"
mkdir -p .claude

# カウンターファイル初期化
if [[ ! -f "$COUNTER_FILE" ]] || [[ ! -s "$COUNTER_FILE" ]]; then
  echo '{}' > "$COUNTER_FILE"
fi

# カウント加算
CURRENT=$(jq -r --arg f "$FILE_PATH" '.[$f] // 0' "$COUNTER_FILE" 2>/dev/null || echo 0)
NEW=$((CURRENT + 1))
jq --arg f "$FILE_PATH" --argjson n "$NEW" '.[$f] = $n' "$COUNTER_FILE" > "${COUNTER_FILE}.tmp" \
  && mv "${COUNTER_FILE}.tmp" "$COUNTER_FILE"

# 3回到達チェック
if [[ $NEW -ge 3 ]]; then
  echo "" >&2
  echo "============================================" >&2
  echo " ⚠️  ROLLBACK: $FILE_PATH" >&2
  echo " ${NEW}回修正しても解決していません。" >&2
  echo " git checkout でHEADに復元します。" >&2
  echo "============================================" >&2
  echo "" >&2

  git checkout HEAD -- "$FILE_PATH" 2>/dev/null || true

  # カウンターリセット
  jq --arg f "$FILE_PATH" '.[$f] = 0' "$COUNTER_FILE" > "${COUNTER_FILE}.tmp" \
    && mv "${COUNTER_FILE}.tmp" "$COUNTER_FILE"

  # エスカレーションレポートに追記
  REPORT=".claude/escalation-report.md"
  {
    echo ""
    echo "## Rollback: $FILE_PATH ($(date '+%Y-%m-%d %H:%M:%S'))"
    echo "- 修正試行回数: $NEW"
    echo "- ファイルをHEADに復元しました"
    echo "- エスカレーションが必要です"
    echo ""
  } >> "$REPORT"

  echo "[HOOK] エスカレーションレポート: $REPORT" >&2
fi
Copy
```
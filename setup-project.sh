#!/bin/bash set -e

echo "=== CloudsVST Project Setup ==="

# Create directory structure

mkdir -p .github/agents mkdir -p .github/skills/{handoff-reader,juce-impl,juce-testing,build-check,juce-builder} mkdir -p .github/hooks mkdir -p .copilot/handoff mkdir -p Source/stubs/clouds/drivers mkdir -p Tests mkdir -p scripts mkdir -p libs

# Init git submodules

if \[ ! -d "libs/eurorack/.git" \]; then echo "Adding eurorack submodule..." git submodule add [https://github.com/pichenettes/eurorack.git](https://github.com/pichenettes/eurorack.git) libs/eurorack 2>/dev/null || true fi

if \[ ! -d "libs/stmlib/.git" \]; then echo "Adding stmlib submodule..." git submodule add [https://github.com/pichenettes/stmlib.git](https://github.com/pichenettes/stmlib.git) libs/stmlib 2>/dev/null || true fi

git submodule update --init --recursive

# Create .gitkeep files

touch .copilot/handoff/.gitkeep

# Make scripts executable

chmod +x scripts/fix-counter.sh

echo "" echo "=== Setup complete ===" echo "" echo "Next steps:" echo " 1. cmake -B build -DCMAKE\_BUILD\_TYPE=Debug" echo " 2. cmake --build build/" echo " 3. ctest --test-dir build/" echo "" echo "Or run with Claude Code:" echo " claude -p 'handoff ファイルを読み込んで実装してください'"
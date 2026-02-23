Copy
# 1. プロジェクトディレクトリを作成
mkdir CloudsVST && cd CloudsVST

# 2. JUCE を配置（サブモジュール or ダウンロード）
git clone https://github.com/juce-framework/JUCE.git

# 3. Mutable Instruments のコードを取得
git clone https://github.com/pichenettes/eurorack.git

# 4. stmlib も取得（eurorack が submodule で参照している）
cd eurorack && git submodule update --init && cd ..
#   ※ stmlib が取れない場合は別途:
#   git clone https://github.com/pichenettes/stmlib.git eurorack/stmlib

# 5. Source/ に上記4ファイルを配置

# 6. ビルド
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release